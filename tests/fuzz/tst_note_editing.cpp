#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <random>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtGui/QUndoStack>
#include <QtTest/QtTest>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

namespace {

constexpr int initialClipCount = 2;
constexpr int initialNoteCountPerClip = 48;
constexpr int fuzzStepCount = 10000;
constexpr std::uint32_t fuzzSeed = 0x4E4F5445U;
constexpr int maximumClipCount = 4;
constexpr int maximumUndoBatchSize = 64;

constexpr int minimumPosition = 0;
constexpr int maximumPosition = 4096;
constexpr int maximumLength = 512;
constexpr int maximumKeyNumber = 128;
constexpr int keyNumberSmallPoolSize = 5;

constexpr int maximumLayoutEditTries = 32;
constexpr int maximumInsertTries = 64;

enum class ScalarField {
    Lyric,
    Language,
    EditedPronunciation,
    CentShift,
    VibratoAmplitude,
    VibratoEnd,
    VibratoFrequency,
    VibratoOffset,
    VibratoPhase,
    VibratoStart,
};

enum class LayoutField {
    Position,
    Length,
    KeyNumber,
};

const QStringList lyricPool {
    QStringLiteral("ka"),
    QStringLiteral("la"),
    QStringLiteral("ma"),
    QStringLiteral("na"),
    QStringLiteral("a"),
    QStringLiteral(""),
};

const QStringList languagePool {
    QStringLiteral("zh"),
    QStringLiteral("en"),
    QStringLiteral("ja"),
    QStringLiteral(""),
};

const QStringList pronunciationPool {
    QStringLiteral("k a"),
    QStringLiteral("l a"),
    QStringLiteral("m a"),
    QStringLiteral("n a"),
    QStringLiteral(""),
};

struct NoteProps {
    int position = 0;
    int length = 0;
    int keyNumber = 0;
    QString lyric;
    QString language;
    QString editedPronunciation;
    int centShift = 0;
    int vibratoAmplitude = 0;
    double vibratoEnd = 0.0;
    double vibratoFrequency = 1.0;
    int vibratoOffset = 0;
    double vibratoPhase = 0.0;
    double vibratoStart = 0.0;

    bool operator==(const NoteProps &other) const {
        return position == other.position && length == other.length
            && keyNumber == other.keyNumber && lyric == other.lyric
            && language == other.language
            && editedPronunciation == other.editedPronunciation
            && centShift == other.centShift
            && vibratoAmplitude == other.vibratoAmplitude
            && vibratoEnd == other.vibratoEnd
            && vibratoFrequency == other.vibratoFrequency
            && vibratoOffset == other.vibratoOffset
            && vibratoPhase == other.vibratoPhase
            && vibratoStart == other.vibratoStart;
    }
};

struct NoteRecord {
    int noteId = -1;
    NoteProps props;
};

struct SliceResult {
    int clipId = -1;
    QList<int> noteIds;
};

QJsonObject notePropsJson(const NoteProps &props, int noteId,
                          bool overlapped, int previousId, int nextId);

class TestSubject {
public:
    virtual ~TestSubject() = default;

    virtual void insertNote(int clipId, int noteId, const NoteProps &props) = 0;
    virtual void deleteNote(int noteId) = 0;
    virtual void detachNote(int noteId) = 0;
    virtual void attachNote(int noteId, int clipId) = 0;
    virtual void setNoteLayout(int noteId, LayoutField field, int value) = 0;
    virtual void setNoteScalar(int noteId, ScalarField field, const QVariant &value) = 0;
    virtual void moveNote(int noteId, int clipId) = 0;
    virtual void createClip(int clipId) = 0;
    virtual void destroyClip(int clipId) = 0;
    virtual SliceResult sliceProbe(int clipId, int position, int length) = 0;
    virtual int step() const = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual QJsonArray snapshot() const = 0;
};

class OracleTestSubject : public TestSubject {
public:
    OracleTestSubject() {
    }

    void insertNote(int clipId, int noteId, const NoteProps &props) override;
    void deleteNote(int noteId) override;
    void detachNote(int noteId) override;
    void attachNote(int noteId, int clipId) override;
    void setNoteLayout(int noteId, LayoutField field, int value) override;
    void setNoteScalar(int noteId, ScalarField field, const QVariant &value) override;
    void moveNote(int noteId, int clipId) override;
    void createClip(int clipId) override;
    void destroyClip(int clipId) override;

    SliceResult sliceProbe(int clipId, int position, int length) override {
        SliceResult result;
        result.clipId = clipId;
        const auto clipIt = m_clips.find(clipId);
        if (clipIt == m_clips.end()) {
            return result;
        }
        for (const auto &record : visibleNotes(clipId)) {
            if (intervalOverlaps(record.props, position, length)) {
                result.noteIds.append(record.noteId);
            }
        }
        return result;
    }

    int step() const override {
        return m_undoStack.index();
    }

    void undo() override {
        m_undoStack.undo();
    }

    void redo() override {
        m_undoStack.redo();
    }

    QList<int> clipIds() const {
        QList<int> result;
        for (const auto &entry : m_clips) {
            result.append(entry.first);
        }
        std::ranges::sort(result);
        return result;
    }

    QList<int> visibleNoteIds() const {
        QList<int> result;
        for (const auto &entry : m_clips) {
            for (const auto &noteEntry : entry.second) {
                result.append(noteEntry.first);
            }
        }
        std::ranges::sort(result);
        return result;
    }

    QList<int> allNoteIds() const {
        QList<int> result = visibleNoteIds();
        for (const auto &entry : m_detached) {
            result.append(entry.first);
        }
        std::ranges::sort(result);
        return result;
    }

    QList<int> detachedNoteIds() const {
        QList<int> result;
        for (const auto &entry : m_detached) {
            result.append(entry.first);
        }
        std::ranges::sort(result);
        return result;
    }

    int clipOfNote(int noteId) const {
        return locateClip(noteId);
    }

    std::optional<NoteProps> noteProps(int noteId) const {
        for (const auto &entry : m_clips) {
            const auto it = entry.second.find(noteId);
            if (it != entry.second.end()) {
                return it->second;
            }
        }
        const auto detachedIt = m_detached.find(noteId);
        if (detachedIt != m_detached.end()) {
            return detachedIt->second;
        }
        return std::nullopt;
    }

    bool tupleUniqueInClip(int clipId, const NoteProps &candidate) const {
        const auto clipIt = m_clips.find(clipId);
        if (clipIt == m_clips.end()) {
            return true;
        }
        for (const auto &noteEntry : clipIt->second) {
            const auto &props = noteEntry.second;
            if (props.position == candidate.position && props.length == candidate.length
                && props.keyNumber == candidate.keyNumber) {
                return false;
            }
        }
        return true;
    }

    bool tupleUniqueInClipExcept(int clipId, const NoteProps &candidate,
                                 int exceptNoteId) const {
        const auto clipIt = m_clips.find(clipId);
        if (clipIt == m_clips.end()) {
            return true;
        }
        for (const auto &[noteId, props] : clipIt->second) {
            if (noteId == exceptNoteId) {
                continue;
            }
            if (props.position == candidate.position && props.length == candidate.length
                && props.keyNumber == candidate.keyNumber) {
                return false;
            }
        }
        return true;
    }

