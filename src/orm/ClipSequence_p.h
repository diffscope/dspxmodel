#ifndef DSPXMODEL_CLIPSEQUENCE_P_H
#define DSPXMODEL_CLIPSEQUENCE_P_H

#include <dspxmodelORM/ClipSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class ClipSequencePrivate {
        Q_DECLARE_PUBLIC(ClipSequence)
    public:
        ClipSequencePrivate(ClipSequence *q, Track *track);

        DSPXMODEL_DECLARE_GET(ClipSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(ClipSequence)

        void refresh(bool notify);

        ClipSequence *q_ptr = nullptr;
        Track *track = nullptr;
        int size = 0;
        Clip *first = nullptr;
        Clip *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_CLIPSEQUENCE_P_H
