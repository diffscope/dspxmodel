#ifndef DSPXMODEL_ANCHORNODESELECTIONMODEL_P_H
#define DSPXMODEL_ANCHORNODESELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>

#include <QSet>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class AnchorNodeSelectionModelPrivate {
        Q_DECLARE_PUBLIC(AnchorNodeSelectionModel)
    public:
        AnchorNodeSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        AnchorNode *currentItem = nullptr;
        QSet<AnchorNode *> selectedItems;
        AnchorNodeSequence *anchorNodeSequenceWithSelectedItems = nullptr;
        QSet<AnchorNode *> connectedItems;
        QMetaObject::Connection anchorNodeSequenceDestroyedConnection;

        bool isValidItem(AnchorNode *item) const;
        bool isValidAnchorNodeSequence(AnchorNodeSequence *sequence) const;
        void connectItem(AnchorNode *item);
        void disconnectItem(AnchorNode *item);
        bool addToSelection(AnchorNode *item);
        bool removeFromSelection(AnchorNode *item);
        bool clearSelection();
        void dropItem(AnchorNode *item);
        void setCurrentItem(AnchorNode *item);
        void connectAnchorNodeSequence(AnchorNodeSequence *sequence);
        void disconnectAnchorNodeSequence();
        void clearAllAndResetAnchorNodeSequence();

        void select(AnchorNode *item, SelectionModel::SelectionCommand command, AnchorNodeSequence *containerItemHint);
    };

}

#endif // DSPXMODEL_ANCHORNODESELECTIONMODEL_P_H
