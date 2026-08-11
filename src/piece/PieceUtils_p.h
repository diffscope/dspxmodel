#ifndef DSPXMODEL_PIECEUTILS_P_H
#define DSPXMODEL_PIECEUTILS_P_H

#include <QHash>
#include <QSet>
#include <QVector>

#include <map>

namespace dspx {

    class TempoSequence;

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
        double bpmAtTick(double tick) const;
    };

    struct PieceTempoState {
        quint64 id = 0;
        int position = 0;
        double bpm = 120.0;

        bool operator==(const PieceTempoState &) const = default;
    };

    struct PieceTempoOrder {
        int position = 0;
        quint64 id = 0;

        bool operator<(const PieceTempoOrder &other) const {
            return position < other.position || (position == other.position && id < other.id);
        }
    };

    class PieceTimeMapCache {
    public:
        void reset(TempoSequence *tempos);
        void update(TempoSequence *tempos, const QSet<quint64> &tempoIds);
        const PieceTimeMap &timeMap() const { return m_timeMap; }

    private:
        void rebuildFrom(int firstChangedPosition);

        QHash<quint64, PieceTempoState> m_states;
        std::map<PieceTempoOrder, quint64> m_order;
        PieceTimeMap m_timeMap;
    };

}

#endif // DSPXMODEL_PIECEUTILS_P_H
