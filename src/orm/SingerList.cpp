#include "SingerList.h"
#include "SingerList_p.h"

#include <cstddef>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        QList<Singer *> singersFromView(ModelPrivate *model, const dini::View &view) {
            QList<Singer *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Singer>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    SingerListPrivate::SingerListPrivate(SingerList *q, Sources *sources) : q_ptr(q), sources(sources) {
    }

    SingerListPrivate::SingerListPrivate(SingerList *q, MixedSinger *mixedSinger)
        : q_ptr(q), mixedSinger(mixedSinger) {
    }

    Handle SingerListPrivate::mixableHandle(bool create) const {
        auto *owner = sources ? static_cast<EntityObject *>(sources) : static_cast<EntityObject *>(mixedSinger);
        if (!owner) {
            return {};
        }
        auto *modelData = ModelPrivate::get(owner->model());
        const auto column = sources ? Schema::mixableSourcesColumn() : Schema::mixableMixedSingerColumn();
        const auto view = modelData->engine->query(Schema::mixableTable(), {
            .filter = orm::equalFilter(dini::FieldRef::column(column), orm::valueFromHandle(owner->handle())),
        });
        if (auto snapshot = orm::firstSnapshot(view)) {
            return orm::handleFromId(snapshot->id);
        }
        if (!create) {
            return {};
        }
        const auto id = modelData->requireTransaction()->insert(Schema::mixableTable(), {
            dini::ColumnValue {
                .column = Schema::mixableSourcesColumn(),
                .value = sources ? orm::valueFromHandle(owner->handle()) : dini::Value::null(),
            },
            dini::ColumnValue {
                .column = Schema::mixableMixedSingerColumn(),
                .value = mixedSinger ? orm::valueFromHandle(owner->handle()) : dini::Value::null(),
            },
        });
        return orm::handleFromId(id);
    }

    dini::Value SingerListPrivate::associationValue(bool create) const {
        return orm::valueFromHandle(mixableHandle(create));
    }

    void SingerListPrivate::refresh(bool notify, bool itemsChanged) {
        Q_Q(SingerList);
        auto *owner = sources ? static_cast<EntityObject *>(sources) : static_cast<EntityObject *>(mixedSinger);
        const auto mixable = mixableHandle(false);
        const auto newSize = (!owner || !mixable) ? 0 : static_cast<int>(ModelPrivate::get(owner->model())
                                                                             ->engine
                                                                             ->query(Schema::singerList(), {
                                                                                 .filter = orm::parentFilter(Schema::singerParent(), mixable),
                                                                             })
                                                                             .count());
        const bool sizeChanged = size != newSize;
        size = newSize;
        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (sizeChanged || itemsChanged) {
            emit q->itemsChanged();
        }
    }

    SingerList::SingerList(Sources *sources) : QObject(sources), d_ptr(new SingerListPrivate(this, sources)) {
    }

    SingerList::SingerList(MixedSinger *mixedSinger)
        : QObject(mixedSinger), d_ptr(new SingerListPrivate(this, mixedSinger)) {
    }

    SingerList::~SingerList() = default;

    int SingerList::size() const {
        Q_D(const SingerList);
        return d->size;
    }

    QList<Singer *> SingerList::items() const {
        Q_D(const SingerList);
        auto *owner = sources() ? static_cast<EntityObject *>(sources()) : static_cast<EntityObject *>(mixedSinger());
        const auto mixable = d->mixableHandle(false);
        if (!owner || !mixable) {
            return {};
        }
        auto *modelData = ModelPrivate::get(owner->model());
        return singersFromView(modelData, modelData->engine->query(Schema::singerList(), {
                                          .filter = orm::parentFilter(Schema::singerParent(), mixable),
                                      }));
    }

    bool SingerList::contains(Singer *item) const {
        return item && item->singerList() == this;
    }

    Singer *SingerList::item(int index) const {
        if (index < 0) {
            return nullptr;
        }
        Q_D(const SingerList);
        auto *owner = sources() ? static_cast<EntityObject *>(sources()) : static_cast<EntityObject *>(mixedSinger());
        const auto mixable = d->mixableHandle(false);
        if (!owner || !mixable) {
            return nullptr;
        }
        auto *modelData = ModelPrivate::get(owner->model());
        const auto values = modelData->engine->query(Schema::singerList(), {
                                      .filter = orm::parentFilter(Schema::singerParent(), mixable),
                                  })
                                .offset(static_cast<std::size_t>(index))
                                .limit(1)
                                .toVector();
        if (values.empty()) {
            return nullptr;
        }
        return modelData->ensure<Singer>(values.front());
    }

    bool SingerList::insertItem(int index, Singer *item) {
        if (index < 0 || !item || item->singerList()) {
            return false;
        }
        Q_D(const SingerList);
        auto *owner = sources() ? static_cast<EntityObject *>(sources()) : static_cast<EntityObject *>(mixedSinger());
        if (!owner || item->model() != owner->model()) {
            return false;
        }
        const auto associationValue = d->associationValue(true);
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(owner->model())->update(item->handle(),
                                                  Schema::singerParent().column(),
                                                  associationValue,
                                                  dini::AssociationUpdateOptions {.targetIndex = static_cast<std::size_t>(index)});
        return true;
    }

    bool SingerList::removeItem(int index) {
        auto *singer = item(index);
        if (!singer) {
            return false;
        }
        ModelPrivate::get(singer->model())->update(singer->handle(), Schema::singerParent().column(), dini::Value::null());
        return true;
    }

    bool SingerList::moveItem(int index, SingerList *list, int newIndex) {
        if (newIndex < 0 || !list) {
            return false;
        }
        auto *singer = item(index);
        if (!singer || !contains(singer) || list->contains(singer)) {
            return false;
        }
        auto *targetOwner = list->sources() ? static_cast<EntityObject *>(list->sources()) : static_cast<EntityObject *>(list->mixedSinger());
        if (!targetOwner || targetOwner->model() != singer->model()) {
            return false;
        }
        const auto associationValue = SingerListPrivate::get(list)->associationValue(true);
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(singer->model())->update(singer->handle(),
                                                   Schema::singerParent().column(),
                                                   associationValue,
                                                   dini::AssociationUpdateOptions {.targetIndex = static_cast<std::size_t>(newIndex)});
        return true;
    }

    bool SingerList::rotate(int leftIndex, int middleIndex, int rightIndex) {
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex) {
            return false;
        }
        Q_D(const SingerList);
        auto *owner = sources() ? static_cast<EntityObject *>(sources()) : static_cast<EntityObject *>(mixedSinger());
        const auto associationValue = d->associationValue(false);
        if (!owner || associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(owner->model())->rotate(Schema::singerList(), associationValue, leftIndex, middleIndex, rightIndex);
        return true;
    }

    Sources *SingerList::sources() const {
        Q_D(const SingerList);
        return d->sources;
    }

    MixedSinger *SingerList::mixedSinger() const {
        Q_D(const SingerList);
        return d->mixedSinger;
    }

}


#include "moc_SingerList.cpp"
