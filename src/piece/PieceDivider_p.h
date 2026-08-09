#ifndef DSPXMODEL_PIECEDIVIDER_P_H
#define DSPXMODEL_PIECEDIVIDER_P_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <dini/event.h>

#include <map>

#include "PieceChange.h"

namespace dini {
    struct EngineEvent;
}

namespace dspx {

    class AnchorNode;
    class AnchorNodeSequence;
    class FreeValueDataArray;
    class Note;
    class Parameter;
    class Phoneme;
    class PhonemeSequence;
    class Piece;
    class PieceDivider;
    class SingingClip;
    class Tempo;
    class VibratoPointDataArray;

    struct PieceTimeMap {
        struct Segment {
            double tick = 0.0;
            double milliseconds = 0.0;
            double bpm = 120.0;

            bool operator==(const Segment &) const = default;
        };

        QVector<Segment> segments;

        double tickToMilliseconds(double tick) const;
        double millisecondsToTick(double milliseconds) const;
    };

    struct PiecePhonemeState {
        quint64 id = 0;
        QString language;
        QString token;
        int start = 0;
        bool onset = false;

        bool operator==(const PiecePhonemeState &) const = default;
    };

    struct PieceNoteState {
        quint64 id = 0;
        int position = 0;
        int length = 0;
        int keyNumber = 0;
        int centShift = 0;
        QString lyric;
        QString language;
        QString editedPronunciation;
        int vibratoAmplitude = 0;
        double vibratoEnd = 0.0;
        double vibratoFrequency = 0.0;
        int vibratoOffset = 0;
        double vibratoPhase = 0.0;
        double vibratoStart = 0.0;
        QList<QPointF> vibratoAmplitudePoints;
        QList<QPointF> vibratoFrequencyPoints;
        QList<PiecePhonemeState> originalPhonemes;
        QList<PiecePhonemeState> editedPhonemes;
        int leadingNonOnsetPhonemes = 0;
        double startMilliseconds = 0.0;
        double endMilliseconds = 0.0;

        bool vibratoEquals(const PieceNoteState &other) const;
    };

    struct PieceNoteOrder {
        int position = 0;
        quint64 id = 0;

        bool operator<(const PieceNoteOrder &other) const {
            return position < other.position || (position == other.position && id < other.id);
        }
    };

    struct PieceBlueprint {
        double startMilliseconds = 0.0;
        double endMilliseconds = 0.0;
        double position = 0.0;
        double length = 0.0;
        int firstNotePosition = 0;
        int lastNotePosition = 0;
        QList<quint64> noteIds;
        Piece *preservedPiece = nullptr;
    };

    struct PieceParameterImpact {
        QString name;
        double firstTick = 0.0;
        double lastTick = 0.0;
        bool throughEnd = false;
        bool wholeDomain = false;
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
        void installPhonemeWatcher(PhonemeSequence *sequence, quint64 noteId, bool original);
        void installPhonemeWatcher(Phoneme *phoneme, quint64 noteId, bool original);
        void installTempoWatcher(Tempo *tempo);
        void installParameterWatcher(const QString &name, Parameter *parameter);
        void installAnchorWatcher(AnchorNodeSequence *sequence, Parameter *parameter);
        void installAnchorWatcher(AnchorNode *node, Parameter *parameter);
        void installFreeWatcher(FreeValueDataArray *array, Parameter *parameter);

        PieceTimeMap captureTimeMap() const;
        PieceNoteState captureNote(Note *note, const PieceTimeMap &map, int absoluteClipStart) const;
        QList<PiecePhonemeState> capturePhonemes(PhonemeSequence *sequence) const;

        void markNote(quint64 id);
        void markParameter(Parameter *parameter, double firstTick, double lastTick, bool throughEnd = false);
        void markParameterWhole(Parameter *parameter, const QString &name = {});
        QString parameterName(Parameter *parameter) const;
        bool transactionIsActive() const;
        void handleEngineEvent(const dini::EngineEvent &event);
        void discardPending();
        void commitPending();

        void initializeCommittedState(bool notify);
        void rebuildForConfigurationChange();
        QList<PieceBlueprint> buildAllBlueprints() const;
        QList<PieceBlueprint> buildLocalBlueprints(int firstAffectedPosition, int lastAffectedPosition) const;
        QList<PieceBlueprint> partitionFrom(std::map<PieceNoteOrder, quint64>::const_iterator first,
                                             std::map<PieceNoteOrder, quint64>::const_iterator last,
                                             int stopAfterPosition,
                                             int oldSearchStart,
                                             int *matchedOldIndex) const;
        PieceBlueprint blueprintFromPiece(Piece *piece) const;
        bool blueprintExactlyEqualsPiece(const PieceBlueprint &blueprint, Piece *piece) const;

        void reconcile(const QList<PieceBlueprint> &blueprints,
                       QHash<Piece *, PieceChange::ChangeTypes> changes = {},
                       QHash<Piece *, QSet<QString>> parameterNames = {});
        QList<Piece *> chooseReusablePieces(const QList<Piece *> &oldPieces,
                                            const QList<PieceBlueprint> &blueprints) const;
        void addParameterChanges(QHash<Piece *, PieceChange::ChangeTypes> &changes,
                                 QHash<Piece *, QSet<QString>> &names,
                                 const QList<Piece *> &oldPieces,
                                 const QHash<Piece *, PieceBlueprint> &oldSnapshots,
                                 const PieceTimeMap &oldMap,
                                 int oldClipStart,
                                 const PieceTimeMap &newMap,
                                 int newClipStart,
                                 bool globalTimeChange) const;

        PieceDivider *q_ptr;
        QPointer<SingingClip> singingClip;
        double paddingBase = 0.0;
        double paddingAdditional = 0.0;
        double paddingGap = 0.0;
        QStringList restLyrics;
        QList<Piece *> pieces;

        QObject *watchContext = nullptr;
        dini::Subscription subscription;
        QHash<quint64, QPointer<Note>> notePointers;
        QHash<Parameter *, QString> parameterNames;
        QHash<quint64, int> anchorPositions;
        QSet<QObject *> watchedObjects;

        PieceTimeMap committedTimeMap;
        int committedClipStart = 0;
        QHash<quint64, PieceNoteState> committedNotes;
        std::map<PieceNoteOrder, quint64> orderedNotes;
        QHash<quint64, Piece *> committedNoteOwners;

        QSet<quint64> pendingNoteIds;
        QList<PieceParameterImpact> pendingParameterImpacts;
        bool tempoDirty = false;
        bool clipStartDirty = false;
        bool configurationChangedInTransaction = false;
    };

}

#endif // DSPXMODEL_PIECEDIVIDER_P_H
