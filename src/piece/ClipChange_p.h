#ifndef DSPXMODEL_CLIPCHANGE_P_H
#define DSPXMODEL_CLIPCHANGE_P_H

#include <QSharedData>

#include "ClipChange.h"

namespace dspx {

    class ClipChangePrivate : public QSharedData {
    public:
        ClipChange::ChangeTypes types;
        QStringList parameterNames;
        QList<ClipChangeRange> ranges;
    };

}

#endif // DSPXMODEL_CLIPCHANGE_P_H
