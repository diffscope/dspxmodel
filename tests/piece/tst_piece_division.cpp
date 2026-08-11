#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include <QtTest/QtTest>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Handle.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <dspxmodelPiece/Piece.h>
#include <dspxmodelPiece/PieceDivider.h>

using namespace dspx;

namespace piece_test_types {

struct PhonemeSpec {
    int start = 0;
    bool onset = false;
};

struct NoteSpec {
    int position = 0;
    int length = 0;
    QString lyric = QStringLiteral("la");
    QList<PhonemeSpec> originalPhonemes;
};

struct TempoSpec {
    int position = 0;
    double value = 120.0;
};

struct Scenario {
    QList<NoteSpec> notes;
    QList<TempoSpec> tempos;
    int clipPosition = 0;
    int clipStart = 0;
    int clipLength = 1920;
    int visibleLength = 1920;
    double paddingBase = 0.0;
    double paddingAdditional = 0.0;
    double paddingGap = 0.0;
    QStringList restLyrics;
};

} // namespace piece_test_types

Q_DECLARE_METATYPE(piece_test_types::Scenario)

namespace {

using piece_test_types::NoteSpec;
using piece_test_types::PhonemeSpec;
using piece_test_types::Scenario;
using piece_test_types::TempoSpec;

constexpr double beatsPerTick = 1.0 / 480.0;
constexpr std::uint32_t bulkSeed = 0x50494543U;

struct ExpectedPiece {
    double position = 0.0;
    double length = 0.0;
};

struct TempoPoint {
    int position = 0;
    double value = 120.0;
    quint64 handle = 0;
};

struct CalculatedNote {
    Note *note = nullptr;
    int relativePosition = 0;
    double startMs = 0.0;
    double endMs = 0.0;
    double leftPadding = 0.0;
    double rightPadding = 0.0;
};

struct PieceSummary {
    double bodyStartMs = 0.0;
    double bodyEndMs = 0.0;
    double leftPadding = 0.0;
    double rightPadding = 0.0;

    double startMs() const {
        return bodyStartMs - leftPadding;
    }

    double endMs() const {
        return bodyEndMs + rightPadding;
    }
};

PhonemeSpec phoneme(int start, bool onset) {
    return {start, onset};
}

NoteSpec note(int position, int length,
              QString lyric = QStringLiteral("la"),
              QList<PhonemeSpec> phonemes = {}) {
    return {position, length, std::move(lyric), std::move(phonemes)};
}

TempoSpec tempo(int position, double value) {
    return {position, value};
}

Scenario scenario(QList<NoteSpec> notes,
                  QList<TempoSpec> tempos = {},
                  double paddingBase = 0.0,
                  double paddingAdditional = 0.0,
                  double paddingGap = 0.0,
                  QStringList restLyrics = {},
                  int clipPosition = 0,
                  int clipStart = 0,
                  int visibleLength = 1920,
                  int clipLength = 1920) {
    return {
        std::move(notes),
        std::move(tempos),
        clipPosition,
        clipStart,
        clipLength,
        visibleLength,
        paddingBase,
        paddingAdditional,
        paddingGap,
        std::move(restLyrics),
    };
}

class TempoMap {
public:
    explicit TempoMap(Model *model) {
        for (auto *item : model->tempos()->asRange()) {
            points.push_back({item->position(), item->value(), item->handle().d});
        }
        std::sort(points.begin(), points.end(), [](const TempoPoint &lhs, const TempoPoint &rhs) {
            if (lhs.position != rhs.position) {
                return lhs.position < rhs.position;
            }
            return lhs.handle < rhs.handle;
        });

        tempoAtZero = 120.0;
        for (const auto &point : points) {
            if (point.position > 0) {
                break;
            }
            tempoAtZero = point.value;
        }
    }

    double toMilliseconds(double tick) const {
        if (tick < 0.0) {
            return tick * millisecondsPerTick(tempoAtZero);
        }

        double result = 0.0;
        double previousTick = 0.0;
        double currentTempo = tempoAtZero;
        for (const auto &point : points) {
            if (point.position == 0) {
                currentTempo = point.value;
                continue;
            }
            if (tick <= point.position) {
                return result + (tick - previousTick) * millisecondsPerTick(currentTempo);
            }
            result += (point.position - previousTick) * millisecondsPerTick(currentTempo);
            previousTick = point.position;
            currentTempo = point.value;
        }
        return result + (tick - previousTick) * millisecondsPerTick(currentTempo);
    }

    double toTicks(double milliseconds) const {
        if (milliseconds < 0.0) {
            return milliseconds / millisecondsPerTick(tempoAtZero);
        }

        double remaining = milliseconds;
        double previousTick = 0.0;
        double currentTempo = tempoAtZero;
        for (const auto &point : points) {
            if (point.position == 0) {
                currentTempo = point.value;
                continue;
            }
            const auto segmentMilliseconds =
                (point.position - previousTick) * millisecondsPerTick(currentTempo);
            if (remaining <= segmentMilliseconds) {
                return previousTick + remaining / millisecondsPerTick(currentTempo);
            }
            remaining -= segmentMilliseconds;
            previousTick = point.position;
            currentTempo = point.value;
        }
        return previousTick + remaining / millisecondsPerTick(currentTempo);
    }

private:
    static double millisecondsPerTick(double bpm) {
        return 60000.0 * beatsPerTick / bpm;
    }

