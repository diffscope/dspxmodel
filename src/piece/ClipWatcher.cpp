#include "ClipWatcher.h"
#include "ClipWatcher_p.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/FreeValueDataArray.h>
#include <dspxmodelORM/MixedSinger.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/Singer.h>
#include <dspxmodelORM/SingerList.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/SingleSinger.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/VibratoPointDataArray.h>

namespace dspx {

    namespace {

        void addRange(QList<ClipChangeRange> &ranges, int position, int length) {
            if (length > 0) {
                ranges.append(ClipChangeRange(position, length));
            }
        }

        void addNoteRange(QList<ClipChangeRange> &ranges, const WatcherNoteState &note) {
            addRange(ranges, note.position, std::max(1, note.length));
        }

        QVector<std::pair<double, double>> differentTimeRanges(
            const PieceTimeMap &oldMap, int oldClipStart,
            const PieceTimeMap &newMap, int newClipStart,
            double domainStart, double domainEnd
        ) {
            QVector<std::pair<double, double>> result;
            if (domainEnd < domainStart) {
                return result;
            }
            if (domainEnd == domainStart) {
                if (oldMap.tickToMilliseconds(oldClipStart + domainStart) !=
                    newMap.tickToMilliseconds(newClipStart + domainStart)) {
                    result.append({domainStart, domainStart});
                }
                return result;
            }

            QVector<double> breakpoints{domainStart, domainEnd};
            auto append = [&breakpoints, domainStart, domainEnd](const PieceTimeMap &map, int clipStart) {
                for (const auto &segment : map.segments) {
                    const double relative = segment.tick - clipStart;
                    if (relative > domainStart && relative < domainEnd) {
                        breakpoints.append(relative);
                    }
                }
            };
            append(oldMap, oldClipStart);
            append(newMap, newClipStart);
            std::sort(breakpoints.begin(), breakpoints.end());
            breakpoints.erase(std::unique(breakpoints.begin(), breakpoints.end()), breakpoints.end());

            for (int i = 0; i + 1 < breakpoints.size(); ++i) {
                const double left = breakpoints.at(i);
                const double right = breakpoints.at(i + 1);
                const double middle = left + (right - left) * 0.5;
                const auto differs = [&](double tick) {
                    return oldMap.tickToMilliseconds(oldClipStart + tick) !=
                           newMap.tickToMilliseconds(newClipStart + tick);
                };
                if (!differs(left) && !differs(middle) && !differs(right)) {
                    continue;
                }
                if (!result.isEmpty() && result.constLast().second == left) {
                    result.last().second = right;
                } else {
                    result.append({left, right});
                }
            }
            return result;
        }

        template <class MultiMap>
        void addIndexedIds(const MultiMap &index, const QVector<std::pair<double, double>> &ranges, QSet<quint64> &ids) {
            for (const auto &[start, end] : ranges) {
                const int firstTick = static_cast<int>(std::ceil(start));
                const int lastTick = static_cast<int>(std::floor(end));
                for (auto it = index.lower_bound(firstTick);
                     it != index.end() && it->first <= lastTick; ++it) {
                    ids.insert(it->second);
                }
            }
        }

        template <class MultiMap>
        void removeIndexedId(MultiMap &index, int key, quint64 id) {
            const auto [first, last] = index.equal_range(key);
            for (auto it = first; it != last; ++it) {
                if (it->second == id) {
                    index.erase(it);
                    return;
                }
            }
        }

        bool freeStatesEqual(const QList<QVariant> &left, const QList<QVariant> &right) {
            int last = std::max(static_cast<int>(left.size()), static_cast<int>(right.size())) - 1;
            while (last >= 0) {
                const QVariant leftValue = last < left.size() ? left.at(last) : QVariant{};
                const QVariant rightValue = last < right.size() ? right.at(last) : QVariant{};
                if (leftValue.isValid() || rightValue.isValid()) {
                    break;
                }
                --last;
            }
            for (int i = 0; i <= last; ++i) {
                const QVariant leftValue = i < left.size() ? left.at(i) : QVariant{};
                const QVariant rightValue = i < right.size() ? right.at(i) : QVariant{};
                if (leftValue != rightValue) {
                    return false;
                }
            }
            return true;
        }

        bool parameterStatesEqual(const WatcherParameterState &left, const WatcherParameterState &right) {
            return left.id == right.id && left.name == right.name &&
                   freeStatesEqual(left.freeEdited, right.freeEdited) &&
                   freeStatesEqual(left.freeTransform, right.freeTransform) &&
                   left.anchorEdited.states == right.anchorEdited.states &&
                   left.anchorTransform.states == right.anchorTransform.states;
        }

        void addFreeDomain(const QList<QVariant> &values, QList<ClipChangeRange> &ranges) {
            const int step = FreeValueDataArray::step();
            int runStart = -1;
            for (int i = 0; i <= values.size(); ++i) {
                const bool valid = i < values.size() && values.at(i).isValid();
                if (valid && runStart < 0) {
                    runStart = i;
                }
                if (!valid && runStart >= 0) {
                    const int last = i - 1;
                    addRange(ranges, runStart * step, (last - runStart) * step + 1);
                    runStart = -1;
                }
            }
        }

        void addAnchorDomain(const WatcherAnchorLayer &layer, QList<ClipChangeRange> &ranges) {
            for (auto it = layer.order.cbegin(); it != layer.order.cend(); ++it) {
                const auto state = layer.states.value(it->second);
                addRange(ranges, state.x, 1);
                const auto next = std::next(it);
                if (next != layer.order.cend() && state.mode != AnchorNode::None) {
                    const int right = layer.states.value(next->second).x;
                    addRange(ranges, state.x, right - state.x);
                }
            }
        }

