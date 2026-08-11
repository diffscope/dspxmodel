#ifndef DSPXMODEL_PIECEDIVIDER_P_H
#define DSPXMODEL_PIECEDIVIDER_P_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <map>
#include <utility>

#include "PieceUtils_p.h"

namespace dspx {

    class Note;
    class Phoneme;
    class PhonemeSequence;
    class Piece;
    class PieceDivider;
    class SingingClip;
    class Tempo;

    struct PiecePhonemeState {
        quint64 id = 0;
        int start = 0;
        bool onset = false;

        bool operator==(const PiecePhonemeState &) const = default;
    };

    struct PieceNoteState {
        quint64 id = 0;
        int position = 0;
        int length = 0;
        QString lyric;
        int leadingNonOnsetPhonemes = 0;

        bool operator==(const PieceNoteState &) const = default;
    };

    struct PieceNoteOrder {
        int position = 0;
        quint64 id = 0;

        bool operator<(const PieceNoteOrder &other) const {
            return position < other.position || (position == other.position && id < other.id);
        }
    };

    struct PieceBlueprint {
        double position = 0.0;
        double length = 0.0;
        int firstNotePosition = 0;
        int lastNotePosition = 0;
        QList<quint64> noteIds;
        Piece *preservedPiece = nullptr;
    };

    class PieceDividerPrivate {
        Q_DECLARE_PUBLIC(PieceDivider)
    public:
        explicit PieceDividerPrivate(PieceDivider *q);
        ~PieceDividerPrivate();

        void bind(SingingClip *clip);
        void installWatchers();
        bool beginWatching(QObject *object);
        void installNoteWatcher(Note *note);
        void installPhonemeWatcher(PhonemeSequence *sequence, quint64 noteId);
        void installPhonemeWatcher(Phoneme *phoneme, quint64 noteId);
        void installTempoWatcher(Tempo *tempo);

        PieceNoteState captureNote(Note *note) const;
        QList<PiecePhonemeState> capturePhonemes(PhonemeSequence *sequence) const;

        void markNote(quint64 id);
        void discardPending();
        void initializeAppliedState();
        void updatePending();
        bool configurationDiffers() const;
        std::pair<int, int> timeMapAffectedPositions(const PieceTimeMap &oldMap,
                                                     int oldClipStart,
                                                     const PieceTimeMap &newMap,
                                                     int newClipStart) const;

        QList<PieceBlueprint> buildAllBlueprints() const;
        QList<PieceBlueprint> buildLocalBlueprints(int firstAffectedPosition, int lastAffectedPosition) const;
        QList<PieceBlueprint> partitionFrom(std::map<PieceNoteOrder, quint64>::const_iterator first,
                                             std::map<PieceNoteOrder, quint64>::const_iterator last,
                                             int stopAfterPosition,
                                             int oldSearchStart,
                                             int *matchedOldIndex) const;
        PieceBlueprint blueprintFromPiece(Piece *piece) const;
        bool blueprintExactlyEqualsPiece(const PieceBlueprint &blueprint, Piece *piece) const;
        void reconcile(const QList<PieceBlueprint> &blueprints);
        QList<Piece *> chooseReusablePieces(const QList<Piece *> &oldPieces,
                                            const QList<PieceBlueprint> &blueprints) const;

        PieceDivider *q_ptr;
        QPointer<SingingClip> singingClip;
        QPointer<SingingClip> appliedSingingClip;
        double paddingBase = 0.0;
        double paddingAdditional = 0.0;
        double paddingGap = 0.0;
        QStringList restLyrics;
        double appliedPaddingBase = 0.0;
        double appliedPaddingAdditional = 0.0;
        double appliedPaddingGap = 0.0;
        QStringList appliedRestLyrics;
        QList<Piece *> pieces;

        QObject *watchContext = nullptr;
        QHash<quint64, QPointer<Note>> notePointers;
        QHash<quint64, quint64> phonemeOwners;
        QSet<QObject *> watchedObjects;

        PieceTimeMapCache timeMapCache;
        PieceTimeMap committedTimeMap;
        int committedClipStart = 0;
        QHash<quint64, PieceNoteState> committedNotes;
        std::map<PieceNoteOrder, quint64> orderedNotes;
        QHash<quint64, Piece *> committedNoteOwners;

        QSet<quint64> pendingNoteIds;
        QSet<quint64> pendingTempoIds;
        bool clipStartDirty = false;
        bool bindingDirty = false;
    };

}

#endif // DSPXMODEL_PIECEDIVIDER_P_H
