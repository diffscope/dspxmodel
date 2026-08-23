#include "AudioDSP.h"
#include "AudioDSP_p.h"

#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <stdcorelib/support/json.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AudioDSPList.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/private/AudioDSPList_p.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Track_p.h>

namespace dspx {

    namespace {

        AudioDSPList *audioDSPOwnerFromParentValue(ModelPrivate &model, const dini::Value &value) {
            const auto parentHandle = orm::handleFromValue(value);
            if (!parentHandle || !model.engine->contains(orm::idFromHandle(parentHandle))) {
                return nullptr;
            }
            const auto parent = model.engine->read(orm::idFromHandle(parentHandle));
            if (!orm::isContainer(parent, Schema::audioDSPParentTable())) {
                return nullptr;
            }
            const auto modelHandle = orm::handleFromValue(
                orm::snapshotValue(parent, Schema::audioDSPParentModelColumn()));
            if (modelHandle) {
                return modelHandle == model.modelHandle ? model.audioDSPs : nullptr;
            }
            const auto trackHandle = orm::handleFromValue(
                orm::snapshotValue(parent, Schema::audioDSPParentTrackColumn()));
            if (trackHandle) {
                auto *track = model.ensure<Track>(trackHandle);
                return track ? TrackPrivate::get(track)->audioDSPs : nullptr;
            }
            return nullptr;
        }

        dini::Value valueFromAudioDSPData(const QJsonValue &data) {
            return dini::Value(stdc::JsonValue(conv::jsonFromQJsonValue(data)).toCbor());
        }

        QJsonValue audioDSPDataFromValue(const dini::Value &value) {
            if (value.isNull()) {
                return {};
            }
            return conv::qJsonValueFromJson(stdc::JsonValue::fromCbor(value.asBinary()));
        }

