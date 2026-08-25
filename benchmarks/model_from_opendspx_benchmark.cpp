#include <cstdint>
#include <sstream>
#include <string>

#include <benchmark/benchmark.h>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <opendspx/serializer/serializer.h>

#include <dspxmodelCore/Document.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/TrackList.h>

namespace {

constexpr std::uint8_t bigDspxData[] = {
#include "big_dspx.inc"
};

void BM_ModelFromOpenDspx(benchmark::State &state) {
    std::istringstream input(
        std::string(reinterpret_cast<const char *>(bigDspxData), sizeof(bigDspxData)),
        std::ios::binary | std::ios::in);
    opendspx::SerializationErrorList errors;
    const auto openDspxModel = opendspx::Serializer::deserialize(input, errors);

    if (!errors.empty()) {
        state.SkipWithError("Failed to deserialize big_dspx.inc");
        return;
    }

    for (auto _ : state) {
        state.PauseTiming();
        {
            dspx::Document document;
            dspx::Model model(&document);
            auto transaction = document.engine()->beginTransaction();
            document.setTransaction(&transaction);

            state.ResumeTiming();
            model.fromOpenDspx(openDspxModel);
            state.PauseTiming();

            benchmark::DoNotOptimize(model.tracks()->size());
            benchmark::ClobberMemory();

            transaction.commit();
            document.setTransaction(nullptr);
        }
        state.ResumeTiming();
    }
}

BENCHMARK(BM_ModelFromOpenDspx);

} // namespace
