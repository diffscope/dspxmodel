#include "PieceDivider.h"
#include "PieceDivider_p.h"

#include "Piece.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <dini/change.h>
#include <dini/engine.h>
#include <dini/handles.h>
#include <dini/value.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Handle.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>

namespace dspx {

    namespace {

        constexpr double ticksPerBeat = 480.0;
        constexpr double defaultTempo = 120.0;
        constexpr std::int64_t originalPhonemeRole = 0;

        using SnapshotMap = std::unordered_map<dini::ItemId, dini::ItemSnapshot>;

        struct PieceValue {
            double position = 0;
            double length = 0;
        };

        struct TempoPoint {
            double tick = 0;
            double milliseconds = 0;
            double bpm = defaultTempo;
        };

        class TempoMap {
        public:
            explicit TempoMap(const TempoSequence *tempos) {
                std::vector<Tempo *> markers;
                markers.reserve(static_cast<std::size_t>(tempos->size()));
                for (auto *tempo : tempos->asRange()) {
                    markers.push_back(tempo);
                }
                std::ranges::sort(markers, [](const Tempo *lhs, const Tempo *rhs) {
                    if (lhs->position() != rhs->position()) {
                        return lhs->position() < rhs->position();
                    }
                    return lhs->handle().d < rhs->handle().d;
                });

                double currentTick = 0;
                double currentMilliseconds = 0;
                double currentBpm = defaultTempo;
                m_points.reserve(markers.size());
                for (const auto *tempo : markers) {
                    const auto markerTick = static_cast<double>(tempo->position());
                    currentMilliseconds += (markerTick - currentTick) * millisecondsPerTick(currentBpm);
                    currentTick = markerTick;
                    currentBpm = tempo->value();
                    m_points.push_back({markerTick, currentMilliseconds, currentBpm});
                }
            }

            double tickToMilliseconds(double tick) const {
                if (tick < 0) {
                    return tick * millisecondsPerTick(defaultTempo);
                }
                const auto it = std::upper_bound(m_points.begin(), m_points.end(), tick, [](double value, const TempoPoint &point) {
                    return value < point.tick;
                });
                if (it == m_points.begin()) {
                    return tick * millisecondsPerTick(defaultTempo);
                }
                const auto &point = *std::prev(it);
                return point.milliseconds + (tick - point.tick) * millisecondsPerTick(point.bpm);
            }

            double millisecondsToTick(double milliseconds) const {
                if (milliseconds < 0) {
                    return milliseconds / millisecondsPerTick(defaultTempo);
                }
                const auto it = std::upper_bound(m_points.begin(), m_points.end(), milliseconds, [](double value, const TempoPoint &point) {
                    return value < point.milliseconds;
                });
                if (it == m_points.begin()) {
                    return milliseconds / millisecondsPerTick(defaultTempo);
                }
                const auto &point = *std::prev(it);
                return point.tick + (milliseconds - point.milliseconds) / millisecondsPerTick(point.bpm);
            }

        private:
            static double millisecondsPerTick(double bpm) {
                return 60000.0 / (bpm * ticksPerBeat);
            }

            std::vector<TempoPoint> m_points;
        };

        struct NoteData {
            quint64 id = 0;
            int position = 0;
            double startMilliseconds = 0;
            double endMilliseconds = 0;
            double leftPadding = 0;
            double rightPadding = 0;
        };

        bool isSnapshotFrom(const dini::ItemSnapshot &snapshot, const dini::TableHandle &table) {
            return snapshot.containerKind == dini::ContainerKind::Table && snapshot.containerId == table.containerId();
        }

        const dini::Value *snapshotValue(const dini::ItemSnapshot &snapshot, const dini::ColumnHandle &column) {
            const auto it = std::ranges::find(snapshot.values, column, &dini::ColumnValue::column);
            return it == snapshot.values.end() ? nullptr : &it->value;
        }

        std::optional<dini::ItemId> itemIdFromValue(const dini::Value &value) {
            if (value.isNull()) {
                return std::nullopt;
            }
            if (value.type() == dini::ValueType::UInt64) {
                return value.asUInt64();
            }
            if (value.type() == dini::ValueType::Int64 && value.asInt64() >= 0) {
                return static_cast<dini::ItemId>(value.asInt64());
            }
            return std::nullopt;
        }

        bool valueReferences(const dini::Value &value, dini::ItemId itemId) {
            const auto referencedId = itemIdFromValue(value);
            return referencedId && *referencedId == itemId;
        }

