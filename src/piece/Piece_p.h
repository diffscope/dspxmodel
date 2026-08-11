#ifndef DSPXMODEL_PIECE_P_H
#define DSPXMODEL_PIECE_P_H

#include <QList>

#include "Piece.h"

namespace dspx {

    class PiecePrivate {
    public:
        static PiecePrivate *get(Piece *piece) { return piece->d_func(); }
        static const PiecePrivate *get(const Piece *piece) { return piece->d_func(); }

        double position = 0.0;
        double length = 0.0;
        int firstNotePosition = 0;
        int lastNotePosition = 0;
        QList<quint64> noteIds;
    };

}

#endif // DSPXMODEL_PIECE_P_H
