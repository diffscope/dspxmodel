#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

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

#include <dspxmodelORM/Tempo.h>
#include <dspxmodelORM/TempoSequence.h>

namespace {

using namespace dspx;

constexpr std::uint32_t noteSeed = 0x44535058;

struct GeneratedNote {
    int keyNumber = 60;
    int position = 0;
    int length = 480;
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

std::vector<GeneratedNote> generateNotes(int noteCount) {
    std::mt19937 rng(noteSeed);
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

SingingClip *createSingingClipWithNotes(Document &document, Model &model, const std::vector<GeneratedNote> &generatedNotes) {
    SingingClip *clip = nullptr;
    withTransaction(document, [&] {
        auto *track = model.createTrack();
        model.tracks()->insertItem(0, track);

        clip = model.createSingingClip();
        clip->setPosition(0);
        clip->setClipStart(0);
        track->clips()->insertItem(clip);

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

    return clip;
}

void buildModelWithSingingClipAndNotes(const std::vector<GeneratedNote> &generatedNotes) {
    Document document;
    Model model(&document);

    createSingingClipWithNotes(document, model, generatedNotes);

    benchmark::DoNotOptimize(model.tracks()->size());
    benchmark::ClobberMemory();
}

void addNoteCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_CreateModelWithSingingClipAndNotes(benchmark::State &state) {
    const auto noteCount = static_cast<int>(state.range(0));
    const auto generatedNotes = generateNotes(noteCount);
    for (auto _ : state) {
        buildModelWithSingingClipAndNotes(generatedNotes);
    }
    state.SetItemsProcessed(state.iterations() * noteCount);
}

BENCHMARK(BM_CreateModelWithSingingClipAndNotes)->Apply(addNoteCounts);

void shiftAllNotePositions(Document &document, SingingClip *clip, int offset) {
    withTransaction(document, [&] {
        QVector<Note *> notes;
        std::ranges::copy(clip->notes()->asRange(), std::back_inserter(notes));
        for (auto *note : notes) {
            note->setPosition(note->position() + offset);
        }
    });
}

void shiftNoteCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_ShiftAllNotePositions(benchmark::State &state) {
    const auto noteCount = static_cast<int>(state.range(0));
    const auto generatedNotes = generateNotes(noteCount);
    for (auto _ : state) {
        state.PauseTiming();
        Document document;
        Model model(&document);
        auto *clip = createSingingClipWithNotes(document, model, generatedNotes);
        state.ResumeTiming();

        shiftAllNotePositions(document, clip, 240);

        benchmark::DoNotOptimize(clip->notes()->size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * noteCount);
}

BENCHMARK(BM_ShiftAllNotePositions)->Apply(shiftNoteCounts);

void removeAllNotes(Document &document, SingingClip *clip) {
    withTransaction(document, [&] {
        QVector<Note *> notes;
        std::ranges::copy(clip->notes()->asRange(), std::back_inserter(notes));
        for (auto it = notes.rbegin(); it != notes.rend(); ++it) {
            clip->notes()->removeItem(*it);
        }
    });
}

void removeNoteCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_RemoveAllNotes(benchmark::State &state) {
    const auto noteCount = static_cast<int>(state.range(0));
    const auto generatedNotes = generateNotes(noteCount);
    for (auto _ : state) {
        state.PauseTiming();
        Document document;
        Model model(&document);
        auto *clip = createSingingClipWithNotes(document, model, generatedNotes);
        state.ResumeTiming();

        removeAllNotes(document, clip);

        benchmark::DoNotOptimize(clip->notes()->size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * noteCount);
}

BENCHMARK(BM_RemoveAllNotes)->Apply(removeNoteCounts);

void createTracks(Document &document, Model &model, int trackCount) {
    withTransaction(document, [&] {
        for (int i = 0; i < trackCount; ++i) {
            auto *track = model.createTrack();
            model.tracks()->insertItem(model.tracks()->size(), track);
        }
    });
}

void buildModelWithTracks(int trackCount) {
    Document document;
    Model model(&document);

    createTracks(document, model, trackCount);

    benchmark::DoNotOptimize(model.tracks()->size());
    benchmark::ClobberMemory();
}

void addTrackCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_CreateModelWithTracks(benchmark::State &state) {
    const auto trackCount = static_cast<int>(state.range(0));
    for (auto _ : state) {
        buildModelWithTracks(trackCount);
    }
    state.SetItemsProcessed(state.iterations() * trackCount);
}

BENCHMARK(BM_CreateModelWithTracks)->Apply(addTrackCounts);

void removeAllTracks(Document &document, Model &model) {
    withTransaction(document, [&] {
        while (model.tracks()->size() > 0) {
            model.tracks()->removeItem(model.tracks()->size() - 1);
        }
    });
}

void removeTrackCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_RemoveAllTracksFromEnd(benchmark::State &state) {
    const auto trackCount = static_cast<int>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        Document document;
        Model model(&document);
        createTracks(document, model, trackCount);
        state.ResumeTiming();

        removeAllTracks(document, model);

        benchmark::DoNotOptimize(model.tracks()->size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * trackCount);
}

BENCHMARK(BM_RemoveAllTracksFromEnd)->Apply(removeTrackCounts);

constexpr double defaultTempoValue = 120.0;

void createTempos(Document &document, Model &model, int tempoCount) {
    withTransaction(document, [&] {
        for (int i = 0; i < tempoCount; ++i) {
            auto *tempo = model.createTempo();
            tempo->setPosition(i);
            tempo->setValue(defaultTempoValue);
            model.tempos()->insertItem(tempo);
        }
    });
}

void buildModelWithTempos(int tempoCount) {
    Document document;
    Model model(&document);

    createTempos(document, model, tempoCount);

    benchmark::DoNotOptimize(model.tempos()->size());
    benchmark::ClobberMemory();
}

void addTempoCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_CreateModelWithTempos(benchmark::State &state) {
    const auto tempoCount = static_cast<int>(state.range(0));
    for (auto _ : state) {
        buildModelWithTempos(tempoCount);
    }
    state.SetItemsProcessed(state.iterations() * tempoCount);
}

BENCHMARK(BM_CreateModelWithTempos)->Apply(addTempoCounts);

void shiftAllTempoPositions(Document &document, Model &model, int offset) {
    withTransaction(document, [&] {
        QVector<Tempo *> tempos;
        std::ranges::copy(model.tempos()->asRange(), std::back_inserter(tempos));
        for (auto it = tempos.rbegin(); it != tempos.rend(); ++it) {
            auto *tempo = *it;
            tempo->setPosition(tempo->position() + offset);
        }
    });
}

void shiftTempoCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_ShiftAllTempoPositions(benchmark::State &state) {
    const auto tempoCount = static_cast<int>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        Document document;
        Model model(&document);
        createTempos(document, model, tempoCount);
        state.ResumeTiming();

        shiftAllTempoPositions(document, model, 240);

        benchmark::DoNotOptimize(model.tempos()->size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * tempoCount);
}

BENCHMARK(BM_ShiftAllTempoPositions)->Apply(shiftTempoCounts);

void removeAllTempos(Document &document, Model &model) {
    withTransaction(document, [&] {
        QVector<Tempo *> tempos;
        std::ranges::copy(model.tempos()->asRange(), std::back_inserter(tempos));
        for (auto it = tempos.rbegin(); it != tempos.rend(); ++it) {
            model.tempos()->removeItem(*it);
        }
    });
}

void removeTempoCounts(benchmark::internal::Benchmark *benchmark) {
    benchmark->Arg(10)->Arg(20)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)->Arg(2000);
}

void BM_RemoveAllTempos(benchmark::State &state) {
    const auto tempoCount = static_cast<int>(state.range(0));
    for (auto _ : state) {
        state.PauseTiming();
        Document document;
        Model model(&document);
        createTempos(document, model, tempoCount);
        state.ResumeTiming();

        removeAllTempos(document, model);

        benchmark::DoNotOptimize(model.tempos()->size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * tempoCount);
}

BENCHMARK(BM_RemoveAllTempos)->Apply(removeTempoCounts);

} // namespace
