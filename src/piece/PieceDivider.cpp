#include "PieceDivider.h"

#include "Piece.h"
#include "Piece_p.h"
#include "PieceDivider_p.h"

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

#include <QPointer>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

namespace dspx {

    namespace {

        constexpr double ticksPerBeat = 480.0;
        constexpr double defaultTempo = 120.0;

        double millisecondsPerTick(double bpm) {
            return 60000.0 / (ticksPerBeat * bpm);
        }

        template <class T>
        bool listsEqual(const QList<T> &left, const QList<T> &right) {
            return left == right;
        }

        int sharedNoteCount(const QList<quint64> &left, const QList<quint64> &right) {
            int result = 0;
            auto leftIt = left.cbegin();
            auto rightIt = right.cbegin();
            while (leftIt != left.cend() && rightIt != right.cend()) {
                if (*leftIt == *rightIt) {
                    ++result;
                    ++leftIt;
                    ++rightIt;
                } else if (*leftIt < *rightIt) {
                    ++leftIt;
                } else {
                    ++rightIt;
                }
            }
            return result;
        }

        QList<quint64> sortedIds(QList<quint64> ids) {
            std::sort(ids.begin(), ids.end());
            return ids;
        }

        dini::ContainerId operationContainerId(const dini::ChangeOperation &operation) {
            return std::visit([](const auto &change) -> dini::ContainerId {
                using Change = std::decay_t<decltype(change)>;
                if constexpr (std::is_same_v<Change, dini::ItemInsertedChange> ||
                              std::is_same_v<Change, dini::ItemRemovedChange> ||
                              std::is_same_v<Change, dini::CascadeRemovedChange>) {
                    return change.item.containerId;
                } else if constexpr (std::is_same_v<Change, dini::ColumnUpdatedChange> ||
                                     std::is_same_v<Change, dini::ComputedColumnUpdatedChange>) {
                    return change.column.containerId();
                } else {
                    return change.list.containerId();
                }
            }, operation.payload());
        }

        bool hasCommittedParameterOperation(const dini::ChangeSet &changeSet) {
            const dini::ContainerId parameterTable = Schema::parameterTable().containerId();
            const dini::ContainerId anchorTable = Schema::anchorNodeTable().containerId();
            const dini::ContainerId anchorRelationTable = Schema::parameterAnchorNodeRelationTable().containerId();
            const dini::ContainerId freeList = Schema::freeValueList().containerId();
            return std::any_of(changeSet.operations().cbegin(), changeSet.operations().cend(),
                               [=](const dini::ChangeOperation &operation) {
                                   const auto container = operationContainerId(operation);
                                   return container == parameterTable || container == anchorTable ||
                                          container == anchorRelationTable || container == freeList;
                               });
        }

    }

    double PieceTimeMap::tickToMilliseconds(double tick) const {
        if (segments.isEmpty()) {
            return tick * millisecondsPerTick(defaultTempo);
        }
        if (tick < 0.0) {
            return tick * millisecondsPerTick(segments.constFirst().bpm);
        }
        const auto it = std::upper_bound(segments.cbegin(), segments.cend(), tick,
                                         [](double value, const Segment &segment) { return value < segment.tick; });
        const auto &segment = it == segments.cbegin() ? segments.constFirst() : *std::prev(it);
        return segment.milliseconds + (tick - segment.tick) * millisecondsPerTick(segment.bpm);
    }

    double PieceTimeMap::millisecondsToTick(double milliseconds) const {
        if (segments.isEmpty()) {
            return milliseconds / millisecondsPerTick(defaultTempo);
        }
        if (milliseconds < 0.0) {
            return milliseconds / millisecondsPerTick(segments.constFirst().bpm);
        }
        const auto it = std::upper_bound(segments.cbegin(), segments.cend(), milliseconds,
                                         [](double value, const Segment &segment) { return value < segment.milliseconds; });
        const auto &segment = it == segments.cbegin() ? segments.constFirst() : *std::prev(it);
        return segment.tick + (milliseconds - segment.milliseconds) / millisecondsPerTick(segment.bpm);
    }

    bool PieceNoteState::vibratoEquals(const PieceNoteState &other) const {
        return vibratoAmplitude == other.vibratoAmplitude && vibratoEnd == other.vibratoEnd &&
               vibratoFrequency == other.vibratoFrequency && vibratoOffset == other.vibratoOffset &&
               vibratoPhase == other.vibratoPhase && vibratoStart == other.vibratoStart &&
               vibratoAmplitudePoints == other.vibratoAmplitudePoints &&
               vibratoFrequencyPoints == other.vibratoFrequencyPoints;
    }

    PieceDividerPrivate::PieceDividerPrivate(PieceDivider *q)
        : q_ptr(q) {
    }

    PieceDividerPrivate::~PieceDividerPrivate() {
        subscription.disconnect();
        delete watchContext;
        for (Piece *piece : std::as_const(pieces)) {
            delete piece;
        }
    }

    PieceTimeMap PieceDividerPrivate::captureTimeMap() const {
        PieceTimeMap result;
        if (!singingClip) {
            result.segments.append({0.0, 0.0, defaultTempo});
            return result;
        }

        struct TempoPoint {
            int position;
            double bpm;
        };
        QVector<TempoPoint> points;
        for (Tempo *tempo : singingClip->model()->tempos()->asRange()) {
            points.append({tempo->position(), tempo->value()});
        }
        std::sort(points.begin(), points.end(), [](const TempoPoint &left, const TempoPoint &right) {
            return left.position < right.position;
        });

        double tempoAtZero = defaultTempo;
        for (const auto &point : points) {
            if (point.position > 0) {
                break;
            }
            tempoAtZero = point.bpm;
        }
        result.segments.append({0.0, 0.0, tempoAtZero});

        double previousTick = 0.0;
        double previousMilliseconds = 0.0;
        double previousTempo = tempoAtZero;
        for (const auto &point : points) {
            if (point.position <= 0) {
                continue;
            }
            const double millisecondsAtPoint = previousMilliseconds +
                                               (static_cast<double>(point.position) - previousTick) *
                                                   millisecondsPerTick(previousTempo);
            if (point.bpm == previousTempo) {
                continue;
            }
            result.segments.append({static_cast<double>(point.position), millisecondsAtPoint, point.bpm});
            previousTick = static_cast<double>(point.position);
            previousMilliseconds = millisecondsAtPoint;
            previousTempo = point.bpm;
        }
        return result;
    }

    QList<PiecePhonemeState> PieceDividerPrivate::capturePhonemes(PhonemeSequence *sequence) const {
        QList<PiecePhonemeState> result;
        if (!sequence) {
            return result;
        }
        result.reserve(sequence->size());
        for (Phoneme *phoneme : sequence->asRange()) {
            result.append({phoneme->handle().d, phoneme->language(), phoneme->token(), phoneme->start(), phoneme->onset()});
        }
        std::sort(result.begin(), result.end(), [](const PiecePhonemeState &left, const PiecePhonemeState &right) {
            if (left.start != right.start) {
                return left.start < right.start;
            }
            if (left.onset != right.onset) {
                return left.onset && !right.onset;
            }
            return left.id < right.id;
        });
        return result;
    }