    QJsonArray snapshot() const override {
        QJsonArray result;
        QList<int> clipIds;
        for (const auto &entry : m_clips) {
            clipIds.append(entry.first);
        }
        std::ranges::sort(clipIds);

        for (const auto clipId : clipIds) {
            const auto clipIt = m_clips.find(clipId);
            Q_ASSERT(clipIt != m_clips.end());
            const auto &notes = clipIt->second;

            const auto sorted = notesInOrder(clipId);

            QJsonObject clipObject;
            clipObject.insert(QStringLiteral("clip_id"), clipId);
            clipObject.insert(QStringLiteral("size"), static_cast<int>(sorted.size()));

            if (sorted.empty()) {
                clipObject.insert(QStringLiteral("first_note"), -1);
                clipObject.insert(QStringLiteral("last_note"), -1);
            } else {
                clipObject.insert(QStringLiteral("first_note"), sorted.front().noteId);
                clipObject.insert(QStringLiteral("last_note"), sorted.back().noteId);
            }

            QJsonArray forwardArray;
            for (const auto &record : sorted) {
                forwardArray.append(recordToJson(record, clipId));
            }
            clipObject.insert(QStringLiteral("notes"), forwardArray);

            QJsonArray backwardArray;
            for (auto it = sorted.rbegin(); it != sorted.rend(); ++it) {
                backwardArray.append(it->noteId);
            }
            clipObject.insert(QStringLiteral("backward"), backwardArray);

            result.append(clipObject);
        }

        QList<int> detachedIds;
        for (const auto &entry : m_detached) {
            detachedIds.append(entry.first);
        }
        std::ranges::sort(detachedIds);

        QJsonArray detachedArray;
        for (const auto noteId : detachedIds) {
            const auto it = m_detached.find(noteId);
            Q_ASSERT(it != m_detached.end());
            detachedArray.append(notePropsJson(it->second, noteId, false, -1, -1));
        }

        QJsonObject root;
        root.insert(QStringLiteral("clips"), result);
        root.insert(QStringLiteral("detached"), detachedArray);
        return QJsonArray { root };
    }

private:
    friend class OracleCommand;
    friend class OracleInsertNoteCommand;
    friend class OracleDeleteNoteCommand;
    friend class OracleDetachNoteCommand;
    friend class OracleAttachNoteCommand;
    friend class OracleSetNoteLayoutCommand;
    friend class OracleSetNoteScalarCommand;
    friend class OracleMoveNoteCommand;
    friend class OracleCreateClipCommand;
    friend class OracleDestroyClipCommand;

    std::unordered_map<int, std::unordered_map<int, NoteProps>> m_clips;
    std::unordered_map<int, NoteProps> m_detached;
    QUndoStack m_undoStack;

    int locateClip(int noteId) const {
        for (const auto &[clipId, clipNotes] : m_clips) {
            if (clipNotes.find(noteId) != clipNotes.end()) {
                return clipId;
            }
        }
        return -1;
    }

    NoteProps &locateProps(int noteId) {
        for (auto &[clipId, clipNotes] : m_clips) {
            Q_UNUSED(clipId)
            auto it = clipNotes.find(noteId);
            if (it != clipNotes.end()) {
                return it->second;
            }
        }
        auto detachedIt = m_detached.find(noteId);
        Q_ASSERT(detachedIt != m_detached.end());
        return detachedIt->second;
    }

    static bool intervalOverlaps(const NoteProps &a, const NoteProps &b) {
        return a.position < b.position + b.length && b.position < a.position + a.length;
    }

    // slice intersects half-open intervals [position, position + length), so a
    // note with an empty interval never intersects (unlike the overlapped
    // predicate, which counts strict point containment).
    static bool intervalOverlaps(const NoteProps &a, int position, int length) {
        return length > 0 && a.length > 0 && a.position < position + length
            && position < a.position + a.length;
    }

    bool hasOverlapInClip(const NoteProps &props, int clipId, int exceptNoteId) const {
        for (const auto &[otherId, otherProps] : m_clips.at(clipId)) {
            if (otherId == exceptNoteId) {
                continue;
            }
            if (intervalOverlaps(props, otherProps)) {
                return true;
            }
        }
        return false;
    }

    std::vector<NoteRecord> notesInOrder(int clipId) const {
        std::vector<NoteRecord> result;
        const auto &notes = m_clips.at(clipId);
        result.reserve(notes.size());
        for (const auto &[noteId, props] : notes) {
            result.push_back({noteId, props});
        }
        std::ranges::sort(result, [](const NoteRecord &a, const NoteRecord &b) {
            return std::tie(a.props.position, a.props.length, a.props.keyNumber)
                < std::tie(b.props.position, b.props.length, b.props.keyNumber);
        });
        return result;
    }

    std::vector<NoteRecord> visibleNotes(int clipId) const {
        return notesInOrder(clipId);
    }

    QJsonObject recordToJson(const NoteRecord &record, int clipId) const {
        const auto &sorted = notesInOrder(clipId);
        int index = -1;
        for (std::size_t i = 0; i < sorted.size(); ++i) {
            if (sorted[i].noteId == record.noteId) {
                index = static_cast<int>(i);
                break;
            }
        }
        Q_ASSERT(index >= 0);

        int previousId = -1;
        if (index > 0) {
            previousId = sorted[index - 1].noteId;
        }
        int nextId = -1;
        if (index + 1 < static_cast<int>(sorted.size())) {
            nextId = sorted[index + 1].noteId;
        }

        bool overlapped = hasOverlapInClip(record.props, clipId, record.noteId);
        return notePropsJson(record.props, record.noteId, overlapped, previousId, nextId);
    }
};

class OracleCommand : public QUndoCommand {
protected:
    explicit OracleCommand(OracleTestSubject &owner) : m_owner(owner) {
    }

    OracleTestSubject &m_owner;
};

class OracleInsertNoteCommand : public OracleCommand {
public:
    OracleInsertNoteCommand(OracleTestSubject &owner, int clipId, int noteId, const NoteProps &props)
        : OracleCommand(owner), m_clipId(clipId), m_noteId(noteId), m_props(props) {
    }

    void redo() override {
        auto &clipNotes = m_owner.m_clips[m_clipId];
        auto [it, inserted] = clipNotes.emplace(m_noteId, m_props);
        Q_ASSERT(inserted);
        Q_UNUSED(it)
    }

    void undo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        auto erased = clipIt->second.erase(m_noteId);
        Q_ASSERT(erased == 1);
        Q_UNUSED(erased)
    }

private:
    int m_clipId;
    int m_noteId;
    NoteProps m_props;
};

class OracleDeleteNoteCommand : public OracleCommand {
public:
    OracleDeleteNoteCommand(OracleTestSubject &owner, int noteId)
        : OracleCommand(owner), m_noteId(noteId) {
        m_clipId = owner.locateClip(noteId);
        Q_ASSERT(m_clipId >= 0);
        m_props = owner.m_clips.at(m_clipId).at(noteId);
    }

    void redo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        auto erased = clipIt->second.erase(m_noteId);
        Q_ASSERT(erased == 1);
        Q_UNUSED(erased)
    }

    void undo() override {
        m_owner.m_clips[m_clipId].emplace(m_noteId, m_props);
    }

private:
    int m_clipId;
    int m_noteId;
    NoteProps m_props;
};

class OracleDetachNoteCommand : public OracleCommand {
public:
    OracleDetachNoteCommand(OracleTestSubject &owner, int noteId)
        : OracleCommand(owner), m_noteId(noteId) {
        m_clipId = owner.locateClip(noteId);
        Q_ASSERT(m_clipId >= 0);
        m_props = owner.m_clips.at(m_clipId).at(noteId);
    }

    void redo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        auto erased = clipIt->second.erase(m_noteId);
        Q_ASSERT(erased == 1);
        Q_UNUSED(erased)
        m_owner.m_detached.emplace(m_noteId, m_props);
    }

    void undo() override {
        auto erased = m_owner.m_detached.erase(m_noteId);
        Q_ASSERT(erased == 1);
        Q_UNUSED(erased)
        m_owner.m_clips[m_clipId].emplace(m_noteId, m_props);
    }

private:
    int m_clipId;
    int m_noteId;
    NoteProps m_props;
};

class OracleAttachNoteCommand : public OracleCommand {
public:
    OracleAttachNoteCommand(OracleTestSubject &owner, int noteId, int clipId)
        : OracleCommand(owner), m_noteId(noteId), m_clipId(clipId) {
    }

    void redo() override {
        auto detachedIt = m_owner.m_detached.find(m_noteId);
        Q_ASSERT(detachedIt != m_owner.m_detached.end());
        auto props = detachedIt->second;
        m_owner.m_detached.erase(detachedIt);
        m_owner.m_clips[m_clipId].emplace(m_noteId, props);
    }

