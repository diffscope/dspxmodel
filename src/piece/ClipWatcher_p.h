#ifndef DSPXMODEL_CLIPWATCHER_P_H
#define DSPXMODEL_CLIPWATCHER_P_H

#include "ClipChange.h"
#include "PieceUtils_p.h"

#include <limits>
#include <map>
#include <optional>
#include <utility>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVariant>

namespace dspx {

    class AnchorNode;
    class AnchorNodeSequence;
    class ClipWatcher;
    class FreeValueDataArray;
    class Note;
    class Parameter;
    class Phoneme;
    class PhonemeSequence;
    class SingingClip;
    class Singer;
    class SingerList;
    class Sources;
    class Tempo;

    struct WatcherPhonemeState {
        quint64 id = 0;
        QString language;
        QString token;
        int start = 0;
        bool onset = false;

        bool operator==(const WatcherPhonemeState &) const = default;
    };

    struct WatcherNoteState {
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
        QList<WatcherPhonemeState> editedPhonemes;

        bool vibratoEquals(const WatcherNoteState &other) const;
    };

    struct WatcherAnchorState {
        quint64 id = 0;
        int x = 0;
        int y = 0;
        int mode = 0;

        bool operator==(const WatcherAnchorState &) const = default;
    };

    struct WatcherAnchorOrder {
        int x = 0;
        quint64 id = 0;

        bool operator<(const WatcherAnchorOrder &other) const {
            return x < other.x || (x == other.x && id < other.id);
        }
    };

    struct WatcherAnchorLayer {
        QHash<quint64, WatcherAnchorState> states;
        std::map<WatcherAnchorOrder, quint64> order;
    };

    struct WatcherParameterState {
        quint64 id = 0;
        QString name;
        QList<QVariant> freeEdited;
        QList<QVariant> freeTransform;
        WatcherAnchorLayer anchorEdited;
        WatcherAnchorLayer anchorTransform;
    };

    struct WatcherFreeDirty {
        int first = std::numeric_limits<int>::max();
        int last = std::numeric_limits<int>::min();
        bool throughEnd = false;

        bool isDirty() const { return first != std::numeric_limits<int>::max(); }
        void include(int firstIndex, int lastIndex, bool suffix);
    };

    struct WatcherParameterDirty {
        bool membership = false;
        WatcherFreeDirty freeEdited;
        WatcherFreeDirty freeTransform;
        QSet<quint64> anchorEdited;
        QSet<quint64> anchorTransform;
    };

    class ClipWatcherPrivate {
        Q_DECLARE_PUBLIC(ClipWatcher)
    public:
        explicit ClipWatcherPrivate(ClipWatcher *q);
        ~ClipWatcherPrivate();

        void bind(SingingClip *clip);
        void captureBaseline();
        void installWatchers();
        bool beginWatching(QObject *object);
        void installNoteWatcher(Note *note);
        void installPhonemeWatcher(PhonemeSequence *sequence, quint64 noteId);
        void installPhonemeWatcher(Phoneme *phoneme, quint64 noteId);
        void installTempoWatcher(Tempo *tempo);
        void installParameterWatcher(const QString &name, Parameter *parameter);
        void installSourceWatchers(Sources *sources);
        void installSingerListWatchers(SingerList *list);
        void installSingerWatcher(Singer *singer);
        void installFreeWatcher(FreeValueDataArray *array, quint64 parameterId, bool edited);
        void installAnchorWatcher(AnchorNodeSequence *sequence, quint64 parameterId, bool edited);
        void installAnchorWatcher(AnchorNode *node, quint64 parameterId, bool edited);

        void markNote(quint64 id);
        void markParameterMembership(quint64 id);
        void markFree(quint64 id, bool edited, int first, int last, bool throughEnd);
        void markAnchor(quint64 parameterId, bool edited, quint64 anchorId);
        void clearPending();

        WatcherNoteState captureNote(Note *note) const;
        QList<WatcherPhonemeState> capturePhonemes(PhonemeSequence *sequence) const;
        WatcherAnchorState captureAnchor(AnchorNode *node) const;
        WatcherAnchorLayer captureAnchors(AnchorNodeSequence *sequence) const;
        WatcherParameterState captureParameter(const QString &name, Parameter *parameter) const;
        QByteArray captureSources() const;
        std::optional<WatcherNoteState> currentNote(quint64 id) const;
        std::optional<WatcherParameterState> currentParameter(quint64 id) const;

        ClipChange takeChanges();

        ClipWatcher *q_ptr;
        QPointer<SingingClip> singingClip;
        QObject *watchContext = nullptr;
        QSet<QObject *> watchedObjects;
        QHash<quint64, QPointer<Note>> notePointers;
        QHash<quint64, quint64> phonemeOwners;
        QHash<quint64, QPointer<Parameter>> parameterPointers;
        QHash<quint64, QPointer<AnchorNode>> anchorPointers;
        QHash<quint64, std::pair<quint64, bool>> anchorOwners;

        PieceTimeMapCache timeMapCache;
        PieceTimeMap baselineTimeMap;
        int baselineClipStart = 0;
        int baselinePosition = 0;
        int baselineLength = 0;
        int baselineVisibleStart = 0;
        int baselineVisibleLength = 0;
        QByteArray baselineSources;
        QHash<quint64, WatcherNoteState> baselineNotes;
        std::multimap<int, quint64> noteStarts;
        std::multimap<int, quint64> noteEnds;
        QHash<quint64, WatcherParameterState> baselineParameters;

        QSet<quint64> pendingNoteIds;
        QHash<quint64, WatcherParameterDirty> pendingParameters;
        QSet<quint64> pendingTempoIds;
        bool clipStartDirty = false;
        bool clipTimingDirty = false;
        bool sourcesDirty = false;
    };

}

#endif // DSPXMODEL_CLIPWATCHER_P_H
