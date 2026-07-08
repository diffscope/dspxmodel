#include "DynamicMixingAnchor.h"
#include "DynamicMixingAnchor_p.h"

#include <cstdint>
#include <cstring>
#include <cstddef>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/dynamicmixinganchor.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/private/DynamicMixingAnchorSequence_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        DynamicMixingAnchorSequence *dynamicMixingAnchorOwnerFromValue(ModelPrivate &model, const dini::Value &value) {
            const auto sourcesHandle = orm::handleFromValue(value);
            auto *sources = sourcesHandle ? model.ensure<Sources>(sourcesHandle) : nullptr;
            return sources ? sources->dynamicMixingAnchors() : nullptr;
        }

        const std::vector<orm::ColumnBinding<DynamicMixingAnchor>> &dynamicMixingAnchorColumnBindings() {
            static const std::vector<orm::ColumnBinding<DynamicMixingAnchor>> bindings {
                orm::intFieldWithSignal<DynamicMixingAnchor, DynamicMixingAnchorPrivate>(Schema::dynamicMixingAnchorPositionColumn(),
                                                                                         &DynamicMixingAnchorPrivate::position,
                                                                                         &DynamicMixingAnchor::positionChanged),
                {Schema::dynamicMixingAnchorRatioColumn(), [](DynamicMixingAnchor *q, const dini::Value &value) {
                     auto *d = DynamicMixingAnchorPrivate::get(q);
                     const auto newValue = orm::ratioFromValue(value);
                     const bool changed = d->ratio != newValue;
                     d->ratio = newValue;
                     return changed;
                 }, [](DynamicMixingAnchor *q) {
                     emit q->ratioChanged(DynamicMixingAnchorPrivate::get(q)->ratio);
                 }},
                orm::previousNextFieldWithSignal<DynamicMixingAnchor, DynamicMixingAnchorPrivate>(
                    Schema::dynamicMixingAnchorPreviousItemColumn(),
                    &DynamicMixingAnchorPrivate::previousHandle,
                    &DynamicMixingAnchorPrivate::previous,
                    &DynamicMixingAnchor::previousItemChanged),
                orm::previousNextFieldWithSignal<DynamicMixingAnchor, DynamicMixingAnchorPrivate>(
                    Schema::dynamicMixingAnchorNextItemColumn(),
                    &DynamicMixingAnchorPrivate::nextHandle,
                    &DynamicMixingAnchorPrivate::next,
                    &DynamicMixingAnchor::nextItemChanged),
                {Schema::dynamicMixingAnchorParent().column(), [](DynamicMixingAnchor *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = DynamicMixingAnchorPrivate::get(q);
                     auto *newSequence = dynamicMixingAnchorOwnerFromValue(*model, value);
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](DynamicMixingAnchor *q) {
                     emit q->dynamicMixingAnchorSequenceChanged(DynamicMixingAnchorPrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &dynamicMixingAnchorOrderSpec() {
            static const OrderSpec spec {{Schema::dynamicMixingAnchorPositionColumn()}, false};
            return spec;
        }

        const TableBinding &dynamicMixingAnchorTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<DynamicMixingAnchor, DynamicMixingAnchorSequence>({
                .table = Schema::dynamicMixingAnchorTable(),
                .membershipColumns = {Schema::dynamicMixingAnchorParent().column()},
                .orderColumns = {Schema::dynamicMixingAnchorPositionColumn()},
                .moveSemantics = MoveSemantics::BetweenOwners,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return model.ensure<DynamicMixingAnchor>(snapshot);
                },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<DynamicMixingAnchor>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.dynamicMixingAnchorObjects.remove(handle); },
                .sync = [](DynamicMixingAnchor *item, const dini::ItemSnapshot &snapshot, bool notify) {
                    syncDynamicMixingAnchorColumns(item, snapshot, notify);
                },
                .applyColumn = [](DynamicMixingAnchor *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
                    return applyDynamicMixingAnchorColumn(item, column, value, notify);
                },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return dynamicMixingAnchorOwnerFromValue(model, orm::snapshotValue(snapshot, Schema::dynamicMixingAnchorParent().column()));
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::dynamicMixingAnchorParent().column()) {
                        return dynamicMixingAnchorOwnerFromValue(model, oldValue ? change.oldValue : change.newValue);
                    }
                    auto *anchor = model.find<DynamicMixingAnchor>(orm::handleFromId(change.itemId));
                    return anchor ? anchor->dynamicMixingAnchorSequence() : nullptr;
                },
                .setOwner = [](DynamicMixingAnchor *item, DynamicMixingAnchorSequence *owner, bool notify) {
                    DynamicMixingAnchorPrivate::get(item)->setSequence(owner, notify);
                },
                .refreshOwner = [](DynamicMixingAnchorSequence *owner, bool notify) {
                    DynamicMixingAnchorSequencePrivate::get(owner)->refresh(notify);
                },
                .itemAboutToInsert = [](DynamicMixingAnchorSequence *owner, DynamicMixingAnchor *item, DynamicMixingAnchorSequence *movedFrom) {
                    emit owner->itemAboutToInsert(item, movedFrom);
                },
                .itemInserted = [](DynamicMixingAnchorSequence *owner, DynamicMixingAnchor *item, DynamicMixingAnchorSequence *movedFrom) {
                    emit owner->itemInserted(item, movedFrom);
                },
                .itemAboutToRemove = [](DynamicMixingAnchorSequence *owner, DynamicMixingAnchor *item, DynamicMixingAnchorSequence *movedTo) {
                    emit owner->itemAboutToRemove(item, movedTo);
                },
                .itemRemoved = [](DynamicMixingAnchorSequence *owner, DynamicMixingAnchor *item, DynamicMixingAnchorSequence *movedTo) {
                    emit owner->itemRemoved(item, movedTo);
                },
            });
            return binding;
        }

        void syncDynamicMixingAnchorColumns(DynamicMixingAnchor *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(dynamicMixingAnchorColumnBindings(), item, snapshot, notify);
        }

        bool applyDynamicMixingAnchorColumn(DynamicMixingAnchor *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(dynamicMixingAnchorColumnBindings(), item, column, value, notify);
        }

        dini::Value valueFromRatio(const QList<double> &ratio) {
            dini::ByteArray bytes;
            bytes.resize(static_cast<std::size_t>(ratio.size()) * sizeof(double));
            auto *dst = bytes.data();
            for (const auto item : ratio) {
                std::memcpy(dst, &item, sizeof(double));
                dst += sizeof(double);
            }
            return dini::Value(std::move(bytes));
        }

        QList<double> ratioFromValue(const dini::Value &value) {
            QList<double> result;
            if (value.isNull()) {
                return result;
            }
            const auto &bytes = value.asBinary();
            result.reserve(static_cast<int>(bytes.size() / sizeof(double)));
            for (std::size_t i = 0; i + sizeof(double) <= bytes.size(); i += sizeof(double)) {
                double item = 0.0;
                std::memcpy(&item, bytes.data() + i, sizeof(double));
                result.append(item);
            }
            return result;
        }

    }

    DynamicMixingAnchorPrivate::DynamicMixingAnchorPrivate(DynamicMixingAnchor *q) : q_ptr(q) {
    }

    void DynamicMixingAnchorPrivate::setSequence(DynamicMixingAnchorSequence *newSequence, bool notify) {
        Q_Q(DynamicMixingAnchor);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->dynamicMixingAnchorSequenceChanged(sequence);
        }
    }

    DynamicMixingAnchor::DynamicMixingAnchor(Handle handle, Model *model)
        : EntityObject(handle, model, model), d_ptr(new DynamicMixingAnchorPrivate(this)) {
    }

    DynamicMixingAnchor::~DynamicMixingAnchor() = default;

    int DynamicMixingAnchor::position() const {
        Q_D(const DynamicMixingAnchor);
        return d->position;
    }

    void DynamicMixingAnchor::setPosition(int position) {
        auto *modelData = ModelPrivate::get(model());
        const auto itemId = orm::idFromHandle(handle());
        const auto snapshot = modelData->engine->read(itemId);
        const auto currentPosition = static_cast<int>(orm::snapshotValue(snapshot, Schema::dynamicMixingAnchorPositionColumn()).asInt64());
        if (currentPosition == position) {
            return;
        }
        modelData->update(handle(), Schema::dynamicMixingAnchorPositionColumn(), dini::Value(static_cast<std::int64_t>(position)));
    }

    QList<double> DynamicMixingAnchor::ratio() const {
        Q_D(const DynamicMixingAnchor);
        return d->ratio;
    }

    void DynamicMixingAnchor::setRatio(const QList<double> &ratio) {
        ModelPrivate::get(model())->update(handle(), Schema::dynamicMixingAnchorRatioColumn(), orm::valueFromRatio(ratio));
    }

    DynamicMixingAnchor *DynamicMixingAnchor::previousItem() const {
        Q_D(const DynamicMixingAnchor);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<DynamicMixingAnchor>(d->previousHandle);
        }
        return d->previous;
    }

    DynamicMixingAnchor *DynamicMixingAnchor::nextItem() const {
        Q_D(const DynamicMixingAnchor);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<DynamicMixingAnchor>(d->nextHandle);
        }
        return d->next;
    }

    DynamicMixingAnchorSequence *DynamicMixingAnchor::dynamicMixingAnchorSequence() const {
        Q_D(const DynamicMixingAnchor);
        return d->sequence;
    }

    opendspx::DynamicMixingAnchor DynamicMixingAnchor::toOpenDSPX() const {
        opendspx::DynamicMixingAnchor target;
        target.pos = position();
        for (const auto item : ratio()) {
            target.ratio.push_back(item);
        }
        return target;
    }

    void DynamicMixingAnchor::fromOpenDSPX(const opendspx::DynamicMixingAnchor &anchor) {
        setPosition(anchor.pos);
        QList<double> targetRatio;
        targetRatio.reserve(static_cast<qsizetype>(anchor.ratio.size()));
        for (const auto item : anchor.ratio) {
            targetRatio.append(item);
        }
        setRatio(targetRatio);
    }

}


#include "moc_DynamicMixingAnchor.cpp"
