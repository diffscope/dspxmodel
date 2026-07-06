#ifndef DSPXMODEL_PARAMETER_P_H
#define DSPXMODEL_PARAMETER_P_H

#include <dspxmodelORM/Parameter.h>

#include <QString>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class ParameterPrivate {
        Q_DECLARE_PUBLIC(Parameter)
    public:
        explicit ParameterPrivate(Parameter *q);

        DSPXMODEL_DECLARE_GET(Parameter)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Parameter)

        void setPlacement(ParameterMap *parameterMap, QString key, bool notify);

        Parameter *q_ptr = nullptr;
        ParameterMap *parameterMap = nullptr;
        QString key;
        FreeValueDataArray *original = nullptr;
        FreeValueDataArray *freeTransform = nullptr;
        FreeValueDataArray *freeEdited = nullptr;
        AnchorNodeSequence *anchorTransform = nullptr;
        AnchorNodeSequence *anchorEdited = nullptr;
    };

}

#endif // DSPXMODEL_PARAMETER_P_H
