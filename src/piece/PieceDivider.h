#ifndef DSPXMODEL_PIECEDIVIDER_H
#define DSPXMODEL_PIECEDIVIDER_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class Piece;
    class PieceDividerPrivate;
    class SingingClip;

    /**
     * @brief Divides a SingingClip into padded, non-overlapping note pieces.
     *
     * Notes are grouped by equal starts and scanned in time order. All note
     * boundaries, gaps and padding decisions are evaluated in absolute
     * milliseconds using the bound clip model's tempo sequence. A cut is made at
     * the earliest legal group boundary, producing the unique maximum-piece
     * partition defined by the note-piece specification. Piece boundaries are
     * converted back to double tick values without clipping to the clip range.
     * A non-rest note receives left padding
     * `min(paddingBase + leadingNonOnsetCount * paddingAdditional, paddingGap)`
     * and right padding `paddingBase`; a strictly matched rest lyric receives no
     * padding. paddingGap is also the minimum unpadded body gap at a legal cut.
     *
     * The divider observes all model signals which can change the partition, but
     * only records them. Pieces are synchronized when update() is called; neither
     * ORM transactions nor their commit/rollback events are observed. Local note
     * and tempo edits rebuild only the affected consecutive portion when
     * possible. Existing Piece objects are reused in content order before objects
     * are removed or inserted, minimizing observable list changes.
     *
     * Any non-null SingingClip is divided using singingClip()->model(); membership
     * in a Track or TrackList is not required. Setting singingClip to null clears
     * all pieces on the next update().
     */
    class DSPXMODEL_PIECE_EXPORT PieceDivider : public QObject {
        Q_OBJECT
        QML_ELEMENT
        Q_DECLARE_PRIVATE(PieceDivider)
        Q_PROPERTY(SingingClip *singingClip READ singingClip WRITE setSingingClip NOTIFY singingClipChanged)
        Q_PROPERTY(double paddingBase READ paddingBase WRITE setPaddingBase NOTIFY paddingBaseChanged)
        Q_PROPERTY(double paddingAdditional READ paddingAdditional WRITE setPaddingAdditional NOTIFY paddingAdditionalChanged)
        Q_PROPERTY(double paddingGap READ paddingGap WRITE setPaddingGap NOTIFY paddingGapChanged)
        Q_PROPERTY(QStringList restLyrics READ restLyrics WRITE setRestLyrics NOTIFY restLyricsChanged)
        Q_PROPERTY(QList<Piece *> pieces READ pieces NOTIFY piecesChanged)

    public:
        /**
         * @brief Creates an unbound divider with zero padding and no rest lyrics.
         */
        explicit PieceDivider(QObject *parent = nullptr);

        /**
         * @brief Destroys the divider and all Piece objects it owns.
         */
        ~PieceDivider() override;

        /**
         * @brief Gets the clip currently being divided, or nullptr.
         */
        SingingClip *singingClip() const;

        /**
         * @brief Selects the clip to divide on the next update().
         * @param singingClip Clip to divide, or nullptr to clear on update().
         */
        void setSingingClip(SingingClip *singingClip);

        /**
         * @brief Gets the latest requested non-rest base padding in milliseconds.
         */
        double paddingBase() const;

        /**
         * @brief Sets the requested non-rest base padding for the next update().
         * @param paddingBase Finite non-negative padding; invalid values are ignored.
         */
        void setPaddingBase(double paddingBase);

        /**
         * @brief Gets the latest requested per-phoneme left padding, in milliseconds.
         */
        double paddingAdditional() const;

        /**
         * @brief Sets the requested additional left padding for the next update().
         * @param paddingAdditional Finite non-negative padding; invalid values are ignored.
         */
        void setPaddingAdditional(double paddingAdditional);

        /**
         * @brief Gets the latest requested body gap/left-padding cap, in milliseconds.
         */
        double paddingGap() const;

        /**
         * @brief Sets the requested body gap and left-padding cap for the next update().
         * @param paddingGap Finite non-negative value; invalid values are ignored.
         */
        void setPaddingGap(double paddingGap);

        /**
         * @brief Gets the latest requested strict-equality rest lyrics.
         */
        QStringList restLyrics() const;

        /**
         * @brief Sets lyrics that identify rest notes on the next update().
         */
        void setRestLyrics(const QStringList &restLyrics);

        /**
         * @brief Gets the last explicitly updated pieces in ascending start-time order.
         *
         * The list position is derived state and has no persistent index semantics.
         */
        QList<Piece *> pieces() const;

        /**
         * @brief Applies every recorded model and configuration change.
         *
         * The method is independent of the document transaction state. Multiple
         * changes are coalesced, and unchanged final state produces no Piece
         * notifications. Until this method is called, pieces() continues to
         * describe the last applied state.
         */
        Q_INVOKABLE void update();

    signals:
        /** @brief Emitted after the requested bound clip changes. */
        void singingClipChanged(SingingClip *singingClip);

        /** @brief Emitted after the base padding accepts a new value. */
        void paddingBaseChanged(double paddingBase);

        /** @brief Emitted after the per-phoneme additional padding accepts a new value. */
        void paddingAdditionalChanged(double paddingAdditional);

        /** @brief Emitted after the gap/cap padding accepts a new value. */
        void paddingGapChanged(double paddingGap);

        /** @brief Emitted after the exact-match rest lyric list changes. */
        void restLyricsChanged(const QStringList &restLyrics);

        /**
         * @brief Emitted before piece is added; piece is not yet in pieces().
         */
        void pieceAboutToInsert(Piece *piece);

        /**
         * @brief Emitted after piece has been added in time order.
         */
        void pieceInserted(Piece *piece);

        /**
         * @brief Emitted before piece is removed; piece is still in pieces().
         */
        void pieceAboutToRemove(Piece *piece);

        /**
         * @brief Emitted after piece is removed but before the object is destroyed.
         */
        void pieceRemoved(Piece *piece);

        /**
         * @brief Emitted once after a rebuild changes list membership.
         */
        void piecesChanged();

    private:
        QScopedPointer<PieceDividerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PIECEDIVIDER_H
