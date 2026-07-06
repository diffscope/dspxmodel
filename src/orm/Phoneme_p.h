#ifndef DSPXMODEL_PHONEME_P_H
#define DSPXMODEL_PHONEME_P_H

#include <dspxmodelORM/Phoneme.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class PhonemePrivate {
        Q_DECLARE_PUBLIC(Phoneme)
    public:
        explicit PhonemePrivate(Phoneme *q);

        DSPXMODEL_DECLARE_GET(Phoneme)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Phoneme)

        void setPlacement(Handle relation, bool notify);
        void setSequence(PhonemeSequence *sequence, bool notify);

        Phoneme *q_ptr = nullptr;
        QString language;
        int start = 0;
        QString token;
        bool onset = false;
        Handle previousHandle;
        Handle nextHandle;
        mutable Phoneme *previous = nullptr;
        mutable Phoneme *next = nullptr;
        Handle relationHandle;
        PhonemeSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_PHONEME_P_H