    void undo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        auto clipNoteIt = clipIt->second.find(m_noteId);
        Q_ASSERT(clipNoteIt != clipIt->second.end());
        auto props = clipNoteIt->second;
        clipIt->second.erase(clipNoteIt);
        m_owner.m_detached.emplace(m_noteId, props);
    }

private:
    int m_noteId;
    int m_clipId;
    NoteProps m_props;
};

class OracleSetNoteLayoutCommand : public OracleCommand {
public:
    OracleSetNoteLayoutCommand(OracleTestSubject &owner, int noteId, LayoutField field, int value)
        : OracleCommand(owner), m_noteId(noteId), m_field(field), m_value(value) {
        const auto &props = owner.locateProps(noteId);
        switch (field) {
        case LayoutField::Position:
            m_oldValue = props.position;
            break;
        case LayoutField::Length:
            m_oldValue = props.length;
            break;
        case LayoutField::KeyNumber:
            m_oldValue = props.keyNumber;
            break;
        }
    }

    void redo() override {
        auto &props = m_owner.locateProps(m_noteId);
        apply(props, m_field, m_value);
    }

    void undo() override {
        auto &props = m_owner.locateProps(m_noteId);
        apply(props, m_field, m_oldValue);
    }

private:
    static void apply(NoteProps &props, LayoutField field, int value) {
        switch (field) {
        case LayoutField::Position:
            props.position = value;
            break;
        case LayoutField::Length:
            props.length = value;
            break;
        case LayoutField::KeyNumber:
            props.keyNumber = value;
            break;
        }
    }

    int m_noteId;
    LayoutField m_field;
    int m_value;
    int m_oldValue;
};

class OracleSetNoteScalarCommand : public OracleCommand {
public:
    OracleSetNoteScalarCommand(OracleTestSubject &owner, int noteId, ScalarField field, const QVariant &value)
        : OracleCommand(owner), m_noteId(noteId), m_field(field), m_value(value) {
        const auto &props = owner.locateProps(noteId);
        switch (field) {
        case ScalarField::Lyric:
            m_oldValue = QVariant(props.lyric);
            break;
        case ScalarField::Language:
            m_oldValue = QVariant(props.language);
            break;
        case ScalarField::EditedPronunciation:
            m_oldValue = QVariant(props.editedPronunciation);
            break;
        case ScalarField::CentShift:
            m_oldValue = QVariant(props.centShift);
            break;
        case ScalarField::VibratoAmplitude:
            m_oldValue = QVariant(props.vibratoAmplitude);
            break;
        case ScalarField::VibratoEnd:
            m_oldValue = QVariant(props.vibratoEnd);
            break;
        case ScalarField::VibratoFrequency:
            m_oldValue = QVariant(props.vibratoFrequency);
            break;
        case ScalarField::VibratoOffset:
            m_oldValue = QVariant(props.vibratoOffset);
            break;
        case ScalarField::VibratoPhase:
            m_oldValue = QVariant(props.vibratoPhase);
            break;
        case ScalarField::VibratoStart:
            m_oldValue = QVariant(props.vibratoStart);
            break;
        }
    }

    void redo() override {
        apply(m_value);
    }

    void undo() override {
        apply(m_oldValue);
    }

private:
    void apply(const QVariant &value) {
        auto &props = m_owner.locateProps(m_noteId);
        switch (m_field) {
        case ScalarField::Lyric:
            props.lyric = value.toString();
            break;
        case ScalarField::Language:
            props.language = value.toString();
            break;
        case ScalarField::EditedPronunciation:
            props.editedPronunciation = value.toString();
            break;
        case ScalarField::CentShift:
            props.centShift = value.toInt();
            break;
        case ScalarField::VibratoAmplitude:
            props.vibratoAmplitude = value.toInt();
            break;
        case ScalarField::VibratoEnd:
            props.vibratoEnd = value.toDouble();
            break;
        case ScalarField::VibratoFrequency:
            props.vibratoFrequency = value.toDouble();
            break;
        case ScalarField::VibratoOffset:
            props.vibratoOffset = value.toInt();
            break;
        case ScalarField::VibratoPhase:
            props.vibratoPhase = value.toDouble();
            break;
        case ScalarField::VibratoStart:
            props.vibratoStart = value.toDouble();
            break;
        }
    }

    int m_noteId;
    ScalarField m_field;
    QVariant m_value;
    QVariant m_oldValue;
};

class OracleMoveNoteCommand : public OracleCommand {
public:
    OracleMoveNoteCommand(OracleTestSubject &owner, int noteId, int clipId)
        : OracleCommand(owner), m_noteId(noteId), m_clipId(clipId) {
        m_fromClipId = owner.locateClip(noteId);
        Q_ASSERT(m_fromClipId >= 0 && m_fromClipId != m_clipId);
    }

    void redo() override {
        auto fromClipIt = m_owner.m_clips.find(m_fromClipId);
        Q_ASSERT(fromClipIt != m_owner.m_clips.end());
        auto it = fromClipIt->second.find(m_noteId);
        Q_ASSERT(it != fromClipIt->second.end());
        auto propsCopy = it->second;
        fromClipIt->second.erase(it);
        m_owner.m_clips[m_clipId].emplace(m_noteId, propsCopy);
    }

    void undo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        auto noteIt = clipIt->second.find(m_noteId);
        Q_ASSERT(noteIt != clipIt->second.end());
        auto props = noteIt->second;
        clipIt->second.erase(noteIt);
        m_owner.m_clips[m_fromClipId].emplace(m_noteId, props);
    }

private:
    int m_fromClipId;
    int m_clipId;
    int m_noteId;
};

class OracleCreateClipCommand : public OracleCommand {
public:
    OracleCreateClipCommand(OracleTestSubject &owner, int clipId)
        : OracleCommand(owner), m_clipId(clipId) {
        Q_ASSERT(owner.m_clips.find(m_clipId) == owner.m_clips.end());
    }

    void redo() override {
        auto [it, inserted] = m_owner.m_clips.emplace(m_clipId, std::unordered_map<int, NoteProps> {});
        Q_ASSERT(inserted);
        Q_UNUSED(it)
    }

    void undo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        Q_ASSERT(clipIt->second.empty());
        m_owner.m_clips.erase(clipIt);
    }

private:
    int m_clipId;
};

class OracleDestroyClipCommand : public OracleCommand {
public:
    OracleDestroyClipCommand(OracleTestSubject &owner, int clipId)
        : OracleCommand(owner), m_clipId(clipId) {
        m_notes = owner.m_clips.at(clipId);
    }

    void redo() override {
        auto clipIt = m_owner.m_clips.find(m_clipId);
        Q_ASSERT(clipIt != m_owner.m_clips.end());
        m_owner.m_clips.erase(clipIt);
    }

    void undo() override {
        auto &clipNotes = m_owner.m_clips[m_clipId];
        for (auto &[noteId, props] : m_notes) {
            Q_UNUSED(noteId)
            clipNotes.emplace(noteId, props);
        }
    }

private:
    int m_clipId;
    std::unordered_map<int, NoteProps> m_notes;
};

void OracleTestSubject::insertNote(int clipId, int noteId, const NoteProps &props) {
    m_undoStack.push(new OracleInsertNoteCommand(*this, clipId, noteId, props));
}

void OracleTestSubject::deleteNote(int noteId) {
    m_undoStack.push(new OracleDeleteNoteCommand(*this, noteId));
}

void OracleTestSubject::detachNote(int noteId) {
    m_undoStack.push(new OracleDetachNoteCommand(*this, noteId));
}

void OracleTestSubject::attachNote(int noteId, int clipId) {
    m_undoStack.push(new OracleAttachNoteCommand(*this, noteId, clipId));
}

void OracleTestSubject::setNoteLayout(int noteId, LayoutField field, int value) {
    m_undoStack.push(new OracleSetNoteLayoutCommand(*this, noteId, field, value));
}

void OracleTestSubject::setNoteScalar(int noteId, ScalarField field, const QVariant &value) {
    m_undoStack.push(new OracleSetNoteScalarCommand(*this, noteId, field, value));
}

