#ifndef DSPXMODEL_PIECEDIVIDER_H
#define DSPXMODEL_PIECEDIVIDER_H

#include <QList>
#include <QObject>
#include <QScopedPointer>
#include <QStringList>

#include <dspxmodelPiece/DSPXModelPieceGlobal.h>

namespace dspx {

    class SingingClip;
    class Piece;
    class PieceDividerPrivate;

    class DSPXMODEL_PIECE_EXPORT PieceDivider : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(PieceDivider)
        Q_PROPERTY(SingingClip *singingClip READ singingClip WRITE setSingingClip NOTIFY singingClipChanged)
        Q_PROPERTY(double paddingBase READ paddingBase WRITE setPaddingBase NOTIFY paddingBaseChanged)
        Q_PROPERTY(double paddingAdditional READ paddingAdditional WRITE setPaddingAdditional NOTIFY paddingAdditionalChanged)
        Q_PROPERTY(double pieceGap READ pieceGap WRITE setPieceGap NOTIFY pieceGapChanged)
        Q_PROPERTY(QStringList restLyrics READ restLyrics WRITE setRestLyrics NOTIFY restLyricsChanged)
        Q_PROPERTY(QList<Piece *> piece READ piece NOTIFY pieceChanged)
    public:
        explicit PieceDivider(QObject *parent = nullptr);
        ~PieceDivider() override;

        SingingClip *singingClip() const;
        void setSingingClip(SingingClip *singingClip);

        double paddingBase() const;
        /**
         * @pre paddingBase is finite and paddingBase >= 0.
         */
        void setPaddingBase(double paddingBase);

        double paddingAdditional() const;
        /**
         * @pre paddingAdditional is finite and paddingAdditional >= 0.
         */
        void setPaddingAdditional(double paddingAdditional);

        double pieceGap() const;
        /**
         * @pre pieceGap is finite and pieceGap >= 0.
         */
        void setPieceGap(double pieceGap);

        QStringList restLyrics() const;
        void setRestLyrics(const QStringList &restLyrics);

        QList<Piece *> piece() const;

    signals:
        void singingClipChanged(SingingClip *singingClip);
        void paddingBaseChanged(double paddingBase);
        void paddingAdditionalChanged(double paddingAdditional);
        void pieceGapChanged(double pieceGap);
        void restLyricsChanged(const QStringList &restLyrics);
        void pieceChanged();
        void pieceAdded(Piece *piece);
        void pieceRemoved(Piece *piece);

    private:
        QScopedPointer<PieceDividerPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PIECEDIVIDER_H