    std::vector<TempoPoint> points;
    double tempoAtZero = 120.0;
};

int leadingNonOnsetCount(Note *note) {
    std::vector<Phoneme *> phonemes;
    for (auto *item : note->originalPhonemes()->asRange()) {
        phonemes.push_back(item);
    }
    std::sort(phonemes.begin(), phonemes.end(), [](Phoneme *lhs, Phoneme *rhs) {
        if (lhs->start() != rhs->start()) {
            return lhs->start() < rhs->start();
        }
        if (lhs->onset() != rhs->onset()) {
            return lhs->onset() && !rhs->onset();
        }
        return lhs->handle().d < rhs->handle().d;
    });

    int result = 0;
    for (auto *item : phonemes) {
        if (item->onset()) {
            break;
        }
        ++result;
    }
    return result;
}

PieceSummary summarize(const std::vector<CalculatedNote> &notes,
                       std::size_t begin, std::size_t end) {
    Q_ASSERT(begin < end);

    PieceSummary result;
    result.bodyStartMs = notes[begin].startMs;
    result.bodyEndMs = notes[begin].endMs;
    for (auto i = begin + 1; i < end; ++i) {
        result.bodyStartMs = std::min(result.bodyStartMs, notes[i].startMs);
        result.bodyEndMs = std::max(result.bodyEndMs, notes[i].endMs);
    }

    result.leftPadding = std::numeric_limits<double>::infinity();
    for (auto i = begin; i < end; ++i) {
        if (notes[i].startMs == result.bodyStartMs) {
            result.leftPadding = std::min(result.leftPadding, notes[i].leftPadding);
        }
    }

    double latestStartAtMaximumEnd = -std::numeric_limits<double>::infinity();
    for (auto i = begin; i < end; ++i) {
        if (notes[i].endMs == result.bodyEndMs) {
            latestStartAtMaximumEnd = std::max(latestStartAtMaximumEnd, notes[i].startMs);
        }
    }

    result.rightPadding = std::numeric_limits<double>::infinity();
    for (auto i = begin; i < end; ++i) {
        if (notes[i].endMs == result.bodyEndMs
            && notes[i].startMs == latestStartAtMaximumEnd) {
            result.rightPadding = std::min(result.rightPadding, notes[i].rightPadding);
        }
    }
    return result;
}

QList<ExpectedPiece> referenceDivision(SingingClip *clip,
                                       double paddingBase,
                                       double paddingAdditional,
                                       double paddingGap,
                                       const QStringList &restLyrics) {
    const TempoMap tempoMap(clip->model());
    const auto absoluteClipStart = static_cast<double>(clip->start());

    std::vector<CalculatedNote> notes;
    for (auto *item : clip->notes()->asRange()) {
        const auto rest = restLyrics.contains(item->lyric());
        const auto leadingCount = leadingNonOnsetCount(item);
        const auto absoluteStart = absoluteClipStart + item->position();
        const auto absoluteEnd = absoluteStart + item->length();
        notes.push_back({
            item,
            item->position(),
            tempoMap.toMilliseconds(absoluteStart),
            tempoMap.toMilliseconds(absoluteEnd),
            rest ? 0.0 : std::min(paddingBase + leadingCount * paddingAdditional,
                                  paddingGap),
            rest ? 0.0 : paddingBase,
        });
    }

    std::sort(notes.begin(), notes.end(), [](const CalculatedNote &lhs,
                                             const CalculatedNote &rhs) {
        if (lhs.relativePosition != rhs.relativePosition) {
            return lhs.relativePosition < rhs.relativePosition;
        }
        return lhs.note->handle().d < rhs.note->handle().d;
    });

    QList<ExpectedPiece> result;
    if (notes.empty()) {
        return result;
    }

    auto appendPiece = [&](std::size_t begin, std::size_t end) {
        const auto summary = summarize(notes, begin, end);
        const auto absoluteStartTick = tempoMap.toTicks(summary.startMs());
        const auto absoluteEndTick = tempoMap.toTicks(summary.endMs());
        result.push_back({
            absoluteStartTick - absoluteClipStart,
            absoluteEndTick - absoluteStartTick,
        });
    };

    std::size_t pieceBegin = 0;
    std::size_t currentEnd = 0;
    while (currentEnd < notes.size()
           && notes[currentEnd].relativePosition == notes[0].relativePosition) {
        ++currentEnd;
    }

    while (currentEnd < notes.size()) {
        const auto nextBegin = currentEnd;
        auto nextEnd = nextBegin + 1;
        while (nextEnd < notes.size()
               && notes[nextEnd].relativePosition == notes[nextBegin].relativePosition) {
            ++nextEnd;
        }

        const auto current = summarize(notes, pieceBegin, currentEnd);
        const auto nextGroup = summarize(notes, nextBegin, nextEnd);
        const auto bodyGap = nextGroup.bodyStartMs - current.bodyEndMs;
        const auto piecesDoNotOverlap = current.endMs() <= nextGroup.startMs();
        if (bodyGap >= paddingGap && piecesDoNotOverlap) {
            appendPiece(pieceBegin, currentEnd);
            pieceBegin = nextBegin;
        }
        currentEnd = nextEnd;
    }
    appendPiece(pieceBegin, notes.size());
    return result;
}

bool closeEnough(double actual, double expected) {
    const auto scale = std::max({1.0, std::abs(actual), std::abs(expected)});
    return std::abs(actual - expected) <= 1e-8 * scale;
}

QList<ExpectedPiece> actualPieces(const PieceDivider &divider) {
    QList<ExpectedPiece> result;
    for (auto *piece : divider.pieces()) {
        result.push_back({piece->position(), piece->length()});
    }
    return result;
}

void comparePieceLists(const QList<ExpectedPiece> &actual,
                       const QList<ExpectedPiece> &expected) {
    QCOMPARE(actual.size(), expected.size());
    for (int i = 0; i < expected.size(); ++i) {
        const auto positionMessage = QStringLiteral(
            "piece %1 position: actual=%2 expected=%3")
                                         .arg(i)
                                         .arg(actual[i].position, 0, 'g', 17)
                                         .arg(expected[i].position, 0, 'g', 17);
        QVERIFY2(closeEnough(actual[i].position, expected[i].position),
                 qPrintable(positionMessage));

        const auto lengthMessage = QStringLiteral(
            "piece %1 length: actual=%2 expected=%3")
                                       .arg(i)
                                       .arg(actual[i].length, 0, 'g', 17)
                                       .arg(expected[i].length, 0, 'g', 17);
        QVERIFY2(closeEnough(actual[i].length, expected[i].length),
                 qPrintable(lengthMessage));
    }
}

void verifyDivision(const PieceDivider &divider) {
    QVERIFY(divider.singingClip() != nullptr);
    const auto expected = referenceDivision(divider.singingClip(),
                                            divider.paddingBase(),
                                            divider.paddingAdditional(),
                                            divider.paddingGap(),
                                            divider.restLyrics());
    comparePieceLists(actualPieces(divider), expected);
}

class PieceFixture {
public:
    Document document;
    Model model {&document};
    PieceDivider divider;
    Track *track = nullptr;
    SingingClip *clip = nullptr;

    PieceFixture() {
        commit([&] {
            track = model.createTrack();
            clip = model.createSingingClip();
            track->clips()->insertItem(clip);
            model.tracks()->insertItem(0, track);
        });
    }

    template <typename Func>
    void commit(Func &&func) {
        auto transaction = document.engine()->beginTransaction();
        document.setTransaction(&transaction);
        try {
            std::invoke(std::forward<Func>(func));
            transaction.commit();
            document.setTransaction(nullptr);
            divider.update();
        } catch (...) {
            if (transaction.state() == dini::TransactionState::Active
                || transaction.state() == dini::TransactionState::Failed) {
                transaction.rollback();
            }
            document.setTransaction(nullptr);
            throw;
        }
    }

    template <typename Func>
    void rollBack(Func &&func) {
        auto transaction = document.engine()->beginTransaction();
        document.setTransaction(&transaction);
        std::invoke(std::forward<Func>(func));
        transaction.rollback();
        document.setTransaction(nullptr);
        divider.update();
    }

    Note *addNote(SingingClip *target, const NoteSpec &spec) {
        auto *result = model.createNote();
        result->setPosition(spec.position);
        result->setLength(spec.length);
        result->setLyric(spec.lyric);
        target->notes()->insertItem(result);
        for (const auto &phonemeSpec : spec.originalPhonemes) {
            auto *item = model.createPhoneme();
            item->setStart(phonemeSpec.start);
            item->setOnset(phonemeSpec.onset);
            result->originalPhonemes()->insertItem(item);
        }
        return result;
    }

    Tempo *addTempo(const TempoSpec &spec) {
        auto *result = model.createTempo();
        result->setPosition(spec.position);
        result->setValue(spec.value);
        model.tempos()->insertItem(result);
        return result;
    }

    Phoneme *addPhoneme(PhonemeSequence *target, int start, bool onset) {
        auto *result = model.createPhoneme();
        result->setStart(start);
        result->setOnset(onset);
        target->insertItem(result);
        return result;
    }

    SingingClip *addClip(Track *targetTrack = nullptr) {
        auto *result = model.createSingingClip();
        if (targetTrack) {
            targetTrack->clips()->insertItem(result);
        }
        return result;
    }

