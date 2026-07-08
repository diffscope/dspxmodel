#ifndef DSPXMODEL_TRACKLIST_P_H
#define DSPXMODEL_TRACKLIST_P_H

#include <dspxmodelORM/TrackList.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class TrackListPrivate {
        Q_DECLARE_PUBLIC(TrackList)
    public:
        TrackListPrivate(TrackList *q, Model *model);

        DSPXMODEL_DECLARE_GET(TrackList)
        DSPXMODEL_FORWARD_CONSTRUCTOR(TrackList)

        void refresh(bool notify, bool itemsChanged);

        TrackList *q_ptr = nullptr;
        Model *model = nullptr;
        int size = 0;
        Track *first = nullptr;
        Track *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_TRACKLIST_P_H
