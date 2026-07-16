#ifndef DSPXMODEL_PIECEDIVIDER_P_H
#define DSPXMODEL_PIECEDIVIDER_P_H

#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <QStringList>

#include <dini/event.h>

#include <dspxmodelPiece/PieceDivider.h>

namespace dspx {

    class Piece;
    class SingingClip;

    class PieceDividerPrivate {
        Q_DECLARE_PUBLIC(PieceDivider)
    public:
        explicit PieceDividerPrivate(PieceDivider *q);
        ~PieceDividerPrivate();

        void bindSingingClip(SingingClip *clip);
        void handleEngineEvent(const dini::EngineEvent &event);
        bool eventAffectsSingingClip(const dini::EngineEvent &event) const;
        void recalculate();

        PieceDivider *q_ptr = nullptr;
        QPointer<SingingClip> singingClip;
        double paddingBase = 0;
        double paddingAdditional = 0;
        double pieceGap = 0;
        QStringList restLyrics;
        QList<Piece *> pieces;
        dini::Subscription engineSubscription;
        QMetaObject::Connection destroyedConnection;
    };

}

#endif // DSPXMODEL_PIECEDIVIDER_P_H