void OracleTestSubject::moveNote(int noteId, int clipId) {
    m_undoStack.push(new OracleMoveNoteCommand(*this, noteId, clipId));
}

void OracleTestSubject::createClip(int clipId) {
    m_undoStack.push(new OracleCreateClipCommand(*this, clipId));
}

void OracleTestSubject::destroyClip(int clipId) {
    m_undoStack.push(new OracleDestroyClipCommand(*this, clipId));
}

class DspxTestSubject : public TestSubject {
public:
    DspxTestSubject() {
        withTransaction([&] {
            m_track = m_model.createTrack();
            m_model.tracks()->insertItem(0, m_track);
        });
        m_document.engine()->clearUndoHistory();
    }

    void insertNote(int clipId, int noteId, const NoteProps &props) override {
        withTransaction([&] {
            auto *clip = resolveClip(clipId);
            if (!clip) {
                qCritical() << "note: clip not found for insert" << clipId;
                return;
            }
            auto *note = m_model.createNote();
            applyProps(note, props);
            if (!clip->notes()->insertItem(note)) {
                qCritical() << "note: failed to insert note" << clipId << noteId << props.position
                            << props.length << props.keyNumber;
                return;
            }
            m_noteHandles[noteId] = note->handle();
        });
    }

    void deleteNote(int noteId) override {
        withTransaction([&] {
            auto *note = resolveNote(noteId);
            if (!note) {
                qCritical() << "note: note not found for delete" << noteId;
                return;
            }
            if (!note->noteSequence()->removeItem(note)) {
                qCritical() << "note: failed to remove note" << noteId;
                return;
            }
            m_model.destroyItem(note);
        });
    }

    void detachNote(int noteId) override {
        withTransaction([&] {
            auto *note = resolveNote(noteId);
            if (!note) {
                qCritical() << "note: note not found for detach" << noteId;
                return;
            }
            if (!note->noteSequence()->removeItem(note)) {
                qCritical() << "note: failed to detach note" << noteId;
            }
        });
    }

    void attachNote(int noteId, int clipId) override {
        withTransaction([&] {
            auto *clip = resolveClip(clipId);
            auto *note = resolveNote(noteId);
            if (!clip || !note) {
                qCritical() << "note: clip or note not found for attach" << clipId << noteId;
                return;
            }
            if (!clip->notes()->insertItem(note)) {
                qCritical() << "note: failed to attach note" << noteId << clipId;
            }
        });
    }

    void setNoteLayout(int noteId, LayoutField field, int value) override {
        withTransaction([&] {
            auto *note = resolveNote(noteId);
            if (!note) {
                qCritical() << "note: note not found for layout edit" << noteId;
                return;
            }
            switch (field) {
            case LayoutField::Position:
                note->setPosition(value);
                break;
            case LayoutField::Length:
                note->setLength(value);
                break;
            case LayoutField::KeyNumber:
                note->setKeyNumber(value);
                break;
            }
        });
    }

    void setNoteScalar(int noteId, ScalarField field, const QVariant &value) override {
        withTransaction([&] {
            auto *note = resolveNote(noteId);
            if (!note) {
                qCritical() << "note: note not found for scalar edit" << noteId;
                return;
            }
            switch (field) {
            case ScalarField::Lyric:
                note->setLyric(value.toString());
                break;
            case ScalarField::Language:
                note->setLanguage(value.toString());
                break;
            case ScalarField::EditedPronunciation:
                note->setEditedPronunciation(value.toString());
                break;
            case ScalarField::CentShift:
                note->setCentShift(value.toInt());
                break;
            case ScalarField::VibratoAmplitude:
                note->setVibratoAmplitude(value.toInt());
                break;
            case ScalarField::VibratoEnd:
                note->setVibratoEnd(value.toDouble());
                break;
            case ScalarField::VibratoFrequency:
                note->setVibratoFrequency(value.toDouble());
                break;
            case ScalarField::VibratoOffset:
                note->setVibratoOffset(value.toInt());
                break;
            case ScalarField::VibratoPhase:
                note->setVibratoPhase(value.toDouble());
                break;
            case ScalarField::VibratoStart:
                note->setVibratoStart(value.toDouble());
                break;
            }
        });
    }

    void moveNote(int noteId, int clipId) override {
        withTransaction([&] {
            auto *note = resolveNote(noteId);
            auto *targetClip = resolveClip(clipId);
            if (!note || !targetClip) {
                qCritical() << "note: note or clip not found for move" << noteId << clipId;
                return;
            }
            auto *sourceSequence = note->noteSequence();
            if (!sourceSequence) {
                qCritical() << "note: moved note has no sequence" << noteId;
                return;
            }
            if (!sourceSequence->moveItem(note, targetClip->notes())) {
                qCritical() << "note: failed to move note" << noteId << clipId;
            }
        });
    }

    void createClip(int clipId) override {
        withTransaction([&] {
            auto *clip = m_model.createSingingClip();
            clip->setPosition(clipId * 1000000);
            clip->setClipStart(0);
            clip->setLength(1000000);
            clip->setClipLength(1000000);
            if (!m_track->clips()->insertItem(clip)) {
                qCritical() << "note: failed to insert clip" << clipId;
                return;
            }
            m_clipHandles[clipId] = clip->handle();
        });
    }

    void destroyClip(int clipId) override {
        withTransaction([&] {
            auto *clip = resolveClip(clipId);
            if (!clip) {
                qCritical() << "note: clip not found for destroy" << clipId;
                return;
            }
            if (!m_track->clips()->removeItem(clip)) {
                qCritical() << "note: failed to remove clip" << clipId;
                return;
            }
            m_model.destroyItem(clip);
        });
    }

    SliceResult sliceProbe(int clipId, int position, int length) override {
        SliceResult result;
        result.clipId = clipId;
        auto *clip = resolveClip(clipId);
        if (!clip) {
            return result;
        }
        const auto notes = clip->notes()->slice(position, length);
        result.noteIds.reserve(notes.size());
        for (auto *note : notes) {
            result.noteIds.append(noteIdOf(note->handle()));
        }
        return result;
    }

    int step() const override {
        return static_cast<int>(m_document.engine()->undoHistory().size());
    }

    void undo() override {
        m_document.engine()->undo();
    }

    void redo() override {
        m_document.engine()->redo();
    }

