#ifndef DSPXMODEL_SINGERLIST_P_H
#define DSPXMODEL_SINGERLIST_P_H

#include <dspxmodelORM/SingerList.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class SingerListPrivate {
        Q_DECLARE_PUBLIC(SingerList)
    public:
        explicit SingerListPrivate(SingerList *q, Sources *sources);
        explicit SingerListPrivate(SingerList *q, MixedSinger *mixedSinger);

        DSPXMODEL_DECLARE_GET(SingerList)
        DSPXMODEL_FORWARD_CONSTRUCTOR(SingerList)

        Handle mixableHandle(bool create) const;
        dini::Value associationValue(bool create) const;
        void refresh(bool notify, bool itemsChanged);

        SingerList *q_ptr = nullptr;
        Sources *sources = nullptr;
        MixedSinger *mixedSinger = nullptr;
        int size = 0;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_SINGERLIST_P_H