        std::optional<std::int64_t> integerValue(const dini::Value *value) {
            if (!value || value->isNull()) {
                return std::nullopt;
            }
            if (value->type() == dini::ValueType::Int64) {
                return value->asInt64();
            }
            if (value->type() == dini::ValueType::UInt64 &&
                value->asUInt64() <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                return static_cast<std::int64_t>(value->asUInt64());
            }
            return std::nullopt;
        }

        void addSnapshot(SnapshotMap &snapshots, const dini::ItemSnapshot &snapshot) {
            snapshots.insert_or_assign(snapshot.id, snapshot);
        }

        SnapshotMap eventSnapshots(const dini::ChangeSet &changeSet) {
            SnapshotMap snapshots;
            for (const auto &operation : changeSet.operations()) {
                const auto &payload = operation.payload();
                if (const auto *change = std::get_if<dini::ItemInsertedChange>(&payload)) {
                    addSnapshot(snapshots, change->item);
                } else if (const auto *change = std::get_if<dini::ItemRemovedChange>(&payload)) {
                    addSnapshot(snapshots, change->item);
                } else if (const auto *change = std::get_if<dini::CascadeRemovedChange>(&payload)) {
                    addSnapshot(snapshots, change->item);
                } else if (const auto *change = std::get_if<dini::ListInsertedChange>(&payload)) {
                    addSnapshot(snapshots, change->item);
                } else if (const auto *change = std::get_if<dini::ListRemovedChange>(&payload)) {
                    addSnapshot(snapshots, change->item);
                }
            }
            return snapshots;
        }

        class ChangeContext {
        public:
            ChangeContext(dini::DocumentEngine *engine, const dini::ChangeSet &changeSet, dini::ItemId clipId)
                : engine(engine), snapshots(eventSnapshots(changeSet)), clipId(clipId) {
            }

            std::optional<dini::ItemSnapshot> itemSnapshot(dini::ItemId itemId) const {
                if (engine->contains(itemId)) {
                    return engine->read(itemId);
                }
                const auto it = snapshots.find(itemId);
                return it == snapshots.end() ? std::nullopt : std::optional<dini::ItemSnapshot>(it->second);
            }

            bool noteBelongsToClip(dini::ItemId noteId) const {
                const auto snapshot = itemSnapshot(noteId);
                if (!snapshot || !isSnapshotFrom(*snapshot, Schema::noteTable())) {
                    return false;
                }
                const auto *parent = snapshotValue(*snapshot, Schema::noteParent().column());
                return parent && valueReferences(*parent, clipId);
            }

            bool valueReferencesNoteInClip(const dini::Value &value) const {
                const auto noteId = itemIdFromValue(value);
                return noteId && noteBelongsToClip(*noteId);
            }

            bool relationBelongsToClip(const dini::ItemSnapshot &relation) const {
                if (!isSnapshotFrom(relation, Schema::notePhonemeRelationTable())) {
                    return false;
                }
                const auto *parent = snapshotValue(relation, Schema::notePhonemeRelationParent().column());
                return parent && valueReferencesNoteInClip(*parent);
            }

            bool relationBelongsToClip(dini::ItemId relationId) const {
                const auto relation = itemSnapshot(relationId);
                return relation && relationBelongsToClip(*relation);
            }

            bool originalRelationBelongsToClip(dini::ItemId relationId) const {
                const auto relation = itemSnapshot(relationId);
                if (!relation || !relationBelongsToClip(*relation)) {
                    return false;
                }
                return integerValue(snapshotValue(*relation, Schema::notePhonemeRelationRoleColumn())) == originalPhonemeRole;
            }

            bool originalRelationValueBelongsToClip(const dini::Value &value) const {
                const auto relationId = itemIdFromValue(value);
                return relationId && originalRelationBelongsToClip(*relationId);
            }

            bool phonemeAffectsClip(const dini::ItemSnapshot &phoneme) const {
                if (!isSnapshotFrom(phoneme, Schema::phonemeTable())) {
                    return false;
                }
                const auto *parent = snapshotValue(phoneme, Schema::phonemeParent().column());
                return parent && originalRelationValueBelongsToClip(*parent);
            }

