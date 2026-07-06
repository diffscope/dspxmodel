#ifndef DSPXMODEL_PARAMETERMAP_P_H
#define DSPXMODEL_PARAMETERMAP_P_H

#include <dspxmodelORM/ParameterMap.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class ParameterMapPrivate {
        Q_DECLARE_PUBLIC(ParameterMap)
    public:
        ParameterMapPrivate(ParameterMap *q, SingingClip *singingClip);

        DSPXMODEL_DECLARE_GET(ParameterMap)
        DSPXMODEL_FORWARD_CONSTRUCTOR(ParameterMap)

        void refresh(bool notify);

        ParameterMap *q_ptr = nullptr;
        SingingClip *singingClip = nullptr;
        int size = 0;
    };

}

#endif // DSPXMODEL_PARAMETERMAP_P_H
