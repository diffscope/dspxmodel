#ifndef DSPXMODEL_PIECE_H
#define DSPXMODEL_PIECE_H

#include <QObject>
#include <QScopedPointer>

#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class PieceDividerPrivate;
    class PiecePrivate;

    class DSPXMODEL_PIECE_EXPORT Piece : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(Piece)
        Q_PROPERTY(double position READ position NOTIFY positionChanged)
        Q_PROPERTY(double length READ length NOTIFY lengthChanged)
    public:
        ~Piece() override;

        /**
         * @brief Gets the position relative to the singing clip, in ticks.
         */
        double position() const;
        /**
         * @brief Gets the length, in ticks.
         * @post length() >= 0.
         */
        double length() const;

    signals:
        void positionChanged(double position);
        void lengthChanged(double length);

    private:
        explicit Piece(double position, double length, QObject *parent = nullptr);

        void setPosition(double position);
        void setLength(double length);

        friend class PieceDividerPrivate;

        QScopedPointer<PiecePrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PIECE_H