        void addParameterDomain(const WatcherParameterState &parameter, QList<ClipChangeRange> &ranges) {
            addFreeDomain(parameter.freeEdited, ranges);
            addFreeDomain(parameter.freeTransform, ranges);
            addAnchorDomain(parameter.anchorEdited, ranges);
            addAnchorDomain(parameter.anchorTransform, ranges);
        }

        std::map<WatcherAnchorOrder, quint64>::const_iterator iteratorForAnchor(
            const WatcherAnchorLayer &layer, quint64 id
        ) {
            const auto stateIt = layer.states.constFind(id);
            if (stateIt == layer.states.cend()) {
                return layer.order.cend();
            }
            return layer.order.find({stateIt->x, id});
        }

        void addAnchorOutgoing(const WatcherAnchorLayer &layer, quint64 id, QList<ClipChangeRange> &ranges) {
            const auto it = iteratorForAnchor(layer, id);
            if (it == layer.order.cend()) {
                return;
            }
            const auto state = layer.states.value(id);
            addRange(ranges, state.x, 1);
            const auto next = std::next(it);
            if (next != layer.order.cend() && state.mode != AnchorNode::None) {
                addRange(ranges, state.x, layer.states.value(next->second).x - state.x);
            }
        }

        void addAnchorInfluence(const WatcherAnchorLayer &layer, quint64 id, QList<ClipChangeRange> &ranges) {
            const auto center = iteratorForAnchor(layer, id);
            if (center == layer.order.cend()) {
                return;
            }
            addRange(ranges, layer.states.value(id).x, 1);

            auto first = center;
            for (int i = 0; i < 2 && first != layer.order.cbegin(); ++i) {
                --first;
            }
            auto last = center;
            for (int i = 0; i < 2 && last != layer.order.cend(); ++i) {
                ++last;
            }
            for (auto left = first; left != last && left != layer.order.cend(); ++left) {
                const auto right = std::next(left);
                if (right == layer.order.cend()) {
                    break;
                }
                const auto leftState = layer.states.value(left->second);
                if (leftState.mode == AnchorNode::None) {
                    continue;
                }
                bool depends = left->second == id || right->second == id;
                if (!depends && leftState.mode == AnchorNode::Hermite) {
                    if (left != layer.order.cbegin() && std::prev(left)->second == id) {
                        depends = true;
                    }
                    const auto outerRight = std::next(right);
                    if (outerRight != layer.order.cend() && outerRight->second == id) {
                        depends = true;
                    }
                }
                if (depends) {
                    addRange(ranges, leftState.x, layer.states.value(right->second).x - leftState.x);
                }
            }
        }

        bool applyAnchorChanges(WatcherAnchorLayer &baseline, const QSet<quint64> &dirtyIds, const QHash<quint64, QPointer<AnchorNode>> &pointers, AnchorNodeSequence *sequence, QList<ClipChangeRange> &ranges) {
            struct Change {
                quint64 id = 0;
                std::optional<WatcherAnchorState> oldState;
                std::optional<WatcherAnchorState> newState;
            };
            QList<Change> changes;
            for (quint64 id : dirtyIds) {
                Change change;
                change.id = id;
                const auto oldIt = baseline.states.constFind(id);
                if (oldIt != baseline.states.cend()) {
                    change.oldState = *oldIt;
                }
                AnchorNode *node = pointers.value(id);
                if (node && node->anchorNodeSequence() == sequence) {
                    change.newState = WatcherAnchorState{
                        id, node->x(), node->y(), static_cast<int>(node->interpolationMode())
                    };
                }
                if (change.oldState != change.newState) {
                    changes.append(change);
                }
            }
            if (changes.isEmpty()) {
                return false;
            }

            for (const auto &change : std::as_const(changes)) {
                if (!change.oldState) {
                    continue;
                }
                const bool modeOnly = change.newState &&
                                      change.oldState->x == change.newState->x &&
                                      change.oldState->y == change.newState->y &&
                                      change.oldState->mode != change.newState->mode;
                if (modeOnly) {
                    addAnchorOutgoing(baseline, change.id, ranges);
                } else {
                    addAnchorInfluence(baseline, change.id, ranges);
                }
            }
            for (const auto &change : std::as_const(changes)) {
                if (change.oldState) {
                    baseline.order.erase({change.oldState->x, change.id});
                    baseline.states.remove(change.id);
                }
            }
            for (const auto &change : std::as_const(changes)) {
                if (change.newState) {
                    baseline.states.insert(change.id, *change.newState);
                    baseline.order.emplace(WatcherAnchorOrder{change.newState->x, change.id}, change.id);
                }
            }
            for (const auto &change : std::as_const(changes)) {
                if (!change.newState) {
                    continue;
                }
                const bool modeOnly = change.oldState &&
                                      change.oldState->x == change.newState->x &&
                                      change.oldState->y == change.newState->y &&
                                      change.oldState->mode != change.newState->mode;
                if (modeOnly) {
                    addAnchorOutgoing(baseline, change.id, ranges);
                } else {
                    addAnchorInfluence(baseline, change.id, ranges);
                }
            }
            return true;
        }

