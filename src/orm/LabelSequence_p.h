#ifndef DSPXMODEL_LABELSEQUENCE_P_H
#define DSPXMODEL_LABELSEQUENCE_P_H

#include <dspxmodelORM/LabelSequence.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class LabelSequencePrivate {
        Q_DECLARE_PUBLIC(LabelSequence)
    public:
        LabelSequencePrivate(LabelSequence *q, Model *model);

        DSPXMODEL_DECLARE_GET(LabelSequence)
        DSPXMODEL_FORWARD_CONSTRUCTOR(LabelSequence)

        void refresh(bool notify);

        LabelSequence *q_ptr = nullptr;
        Model *model = nullptr;
        int size = 0;
        Label *first = nullptr;
        Label *last = nullptr;
    };

}

#endif // DSPXMODEL_LABELSEQUENCE_P_H