    PieceNoteState PieceDividerPrivate::captureNote(Note *note, const PieceTimeMap &map, int absoluteClipStart) const {
        PieceNoteState result;
        result.id = note->handle().d;
        result.position = note->position();
        result.length = note->length();
        result.keyNumber = note->keyNumber();
        result.centShift = note->centShift();
        result.lyric = note->lyric();
        result.language = note->language();
        result.editedPronunciation = note->editedPronunciation();
        result.vibratoAmplitude = note->vibratoAmplitude();
        result.vibratoEnd = note->vibratoEnd();
        result.vibratoFrequency = note->vibratoFrequency();
        result.vibratoOffset = note->vibratoOffset();
        result.vibratoPhase = note->vibratoPhase();
        result.vibratoStart = note->vibratoStart();
        result.vibratoAmplitudePoints = note->vibratoAmplitudeControlPoints()->items();
        result.vibratoFrequencyPoints = note->vibratoFrequencyControlPoints()->items();
        result.originalPhonemes = capturePhonemes(note->originalPhonemes());
        result.editedPhonemes = capturePhonemes(note->editedPhonemes());
        for (const auto &phoneme : std::as_const(result.originalPhonemes)) {
            if (phoneme.onset) {
                break;
            }
            ++result.leadingNonOnsetPhonemes;
        }
        result.startMilliseconds = map.tickToMilliseconds(static_cast<double>(absoluteClipStart) + result.position);
        result.endMilliseconds = map.tickToMilliseconds(static_cast<double>(absoluteClipStart) + result.position + result.length);
        return result;
    }

    void PieceDividerPrivate::markNote(quint64 id) {
        if (id) {
            pendingNoteIds.insert(id);
        }
    }

    QString PieceDividerPrivate::parameterName(Parameter *parameter) const {
        const auto it = parameterNames.constFind(parameter);
        if (it != parameterNames.cend()) {
            return *it;
        }
        if (parameter && parameter->parameterMap() == singingClip->parameters()) {
            for (const QString &key : singingClip->parameters()->keys()) {
                if (singingClip->parameters()->item(key) == parameter) {
                    return key;
                }
            }
        }
        return {};
    }

    void PieceDividerPrivate::markParameter(Parameter *parameter, double firstTick, double lastTick, bool throughEnd) {
        const QString name = parameterName(parameter);
        if (name.isEmpty()) {
            return;
        }
        if (lastTick < firstTick) {
            std::swap(firstTick, lastTick);
        }
        pendingParameterImpacts.append({name, firstTick, lastTick, throughEnd, false});
    }

    void PieceDividerPrivate::markParameterWhole(Parameter *parameter, const QString &name) {
        const QString resolvedName = name.isEmpty() ? parameterName(parameter) : name;
        if (!resolvedName.isEmpty()) {
            pendingParameterImpacts.append({resolvedName, 0.0, 0.0, false, true});
        }
    }

    bool PieceDividerPrivate::transactionIsActive() const {
        if (!singingClip) {
            return false;
        }
        const auto *transaction = singingClip->model()->document()->transaction();
        return transaction && transaction->state() == dini::TransactionState::Active;
    }

    bool PieceDividerPrivate::beginWatching(QObject *object) {
        if (!object || !watchContext || watchedObjects.contains(object)) {
            return false;
        }
        watchedObjects.insert(object);
        QObject::connect(object, &QObject::destroyed, watchContext,
                         [this](QObject *destroyed) { watchedObjects.remove(destroyed); });
        return true;
    }

    void PieceDividerPrivate::installPhonemeWatcher(Phoneme *phoneme, quint64 noteId, bool) {
        if (!phoneme || !watchContext) {
            return;
        }
        if (!beginWatching(phoneme)) {
            return;
        }
        auto changed = [this, noteId] { markNote(noteId); };
        QObject::connect(phoneme, &Phoneme::languageChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::startChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::tokenChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::onsetChanged, watchContext, changed);
    }

    void PieceDividerPrivate::installPhonemeWatcher(PhonemeSequence *sequence, quint64 noteId, bool original) {
        if (!sequence || !watchContext) {
            return;
        }
        if (!beginWatching(sequence)) {
            return;
        }
        for (Phoneme *phoneme : sequence->asRange()) {
            installPhonemeWatcher(phoneme, noteId, original);
        }
        QObject::connect(sequence, &PhonemeSequence::itemAboutToRemove, watchContext,
                         [this, noteId](Phoneme *, PhonemeSequence *) { markNote(noteId); });
        QObject::connect(sequence, &PhonemeSequence::itemInserted, watchContext,
                         [this, noteId, original](Phoneme *phoneme, PhonemeSequence *) {
                             installPhonemeWatcher(phoneme, noteId, original);
                             markNote(noteId);
                         });
    }

    void PieceDividerPrivate::installNoteWatcher(Note *note) {
        if (!note || !watchContext) {
            return;
        }
        const quint64 id = note->handle().d;
        notePointers.insert(id, note);
        if (!beginWatching(note)) {
            return;
        }
        auto changed = [this, id] { markNote(id); };
        QObject::connect(note, &Note::centShiftChanged, watchContext, changed);
        QObject::connect(note, &Note::keyNumberChanged, watchContext, changed);
        QObject::connect(note, &Note::languageChanged, watchContext, changed);
        QObject::connect(note, &Note::lengthChanged, watchContext, changed);
        QObject::connect(note, &Note::lyricChanged, watchContext, changed);
        QObject::connect(note, &Note::positionChanged, watchContext, changed);
        QObject::connect(note, &Note::editedPronunciationChanged, watchContext, changed);
        QObject::connect(note, &Note::vibratoAmplitudeChanged, watchContext, changed);
        QObject::connect(note, &Note::vibratoEndChanged, watchContext, changed);
        QObject::connect(note, &Note::vibratoFrequencyChanged, watchContext, changed);
        QObject::connect(note, &Note::vibratoOffsetChanged, watchContext, changed);
        QObject::connect(note, &Note::vibratoPhaseChanged, watchContext, changed);
        QObject::connect(note, &Note::vibratoStartChanged, watchContext, changed);
        installPhonemeWatcher(note->originalPhonemes(), id, true);
        installPhonemeWatcher(note->editedPhonemes(), id, false);
        QObject::connect(note->vibratoAmplitudeControlPoints(), &VibratoPointDataArray::spliced,
                         watchContext, [changed](int, int, const QList<QPointF> &) { changed(); });
        QObject::connect(note->vibratoAmplitudeControlPoints(), &VibratoPointDataArray::rotated,
                         watchContext, [changed](int, int, int) { changed(); });
        QObject::connect(note->vibratoFrequencyControlPoints(), &VibratoPointDataArray::spliced,
                         watchContext, [changed](int, int, const QList<QPointF> &) { changed(); });
        QObject::connect(note->vibratoFrequencyControlPoints(), &VibratoPointDataArray::rotated,
                         watchContext, [changed](int, int, int) { changed(); });
    }

    void PieceDividerPrivate::installTempoWatcher(Tempo *tempo) {
        if (!tempo || !watchContext) {
            return;
        }
        if (!beginWatching(tempo)) {
            return;
        }
        auto changed = [this] { tempoDirty = true; };
        QObject::connect(tempo, &Tempo::positionChanged, watchContext, changed);
        QObject::connect(tempo, &Tempo::valueChanged, watchContext, changed);
    }

    void PieceDividerPrivate::installFreeWatcher(FreeValueDataArray *array, Parameter *parameter) {
        if (!array || !watchContext) {
            return;
        }
        if (!beginWatching(array)) {
            return;
        }
        QObject::connect(array, &FreeValueDataArray::spliced, watchContext,
                         [this, parameter](int index, int removed, const QList<QVariant> &values) {
                             const int first = std::max(0, index - 1);
                             const int inserted = static_cast<int>(values.size());
                             if (removed != inserted) {
                                 markParameter(parameter, first * FreeValueDataArray::step(), 0.0, true);
                             } else {
                                 const int last = index + std::max(removed, inserted) + 1;
                                 markParameter(parameter, first * FreeValueDataArray::step(), last * FreeValueDataArray::step());
                             }
                         });
        QObject::connect(array, &FreeValueDataArray::rotated, watchContext,
                         [this, parameter](int left, int, int right) {
                             markParameter(parameter,
                                           std::max(0, left - 1) * FreeValueDataArray::step(),
                                           (right + 1) * FreeValueDataArray::step());
                         });
    }

