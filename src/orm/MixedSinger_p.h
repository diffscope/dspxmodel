#ifndef DSPXMODEL_MIXEDSINGER_P_H
#define DSPXMODEL_MIXEDSINGER_P_H

#include <dspxmodelORM/MixedSinger.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class MixedSingerPrivate {
        Q_DECLARE_PUBLIC(MixedSinger)
    public:
        explicit MixedSingerPrivate(MixedSinger *q);

        DSPXMODEL_DECLARE_GET(MixedSinger)
        DSPXMODEL_FORWARD_CONSTRUCTOR(MixedSinger)

        MixedSinger *q_ptr = nullptr;
        QList<double> ratio;
        SingerList *singers = nullptr;
    };

}

#endif // DSPXMODEL_MIXEDSINGER_P_H
