#ifndef DSPXMODEL_TIMESIGNATURE_P_H
#define DSPXMODEL_TIMESIGNATURE_P_H

#include <dspxmodelORM/TimeSignature.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class TimeSignaturePrivate {
        Q_DECLARE_PUBLIC(TimeSignature)
    public:
        explicit TimeSignaturePrivate(TimeSignature *q);

        DSPXMODEL_DECLARE_GET(TimeSignature)
        DSPXMODEL_FORWARD_CONSTRUCTOR(TimeSignature)

        void setSequence(TimeSignatureSequence *sequence, bool notify);

        TimeSignature *q_ptr = nullptr;
        int index = 0;
        int numerator = 4;
        int denominator = 4;
        Handle previousHandle;
        Handle nextHandle;
        mutable TimeSignature *previous = nullptr;
        mutable TimeSignature *next = nullptr;
        TimeSignatureSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_TIMESIGNATURE_P_H