    void PieceDividerPrivate::installAnchorWatcher(AnchorNode *node, Parameter *parameter) {
        if (!node || !watchContext) {
            return;
        }
        if (!beginWatching(node)) {
            return;
        }
        anchorPositions.insert(node->handle().d, node->x());
        const quint64 nodeId = node->handle().d;
        QObject::connect(node, &QObject::destroyed, watchContext,
                         [this, nodeId] { anchorPositions.remove(nodeId); });
        auto influenceRange = [node](int extraX) {
            int first = std::min(node->x(), extraX);
            int last = std::max(node->x(), extraX);
            AnchorNode *cursor = node;
            for (int i = 0; i < 2 && cursor->previousItem(); ++i) {
                cursor = cursor->previousItem();
                first = std::min(first, cursor->x());
            }
            cursor = node;
            for (int i = 0; i < 2 && cursor->nextItem(); ++i) {
                cursor = cursor->nextItem();
                last = std::max(last, cursor->x());
            }
            return std::pair(first, last);
        };
        QObject::connect(node, &AnchorNode::xChanged, watchContext,
                         [this, node, parameter, influenceRange](int x) {
                             const int oldX = anchorPositions.value(node->handle().d, x);
                             anchorPositions.insert(node->handle().d, x);
                             const auto range = influenceRange(oldX);
                             markParameter(parameter, range.first, range.second);
                         });
        auto valueChanged = [this, node, parameter, influenceRange] {
            const auto range = influenceRange(node->x());
            markParameter(parameter, range.first, range.second);
        };
        QObject::connect(node, &AnchorNode::yChanged, watchContext, valueChanged);
        QObject::connect(node, &AnchorNode::interpolationModeChanged, watchContext, valueChanged);
    }

    void PieceDividerPrivate::installAnchorWatcher(AnchorNodeSequence *sequence, Parameter *parameter) {
        if (!sequence || !watchContext) {
            return;
        }
        if (!beginWatching(sequence)) {
            return;
        }
        for (AnchorNode *node : sequence->asRange()) {
            installAnchorWatcher(node, parameter);
        }
        QObject::connect(sequence, &AnchorNodeSequence::itemAboutToRemove, watchContext,
                         [this, parameter](AnchorNode *node, AnchorNodeSequence *) {
                             int first = node->x();
                             int last = node->x();
                             AnchorNode *cursor = node;
                             for (int i = 0; i < 2 && cursor->previousItem(); ++i) {
                                 cursor = cursor->previousItem();
                                 first = cursor->x();
                             }
                             cursor = node;
                             for (int i = 0; i < 2 && cursor->nextItem(); ++i) {
                                 cursor = cursor->nextItem();
                                 last = cursor->x();
                             }
                             markParameter(parameter, first, last);
                         });
        QObject::connect(sequence, &AnchorNodeSequence::itemInserted, watchContext,
                         [this, parameter](AnchorNode *node, AnchorNodeSequence *) {
                             installAnchorWatcher(node, parameter);
                             int first = node->previousItem() && node->previousItem()->previousItem()
                                             ? node->previousItem()->previousItem()->x()
                                             : (node->previousItem() ? node->previousItem()->x() : node->x());
                             int last = node->nextItem() && node->nextItem()->nextItem()
                                            ? node->nextItem()->nextItem()->x()
                                            : (node->nextItem() ? node->nextItem()->x() : node->x());
                             markParameter(parameter, first, last);
                         });
    }

    void PieceDividerPrivate::installParameterWatcher(const QString &name, Parameter *parameter) {
        if (!parameter || !watchContext) {
            return;
        }
        parameterNames.insert(parameter, name);
        if (!beginWatching(parameter)) {
            return;
        }
        QObject::connect(parameter, &QObject::destroyed, watchContext,
                         [this, parameter] { parameterNames.remove(parameter); });
        installFreeWatcher(parameter->freeEdited(), parameter);
        installFreeWatcher(parameter->freeTransform(), parameter);
        installAnchorWatcher(parameter->anchorEdited(), parameter);
        installAnchorWatcher(parameter->anchorTransform(), parameter);
    }

    void PieceDividerPrivate::installWatchers() {
        delete watchContext;
        watchContext = nullptr;
        subscription.disconnect();
        notePointers.clear();
        parameterNames.clear();
        anchorPositions.clear();
        watchedObjects.clear();
        if (!singingClip) {
            return;
        }

        watchContext = new QObject(q_ptr);
        QObject::connect(singingClip, &QObject::destroyed, watchContext, [this] {
            subscription.disconnect();
            singingClip = nullptr;
            initializeCommittedState(true);
            emit q_ptr->singingClipChanged(nullptr);
        });
        QObject::connect(singingClip, &SingingClip::startChanged, watchContext,
                         [this](int) { clipStartDirty = true; });

        NoteSequence *notes = singingClip->notes();
        for (Note *note : notes->asRange()) {
            installNoteWatcher(note);
        }
        QObject::connect(notes, &NoteSequence::itemAboutToRemove, watchContext,
                         [this](Note *note, NoteSequence *) { markNote(note->handle().d); });
        QObject::connect(notes, &NoteSequence::itemInserted, watchContext,
                         [this](Note *note, NoteSequence *) {
                             installNoteWatcher(note);
                             markNote(note->handle().d);
                         });

        TempoSequence *tempos = singingClip->model()->tempos();
        for (Tempo *tempo : tempos->asRange()) {
            installTempoWatcher(tempo);
        }
        QObject::connect(tempos, &TempoSequence::itemAboutToRemove, watchContext,
                         [this](Tempo *) { tempoDirty = true; });
        QObject::connect(tempos, &TempoSequence::itemInserted, watchContext,
                         [this](Tempo *tempo) {
                             installTempoWatcher(tempo);
                             tempoDirty = true;
                         });

        ParameterMap *parameters = singingClip->parameters();
        for (const QString &key : parameters->keys()) {
            installParameterWatcher(key, parameters->item(key));
        }
        QObject::connect(parameters, &ParameterMap::itemAboutToRemove, watchContext,
                         [this](const QString &key, Parameter *parameter, ParameterMap *) {
                             parameterNames.insert(parameter, key);
                             markParameterWhole(parameter, key);
                         });
        QObject::connect(parameters, &ParameterMap::itemInserted, watchContext,
                         [this](const QString &key, Parameter *parameter, ParameterMap *) {
                             installParameterWatcher(key, parameter);
                             markParameterWhole(parameter, key);
                         });

        subscription = singingClip->model()->document()->engine()->subscribe(
            [this](const dini::EngineEvent &event) { handleEngineEvent(event); });
    }

    void PieceDividerPrivate::discardPending() {
        pendingNoteIds.clear();
        pendingParameterImpacts.clear();
        tempoDirty = false;
        clipStartDirty = false;
        configurationChangedInTransaction = false;
    }

    void PieceDividerPrivate::handleEngineEvent(const dini::EngineEvent &event) {
        if (event.kind == dini::EventKind::AfterCommit) {
            if (event.changeSet.empty()) {
                discardPending();
            } else {
                if (!pendingParameterImpacts.isEmpty() && !hasCommittedParameterOperation(event.changeSet)) {
                    pendingParameterImpacts.clear();
                }
                commitPending();
            }
        } else if (event.kind == dini::EventKind::Rollback) {
            const bool mustRestore = configurationChangedInTransaction;
            discardPending();
            if (mustRestore) {
                initializeCommittedState(true);
            }
        }
    }

    void PieceDividerPrivate::bind(SingingClip *clip) {
        singingClip = clip;
        installWatchers();
        initializeCommittedState(true);
        if (transactionIsActive()) {
            configurationChangedInTransaction = true;
        }
    }

