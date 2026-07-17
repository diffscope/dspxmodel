#ifndef DSPXMODEL_MODEL_P_H
#define DSPXMODEL_MODEL_P_H

#include <dspxmodelORM/Model.h>

#include <utility>
#include <functional>
#include <vector>

#include <QHash>
#include <QPointer>

#include <dini/types.h>
#include <dini/event.h>
#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/AnchorNode_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/AudioClip_p.h>
#include <dspxmodelORM/private/Clip_p.h>
#include <dspxmodelORM/private/DynamicMixingAnchor_p.h>
#include <dspxmodelORM/private/Label_p.h>
#include <dspxmodelORM/private/KeySignature_p.h>
#include <dspxmodelORM/private/Note_p.h>
#include <dspxmodelORM/private/Parameter_p.h>
#include <dspxmodelORM/private/Phoneme_p.h>
#include <dspxmodelORM/private/Singer_p.h>
#include <dspxmodelORM/private/SingleSinger_p.h>
#include <dspxmodelORM/private/SingingClip_p.h>
#include <dspxmodelORM/private/Sources_p.h>
#include <dspxmodelORM/private/MixedSinger_p.h>
#include <dspxmodelORM/private/Tempo_p.h>
#include <dspxmodelORM/private/TimeSignature_p.h>
#include <dspxmodelORM/private/Track_p.h>
#include <dspxmodelCore/Schema.h>

namespace dini {
    class DocumentEngine;
}

namespace dspx {

    namespace orm {
        struct ListBinding;
        struct TableBinding;
    }

    class ModelPrivate {
        Q_DECLARE_PUBLIC(Model)
    public:
        ModelPrivate(Model *q, Document *document);
        ~ModelPrivate();

        DSPXMODEL_DECLARE_GET(Model)