    QJsonArray snapshot() const override {
        QJsonArray clipArray;
        QList<int> clipIds;
        for (const auto &entry : m_clipHandles) {
            clipIds.append(entry.first);
        }
        std::ranges::sort(clipIds);

        for (const auto clipId : clipIds) {
            auto *clip = resolveClip(clipId);
            if (!clip) {
                continue;
            }

            std::vector<dspx::Note *> forwardNotes;
            for (auto *note : clip->notes()->asRange()) {
                forwardNotes.push_back(note);
            }

            QJsonArray forwardArray;
            for (std::size_t i = 0; i < forwardNotes.size(); ++i) {
                auto *note = forwardNotes[i];
                const auto props = notePropsOf(note);
                const auto noteId = noteIdOf(note->handle());
                Q_ASSERT(noteId >= 0);

                int previousId = -1;
                if (i > 0) {
                    previousId = noteIdOf(forwardNotes[i - 1]->handle());
                }
                int nextId = -1;
                if (i + 1 < forwardNotes.size()) {
                    nextId = noteIdOf(forwardNotes[i + 1]->handle());
                }

                bool overlapped = false;
                for (std::size_t j = 0; j < forwardNotes.size(); ++j) {
                    if (j == i) {
                        continue;
                    }
                    if (intervalOverlaps(props, notePropsOf(forwardNotes[j]))) {
                        overlapped = true;
                        break;
                    }
                }

                forwardArray.append(notePropsJson(props, noteId, overlapped, previousId, nextId));
            }

            QJsonArray backwardArray;
            for (auto *note = clip->notes()->lastItem(); note; note = note->previousItem()) {
                backwardArray.append(noteIdOf(note->handle()));
            }

            QJsonObject clipObject;
            clipObject.insert(QStringLiteral("clip_id"), clipId);
            clipObject.insert(QStringLiteral("size"), static_cast<int>(forwardNotes.size()));
            clipObject.insert(QStringLiteral("first_note"),
                              forwardNotes.empty() ? -1 : noteIdOf(forwardNotes.front()->handle()));
            clipObject.insert(QStringLiteral("last_note"),
                              forwardNotes.empty() ? -1 : noteIdOf(forwardNotes.back()->handle()));
            clipObject.insert(QStringLiteral("notes"), forwardArray);
            clipObject.insert(QStringLiteral("backward"), backwardArray);
            clipArray.append(clipObject);
        }

        QList<int> detachedIds;
        for (const auto &entry : m_noteHandles) {
            const auto noteId = entry.first;
            auto *note = resolveNote(noteId);
            if (!note) {
                continue;
            }
            bool contained = false;
            for (const auto clipId : clipIds) {
                auto *clip = resolveClip(clipId);
                if (clip && clip->notes()->contains(note)) {
                    contained = true;
                    break;
                }
            }
            if (!contained) {
                detachedIds.append(noteId);
            }
        }
        std::ranges::sort(detachedIds);

        QJsonArray detachedArray;
        for (const auto noteId : detachedIds) {
            auto *note = resolveNote(noteId);
            Q_ASSERT(note);
            detachedArray.append(notePropsJson(notePropsOf(note), noteId, false, -1, -1));
        }

        QJsonArray result;
        QJsonObject root;
        root.insert(QStringLiteral("clips"), clipArray);
        root.insert(QStringLiteral("detached"), detachedArray);
        result.append(root);
        return result;
    }

private:
    dspx::Document m_document;
    mutable dspx::Model m_model {&m_document};
    dspx::Track *m_track = nullptr;
    std::unordered_map<int, dspx::Handle> m_clipHandles;
    std::unordered_map<int, dspx::Handle> m_noteHandles;

    template <typename Func>
    void withTransaction(Func &&func) {
        auto transaction = m_document.engine()->beginTransaction();
        m_document.setTransaction(&transaction);
        try {
            std::invoke(std::forward<Func>(func));
            transaction.commit();
            m_document.setTransaction(nullptr);
        } catch (...) {
            m_document.setTransaction(nullptr);
            throw;
        }
    }

    dspx::SingingClip *resolveClip(int clipId) const {
        const auto it = m_clipHandles.find(clipId);
        if (it == m_clipHandles.end()) {
            return nullptr;
        }
        return m_model.find<dspx::SingingClip>(it->second);
    }

    dspx::Note *resolveNote(int noteId) const {
        const auto it = m_noteHandles.find(noteId);
        if (it == m_noteHandles.end()) {
            return nullptr;
        }
        return m_model.find<dspx::Note>(it->second);
    }

    int noteIdOf(const dspx::Handle &handle) const {
        for (const auto &[noteId, noteHandle] : m_noteHandles) {
            if (noteHandle == handle) {
                return noteId;
            }
        }
        return -1;
    }

    static void applyProps(dspx::Note *note, const NoteProps &props) {
        note->setPosition(props.position);
        note->setLength(props.length);
        note->setKeyNumber(props.keyNumber);
        note->setLyric(props.lyric);
        note->setLanguage(props.language);
        note->setEditedPronunciation(props.editedPronunciation);
        note->setCentShift(props.centShift);
        note->setVibratoAmplitude(props.vibratoAmplitude);
        note->setVibratoEnd(props.vibratoEnd);
        note->setVibratoFrequency(props.vibratoFrequency);
        note->setVibratoOffset(props.vibratoOffset);
        note->setVibratoPhase(props.vibratoPhase);
        note->setVibratoStart(props.vibratoStart);
    }

    static NoteProps notePropsOf(dspx::Note *note) {
        NoteProps props;
        props.position = note->position();
        props.length = note->length();
        props.keyNumber = note->keyNumber();
        props.lyric = note->lyric();
        props.language = note->language();
        props.editedPronunciation = note->editedPronunciation();
        props.centShift = note->centShift();
        props.vibratoAmplitude = note->vibratoAmplitude();
        props.vibratoEnd = note->vibratoEnd();
        props.vibratoFrequency = note->vibratoFrequency();
        props.vibratoOffset = note->vibratoOffset();
        props.vibratoPhase = note->vibratoPhase();
        props.vibratoStart = note->vibratoStart();
        return props;
    }

    static bool intervalOverlaps(const NoteProps &a, const NoteProps &b) {
        return a.position < b.position + b.length && b.position < a.position + a.length;
    }
};

QJsonObject notePropsJson(const NoteProps &props, int noteId,
                          bool overlapped, int previousId, int nextId) {
    return {
        {QStringLiteral("note_id"), noteId},
        {QStringLiteral("position"), props.position},
        {QStringLiteral("length"), props.length},
        {QStringLiteral("key_number"), props.keyNumber},
        {QStringLiteral("lyric"), props.lyric},
        {QStringLiteral("language"), props.language},
        // original_pronunciation is stored only by the ORM outside the undo
        // history, so it is not compared.
        {QStringLiteral("edited_pronunciation"), props.editedPronunciation},
        {QStringLiteral("cent_shift"), props.centShift},
        {QStringLiteral("vibrato_amplitude"), props.vibratoAmplitude},
        {QStringLiteral("vibrato_end"), props.vibratoEnd},
        {QStringLiteral("vibrato_frequency"), props.vibratoFrequency},
        {QStringLiteral("vibrato_offset"), props.vibratoOffset},
        {QStringLiteral("vibrato_phase"), props.vibratoPhase},
        {QStringLiteral("vibrato_start"), props.vibratoStart},
        {QStringLiteral("overlapped"), overlapped},
        {QStringLiteral("previous"), previousId},
        {QStringLiteral("next"), nextId},
    };
}

int randomInteger(std::mt19937 &rng, int minimum, int maximum) {
    return std::uniform_int_distribution<int>(minimum, maximum)(rng);
}

double randomDouble(std::mt19937 &rng, double minimum, double maximum) {
    return std::uniform_real_distribution<double>(minimum, maximum)(rng);
}

QString pickString(std::mt19937 &rng, const QStringList &pool) {
    return pool.at(randomInteger(rng, 0, static_cast<int>(pool.size()) - 1));
}

int randomKeyNumber(std::mt19937 &rng) {
    if (randomInteger(rng, 0, 99) < 30) {
        return randomInteger(rng, 60, 60 + keyNumberSmallPoolSize - 1);
    }
    return randomInteger(rng, 0, maximumKeyNumber - 1);
}

int randomLength(std::mt19937 &rng) {
    if (randomInteger(rng, 0, 99) < 15) {
        return 0;
    }
    return randomInteger(rng, 1, maximumLength);
}

NoteProps randomNoteProps(std::mt19937 &rng) {
    NoteProps props;
    props.position = randomInteger(rng, minimumPosition, maximumPosition);
    props.length = randomLength(rng);
    props.keyNumber = randomKeyNumber(rng);
    props.lyric = pickString(rng, lyricPool);
    props.language = pickString(rng, languagePool);
    props.editedPronunciation = pickString(rng, pronunciationPool);
    props.centShift = randomInteger(rng, -50, 50);
    props.vibratoAmplitude = randomInteger(rng, 0, 200);
    props.vibratoEnd = randomDouble(rng, 0.0, 1.0);
    props.vibratoFrequency = randomDouble(rng, 0.0, 10.0);
    props.vibratoOffset = randomInteger(rng, -480, 480);
    props.vibratoPhase = randomDouble(rng, 0.0, 1.0);
    props.vibratoStart = randomDouble(rng, 0.0, 1.0);
    return props;
}

