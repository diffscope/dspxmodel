#include "Singer.h"
#include "Singer_p.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QVariant>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/mixedsinger.h>
#include <opendspx/singlesinger.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/DynamicMixingAnchor_p.h>
#include <dspxmodelORM/private/MixedSinger_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/SingerList_p.h>
#include <dspxmodelORM/private/SingleSinger_p.h>

namespace dspx {

    namespace {

        std::vector<std::uint8_t> bytesFromQByteArray(const QByteArray &bytes) {
            std::vector<std::uint8_t> result;
            result.reserve(static_cast<std::size_t>(bytes.size()));
            for (auto byte : bytes) {
                result.push_back(static_cast<std::uint8_t>(byte));
            }
            return result;
        }

        QByteArray qByteArrayFromBytes(const std::vector<std::uint8_t> &bytes) {
            QByteArray result;
            result.resize(static_cast<int>(bytes.size()));
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                result[static_cast<int>(i)] = static_cast<char>(bytes[i]);
            }
            return result;
        }

        dini::Value valueFromJsonValue(const QJsonValue &value) {
            QByteArray bytes;
            QDataStream stream(&bytes, QIODevice::WriteOnly);
            stream.setVersion(QDataStream::Qt_5_15);
            stream << value.toVariant();
            return dini::Value(bytesFromQByteArray(bytes));
        }

        QJsonValue jsonValueFromValue(const dini::Value &value) {
            if (value.isNull()) {
                return {};
            }
            QVariant variant;
            QDataStream stream(qByteArrayFromBytes(value.asBinary()));
            stream.setVersion(QDataStream::Qt_5_15);
            stream >> variant;
            return QJsonValue::fromVariant(variant);
        }

        SingerList *singerOwnerFromMixableValue(ModelPrivate &model, const dini::Value &value) {
            const auto mixableHandle = orm::handleFromValue(value);
            if (!mixableHandle || !model.engine->contains(orm::idFromHandle(mixableHandle))) {
                return nullptr;
            }
            const auto mixable = model.engine->read(orm::idFromHandle(mixableHandle));
            if (!orm::isContainer(mixable, Schema::mixableTable())) {
                return nullptr;
            }
            const auto sourcesHandle = orm::handleFromValue(orm::snapshotValue(mixable, Schema::mixableSourcesColumn()));
            if (sourcesHandle) {
                auto *sources = model.ensure<Sources>(sourcesHandle);
                return sources ? sources->singers() : nullptr;
            }
            const auto mixedSingerHandle = orm::handleFromValue(orm::snapshotValue(mixable, Schema::mixableMixedSingerColumn()));
            if (mixedSingerHandle) {
                auto *mixedSinger = model.ensure<MixedSinger>(mixedSingerHandle);
                return mixedSinger ? mixedSinger->singers() : nullptr;
            }
            return nullptr;
        }

        const std::vector<orm::ColumnBinding<Singer>> &singerColumnBindings() {
            static const std::vector<orm::ColumnBinding<Singer>> bindings {
                {Schema::singerExtraColumn(), [](Singer *q, const dini::Value &value) {
                     auto *d = SingerPrivate::get(q);
                     const auto newValue = jsonValueFromValue(value);
                     const bool changed = d->extra != newValue;
                     d->extra = newValue;
                     return changed;
                 }, [](Singer *q) {
                     emit q->extraChanged(SingerPrivate::get(q)->extra);
                 }, [](Singer *q) {
                     emit q->extraChangedAfterCommit(SingerPrivate::get(q)->extra);
                 }},
                orm::binaryField<Singer, SingerPrivate>(Schema::singerWorkspaceColumn(), &SingerPrivate::workspaceData, nullptr, nullptr),
                {Schema::singerParent().column(), [](Singer *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = SingerPrivate::get(q);
                     auto *newList = singerOwnerFromMixableValue(*model, value);
                     const bool changed = d->singerList != newList;
                     d->singerList = newList;
                     return changed;
                 }, [](Singer *q) {
                     emit q->singerListChanged(SingerPrivate::get(q)->singerList);
                 }, [](Singer *q) {
                     emit q->singerListChangedAfterCommit(SingerPrivate::get(q)->singerList);
                 }},
            };
            return bindings;
        }