            bool snapshotAffectsClip(const dini::ItemSnapshot &snapshot) const {
                if (isSnapshotFrom(snapshot, Schema::tempoTable())) {
                    return true;
                }
                if (isSnapshotFrom(snapshot, Schema::clipTable())) {
                    return snapshot.id == clipId;
                }
                if (isSnapshotFrom(snapshot, Schema::noteTable())) {
                    const auto *parent = snapshotValue(snapshot, Schema::noteParent().column());
                    return (parent && valueReferences(*parent, clipId)) || noteBelongsToClip(snapshot.id);
                }
                if (isSnapshotFrom(snapshot, Schema::notePhonemeRelationTable())) {
                    return relationBelongsToClip(snapshot) || relationBelongsToClip(snapshot.id);
                }
                if (isSnapshotFrom(snapshot, Schema::phonemeTable())) {
                    if (phonemeAffectsClip(snapshot)) {
                        return true;
                    }
                    const auto current = itemSnapshot(snapshot.id);
                    return current && phonemeAffectsClip(*current);
                }
                return false;
            }

            dini::DocumentEngine *engine = nullptr;
            SnapshotMap snapshots;
            dini::ItemId clipId = 0;
        };

        bool snapshotOperationAffectsClip(const dini::ChangeOperation::Payload &payload, const ChangeContext &context) {
            if (const auto *change = std::get_if<dini::ItemInsertedChange>(&payload)) {
                return context.snapshotAffectsClip(change->item);
            }
            if (const auto *change = std::get_if<dini::ItemRemovedChange>(&payload)) {
                return context.snapshotAffectsClip(change->item);
            }
            if (const auto *change = std::get_if<dini::CascadeRemovedChange>(&payload)) {
                return context.snapshotAffectsClip(change->item);
            }
            if (const auto *change = std::get_if<dini::ListInsertedChange>(&payload)) {
                return context.snapshotAffectsClip(change->item);
            }
            if (const auto *change = std::get_if<dini::ListRemovedChange>(&payload)) {
                return context.snapshotAffectsClip(change->item);
            }
            return false;
        }

        bool columnUpdateAffectsClip(const dini::ColumnUpdatedChange &change, const ChangeContext &context) {
            const auto containerId = change.column.containerId();
            if (containerId == Schema::tempoTable().containerId()) {
                return true;
            }
            if (containerId == Schema::clipTable().containerId()) {
                return change.itemId == context.clipId &&
                       (change.column == Schema::clipPositionColumn() || change.column == Schema::clipClipStartColumn());
            }
            if (containerId == Schema::noteTable().containerId()) {
                if (change.column == Schema::noteParent().column()) {
                    return valueReferences(change.oldValue, context.clipId) ||
                           valueReferences(change.newValue, context.clipId) ||
                           context.noteBelongsToClip(change.itemId);
                }
                if (change.column == Schema::notePositionColumn() ||
                    change.column == Schema::noteLengthColumn() ||
                    change.column == Schema::noteLyricColumn()) {
                    return context.noteBelongsToClip(change.itemId);
                }
                return false;
            }
            if (containerId == Schema::notePhonemeRelationTable().containerId()) {
                if (change.column == Schema::notePhonemeRelationParent().column() &&
                    (context.valueReferencesNoteInClip(change.oldValue) || context.valueReferencesNoteInClip(change.newValue))) {
                    return true;
                }
                return context.relationBelongsToClip(change.itemId);
            }
            if (containerId == Schema::phonemeTable().containerId()) {
                if (change.column == Schema::phonemeParent().column() &&
                    (context.originalRelationValueBelongsToClip(change.oldValue) ||
                     context.originalRelationValueBelongsToClip(change.newValue))) {
                    return true;
                }
                const auto snapshot = context.itemSnapshot(change.itemId);
                return snapshot && context.phonemeAffectsClip(*snapshot);
            }
            return false;
        }

