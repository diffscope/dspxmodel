find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(benchmark CONFIG REQUIRED)

file(GLOB_RECURSE _benchmark_src *.cpp)

if(NOT _benchmark_src)
    return()
endif()

add_executable(dspxmodelBenchmarks
    ${_benchmark_src}
)

target_compile_features(dspxmodelBenchmarks PRIVATE cxx_std_20)

target_link_libraries(dspxmodelBenchmarks PRIVATE
    dspxmodelCore
    dspxmodelORM
    Qt6::Core
    benchmark::benchmark
    benchmark::benchmark_main
)

add_test(
    NAME dspxmodelBenchmarks
    COMMAND $<TARGET_FILE:dspxmodelBenchmarks>
        --benchmark_min_time=0.005s
        --benchmark_repetitions=1
        --benchmark_report_aggregates_only=true
)

set_tests_properties(dspxmodelBenchmarks PROPERTIES
    ENVIRONMENT "PATH=$<TARGET_FILE_DIR:dspxmodelBenchmarks>;$ENV{PATH}"
)
