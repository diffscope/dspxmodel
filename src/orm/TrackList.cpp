#include "TrackList.h"
#include "TrackList_p.h"

#include <cstddef>
#include <cstdint>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/model.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Track_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec trackListQuery(Handle modelHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::trackParent(), modelHandle),
            };
        }

        QList<Track *> tracksFromView(ModelPrivate *model, const dini::View &view) {
            QList<Track *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Track>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    namespace orm {

        const ListBinding &trackListBinding() {
            static const ListBinding binding = makeIndexedListBinding<Track, TrackList>({
                .list = Schema::trackList(),
                .associationColumn = Schema::trackParent().column(),
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Track>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Track>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.trackObjects.remove(handle); },
                .sync = [](Track *item, const dini::ItemSnapshot &snapshot, bool notify) { syncTrackColumns(item, snapshot, notify); },
                .applyColumn = [](Track *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyTrackColumn(item, column, value, notify); },
                .ownerForAssociationValue = [](ModelPrivate &model, const dini::Value &value) {
                    return model.isModelValue(value) ? model.tracks : nullptr;
                },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return model.isModelValue(orm::snapshotValue(snapshot, Schema::trackParent().column())) ? model.tracks : nullptr;
                },
                .setOwner = [](Track *item, TrackList *owner, bool notify) { TrackPrivate::get(item)->setTrackList(owner, notify); },
                .refreshOwner = [](TrackList *owner, bool notify, bool itemsChanged) { TrackListPrivate::get(owner)->refresh(notify, itemsChanged); },
                .itemAboutToInsert = [](TrackList *owner, int index, Track *item, TrackList *) { emit owner->itemAboutToInsert(index, item); },
                .itemInserted = [](TrackList *owner, int index, Track *item, TrackList *) { emit owner->itemInserted(index, item); },
                .itemAboutToRemove = [](TrackList *owner, int index, Track *item, TrackList *) { emit owner->itemAboutToRemove(index, item); },
                .itemRemoved = [](TrackList *owner, int index, Track *item, TrackList *) { emit owner->itemRemoved(index, item); },
                .aboutToRotate = [](TrackList *owner, int left, int middle, int right) { emit owner->aboutToRotate(left, middle, right); },
                .rotated = [](TrackList *owner, int left, int middle, int right) { emit owner->rotated(left, middle, right); },
            });
            return binding;
        }

    }

    TrackListPrivate::TrackListPrivate(TrackList *q, Model *model) : q_ptr(q), model(model) {
    }

    void TrackListPrivate::refresh(bool notify, bool itemsChanged) {
        auto *modelData = ModelPrivate::get(model);
        const auto view = modelData->engine->query(Schema::trackList(), trackListQuery(modelData->modelHandle));
        const auto newSize = static_cast<int>(view.count());
        Track *newFirst = nullptr;
        Track *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Track>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto last = view.offset(static_cast<std::size_t>(newSize - 1)).limit(1).toVector();
            if (!last.empty()) {
                newLast = modelData->ensure<Track>(last.front());
            }
        }

        const bool sizeChanged = size != newSize;
        const bool orderChanged = first != newFirst || last != newLast || itemsChanged;
        size = newSize;
        first = newFirst;
        last = newLast;

        if (!notify) {
            return;
        }
        auto *q = q_func();
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (orderChanged) {
            emit q->itemsChanged();
        }
    }

    TrackList::TrackList(Model *model) : QObject(model), d_ptr(new TrackListPrivate(this, model)) {
    }

    TrackList::~TrackList() = default;

    int TrackList::size() const {
        return TrackListPrivate::get(this)->size;
    }

    QList<Track *> TrackList::items() const {
        auto *modelData = ModelPrivate::get(model());
        return tracksFromView(modelData, modelData->engine->query(Schema::trackList(), trackListQuery(modelData->modelHandle)));
    }

    bool TrackList::contains(Track *item) const {
        return item && item->trackList() == this;
    }

    Track *TrackList::item(int index) const {
        if (index < 0) {
            return nullptr;
        }
        auto *modelData = ModelPrivate::get(model());
        const auto values = modelData->engine->query(Schema::trackList(), trackListQuery(modelData->modelHandle))
                                .offset(static_cast<std::size_t>(index))
                                .limit(1)
                                .toVector();
        if (values.empty()) {
            return nullptr;
        }
        return modelData->ensure<Track>(values.front());
    }

    bool TrackList::insertItem(int index, Track *item) {
        if (index < 0 || !item || item->trackList()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(model());
        modelData->update(item->handle(), Schema::trackParent().column(), orm::valueFromHandle(modelData->modelHandle), dini::AssociationUpdateOptions {.targetIndex = static_cast<std::size_t>(index)});
        return true;
    }

    bool TrackList::removeItem(int index) {
        auto *track = item(index);
        if (!track) {
            return false;
        }
        ModelPrivate::get(model())->update(track->handle(), Schema::trackParent().column(), dini::Value::null());
        return true;
    }

    bool TrackList::rotate(int leftIndex, int middleIndex, int rightIndex) {
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex) {
            return false;
        }
        ModelPrivate::get(model())->rotate(Schema::trackList(), orm::valueFromHandle(ModelPrivate::get(model())->modelHandle), leftIndex, middleIndex, rightIndex);
        return true;
    }

    Model *TrackList::model() const {
        return TrackListPrivate::get(this)->model;
    }

    std::vector<opendspx::Track> TrackList::toOpenDSPX() const {
        std::vector<opendspx::Track> result;
        result.reserve(static_cast<std::size_t>(size()));
        for (auto track : items()) {
            result.push_back(track->toOpenDSPX());
        }
        return result;
    }

    void TrackList::fromOpenDSPX(const std::vector<opendspx::Track> &tracks) {
        while (size() > 0) {
            removeItem(size() - 1);
        }
        for (const auto &source : tracks) {
            auto track = model()->createTrack();
            track->fromOpenDSPX(source);
            insertItem(size(), track);
        }
    }

}
