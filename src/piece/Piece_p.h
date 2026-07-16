#ifndef DSPXMODEL_PIECE_P_H
#define DSPXMODEL_PIECE_P_H

namespace dspx {

    class PiecePrivate {
    public:
        PiecePrivate(double position, double length) : position(position), length(length) {
        }

        double position = 0;
        double length = 0;
    };

}

#endif // DSPXMODEL_PIECE_P_H