    void load(const Scenario &input) {
        commit([&] {
            clip->setPosition(input.clipPosition);
            clip->setClipStart(input.clipStart);
            clip->setLength(input.visibleLength);
            clip->setClipLength(input.clipLength);
            for (const auto &tempoSpec : input.tempos) {
                addTempo(tempoSpec);
            }
            for (const auto &noteSpec : input.notes) {
                addNote(clip, noteSpec);
            }
        });

        divider.setPaddingBase(input.paddingBase);
        divider.setPaddingAdditional(input.paddingAdditional);
        divider.setPaddingGap(input.paddingGap);
        divider.setRestLyrics(input.restLyrics);
        divider.setSingingClip(clip);
        divider.update();
        document.engine()->clearUndoHistory();
    }
};

enum NoteEdit {
    InsertNote,
    RemoveNote,
    DestroyRemovedNote,
    MoveNoteEarlier,
    MoveNoteLater,
    ShortenNote,
    LengthenNoteAcrossNeighbors,
    MakeRest,
    MakeNonRest,
    MoveNoteIntoClip,
    MoveNoteOutOfClip,
    InsertEqualStartNote,
    InsertZeroLengthNote,
    EditSeveralNoteFields,
};

enum PhonemeEdit {
    InsertLeadingNonOnset,
    InsertTrailingNonOnset,
    RemoveLeadingNonOnset,
    ChangeStartBeforeOnset,
    ChangeStartAfterOnset,
    ToggleLeadingToOnset,
    ToggleOnsetToNonOnset,
    MoveOriginalToEdited,
    MoveEditedToOriginal,
    MoveOriginalBetweenNotes,
    AddEqualStartOnsetTie,
    EditIgnoredOriginalFields,
    EditEditedPhoneme,
};

enum TempoEdit {
    InsertTempoPoint,
    InsertTempoAtZero,
    RemoveTempoPoint,
    RemoveAllTempoPoints,
    ChangeTempoValue,
    MoveTempoPoint,
    EditSeveralTempoPoints,
};

} // namespace

class PieceDivisionTest : public QObject {
    Q_OBJECT

private slots:
    void staticDivision_data();
    void staticDivision();
    void noteRebuildOperations_data();
    void noteRebuildOperations();
    void originalPhonemeRebuildOperations_data();
    void originalPhonemeRebuildOperations();
    void tempoRebuildOperations_data();
    void tempoRebuildOperations();
    void dividerConfigurationChanges();
    void commitAndRollbackTransactions();
    void undoAndRedo();
    void sequentialTransactionsAndCombinedEdges();
    void clipTimingAndBindingChanges();
    void deterministicLargeBatch();
};

