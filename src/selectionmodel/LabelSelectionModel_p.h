#ifndef DSPXMODEL_LABELSELECTIONMODEL_P_H
#define DSPXMODEL_LABELSELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/LabelSelectionModel.h>

#include <QSet>

#include <dspxmodelORM/Label.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class LabelSequence;

    class LabelSelectionModelPrivate {
        Q_DECLARE_PUBLIC(LabelSelectionModel)
    public:
        LabelSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Label *> selectedItems;
        Label *currentItem = nullptr;
        QSet<Label *> connectedItems;

        bool isValidItem(Label *item) const;
        void connectItem(Label *item);
        void disconnectItem(Label *item);
        bool addToSelection(Label *item);
        bool removeFromSelection(Label *item);
        bool clearSelection();
        void dropItem(Label *item);
        void setCurrentItem(Label *item);

        void select(Label *item, SelectionModel::SelectionCommand command);
    };

}

#endif // DSPXMODEL_LABELSELECTIONMODEL_P_H
