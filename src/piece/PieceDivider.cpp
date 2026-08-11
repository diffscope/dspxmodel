#include "PieceDivider.h"

#include "Piece.h"
#include "Piece_p.h"
#include "PieceDivider_p.h"

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>

#include <QPointer>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <utility>

namespace dspx {

    namespace {

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

    }

    PieceDividerPrivate::PieceDividerPrivate(PieceDivider *q)
        : q_ptr(q) {
    }

    PieceDividerPrivate::~PieceDividerPrivate() {
        delete watchContext;
        for (Piece *piece : std::as_const(pieces)) {
            delete piece;
        }
    }

    QList<PiecePhonemeState> PieceDividerPrivate::capturePhonemes(PhonemeSequence *sequence) const {
        QList<PiecePhonemeState> result;
        if (!sequence) {
            return result;
        }
        result.reserve(sequence->size());
        for (Phoneme *phoneme : sequence->asRange()) {
            result.append({phoneme->handle().d, phoneme->start(), phoneme->onset()});
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

    PieceNoteState PieceDividerPrivate::captureNote(Note *note) const {
        PieceNoteState result;
        result.id = note->handle().d;
        result.position = note->position();
        result.length = note->length();
        result.lyric = note->lyric();
        const auto originalPhonemes = capturePhonemes(note->originalPhonemes());
        for (const auto &phoneme : originalPhonemes) {
            if (phoneme.onset) {
                break;
            }
            ++result.leadingNonOnsetPhonemes;
        }
        return result;
    }

    void PieceDividerPrivate::markNote(quint64 id) {
        if (id) {
            pendingNoteIds.insert(id);
        }
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

    void PieceDividerPrivate::installPhonemeWatcher(Phoneme *phoneme, quint64 noteId) {
        if (!phoneme || !watchContext) {
            return;
        }
        const quint64 id = phoneme->handle().d;
        phonemeOwners.insert(id, noteId);
        if (!beginWatching(phoneme)) {
            return;
        }
        auto changed = [this, id] { markNote(phonemeOwners.value(id)); };
        QObject::connect(phoneme, &Phoneme::startChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::onsetChanged, watchContext, changed);
        QObject::connect(phoneme, &QObject::destroyed, watchContext,
                         [this, id] { markNote(phonemeOwners.value(id)); });
    }

    void PieceDividerPrivate::installPhonemeWatcher(PhonemeSequence *sequence, quint64 noteId) {
        if (!sequence || !watchContext) {
            return;
        }
        if (!beginWatching(sequence)) {
            return;
        }
        for (Phoneme *phoneme : sequence->asRange()) {
            installPhonemeWatcher(phoneme, noteId);
        }
        QObject::connect(sequence, &PhonemeSequence::itemAboutToRemove, watchContext,
                         [this, noteId](Phoneme *, PhonemeSequence *) { markNote(noteId); });
        QObject::connect(sequence, &PhonemeSequence::itemInserted, watchContext,
                         [this, noteId](Phoneme *phoneme, PhonemeSequence *) {
                             installPhonemeWatcher(phoneme, noteId);
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
        QObject::connect(note, &Note::lengthChanged, watchContext, changed);
        QObject::connect(note, &Note::lyricChanged, watchContext, changed);
        QObject::connect(note, &Note::positionChanged, watchContext, changed);
        QObject::connect(note, &QObject::destroyed, watchContext,
                         [this, id] { markNote(id); });
        installPhonemeWatcher(note->originalPhonemes(), id);
    }

    void PieceDividerPrivate::installTempoWatcher(Tempo *tempo) {
        if (!tempo || !watchContext) {
            return;
        }
        if (!beginWatching(tempo)) {
            return;
        }
        const quint64 id = tempo->handle().d;
        auto changed = [this, id] { pendingTempoIds.insert(id); };
        QObject::connect(tempo, &Tempo::positionChanged, watchContext, changed);
        QObject::connect(tempo, &Tempo::valueChanged, watchContext, changed);
        QObject::connect(tempo, &QObject::destroyed, watchContext,
                         [this, id] { pendingTempoIds.insert(id); });
    }

    void PieceDividerPrivate::installWatchers() {
        delete watchContext;
        watchContext = nullptr;
        notePointers.clear();
        phonemeOwners.clear();
        watchedObjects.clear();
        if (!singingClip) {
            return;
        }

        watchContext = new QObject(q_ptr);
        QObject::connect(singingClip, &QObject::destroyed, watchContext, [this] {
            singingClip = nullptr;
            bindingDirty = true;
            discardPending();
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
                         [this](Tempo *tempo) { pendingTempoIds.insert(tempo->handle().d); });
        QObject::connect(tempos, &TempoSequence::itemInserted, watchContext,
                         [this](Tempo *tempo) {
                             installTempoWatcher(tempo);
                             pendingTempoIds.insert(tempo->handle().d);
                         });

    }

    void PieceDividerPrivate::discardPending() {
        pendingNoteIds.clear();
        pendingTempoIds.clear();
        clipStartDirty = false;
    }

    void PieceDividerPrivate::bind(SingingClip *clip) {
        discardPending();
        singingClip = clip;
        installWatchers();
        bindingDirty = true;
    }

    bool PieceDividerPrivate::configurationDiffers() const {
        return paddingBase != appliedPaddingBase || paddingAdditional != appliedPaddingAdditional ||
               paddingGap != appliedPaddingGap || restLyrics != appliedRestLyrics;
    }

    void PieceDividerPrivate::initializeAppliedState() {
        committedNotes.clear();
        orderedNotes.clear();
        committedNoteOwners.clear();
        if (!singingClip) {
            reconcile({});
            timeMapCache.reset(nullptr);
            committedTimeMap = timeMapCache.timeMap();
            committedClipStart = 0;
        } else {
            timeMapCache.reset(singingClip->model()->tempos());
            committedTimeMap = timeMapCache.timeMap();
            committedClipStart = singingClip->start();
            for (Note *note : singingClip->notes()->asRange()) {
                const auto state = captureNote(note);
                committedNotes.insert(state.id, state);
                orderedNotes.emplace(PieceNoteOrder {state.position, state.id}, state.id);
                notePointers.insert(state.id, note);
            }
            reconcile(buildAllBlueprints());
            committedNoteOwners.clear();
            for (Piece *piece : std::as_const(pieces)) {
                for (quint64 id : PiecePrivate::get(piece)->noteIds) {
                    committedNoteOwners.insert(id, piece);
                }
            }
        }
        appliedSingingClip = singingClip;
        appliedPaddingBase = paddingBase;
        appliedPaddingAdditional = paddingAdditional;
        appliedPaddingGap = paddingGap;
        appliedRestLyrics = restLyrics;
        bindingDirty = false;
        discardPending();
    }

    PieceBlueprint PieceDividerPrivate::blueprintFromPiece(Piece *piece) const {
        const auto *d = PiecePrivate::get(piece);
        return {d->position,
                d->length,
                d->firstNotePosition,
                d->lastNotePosition,
                d->noteIds,
                piece};
    }

    bool PieceDividerPrivate::blueprintExactlyEqualsPiece(const PieceBlueprint &blueprint, Piece *piece) const {
        const auto *d = PiecePrivate::get(piece);
        return d->position == blueprint.position &&
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
                    const double noteStartMilliseconds = committedTimeMap.tickToMilliseconds(
                        static_cast<double>(committedClipStart) + note.position);
                    const double noteEndMilliseconds = committedTimeMap.tickToMilliseconds(
                        static_cast<double>(committedClipStart) + note.position + note.length);
                    group.startMilliseconds = noteStartMilliseconds;
                    const bool rest = restLyrics.contains(note.lyric, Qt::CaseSensitive);
                    const double leftPadding = rest
                                                   ? 0.0
                                                   : std::min(paddingBase + note.leadingNonOnsetPhonemes * paddingAdditional,
                                                              paddingGap);
                    const double rightPadding = rest ? 0.0 : paddingBase;
                    group.lambda = std::min(group.lambda, leftPadding);
                    if (noteEndMilliseconds > group.bodyEndMilliseconds) {
                        group.bodyEndMilliseconds = noteEndMilliseconds;
                        rhoStart = noteStartMilliseconds;
                        group.rho = rightPadding;
                    } else if (noteEndMilliseconds == group.bodyEndMilliseconds) {
                        if (noteStartMilliseconds > rhoStart) {
                            rhoStart = noteStartMilliseconds;
                            group.rho = rightPadding;
                        } else if (noteStartMilliseconds == rhoStart) {
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
            const double startTick = committedTimeMap.millisecondsToTick(bodyStart - lambda);
            const double endTick = committedTimeMap.millisecondsToTick(bodyEnd + rho);
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
                                    std::abs(oldData->position - blueprints.at(candidate).position) +
                                    std::abs(oldData->length - blueprints.at(candidate).length);
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
                                    std::abs(oldData->position - blueprints.at(newCursor).position) +
                                    std::abs(oldData->length - blueprints.at(newCursor).length);
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
                match.boundaryDistance += std::abs(oldData->position - blueprints.at(j - 1).position) +
                                          std::abs(oldData->length - blueprints.at(j - 1).length);
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

    void PieceDividerPrivate::reconcile(const QList<PieceBlueprint> &blueprints) {
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

    }

    std::pair<int, int> PieceDividerPrivate::timeMapAffectedPositions(
        const PieceTimeMap &oldMap, int oldClipStart,
        const PieceTimeMap &newMap, int newClipStart) const {
        const int noneFirst = std::numeric_limits<int>::max();
        const int noneLast = std::numeric_limits<int>::min();
        if (pieces.isEmpty()) {
            return {noneFirst, noneLast};
        }

        const auto *firstPiece = PiecePrivate::get(pieces.constFirst());
        const auto *lastPiece = PiecePrivate::get(pieces.constLast());
        const double domainStart = firstPiece->position;
        const double domainEnd = lastPiece->position + lastPiece->length;
        QVector<double> breakpoints {domainStart, domainEnd};
        auto appendBreakpoints = [&breakpoints, domainStart, domainEnd](const PieceTimeMap &map, int clipStart) {
            for (const auto &segment : map.segments) {
                const double relative = segment.tick - clipStart;
                if (relative > domainStart && relative < domainEnd) {
                    breakpoints.append(relative);
                }
            }
        };
        appendBreakpoints(oldMap, oldClipStart);
        appendBreakpoints(newMap, newClipStart);
        std::sort(breakpoints.begin(), breakpoints.end());
        breakpoints.erase(std::unique(breakpoints.begin(), breakpoints.end()), breakpoints.end());

        double affectedStart = std::numeric_limits<double>::infinity();
        double affectedEnd = -std::numeric_limits<double>::infinity();
        for (int i = 0; i + 1 < breakpoints.size(); ++i) {
            const double left = breakpoints.at(i);
            const double right = breakpoints.at(i + 1);
            const double probe = left + (right - left) * 0.5;
            if (oldMap.bpmAtTick(oldClipStart + probe) != newMap.bpmAtTick(newClipStart + probe)) {
                affectedStart = std::min(affectedStart, left);
                affectedEnd = std::max(affectedEnd, right);
            }
        }
        if (!std::isfinite(affectedStart)) {
            return {noneFirst, noneLast};
        }

        const auto firstAffectedPiece = std::lower_bound(
            pieces.cbegin(), pieces.cend(), affectedStart,
            [](Piece *piece, double position) {
                const auto *data = PiecePrivate::get(piece);
                return data->position + data->length < position;
            });
        int firstIndex = static_cast<int>(std::distance(pieces.cbegin(), firstAffectedPiece));
        const int pieceCount = static_cast<int>(pieces.size());
        firstIndex = std::clamp(firstIndex - 1, 0, pieceCount - 1);
        int lastIndex = firstIndex;
        while (lastIndex + 1 < pieces.size() &&
               PiecePrivate::get(pieces.at(lastIndex + 1))->position <= affectedEnd) {
            ++lastIndex;
        }
        lastIndex = std::min(lastIndex + 1, pieceCount - 1);
        return {PiecePrivate::get(pieces.at(firstIndex))->firstNotePosition,
                PiecePrivate::get(pieces.at(lastIndex))->lastNotePosition};
    }

    void PieceDividerPrivate::updatePending() {
        if (bindingDirty || appliedSingingClip != singingClip || configurationDiffers()) {
            initializeAppliedState();
            return;
        }
        if (!singingClip ||
            (pendingNoteIds.isEmpty() && pendingTempoIds.isEmpty() && !clipStartDirty)) {
            discardPending();
            return;
        }

        const PieceTimeMap oldTimeMap = committedTimeMap;
        const int oldClipStart = committedClipStart;
        if (!pendingTempoIds.isEmpty()) {
            timeMapCache.update(singingClip->model()->tempos(), pendingTempoIds);
        }
        const PieceTimeMap newTimeMap = timeMapCache.timeMap();
        const int newClipStart = singingClip->start();
        const bool timeMapChanged = newClipStart != oldClipStart || newTimeMap.segments != oldTimeMap.segments;
        const auto timePositions = timeMapChanged
                                       ? timeMapAffectedPositions(oldTimeMap, oldClipStart, newTimeMap, newClipStart)
                                       : std::pair<int, int> {std::numeric_limits<int>::max(),
                                                             std::numeric_limits<int>::min()};

        QHash<quint64, PieceNoteState> oldAffectedStates;
        int firstAffectedPosition = timePositions.first;
        int lastAffectedPosition = timePositions.second;
        for (quint64 id : std::as_const(pendingNoteIds)) {
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
                const auto state = captureNote(note);
                committedNotes.insert(id, state);
                orderedNotes.emplace(PieceNoteOrder {state.position, id}, id);
                notePointers.insert(id, note);
                firstAffectedPosition = std::min(firstAffectedPosition, state.position);
                lastAffectedPosition = std::max(lastAffectedPosition, state.position);
            } else {
                notePointers.remove(id);
            }
        }

        bool partitionDirty = timePositions.first != std::numeric_limits<int>::max();
        for (quint64 id : std::as_const(pendingNoteIds)) {
            const auto oldIt = oldAffectedStates.constFind(id);
            const auto newIt = committedNotes.constFind(id);
            if ((oldIt == oldAffectedStates.cend()) != (newIt == committedNotes.cend())) {
                partitionDirty = true;
                continue;
            }
            if (oldIt == oldAffectedStates.cend()) {
                continue;
            }
            const bool oldRest = restLyrics.contains(oldIt->lyric, Qt::CaseSensitive);
            const bool newRest = restLyrics.contains(newIt->lyric, Qt::CaseSensitive);
            if (oldIt->position != newIt->position || oldIt->length != newIt->length ||
                oldIt->leadingNonOnsetPhonemes != newIt->leadingNonOnsetPhonemes || oldRest != newRest) {
                partitionDirty = true;
            }
        }

        committedTimeMap = newTimeMap;
        committedClipStart = newClipStart;
        if (partitionDirty) {
            if (pieces.isEmpty() || firstAffectedPosition == std::numeric_limits<int>::max()) {
                reconcile(buildAllBlueprints());
            } else {
                reconcile(buildLocalBlueprints(firstAffectedPosition, lastAffectedPosition));
            }
        }
        discardPending();
    }

    PieceDivider::PieceDivider(QObject *parent)
        : QObject(parent), d_ptr(new PieceDividerPrivate(this)) {
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
    }

    QList<Piece *> PieceDivider::pieces() const {
        Q_D(const PieceDivider);
        return d->pieces;
    }

    void PieceDivider::update() {
        Q_D(PieceDivider);
        d->updatePending();
    }

}

#include "moc_PieceDivider.cpp"
