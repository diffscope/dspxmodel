#ifndef DSPXMODEL_SINGLESINGER_P_H
#define DSPXMODEL_SINGLESINGER_P_H

#include <dspxmodelORM/SingleSinger.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class SingleSingerPrivate {
        Q_DECLARE_PUBLIC(SingleSinger)
    public:
        explicit SingleSingerPrivate(SingleSinger *q);

        DSPXMODEL_DECLARE_GET(SingleSinger)
        DSPXMODEL_FORWARD_CONSTRUCTOR(SingleSinger)

        SingleSinger *q_ptr = nullptr;
        QString id;
    };

}

#endif // DSPXMODEL_SINGLESINGER_P_H