void PieceDivisionTest::staticDivision_data() {
    QTest::addColumn<Scenario>("input");

    auto add = [](const char *name, Scenario input) {
        QTest::newRow(name) << std::move(input);
    };

    add("empty-clip", scenario({}));
    add("single-zero-length", scenario({note(0, 0)}));
    add("single-ordinary-note", scenario({note(120, 480)}));
    add("single-note-base-padding", scenario({note(120, 480)}, {}, 80, 0, 200));
    add("single-note-one-leading-phoneme",
        scenario({note(120, 480, QStringLiteral("la"), {phoneme(-40, false)})},
                 {}, 60, 25, 200));
    add("single-note-many-leading-phonemes-capped",
        scenario({note(120, 480, QStringLiteral("la"),
                       {phoneme(-80, false), phoneme(-40, false), phoneme(0, false)})},
                 {}, 50, 80, 140));
    add("first-phoneme-is-onset",
        scenario({note(120, 480, QStringLiteral("la"),
                       {phoneme(-80, true), phoneme(-40, false)})},
                 {}, 50, 80, 300));
    add("non-onset-prefix-stops-at-onset",
        scenario({note(120, 480, QStringLiteral("la"),
                       {phoneme(-120, false), phoneme(-80, false), phoneme(-40, true),
                        phoneme(0, false)})},
                 {}, 30, 40, 300));
    add("phonemes-sorted-by-start-not-insertion",
        scenario({note(120, 480, QStringLiteral("la"),
                       {phoneme(20, false), phoneme(-40, true), phoneme(-100, false)})},
                 {}, 30, 40, 300));
    add("equal-phoneme-start-onset-precedes-non-onset",
        scenario({note(120, 480, QStringLiteral("la"),
                       {phoneme(-100, false), phoneme(-100, true), phoneme(-120, false)})},
                 {}, 30, 40, 300));
    add("duplicate-equal-non-onset-phonemes",
        scenario({note(120, 480, QStringLiteral("la"),
                       {phoneme(-100, false), phoneme(-100, false), phoneme(0, true)})},
                 {}, 30, 40, 300));
    add("negative-original-phoneme-start",
        scenario({note(0, 480, QStringLiteral("la"),
                       {phoneme(-1000, false), phoneme(-999, true)})},
                 {}, 20, 35, 200));
    add("strict-rest-match",
        scenario({note(120, 480, QStringLiteral("R"))}, {}, 100, 70, 300,
                 {QStringLiteral("R")}));
    add("rest-match-is-case-sensitive",
        scenario({note(120, 480, QStringLiteral("r"))}, {}, 100, 70, 300,
                 {QStringLiteral("R")}));
    add("empty-lyric-can-be-rest",
        scenario({note(120, 480, QString())}, {}, 100, 70, 300,
                 {QString()}));
    add("duplicate-rest-lyrics",
        scenario({note(120, 480, QStringLiteral("SP"))}, {}, 100, 70, 300,
                 {QStringLiteral("SP"), QStringLiteral("SP")}));
    add("padding-extends-before-clip",
        scenario({note(0, 240)}, {}, 180, 0, 300));
    add("padding-extends-after-visible-clip",
        scenario({note(1800, 480)}, {}, 180, 0, 300, {}, 0, 0, 300, 300));
    add("visible-range-does-not-clip-any-boundary",
        scenario({note(0, 240), note(3000, 240)}, {}, 120, 0, 200, {},
                 960, 720, 1, 1));
    add("two-notes-split-with-zero-gap-rule",
        scenario({note(0, 240), note(480, 240)}));
    add("two-touching-notes-are-separate-at-zero-padding",
        scenario({note(0, 240), note(240, 240)}));
    add("body-gap-exactly-padding-gap",
        scenario({note(0, 240), note(624, 240)}, {}, 20, 0, 100));
    add("body-gap-one-tick-below-padding-gap",
        scenario({note(0, 240), note(623, 240)}, {}, 20, 0, 100));
    add("padding-overlap-prevents-otherwise-legal-cut",
        scenario({note(0, 240), note(720, 240)}, {}, 180, 0, 100));
    add("piece-padding-touches-exactly",
        scenario({note(0, 240), note(816, 240)}, {}, 75, 0, 150));
    add("left-padding-is-capped-by-gap",
        scenario({note(0, 240, QStringLiteral("la"),
                       {phoneme(-80, false), phoneme(-40, false)})},
                 {}, 100, 100, 120));
    add("overlapping-notes-merge",
        scenario({note(0, 960), note(480, 960)}, {}, 20, 10, 100));
    add("nested-notes-merge",
        scenario({note(0, 1920), note(240, 240), note(960, 120)}, {}, 50, 20, 80));
    add("crossing-overlaps-form-one-chain",
        scenario({note(0, 600), note(480, 600), note(960, 600)}, {}, 0, 0, 20));
    add("same-start-group-is-indivisible",
        scenario({note(0, 0), note(0, 240), note(0, 960), note(1440, 240)},
                 {}, 20, 10, 100));
    add("same-start-group-left-padding-uses-minimum",
        scenario({note(480, 240, QStringLiteral("R")),
                  note(480, 480, QStringLiteral("la"), {phoneme(-20, false)})},
                 {}, 100, 50, 300, {QStringLiteral("R")}));
    add("same-start-group-right-padding-uses-minimum",
        scenario({note(480, 480, QStringLiteral("R")),
                  note(480, 480, QStringLiteral("la"))},
                 {}, 100, 0, 300, {QStringLiteral("R")}));
    add("maximum-end-tie-prefers-latest-start",
        scenario({note(0, 960, QStringLiteral("la")),
                  note(480, 480, QStringLiteral("R"))},
                 {}, 100, 0, 300, {QStringLiteral("R")}));
    add("maximum-end-tie-latest-start-non-rest",
        scenario({note(0, 960, QStringLiteral("R")),
                  note(480, 480, QStringLiteral("la"))},
                 {}, 100, 0, 300, {QStringLiteral("R")}));
    add("zero-length-note-between-pieces",
        scenario({note(0, 240), note(720, 0), note(1440, 240)}, {}, 0, 0, 100));
    add("zero-length-notes-at-same-start",
        scenario({note(480, 0), note(480, 0), note(960, 0)}, {}, 0, 0, 100));
    add("zero-length-piece-with-padding",
        scenario({note(480, 0)}, {}, 90, 0, 200));
    add("rest-zero-length-piece",
        scenario({note(480, 0, QStringLiteral("R"))}, {}, 90, 0, 200,
                 {QStringLiteral("R")}));
    add("unordered-note-creation",
        scenario({note(1920, 240), note(0, 240), note(960, 240), note(480, 240)},
                 {}, 25, 10, 100));
    add("many-greedy-cuts",
        scenario({note(0, 120), note(720, 120), note(1440, 120), note(2160, 120),
                  note(2880, 120), note(3600, 120)},
                 {}, 30, 0, 100));
    add("suffix-stability-equal-maximum-end",
        scenario({note(0, 240), note(720, 960), note(1200, 480), note(2400, 240)},
                 {}, 30, 0, 100));
    add("handle-tie-order-same-start-overlaps",
        scenario({note(480, 960), note(480, 120), note(480, 480), note(2400, 120)},
                 {}, 45, 15, 120));
    add("clip-start-is-position-minus-trim",
        scenario({note(0, 480), note(960, 480)}, {}, 50, 10, 120, {},
                 240, 720));
    add("negative-absolute-clip-start",
        scenario({note(0, 480), note(1200, 480)}, {}, 50, 10, 120, {},
                 0, 960));
    add("large-positive-absolute-clip-start",
        scenario({note(0, 480), note(1200, 480)}, {}, 50, 10, 120, {},
                 10000, 120));

    add("tempo-default-before-first-point",
        scenario({note(0, 480), note(1200, 480)}, {tempo(4000, 60)}, 40, 10, 120));
    add("tempo-at-zero-overrides-default",
        scenario({note(0, 480), note(1200, 480)}, {tempo(0, 60)}, 40, 10, 120));
    add("tempo-change-at-note-start",
        scenario({note(0, 480), note(960, 480)}, {tempo(0, 120), tempo(960, 60)},
                 80, 0, 150));
    add("tempo-change-at-note-end",
        scenario({note(0, 960), note(1920, 240)}, {tempo(0, 120), tempo(960, 240)},
                 80, 0, 150));
    add("note-crosses-tempo-change",
        scenario({note(720, 600), note(1920, 240)}, {tempo(0, 120), tempo(960, 60)},
                 80, 0, 150));
    add("left-padding-crosses-tempo-change",
        scenario({note(960, 240)}, {tempo(0, 60), tempo(900, 240)}, 100, 0, 200));
    add("right-padding-crosses-tempo-change",
        scenario({note(720, 180)}, {tempo(0, 240), tempo(960, 60)}, 100, 0, 200));
    add("multiple-tempo-segments",
        scenario({note(0, 480), note(900, 480), note(1800, 480), note(3000, 480)},
                 {tempo(0, 90), tempo(600, 180), tempo(1440, 45), tempo(2640, 300)},
                 55, 17, 140));
    add("fractional-bpm-tempo",
        scenario({note(100, 777), note(1500, 333)},
                 {tempo(0, 123.456), tempo(999, 87.65)}, 33.3, 12.25, 111.75));
    add("negative-absolute-time-uses-tempo-at-zero",
        scenario({note(0, 240), note(1440, 240)},
                 {tempo(0, 75), tempo(480, 300)}, 100, 30, 200, {}, 0, 1920));
    add("tempo-point-after-all-padding",
        scenario({note(0, 480), note(1440, 480)}, {tempo(100000, 10)}, 50, 0, 100));
    add("slow-tempo-turns-tick-gap-into-legal-ms-gap",
        scenario({note(0, 240), note(720, 240)}, {tempo(0, 30)}, 10, 0, 200));
    add("fast-tempo-turns-tick-gap-into-illegal-ms-gap",
        scenario({note(0, 240), note(720, 240)}, {tempo(0, 480)}, 10, 0, 200));
    add("tempo-change-inside-body-gap",
        scenario({note(0, 240), note(1440, 240)}, {tempo(600, 30)}, 40, 20, 220));
    add("tempo-change-inside-both-paddings",
        scenario({note(480, 120), note(1440, 120)},
                 {tempo(0, 45), tempo(450, 300), tempo(1550, 60)},
                 180, 40, 250));
    add("tempo-and-rest-combination",
        scenario({note(0, 480, QStringLiteral("R")),
                  note(1440, 480, QStringLiteral("la"), {phoneme(-50, false)})},
                 {tempo(0, 80), tempo(960, 200)}, 100, 60, 220,
                 {QStringLiteral("R")}));
    add("tempo-same-start-and-end-ties",
        scenario({note(0, 960), note(480, 480, QStringLiteral("R")),
                  note(1920, 0), note(1920, 240)},
                 {tempo(0, 100), tempo(480, 160), tempo(1920, 70)},
                 70, 25, 160, {QStringLiteral("R")}));
    add("complex-boundary-combination",
        scenario({note(0, 0, QStringLiteral("R")),
                  note(0, 360, QStringLiteral("la"), {phoneme(-20, false)}),
                  note(840, 720, QStringLiteral("la"),
                       {phoneme(-100, false), phoneme(-50, true)}),
                  note(1560, 0), note(2640, 240, QStringLiteral("SP")),
                  note(3360, 960)},
                 {tempo(0, 110), tempo(700, 55), tempo(1600, 220), tempo(3000, 91.25)},
                 85, 37, 175, {QStringLiteral("R"), QStringLiteral("SP")},
                 120, 600, 240, 120));
}

void PieceDivisionTest::staticDivision() {
    QFETCH(Scenario, input);
    PieceFixture fixture;
    fixture.load(input);
    verifyDivision(fixture.divider);
}

void PieceDivisionTest::noteRebuildOperations_data() {
    QTest::addColumn<int>("operation");
    QTest::newRow("insert-note") << int(InsertNote);
    QTest::newRow("remove-note") << int(RemoveNote);
    QTest::newRow("destroy-removed-note") << int(DestroyRemovedNote);
    QTest::newRow("move-note-earlier") << int(MoveNoteEarlier);
    QTest::newRow("move-note-later") << int(MoveNoteLater);
    QTest::newRow("shorten-note") << int(ShortenNote);
    QTest::newRow("lengthen-note-across-neighbors") << int(LengthenNoteAcrossNeighbors);
    QTest::newRow("lyric-becomes-rest") << int(MakeRest);
    QTest::newRow("rest-becomes-non-rest") << int(MakeNonRest);
    QTest::newRow("move-note-into-target-clip") << int(MoveNoteIntoClip);
    QTest::newRow("move-note-out-of-target-clip") << int(MoveNoteOutOfClip);
    QTest::newRow("insert-note-at-existing-start") << int(InsertEqualStartNote);
    QTest::newRow("insert-zero-length-note") << int(InsertZeroLengthNote);
    QTest::newRow("edit-position-length-and-lyric-together") << int(EditSeveralNoteFields);
}