    void PieceDividerPrivate::initializeCommittedState(bool notify) {
        committedNotes.clear();
        orderedNotes.clear();
        if (!singingClip) {
            if (notify) {
                reconcile({});
            } else {
                for (Piece *piece : std::as_const(pieces)) {
                    delete piece;
                }
                pieces.clear();
                committedNoteOwners.clear();
            }
            discardPending();
            return;
        }

        committedTimeMap = captureTimeMap();
        committedClipStart = singingClip->start();
        for (Note *note : singingClip->notes()->asRange()) {
            const auto state = captureNote(note, committedTimeMap, committedClipStart);
            committedNotes.insert(state.id, state);
            orderedNotes.emplace(PieceNoteOrder {state.position, state.id}, state.id);
            notePointers.insert(state.id, note);
        }
        const auto blueprints = buildAllBlueprints();
        if (notify) {
            reconcile(blueprints);
        }
        discardPending();
    }

    void PieceDividerPrivate::rebuildForConfigurationChange() {
        if (!singingClip) {
            reconcile({});
            return;
        }
        reconcile(buildAllBlueprints());
        if (transactionIsActive()) {
            configurationChangedInTransaction = true;
        }
    }

    PieceBlueprint PieceDividerPrivate::blueprintFromPiece(Piece *piece) const {
        const auto *d = PiecePrivate::get(piece);
        return {d->startMilliseconds,
                d->endMilliseconds,
                d->position,
                d->length,
                d->firstNotePosition,
                d->lastNotePosition,
                d->noteIds,
                piece};
    }

    bool PieceDividerPrivate::blueprintExactlyEqualsPiece(const PieceBlueprint &blueprint, Piece *piece) const {
        const auto *d = PiecePrivate::get(piece);
        return d->startMilliseconds == blueprint.startMilliseconds &&
               d->endMilliseconds == blueprint.endMilliseconds && d->position == blueprint.position &&
               d->length == blueprint.length && d->noteIds == blueprint.noteIds;
    }

    QList<PieceBlueprint> PieceDividerPrivate::partitionFrom(
        std::map<PieceNoteOrder, quint64>::const_iterator first,
        std::map<PieceNoteOrder, quint64>::const_iterator last,
        int stopAfterPosition,
        int oldSearchStart,
        int *matchedOldIndex) const {
        QList<PieceBlueprint> result;
        if (matchedOldIndex) {
            *matchedOldIndex = -1;
        }
        if (first == last) {
            return result;
        }

        struct StartGroup {
            int position = 0;
            double startMilliseconds = 0.0;
            double bodyEndMilliseconds = 0.0;
            double lambda = 0.0;
            double rho = 0.0;
            QList<quint64> noteIds;
        };

        auto readGroup = [this, last](std::map<PieceNoteOrder, quint64>::const_iterator &it) {
            StartGroup group;
            group.position = it->first.position;
            group.lambda = std::numeric_limits<double>::infinity();
            group.bodyEndMilliseconds = -std::numeric_limits<double>::infinity();
            double rhoStart = -std::numeric_limits<double>::infinity();
            group.rho = std::numeric_limits<double>::infinity();
            while (it != last && it->first.position == group.position) {
                const auto noteIt = committedNotes.constFind(it->second);
                if (noteIt != committedNotes.cend()) {
                    const auto &note = *noteIt;
                    group.startMilliseconds = note.startMilliseconds;
                    const bool rest = restLyrics.contains(note.lyric, Qt::CaseSensitive);
                    const double leftPadding = rest
                                                   ? 0.0
                                                   : std::min(paddingBase + note.leadingNonOnsetPhonemes * paddingAdditional,
                                                              paddingGap);
                    const double rightPadding = rest ? 0.0 : paddingBase;
                    group.lambda = std::min(group.lambda, leftPadding);
                    if (note.endMilliseconds > group.bodyEndMilliseconds) {
                        group.bodyEndMilliseconds = note.endMilliseconds;
                        rhoStart = note.startMilliseconds;
                        group.rho = rightPadding;
                    } else if (note.endMilliseconds == group.bodyEndMilliseconds) {
                        if (note.startMilliseconds > rhoStart) {
                            rhoStart = note.startMilliseconds;
                            group.rho = rightPadding;
                        } else if (note.startMilliseconds == rhoStart) {
                            group.rho = std::min(group.rho, rightPadding);
                        }
                    }
                    group.noteIds.append(note.id);
                }
                ++it;
            }
            group.noteIds = sortedIds(std::move(group.noteIds));
            return group;
        };

        auto iterator = first;
        StartGroup currentGroup = readGroup(iterator);
        double bodyStart = currentGroup.startMilliseconds;
        double bodyEnd = currentGroup.bodyEndMilliseconds;
        double lambda = currentGroup.lambda;
        double rho = currentGroup.rho;
        double rhoStart = currentGroup.startMilliseconds;
        int firstPosition = currentGroup.position;
        int lastPosition = currentGroup.position;
        QList<quint64> ids = currentGroup.noteIds;

        auto appendPiece = [this, &result, &bodyStart, &bodyEnd, &lambda, &rho, &firstPosition, &lastPosition, &ids,
                            stopAfterPosition, oldSearchStart, matchedOldIndex]() {
            PieceBlueprint blueprint;
            blueprint.startMilliseconds = bodyStart - lambda;
            blueprint.endMilliseconds = bodyEnd + rho;
            const double startTick = committedTimeMap.millisecondsToTick(blueprint.startMilliseconds);
            const double endTick = committedTimeMap.millisecondsToTick(blueprint.endMilliseconds);
            blueprint.position = startTick - committedClipStart;
            blueprint.length = endTick - startTick;
            blueprint.firstNotePosition = firstPosition;
            blueprint.lastNotePosition = lastPosition;
            blueprint.noteIds = sortedIds(ids);
            result.append(blueprint);

            if (oldSearchStart >= 0) {
                const qsizetype searchIndex = std::clamp<qsizetype>(oldSearchStart, 0, pieces.size());
                const auto searchFirst = pieces.cbegin() + searchIndex;
                const auto candidate = std::lower_bound(searchFirst, pieces.cend(), firstPosition,
                                                        [](Piece *piece, int position) {
                                                            return PiecePrivate::get(piece)->firstNotePosition < position;
                                                        });
                if (candidate != pieces.cend() &&
                    PiecePrivate::get(*candidate)->firstNotePosition == firstPosition &&
                    blueprintExactlyEqualsPiece(blueprint, *candidate)) {
                    result.last().preservedPiece = *candidate;
                    if (matchedOldIndex && firstPosition > stopAfterPosition) {
                        *matchedOldIndex = static_cast<int>(std::distance(pieces.cbegin(), candidate));
                    }
                }
            }
        };

        while (iterator != last) {
            StartGroup next = readGroup(iterator);
            const double gap = next.startMilliseconds - bodyEnd;
            if (gap >= std::max(paddingGap, rho + next.lambda)) {
                appendPiece();
                if (matchedOldIndex && *matchedOldIndex >= 0) {
                    return result;
                }
                bodyStart = next.startMilliseconds;
                bodyEnd = next.bodyEndMilliseconds;
                lambda = next.lambda;
                rho = next.rho;
                rhoStart = next.startMilliseconds;
                firstPosition = next.position;
                lastPosition = next.position;
                ids = next.noteIds;
                continue;
            }

            lastPosition = next.position;
            ids.append(next.noteIds);
            if (next.bodyEndMilliseconds > bodyEnd) {
                bodyEnd = next.bodyEndMilliseconds;
                rho = next.rho;
                rhoStart = next.startMilliseconds;
            } else if (next.bodyEndMilliseconds == bodyEnd) {
                if (next.startMilliseconds > rhoStart) {
                    rho = next.rho;
                    rhoStart = next.startMilliseconds;
                } else if (next.startMilliseconds == rhoStart) {
                    rho = std::min(rho, next.rho);
                }
            }
        }
        appendPiece();
        return result;
    }

    QList<PieceBlueprint> PieceDividerPrivate::buildAllBlueprints() const {
        return partitionFrom(orderedNotes.cbegin(), orderedNotes.cend(),
                             std::numeric_limits<int>::max(), -1, nullptr);
    }