QVariant randomScalarValue(ScalarField field, std::mt19937 &rng) {
    switch (field) {
    case ScalarField::Lyric:
        return QVariant(pickString(rng, lyricPool));
    case ScalarField::Language:
        return QVariant(pickString(rng, languagePool));
    case ScalarField::EditedPronunciation:
        return QVariant(pickString(rng, pronunciationPool));
    case ScalarField::CentShift:
        return QVariant(randomInteger(rng, -50, 50));
    case ScalarField::VibratoAmplitude:
        return QVariant(randomInteger(rng, 0, 200));
    case ScalarField::VibratoEnd:
        return QVariant(randomDouble(rng, 0.0, 1.0));
    case ScalarField::VibratoFrequency:
        return QVariant(randomDouble(rng, 0.0, 10.0));
    case ScalarField::VibratoOffset:
        return QVariant(randomInteger(rng, -480, 480));
    case ScalarField::VibratoPhase:
        return QVariant(randomDouble(rng, 0.0, 1.0));
    case ScalarField::VibratoStart:
        return QVariant(randomDouble(rng, 0.0, 1.0));
    }
    return {};
}

const char *scalarFieldName(ScalarField field) {
    switch (field) {
    case ScalarField::Lyric:
        return "lyric";
    case ScalarField::Language:
        return "language";
    case ScalarField::EditedPronunciation:
        return "edited_pronunciation";
    case ScalarField::CentShift:
        return "cent_shift";
    case ScalarField::VibratoAmplitude:
        return "vibrato_amplitude";
    case ScalarField::VibratoEnd:
        return "vibrato_end";
    case ScalarField::VibratoFrequency:
        return "vibrato_frequency";
    case ScalarField::VibratoOffset:
        return "vibrato_offset";
    case ScalarField::VibratoPhase:
        return "vibrato_phase";
    case ScalarField::VibratoStart:
        return "vibrato_start";
    }
    return "unknown";
}

struct ComparisonTarget {
    QString name;
    TestSubject *object = nullptr;
};

QString compareSnapshots(const QJsonArray &oracle, const QJsonArray &actual,
                         const QString &operationType, int oracleStep, int actualStep) {
    const auto operationFailure = [&](const QString &message) {
        return QStringLiteral("Failure after operation %1: %2").arg(operationType, message);
    };

    const QJsonObject oracleRoot = oracle.first().toObject();
    const QJsonObject actualRoot = actual.first().toObject();

    const auto compareArray = [&](const char *key, const QJsonArray &a, const QJsonArray &b) {
        const auto commonSize = qMin(a.size(), b.size());
        for (qsizetype i = 0; i < commonSize; ++i) {
            if (a[i] != b[i]) {
                return operationFailure(QStringLiteral("differential mismatch for %1 at index %2 "
                                                       "(oracle size %3, actual size %4)")
                                           .arg(QLatin1String(key))
                                           .arg(i)
                                           .arg(a.size())
                                           .arg(b.size()));
            }
        }
        if (a.size() != b.size()) {
            return operationFailure(QStringLiteral("size mismatch for %1 (oracle %2, actual %3)")
                                       .arg(QLatin1String(key))
                                       .arg(a.size())
                                       .arg(b.size()));
        }
        return QString {};
    };

    auto error = compareArray("clips",
                              oracleRoot.value(QStringLiteral("clips")).toArray(),
                              actualRoot.value(QStringLiteral("clips")).toArray());
    if (!error.isEmpty()) {
        return error;
    }
    error = compareArray("detached",
                         oracleRoot.value(QStringLiteral("detached")).toArray(),
                         actualRoot.value(QStringLiteral("detached")).toArray());
    if (!error.isEmpty()) {
        return error;
    }
    if (oracleStep != actualStep) {
        return operationFailure(QStringLiteral("undo step mismatch (oracle %1, actual %2)")
                                   .arg(oracleStep)
                                   .arg(actualStep));
    }
    return {};
}

QString writeLogAndCompare(QFile &logFile,
                           const QJsonObject &operation,
                           const std::vector<ComparisonTarget> &targets) {
    const auto operationType = operation.value(QStringLiteral("type")).toString();
    const auto operationFailure = [&](const QString &message) {
        return QStringLiteral("Failure after operation %1: %2").arg(operationType, message);
    };

    QJsonObject data;
    std::vector<QJsonArray> snapshots;
    snapshots.reserve(targets.size());
    for (const auto &target : targets) {
        snapshots.push_back(target.object->snapshot());
        data.insert(target.name, snapshots.back());
    }

    const QJsonObject record {
        {QStringLiteral("op"), operation},
        {QStringLiteral("data"), data},
    };
    auto line = QJsonDocument(record).toJson(QJsonDocument::Compact);
    line.append('\n');
    if (logFile.write(line) != line.size() || !logFile.flush()) {
        return operationFailure(
            QStringLiteral("failed to write fuzz log: %1").arg(logFile.errorString()));
    }

    return compareSnapshots(snapshots.front(), snapshots[1], operationType,
                            targets.front().object->step(), targets[1].object->step());
}

template <typename Func>
QString invokeLogAndCompare(QFile &logFile,
                            const QJsonObject &operation,
                            const std::vector<ComparisonTarget> &targets,
                            Func &&func) {
    for (const auto &target : targets) {
        std::invoke(func, *target.object);
    }
    return writeLogAndCompare(logFile, operation, targets);
}

QJsonObject scalarOperation(int noteId, const char *field, const QVariant &value) {
    return {
        {QStringLiteral("type"), QStringLiteral("scalar")},
        {QStringLiteral("note_id"), noteId},
        {QStringLiteral("field"), QLatin1String(field)},
        {QStringLiteral("value"), QJsonValue::fromVariant(value)},
    };
}

QJsonObject layoutOperation(int noteId, const char *field, int value) {
    return {
        {QStringLiteral("type"), QStringLiteral("layout")},
        {QStringLiteral("note_id"), noteId},
        {QStringLiteral("field"), QLatin1String(field)},
        {QStringLiteral("value"), value},
    };
}

QJsonObject clipOperation(const char *type, int clipId) {
    return {
        {QStringLiteral("type"), QLatin1String(type)},
        {QStringLiteral("clip_id"), clipId},
    };
}

QJsonObject noteOperation(const char *type, int noteId) {
    return {
        {QStringLiteral("type"), QLatin1String(type)},
        {QStringLiteral("note_id"), noteId},
    };
}

QJsonObject historyOperation(const char *type, int undoCount, int redoCount, int ordinal) {
    return {
        {QStringLiteral("type"), QLatin1String(type)},
        {QStringLiteral("n"), undoCount},
        {QStringLiteral("m"), redoCount},
        {QStringLiteral("ordinal"), ordinal},
    };
}

QString writeSliceLogAndCompare(QFile &logFile,
                                const QJsonObject &operation,
                                const QList<int> &oracleIds,
                                const QList<int> &dspxIds,
                                int oracleStep, int dspxStep) {
    const auto operationType = operation.value(QStringLiteral("type")).toString();
    const auto operationFailure = [&](const QString &message) {
        return QStringLiteral("Failure after operation %1: %2").arg(operationType, message);
    };

    QJsonArray oracleArray;
    for (const auto noteId : oracleIds) {
        oracleArray.append(noteId);
    }
    QJsonArray dspxArray;
    for (const auto noteId : dspxIds) {
        dspxArray.append(noteId);
    }

    const QJsonObject record {
        {QStringLiteral("op"), operation},
        {QStringLiteral("data"), QJsonObject {
            {QStringLiteral("oracle"), oracleArray},
            {QStringLiteral("dspx"), dspxArray},
        }},
    };
    auto line = QJsonDocument(record).toJson(QJsonDocument::Compact);
    line.append('\n');
    if (logFile.write(line) != line.size() || !logFile.flush()) {
        return operationFailure(
            QStringLiteral("failed to write fuzz log: %1").arg(logFile.errorString()));
    }

    const auto commonSize = qMin(oracleIds.size(), dspxIds.size());
    for (qsizetype i = 0; i < commonSize; ++i) {
        if (oracleIds[i] != dspxIds[i]) {
            return operationFailure(
                QStringLiteral("differential mismatch for slice at index %1 (oracle size %2, "
                               "actual size %3)")
                    .arg(i)
                    .arg(oracleIds.size())
                    .arg(dspxIds.size()));
        }
    }
    if (oracleIds.size() != dspxIds.size()) {
        return operationFailure(
            QStringLiteral("size mismatch for slice (oracle %1, actual %2)")
                .arg(oracleIds.size())
                .arg(dspxIds.size()));
    }
    if (oracleStep != dspxStep) {
        return operationFailure(QStringLiteral("undo step mismatch (oracle %1, actual %2)")
                                   .arg(oracleStep)
                                   .arg(dspxStep));
    }
    return {};
}

} // anonymous namespace