void PieceDivisionTest::noteRebuildOperations() {
    QFETCH(int, operation);

    PieceFixture fixture;
    QList<Note *> notes;
    SingingClip *otherClip = nullptr;
    Note *outsideNote = nullptr;
    fixture.commit([&] {
        fixture.clip->setPosition(480);
        fixture.clip->setClipStart(120);
        notes.push_back(fixture.addNote(fixture.clip, note(0, 480, QStringLiteral("la"))));
        notes.push_back(fixture.addNote(fixture.clip, note(1440, 480, QStringLiteral("R"))));
        notes.push_back(fixture.addNote(fixture.clip, note(3000, 360, QStringLiteral("la"))));
        otherClip = fixture.addClip(fixture.track);
        outsideNote = fixture.addNote(otherClip, note(2200, 240, QStringLiteral("la")));
        fixture.addTempo(tempo(0, 100));
        fixture.addTempo(tempo(1920, 180));
    });
    fixture.divider.setPaddingBase(70);
    fixture.divider.setPaddingAdditional(35);
    fixture.divider.setPaddingGap(180);
    fixture.divider.setRestLyrics({QStringLiteral("R")});
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();
    verifyDivision(fixture.divider);

    fixture.commit([&] {
        switch (static_cast<NoteEdit>(operation)) {
        case InsertNote:
            fixture.addNote(fixture.clip, note(2200, 240, QStringLiteral("la")));
            break;
        case RemoveNote:
            fixture.clip->notes()->removeItem(notes[1]);
            break;
        case DestroyRemovedNote:
            fixture.clip->notes()->removeItem(notes[1]);
            fixture.model.destroyItem(notes[1]);
            break;
        case MoveNoteEarlier:
            notes[2]->setPosition(1000);
            break;
        case MoveNoteLater:
            notes[0]->setPosition(2600);
            break;
        case ShortenNote:
            notes[0]->setLength(0);
            break;
        case LengthenNoteAcrossNeighbors:
            notes[0]->setLength(3400);
            break;
        case MakeRest:
            notes[0]->setLyric(QStringLiteral("R"));
            break;
        case MakeNonRest:
            notes[1]->setLyric(QStringLiteral("not-rest"));
            break;
        case MoveNoteIntoClip:
            otherClip->notes()->moveItem(outsideNote, fixture.clip->notes());
            break;
        case MoveNoteOutOfClip:
            fixture.clip->notes()->moveItem(notes[1], otherClip->notes());
            break;
        case InsertEqualStartNote:
            fixture.addNote(fixture.clip,
                            note(1440, 1200, QStringLiteral("la"),
                                 {phoneme(-100, false)}));
            break;
        case InsertZeroLengthNote:
            fixture.addNote(fixture.clip, note(2200, 0, QStringLiteral("R")));
            break;
        case EditSeveralNoteFields:
            notes[0]->setPosition(720);
            notes[0]->setLength(1800);
            notes[0]->setLyric(QStringLiteral("R"));
            notes[2]->setPosition(2520);
            notes[2]->setLength(0);
            break;
        }
    });

    verifyDivision(fixture.divider);
}

void PieceDivisionTest::originalPhonemeRebuildOperations_data() {
    QTest::addColumn<int>("operation");
    QTest::newRow("insert-leading-non-onset") << int(InsertLeadingNonOnset);
    QTest::newRow("insert-trailing-non-onset") << int(InsertTrailingNonOnset);
    QTest::newRow("remove-leading-non-onset") << int(RemoveLeadingNonOnset);
    QTest::newRow("move-phoneme-before-onset") << int(ChangeStartBeforeOnset);
    QTest::newRow("move-phoneme-after-onset") << int(ChangeStartAfterOnset);
    QTest::newRow("toggle-leading-phoneme-to-onset") << int(ToggleLeadingToOnset);
    QTest::newRow("toggle-onset-to-non-onset") << int(ToggleOnsetToNonOnset);
    QTest::newRow("move-original-to-edited") << int(MoveOriginalToEdited);
    QTest::newRow("move-edited-to-original") << int(MoveEditedToOriginal);
    QTest::newRow("move-original-between-notes") << int(MoveOriginalBetweenNotes);
    QTest::newRow("add-equal-start-onset-tie") << int(AddEqualStartOnsetTie);
    QTest::newRow("edit-ignored-original-fields") << int(EditIgnoredOriginalFields);
    QTest::newRow("edit-edited-phoneme-is-ignored") << int(EditEditedPhoneme);
}

void PieceDivisionTest::originalPhonemeRebuildOperations() {
    QFETCH(int, operation);

    PieceFixture fixture;
    Note *first = nullptr;
    Note *second = nullptr;
    Phoneme *leading = nullptr;
    Phoneme *onset = nullptr;
    Phoneme *trailing = nullptr;
    Phoneme *edited = nullptr;
    fixture.commit([&] {
        first = fixture.addNote(fixture.clip, note(480, 480));
        second = fixture.addNote(fixture.clip, note(1920, 480));
        leading = fixture.addPhoneme(first->originalPhonemes(), -120, false);
        onset = fixture.addPhoneme(first->originalPhonemes(), -40, true);
        trailing = fixture.addPhoneme(first->originalPhonemes(), 20, false);
        fixture.addPhoneme(second->originalPhonemes(), -80, false);
        fixture.addPhoneme(second->originalPhonemes(), 0, true);
        edited = fixture.addPhoneme(first->editedPhonemes(), -500, false);
        fixture.addTempo(tempo(0, 120));
        fixture.addTempo(tempo(1440, 75));
    });
    fixture.divider.setPaddingBase(60);
    fixture.divider.setPaddingAdditional(55);
    fixture.divider.setPaddingGap(220);
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();
    verifyDivision(fixture.divider);

    fixture.commit([&] {
        switch (static_cast<PhonemeEdit>(operation)) {
        case InsertLeadingNonOnset:
            fixture.addPhoneme(first->originalPhonemes(), -200, false);
            break;
        case InsertTrailingNonOnset:
            fixture.addPhoneme(first->originalPhonemes(), 200, false);
            break;
        case RemoveLeadingNonOnset:
            first->originalPhonemes()->removeItem(leading);
            break;
        case ChangeStartBeforeOnset:
            trailing->setStart(-80);
            break;
        case ChangeStartAfterOnset:
            leading->setStart(80);
            break;
        case ToggleLeadingToOnset:
            leading->setOnset(true);
            break;
        case ToggleOnsetToNonOnset:
            onset->setOnset(false);
            break;
        case MoveOriginalToEdited:
            first->originalPhonemes()->moveItem(leading, first->editedPhonemes());
            break;
        case MoveEditedToOriginal:
            first->editedPhonemes()->moveItem(edited, first->originalPhonemes());
            break;
        case MoveOriginalBetweenNotes:
            first->originalPhonemes()->moveItem(leading, second->originalPhonemes());
            break;
        case AddEqualStartOnsetTie:
            fixture.addPhoneme(first->originalPhonemes(), leading->start(), true);
            fixture.addPhoneme(first->originalPhonemes(), leading->start(), false);
            break;
        case EditIgnoredOriginalFields:
            leading->setLanguage(QStringLiteral("zh"));
            leading->setToken(QStringLiteral("ignored"));
            break;
        case EditEditedPhoneme:
            edited->setStart(-1000);
            edited->setOnset(true);
            edited->setLanguage(QStringLiteral("ja"));
            edited->setToken(QStringLiteral("ignored"));
            break;
        }
    });

    verifyDivision(fixture.divider);
}

