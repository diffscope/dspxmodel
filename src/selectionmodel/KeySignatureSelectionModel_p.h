#ifndef DSPXMODEL_KEYSIGNATURESELECTIONMODEL_P_H
#define DSPXMODEL_KEYSIGNATURESELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/KeySignatureSelectionModel.h>

#include <QSet>

#include <dspxmodelORM/KeySignature.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class KeySignatureSequence;

    class KeySignatureSelectionModelPrivate {
        Q_DECLARE_PUBLIC(KeySignatureSelectionModel)
    public:
        KeySignatureSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<KeySignature *> selectedItems;
        KeySignature *currentItem = nullptr;
        QSet<KeySignature *> connectedItems;

        bool isValidItem(KeySignature *item) const;
        void connectItem(KeySignature *item);
        void disconnectItem(KeySignature *item);
        bool addToSelection(KeySignature *item);
        bool removeFromSelection(KeySignature *item);
        bool clearSelection();
        void dropItem(KeySignature *item);
        void setCurrentItem(KeySignature *item);

        void select(KeySignature *item, SelectionModel::SelectionCommand command);
    };

}

#endif // DSPXMODEL_KEYSIGNATURESELECTIONMODEL_P_H