        std::vector<PieceValue> divide(const SingingClip *clip,
                                       double paddingBase,
                                       double paddingAdditional,
                                       double pieceGap,
                                       const QStringList &restLyrics) {
            std::vector<Note *> notes;
            notes.reserve(static_cast<std::size_t>(clip->notes()->size()));
            for (auto *note : clip->notes()->asRange()) {
                notes.push_back(note);
            }
            std::ranges::sort(notes, [](const Note *lhs, const Note *rhs) {
                if (lhs->position() != rhs->position()) {
                    return lhs->position() < rhs->position();
                }
                return lhs->handle().d < rhs->handle().d;
            });
            if (notes.empty()) {
                return {};
            }

            const TempoMap tempoMap(clip->model()->tempos());
            const auto clipStartTick = static_cast<double>(clip->start());
            std::vector<NoteData> noteData;
            noteData.reserve(notes.size());
            for (auto *note : notes) {
                std::vector<Phoneme *> phonemes;
                phonemes.reserve(static_cast<std::size_t>(note->originalPhonemes()->size()));
                for (auto *phoneme : note->originalPhonemes()->asRange()) {
                    phonemes.push_back(phoneme);
                }
                std::ranges::sort(phonemes, [](const Phoneme *lhs, const Phoneme *rhs) {
                    if (lhs->start() != rhs->start()) {
                        return lhs->start() < rhs->start();
                    }
                    return lhs->handle().d < rhs->handle().d;
                });

                std::size_t leadingNonOnsetCount = 0;
                while (leadingNonOnsetCount < phonemes.size() && !phonemes[leadingNonOnsetCount]->onset()) {
                    ++leadingNonOnsetCount;
                }

                const bool isRest = restLyrics.contains(note->lyric(), Qt::CaseSensitive);
                const auto startTick = clipStartTick + static_cast<double>(note->position());
                const auto endTick = startTick + static_cast<double>(note->length());
                noteData.push_back({
                    .id = note->handle().d,
                    .position = note->position(),
                    .startMilliseconds = tempoMap.tickToMilliseconds(startTick),
                    .endMilliseconds = tempoMap.tickToMilliseconds(endTick),
                    .leftPadding = isRest ? 0.0 : std::min(paddingBase + static_cast<double>(leadingNonOnsetCount) * paddingAdditional, pieceGap),
                    .rightPadding = isRest ? 0.0 : paddingBase,
                });
            }

            std::vector<PieceValue> result;
            result.reserve(noteData.size());

            const auto appendPiece = [&](std::size_t leftIndex, std::size_t rightIndex) {
                const auto startMilliseconds = noteData[leftIndex].startMilliseconds - noteData[leftIndex].leftPadding;
                const auto endMilliseconds = noteData[rightIndex].endMilliseconds + noteData[rightIndex].rightPadding;
                const auto position = tempoMap.millisecondsToTick(startMilliseconds) - clipStartTick;
                const auto end = tempoMap.millisecondsToTick(endMilliseconds) - clipStartTick;
                result.push_back({position, end - position});
            };

            std::size_t groupStart = 0;
            std::size_t rightmost = 0;
            for (std::size_t index = 1; index < noteData.size(); ++index) {
                const auto gap = noteData[index].startMilliseconds - noteData[rightmost].endMilliseconds;
                const auto requiredGap = std::max(pieceGap, noteData[rightmost].rightPadding + noteData[index].leftPadding);
                const bool sameStart = noteData[index].position == noteData[index - 1].position;
                if (!sameStart && gap >= requiredGap) {
                    appendPiece(groupStart, rightmost);
                    groupStart = index;
                    rightmost = index;
                    continue;
                }

                if (noteData[index].endMilliseconds > noteData[rightmost].endMilliseconds ||
                    (noteData[index].endMilliseconds == noteData[rightmost].endMilliseconds && noteData[index].id < noteData[rightmost].id)) {
                    rightmost = index;
                }
            }
            appendPiece(groupStart, rightmost);
            return result;
        }

    }

    PieceDividerPrivate::PieceDividerPrivate(PieceDivider *q) : q_ptr(q) {
    }

    PieceDividerPrivate::~PieceDividerPrivate() {
        engineSubscription.disconnect();
        QObject::disconnect(destroyedConnection);
    }

    void PieceDividerPrivate::bindSingingClip(SingingClip *clip) {
        Q_Q(PieceDivider);
        if (singingClip == clip) {
            return;
        }

        engineSubscription.disconnect();
        QObject::disconnect(destroyedConnection);
        destroyedConnection = {};
        singingClip = clip;

        if (clip) {
            destroyedConnection = QObject::connect(clip, &QObject::destroyed, q, [this]() {
                Q_Q(PieceDivider);
                engineSubscription.disconnect();
                QObject::disconnect(destroyedConnection);
                destroyedConnection = {};
                singingClip.clear();
                emit q->singingClipChanged(nullptr);
                recalculate();
            });
            auto *engine = clip->model()->document()->engine();
            engineSubscription = engine->subscribe([this](const dini::EngineEvent &event) {
                handleEngineEvent(event);
            });
        }

        emit q->singingClipChanged(clip);
        recalculate();
    }