        bool applyFreeChanges(QList<QVariant> &baseline, FreeValueDataArray *array, const WatcherFreeDirty &dirty, QList<ClipChangeRange> &ranges) {
            if (!array || !dirty.isDirty()) {
                return false;
            }
            const int first = std::max(0, dirty.first);
            const bool suffix = dirty.throughEnd || baseline.size() != array->size();
            const int baselineSize = static_cast<int>(baseline.size());
            const int arraySize = array->size();
            const int oldEnd = suffix ? baselineSize : std::min(baselineSize, dirty.last);
            const int newEnd = suffix ? arraySize : std::min(arraySize, dirty.last);
            const QList<QVariant> current = array->slice(first, std::max(0, newEnd - first));
            const QList<QVariant> previous = baseline.mid(first, std::max(0, oldEnd - first));
            if (previous == current && baseline.size() == array->size()) {
                return false;
            }

            const int step = FreeValueDataArray::step();
            auto oldValue = [&baseline](int index) {
                return index >= 0 && index < baseline.size() ? baseline.at(index) : QVariant{};
            };
            auto newValue = [&baseline, &current, first, newEnd, suffix](int index) {
                if (index >= first && index < newEnd) {
                    return current.at(index - first);
                }
                if (suffix && index >= newEnd) {
                    return QVariant{};
                }
                return index >= 0 && index < baseline.size() ? baseline.at(index) : QVariant{};
            };
            const int compareEnd = std::max(oldEnd, newEnd);
            bool outputChanged = false;
            for (int i = first; i < compareEnd; ++i) {
                const QVariant oldPoint = oldValue(i);
                const QVariant newPoint = newValue(i);
                if (oldPoint != newPoint && (oldPoint.isValid() || newPoint.isValid())) {
                    addRange(ranges, i * step, 1);
                    outputChanged = true;
                }
            }
            for (int i = std::max(0, first - 1); i < compareEnd; ++i) {
                const QVariant oldLeft = oldValue(i);
                const QVariant oldRight = oldValue(i + 1);
                const QVariant newLeft = newValue(i);
                const QVariant newRight = newValue(i + 1);
                if (oldLeft == newLeft && oldRight == newRight) {
                    continue;
                }
                if ((oldLeft.isValid() && oldRight.isValid()) ||
                    (newLeft.isValid() && newRight.isValid())) {
                    addRange(ranges, i * step, step);
                    outputChanged = true;
                }
            }

            baseline.erase(baseline.begin() + std::min(first, baselineSize), baseline.begin() + oldEnd);
            for (int i = 0; i < current.size(); ++i) {
                baseline.insert(first + i, current.at(i));
            }
            return outputChanged;
        }

        bool addTimeAffectedFree(const QList<QVariant> &values, const PieceTimeMap &oldMap, int oldStart, const PieceTimeMap &newMap, int newStart, QList<ClipChangeRange> &ranges) {
            if (values.isEmpty()) {
                return false;
            }
            int lastValid = static_cast<int>(values.size()) - 1;
            while (lastValid >= 0 && !values.at(lastValid).isValid()) {
                --lastValid;
            }
            if (lastValid < 0) {
                return false;
            }
            const int step = FreeValueDataArray::step();
            const double end = static_cast<double>(lastValid) * step;
            const auto changed = differentTimeRanges(oldMap, oldStart, newMap, newStart, 0.0, end);
            QSet<int> affectedIndices;
            for (const auto &[left, right] : changed) {
                const int first = std::max(0, static_cast<int>(std::ceil(left / step)));
                const int valueCount = lastValid + 1;
                const int last = std::min(valueCount - 1, static_cast<int>(std::floor(right / step)));
                for (int i = first; i <= last; ++i) {
                    if (values.at(i).isValid() &&
                        oldMap.tickToMilliseconds(oldStart + i * step) !=
                            newMap.tickToMilliseconds(newStart + i * step)) {
                        affectedIndices.insert(i);
                    }
                }
            }
            for (int index : std::as_const(affectedIndices)) {
                addRange(ranges, index * step, 1);
                if (index > 0 && values.at(index - 1).isValid()) {
                    addRange(ranges, (index - 1) * step, step);
                }
                if (index + 1 < values.size() && values.at(index + 1).isValid()) {
                    addRange(ranges, index * step, step);
                }
            }
            return !affectedIndices.isEmpty();
        }

        bool addTimeAffectedAnchors(const WatcherAnchorLayer &layer, const PieceTimeMap &oldMap, int oldStart, const PieceTimeMap &newMap, int newStart, QList<ClipChangeRange> &ranges) {
            if (layer.order.empty()) {
                return false;
            }
            const double domainStart = layer.order.cbegin()->first.x;
            const double domainEnd = layer.order.crbegin()->first.x;
            const auto changed = differentTimeRanges(oldMap, oldStart, newMap, newStart, domainStart, domainEnd);
            QSet<quint64> affectedIds;
            for (const auto &[left, right] : changed) {
                const int first = static_cast<int>(std::ceil(left));
                const int last = static_cast<int>(std::floor(right));
                for (auto it = layer.order.lower_bound(WatcherAnchorOrder{first, 0});
                     it != layer.order.cend() && it->first.x <= last; ++it) {
                    const int x = it->first.x;
                    if (oldMap.tickToMilliseconds(oldStart + x) !=
                        newMap.tickToMilliseconds(newStart + x)) {
                        affectedIds.insert(it->second);
                    }
                }
            }
            bool affected = false;
            for (quint64 id : std::as_const(affectedIds)) {
                addAnchorInfluence(layer, id, ranges);
                affected = true;
            }
            return affected;
        }

    }

    bool WatcherNoteState::vibratoEquals(const WatcherNoteState &other) const {
        return vibratoAmplitude == other.vibratoAmplitude && vibratoEnd == other.vibratoEnd &&
               vibratoFrequency == other.vibratoFrequency && vibratoOffset == other.vibratoOffset &&
               vibratoPhase == other.vibratoPhase && vibratoStart == other.vibratoStart &&
               vibratoAmplitudePoints == other.vibratoAmplitudePoints &&
               vibratoFrequencyPoints == other.vibratoFrequencyPoints;
    }

    void WatcherFreeDirty::include(int firstIndex, int lastIndex, bool suffix) {
        first = std::min(first, std::max(0, firstIndex));
        last = std::max(last, lastIndex);
        throughEnd = throughEnd || suffix;
    }

    ClipWatcherPrivate::ClipWatcherPrivate(ClipWatcher *q)
        : q_ptr(q) {
    }

