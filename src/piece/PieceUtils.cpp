#include "PieceUtils_p.h"

#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <utility>

namespace dspx {

    namespace {

        constexpr double ticksPerBeat = 480.0;
        constexpr double defaultTempo = 120.0;

        double millisecondsPerTick(double bpm) {
            return 60000.0 / (ticksPerBeat * bpm);
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
                                         [](double value, const Segment &segment) {
                                             return value < segment.tick;
                                         });
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
                                         [](double value, const Segment &segment) {
                                             return value < segment.milliseconds;
                                         });
        const auto &segment = it == segments.cbegin() ? segments.constFirst() : *std::prev(it);
        return segment.tick + (milliseconds - segment.milliseconds) / millisecondsPerTick(segment.bpm);
    }

    double PieceTimeMap::bpmAtTick(double tick) const {
        if (segments.isEmpty()) {
            return defaultTempo;
        }
        if (tick < 0.0) {
            return segments.constFirst().bpm;
        }
        const auto it = std::upper_bound(segments.cbegin(), segments.cend(), tick,
                                         [](double value, const Segment &segment) {
                                             return value < segment.tick;
                                         });
        return (it == segments.cbegin() ? segments.constFirst() : *std::prev(it)).bpm;
    }

    void PieceTimeMapCache::reset(TempoSequence *tempos) {
        m_states.clear();
        m_order.clear();
        if (tempos) {
            for (Tempo *tempo : tempos->asRange()) {
                const PieceTempoState state {tempo->handle().d, tempo->position(), tempo->value()};
                m_states.insert(state.id, state);
                m_order.emplace(PieceTempoOrder {state.position, state.id}, state.id);
            }
        }
        m_timeMap = {};
        rebuildFrom(std::numeric_limits<int>::min());
    }

    void PieceTimeMapCache::update(TempoSequence *tempos, const QSet<quint64> &tempoIds) {
        if (!tempos || tempoIds.isEmpty()) {
            return;
        }
        int firstChangedPosition = std::numeric_limits<int>::max();
        for (quint64 id : tempoIds) {
            const auto oldIt = m_states.constFind(id);
            if (oldIt != m_states.cend()) {
                firstChangedPosition = std::min(firstChangedPosition, oldIt->position);
                m_order.erase(PieceTempoOrder {oldIt->position, id});
                m_states.remove(id);
            }

            Tempo *tempo = tempos->model()->find<Tempo>(Handle {id});
            if (tempo && tempo->tempoSequence() == tempos) {
                const PieceTempoState state {id, tempo->position(), tempo->value()};
                firstChangedPosition = std::min(firstChangedPosition, state.position);
                m_states.insert(id, state);
                m_order.emplace(PieceTempoOrder {state.position, id}, id);
            }
        }
        if (firstChangedPosition != std::numeric_limits<int>::max()) {
            rebuildFrom(firstChangedPosition);
        }
    }

    void PieceTimeMapCache::rebuildFrom(int firstChangedPosition) {
        QVector<PieceTimeMap::Segment> segments;
        double previousTick = 0.0;
        double previousMilliseconds = 0.0;
        double previousTempo = defaultTempo;
        std::map<PieceTempoOrder, quint64>::const_iterator pointIt;

        if (firstChangedPosition > 0 && !m_timeMap.segments.isEmpty()) {
            for (const auto &segment : std::as_const(m_timeMap.segments)) {
                if (segment.tick >= firstChangedPosition) {
                    break;
                }
                segments.append(segment);
            }
        }

        if (segments.isEmpty()) {
            const auto atZero = m_order.lower_bound(PieceTempoOrder {0, 0});
            if (atZero != m_order.cend() && atZero->first.position == 0) {
                previousTempo = m_states.value(atZero->second).bpm;
            }
            segments.append({0.0, 0.0, previousTempo});
            pointIt = m_order.upper_bound(PieceTempoOrder {
                0, std::numeric_limits<quint64>::max()});
        } else {
            const auto &last = segments.constLast();
            previousTick = last.tick;
            previousMilliseconds = last.milliseconds;
            previousTempo = last.bpm;
            pointIt = m_order.upper_bound(PieceTempoOrder {
                static_cast<int>(previousTick), std::numeric_limits<quint64>::max()});
        }

        for (; pointIt != m_order.cend(); ++pointIt) {
            const auto state = m_states.value(pointIt->second);
            const double millisecondsAtPoint = previousMilliseconds +
                                               (state.position - previousTick) *
                                                   millisecondsPerTick(previousTempo);
            if (state.bpm == previousTempo) {
                continue;
            }
            segments.append({static_cast<double>(state.position), millisecondsAtPoint, state.bpm});
            previousTick = state.position;
            previousMilliseconds = millisecondsAtPoint;
            previousTempo = state.bpm;
        }
        m_timeMap.segments = std::move(segments);
    }

}
