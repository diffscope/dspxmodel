#ifndef DSPXMODEL_TRACK_P_H
#define DSPXMODEL_TRACK_P_H

#include <dspxmodelORM/Track.h>

#include <dini/types.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class TrackPrivate {
        Q_DECLARE_PUBLIC(Track);
    public:
        explicit TrackPrivate(Track *q);

        DSPXMODEL_DECLARE_GET(Track)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Track)

        void setTrackList(TrackList *list, bool notify);
        dini::ByteArray workspace() const;
        void setWorkspace(dini::ByteArray workspace);

        Track *q_ptr = nullptr;
        ClipSequence *clips = nullptr;
        int colorId = 0;
        double height = 0.0;
        QString name;
        double gain = 1.0;
        double pan = 0.0;
        bool mute = false;
        bool solo = false;
        bool record = false;
        dini::ByteArray workspaceData;
        TrackList *trackList = nullptr;
    };

}

#endif // DSPXMODEL_TRACK_P_H