        void initialize();
        void syncModel(bool notify);
        bool applyModelColumn(const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void refreshContainers(bool notify);
        void handleEvent(const dini::EngineEvent &event);
        void applyChangeSetEvent(const dini::EngineEvent &event);
        void applyAfterCommitEvent(const dini::EngineEvent &event);
        void applyOperation(const dini::ChangeOperation &operation);
        void applyItemInserted(const dini::ItemInsertedChange &change);
        void applyItemRemoved(const dini::ItemSnapshot &snapshot, bool cascade);
        void applyListInserted(const dini::ListInsertedChange &change);
        void applyListRemoved(const dini::ListRemovedChange &change);
        void applyColumnUpdated(const dini::ColumnUpdatedChange &change);
        void applyComputedColumnUpdated(const dini::ComputedColumnUpdatedChange &change);
        void applyListRotated(const dini::ListRotatedChange &change);
        void deferObjectDestruction(QObject *object, std::function<void()> finalize);
        void finalizeDeferredDestructions();
        void clearDeferredDestructions();
        dini::ByteArray workspace() const;
        void setWorkspace(dini::ByteArray workspace);

        bool isModelValue(const dini::Value &value) const;
        dini::Transaction *requireTransaction() const;

        const orm::TableBinding *tableBinding(dini::ContainerId containerId) const;
        const orm::ListBinding *listBinding(dini::ContainerId containerId) const;

        template <typename T>
        QHash<Handle, T *> &objectMap() {
            if constexpr (std::is_same_v<T, Label>) {
                return labelObjects;
            } else if constexpr (std::is_same_v<T, AnchorNode>) {
                return anchorNodeObjects;
            } else if constexpr (std::is_same_v<T, Note>) {
                return noteObjects;
            } else if constexpr (std::is_same_v<T, DynamicMixingAnchor>) {
                return dynamicMixingAnchorObjects;
            } else if constexpr (std::is_same_v<T, Parameter>) {
                return parameterObjects;
            } else if constexpr (std::is_same_v<T, Phoneme>) {
                return phonemeObjects;
            } else if constexpr (std::is_same_v<T, Singer>) {
                return singerObjects;
            } else if constexpr (std::is_same_v<T, SingleSinger>) {
                return singleSingerObjects;
            } else if constexpr (std::is_same_v<T, MixedSinger>) {
                return mixedSingerObjects;
            } else if constexpr (std::is_same_v<T, KeySignature>) {
                return keySignatureObjects;
            } else if constexpr (std::is_same_v<T, Tempo>) {
                return tempoObjects;
            } else if constexpr (std::is_same_v<T, TimeSignature>) {
                return timeSignatureObjects;
            } else if constexpr (std::is_same_v<T, Track>) {
                return trackObjects;
            } else if constexpr (std::is_same_v<T, Clip>) {
                return clipObjects;
            } else if constexpr (std::is_same_v<T, AudioClip>) {
                return audioClipObjects;
            } else if constexpr (std::is_same_v<T, SingingClip>) {
                return singingClipObjects;
            } else if constexpr (std::is_same_v<T, Sources>) {
                return sourcesObjects;
            } else {
                static_assert(std::is_same_v<T, void>);
                return {};
            }
        }

        template <typename T>
        const QHash<Handle, T *> &objectMap() const {
            return const_cast<ModelPrivate *>(this)->objectMap<T>();
        }

        template <typename T>
        static T *createObject(Handle handle, Model *model) {
            if constexpr (std::is_same_v<T, Label>) {
                return LabelPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, AnchorNode>) {
                return AnchorNodePrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Note>) {
                return NotePrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, DynamicMixingAnchor>) {
                return DynamicMixingAnchorPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Parameter>) {
                return ParameterPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Phoneme>) {
                return PhonemePrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, SingleSinger>) {
                return SingleSingerPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, MixedSinger>) {
                return MixedSingerPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, KeySignature>) {
                return KeySignaturePrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Tempo>) {
                return TempoPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, TimeSignature>) {
                return TimeSignaturePrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Track>) {
                return TrackPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, AudioClip>) {
                return AudioClipPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, SingingClip>) {
                return SingingClipPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Sources>) {
                return SourcesPrivate::create(handle, model);
            } else {
                static_assert(std::is_same_v<T, void>);
                return nullptr;
            }
        }

        template <typename T>
        T *ensure(Handle handle) {
            if (!handle || !engine->contains(orm::idFromHandle(handle))) {
                return nullptr;
            }
            if (auto it = objectMap<T>().find(handle); it != objectMap<T>().end()) {
                return it.value();
            }
            return ensure<T>(engine->read(orm::idFromHandle(handle)));
        }

        template <typename T>
        T *ensure(const dini::ItemSnapshot &snapshot) {
            Q_Q(Model);
            if constexpr (std::is_same_v<T, Label>) {
                if (!orm::isContainer(snapshot, Schema::labelTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, AnchorNode>) {
                if (!orm::isContainer(snapshot, Schema::anchorNodeTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Note>) {
                if (!orm::isContainer(snapshot, Schema::noteTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, DynamicMixingAnchor>) {
                if (!orm::isContainer(snapshot, Schema::dynamicMixingAnchorTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Parameter>) {
                if (!orm::isContainer(snapshot, Schema::parameterTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Phoneme>) {
                if (!orm::isContainer(snapshot, Schema::phonemeTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Singer>) {
                if (!orm::isContainer(snapshot, Schema::singerList())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, SingleSinger>) {
                if (!orm::isContainer(snapshot, Schema::singerList()) || !snapshot.variant.has_value() ||
                    snapshot.variant.value() != Schema::singleSingerVariant()) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, MixedSinger>) {
                if (!orm::isContainer(snapshot, Schema::singerList()) || !snapshot.variant.has_value() ||
                    snapshot.variant.value() != Schema::mixedSingerVariant()) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, KeySignature>) {
                if (!orm::isContainer(snapshot, Schema::keySignatureTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Tempo>) {
                if (!orm::isContainer(snapshot, Schema::tempoTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, TimeSignature>) {
                if (!orm::isContainer(snapshot, Schema::timeSignatureTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Track>) {
                if (!orm::isContainer(snapshot, Schema::trackList())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Clip>) {
                if (!orm::isContainer(snapshot, Schema::clipTable())) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, AudioClip>) {
                if (!orm::isContainer(snapshot, Schema::clipTable()) || !snapshot.variant.has_value() ||
                    snapshot.variant.value() != Schema::audioClipVariant()) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, SingingClip>) {
                if (!orm::isContainer(snapshot, Schema::clipTable()) || !snapshot.variant.has_value() ||
                    snapshot.variant.value() != Schema::singingClipVariant()) {
                    return nullptr;
                }
            } else if constexpr (std::is_same_v<T, Sources>) {
                if (!orm::isContainer(snapshot, Schema::sourcesTable())) {
                    return nullptr;
                }
            } else {
                static_assert(std::is_same_v<T, void>);
            }
            const auto handle = orm::handleFromId(snapshot.id);
            if (auto it = objectMap<T>().find(handle); it != objectMap<T>().end()) {
                return it.value();
            }
            if constexpr (std::is_same_v<T, Clip>) {
                Clip *object = nullptr;
                if (snapshot.variant.has_value() && snapshot.variant.value() == Schema::audioClipVariant()) {
                    object = AudioClipPrivate::create(handle, q);
                    audioClipObjects.insert(handle, static_cast<AudioClip *>(object));
                } else if (snapshot.variant.has_value() && snapshot.variant.value() == Schema::singingClipVariant()) {
                    object = SingingClipPrivate::create(handle, q);
                    singingClipObjects.insert(handle, static_cast<SingingClip *>(object));
                }
                if (!object) {
                    return nullptr;
                }
                clipObjects.insert(handle, object);
                orm::syncClipColumns(object, snapshot, false);
                return object;
            } else if constexpr (std::is_same_v<T, Singer>) {
                Singer *object = nullptr;
                if (snapshot.variant.has_value() && snapshot.variant.value() == Schema::singleSingerVariant()) {
                    object = SingleSingerPrivate::create(handle, q);
                    singleSingerObjects.insert(handle, static_cast<SingleSinger *>(object));
                } else if (snapshot.variant.has_value() && snapshot.variant.value() == Schema::mixedSingerVariant()) {
                    object = MixedSingerPrivate::create(handle, q);
                    mixedSingerObjects.insert(handle, static_cast<MixedSinger *>(object));
                }
                if (!object) {
                    return nullptr;
                }
                singerObjects.insert(handle, object);
                orm::syncSingerColumns(object, snapshot, false);
                return object;
            } else {
                if constexpr (std::is_same_v<T, AudioClip> || std::is_same_v<T, SingingClip>) {
                    if (auto it = clipObjects.find(handle); it != clipObjects.end()) {
                        if (auto *object = qobject_cast<T *>(it.value())) {
                            objectMap<T>().insert(handle, object);
                            return object;
                        }
                        return nullptr;
                    }
                }
                if constexpr (std::is_same_v<T, SingleSinger> || std::is_same_v<T, MixedSinger>) {
                    if (auto it = singerObjects.find(handle); it != singerObjects.end()) {
                        if (auto *object = qobject_cast<T *>(it.value())) {
                            objectMap<T>().insert(handle, object);
                            return object;
                        }
                        return nullptr;
                    }
                }
                auto *object = createObject<T>(handle, q);
                objectMap<T>().insert(handle, object);
                if constexpr (std::is_same_v<T, AudioClip> || std::is_same_v<T, SingingClip>) {
                    clipObjects.insert(handle, object);
                }
                if constexpr (std::is_same_v<T, SingleSinger> || std::is_same_v<T, MixedSinger>) {
                    singerObjects.insert(handle, object);
                }
                if constexpr (std::is_same_v<T, Label>) {
                    orm::syncLabelColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, AnchorNode>) {
                    orm::syncAnchorNodeColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, Note>) {
                    orm::syncNoteColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, DynamicMixingAnchor>) {
                    orm::syncDynamicMixingAnchorColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, Parameter>) {
                    orm::syncParameterColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, Phoneme>) {
                    orm::syncPhonemeColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, SingleSinger> || std::is_same_v<T, MixedSinger>) {
                    orm::syncSingerColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, KeySignature>) {
                    orm::syncKeySignatureColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, Tempo>) {
                    orm::syncTempoColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, TimeSignature>) {
                    orm::syncTimeSignatureColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, Track>) {
                    orm::syncTrackColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, AudioClip> || std::is_same_v<T, SingingClip>) {
                    orm::syncClipColumns(object, snapshot, false);
                } else if constexpr (std::is_same_v<T, Sources>) {
                    orm::syncSourcesColumns(object, snapshot, false);
                }
                return object;
            }
        }

        template <typename T>
        T *find(Handle handle) const {
            const auto it = objectMap<T>().find(handle);
            return it == objectMap<T>().end() ? nullptr : it.value();
        }

        void update(Handle handle, dini::ColumnHandle column, const dini::Value &value, const dini::AssociationUpdateOptions &options = {}) {
            requireTransaction()->update(orm::idFromHandle(handle), column, value, options);
        }

        void update(Handle handle, std::vector<dini::ColumnValue> values) {
            requireTransaction()->update(orm::idFromHandle(handle), std::move(values));
        }

        void rotate(dini::ListHandle list, const dini::Value &associationValue, int leftIndex, int middleIndex, int rightIndex) {
            requireTransaction()->rotate(list, associationValue, dini::ListRotation {
                .startIndex = static_cast<std::size_t>(leftIndex),
                .count = static_cast<std::size_t>(rightIndex - leftIndex),
                .offset = static_cast<std::ptrdiff_t>(middleIndex - leftIndex),
            });
        }

        Model *q_ptr = nullptr;
        Document *document = nullptr;
        dini::DocumentEngine *engine = nullptr;
        Handle modelHandle;
        dini::Subscription subscription;
        bool destroying = false;

        struct DeferredDestruction {
            QPointer<QObject> object;
            std::function<void()> finalize;
        };

        std::vector<DeferredDestruction> deferredDestructions;

        LabelSequence *labels = nullptr;
        KeySignatureSequence *keySignatures = nullptr;
        TempoSequence *tempos = nullptr;
        TimeSignatureSequence *timeSignatures = nullptr;
        TrackList *tracks = nullptr;

        std::vector<const orm::TableBinding *> tableBindings;
        std::vector<const orm::ListBinding *> listBindings;
        bool eventSnapshotOverridesActive = false;
        QHash<dini::ItemId, dini::ItemSnapshot> eventSnapshotOverrides;

        QHash<Handle, Label *> labelObjects;
        QHash<Handle, AnchorNode *> anchorNodeObjects;
        QHash<Handle, Note *> noteObjects;
        QHash<Handle, DynamicMixingAnchor *> dynamicMixingAnchorObjects;
        QHash<Handle, Parameter *> parameterObjects;
        QHash<Handle, Phoneme *> phonemeObjects;
        QHash<Handle, Singer *> singerObjects;
        QHash<Handle, SingleSinger *> singleSingerObjects;
        QHash<Handle, MixedSinger *> mixedSingerObjects;
        QHash<Handle, KeySignature *> keySignatureObjects;
        QHash<Handle, Tempo *> tempoObjects;
        QHash<Handle, TimeSignature *> timeSignatureObjects;
        QHash<Handle, Track *> trackObjects;
        QHash<Handle, Clip *> clipObjects;
        QHash<Handle, AudioClip *> audioClipObjects;
        QHash<Handle, SingingClip *> singingClipObjects;
        QHash<Handle, Sources *> sourcesObjects;

        QString projectName;
        QString projectAuthor;
        int globalCentShift = 0;
        bool multiChannelOutput = false;
        double gain = 1.0;
        double pan = 0.0;
        bool mute = false;
        bool loopEnabled = false;
        int loopStart = 0;
        int loopLength = 1;
        dini::ByteArray workspaceData;
    };

    namespace orm {

        template <typename Related, typename Object>
        Related *resolvePreviousNextObject(Object *object, Handle handle) {
            return ModelPrivate::get(object->model())->template find<Related>(handle);
        }

    }

}

#endif // DSPXMODEL_MODEL_P_H
