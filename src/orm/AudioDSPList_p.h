#ifndef DSPXMODEL_AUDIODSPLIST_P_H
#define DSPXMODEL_AUDIODSPLIST_P_H

#include <dspxmodelORM/AudioDSPList.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class AudioDSPListPrivate {
        Q_DECLARE_PUBLIC(AudioDSPList)
    public:
        AudioDSPListPrivate(AudioDSPList *q, Track *track);

        DSPXMODEL_DECLARE_GET(AudioDSPList)
        DSPXMODEL_FORWARD_CONSTRUCTOR(AudioDSPList)

        void refresh(bool notify, bool itemsChanged);

        AudioDSPList *q_ptr = nullptr;
        Track *track = nullptr;
        int size = 0;
        AudioDSP *first = nullptr;
        AudioDSP *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_AUDIODSPLIST_P_H