    void PieceDividerPrivate::handleEngineEvent(const dini::EngineEvent &event) {
        if (event.kind != dini::EventKind::AfterCommit || !singingClip) {
            return;
        }
        auto *clip = singingClip.data();
        auto *engine = clip->model()->document()->engine();
        const auto clipId = static_cast<dini::ItemId>(clip->handle().d);
        if (!engine->contains(clipId)) {
            bindSingingClip(nullptr);
            return;
        }
        if (eventAffectsSingingClip(event)) {
            recalculate();
        }
    }

    bool PieceDividerPrivate::eventAffectsSingingClip(const dini::EngineEvent &event) const {
        auto *clip = singingClip.data();
        if (!clip) {
            return false;
        }
        auto *engine = clip->model()->document()->engine();
        const ChangeContext context(engine, event.changeSet, static_cast<dini::ItemId>(clip->handle().d));
        for (const auto &operation : event.changeSet.operations()) {
            const auto &payload = operation.payload();
            if (const auto *change = std::get_if<dini::ColumnUpdatedChange>(&payload)) {
                if (columnUpdateAffectsClip(*change, context)) {
                    return true;
                }
                continue;
            }
            if (snapshotOperationAffectsClip(payload, context)) {
                return true;
            }
        }
        return false;
    }

    void PieceDividerPrivate::recalculate() {
        Q_Q(PieceDivider);
        const auto values = singingClip
                                ? divide(singingClip.data(), paddingBase, paddingAdditional, pieceGap, restLyrics)
                                : std::vector<PieceValue> {};

        bool changed = false;
        const auto commonSize = std::min(pieces.size(), static_cast<qsizetype>(values.size()));
        for (qsizetype index = 0; index < commonSize; ++index) {
            auto *piece = pieces.at(index);
            const auto &value = values[static_cast<std::size_t>(index)];
            if (piece->position() != value.position) {
                piece->setPosition(value.position);
                changed = true;
            }
            if (piece->length() != value.length) {
                piece->setLength(value.length);
                changed = true;
            }
        }

        while (pieces.size() > static_cast<qsizetype>(values.size())) {
            auto *piece = pieces.takeLast();
            emit q->pieceRemoved(piece);
            delete piece;
            changed = true;
        }
        while (pieces.size() < static_cast<qsizetype>(values.size())) {
            const auto &value = values[static_cast<std::size_t>(pieces.size())];
            auto *piece = new Piece(value.position, value.length, q);
            pieces.append(piece);
            emit q->pieceAdded(piece);
            changed = true;
        }

        if (changed) {
            emit q->pieceChanged();
        }
    }

    PieceDivider::PieceDivider(QObject *parent)
        : QObject(parent), d_ptr(new PieceDividerPrivate(this)) {
    }

    PieceDivider::~PieceDivider() = default;

    SingingClip *PieceDivider::singingClip() const {
        Q_D(const PieceDivider);
        return d->singingClip.data();
    }

    void PieceDivider::setSingingClip(SingingClip *singingClip) {
        Q_D(PieceDivider);
        d->bindSingingClip(singingClip);
    }

    double PieceDivider::paddingBase() const {
        Q_D(const PieceDivider);
        return d->paddingBase;
    }

    void PieceDivider::setPaddingBase(double paddingBase) {
        Q_D(PieceDivider);
        if (d->paddingBase == paddingBase) {
            return;
        }
        d->paddingBase = paddingBase;
        emit paddingBaseChanged(paddingBase);
        d->recalculate();
    }

    double PieceDivider::paddingAdditional() const {
        Q_D(const PieceDivider);
        return d->paddingAdditional;
    }

    void PieceDivider::setPaddingAdditional(double paddingAdditional) {
        Q_D(PieceDivider);
        if (d->paddingAdditional == paddingAdditional) {
            return;
        }
        d->paddingAdditional = paddingAdditional;
        emit paddingAdditionalChanged(paddingAdditional);
        d->recalculate();
    }

    double PieceDivider::pieceGap() const {
        Q_D(const PieceDivider);
        return d->pieceGap;
    }

    void PieceDivider::setPieceGap(double pieceGap) {
        Q_D(PieceDivider);
        if (d->pieceGap == pieceGap) {
            return;
        }
        d->pieceGap = pieceGap;
        emit pieceGapChanged(pieceGap);
        d->recalculate();
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
        d->recalculate();
    }

    QList<Piece *> PieceDivider::piece() const {
        Q_D(const PieceDivider);
        return d->pieces;
    }

}


#include "moc_PieceDivider.cpp"
