#include "AudioDSPList.h"
#include "AudioDSPList_p.h"

#include <cstddef>
#include <cstdint>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AudioDSP.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/private/AudioDSP_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Track_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec audioDSPListQuery(Handle parentHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::audioDSPParent(), parentHandle),
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

    AudioDSPListPrivate::AudioDSPListPrivate(AudioDSPList *q, Track *track)
        : q_ptr(q), model(track ? track->model() : nullptr), track(track),
          jsIterable(new JSIterable(q, JSIterable::List)) {
    }

    AudioDSPListPrivate::AudioDSPListPrivate(AudioDSPList *q, Model *model)
        : q_ptr(q), model(model), jsIterable(new JSIterable(q, JSIterable::List)) {
    }

    Handle AudioDSPListPrivate::parentHandle(bool create) const {
        if (!model) {
            return {};
        }
        auto *modelData = ModelPrivate::get(model);
        const auto column = track ? Schema::audioDSPParentTrackColumn()
                                  : Schema::audioDSPParentModelColumn();
        const auto ownerHandle = track ? track->handle() : model->handle();
        const auto view = modelData->engine->query(Schema::audioDSPParentTable(), {
            .filter = orm::equalFilter(dini::FieldRef::column(column),
                                       orm::valueFromHandle(ownerHandle)),
        });
        if (auto snapshot = orm::firstSnapshot(view)) {
            return orm::handleFromId(snapshot->id);
        }
        if (!create) {
            return {};
        }
        const auto id = modelData->requireTransaction()->insert(Schema::audioDSPParentTable(), {
            dini::ColumnValue {
                .column = Schema::audioDSPParentModelColumn(),
                .value = track ? dini::Value::null() : orm::valueFromHandle(ownerHandle),
            },
            dini::ColumnValue {
                .column = Schema::audioDSPParentTrackColumn(),
                .value = track ? orm::valueFromHandle(ownerHandle) : dini::Value::null(),
            },
        });
        return orm::handleFromId(id);
    }

    dini::Value AudioDSPListPrivate::associationValue(bool create) const {
        return orm::valueFromHandle(parentHandle(create));
    }

    void AudioDSPListPrivate::refresh(bool notify, bool itemsChanged) {
        Q_Q(AudioDSPList);
        const auto parent = parentHandle(false);
        auto *modelData = model ? ModelPrivate::get(model) : nullptr;
        const auto newSize = modelData && parent
                                 ? static_cast<int>(modelData->engine
                                                        ->query(Schema::audioDSPList(), audioDSPListQuery(parent))
                                                        .count())
                                 : 0;
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

    AudioDSPList::AudioDSPList(Track *track) : QObject(track), d_ptr(new AudioDSPListPrivate(this, track)) {
    }

    AudioDSPList::AudioDSPList(Model *model) : QObject(model), d_ptr(new AudioDSPListPrivate(this, model)) {
    }

    AudioDSPList::~AudioDSPList() = default;

    int AudioDSPList::size() const {
        return AudioDSPListPrivate::get(this)->size;
    }

    QList<AudioDSP *> AudioDSPList::items() const {
        Q_D(const AudioDSPList);
        const auto parent = d->parentHandle(false);
        if (!d->model || !parent) {
            return {};
        }
        auto *modelData = ModelPrivate::get(d->model);
        return audioDSPsFromView(modelData,
                                 modelData->engine->query(Schema::audioDSPList(),
                                                          audioDSPListQuery(parent)));
    }

    bool AudioDSPList::contains(AudioDSP *item) const {
        return item && item->audioDSPList() == this;
    }

    AudioDSP *AudioDSPList::item(int index) const {
        if (index < 0) {
            return nullptr;
        }
        Q_D(const AudioDSPList);
        const auto parent = d->parentHandle(false);
        if (!d->model || !parent) {
            return nullptr;
        }
        auto *modelData = ModelPrivate::get(d->model);
        const auto values = modelData->engine->query(Schema::audioDSPList(), audioDSPListQuery(parent))
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
        Q_D(const AudioDSPList);
        if (!d->model || item->model() != d->model) {
            return false;
        }
        const auto associationValue = d->associationValue(true);
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(d->model)->update(item->handle(),
                                            Schema::audioDSPParent().column(),
                                            associationValue,
                                            dini::AssociationUpdateOptions {.targetIndex = static_cast<std::size_t>(index)});
        return true;
    }

    AudioDSP *AudioDSPList::removeItem(int index) {
        auto *audioDSP = item(index);
        if (!audioDSP) {
            return nullptr;
        }
        ModelPrivate::get(audioDSP->model())->update(audioDSP->handle(),
                                                    Schema::audioDSPParent().column(),
                                                    dini::Value::null());
        return audioDSP;
    }

    bool AudioDSPList::rotate(int leftIndex, int middleIndex, int rightIndex) {
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex) {
            return false;
        }
        Q_D(const AudioDSPList);
        const auto associationValue = d->associationValue(false);
        if (!d->model || associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(d->model)->rotate(Schema::audioDSPList(),
                                            associationValue,
                                            leftIndex,
                                            middleIndex,
                                            rightIndex);
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
        Q_D(AudioDSPList);
        while (size() > 0) {
            removeItem(size() - 1);
        }
        if (!d->model) {
            return;
        }
        for (const auto &source : audioDSPs) {
            if (!source.isObject()) {
                continue;
            }
            auto audioDSP = d->model->createAudioDSP();
            audioDSP->fromOpenDSPX(source.toObject());
            insertItem(size(), audioDSP);
        }
    }

}


#include "moc_AudioDSPList.cpp"
