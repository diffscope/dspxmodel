#include "AudioDSPList.h"
#include "AudioDSPList_p.h"

#include <cstddef>
#include <cstdint>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AudioDSP.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/private/AudioDSP_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Track_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec audioDSPListQuery(Handle trackHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::audioDSPParent(), trackHandle),
            };
        }

        QList<AudioDSP *> audioDSPsFromView(ModelPrivate *model, const dini::View &view) {
            QList<AudioDSP *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<AudioDSP>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    AudioDSPListPrivate::AudioDSPListPrivate(AudioDSPList *q, Track *track) : q_ptr(q), track(track), jsIterable(new JSIterable(q, JSIterable::List)) {
    }

    void AudioDSPListPrivate::refresh(bool notify, bool itemsChanged) {
        Q_Q(AudioDSPList);
        auto *modelData = ModelPrivate::get(track->model());
        const auto view = modelData->engine->query(Schema::audioDSPList(), audioDSPListQuery(track->handle()));
        const auto newSize = static_cast<int>(view.count());
        AudioDSP *newFirst = nullptr;
        AudioDSP *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<AudioDSP>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto last = view.offset(static_cast<std::size_t>(newSize - 1)).limit(1).toVector();
            if (!last.empty()) {
                newLast = modelData->ensure<AudioDSP>(last.front());
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
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (orderChanged) {
            emit q->itemsChanged();
        }
    }

    AudioDSPList::AudioDSPList(Track *track) : QObject(track), d_ptr(new AudioDSPListPrivate(this, track)) {
    }

    AudioDSPList::~AudioDSPList() = default;

    int AudioDSPList::size() const {
        return AudioDSPListPrivate::get(this)->size;
    }

    QList<AudioDSP *> AudioDSPList::items() const {
        auto *modelData = ModelPrivate::get(track()->model());
        return audioDSPsFromView(modelData, modelData->engine->query(Schema::audioDSPList(), audioDSPListQuery(track()->handle())));
    }

    bool AudioDSPList::contains(AudioDSP *item) const {
        return item && item->audioDSPList() == this;
    }

    AudioDSP *AudioDSPList::item(int index) const {
        if (index < 0) {
            return nullptr;
        }
        auto *modelData = ModelPrivate::get(track()->model());
        const auto values = modelData->engine->query(Schema::audioDSPList(), audioDSPListQuery(track()->handle()))
                                .offset(static_cast<std::size_t>(index))
                                .limit(1)
                                .toVector();
        if (values.empty()) {
            return nullptr;
        }
        return modelData->ensure<AudioDSP>(values.front());
    }

    bool AudioDSPList::insertItem(int index, AudioDSP *item) {
        if (index < 0 || !item || item->audioDSPList()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(track()->model());
        modelData->update(item->handle(), Schema::audioDSPParent().column(), orm::valueFromHandle(track()->handle()), dini::AssociationUpdateOptions {.targetIndex = static_cast<std::size_t>(index)});
        return true;
    }

    AudioDSP *AudioDSPList::removeItem(int index) {
        auto *audioDSP = item(index);
        if (!audioDSP) {
            return nullptr;
        }
        ModelPrivate::get(track()->model())->update(audioDSP->handle(), Schema::audioDSPParent().column(), dini::Value::null());
        return audioDSP;
    }

    bool AudioDSPList::rotate(int leftIndex, int middleIndex, int rightIndex) {
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex) {
            return false;
        }
        ModelPrivate::get(track()->model())->rotate(Schema::audioDSPList(), orm::valueFromHandle(track()->handle()), leftIndex, middleIndex, rightIndex);
        return true;
    }

    Track *AudioDSPList::track() const {
        return AudioDSPListPrivate::get(this)->track;
    }

    stdc::JsonArray AudioDSPList::toOpenDSPX() const {
        stdc::JsonArray result;
        result.reserve(static_cast<std::size_t>(size()));
        for (auto audioDSP : items()) {
            result.push_back(audioDSP->toOpenDSPX());
        }
        return result;
    }

    void AudioDSPList::fromOpenDSPX(const stdc::JsonArray &audioDSPs) {
        while (size() > 0) {
            removeItem(size() - 1);
        }
        for (const auto &source : audioDSPs) {
            if (!source.isObject()) {
                continue;
            }
            auto audioDSP = track()->model()->createAudioDSP();
            audioDSP->fromOpenDSPX(source.toObject());
            insertItem(size(), audioDSP);
        }
    }

}


#include "moc_AudioDSPList.cpp"
