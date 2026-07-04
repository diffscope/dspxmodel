#ifndef DSPXMODEL_TEMPOSEQUENCE_P_H
#define DSPXMODEL_TEMPOSEQUENCE_P_H

#include <dspxmodelORM/TempoSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class TempoSequencePrivate {
        Q_DECLARE_PUBLIC(TempoSequence)
    public:
        TempoSequencePrivate(TempoSequence *q, Model *model);

        DSPXMODEL_DECLARE_GET(TempoSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(TempoSequence)

        void refresh(bool notify);
        Handle itemAtPosition(int position, Handle except = {}) const;

        TempoSequence *q_ptr = nullptr;
        Model *model = nullptr;
        int size = 0;
        Tempo *first = nullptr;
        Tempo *last = nullptr;
    };

}

#endif // DSPXMODEL_TEMPOSEQUENCE_P_H
