#ifndef DSPXMODEL_PIECE_H
#define DSPXMODEL_PIECE_H

#include <QObject>
#include <QScopedPointer>
#include <qqmlintegration.h>

#include <dspxmodelPiece/DSPXModelPieceGlobal.h>
#include <dspxmodelPiece/PieceChange.h>

namespace dspx {

    class PieceDivider;
    class PieceDividerPrivate;
    class PiecePrivate;

    /**
     * @brief One time-ordered note piece produced by PieceDivider.
     *
     * A Piece is induced by a non-empty consecutive group of note-start groups.
     * Its boundaries include the millisecond padding selected by the divider and
     * are converted back independently to tick coordinates. Boundaries are not
     * clipped to the visible clip range, so position() may be negative and the
     * end may exceed the clip length.
     *
     * Piece objects have stable QObject identity while PieceDivider can preserve
     * their musical content through a rebuild. They have no persistent index;
     * their current order is represented solely by PieceDivider::pieces().
     * Content notifications are emitted after transaction-committed rebuilding.
     */
    class DSPXMODEL_PIECE_EXPORT Piece : public QObject {
        Q_OBJECT
        QML_ELEMENT
        QML_UNCREATABLE("")
        Q_DECLARE_PRIVATE(Piece)
        Q_PROPERTY(double position READ position NOTIFY positionChanged)
        Q_PROPERTY(double length READ length NOTIFY lengthChanged)

    public:
        /**
         * @brief Gets the piece start relative to the clip start, in ticks.
         */
        double position() const;

        /**
         * @brief Gets the piece duration in ticks.
         * @post length() >= 0.0.
         */
        double length() const;

    signals:
        /**
         * @brief Emitted after the tick start changes during a rebuild.
         * @param position New clip-relative start in ticks.
         */
        void positionChanged(double position);

        /**
         * @brief Emitted after the tick duration changes during a rebuild.
         * @param length New duration in ticks.
         */
        void lengthChanged(double length);

        /**
         * @brief Reports all committed content changes affecting this piece.
         *
         * This signal is emitted after all piece boundary and list notifications
         * for the transaction have completed, at most once per transaction.
         * @param change Aggregated committed change categories.
         */
        void updated(const PieceChange &change);

    private:
        explicit Piece(PieceDivider *divider);
        ~Piece() override;

        QScopedPointer<PiecePrivate> d_ptr;

        friend class PieceDividerPrivate;
    };

}

#endif // DSPXMODEL_PIECE_H