    QList<PieceBlueprint> PieceDividerPrivate::buildLocalBlueprints(int firstAffectedPosition,
                                                                     int lastAffectedPosition) const {
        if (pieces.isEmpty()) {
            return buildAllBlueprints();
        }

        const auto containing = std::lower_bound(pieces.cbegin(), pieces.cend(), firstAffectedPosition,
                                                 [](Piece *piece, int position) {
                                                     return PiecePrivate::get(piece)->lastNotePosition < position;
                                                 });
        const int containingIndex = static_cast<int>(std::distance(pieces.cbegin(), containing));
        int startIndex;
        if (containingIndex == pieces.size()) {
            startIndex = pieces.size() - 1;
        } else {
            startIndex = std::max(0, containingIndex - 1);
        }

        QList<PieceBlueprint> result;
        result.reserve(pieces.size() + 2);
        for (int i = 0; i < startIndex; ++i) {
            result.append(blueprintFromPiece(pieces.at(i)));
        }

        // An affected note may have moved before the first note of the old Piece.
        // Starting only at the old Piece boundary would omit that note from the
        // rebuilt suffix until an unrelated full rebuild occurred.
        const int startPosition = std::min(PiecePrivate::get(pieces.at(startIndex))->firstNotePosition,
                                           firstAffectedPosition);
        const auto first = orderedNotes.lower_bound(PieceNoteOrder {startPosition, 0});
        int matchedOldIndex = -1;
        result.append(partitionFrom(first, orderedNotes.cend(), lastAffectedPosition,
                                    startIndex, &matchedOldIndex));
        if (matchedOldIndex >= 0) {
            for (int i = matchedOldIndex + 1; i < pieces.size(); ++i) {
                result.append(blueprintFromPiece(pieces.at(i)));
            }
        }
        return result;
    }

    QList<Piece *> PieceDividerPrivate::chooseReusablePieces(const QList<Piece *> &oldPieces,
                                                              const QList<PieceBlueprint> &blueprints) const {
        const int oldCount = oldPieces.size();
        const int newCount = blueprints.size();
        QList<Piece *> result(newCount, nullptr);
        if (!oldCount || !newCount) {
            return result;
        }

        bool allPreserved = oldCount == newCount;
        bool preservedOrder = true;
        int previousOldIndex = -1;
        QHash<Piece *, int> oldIndexes;
        oldIndexes.reserve(oldCount);
        for (int i = 0; i < oldCount; ++i) {
            oldIndexes.insert(oldPieces.at(i), i);
        }
        for (int i = 0; i < newCount; ++i) {
            Piece *preserved = blueprints.at(i).preservedPiece;
            const int oldIndex = oldIndexes.value(preserved, -1);
            if (oldIndex < 0) {
                allPreserved = false;
                continue;
            }
            if (oldIndex <= previousOldIndex) {
                allPreserved = false;
                preservedOrder = false;
                continue;
            }
            result[i] = preserved;
            previousOldIndex = oldIndex;
        }
        if (allPreserved && preservedOrder && previousOldIndex == oldCount - 1) {
            return result;
        }

        if (oldCount == newCount) {
            bool sameContents = true;
            for (int i = 0; i < oldCount; ++i) {
                if (result.at(i) != oldPieces.at(i) &&
                    PiecePrivate::get(oldPieces.at(i))->noteIds != blueprints.at(i).noteIds) {
                    sameContents = false;
                    break;
                }
            }
            if (sameContents) {
                return oldPieces;
            }
        }
        const QList<Piece *> preservedMatches = result;
        result.fill(nullptr);

        constexpr qsizetype maximumDpCells = 1000000;
        if (static_cast<qsizetype>(oldCount + 1) * static_cast<qsizetype>(newCount + 1) > maximumDpCells) {
            if (preservedOrder) {
                result = preservedMatches;
                auto matchGap = [this, &result, &oldPieces, &blueprints](int oldFirst, int oldLast,
                                                                         int newFirst, int newLast) {
                    int oldCursor = oldFirst;
                    int newCursor = newFirst;
                    while (oldCursor < oldLast && newCursor < newLast) {
                        if (oldLast - oldCursor <= newLast - newCursor) {
                            const int maximumNew = newLast - (oldLast - oldCursor);
                            int bestNew = newCursor;
                            int bestOverlap = -1;
                            long double bestDistance = std::numeric_limits<long double>::infinity();
                            const auto *oldData = PiecePrivate::get(oldPieces.at(oldCursor));
                            for (int candidate = newCursor; candidate <= maximumNew; ++candidate) {
                                const int overlap = sharedNoteCount(oldData->noteIds, blueprints.at(candidate).noteIds);
                                const long double distance =
                                    std::abs(oldData->startMilliseconds - blueprints.at(candidate).startMilliseconds) +
                                    std::abs(oldData->endMilliseconds - blueprints.at(candidate).endMilliseconds);
                                if (overlap > bestOverlap || (overlap == bestOverlap && distance < bestDistance)) {
                                    bestNew = candidate;
                                    bestOverlap = overlap;
                                    bestDistance = distance;
                                }
                            }
                            result[bestNew] = oldPieces.at(oldCursor);
                            ++oldCursor;
                            newCursor = bestNew + 1;
                        } else {
                            const int maximumOld = oldLast - (newLast - newCursor);
                            int bestOld = oldCursor;
                            int bestOverlap = -1;
                            long double bestDistance = std::numeric_limits<long double>::infinity();
                            for (int candidate = oldCursor; candidate <= maximumOld; ++candidate) {
                                const auto *oldData = PiecePrivate::get(oldPieces.at(candidate));
                                const int overlap = sharedNoteCount(oldData->noteIds,
                                                                    blueprints.at(newCursor).noteIds);
                                const long double distance =
                                    std::abs(oldData->startMilliseconds - blueprints.at(newCursor).startMilliseconds) +
                                    std::abs(oldData->endMilliseconds - blueprints.at(newCursor).endMilliseconds);
                                if (overlap > bestOverlap || (overlap == bestOverlap && distance < bestDistance)) {
                                    bestOld = candidate;
                                    bestOverlap = overlap;
                                    bestDistance = distance;
                                }
                            }
                            result[newCursor] = oldPieces.at(bestOld);
                            oldCursor = bestOld + 1;
                            ++newCursor;
                        }
                    }
                };

                int previousOld = -1;
                int previousNew = -1;
                for (int newIndex = 0; newIndex <= newCount; ++newIndex) {
                    Piece *anchor = newIndex < newCount ? preservedMatches.at(newIndex) : nullptr;
                    const int oldIndex = anchor ? oldIndexes.value(anchor, -1) : oldCount;
                    if (newIndex != newCount && oldIndex < 0) {
                        continue;
                    }
                    matchGap(previousOld + 1, oldIndex, previousNew + 1, newIndex);
                    if (newIndex < newCount) {
                        result[newIndex] = anchor;
                    }
                    previousOld = oldIndex;
                    previousNew = newIndex;
                }
                int reusedCount = 0;
                for (Piece *piece : std::as_const(result)) {
                    reusedCount += piece != nullptr;
                }
                if (reusedCount == std::min(oldCount, newCount)) {
                    return result;
                }
                result.fill(nullptr);
            }
            const int common = std::min(oldCount, newCount);
            for (int i = 0; i < common; ++i) {
                result[i] = oldPieces.at(i);
            }
            return result;
        }

        struct Score {
            int matches = 0;
            int overlap = 0;
            long double boundaryDistance = 0.0;
        };
        auto better = [](const Score &left, const Score &right) {
            if (left.matches != right.matches) {
                return left.matches > right.matches;
            }
            if (left.overlap != right.overlap) {
                return left.overlap > right.overlap;
            }
            return left.boundaryDistance < right.boundaryDistance;
        };

        QVector<Score> previous(newCount + 1);
        QVector<Score> current(newCount + 1);
        QVector<quint8> decisions((oldCount + 1) * (newCount + 1));
        for (int i = 1; i <= oldCount; ++i) {
            current[0] = {};
            decisions[i * (newCount + 1)] = 1;
            const auto *oldData = PiecePrivate::get(oldPieces.at(i - 1));
            for (int j = 1; j <= newCount; ++j) {
                Score best = previous[j];
                quint8 decision = 1; // Skip old.
                if (better(current[j - 1], best)) {
                    best = current[j - 1];
                    decision = 2; // Skip new.
                }
                Score match = previous[j - 1];
                ++match.matches;
                match.overlap += sharedNoteCount(oldData->noteIds, blueprints.at(j - 1).noteIds);
                match.boundaryDistance += std::abs(oldData->startMilliseconds - blueprints.at(j - 1).startMilliseconds) +
                                          std::abs(oldData->endMilliseconds - blueprints.at(j - 1).endMilliseconds);
                if (better(match, best) || (!better(best, match) && !better(match, best))) {
                    best = match;
                    decision = 3;
                }
                current[j] = best;
                decisions[i * (newCount + 1) + j] = decision;
            }
            std::swap(previous, current);
        }

        int i = oldCount;
        int j = newCount;
        while (i > 0 || j > 0) {
            const quint8 decision = decisions[i * (newCount + 1) + j];
            if (decision == 3) {
                result[j - 1] = oldPieces.at(i - 1);
                --i;
                --j;
            } else if (decision == 1) {
                --i;
            } else {
                --j;
            }
        }
        return result;
    }