        const std::vector<orm::ColumnBinding<AudioDSP>> &audioDSPColumnBindings() {
            static const std::vector<orm::ColumnBinding<AudioDSP>> bindings {
                orm::stringFieldWithSignal<AudioDSP, AudioDSPPrivate>(Schema::audioDSPIdColumn(), &AudioDSPPrivate::id, &AudioDSP::idChanged),
                {Schema::audioDSPDataColumn(), [](AudioDSP *q, const dini::Value &value) {
                     auto *d = AudioDSPPrivate::get(q);
                     const auto newValue = audioDSPDataFromValue(value);
                     const bool changed = d->data != newValue;
                     d->data = newValue;
                     return changed;
                 }, [](AudioDSP *q) {
                     emit q->dataChanged(AudioDSPPrivate::get(q)->data);
                 }},
                orm::boolFieldWithSignal<AudioDSP, AudioDSPPrivate>(Schema::audioDSPEnabledColumn(), &AudioDSPPrivate::enabled, &AudioDSP::enabledChanged),
                {Schema::audioDSPParent().column(), [](AudioDSP *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = AudioDSPPrivate::get(q);
                     auto *newList = audioDSPOwnerFromParentValue(*model, value);
                     const bool changed = d->list != newList;
                     d->list = newList;
                     return changed;
                 }, [](AudioDSP *q) {
                     emit q->audioDSPListChanged(AudioDSPPrivate::get(q)->list);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        void syncAudioDSPColumns(AudioDSP *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(audioDSPColumnBindings(), item, snapshot, notify);
        }

        bool applyAudioDSPColumn(AudioDSP *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(audioDSPColumnBindings(), item, column, value, notify);
        }

        const ListBinding &audioDSPListBinding() {
            static const ListBinding binding = makeIndexedListBinding<AudioDSP, AudioDSPList>({
                .list = Schema::audioDSPList(),
                .associationColumn = Schema::audioDSPParent().column(),
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<AudioDSP>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<AudioDSP>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.audioDSPObjects.remove(handle); },
                .sync = [](AudioDSP *item, const dini::ItemSnapshot &snapshot, bool notify) { syncAudioDSPColumns(item, snapshot, notify); },
                .applyColumn = [](AudioDSP *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyAudioDSPColumn(item, column, value, notify); },
                .ownerForAssociationValue = [](ModelPrivate &model, const dini::Value &value) {
                    return audioDSPOwnerFromParentValue(model, value);
                },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return snapshot.listAssociationValue.has_value()
                               ? audioDSPOwnerFromParentValue(model, snapshot.listAssociationValue.value())
                               : nullptr;
                },
                .setOwner = [](AudioDSP *item, AudioDSPList *owner, bool notify) { AudioDSPPrivate::get(item)->setAudioDSPList(owner, notify); },
                .refreshOwner = [](AudioDSPList *owner, bool notify, bool itemsChanged) { AudioDSPListPrivate::get(owner)->refresh(notify, itemsChanged); },
                .itemAboutToInsert = [](AudioDSPList *owner, int index, AudioDSP *item, AudioDSPList *) { emit owner->itemAboutToInsert(index, item); },
                .itemInserted = [](AudioDSPList *owner, int index, AudioDSP *item, AudioDSPList *) { emit owner->itemInserted(index, item); },
                .itemAboutToRemove = [](AudioDSPList *owner, int index, AudioDSP *item, AudioDSPList *) { emit owner->itemAboutToRemove(index, item); },
                .itemRemoved = [](AudioDSPList *owner, int index, AudioDSP *item, AudioDSPList *) { emit owner->itemRemoved(index, item); },
                .aboutToRotate = [](AudioDSPList *owner, int left, int middle, int right) { emit owner->aboutToRotate(left, middle, right); },
                .rotated = [](AudioDSPList *owner, int left, int middle, int right) { emit owner->rotated(left, middle, right); },
            });
            return binding;
        }

    }

    AudioDSPPrivate::AudioDSPPrivate(AudioDSP *q) : q_ptr(q) {
    }

    void AudioDSPPrivate::setAudioDSPList(AudioDSPList *newList, bool notify) {
        Q_Q(AudioDSP);
        if (list == newList) {
            return;
        }
        list = newList;
        if (notify) {
            emit q->audioDSPListChanged(list);
        }
    }

    AudioDSP::AudioDSP(Handle handle, Model *model)
        : EntityObject(handle, model, model), d_ptr(new AudioDSPPrivate(this)) {
    }

    AudioDSP::~AudioDSP() = default;

    QString AudioDSP::id() const {
        Q_D(const AudioDSP);
        return d->id;
    }

    void AudioDSP::setId(const QString &id) {
        ModelPrivate::get(model())->update(handle(), Schema::audioDSPIdColumn(), orm::valueFromString(id));
    }

    QJsonValue AudioDSP::data() const {
        Q_D(const AudioDSP);
        return d->data;
    }

    void AudioDSP::setData(const QJsonValue &data) {
        ModelPrivate::get(model())->update(handle(), Schema::audioDSPDataColumn(), valueFromAudioDSPData(data));
    }

    bool AudioDSP::enabled() const {
        Q_D(const AudioDSP);
        return d->enabled;
    }

    void AudioDSP::setEnabled(bool enabled) {
        ModelPrivate::get(model())->update(handle(), Schema::audioDSPEnabledColumn(), dini::Value(enabled));
    }

    AudioDSPList *AudioDSP::audioDSPList() const {
        Q_D(const AudioDSP);
        return d->list;
    }

    stdc::JsonObject AudioDSP::toOpenDSPX() const {
        stdc::JsonObject result;
        result["id"] = id().toStdString();
        result["data"] = conv::jsonFromQJsonValue(data());
        result["enabled"] = enabled();
        return result;
    }

    void AudioDSP::fromOpenDSPX(const stdc::JsonObject &audioDSP) {
        if (auto it = audioDSP.find("id"); it != audioDSP.end() && it->second.isString()) {
            setId(QString::fromStdString(it->second.toString()));
        }
        if (auto it = audioDSP.find("data"); it != audioDSP.end()) {
            setData(conv::qJsonValueFromJson(it->second));
        }
        if (auto it = audioDSP.find("enabled"); it != audioDSP.end() && it->second.isBool()) {
            setEnabled(it->second.toBool());
        }
    }

}


#include "moc_AudioDSP.cpp"
