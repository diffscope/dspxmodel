#ifndef DSPXMODEL_TEMPOSELECTIONMODEL_P_H
#define DSPXMODEL_TEMPOSELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/TempoSelectionModel.h>

#include <QSet>

#include <dspxmodelORM/Tempo.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class TempoSequence;

    class TempoSelectionModelPrivate {
        Q_DECLARE_PUBLIC(TempoSelectionModel)
    public:
        TempoSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Tempo *> selectedItems;
        Tempo *currentItem = nullptr;
        QSet<Tempo *> connectedItems;

        bool isValidItem(Tempo *item) const;
        void connectItem(Tempo *item);
        void disconnectItem(Tempo *item);
        bool addToSelection(Tempo *item);
        bool removeFromSelection(Tempo *item);
        bool clearSelection();
        void dropItem(Tempo *item);
        void setCurrentItem(Tempo *item);

        void select(Tempo *item, SelectionModel::SelectionCommand command);
    };

}

#endif // DSPXMODEL_TEMPOSELECTIONMODEL_P_H