    void PieceDividerPrivate::reconcile(const QList<PieceBlueprint> &blueprints,
                                         QHash<Piece *, PieceChange::ChangeTypes> changes,
                                         QHash<Piece *, QSet<QString>> names) {
        Q_Q(PieceDivider);
        const QList<Piece *> oldPieces = pieces;
        QList<Piece *> reusable = chooseReusablePieces(oldPieces, blueprints);
        QSet<Piece *> reusedSet;
        for (Piece *piece : std::as_const(reusable)) {
            if (piece) {
                reusedSet.insert(piece);
            }
        }

        for (int i = 0; i < reusable.size(); ++i) {
            Piece *piece = reusable.at(i);
            if (!piece) {
                continue;
            }
            PiecePrivate *data = PiecePrivate::get(piece);
            const auto &blueprint = blueprints.at(i);
            const bool positionChanged = data->position != blueprint.position;
            const bool lengthChanged = data->length != blueprint.length;
            const bool contentsChanged = data->noteIds != blueprint.noteIds;
            if (contentsChanged) {
                for (quint64 id : std::as_const(data->noteIds)) {
                    if (committedNoteOwners.value(id) == piece) {
                        committedNoteOwners.remove(id);
                    }
                }
            }
            data->position = blueprint.position;
            data->length = blueprint.length;
            data->startMilliseconds = blueprint.startMilliseconds;
            data->endMilliseconds = blueprint.endMilliseconds;
            data->firstNotePosition = blueprint.firstNotePosition;
            data->lastNotePosition = blueprint.lastNotePosition;
            data->noteIds = blueprint.noteIds;
            if (contentsChanged) {
                for (quint64 id : std::as_const(data->noteIds)) {
                    committedNoteOwners.insert(id, piece);
                }
            }
            if (positionChanged) {
                emit piece->positionChanged(data->position);
            }
            if (lengthChanged) {
                emit piece->lengthChanged(data->length);
            }
        }

        bool membershipChanged = false;
        for (Piece *piece : oldPieces) {
            if (reusedSet.contains(piece)) {
                continue;
            }
            for (quint64 id : PiecePrivate::get(piece)->noteIds) {
                if (committedNoteOwners.value(id) == piece) {
                    committedNoteOwners.remove(id);
                }
            }
            emit q->pieceAboutToRemove(piece);
            pieces.removeOne(piece);
            emit q->pieceRemoved(piece);
            delete piece;
            membershipChanged = true;
        }

        for (int i = 0; i < blueprints.size(); ++i) {
            if (reusable.at(i)) {
                continue;
            }
            Piece *piece = new Piece(q);
            PiecePrivate *data = PiecePrivate::get(piece);
            const auto &blueprint = blueprints.at(i);
            data->position = blueprint.position;
            data->length = blueprint.length;
            data->startMilliseconds = blueprint.startMilliseconds;
            data->endMilliseconds = blueprint.endMilliseconds;
            data->firstNotePosition = blueprint.firstNotePosition;
            data->lastNotePosition = blueprint.lastNotePosition;
            data->noteIds = blueprint.noteIds;
            for (quint64 id : std::as_const(data->noteIds)) {
                committedNoteOwners.insert(id, piece);
            }
            reusable[i] = piece;
            emit q->pieceAboutToInsert(piece);
            pieces.insert(i, piece);
            emit q->pieceInserted(piece);
            membershipChanged = true;
        }
        Q_ASSERT(pieces == reusable);
        if (membershipChanged) {
            emit q->piecesChanged();
        }

        QList<Piece *> changedPieces = changes.keys();
        std::sort(changedPieces.begin(), changedPieces.end(), [](Piece *left, Piece *right) {
            const double leftStart = PiecePrivate::get(left)->startMilliseconds;
            const double rightStart = PiecePrivate::get(right)->startMilliseconds;
            return leftStart < rightStart ||
                   (leftStart == rightStart && std::less<Piece *> {}(left, right));
        });
        for (Piece *piece : std::as_const(changedPieces)) {
            const auto types = changes.value(piece);
            if (!types) {
                continue;
            }
            const QSet<QString> nameSet = names.value(piece);
            QStringList parameterNameList(nameSet.cbegin(), nameSet.cend());
            emit piece->updated(PieceChange(types, std::move(parameterNameList)));
        }
    }