    ClipWatcherPrivate::~ClipWatcherPrivate() {
        delete watchContext;
    }

    void ClipWatcherPrivate::clearPending() {
        pendingNoteIds.clear();
        pendingParameters.clear();
        pendingTempoIds.clear();
        clipStartDirty = false;
        clipTimingDirty = false;
        sourcesDirty = false;
    }

    void ClipWatcherPrivate::bind(SingingClip *clip) {
        delete watchContext;
        watchContext = nullptr;
        singingClip = clip;
        installWatchers();
        captureBaseline();
    }

    bool ClipWatcherPrivate::beginWatching(QObject *object) {
        if (!object || !watchContext || watchedObjects.contains(object)) {
            return false;
        }
        watchedObjects.insert(object);
        QObject::connect(object, &QObject::destroyed, watchContext, [this](QObject *destroyed) { watchedObjects.remove(destroyed); });
        return true;
    }

    void ClipWatcherPrivate::markNote(quint64 id) {
        if (singingClip && id) {
            pendingNoteIds.insert(id);
        }
    }

    void ClipWatcherPrivate::markParameterMembership(quint64 id) {
        if (singingClip && id) {
            pendingParameters[id].membership = true;
        }
    }

    void ClipWatcherPrivate::markFree(quint64 id, bool edited, int first, int last, bool throughEnd) {
        auto &dirty = edited ? pendingParameters[id].freeEdited
                             : pendingParameters[id].freeTransform;
        dirty.include(first, last, throughEnd);
    }

    void ClipWatcherPrivate::markAnchor(quint64 parameterId, bool edited, quint64 anchorId) {
        auto &dirty = pendingParameters[parameterId];
        (edited ? dirty.anchorEdited : dirty.anchorTransform).insert(anchorId);
    }