        const std::vector<orm::ColumnBinding<SingleSinger>> &singleSingerColumnBindings() {
            static const std::vector<orm::ColumnBinding<SingleSinger>> bindings {
                orm::stringFieldWithSignal<SingleSinger, SingleSingerPrivate>(Schema::singleSingerIdColumn(),
                                                                              &SingleSingerPrivate::id,
                                                                              &SingleSinger::idChanged,
                                                                              &SingleSinger::idChangedAfterCommit),
            };
            return bindings;
        }

        const std::vector<orm::ColumnBinding<MixedSinger>> &mixedSingerColumnBindings() {
            static const std::vector<orm::ColumnBinding<MixedSinger>> bindings {
                {Schema::mixedSingerRatioColumn(), [](MixedSinger *q, const dini::Value &value) {
                     auto *d = MixedSingerPrivate::get(q);
                     const auto newValue = orm::ratioFromValue(value);
                     const bool changed = d->ratio != newValue;
                     d->ratio = newValue;
                     return changed;
                 }, [](MixedSinger *q) {
                     emit q->ratioChanged(MixedSingerPrivate::get(q)->ratio);
                 }, [](MixedSinger *q) {
                     emit q->ratioChangedAfterCommit(MixedSingerPrivate::get(q)->ratio);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const ListBinding &singerListBinding() {
            static const ListBinding binding = makeIndexedListBinding<Singer, SingerList>({
                .list = Schema::singerList(),
                .associationColumn = Schema::singerParent().column(),
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Singer>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Singer>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) {
                    model.singerObjects.remove(handle);
                    model.singleSingerObjects.remove(handle);
                    model.mixedSingerObjects.remove(handle);
                },
                .sync = [](Singer *item, const dini::ItemSnapshot &snapshot, bool notify) { syncSingerColumns(item, snapshot, notify); },
                .applyColumn = [](Singer *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
                    return applySingerColumn(item, column, value, notify);
                },
                .ownerForAssociationValue = [](ModelPrivate &model, const dini::Value &value) {
                    return singerOwnerFromMixableValue(model, value);
                },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return snapshot.listAssociationValue.has_value() ? singerOwnerFromMixableValue(model, snapshot.listAssociationValue.value()) : nullptr;
                },
                .setOwner = [](Singer *item, SingerList *owner, bool notify) { SingerPrivate::get(item)->setSingerList(owner, notify); },
                .ownerChangedAfterCommit = [](Singer *item, SingerList *owner) { emit item->singerListChangedAfterCommit(owner); },
                .refreshOwner = [](SingerList *owner, bool notify, bool itemsChanged) { SingerListPrivate::get(owner)->refresh(notify, itemsChanged); },
                .refreshOwnerAfterCommit = [](SingerList *owner, bool sizeChanged, bool itemsChanged) {
                    if (sizeChanged) emit owner->sizeChangedAfterCommit(owner->size());
                    if (itemsChanged) emit owner->itemsChangedAfterCommit();
                },
                .itemAboutToInsert = [](SingerList *owner, int index, Singer *item, SingerList *movedFrom) { emit owner->itemAboutToInsert(index, item, movedFrom); },
                .itemInserted = [](SingerList *owner, int index, Singer *item, SingerList *movedFrom) { emit owner->itemInserted(index, item, movedFrom); },
                .itemAboutToRemove = [](SingerList *owner, int index, Singer *item, SingerList *movedTo) { emit owner->itemAboutToRemove(index, item, movedTo); },
                .itemRemoved = [](SingerList *owner, int index, Singer *item, SingerList *movedTo) { emit owner->itemRemoved(index, item, movedTo); },
                .aboutToRotate = [](SingerList *owner, int left, int middle, int right) { emit owner->aboutToRotate(left, middle, right); },
                .rotated = [](SingerList *owner, int left, int middle, int right) { emit owner->rotated(left, middle, right); },
                .itemAboutToInsertAfterCommit = [](SingerList *owner, int index, Singer *item, SingerList *movedFrom) { emit owner->itemAboutToInsertAfterCommit(index, item, movedFrom); },
                .itemInsertedAfterCommit = [](SingerList *owner, int index, Singer *item, SingerList *movedFrom) { emit owner->itemInsertedAfterCommit(index, item, movedFrom); },
                .itemAboutToRemoveAfterCommit = [](SingerList *owner, int index, Singer *item, SingerList *movedTo) { emit owner->itemAboutToRemoveAfterCommit(index, item, movedTo); },
                .itemRemovedAfterCommit = [](SingerList *owner, int index, Singer *item, SingerList *movedTo) { emit owner->itemRemovedAfterCommit(index, item, movedTo); },
                .aboutToRotateAfterCommit = [](SingerList *owner, int left, int middle, int right) { emit owner->aboutToRotateAfterCommit(left, middle, right); },
                .rotatedAfterCommit = [](SingerList *owner, int left, int middle, int right) { emit owner->rotatedAfterCommit(left, middle, right); },
            });
            return binding;
        }

        void syncSingleSingerColumns(SingleSinger *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(singleSingerColumnBindings(), item, snapshot, notify);
        }

        bool applySingleSingerColumn(SingleSinger *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(singleSingerColumnBindings(), item, column, value, notify);
        }

        void syncMixedSingerColumns(MixedSinger *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(mixedSingerColumnBindings(), item, snapshot, notify);
        }

        bool applyMixedSingerColumn(MixedSinger *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(mixedSingerColumnBindings(), item, column, value, notify);
        }

        void syncSingerColumns(Singer *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(singerColumnBindings(), item, snapshot, notify);
            if (auto *single = qobject_cast<SingleSinger *>(item)) {
                syncSingleSingerColumns(single, snapshot, notify);
            } else if (auto *mixed = qobject_cast<MixedSinger *>(item)) {
                syncMixedSingerColumns(mixed, snapshot, notify);
            }
        }

        bool applySingerColumn(Singer *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            if (applyColumnBinding(singerColumnBindings(), item, column, value, notify)) {
                return true;
            }
            if (auto *single = qobject_cast<SingleSinger *>(item)) {
                return applySingleSingerColumn(single, column, value, notify);
            }
            if (auto *mixed = qobject_cast<MixedSinger *>(item)) {
                return applyMixedSingerColumn(mixed, column, value, notify);
            }
            return false;
        }

    }

    SingerPrivate::SingerPrivate(Singer *q) : q_ptr(q) {
    }

    void SingerPrivate::setSingerList(SingerList *newList, bool notify) {
        Q_Q(Singer);
        if (singerList == newList) {
            return;
        }
        singerList = newList;
        if (notify) {
            emit q->singerListChanged(singerList);
        }
    }

    dini::ByteArray SingerPrivate::workspace() const {
        return workspaceData;
    }

    void SingerPrivate::setWorkspace(dini::ByteArray workspace) {
        Q_Q(Singer);
        ModelPrivate::get(q->model())->update(q->handle(), Schema::singerWorkspaceColumn(), dini::Value(std::move(workspace)));
    }

    Singer::Singer(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new SingerPrivate(this)) {
    }

    Singer::~Singer() = default;

    Singer::SingerType Singer::type() const {
        Q_D(const Singer);
        return d->type;
    }

    QJsonValue Singer::extra() const {
        Q_D(const Singer);
        return d->extra;
    }

    void Singer::setExtra(const QJsonValue &extra) {
        ModelPrivate::get(model())->update(handle(), Schema::singerExtraColumn(), valueFromJsonValue(extra));
    }

    SingerList *Singer::singerList() const {
        Q_D(const Singer);
        return d->singerList;
    }

    std::shared_ptr<opendspx::Singer> Singer::toOpenDSPX() const {
        switch (type()) {
            case Single:
                if (auto *single = qobject_cast<const SingleSinger *>(this)) {
                    return std::make_shared<opendspx::SingleSinger>(single->toOpenDSPX());
                }
                break;
            case Mixed:
                if (auto *mixed = qobject_cast<const MixedSinger *>(this)) {
                    return std::make_shared<opendspx::MixedSinger>(mixed->toOpenDSPX());
                }
                break;
        }
        Q_UNREACHABLE();
    }

    void Singer::fromOpenDSPX(const std::shared_ptr<opendspx::Singer> &singer) {
        if (!singer) {
            return;
        }
        switch (singer->type) {
            case opendspx::Singer::Type::Single: {
                if (auto *single = qobject_cast<SingleSinger *>(this)) {
                    single->fromOpenDSPX(static_cast<const opendspx::SingleSinger &>(*singer));
                }
                break;
            }
            case opendspx::Singer::Type::Mixed: {
                if (auto *mixed = qobject_cast<MixedSinger *>(this)) {
                    mixed->fromOpenDSPX(static_cast<const opendspx::MixedSinger &>(*singer));
                }
                break;
            }
        }
    }

    void Singer::fromOpenDSPXBase(const opendspx::Singer &singer) {
        Q_D(Singer);
        d->setWorkspace(conv::serializeWorkspace(singer.workspace));
        setExtra(conv::qJsonValueFromJson(singer.extra));
    }

    void Singer::toOpenDSPXBase(opendspx::Singer &singer) const {
        Q_D(const Singer);
        singer.workspace = conv::deserializeWorkspace(d->workspace());
        singer.extra = conv::jsonFromQJsonValue(extra());
    }

    SingleSingerPrivate::SingleSingerPrivate(SingleSinger *q) : q_ptr(q) {
    }

    SingleSinger::SingleSinger(Handle handle, Model *model) : Singer(handle, model), d_ptr(new SingleSingerPrivate(this)) {
        SingerPrivate::get(static_cast<Singer *>(this))->type = Singer::Single;
    }

    SingleSinger::~SingleSinger() = default;

    QString SingleSinger::id() const {
        Q_D(const SingleSinger);
        return d->id;
    }

    void SingleSinger::setId(const QString &id) {
        ModelPrivate::get(model())->update(handle(), Schema::singleSingerIdColumn(), orm::valueFromString(id));
    }

    opendspx::SingleSinger SingleSinger::toOpenDSPX() const {
        opendspx::SingleSinger target;
        toOpenDSPXBase(target);
        target.id = id().toStdString();
        OpenDSPXConversion::convertSingerToOpenDSPX(this, target);
        return target;
    }

    void SingleSinger::fromOpenDSPX(const opendspx::SingleSinger &singer) {
        fromOpenDSPXBase(singer);
        setId(QString::fromStdString(singer.id));
        OpenDSPXConversion::convertSingerFromOpenDSPX(this, singer);
    }

    MixedSingerPrivate::MixedSingerPrivate(MixedSinger *q) : q_ptr(q) {
    }

    MixedSinger::MixedSinger(Handle handle, Model *model) : Singer(handle, model), d_ptr(new MixedSingerPrivate(this)) {
        auto *singerData = SingerPrivate::get(static_cast<Singer *>(this));
        singerData->type = Singer::Mixed;
        Q_D(MixedSinger);
        d->singers = SingerListPrivate::create(this);
        SingerListPrivate::get(d->singers)->refresh(false, false);
    }

    MixedSinger::~MixedSinger() = default;

    QList<double> MixedSinger::ratio() const {
        Q_D(const MixedSinger);
        return d->ratio;
    }

    void MixedSinger::setRatio(const QList<double> &ratio) {
        ModelPrivate::get(model())->update(handle(), Schema::mixedSingerRatioColumn(), orm::valueFromRatio(ratio));
    }

    SingerList *MixedSinger::singers() const {
        Q_D(const MixedSinger);
        return d->singers;
    }

    opendspx::MixedSinger MixedSinger::toOpenDSPX() const {
        opendspx::MixedSinger target;
        toOpenDSPXBase(target);
        target.singers = singers()->toOpenDSPX();
        for (const auto item : ratio()) {
            target.ratio.push_back(item);
        }
        OpenDSPXConversion::convertSingerToOpenDSPX(this, target);
        return target;
    }

    void MixedSinger::fromOpenDSPX(const opendspx::MixedSinger &singer) {
        fromOpenDSPXBase(singer);
        QList<double> targetRatio;
        targetRatio.reserve(static_cast<qsizetype>(singer.ratio.size()));
        for (const auto item : singer.ratio) {
            targetRatio.append(item);
        }
        setRatio(targetRatio);
        singers()->fromOpenDSPX(singer.singers);
        OpenDSPXConversion::convertSingerFromOpenDSPX(this, singer);
    }

}


#include "moc_Singer.cpp"
#include "moc_MixedSinger.cpp"
#include "moc_SingleSinger.cpp"
