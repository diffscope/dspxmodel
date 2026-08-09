#ifndef DSPXMODEL_PIECECHANGE_P_H
#define DSPXMODEL_PIECECHANGE_P_H

#include <QSharedData>

#include "PieceChange.h"

namespace dspx {

    class PieceChangePrivate : public QSharedData {
    public:
        PieceChange::ChangeTypes types;
        QStringList parameterNames;
    };

}

#endif // DSPXMODEL_PIECECHANGE_P_H