void PieceDivisionTest::tempoRebuildOperations_data() {
    QTest::addColumn<int>("operation");
    QTest::newRow("insert-tempo-point") << int(InsertTempoPoint);
    QTest::newRow("insert-tempo-at-zero") << int(InsertTempoAtZero);
    QTest::newRow("remove-tempo-point") << int(RemoveTempoPoint);
    QTest::newRow("remove-all-tempo-points") << int(RemoveAllTempoPoints);
    QTest::newRow("change-tempo-value") << int(ChangeTempoValue);
    QTest::newRow("move-tempo-point") << int(MoveTempoPoint);
    QTest::newRow("edit-several-tempo-points") << int(EditSeveralTempoPoints);
}

void PieceDivisionTest::tempoRebuildOperations() {
    QFETCH(int, operation);

    PieceFixture fixture;
    QList<Tempo *> tempos;
    fixture.commit([&] {
        fixture.clip->setPosition(720);
        fixture.clip->setClipStart(120);
        fixture.addNote(fixture.clip, note(0, 600));
        fixture.addNote(fixture.clip, note(1200, 360));
        fixture.addNote(fixture.clip, note(2520, 720));
        tempos.push_back(fixture.addTempo(tempo(480, 90)));
        tempos.push_back(fixture.addTempo(tempo(1920, 180)));
        tempos.push_back(fixture.addTempo(tempo(3600, 60)));
    });
    fixture.divider.setPaddingBase(125);
    fixture.divider.setPaddingAdditional(30);
    fixture.divider.setPaddingGap(210);
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();
    verifyDivision(fixture.divider);

    fixture.commit([&] {
        switch (static_cast<TempoEdit>(operation)) {
        case InsertTempoPoint:
            fixture.addTempo(tempo(1440, 45));
            break;
        case InsertTempoAtZero:
            fixture.addTempo(tempo(0, 240));
            break;
        case RemoveTempoPoint:
            fixture.model.tempos()->removeItem(tempos[1]);
            break;
        case RemoveAllTempoPoints:
            for (auto *item : tempos) {
                fixture.model.tempos()->removeItem(item);
            }
            break;
        case ChangeTempoValue:
            tempos[0]->setValue(333.333);
            break;
        case MoveTempoPoint:
            tempos[1]->setPosition(1680);
            break;
        case EditSeveralTempoPoints:
            tempos[0]->setPosition(240);
            tempos[0]->setValue(150);
            tempos[1]->setPosition(2160);
            tempos[1]->setValue(55);
            fixture.model.tempos()->removeItem(tempos[2]);
            fixture.addTempo(tempo(2880, 271.5));
            break;
        }
    });

    verifyDivision(fixture.divider);
}

void PieceDivisionTest::dividerConfigurationChanges() {
    PieceFixture fixture;
    fixture.load(scenario({
                              note(0, 480, QStringLiteral("la"),
                                   {phoneme(-120, false), phoneme(-60, false), phoneme(0, true)}),
                              note(1200, 480, QStringLiteral("R")),
                              note(2520, 240, QStringLiteral("SP")),
                              note(3600, 0, QStringLiteral("la")),
                          },
                          {tempo(0, 100), tempo(1920, 220)}, 10, 5, 50,
                          {QStringLiteral("R")}));

    const auto check = [&] {
        fixture.divider.update();
        verifyDivision(fixture.divider);
    };
    check();

    const auto beforeRestoredConfiguration = actualPieces(fixture.divider);
    int pieceSignalCount = 0;
    for (Piece *piece : fixture.divider.pieces()) {
        connect(piece, &Piece::positionChanged, this, [&pieceSignalCount] { ++pieceSignalCount; });
        connect(piece, &Piece::lengthChanged, this, [&pieceSignalCount] { ++pieceSignalCount; });
    }
    connect(&fixture.divider, &PieceDivider::pieceAboutToInsert, this,
            [&pieceSignalCount] { ++pieceSignalCount; });
    connect(&fixture.divider, &PieceDivider::pieceInserted, this,
            [&pieceSignalCount] { ++pieceSignalCount; });
    connect(&fixture.divider, &PieceDivider::pieceAboutToRemove, this,
            [&pieceSignalCount] { ++pieceSignalCount; });
    connect(&fixture.divider, &PieceDivider::pieceRemoved, this,
            [&pieceSignalCount] { ++pieceSignalCount; });
    connect(&fixture.divider, &PieceDivider::piecesChanged, this,
            [&pieceSignalCount] { ++pieceSignalCount; });
    fixture.divider.setPaddingBase(999);
    fixture.divider.setPaddingAdditional(998);
    fixture.divider.setPaddingGap(997);
    fixture.divider.setRestLyrics({QStringLiteral("temporary")});
    fixture.divider.setPaddingBase(10);
    fixture.divider.setPaddingAdditional(5);
    fixture.divider.setPaddingGap(50);
    fixture.divider.setRestLyrics({QStringLiteral("R")});
    fixture.divider.update();
    comparePieceLists(actualPieces(fixture.divider), beforeRestoredConfiguration);
    QCOMPARE(pieceSignalCount, 0);

    const auto beforeDeferredConfiguration = actualPieces(fixture.divider);
    fixture.divider.setPaddingBase(120);
    comparePieceLists(actualPieces(fixture.divider), beforeDeferredConfiguration);
    check();
    fixture.divider.setPaddingAdditional(45);
    check();
    fixture.divider.setPaddingGap(260);
    check();
    fixture.divider.setRestLyrics({QStringLiteral("R"), QStringLiteral("SP")});
    check();
    fixture.divider.setPaddingBase(0);
    check();
    fixture.divider.setPaddingAdditional(0);
    check();
    fixture.divider.setPaddingGap(0);
    check();
    fixture.divider.setRestLyrics({});
    check();

    const auto beforeInvalidValues = actualPieces(fixture.divider);
    fixture.divider.setPaddingBase(-1);
    fixture.divider.setPaddingAdditional(std::numeric_limits<double>::quiet_NaN());
    fixture.divider.setPaddingGap(std::numeric_limits<double>::infinity());
    comparePieceLists(actualPieces(fixture.divider), beforeInvalidValues);
    check();

    fixture.divider.setPaddingBase(1e6);
    fixture.divider.setPaddingAdditional(1e6);
    fixture.divider.setPaddingGap(1e6);
    fixture.divider.setRestLyrics({QStringLiteral("not-present"), QStringLiteral("R")});
    check();
}

