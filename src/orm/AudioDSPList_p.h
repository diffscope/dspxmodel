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
        AudioDSPListPrivate(AudioDSPList *q, Model *model);

        DSPXMODEL_DECLARE_GET(AudioDSPList)
        DSPXMODEL_FORWARD_CONSTRUCTOR(AudioDSPList)

        Handle parentHandle(bool create) const;
        dini::Value associationValue(bool create) const;
        void refresh(bool notify, bool itemsChanged);

        AudioDSPList *q_ptr = nullptr;
        Model *model = nullptr;
        Track *track = nullptr;
        int size = 0;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_AUDIODSPLIST_P_H
