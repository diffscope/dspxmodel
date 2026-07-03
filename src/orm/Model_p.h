#ifndef DSPXMODEL_MODEL_P_H
#define DSPXMODEL_MODEL_P_H

#include <dspxmodelORM/Model.h>

#include <vector>

#include <QHash>

#include <dini/types.h>
#include <dini/event.h>
#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/Label_p.h>
#include <dspxmodelORM/private/KeySignature_p.h>
#include <dspxmodelORM/private/Track_p.h>

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
        void applyOperation(const dini::ChangeOperation &operation);
        void applyItemInserted(const dini::ItemInsertedChange &change);
        void applyItemRemoved(const dini::ItemSnapshot &snapshot, bool cascade);
        void applyListInserted(const dini::ListInsertedChange &change);
        void applyListRemoved(const dini::ListRemovedChange &change);
        void applyColumnUpdated(const dini::ColumnUpdatedChange &change);
        void applyComputedColumnUpdated(const dini::ComputedColumnUpdatedChange &change);
        void applyListRotated(const dini::ListRotatedChange &change);

        bool isModelValue(const dini::Value &value) const;
        dini::Transaction *requireTransaction() const;

        const orm::TableBinding *tableBinding(dini::ContainerId containerId) const;
        const orm::ListBinding *listBinding(dini::ContainerId containerId) const;

        template <typename T>
        QHash<Handle, T *> &objectMap() {
            if constexpr (std::is_same_v<T, Label>) {
                return labelObjects;
            } else if constexpr (std::is_same_v<T, KeySignature>) {
                return keySignatureObjects;
            } else if constexpr (std::is_same_v<T, Track>) {
                return trackObjects;
            } else {
                static_assert(std::is_same_v<T, void>);
                return {};
            }
        }

        template <typename T>
        static T *createObject(Handle handle, Model *model) {
            if constexpr (std::is_same_v<T, Label>) {
                return LabelPrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, KeySignature>) {
                return KeySignaturePrivate::create(handle, model);
            } else if constexpr (std::is_same_v<T, Track>) {
                return TrackPrivate::create(handle, model);
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
            if (!orm::isContainer(snapshot, Schema::labelTable())) {
                return nullptr;
            }
            const auto handle = orm::handleFromId(snapshot.id);
            if (auto it = objectMap<T>().find(handle); it != objectMap<T>().end()) {
                return it.value();
            }
            auto *object = createObject<T>(handle, q_func());
            labelObjects.insert(handle, object);
            orm::syncLabelColumns(object, snapshot, false);
            return object;
        }

        template <typename T>
        T *find(Handle handle) const {
            const auto it = objectMap<T>().find(handle);
            return it == objectMap<T>().end() ? nullptr : it.value();
        }

        void update(Handle handle, dini::ColumnHandle column, const dini::Value &value, const dini::AssociationUpdateOptions &options = {}) {
            requireTransaction()->update(orm::idFromHandle(handle), column, value, options);
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

        LabelSequence *labels = nullptr;
        KeySignatureSequence *keySignatures = nullptr;
        TrackList *tracks = nullptr;

        std::vector<const orm::TableBinding *> tableBindings;
        std::vector<const orm::ListBinding *> listBindings;

        QHash<Handle, Label *> labelObjects;
        QHash<Handle, KeySignature *> keySignatureObjects;
        QHash<Handle, Track *> trackObjects;

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
    };

    namespace orm {

        template <typename Related, typename Object>
        Related *resolvePreviousNextObject(Object *object, Handle handle) {
            return ModelPrivate::get(object->model())->template find<Related>(handle);
        }

    }

}

#endif // DSPXMODEL_MODEL_P_H
