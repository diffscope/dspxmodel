#ifndef DSPXMODEL_TEMPO_P_H
#define DSPXMODEL_TEMPO_P_H

#include <dspxmodelORM/Tempo.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class TempoPrivate {
        Q_DECLARE_PUBLIC(Tempo)
    public:
        explicit TempoPrivate(Tempo *q);

        DSPXMODEL_DECLARE_GET(Tempo)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Tempo)

        void setSequence(TempoSequence *sequence, bool notify);

        Tempo *q_ptr = nullptr;
        int position = 0;
        double value = 120.0;
        Handle previousHandle;
        Handle nextHandle;
        mutable Tempo *previous = nullptr;
        mutable Tempo *next = nullptr;
        TempoSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_TEMPO_P_H
