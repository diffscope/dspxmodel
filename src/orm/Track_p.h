#ifndef DSPXMODEL_TRACK_P_H
#define DSPXMODEL_TRACK_P_H

#include <dspxmodelORM/Track.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class TrackPrivate {
    public:
        explicit TrackPrivate(Track *q);
        Track *q_func() const { return q_ptr; }

        DSPXMODEL_DECLARE_GET(Track)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Track)

        void setTrackList(TrackList *list, bool notify);

        Track *q_ptr = nullptr;
        int colorId = 0;
        double height = 0.0;
        QString name;
        double gain = 1.0;
        double pan = 0.0;
        bool mute = false;
        bool solo = false;
        bool record = false;
        TrackList *trackList = nullptr;
    };

}

#endif // DSPXMODEL_TRACK_P_H
