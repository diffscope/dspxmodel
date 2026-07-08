#ifndef DSPXMODEL_TIMESIGNATURESEQUENCE_P_H
#define DSPXMODEL_TIMESIGNATURESEQUENCE_P_H

#include <dspxmodelORM/TimeSignatureSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class TimeSignatureSequencePrivate {
        Q_DECLARE_PUBLIC(TimeSignatureSequence)
    public:
        TimeSignatureSequencePrivate(TimeSignatureSequence *q, Model *model);

        DSPXMODEL_DECLARE_GET(TimeSignatureSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(TimeSignatureSequence)

        void refresh(bool notify);
        TimeSignatureSequence *q_ptr = nullptr;
        Model *model = nullptr;
        int size = 0;
        TimeSignature *first = nullptr;
        TimeSignature *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_TIMESIGNATURESEQUENCE_P_H