    void PieceDividerPrivate::addParameterChanges(
        QHash<Piece *, PieceChange::ChangeTypes> &changes,
        QHash<Piece *, QSet<QString>> &names,
        const QList<Piece *> &oldPieces,
        const QHash<Piece *, PieceBlueprint> &oldSnapshots,
        const PieceTimeMap &oldMap,
        int oldClipStart,
        const PieceTimeMap &newMap,
        int newClipStart,
        bool globalTimeChange) const {
        const bool membershipMayHaveChanged = !oldSnapshots.isEmpty();
        const QSet<Piece *> finalPieces = membershipMayHaveChanged
                                             ? QSet<Piece *>(pieces.cbegin(), pieces.cend())
                                             : QSet<Piece *> {};
        auto add = [&changes, &names, &finalPieces, membershipMayHaveChanged](Piece *piece, const QString &name) {
            if (!piece || (membershipMayHaveChanged && !finalPieces.contains(piece)) || name.isEmpty()) {
                return;
            }
            changes[piece] |= PieceChange::Parameter;
            names[piece].insert(name);
        };

        QList<PieceParameterImpact> impacts = pendingParameterImpacts;
        std::sort(impacts.begin(), impacts.end(), [](const PieceParameterImpact &left,
                                                      const PieceParameterImpact &right) {
            if (left.name != right.name) {
                return left.name < right.name;
            }
            if (left.wholeDomain != right.wholeDomain) {
                return left.wholeDomain;
            }
            return left.firstTick < right.firstTick;
        });
        QList<PieceParameterImpact> mergedImpacts;
        for (const auto &impact : std::as_const(impacts)) {
            if (!mergedImpacts.isEmpty() && mergedImpacts.constLast().name == impact.name) {
                auto &last = mergedImpacts.last();
                if (last.wholeDomain || impact.wholeDomain) {
                    last.wholeDomain = true;
                    last.firstTick = 0.0;
                    last.lastTick = 0.0;
                    last.throughEnd = false;
                    continue;
                }
                if (last.throughEnd || impact.firstTick <= last.lastTick) {
                    last.lastTick = std::max(last.lastTick, impact.lastTick);
                    last.throughEnd = last.throughEnd || impact.throughEnd;
                    continue;
                }
            }
            mergedImpacts.append(impact);
        }

        auto addOldIntersections = [this, &oldPieces, &oldSnapshots, &add](double start, double end,
                                                                           const QString &name) {
            auto snapshot = [this, &oldSnapshots](Piece *piece) {
                const auto it = oldSnapshots.constFind(piece);
                return it == oldSnapshots.cend() ? blueprintFromPiece(piece) : *it;
            };
            const auto first = std::lower_bound(oldPieces.cbegin(), oldPieces.cend(), start,
                                                [&snapshot](Piece *piece, double value) {
                                                    return snapshot(piece).endMilliseconds < value;
                                                });
            for (auto it = first; it != oldPieces.cend(); ++it) {
                const auto pieceSnapshot = snapshot(*it);
                if (pieceSnapshot.startMilliseconds > end) {
                    break;
                }
                add(*it, name);
            }
        };
        auto addNewIntersections = [this, &add](double start, double end, const QString &name) {
            const auto first = std::lower_bound(pieces.cbegin(), pieces.cend(), start,
                                                [](Piece *piece, double value) {
                                                    return PiecePrivate::get(piece)->endMilliseconds < value;
                                                });
            for (auto it = first; it != pieces.cend(); ++it) {
                const auto *data = PiecePrivate::get(*it);
                if (data->startMilliseconds > end) {
                    break;
                }
                add(*it, name);
            }
        };

        if (globalTimeChange) {
            ParameterMap *map = singingClip->parameters();
            for (const QString &name : map->keys()) {
                Parameter *parameter = map->item(name);
                if (!parameter) {
                    continue;
                }
                bool hasDomain = false;
                double firstTick = std::numeric_limits<double>::infinity();
                double lastTick = -std::numeric_limits<double>::infinity();
                auto includeFree = [&hasDomain, &firstTick, &lastTick](FreeValueDataArray *array) {
                    if (array->size() <= 0) {
                        return;
                    }
                    hasDomain = true;
                    firstTick = std::min(firstTick, 0.0);
                    lastTick = std::max(lastTick,
                                        static_cast<double>(array->size() - 1) * FreeValueDataArray::step());
                };
                auto includeAnchors = [&hasDomain, &firstTick, &lastTick](AnchorNodeSequence *sequence) {
                    if (!sequence->firstItem()) {
                        return;
                    }
                    hasDomain = true;
                    firstTick = std::min(firstTick, static_cast<double>(sequence->firstItem()->x()));
                    lastTick = std::max(lastTick, static_cast<double>(sequence->lastItem()->x()));
                };
                includeFree(parameter->freeEdited());
                includeFree(parameter->freeTransform());
                includeAnchors(parameter->anchorEdited());
                includeAnchors(parameter->anchorTransform());
                if (!hasDomain) {
                    continue;
                }
                const double oldStart = oldMap.tickToMilliseconds(static_cast<double>(oldClipStart) + firstTick);
                const double oldEnd = oldMap.tickToMilliseconds(static_cast<double>(oldClipStart) + lastTick);
                const double newStart = newMap.tickToMilliseconds(static_cast<double>(newClipStart) + firstTick);
                const double newEnd = newMap.tickToMilliseconds(static_cast<double>(newClipStart) + lastTick);
                addOldIntersections(oldStart, oldEnd, name);
                addNewIntersections(newStart, newEnd, name);
            }
        }

        for (const auto &impact : std::as_const(mergedImpacts)) {
            if (impact.wholeDomain) {
                for (Piece *piece : oldPieces) {
                    add(piece, impact.name);
                }
                for (Piece *piece : pieces) {
                    add(piece, impact.name);
                }
                continue;
            }

            const double oldStart = oldMap.tickToMilliseconds(static_cast<double>(oldClipStart) + impact.firstTick);
            const double newStart = newMap.tickToMilliseconds(static_cast<double>(newClipStart) + impact.firstTick);
            const double oldEnd = impact.throughEnd
                                      ? std::numeric_limits<double>::infinity()
                                      : oldMap.tickToMilliseconds(static_cast<double>(oldClipStart) + impact.lastTick);
            const double newEnd = impact.throughEnd
                                      ? std::numeric_limits<double>::infinity()
                                      : newMap.tickToMilliseconds(static_cast<double>(newClipStart) + impact.lastTick);

            addOldIntersections(oldStart, oldEnd, impact.name);
            addNewIntersections(newStart, newEnd, impact.name);
        }
    }

