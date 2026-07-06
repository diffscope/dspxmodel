#ifndef DSPXMODEL_DYNAMICMIXINGANCHORSEQUENCE_P_H
#define DSPXMODEL_DYNAMICMIXINGANCHORSEQUENCE_P_H

#include <dspxmodelORM/DynamicMixingAnchorSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class DynamicMixingAnchorSequencePrivate {
        Q_DECLARE_PUBLIC(DynamicMixingAnchorSequence)
    public:
        DynamicMixingAnchorSequencePrivate(DynamicMixingAnchorSequence *q, Sources *sources);

        DSPXMODEL_DECLARE_GET(DynamicMixingAnchorSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(DynamicMixingAnchorSequence)

        void refresh(bool notify);
        Handle itemAtPosition(int position, Handle except = {}) const;

        DynamicMixingAnchorSequence *q_ptr = nullptr;
        Sources *sources = nullptr;
        int size = 0;
        DynamicMixingAnchor *first = nullptr;
        DynamicMixingAnchor *last = nullptr;
    };

}

#endif // DSPXMODEL_DYNAMICMIXINGANCHORSEQUENCE_P_H
