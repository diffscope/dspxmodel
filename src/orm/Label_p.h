#ifndef DSPXMODEL_LABEL_P_H
#define DSPXMODEL_LABEL_P_H

#include <dspxmodelORM/Label.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class LabelPrivate {
        Q_DECLARE_PUBLIC(Label)
    public:
        explicit LabelPrivate(Label *q);

        DSPXMODEL_DECLARE_GET(Label)
        DSPXMODEL_FORWARD_CONSTRUCTOR(Label)

        void setSequence(LabelSequence *sequence, bool notify);

        Label *q_ptr = nullptr;
        int position = 0;
        QString text;
        Handle previousHandle;
        Handle nextHandle;
        mutable Label *previous = nullptr;
        mutable Label *next = nullptr;
        LabelSequence *sequence = nullptr;
    };

}

#endif // DSPXMODEL_LABEL_P_H
