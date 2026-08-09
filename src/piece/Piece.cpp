#include "Piece.h"

#include "Piece_p.h"
#include "PieceDivider.h"

namespace dspx {

    Piece::Piece(PieceDivider *divider)
        : QObject(divider), d_ptr(new PiecePrivate) {
    }

    Piece::~Piece() = default;

    double Piece::position() const {
        Q_D(const Piece);
        return d->position;
    }

    double Piece::length() const {
        Q_D(const Piece);
        return d->length;
    }

}
