#ifndef DSPXMODEL_NOTESEQUENCE_P_H
#define DSPXMODEL_NOTESEQUENCE_P_H

#include <dspxmodelORM/NoteSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/JSIterable_p.h>

namespace dspx {

    class NoteSequencePrivate {
        Q_DECLARE_PUBLIC(NoteSequence)
    public:
        NoteSequencePrivate(NoteSequence *q, SingingClip *singingClip);

        DSPXMODEL_DECLARE_GET(NoteSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(NoteSequence)

        void refresh(bool notify);

        NoteSequence *q_ptr = nullptr;
        SingingClip *singingClip = nullptr;
        int size = 0;
        Note *first = nullptr;
        Note *last = nullptr;

        JSIterable *jsIterable = nullptr;
    };

}

#endif // DSPXMODEL_NOTESEQUENCE_P_H
