#include "Piece.h"
#include "Piece_p.h"

namespace dspx {

    Piece::Piece(double position, double length, QObject *parent)
        : QObject(parent), d_ptr(new PiecePrivate(position, length)) {
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

    void Piece::setPosition(double position) {
        Q_D(Piece);
        if (d->position == position) {
            return;
        }
        d->position = position;
        emit positionChanged(position);
    }

    void Piece::setLength(double length) {
        Q_D(Piece);
        if (d->length == length) {
            return;
        }
        d->length = length;
        emit lengthChanged(length);
    }

}


#include "moc_Piece.cpp"
