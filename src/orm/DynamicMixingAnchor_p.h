#ifndef DSPXMODEL_DYNAMICMIXINGANCHOR_P_H
#define DSPXMODEL_DYNAMICMIXINGANCHOR_P_H

#include <dspxmodelORM/DynamicMixingAnchor.h>

#include <dini/value.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class DynamicMixingAnchorPrivate {
        Q_DECLARE_PUBLIC(DynamicMixingAnchor)
    public:
        explicit DynamicMixingAnchorPrivate(DynamicMixingAnchor *q);

        DSPXMODEL_DECLARE_GET(DynamicMixingAnchor)
        DSPXMODEL_FORWARD_CONSTRUCTOR(DynamicMixingAnchor)

        void setSequence(DynamicMixingAnchorSequence *sequence, bool notify);

        DynamicMixingAnchor *q_ptr = nullptr;
        int position = 0;
        QList<double> ratio;
        Handle previousHandle;
        Handle nextHandle;
        mutable DynamicMixingAnchor *previous = nullptr;
        mutable DynamicMixingAnchor *next = nullptr;
        DynamicMixingAnchorSequence *sequence = nullptr;
    };

    namespace orm {

        dini::Value valueFromRatio(const QList<double> &ratio);
        QList<double> ratioFromValue(const dini::Value &value);

    }

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHOR_P_H
