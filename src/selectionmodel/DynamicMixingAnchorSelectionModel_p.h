#ifndef DSPXMODEL_DYNAMICMIXINGANCHORSELECTIONMODEL_P_H
#define DSPXMODEL_DYNAMICMIXINGANCHORSELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>

#include <QSet>

#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class DynamicMixingAnchorSelectionModelPrivate {
        Q_DECLARE_PUBLIC(DynamicMixingAnchorSelectionModel)
    public:
        DynamicMixingAnchorSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        DynamicMixingAnchor *currentItem = nullptr;
        QSet<DynamicMixingAnchor *> selectedItems;
        DynamicMixingAnchorSequence *dynamicMixingAnchorSequenceWithSelectedItems = nullptr;
        QSet<DynamicMixingAnchor *> connectedItems;
        QMetaObject::Connection sequenceDestroyedConnection;

        bool isValidItem(DynamicMixingAnchor *item) const;
        bool isValidSequence(DynamicMixingAnchorSequence *sequence) const;
        void connectItem(DynamicMixingAnchor *item);
        void disconnectItem(DynamicMixingAnchor *item);
        bool addToSelection(DynamicMixingAnchor *item);
        bool removeFromSelection(DynamicMixingAnchor *item);
        bool clearSelection();
        void dropItem(DynamicMixingAnchor *item);
        void setCurrentItem(DynamicMixingAnchor *item);
        void connectSequence(DynamicMixingAnchorSequence *sequence);
        void disconnectSequence();
        void clearAllAndResetSequence();
        void select(DynamicMixingAnchor *item, SelectionModel::SelectionCommand command,
                    DynamicMixingAnchorSequence *containerItemHint);
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHORSELECTIONMODEL_P_H