class NoteEditingFuzzTest : public QObject {
    Q_OBJECT

private slots:
    void fuzzTest() {
        std::mt19937 rng(fuzzSeed);

        OracleTestSubject oracle;
        DspxTestSubject dspx;

        const std::vector<ComparisonTarget> targets {
            {QStringLiteral("oracle"), &oracle},
            {QStringLiteral("dspx"), &dspx},
        };

        const auto logFileName = QDateTime::currentDateTimeUtc().toString(
                                     QStringLiteral("yyyyMMdd'T'HHmmss'Z'"))
                                 + QStringLiteral(".log");
        QFile logFile(logFileName);
        QVERIFY2(logFile.open(QIODevice::WriteOnly | QIODevice::Text),
                 qPrintable(QStringLiteral("Failed to open fuzz log %1: %2")
                                .arg(logFileName, logFile.errorString())));

        int nextClipId = 0;
        int nextNoteId = 0;

        auto createClip = [&]() {
            const auto clipId = nextClipId++;
            oracle.createClip(clipId);
            dspx.createClip(clipId);
            return clipId;
        };
        createClip();
        createClip();

        for (int clipStep = 0; clipStep < initialClipCount * initialNoteCountPerClip; ++clipStep) {
            const auto clipIds = oracle.clipIds();
            const auto clipId = clipIds.at(randomInteger(rng, 0, static_cast<int>(clipIds.size()) - 1));
            std::optional<NoteProps> props;
            for (int tryCount = 0; tryCount < maximumInsertTries; ++tryCount) {
                const auto candidate = randomNoteProps(rng);
                if (oracle.tupleUniqueInClip(clipId, candidate)) {
                    props = candidate;
                    break;
                }
            }
            QVERIFY2(props.has_value(), "Failed to generate initial unique note props.");
            const auto noteId = nextNoteId++;
            auto error = invokeLogAndCompare(
                logFile,
                {{QStringLiteral("type"), QStringLiteral("insert_note")},
                 {QStringLiteral("clip_id"), clipId},
                 {QStringLiteral("note_id"), noteId},
                 {QStringLiteral("position"), props->position},
                 {QStringLiteral("length"), props->length},
                 {QStringLiteral("key_number"), props->keyNumber}},
                targets,
                [&](TestSubject &subject) {
                    subject.insertNote(clipId, noteId, *props);
                });
            if (!error.isEmpty()) {
                QFAIL(qPrintable(error));
            }
        }

        for (int fuzzStep = 0; fuzzStep < fuzzStepCount; ++fuzzStep) {
            const auto operationBucket = randomInteger(rng, 0, 99);

            QString error;
            bool handled = false;

            if (operationBucket < 23) {
                // Insert a new note.
                const auto clipIds = oracle.clipIds();
                if (!clipIds.isEmpty()) {
                    const auto clipId = clipIds.at(randomInteger(rng, 0, static_cast<int>(clipIds.size()) - 1));
                    std::optional<NoteProps> props;
                    for (int tryCount = 0; tryCount < maximumInsertTries; ++tryCount) {
                        const auto candidate = randomNoteProps(rng);
                        if (oracle.tupleUniqueInClip(clipId, candidate)) {
                            props = candidate;
                            break;
                        }
                    }
                    if (props.has_value()) {
                        const auto noteId = nextNoteId++;
                        handled = true;
                        error = invokeLogAndCompare(
                            logFile,
                            {{QStringLiteral("type"), QStringLiteral("insert_note")},
                             {QStringLiteral("clip_id"), clipId},
                             {QStringLiteral("note_id"), noteId},
                             {QStringLiteral("position"), props->position},
                             {QStringLiteral("length"), props->length},
                             {QStringLiteral("key_number"), props->keyNumber}},
                            targets,
                            [&](TestSubject &subject) {
                                subject.insertNote(clipId, noteId, *props);
                            });
                    }
                }
            } else if (operationBucket < 33) {
                // Delete a random visible note.
                const auto noteIds = oracle.visibleNoteIds();
                if (!noteIds.isEmpty()) {
                    const auto noteId = noteIds.at(randomInteger(rng, 0, static_cast<int>(noteIds.size()) - 1));
                    handled = true;
                    error = invokeLogAndCompare(
                        logFile,
                        noteOperation("delete_note", noteId),
                        targets,
                        [&](TestSubject &subject) { subject.deleteNote(noteId); });
                }
            } else if (operationBucket < 37) {
                // Detach a random visible note.
                const auto noteIds = oracle.visibleNoteIds();
                if (!noteIds.isEmpty()) {
                    const auto noteId = noteIds.at(randomInteger(rng, 0, static_cast<int>(noteIds.size()) - 1));
                    handled = true;
                    error = invokeLogAndCompare(
                        logFile,
                        noteOperation("detach_note", noteId),
                        targets,
                        [&](TestSubject &subject) { subject.detachNote(noteId); });
                }
            } else if (operationBucket < 41) {
                // Attach a detached note to a clip.
                const auto detachedIds = oracle.detachedNoteIds();
                const auto clipIds = oracle.clipIds();
                if (!detachedIds.isEmpty() && !clipIds.isEmpty()) {
                    const auto noteId = detachedIds.at(randomInteger(rng, 0, static_cast<int>(detachedIds.size()) - 1));
                    const auto clipId = clipIds.at(randomInteger(rng, 0, static_cast<int>(clipIds.size()) - 1));
                    handled = true;
                    error = invokeLogAndCompare(
                        logFile,
                        {{QStringLiteral("type"), QStringLiteral("attach_note")},
                         {QStringLiteral("clip_id"), clipId},
                         {QStringLiteral("note_id"), noteId}},
                        targets,
                        [&](TestSubject &subject) { subject.attachNote(noteId, clipId); });
                }
            } else if (operationBucket < 57) {
                // Modify a layout property.
                const auto noteIds = oracle.allNoteIds();
                if (!noteIds.isEmpty()) {
                    const auto noteId = noteIds.at(randomInteger(rng, 0, static_cast<int>(noteIds.size()) - 1));
                    const auto clipId = oracle.clipOfNote(noteId);
                    const auto currentProps = oracle.noteProps(noteId);
                    Q_ASSERT(currentProps.has_value());
                    const LayoutField fields[] {
                        LayoutField::Position,
                        LayoutField::Length,
                        LayoutField::KeyNumber,
                    };
                    const auto field = fields[randomInteger(rng, 0, 2)];
                    std::optional<int> newValue;
                    for (int tryCount = 0; tryCount < maximumLayoutEditTries; ++tryCount) {
                        const auto candidate = [&]() -> int {
                            switch (field) {
                            case LayoutField::Position:
                                return randomInteger(rng, minimumPosition, maximumPosition);
                            case LayoutField::Length:
                                return randomLength(rng);
                            case LayoutField::KeyNumber:
                                return randomKeyNumber(rng);
                            }
                            return 0;
                        }();
                        NoteProps candidateProps = *currentProps;
                        switch (field) {
                        case LayoutField::Position:
                            candidateProps.position = candidate;
                            break;
                        case LayoutField::Length:
                            candidateProps.length = candidate;
                            break;
                        case LayoutField::KeyNumber:
                            candidateProps.keyNumber = candidate;
                            break;
                        }
                        const bool different = [&]() {
                            switch (field) {
                            case LayoutField::Position:
                                return candidate != currentProps->position;
                            case LayoutField::Length:
                                return candidate != currentProps->length;
                            case LayoutField::KeyNumber:
                                return candidate != currentProps->keyNumber;
                            }
                            return true;
                        }();
                        if (different
                            && (clipId < 0
                                || oracle.tupleUniqueInClipExcept(clipId, candidateProps, noteId))) {
                            newValue = candidate;
                            break;
                        }
                    }
                    if (newValue.has_value()) {
                        handled = true;
                        error = invokeLogAndCompare(
                            logFile,
                            layoutOperation(noteId,
                                            field == LayoutField::Position ? "position"
                                                : field == LayoutField::Length ? "length"
                                                                               : "key_number",
                                            *newValue),
                            targets,
                            [&](TestSubject &subject) {
                                subject.setNoteLayout(noteId, field, *newValue);
                            });
                    }
                }
            } else if (operationBucket < 67) {
                // Modify a scalar property.
                const auto noteIds = oracle.allNoteIds();
                if (!noteIds.isEmpty()) {
                    const auto noteId = noteIds.at(randomInteger(rng, 0, static_cast<int>(noteIds.size()) - 1));
                    const ScalarField fields[] {
                        ScalarField::Lyric,
                        ScalarField::Language,
                        ScalarField::EditedPronunciation,
                        ScalarField::CentShift,
                        ScalarField::VibratoAmplitude,
                        ScalarField::VibratoEnd,
                        ScalarField::VibratoFrequency,
                        ScalarField::VibratoOffset,
                        ScalarField::VibratoPhase,
                        ScalarField::VibratoStart,
                    };
                    const auto field = fields[randomInteger(rng, 0, 9)];
                    const auto oldValue = [&]() -> QVariant {
                        const auto props = oracle.noteProps(noteId);
                        Q_ASSERT(props.has_value());
                        switch (field) {
                        case ScalarField::Lyric:
                            return QVariant(props->lyric);
                        case ScalarField::Language:
                            return QVariant(props->language);
                        case ScalarField::EditedPronunciation:
                            return QVariant(props->editedPronunciation);
                        case ScalarField::CentShift:
                            return QVariant(props->centShift);
                        case ScalarField::VibratoAmplitude:
                            return QVariant(props->vibratoAmplitude);
                        case ScalarField::VibratoEnd:
                            return QVariant(props->vibratoEnd);
                        case ScalarField::VibratoFrequency:
                            return QVariant(props->vibratoFrequency);
                        case ScalarField::VibratoOffset:
                            return QVariant(props->vibratoOffset);
                        case ScalarField::VibratoPhase:
                            return QVariant(props->vibratoPhase);
                        case ScalarField::VibratoStart:
                            return QVariant(props->vibratoStart);
                        }
                        return {};
                    }();
                    auto newValue = randomScalarValue(field, rng);
                    for (int tryCount = 0; tryCount < maximumLayoutEditTries
                            && newValue == oldValue;
                         ++tryCount) {
                        newValue = randomScalarValue(field, rng);
                    }
                    if (newValue != oldValue) {
                        handled = true;
                        error = invokeLogAndCompare(
                            logFile,
                            scalarOperation(noteId, scalarFieldName(field), newValue),
                            targets,
                            [&](TestSubject &subject) {
                                subject.setNoteScalar(noteId, field, newValue);
                            });
                    }
                }
            } else if (operationBucket < 77) {
                // Move a visible note to another clip.
                const auto noteIds = oracle.visibleNoteIds();
                if (static_cast<int>(noteIds.size()) > 0 && oracle.clipIds().size() > 1) {
                    const auto noteId = noteIds.at(randomInteger(rng, 0, static_cast<int>(noteIds.size()) - 1));
                    const auto sourceClipId = oracle.clipOfNote(noteId);
                    std::optional<int> targetClipId;
                    for (int tryCount = 0; tryCount < maximumInsertTries; ++tryCount) {
                        const auto clipIds = oracle.clipIds();
                        const auto candidate = clipIds.at(randomInteger(rng, 0, static_cast<int>(clipIds.size()) - 1));
                        if (candidate == sourceClipId) {
                            continue;
                        }
                        const auto props = oracle.noteProps(noteId);
                        Q_ASSERT(props.has_value());
                        if (oracle.tupleUniqueInClip(candidate, *props)) {
                            targetClipId = candidate;
                            break;
                        }
                    }
                    if (targetClipId.has_value()) {
                        handled = true;
                        error = invokeLogAndCompare(
                            logFile,
                            {{QStringLiteral("type"), QStringLiteral("move_note")},
                             {QStringLiteral("clip_id"), *targetClipId},
                             {QStringLiteral("note_id"), noteId}},
                            targets,
                            [&](TestSubject &subject) { subject.moveNote(noteId, *targetClipId); });
                    }
                }
            } else if (operationBucket < 80) {
                // Create a clip.
                if (oracle.clipIds().size() < maximumClipCount) {
                    const auto clipId = nextClipId++;
                    handled = true;
                    error = invokeLogAndCompare(
                        logFile,
                        clipOperation("create_clip", clipId),
                        targets,
                        [&](TestSubject &subject) { subject.createClip(clipId); });
                }
            } else if (operationBucket < 81) {
                // Destroy a clip.
                const auto clipIds = oracle.clipIds();
                if (!clipIds.isEmpty()) {
                    const auto clipId = clipIds.at(randomInteger(rng, 0, static_cast<int>(clipIds.size()) - 1));
                    handled = true;
                    error = invokeLogAndCompare(
                        logFile,
                        clipOperation("destroy_clip", clipId),
                        targets,
                        [&](TestSubject &subject) { subject.destroyClip(clipId); });
                }
            } else if (operationBucket < 85) {
                // Slice probe.
                const auto clipIds = oracle.clipIds();
                if (!clipIds.isEmpty()) {
                    const auto clipId = clipIds.at(randomInteger(rng, 0, static_cast<int>(clipIds.size()) - 1));
                    const auto position = randomInteger(rng, 0, maximumPosition * 2);
                    const auto length = randomInteger(rng, 0, maximumLength * 2);

                    const auto operation = QJsonObject {
                        {QStringLiteral("type"), QStringLiteral("slice_probe")},
                        {QStringLiteral("clip_id"), clipId},
                        {QStringLiteral("position"), position},
                        {QStringLiteral("length"), length},
                    };
                    const auto oracleResult = oracle.sliceProbe(clipId, position, length);
                    const auto dspxResult = dspx.sliceProbe(clipId, position, length);

                    handled = true;
                    error = writeSliceLogAndCompare(logFile, operation,
                                                    oracleResult.noteIds, dspxResult.noteIds,
                                                    oracle.step(), dspx.step());
                }
            } else {
                // Undo/redo batch.
                const auto undoableCount = oracle.step();
                const auto nBucket = randomInteger(rng, 0, 99);
                const auto undoCount = undoableCount == 0 || nBucket < 20 ? 0
                    : nBucket < 30 ? undoableCount
                                   : randomInteger(rng, 1,
                                                   qMin(undoableCount, maximumUndoBatchSize));
                const auto mBucket = randomInteger(rng, 0, 99);
                const auto redoCount = undoCount == 0 || mBucket < 20 ? 0
                    : mBucket < 40 ? undoCount
                                   : randomInteger(rng, 0, undoCount);

                for (int i = 0; i < undoCount; ++i) {
                    handled = true;
                    error = invokeLogAndCompare(
                        logFile,
                        historyOperation("undo", undoCount, redoCount, i + 1),
                        targets,
                        [](TestSubject &subject) { subject.undo(); });
                    if (!error.isEmpty()) {
                        break;
                    }
                }
                for (int i = 0; handled && error.isEmpty() && i < redoCount; ++i) {
                    error = invokeLogAndCompare(
                        logFile,
                        historyOperation("redo", undoCount, redoCount, i + 1),
                        targets,
                        [](TestSubject &subject) { subject.redo(); });
                }
            }

            if (handled) {
                if (!error.isEmpty()) {
                    qInfo().noquote() << QStringLiteral("Step %1").arg(fuzzStep + 2);
                    QFAIL(qPrintable(error));
                }
            } else {
                // No valid operation was possible in this step; skip it.
            }
        }
    }
};

QTEST_GUILESS_MAIN(NoteEditingFuzzTest)

#include "tst_note_editing.moc"
