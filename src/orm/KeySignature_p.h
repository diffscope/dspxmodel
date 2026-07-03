#ifndef DSPXMODEL_KEYSIGNATURE_P_H
#define DSPXMODEL_KEYSIGNATURE_P_H

#include <dspxmodelORM/KeySignature.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class KeySignaturePrivate {
        Q_DECLARE_PUBLIC(KeySignature)
    public:
        explicit KeySignaturePrivate(KeySignature *q);

        DSPXMODEL_DECLARE_GET(KeySignature)
        DSPXMODEL_FORWARD_CONSTRUCTOR(KeySignature)

        void setSequence(KeySignatureSequence *sequence, bool notify);

        KeySignature *q_ptr = nullptr;
        int position = 0;
        int mode = 0;
        int tonality = 0;
        KeySignature::AccidentalType accidentalType = KeySignature::Flat;
        Handle previousHandle;
        Handle nextHandle;
        mutable KeySignature *previous = nullptr;
        mutable KeySignature *next = nullptr;
        KeySignatureSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURE_P_H
