#ifndef DSPXMODEL_ANCHORNODE_P_H
#define DSPXMODEL_ANCHORNODE_P_H

#include <dspxmodelORM/AnchorNode.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class AnchorNodePrivate {
        Q_DECLARE_PUBLIC(AnchorNode)
    public:
        explicit AnchorNodePrivate(AnchorNode *q);

        DSPXMODEL_DECLARE_GET(AnchorNode)
        DSPXMODEL_FORWARD_CONSTRUCTOR(AnchorNode)

        void setPlacement(Handle relation, bool notify);
        void setSequence(AnchorNodeSequence *sequence, bool notify);

        AnchorNode *q_ptr = nullptr;
        AnchorNode::InterpolationMode interpolationMode = AnchorNode::None;
        int x = 0;
        int y = 0;
        Handle previousHandle;
        Handle nextHandle;
        mutable AnchorNode *previous = nullptr;
        mutable AnchorNode *next = nullptr;
        Handle relationHandle;
        AnchorNodeSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_ANCHORNODE_P_H