void PieceDivisionTest::commitAndRollbackTransactions() {
    PieceFixture fixture;
    fixture.load(scenario({note(0, 480), note(1440, 480), note(3000, 240)},
                          {tempo(0, 120), tempo(1920, 80)}, 60, 25, 180,
                          {QStringLiteral("R")}));
    auto *first = fixture.clip->notes()->firstItem();
    auto *last = fixture.clip->notes()->lastItem();

    const auto committedBeforeEdit = actualPieces(fixture.divider);
    auto transaction = fixture.document.engine()->beginTransaction();
    fixture.document.setTransaction(&transaction);
    first->setPosition(1000);
    first->setLength(2400);
    first->setLyric(QStringLiteral("R"));
    fixture.addPhoneme(first->originalPhonemes(), -200, false);
    fixture.addTempo(tempo(960, 240));

    // ORM values are visible immediately, while the divider remains on committed state.
    comparePieceLists(actualPieces(fixture.divider), committedBeforeEdit);
    transaction.commit();
    fixture.document.setTransaction(nullptr);
    comparePieceLists(actualPieces(fixture.divider), committedBeforeEdit);
    fixture.divider.update();
    verifyDivision(fixture.divider);

    const auto beforeRollback = actualPieces(fixture.divider);
    fixture.rollBack([&] {
        first->setPosition(0);
        first->setLength(0);
        last->setPosition(0);
        last->setLength(5000);
        fixture.model.tempos()->firstItem()->setValue(500);
    });
    comparePieceLists(actualPieces(fixture.divider), beforeRollback);
    verifyDivision(fixture.divider);

    // If a transaction is explicitly refreshed, rollback is another recorded
    // model edit and remains deferred until the following update().
    const auto beforeRefreshedRollback = actualPieces(fixture.divider);
    auto refreshedTransaction = fixture.document.engine()->beginTransaction();
    fixture.document.setTransaction(&refreshedTransaction);
    first->setPosition(2600);
    first->setLength(3200);
    last->setPosition(0);
    fixture.divider.update();
    verifyDivision(fixture.divider);
    const auto refreshedInsideTransaction = actualPieces(fixture.divider);
    refreshedTransaction.rollback();
    fixture.document.setTransaction(nullptr);
    comparePieceLists(actualPieces(fixture.divider), refreshedInsideTransaction);
    fixture.divider.update();
    comparePieceLists(actualPieces(fixture.divider), beforeRefreshedRollback);
    verifyDivision(fixture.divider);

    // Divider configuration is independent of the ORM rollback and is applied
    // only by the explicit update performed by the fixture.
    fixture.rollBack([&] {
        first->setPosition(2500);
        first->setLength(1);
        fixture.divider.setPaddingBase(135);
        fixture.divider.setPaddingAdditional(77);
        fixture.divider.setPaddingGap(333);
        fixture.divider.setRestLyrics({QStringLiteral("R"), QStringLiteral("SP")});
    });
    verifyDivision(fixture.divider);

    // Committing edits while changing every configuration field exercises the
    // desired/applied configuration boundary documented by PieceDivider.
    fixture.commit([&] {
        first->setPosition(720);
        first->setLength(840);
        last->setPosition(3600);
        last->setLyric(QStringLiteral("SP"));
        fixture.divider.setPaddingBase(25);
        fixture.divider.setPaddingAdditional(150);
        fixture.divider.setPaddingGap(210);
        fixture.divider.setRestLyrics({QStringLiteral("SP")});
    });
    verifyDivision(fixture.divider);
}

void PieceDivisionTest::undoAndRedo() {
    PieceFixture fixture;
    Note *first = nullptr;
    Note *second = nullptr;
    Phoneme *leading = nullptr;
    Tempo *middleTempo = nullptr;
    fixture.commit([&] {
        fixture.clip->setPosition(480);
        first = fixture.addNote(fixture.clip, note(0, 480, QStringLiteral("la")));
        second = fixture.addNote(fixture.clip, note(1440, 480, QStringLiteral("R")));
        fixture.addNote(fixture.clip, note(3000, 240, QStringLiteral("la")));
        leading = fixture.addPhoneme(first->originalPhonemes(), -100, false);
        fixture.addPhoneme(first->originalPhonemes(), 0, true);
        fixture.addTempo(tempo(0, 100));
        middleTempo = fixture.addTempo(tempo(1920, 200));
    });
    fixture.divider.setPaddingBase(90);
    fixture.divider.setPaddingAdditional(40);
    fixture.divider.setPaddingGap(200);
    fixture.divider.setRestLyrics({QStringLiteral("R")});
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();

    const auto beforeEdit = actualPieces(fixture.divider);
    fixture.commit([&] {
        first->setPosition(720);
        first->setLength(1900);
        first->setLyric(QStringLiteral("R"));
        second->setPosition(2600);
        second->setLyric(QStringLiteral("la"));
        leading->setOnset(true);
        middleTempo->setPosition(1680);
        middleTempo->setValue(55);
    });
    verifyDivision(fixture.divider);
    const auto afterEdit = actualPieces(fixture.divider);

    QVERIFY(fixture.document.engine()->canUndo());
    fixture.document.engine()->undo();
    fixture.divider.update();
    comparePieceLists(actualPieces(fixture.divider), beforeEdit);
    verifyDivision(fixture.divider);

    QVERIFY(fixture.document.engine()->canRedo());
    fixture.document.engine()->redo();
    fixture.divider.update();
    comparePieceLists(actualPieces(fixture.divider), afterEdit);
    verifyDivision(fixture.divider);

    fixture.commit([&] {
        fixture.addNote(fixture.clip,
                        note(1100, 0, QStringLiteral("la"), {phoneme(-20, false)}));
        fixture.clip->notes()->removeItem(second);
    });
    const auto afterMembershipChange = actualPieces(fixture.divider);
    verifyDivision(fixture.divider);

    fixture.document.engine()->undo();
    fixture.divider.update();
    verifyDivision(fixture.divider);
    fixture.document.engine()->redo();
    fixture.divider.update();
    comparePieceLists(actualPieces(fixture.divider), afterMembershipChange);
    verifyDivision(fixture.divider);

    // Multiple undo/redo steps must each restore a fully valid division.
    fixture.commit([&] {
        first->setPosition(0);
        first->setLength(0);
    });
    verifyDivision(fixture.divider);
    fixture.commit([&] {
        middleTempo->setValue(333);
    });
    verifyDivision(fixture.divider);
    fixture.document.engine()->undo();
    fixture.divider.update();
    verifyDivision(fixture.divider);
    fixture.document.engine()->undo();
    fixture.divider.update();
    verifyDivision(fixture.divider);
    fixture.document.engine()->redo();
    fixture.divider.update();
    verifyDivision(fixture.divider);
    fixture.document.engine()->redo();
    fixture.divider.update();
    verifyDivision(fixture.divider);
}

void PieceDivisionTest::sequentialTransactionsAndCombinedEdges() {
    PieceFixture fixture;
    QList<Note *> notes;
    Tempo *tempoAtZero = nullptr;
    Tempo *laterTempo = nullptr;
    Phoneme *firstLeading = nullptr;
    fixture.commit([&] {
        fixture.clip->setPosition(960);
        fixture.clip->setClipStart(240);
        notes.push_back(fixture.addNote(fixture.clip, note(0, 240)));
        notes.push_back(fixture.addNote(fixture.clip, note(720, 240, QStringLiteral("R"))));
        notes.push_back(fixture.addNote(fixture.clip, note(1680, 720)));
        notes.push_back(fixture.addNote(fixture.clip, note(3000, 0)));
        firstLeading = fixture.addPhoneme(notes[0]->originalPhonemes(), -80, false);
        fixture.addPhoneme(notes[0]->originalPhonemes(), 0, true);
        tempoAtZero = fixture.addTempo(tempo(0, 120));
        laterTempo = fixture.addTempo(tempo(2400, 60));
    });
    fixture.divider.setPaddingBase(40);
    fixture.divider.setPaddingAdditional(30);
    fixture.divider.setPaddingGap(150);
    fixture.divider.setRestLyrics({QStringLiteral("R")});
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();
    verifyDivision(fixture.divider);

    const auto step = [&](const std::function<void()> &edit) {
        fixture.commit(edit);
        verifyDivision(fixture.divider);
    };

    step([&] { notes[1]->setPosition(480); });
    step([&] { notes[2]->setLength(120); });
    step([&] { notes[3]->setPosition(1680); });
    step([&] { notes[1]->setLyric(QStringLiteral("la")); });
    step([&] { firstLeading->setOnset(true); });

    Phoneme *newLeading = nullptr;
    step([&] {
        newLeading = fixture.addPhoneme(notes[2]->originalPhonemes(), -300, false);
        fixture.addPhoneme(notes[2]->originalPhonemes(), -100, false);
        fixture.addPhoneme(notes[2]->originalPhonemes(), 0, true);
    });
    step([&] { newLeading->setStart(200); });
    step([&] { tempoAtZero->setValue(75); });
    step([&] { laterTempo->setPosition(1800); });
    step([&] { fixture.addTempo(tempo(900, 240)); });
    step([&] { fixture.clip->setPosition(120); });
    step([&] { fixture.clip->setClipStart(960); });

    step([&] {
        notes[0]->setPosition(1680);
        notes[0]->setLength(720);
        notes[0]->setLyric(QStringLiteral("R"));
        notes[2]->setPosition(0);
        notes[2]->setLength(0);
        fixture.divider.setPaddingBase(175);
        fixture.divider.setPaddingAdditional(85);
        fixture.divider.setPaddingGap(275);
        fixture.divider.setRestLyrics({QStringLiteral("R"), QStringLiteral("SP")});
    });

    step([&] {
        fixture.clip->notes()->removeItem(notes[3]);
        fixture.addNote(fixture.clip,
                        note(4200, 360, QStringLiteral("SP"),
                             {phoneme(-500, false), phoneme(-400, false),
                              phoneme(-300, true)}));
        tempoAtZero->setValue(180);
        laterTempo->setValue(45);
        fixture.clip->setPosition(2000);
        fixture.clip->setClipStart(100);
    });
}

