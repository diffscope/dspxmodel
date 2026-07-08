#ifndef DSPXMODEL_ANCHORNODESEQUENCE_P_H
#define DSPXMODEL_ANCHORNODESEQUENCE_P_H

#include <dspxmodelORM/AnchorNodeSequence.h>

#include <dini/value.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class AnchorNodeSequencePrivate {
        Q_DECLARE_PUBLIC(AnchorNodeSequence)
    public:
        AnchorNodeSequencePrivate(AnchorNodeSequence *q, Parameter *parameter, AnchorNodeSequence::AnchorNodeRole role);

        DSPXMODEL_DECLARE_GET(AnchorNodeSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(AnchorNodeSequence)

        void refresh(bool notify);
        Handle relationHandle() const;
        dini::Value associationValue() const;

        AnchorNodeSequence *q_ptr = nullptr;
        Parameter *parameter = nullptr;
        AnchorNodeSequence::AnchorNodeRole role = AnchorNodeSequence::Transform;
        int size = 0;
        AnchorNode *first = nullptr;
        AnchorNode *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_ANCHORNODESEQUENCE_P_H
