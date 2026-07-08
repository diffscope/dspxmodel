#ifndef DSPXMODEL_KEYSIGNATURESEQUENCE_P_H
#define DSPXMODEL_KEYSIGNATURESEQUENCE_P_H

#include <dspxmodelORM/KeySignatureSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class KeySignatureSequencePrivate {
        Q_DECLARE_PUBLIC(KeySignatureSequence)
    public:
        KeySignatureSequencePrivate(KeySignatureSequence *q, Model *model);

        DSPXMODEL_DECLARE_GET(KeySignatureSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(KeySignatureSequence)

        void refresh(bool notify);
        KeySignatureSequence *q_ptr = nullptr;
        Model *model = nullptr;
        int size = 0;
        KeySignature *first = nullptr;
        KeySignature *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_KEYSIGNATURESEQUENCE_P_H