void PieceDivisionTest::clipTimingAndBindingChanges() {
    PieceFixture fixture;
    SingingClip *otherClip = nullptr;
    Track *otherTrack = nullptr;
    fixture.commit([&] {
        fixture.addNote(fixture.clip, note(0, 480));
        fixture.addNote(fixture.clip, note(1440, 480));
        fixture.addTempo(tempo(0, 80));
        fixture.addTempo(tempo(960, 240));

        otherTrack = fixture.model.createTrack();
        fixture.model.tracks()->insertItem(1, otherTrack);
        otherClip = fixture.addClip(otherTrack);
        otherClip->setPosition(5000);
        otherClip->setClipStart(720);
        otherClip->setLength(1);
        otherClip->setClipLength(1);
        fixture.addNote(otherClip, note(0, 0));
        fixture.addNote(otherClip, note(600, 180));
    });
    fixture.divider.setPaddingBase(120);
    fixture.divider.setPaddingAdditional(35);
    fixture.divider.setPaddingGap(200);
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();
    verifyDivision(fixture.divider);

    fixture.commit([&] { fixture.clip->setPosition(1440); });
    verifyDivision(fixture.divider);
    fixture.commit([&] { fixture.clip->setClipStart(1920); });
    verifyDivision(fixture.divider);
    fixture.commit([&] {
        fixture.clip->setPosition(3000);
        fixture.clip->setClipStart(120);
    });
    verifyDivision(fixture.divider);

    const auto beforeVisibleRangeChange = actualPieces(fixture.divider);
    fixture.commit([&] {
        fixture.clip->setLength(0);
        fixture.clip->setClipLength(0);
    });
    comparePieceLists(actualPieces(fixture.divider), beforeVisibleRangeChange);
    verifyDivision(fixture.divider);

    fixture.commit([&] {
        fixture.track->clips()->moveItem(fixture.clip, otherTrack->clips());
    });
    verifyDivision(fixture.divider);
    fixture.commit([&] {
        otherTrack->clips()->removeItem(fixture.clip);
    });
    verifyDivision(fixture.divider);

    const int piecesBeforeUnbind = fixture.divider.pieces().size();
    fixture.divider.setSingingClip(nullptr);
    QCOMPARE(fixture.divider.pieces().size(), piecesBeforeUnbind);
    fixture.divider.update();
    QCOMPARE(fixture.divider.pieces().size(), 0);
    fixture.divider.setSingingClip(otherClip);
    fixture.divider.update();
    verifyDivision(fixture.divider);
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    verifyDivision(fixture.divider);
}

void PieceDivisionTest::deterministicLargeBatch() {
    PieceFixture fixture;
    std::mt19937 rng(bulkSeed);
    std::uniform_int_distribution<int> gapDistribution(0, 600);
    std::uniform_int_distribution<int> lengthDistribution(0, 960);
    std::uniform_int_distribution<int> phonemeCountDistribution(0, 4);
    std::uniform_int_distribution<int> phonemeStartDistribution(-300, 200);
    std::bernoulli_distribution onsetDistribution(0.3);
    std::bernoulli_distribution restDistribution(0.12);

    QList<Note *> notes;
    fixture.commit([&] {
        fixture.clip->setPosition(2400);
        fixture.clip->setClipStart(360);
        fixture.addTempo(tempo(0, 97.5));
        fixture.addTempo(tempo(4800, 55));
        fixture.addTempo(tempo(12000, 210));
        fixture.addTempo(tempo(24000, 71.25));

        int position = 0;
        for (int i = 0; i < 160; ++i) {
            position += gapDistribution(rng);
            auto *item = fixture.addNote(
                fixture.clip,
                note(position, lengthDistribution(rng),
                     restDistribution(rng) ? QStringLiteral("R") : QStringLiteral("la")));
            notes.push_back(item);
            const auto phonemeCount = phonemeCountDistribution(rng);
            for (int j = 0; j < phonemeCount; ++j) {
                fixture.addPhoneme(item->originalPhonemes(),
                                   phonemeStartDistribution(rng), onsetDistribution(rng));
            }
        }
    });
    fixture.divider.setPaddingBase(83.5);
    fixture.divider.setPaddingAdditional(27.25);
    fixture.divider.setPaddingGap(190.75);
    fixture.divider.setRestLyrics({QStringLiteral("R")});
    fixture.divider.setSingingClip(fixture.clip);
    fixture.divider.update();
    fixture.document.engine()->clearUndoHistory();
    verifyDivision(fixture.divider);

    // One commit changes every note, but the dataset remains deliberately modest.
    fixture.commit([&] {
        std::uniform_int_distribution<int> positionDelta(-400, 400);
        for (int i = 0; i < notes.size(); ++i) {
            auto *item = notes[i];
            item->setPosition(std::max(0, item->position() + positionDelta(rng)));
            item->setLength(lengthDistribution(rng));
            item->setLyric(i % 13 == 0 ? QStringLiteral("R") : QStringLiteral("la"));
            if (auto *firstPhoneme = item->originalPhonemes()->firstItem()) {
                firstPhoneme->setStart(phonemeStartDistribution(rng));
                firstPhoneme->setOnset(onsetDistribution(rng));
            }
        }
        fixture.clip->setPosition(120);
        fixture.clip->setClipStart(1440);
        fixture.model.tempos()->firstItem()->setValue(143.25);
        fixture.divider.setPaddingBase(111.0);
        fixture.divider.setPaddingAdditional(49.5);
        fixture.divider.setPaddingGap(225.0);
    });
    verifyDivision(fixture.divider);

    fixture.commit([&] {
        for (int i = 0; i < 32; ++i) {
            fixture.addNote(fixture.clip,
                            note(gapDistribution(rng) * 20,
                                 lengthDistribution(rng),
                                 i % 7 == 0 ? QStringLiteral("R")
                                            : QStringLiteral("la"),
                                 {phoneme(-100, false), phoneme(0, true)}));
        }
        for (int i = 0; i < notes.size(); i += 10) {
            fixture.clip->notes()->removeItem(notes[i]);
        }
    });
    verifyDivision(fixture.divider);
}

QTEST_GUILESS_MAIN(PieceDivisionTest)

#include "tst_piece_division.moc"
