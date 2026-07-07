#ifndef DSPXMODEL_TRACKSELECTIONMODEL_P_H
#define DSPXMODEL_TRACKSELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/TrackSelectionModel.h>

#include <QSet>

#include <dspxmodelORM/Track.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class SelectionModel;
    class TrackList;

    class TrackSelectionModelPrivate {
        Q_DECLARE_PUBLIC(TrackSelectionModel)
    public:
        TrackSelectionModel *q_ptr;
        SelectionModel *selectionModel;
        QSet<Track *> selectedItems;
        Track *currentItem = nullptr;
        QSet<Track *> connectedItems;

        bool isValidItem(Track *item) const;
        void connectItem(Track *item);
        void disconnectItem(Track *item);
        bool addToSelection(Track *item);
        bool removeFromSelection(Track *item);
        bool clearSelection();
        void dropItem(Track *item);
        void setCurrentItem(Track *item);

        void select(Track *item, SelectionModel::SelectionCommand command);
    };

}

#endif // DSPXMODEL_TRACKSELECTIONMODEL_P_H
