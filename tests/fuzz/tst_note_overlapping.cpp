#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest/QtTest>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelCore/Schema.h>

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/TrackList.h>

#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/Note_p.h>

using namespace dspx;

namespace {

constexpr int initialNoteCount = 100;
constexpr int stepCount = 10000;
constexpr std::uint32_t fuzzSeed = 0x46555A5A;

struct GeneratedNote {
    int keyNumber = 60;
    int position = 0;
    int length = 480;
};

struct NoteInfo {
    Note *note = nullptr;
    int position = 0;
    int length = 0;
    int keyNumber = 0;
};

template <typename Func>
void withTransaction(Document &document, Func &&func) {
    auto transaction = document.engine()->beginTransaction();
    document.setTransaction(&transaction);
    std::invoke(std::forward<Func>(func));
    transaction.commit();
    document.setTransaction(nullptr);
}

int randomGap(std::mt19937 &rng) {
    std::uniform_int_distribution<int> bucketDistribution(0, 99);
    const auto bucket = bucketDistribution(rng);
    if (bucket < 80) {
        return 0;
    }
    if (bucket < 90) {
        std::uniform_int_distribution<int> negativeGapDistribution(-480, 0);
        return negativeGapDistribution(rng);
    }
    std::uniform_int_distribution<int> positiveGapDistribution(0, 480);
    return positiveGapDistribution(rng);
}

std::vector<GeneratedNote> generateNotes(int noteCount, std::mt19937 &rng) {
    std::uniform_int_distribution<int> keyNumberDistribution(48, 72);
    std::uniform_int_distribution<int> lengthDistribution(240, 960);

    std::vector<GeneratedNote> notes;
    notes.reserve(static_cast<std::size_t>(noteCount));

    for (int i = 0; i < noteCount; ++i) {
        GeneratedNote note;
        note.keyNumber = keyNumberDistribution(rng);
        note.length = lengthDistribution(rng);
        if (!notes.empty()) {
            const auto &previousNote = notes.back();
            note.position = qMax(0, previousNote.position + previousNote.length + randomGap(rng));
        }
        notes.push_back(note);
    }

    return notes;
}

std::vector<NoteInfo> collectNotes(SingingClip *clip) {
    std::vector<NoteInfo> result;
    for (auto *note : clip->notes()->asRange()) {
        result.push_back({note, note->position(), note->length(), note->keyNumber()});
    }
    return result;
}

int oracleOverlapCount(const NoteInfo &probe, const std::vector<NoteInfo> &allNotes) {
    int count = 0;
    for (const auto &other : allNotes) {
        if (probe.note == other.note) {
            continue;
        }
        if (probe.position < other.position + other.length
            && other.position < probe.position + probe.length) {
            count++;
        }
    }
    return count;
}

int diniOverlapCount(Note *note) {
    return NotePrivate::get(note)->overlappedCount;
}

void createClipWithNotes(Document &document, Model &model,
                         const std::vector<GeneratedNote> &generatedNotes,
                         SingingClip *&outClip) {
    withTransaction(document, [&] {
        auto *track = model.createTrack();
        model.tracks()->insertItem(0, track);

        outClip = model.createSingingClip();
        outClip->setPosition(0);
        outClip->setClipStart(0);
        track->clips()->insertItem(outClip);

        for (const auto &generatedNote : generatedNotes) {
            auto *note = model.createNote();
            note->setKeyNumber(generatedNote.keyNumber);
            note->setLength(generatedNote.length);
            note->setPosition(generatedNote.position);
            outClip->notes()->insertItem(note);
        }

        if (!generatedNotes.empty()) {
            const auto &lastNote = generatedNotes.back();
            const auto clipLength = lastNote.position + lastNote.length;
            outClip->setLength(clipLength);
            outClip->setClipLength(clipLength);
        }
    });
}

void clearAndRegenerateNotes(Document &document, Model &model, SingingClip *clip,
                             std::mt19937 &rng) {
    auto generatedNotes = generateNotes(initialNoteCount, rng);

    withTransaction(document, [&] {
        QVector<Note *> notes;
        std::ranges::copy(clip->notes()->asRange(), std::back_inserter(notes));
        for (auto it = notes.rbegin(); it != notes.rend(); ++it) {
            clip->notes()->removeItem(*it);
        }

        for (const auto &generatedNote : generatedNotes) {
            auto *note = model.createNote();
            note->setKeyNumber(generatedNote.keyNumber);
            note->setLength(generatedNote.length);
            note->setPosition(generatedNote.position);
            clip->notes()->insertItem(note);
        }

        if (!generatedNotes.empty()) {
            const auto &lastNote = generatedNotes.back();
            const auto clipLength = lastNote.position + lastNote.length;
            clip->setLength(clipLength);
            clip->setClipLength(clipLength);
        }
    });
}

QJsonObject noteToJson(const NoteInfo &info, int oracleCount, int diniCount) {
    return QJsonObject{
        {QStringLiteral("p"), info.position},
        {QStringLiteral("l"), info.length},
        {QStringLiteral("k"), info.keyNumber},
        {QStringLiteral("o0"), oracleCount},
        {QStringLiteral("o"), diniCount},
    };
}

} // anonymous namespace