    QList<WatcherPhonemeState> ClipWatcherPrivate::capturePhonemes(PhonemeSequence *sequence) const {
        QList<WatcherPhonemeState> result;
        if (!sequence) {
            return result;
        }
        result.reserve(sequence->size());
        for (Phoneme *phoneme : sequence->asRange()) {
            result.append({phoneme->handle().d, phoneme->language(), phoneme->token(), phoneme->start(), phoneme->onset()});
        }
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) {
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

    WatcherNoteState ClipWatcherPrivate::captureNote(Note *note) const {
        WatcherNoteState result;
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
        result.editedPhonemes = capturePhonemes(note->editedPhonemes());
        return result;
    }

    WatcherAnchorState ClipWatcherPrivate::captureAnchor(AnchorNode *node) const {
        return {node->handle().d, node->x(), node->y(), static_cast<int>(node->interpolationMode())};
    }

    WatcherAnchorLayer ClipWatcherPrivate::captureAnchors(AnchorNodeSequence *sequence) const {
        WatcherAnchorLayer result;
        for (AnchorNode *node : sequence->asRange()) {
            const auto state = captureAnchor(node);
            result.states.insert(state.id, state);
            result.order.emplace(WatcherAnchorOrder{state.x, state.id}, state.id);
        }
        return result;
    }

    WatcherParameterState ClipWatcherPrivate::captureParameter(const QString &name, Parameter *parameter) const {
        WatcherParameterState result;
        result.id = parameter->handle().d;
        result.name = name;
        result.freeEdited = parameter->freeEdited()->items();
        result.freeTransform = parameter->freeTransform()->items();
        result.anchorEdited = captureAnchors(parameter->anchorEdited());
        result.anchorTransform = captureAnchors(parameter->anchorTransform());
        return result;
    }

    QByteArray ClipWatcherPrivate::captureSources() const {
        if (!singingClip || !singingClip->sources()) {
            return {};
        }
        const auto encodeSinger = [&](const auto &self, Singer *singer) -> QJsonObject {
            QJsonObject object{
                {QStringLiteral("type"), static_cast<int>(singer->kind())},
                {QStringLiteral("extra"), singer->extra()},
            };
            if (singer->kind() == Singer::Single) {
                object.insert(QStringLiteral("id"), static_cast<SingleSinger *>(singer)->id());
            } else {
                auto mixed = static_cast<MixedSinger *>(singer);
                QJsonArray ratio;
                for (double value : mixed->ratio()) {
                    ratio.append(value);
                }
                QJsonArray children;
                for (Singer *child : mixed->singers()->items()) {
                    children.append(self(self, child));
                }
                object.insert(QStringLiteral("ratio"), ratio);
                object.insert(QStringLiteral("singers"), children);
            }
            return object;
        };

        Sources *sources = singingClip->sources();
        QJsonArray singers;
        for (Singer *singer : sources->singers()->items()) {
            singers.append(encodeSinger(encodeSinger, singer));
        }
        QJsonArray anchors;
        for (DynamicMixingAnchor *anchor : sources->dynamicMixingAnchors()->asRange()) {
            QJsonArray ratio;
            for (double value : anchor->ratio()) {
                ratio.append(value);
            }
            anchors.append(QJsonObject{
                {QStringLiteral("position"), anchor->position()},
                {QStringLiteral("ratio"), ratio},
            });
        }
        return QJsonDocument(QJsonObject{
                                 {QStringLiteral("category"), sources->category()},
                                 {QStringLiteral("singers"), singers},
                                 {QStringLiteral("anchors"), anchors},
                             })
            .toJson(QJsonDocument::Compact);
    }

    void ClipWatcherPrivate::captureBaseline() {
        watchedObjects.remove(nullptr);
        baselineNotes.clear();
        noteStarts.clear();
        noteEnds.clear();
        baselineParameters.clear();
        clearPending();
        if (!singingClip) {
            timeMapCache.reset(nullptr);
            baselineTimeMap = timeMapCache.timeMap();
            baselineClipStart = 0;
            return;
        }
        timeMapCache.reset(singingClip->model()->tempos());
        baselineTimeMap = timeMapCache.timeMap();
        baselineClipStart = singingClip->start();
        baselinePosition = singingClip->position();
        baselineLength = singingClip->length();
        baselineVisibleStart = singingClip->clipStart();
        baselineVisibleLength = singingClip->clipLength();
        baselineSources = captureSources();
        for (Note *note : singingClip->notes()->asRange()) {
            const auto state = captureNote(note);
            baselineNotes.insert(state.id, state);
            noteStarts.emplace(state.position, state.id);
            noteEnds.emplace(state.position + state.length, state.id);
        }
        ParameterMap *map = singingClip->parameters();
        for (const QString &name : map->keys()) {
            Parameter *parameter = map->item(name);
            if (parameter) {
                baselineParameters.insert(parameter->handle().d, captureParameter(name, parameter));
            }
        }
    }

    void ClipWatcherPrivate::installPhonemeWatcher(Phoneme *phoneme, quint64 noteId) {
        if (!phoneme) {
            return;
        }
        const quint64 id = phoneme->handle().d;
        phonemeOwners.insert(id, noteId);
        if (!beginWatching(phoneme)) {
            return;
        }
        auto changed = [this, id] { markNote(phonemeOwners.value(id)); };
        QObject::connect(phoneme, &Phoneme::languageChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::startChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::tokenChanged, watchContext, changed);
        QObject::connect(phoneme, &Phoneme::onsetChanged, watchContext, changed);
        QObject::connect(phoneme, &QObject::destroyed, watchContext, [this, id] { markNote(phonemeOwners.value(id)); });
    }

    void ClipWatcherPrivate::installPhonemeWatcher(PhonemeSequence *sequence, quint64 noteId) {
        if (!sequence || !beginWatching(sequence)) {
            return;
        }
        for (Phoneme *phoneme : sequence->asRange()) {
            installPhonemeWatcher(phoneme, noteId);
        }
        QObject::connect(sequence, &PhonemeSequence::itemAboutToRemove, watchContext, [this, noteId](Phoneme *, PhonemeSequence *) { markNote(noteId); });
        QObject::connect(sequence, &PhonemeSequence::itemInserted, watchContext, [this, noteId](Phoneme *phoneme, PhonemeSequence *) {
            installPhonemeWatcher(phoneme, noteId);
            markNote(noteId);
        });
    }

    void ClipWatcherPrivate::installNoteWatcher(Note *note) {
        if (!note) {
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
        QObject::connect(note, &QObject::destroyed, watchContext, [this, id] { markNote(id); });
        installPhonemeWatcher(note->editedPhonemes(), id);
        QObject::connect(note->vibratoAmplitudeControlPoints(), &VibratoPointDataArray::spliced, watchContext, [changed](int, int, const QList<QPointF> &) { changed(); });
        QObject::connect(note->vibratoAmplitudeControlPoints(), &VibratoPointDataArray::rotated, watchContext, [changed](int, int, int) { changed(); });
        QObject::connect(note->vibratoFrequencyControlPoints(), &VibratoPointDataArray::spliced, watchContext, [changed](int, int, const QList<QPointF> &) { changed(); });
        QObject::connect(note->vibratoFrequencyControlPoints(), &VibratoPointDataArray::rotated, watchContext, [changed](int, int, int) { changed(); });
    }

    void ClipWatcherPrivate::installTempoWatcher(Tempo *tempo) {
        if (!tempo || !beginWatching(tempo)) {
            return;
        }
        const quint64 id = tempo->handle().d;
        auto changed = [this, id] { pendingTempoIds.insert(id); };
        QObject::connect(tempo, &Tempo::positionChanged, watchContext, changed);
        QObject::connect(tempo, &Tempo::valueChanged, watchContext, changed);
        QObject::connect(tempo, &QObject::destroyed, watchContext, [this, id] { pendingTempoIds.insert(id); });
    }

    void ClipWatcherPrivate::installFreeWatcher(FreeValueDataArray *array, quint64 parameterId, bool edited) {
        if (!array || !beginWatching(array)) {
            return;
        }
        QObject::connect(array, &FreeValueDataArray::spliced, watchContext, [this, parameterId, edited](int index, int removed, const QList<QVariant> &values) {
            const int inserted = values.size();
            markFree(parameterId, edited, std::max(0, index - 1), index + std::max(removed, inserted) + 2, removed != inserted);
        });
        QObject::connect(array, &FreeValueDataArray::rotated, watchContext, [this, parameterId, edited](int left, int, int right) {
            markFree(parameterId, edited, std::max(0, left - 1), right + 1, false);
        });
        QObject::connect(array, &QObject::destroyed, watchContext, [this, parameterId, edited] {
            markFree(parameterId, edited, 0, 0, true);
        });
    }

    void ClipWatcherPrivate::installAnchorWatcher(AnchorNode *node, quint64 parameterId, bool edited) {
        if (!node) {
            return;
        }
        const quint64 id = node->handle().d;
        anchorPointers.insert(id, node);
        anchorOwners.insert(id, {parameterId, edited});
        if (!beginWatching(node)) {
            return;
        }
        auto changed = [this, id] {
            const auto owner = anchorOwners.value(id);
            markAnchor(owner.first, owner.second, id);
        };
        QObject::connect(node, &AnchorNode::xChanged, watchContext, changed);
        QObject::connect(node, &AnchorNode::yChanged, watchContext, changed);
        QObject::connect(node, &AnchorNode::interpolationModeChanged, watchContext, changed);
        QObject::connect(node, &QObject::destroyed, watchContext, [this, id] {
            const auto owner = anchorOwners.value(id);
            markAnchor(owner.first, owner.second, id);
        });
    }

    void ClipWatcherPrivate::installAnchorWatcher(AnchorNodeSequence *sequence, quint64 parameterId, bool edited) {
        if (!sequence || !beginWatching(sequence)) {
            return;
        }
        for (AnchorNode *node : sequence->asRange()) {
            installAnchorWatcher(node, parameterId, edited);
        }
        QObject::connect(sequence, &AnchorNodeSequence::itemAboutToRemove, watchContext, [this, parameterId, edited](AnchorNode *node, AnchorNodeSequence *) {
            markAnchor(parameterId, edited, node->handle().d);
        });
        QObject::connect(sequence, &AnchorNodeSequence::itemInserted, watchContext, [this, parameterId, edited](AnchorNode *node, AnchorNodeSequence *) {
            installAnchorWatcher(node, parameterId, edited);
            markAnchor(parameterId, edited, node->handle().d);
        });
    }

    void ClipWatcherPrivate::installParameterWatcher(const QString &, Parameter *parameter) {
        if (!parameter) {
            return;
        }
        const quint64 id = parameter->handle().d;
        parameterPointers.insert(id, parameter);
        if (!beginWatching(parameter)) {
            return;
        }
        installFreeWatcher(parameter->freeEdited(), id, true);
        installFreeWatcher(parameter->freeTransform(), id, false);
        installAnchorWatcher(parameter->anchorEdited(), id, true);
        installAnchorWatcher(parameter->anchorTransform(), id, false);
        QObject::connect(parameter, &QObject::destroyed, watchContext, [this, id] { markParameterMembership(id); });
    }

    void ClipWatcherPrivate::installSingerWatcher(Singer *singer) {
        if (!singer || !beginWatching(singer)) {
            return;
        }
        auto changed = [this] { sourcesDirty = true; };
        QObject::connect(singer, &Singer::extraChanged, watchContext, changed);
        if (singer->kind() == Singer::Single) {
            QObject::connect(static_cast<SingleSinger *>(singer), &SingleSinger::idChanged, watchContext, changed);
            return;
        }
        auto mixed = static_cast<MixedSinger *>(singer);
        QObject::connect(mixed, &MixedSinger::ratioChanged, watchContext, changed);
        installSingerListWatchers(mixed->singers());
    }

    void ClipWatcherPrivate::installSingerListWatchers(SingerList *list) {
        if (!list || !beginWatching(list)) {
            return;
        }
        for (Singer *singer : list->items()) {
            installSingerWatcher(singer);
        }
        QObject::connect(list, &SingerList::itemAboutToRemove, watchContext, [this](int, Singer *, SingerList *) { sourcesDirty = true; });
        QObject::connect(list, &SingerList::itemInserted, watchContext, [this](int, Singer *singer, SingerList *) {
            installSingerWatcher(singer);
            sourcesDirty = true;
        });
        QObject::connect(list, &SingerList::rotated, watchContext, [this](int, int, int) { sourcesDirty = true; });
    }

    void ClipWatcherPrivate::installSourceWatchers(Sources *sources) {
        if (!sources || !beginWatching(sources)) {
            return;
        }
        QObject::connect(sources, &Sources::categoryChanged, watchContext, [this] { sourcesDirty = true; });
        installSingerListWatchers(sources->singers());
        auto anchors = sources->dynamicMixingAnchors();
        if (beginWatching(anchors)) {
            auto watchAnchor = [this](DynamicMixingAnchor *anchor) {
                if (!anchor || !beginWatching(anchor)) {
                    return;
                }
                QObject::connect(anchor, &DynamicMixingAnchor::positionChanged, watchContext, [this] { sourcesDirty = true; });
                QObject::connect(anchor, &DynamicMixingAnchor::ratioChanged, watchContext, [this] { sourcesDirty = true; });
            };
            for (DynamicMixingAnchor *anchor : anchors->asRange()) {
                watchAnchor(anchor);
            }
            QObject::connect(anchors, &DynamicMixingAnchorSequence::itemAboutToRemove, watchContext, [this](DynamicMixingAnchor *, DynamicMixingAnchorSequence *) {
                sourcesDirty = true;
            });
            QObject::connect(anchors, &DynamicMixingAnchorSequence::itemInserted, watchContext, [this, watchAnchor](DynamicMixingAnchor *anchor, DynamicMixingAnchorSequence *) {
                watchAnchor(anchor);
                sourcesDirty = true;
            });
        }
    }

    void ClipWatcherPrivate::installWatchers() {
        watchedObjects.clear();
        notePointers.clear();
        phonemeOwners.clear();
        parameterPointers.clear();
        anchorPointers.clear();
        anchorOwners.clear();
        if (!singingClip) {
            return;
        }
        watchContext = new QObject(q_ptr);
        QObject::connect(singingClip, &QObject::destroyed, watchContext, [this] {
            singingClip = nullptr;
            baselineNotes.clear();
            baselineParameters.clear();
            noteStarts.clear();
            noteEnds.clear();
            clearPending();
            emit q_ptr->singingClipChanged(nullptr);
        });
        QObject::connect(singingClip, &SingingClip::startChanged, watchContext, [this](int) { clipStartDirty = true; });
        auto timingChanged = [this] { clipTimingDirty = true; };
        QObject::connect(singingClip, &SingingClip::positionChanged, watchContext, timingChanged);
        QObject::connect(singingClip, &SingingClip::lengthChanged, watchContext, timingChanged);
        QObject::connect(singingClip, &SingingClip::clipStartChanged, watchContext, timingChanged);
        QObject::connect(singingClip, &SingingClip::clipLengthChanged, watchContext, timingChanged);
        QObject::connect(singingClip, &SingingClip::sourcesChanged, watchContext, [this](Sources *sources) {
            sourcesDirty = true;
            installSourceWatchers(sources);
        });
        installSourceWatchers(singingClip->sources());

        NoteSequence *notes = singingClip->notes();
        for (Note *note : notes->asRange()) {
            installNoteWatcher(note);
        }
        QObject::connect(notes, &NoteSequence::itemAboutToRemove, watchContext, [this](Note *note, NoteSequence *) { markNote(note->handle().d); });
        QObject::connect(notes, &NoteSequence::itemInserted, watchContext, [this](Note *note, NoteSequence *) {
            installNoteWatcher(note);
            markNote(note->handle().d);
        });

        TempoSequence *tempos = singingClip->model()->tempos();
        for (Tempo *tempo : tempos->asRange()) {
            installTempoWatcher(tempo);
        }
        QObject::connect(tempos, &TempoSequence::itemAboutToRemove, watchContext, [this](Tempo *tempo) { pendingTempoIds.insert(tempo->handle().d); });
        QObject::connect(tempos, &TempoSequence::itemInserted, watchContext, [this](Tempo *tempo) {
            installTempoWatcher(tempo);
            pendingTempoIds.insert(tempo->handle().d);
        });

        ParameterMap *parameters = singingClip->parameters();
        for (const QString &name : parameters->keys()) {
            installParameterWatcher(name, parameters->item(name));
        }
        QObject::connect(parameters, &ParameterMap::itemAboutToRemove, watchContext, [this](const QString &, Parameter *parameter, ParameterMap *) {
            markParameterMembership(parameter->handle().d);
        });
        QObject::connect(parameters, &ParameterMap::itemInserted, watchContext, [this](const QString &name, Parameter *parameter, ParameterMap *) {
            installParameterWatcher(name, parameter);
            markParameterMembership(parameter->handle().d);
        });
    }

    std::optional<WatcherNoteState> ClipWatcherPrivate::currentNote(quint64 id) const {
        if (!singingClip) {
            return std::nullopt;
        }
        Note *note = notePointers.value(id);
        if (!note) {
            note = singingClip->model()->find<Note>(Handle{id});
        }
        if (!note || note->noteSequence() != singingClip->notes()) {
            return std::nullopt;
        }
        return captureNote(note);
    }

    std::optional<WatcherParameterState> ClipWatcherPrivate::currentParameter(quint64 id) const {
        if (!singingClip) {
            return std::nullopt;
        }
        Parameter *parameter = parameterPointers.value(id);
        if (!parameter || parameter->parameterMap() != singingClip->parameters()) {
            return std::nullopt;
        }
        for (const QString &name : singingClip->parameters()->keys()) {
            if (singingClip->parameters()->item(name) == parameter) {
                return captureParameter(name, parameter);
            }
        }
        return std::nullopt;
    }

    ClipChange ClipWatcherPrivate::takeChanges() {
        if (!singingClip) {
            clearPending();
            return {};
        }

        ClipChange::ChangeTypes types;
        QSet<QString> parameterNameSet;
        QList<ClipChangeRange> ranges;
        if (sourcesDirty) {
            const auto sources = captureSources();
            if (sources != baselineSources) {
                types |= ClipChange::Sources;
                addRange(ranges, 0, std::max(1, singingClip->length()));
                baselineSources = sources;
            }
        }
        if (clipTimingDirty) {
            const bool changed = baselinePosition != singingClip->position() ||
                                 baselineLength != singingClip->length() ||
                                 baselineVisibleStart != singingClip->clipStart() ||
                                 baselineVisibleLength != singingClip->clipLength();
            if (changed) {
                types |= ClipChange::ClipTiming;
                addRange(ranges, 0, std::max({1, baselineLength, singingClip->length()}));
                baselinePosition = singingClip->position();
                baselineLength = singingClip->length();
                baselineVisibleStart = singingClip->clipStart();
                baselineVisibleLength = singingClip->clipLength();
            }
        }
        if (!pendingTempoIds.isEmpty()) {
            timeMapCache.update(singingClip->model()->tempos(), pendingTempoIds);
        }
        const PieceTimeMap newTimeMap = timeMapCache.timeMap();
        const int newClipStart = clipStartDirty ? singingClip->start() : baselineClipStart;
        const bool timeMappingChanged = newClipStart != baselineClipStart ||
                                        newTimeMap.segments != baselineTimeMap.segments;

        QSet<quint64> noteIds = pendingNoteIds;
        if (timeMappingChanged) {
            if (!noteStarts.empty() || !noteEnds.empty()) {
                int domainStart = std::numeric_limits<int>::max();
                int domainEnd = std::numeric_limits<int>::min();
                if (!noteStarts.empty()) {
                    domainStart = std::min(domainStart, noteStarts.begin()->first);
                    domainEnd = std::max(domainEnd, noteStarts.rbegin()->first);
                }
                if (!noteEnds.empty()) {
                    domainStart = std::min(domainStart, noteEnds.begin()->first);
                    domainEnd = std::max(domainEnd, noteEnds.rbegin()->first);
                }
                const auto changed = differentTimeRanges(baselineTimeMap, baselineClipStart, newTimeMap, newClipStart, domainStart, domainEnd);
                addIndexedIds(noteStarts, changed, noteIds);
                addIndexedIds(noteEnds, changed, noteIds);
            }
        }

        for (quint64 id : std::as_const(noteIds)) {
            const auto oldIt = baselineNotes.constFind(id);
            const std::optional<WatcherNoteState> oldState = oldIt == baselineNotes.cend()
                                                                 ? std::nullopt
                                                                 : std::optional(*oldIt);
            const auto newState = currentNote(id);
            ClipChange::ChangeTypes noteTypes;
            if (oldState.has_value() != newState.has_value()) {
                noteTypes |= ClipChange::Score;
            } else if (oldState && newState) {
                if (oldState->lyric != newState->lyric || oldState->language != newState->language) {
                    noteTypes |= ClipChange::Lyric;
                }
                if (oldState->editedPronunciation != newState->editedPronunciation) {
                    noteTypes |= ClipChange::Pronunciation;
                }
                const double oldStartMs = baselineTimeMap.tickToMilliseconds(
                    static_cast<double>(baselineClipStart) + oldState->position
                );
                const double oldEndMs = baselineTimeMap.tickToMilliseconds(
                    static_cast<double>(baselineClipStart) + oldState->position + oldState->length
                );
                const double newStartMs = newTimeMap.tickToMilliseconds(
                    static_cast<double>(newClipStart) + newState->position
                );
                const double newEndMs = newTimeMap.tickToMilliseconds(
                    static_cast<double>(newClipStart) + newState->position + newState->length
                );
                if (oldState->keyNumber != newState->keyNumber ||
                    oldState->centShift != newState->centShift ||
                    oldStartMs != newStartMs || oldEndMs != newEndMs) {
                    noteTypes |= ClipChange::Note;
                }
                if (oldState->editedPhonemes != newState->editedPhonemes) {
                    noteTypes |= ClipChange::Phoneme;
                }
                if (!oldState->vibratoEquals(*newState)) {
                    noteTypes |= ClipChange::Vibrato;
                }
            }
            if (noteTypes) {
                types |= noteTypes;
                if (oldState) {
                    addNoteRange(ranges, *oldState);
                }
                if (newState) {
                    addNoteRange(ranges, *newState);
                }
            }

            if (oldState) {
                removeIndexedId(noteStarts, oldState->position, id);
                removeIndexedId(noteEnds, oldState->position + oldState->length, id);
                baselineNotes.remove(id);
            }
            if (newState) {
                baselineNotes.insert(id, *newState);
                noteStarts.emplace(newState->position, id);
                noteEnds.emplace(newState->position + newState->length, id);
            }
        }

        for (auto dirtyIt = pendingParameters.begin(); dirtyIt != pendingParameters.end(); ++dirtyIt) {
            const quint64 id = dirtyIt.key();
            auto oldIt = baselineParameters.find(id);
            if (dirtyIt->membership) {
                const std::optional<WatcherParameterState> oldState = oldIt == baselineParameters.end()
                                                                          ? std::nullopt
                                                                          : std::optional(*oldIt);
                const auto newState = currentParameter(id);
                if (oldState.has_value() != newState.has_value() ||
                    (oldState && newState && !parameterStatesEqual(*oldState, *newState))) {
                    types |= ClipChange::Parameter;
                    if (oldState) {
                        parameterNameSet.insert(oldState->name);
                        addParameterDomain(*oldState, ranges);
                    }
                    if (newState) {
                        parameterNameSet.insert(newState->name);
                        addParameterDomain(*newState, ranges);
                    }
                }
                baselineParameters.remove(id);
                if (newState) {
                    baselineParameters.insert(id, *newState);
                }
                continue;
            }
            if (oldIt == baselineParameters.end()) {
                continue;
            }
            Parameter *parameter = parameterPointers.value(id);
            if (!parameter || parameter->parameterMap() != singingClip->parameters()) {
                continue;
            }
            bool changed = false;
            changed |= applyFreeChanges(oldIt->freeEdited, parameter->freeEdited(), dirtyIt->freeEdited, ranges);
            changed |= applyFreeChanges(oldIt->freeTransform, parameter->freeTransform(), dirtyIt->freeTransform, ranges);
            changed |= applyAnchorChanges(oldIt->anchorEdited, dirtyIt->anchorEdited, anchorPointers, parameter->anchorEdited(), ranges);
            changed |= applyAnchorChanges(oldIt->anchorTransform, dirtyIt->anchorTransform, anchorPointers, parameter->anchorTransform(), ranges);
            if (changed) {
                types |= ClipChange::Parameter;
                parameterNameSet.insert(oldIt->name);
            }
        }

        if (timeMappingChanged) {
            for (auto it = baselineParameters.begin(); it != baselineParameters.end(); ++it) {
                bool affected = false;
                affected |= addTimeAffectedFree(it->freeEdited, baselineTimeMap, baselineClipStart, newTimeMap, newClipStart, ranges);
                affected |= addTimeAffectedFree(it->freeTransform, baselineTimeMap, baselineClipStart, newTimeMap, newClipStart, ranges);
                affected |= addTimeAffectedAnchors(it->anchorEdited, baselineTimeMap, baselineClipStart, newTimeMap, newClipStart, ranges);
                affected |= addTimeAffectedAnchors(it->anchorTransform, baselineTimeMap, baselineClipStart, newTimeMap, newClipStart, ranges);
                if (affected) {
                    types |= ClipChange::Parameter;
                    parameterNameSet.insert(it->name);
                }
            }
        }

        baselineTimeMap = newTimeMap;
        baselineClipStart = newClipStart;
        clearPending();
        QStringList names(parameterNameSet.cbegin(), parameterNameSet.cend());
        return ClipChange(types, std::move(names), std::move(ranges));
    }

    ClipWatcher::ClipWatcher(QObject *parent)
        : QObject(parent), d_ptr(new ClipWatcherPrivate(this)) {
        qRegisterMetaType<ClipChange>();
        qRegisterMetaType<ClipChangeRange>();
    }

    ClipWatcher::~ClipWatcher() = default;

    SingingClip *ClipWatcher::singingClip() const {
        Q_D(const ClipWatcher);
        return d->singingClip;
    }

    void ClipWatcher::setSingingClip(SingingClip *singingClip) {
        Q_D(ClipWatcher);
        if (d->singingClip == singingClip) {
            return;
        }
        d->bind(singingClip);
        emit singingClipChanged(singingClip);
    }

    ClipChange ClipWatcher::takeChanges() {
        Q_D(ClipWatcher);
        return d->takeChanges();
    }

}

#include "moc_ClipWatcher.cpp"