    void PieceDividerPrivate::commitPending() {
        if (!singingClip) {
            discardPending();
            return;
        }
        if (pendingNoteIds.isEmpty() && pendingParameterImpacts.isEmpty() && !tempoDirty && !clipStartDirty &&
            !configurationChangedInTransaction) {
            return;
        }

        const QList<Piece *> oldPieces = pieces;
        QHash<Piece *, PieceBlueprint> oldPieceSnapshots;
        const PieceTimeMap oldTimeMap = committedTimeMap;
        const int oldClipStart = committedClipStart;
        QHash<quint64, PieceNoteState> oldAffectedStates;

        const PieceTimeMap newTimeMap = tempoDirty ? captureTimeMap() : committedTimeMap;
        const int newClipStart = singingClip->start();
        const bool globalTimeChange = newClipStart != oldClipStart || newTimeMap.segments != oldTimeMap.segments;
        QSet<quint64> affectedIds = pendingNoteIds;
        int firstAffectedPosition = std::numeric_limits<int>::max();
        int lastAffectedPosition = std::numeric_limits<int>::min();

        if (globalTimeChange) {
            oldAffectedStates = committedNotes;
            for (auto it = committedNotes.cbegin(); it != committedNotes.cend(); ++it) {
                affectedIds.insert(it.key());
            }
            committedNotes.clear();
            orderedNotes.clear();
            notePointers.clear();
            for (Note *note : singingClip->notes()->asRange()) {
                const auto state = captureNote(note, newTimeMap, newClipStart);
                committedNotes.insert(state.id, state);
                orderedNotes.emplace(PieceNoteOrder {state.position, state.id}, state.id);
                notePointers.insert(state.id, note);
                affectedIds.insert(state.id);
            }
        } else {
            for (quint64 id : std::as_const(affectedIds)) {
                const auto oldIt = committedNotes.constFind(id);
                if (oldIt != committedNotes.cend()) {
                    oldAffectedStates.insert(id, *oldIt);
                    firstAffectedPosition = std::min(firstAffectedPosition, oldIt->position);
                    lastAffectedPosition = std::max(lastAffectedPosition, oldIt->position);
                    orderedNotes.erase(PieceNoteOrder {oldIt->position, id});
                    committedNotes.remove(id);
                }

                Note *note = notePointers.value(id);
                if (!note) {
                    note = singingClip->model()->find<Note>(Handle {id});
                }
                if (note && note->noteSequence() == singingClip->notes()) {
                    const auto state = captureNote(note, newTimeMap, newClipStart);
                    committedNotes.insert(id, state);
                    orderedNotes.emplace(PieceNoteOrder {state.position, id}, id);
                    notePointers.insert(id, note);
                    firstAffectedPosition = std::min(firstAffectedPosition, state.position);
                    lastAffectedPosition = std::max(lastAffectedPosition, state.position);
                } else {
                    notePointers.remove(id);
                }
            }
        }

        committedTimeMap = newTimeMap;
        committedClipStart = newClipStart;

        bool partitionDirty = globalTimeChange;
        if (!partitionDirty) {
            for (quint64 id : std::as_const(affectedIds)) {
                const auto oldIt = oldAffectedStates.constFind(id);
                const auto newIt = committedNotes.constFind(id);
                if ((oldIt == oldAffectedStates.cend()) != (newIt == committedNotes.cend())) {
                    partitionDirty = true;
                    break;
                }
                if (oldIt == oldAffectedStates.cend()) {
                    continue;
                }
                const bool oldRest = restLyrics.contains(oldIt->lyric, Qt::CaseSensitive);
                const bool newRest = restLyrics.contains(newIt->lyric, Qt::CaseSensitive);
                if (oldIt->position != newIt->position || oldIt->length != newIt->length ||
                    oldIt->leadingNonOnsetPhonemes != newIt->leadingNonOnsetPhonemes || oldRest != newRest) {
                    partitionDirty = true;
                    break;
                }
            }
        }
        if (globalTimeChange || (partitionDirty && !pendingParameterImpacts.isEmpty())) {
            oldPieceSnapshots.reserve(oldPieces.size());
            for (Piece *piece : oldPieces) {
                oldPieceSnapshots.insert(piece, blueprintFromPiece(piece));
            }
        }

        QHash<quint64, Piece *> oldOwners;
        oldOwners.reserve(affectedIds.size());
        for (quint64 id : std::as_const(affectedIds)) {
            if (Piece *piece = committedNoteOwners.value(id)) {
                oldOwners.insert(id, piece);
            }
        }

        if (partitionDirty) {
            QList<PieceBlueprint> blueprints;
            if (globalTimeChange) {
                blueprints = buildAllBlueprints();
            } else if (!affectedIds.isEmpty()) {
                if (firstAffectedPosition == std::numeric_limits<int>::max()) {
                    blueprints = buildAllBlueprints();
                } else {
                    blueprints = buildLocalBlueprints(firstAffectedPosition, lastAffectedPosition);
                }
            } else {
                blueprints.reserve(pieces.size());
                for (Piece *piece : std::as_const(pieces)) {
                    blueprints.append(blueprintFromPiece(piece));
                }
            }
            reconcile(blueprints);
        }
        const auto &newOwners = committedNoteOwners;
        QHash<Piece *, PieceChange::ChangeTypes> changes;
        QHash<Piece *, QSet<QString>> changedParameterNames;
        const QSet<Piece *> finalPieceSet = partitionDirty
                                                ? QSet<Piece *>(pieces.cbegin(), pieces.cend())
                                                : QSet<Piece *> {};

        auto addType = [&changes, &finalPieceSet, partitionDirty](Piece *piece, PieceChange::ChangeType type) {
            if (piece && (!partitionDirty || finalPieceSet.contains(piece))) {
                changes[piece] |= type;
            }
        };

        for (quint64 id : std::as_const(affectedIds)) {
            const auto oldIt = oldAffectedStates.constFind(id);
            const auto newIt = committedNotes.constFind(id);
            const bool hadOld = oldIt != oldAffectedStates.cend();
            const bool hasNew = newIt != committedNotes.cend();
            PieceChange::ChangeTypes types;
            if (hadOld != hasNew) {
                types |= PieceChange::Score;
            } else if (hadOld) {
                if (oldIt->lyric != newIt->lyric || oldIt->language != newIt->language) {
                    types |= PieceChange::Lyric;
                }
                if (oldIt->editedPronunciation != newIt->editedPronunciation) {
                    types |= PieceChange::Pronunciation;
                }
                if (oldIt->keyNumber != newIt->keyNumber || oldIt->centShift != newIt->centShift ||
                    oldIt->startMilliseconds != newIt->startMilliseconds ||
                    oldIt->endMilliseconds != newIt->endMilliseconds) {
                    types |= PieceChange::Note;
                }
                if (!listsEqual(oldIt->editedPhonemes, newIt->editedPhonemes)) {
                    types |= PieceChange::Phoneme;
                }
                if (!oldIt->vibratoEquals(*newIt)) {
                    types |= PieceChange::Vibrato;
                }
            }
            if (!types) {
                continue;
            }
            Piece *oldPiece = oldOwners.value(id);
            Piece *newPiece = newOwners.value(id);
            for (int bit = PieceChange::Score; bit <= PieceChange::Parameter; bit <<= 1) {
                const auto type = static_cast<PieceChange::ChangeType>(bit);
                if (types.testFlag(type)) {
                    addType(oldPiece, type);
                    addType(newPiece, type);
                }
            }
        }

        addParameterChanges(changes, changedParameterNames, oldPieces, oldPieceSnapshots,
                            oldTimeMap, oldClipStart, newTimeMap, newClipStart, globalTimeChange);

        QList<Piece *> changedPieces = changes.keys();
        std::sort(changedPieces.begin(), changedPieces.end(), [](Piece *left, Piece *right) {
            const double leftStart = PiecePrivate::get(left)->startMilliseconds;
            const double rightStart = PiecePrivate::get(right)->startMilliseconds;
            return leftStart < rightStart ||
                   (leftStart == rightStart && std::less<Piece *> {}(left, right));
        });
        for (Piece *piece : std::as_const(changedPieces)) {
            const auto types = changes.value(piece);
            if (!types) {
                continue;
            }
            const QSet<QString> nameSet = changedParameterNames.value(piece);
            QStringList names(nameSet.cbegin(), nameSet.cend());
            emit piece->updated(PieceChange(types, std::move(names)));
        }

        ParameterMap *map = singingClip->parameters();
        for (const QString &key : map->keys()) {
            parameterNames.insert(map->item(key), key);
        }
        discardPending();
    }

    PieceDivider::PieceDivider(QObject *parent)
        : QObject(parent), d_ptr(new PieceDividerPrivate(this)) {
        qRegisterMetaType<PieceChange>();
    }

    PieceDivider::~PieceDivider() = default;

    SingingClip *PieceDivider::singingClip() const {
        Q_D(const PieceDivider);
        return d->singingClip;
    }

    void PieceDivider::setSingingClip(SingingClip *singingClip) {
        Q_D(PieceDivider);
        if (d->singingClip == singingClip) {
            return;
        }
        d->bind(singingClip);
        emit singingClipChanged(singingClip);
    }

    double PieceDivider::paddingBase() const {
        Q_D(const PieceDivider);
        return d->paddingBase;
    }

    void PieceDivider::setPaddingBase(double paddingBase) {
        Q_D(PieceDivider);
        if (!std::isfinite(paddingBase) || paddingBase < 0.0 || d->paddingBase == paddingBase) {
            return;
        }
        d->paddingBase = paddingBase;
        emit paddingBaseChanged(paddingBase);
        d->rebuildForConfigurationChange();
    }

    double PieceDivider::paddingAdditional() const {
        Q_D(const PieceDivider);
        return d->paddingAdditional;
    }

    void PieceDivider::setPaddingAdditional(double paddingAdditional) {
        Q_D(PieceDivider);
        if (!std::isfinite(paddingAdditional) || paddingAdditional < 0.0 || d->paddingAdditional == paddingAdditional) {
            return;
        }
        d->paddingAdditional = paddingAdditional;
        emit paddingAdditionalChanged(paddingAdditional);
        d->rebuildForConfigurationChange();
    }

    double PieceDivider::paddingGap() const {
        Q_D(const PieceDivider);
        return d->paddingGap;
    }

    void PieceDivider::setPaddingGap(double paddingGap) {
        Q_D(PieceDivider);
        if (!std::isfinite(paddingGap) || paddingGap < 0.0 || d->paddingGap == paddingGap) {
            return;
        }
        d->paddingGap = paddingGap;
        emit paddingGapChanged(paddingGap);
        d->rebuildForConfigurationChange();
    }

    QStringList PieceDivider::restLyrics() const {
        Q_D(const PieceDivider);
        return d->restLyrics;
    }

    void PieceDivider::setRestLyrics(const QStringList &restLyrics) {
        Q_D(PieceDivider);
        if (d->restLyrics == restLyrics) {
            return;
        }
        d->restLyrics = restLyrics;
        emit restLyricsChanged(restLyrics);
        d->rebuildForConfigurationChange();
    }

    QList<Piece *> PieceDivider::pieces() const {
        Q_D(const PieceDivider);
        return d->pieces;
    }

}

#include "moc_PieceDivider.cpp"