class NoteOverlappingFuzzTest : public QObject {
    Q_OBJECT

private slots:
    void fuzzTest() {
        std::mt19937 rng(fuzzSeed);

        Document document;
        Model model(&document);

        SingingClip *clip = nullptr;
        auto generatedNotes = generateNotes(initialNoteCount, rng);
        createClipWithNotes(document, model, generatedNotes, clip);

        auto allNotes = collectNotes(clip);

        {
            // Log initial state
            bool stepError = false;
            QJsonArray stepArray;
            for (const auto &noteInfo : allNotes) {
                int oracleCount = oracleOverlapCount(noteInfo, allNotes);
                int diniCount = diniOverlapCount(noteInfo.note);
                if (oracleCount != diniCount) {
                    stepError = true;
                }
                stepArray.append(noteToJson(noteInfo, oracleCount, diniCount));
            }
            qDebug().noquote().nospace() << QJsonDocument(stepArray).toJson(QJsonDocument::Compact);

            if (stepError) {
                QFAIL("Overlap count mismatch detected at initial state; check logs for details.");
            }
        }

        for (int step = 0; step < stepCount; ++step) {
            const int currentSize = clip->notes()->size();

            if (currentSize == 0) {
                clearAndRegenerateNotes(document, model, clip, rng);
            } else {
                std::uniform_int_distribution<int> bucketDistribution(0, 99);
                const auto bucket = bucketDistribution(rng);

                if (bucket < 80) {
                    // 80%: modify a random note's position
                    std::uniform_int_distribution<int> indexDistribution(0, currentSize - 1);
                    const auto targetIndex = indexDistribution(rng);

                    withTransaction(document, [&] {
                        int idx = 0;
                        for (auto *note : clip->notes()->asRange()) {
                            if (idx == targetIndex) {
                                std::uniform_int_distribution<int> deltaDistribution(-480, 480);
                                const auto delta = deltaDistribution(rng);
                                const auto newPosition = qMax(0, note->position() + delta);
                                note->setPosition(newPosition);
                                break;
                            }
                            ++idx;
                        }
                    });
                } else if (bucket < 90) {
                    // 10%: delete a random note
                    std::uniform_int_distribution<int> indexDistribution(0, currentSize - 1);
                    const auto targetIndex = indexDistribution(rng);

                    withTransaction(document, [&] {
                        int idx = 0;
                        for (auto *note : clip->notes()->asRange()) {
                            if (idx == targetIndex) {
                                clip->notes()->removeItem(note);
                                break;
                            }
                            ++idx;
                        }
                    });

                    if (clip->notes()->size() == 0) {
                        clearAndRegenerateNotes(document, model, clip, rng);
                    }
                } else {
                    // 10%: insert a new note between first and last position
                    int minPosition = 0;
                    int maxPosition = 0;
                    {
                        auto *first = clip->notes()->firstItem();
                        auto *last = clip->notes()->lastItem();
                        if (first && last) {
                            minPosition = first->position();
                            maxPosition = last->position();
                        }
                    }

                    std::uniform_int_distribution<int> positionDistribution(minPosition, maxPosition);
                    std::uniform_int_distribution<int> keyNumberDistribution(48, 72);
                    std::uniform_int_distribution<int> lengthDistribution(240, 960);

                    const auto newPosition = positionDistribution(rng);
                    const auto newKeyNumber = keyNumberDistribution(rng);
                    const auto newLength = lengthDistribution(rng);

                    withTransaction(document, [&] {
                        auto *note = model.createNote();
                        note->setPosition(newPosition);
                        note->setKeyNumber(newKeyNumber);
                        note->setLength(newLength);
                        clip->notes()->insertItem(note);
                    });
                }
            }

            allNotes = collectNotes(clip);

            bool stepError = false;
            QJsonArray stepArray;
            for (const auto &noteInfo : allNotes) {
                int oracleCount = oracleOverlapCount(noteInfo, allNotes);
                int diniCount = diniOverlapCount(noteInfo.note);
                if (oracleCount != diniCount) {
                    stepError = true;
                }
                stepArray.append(noteToJson(noteInfo, oracleCount, diniCount));
            }
            qDebug().noquote() << QJsonDocument(stepArray).toJson(QJsonDocument::Compact);

            if (stepError) {
                QFAIL(qPrintable(QString::asprintf("Overlap count mismatch detected at step %d; check logs for details.", step)));
            }
        }
    }
};

QTEST_GUILESS_MAIN(NoteOverlappingFuzzTest)

#include "tst_note_overlapping.moc"
