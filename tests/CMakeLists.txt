find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)
set(DSPXMODEL_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(dspxmodel_add_qt_test target)
    add_executable(${target}
        ${ARGN}
    )

    target_compile_features(${target} PRIVATE cxx_std_20)

    target_link_libraries(${target} PRIVATE
        dspxmodelCore
        dspxmodelORM
        Qt6::Core
        Qt6::Test
    )

    target_include_directories(${target} PRIVATE
        ${DSPXMODEL_TESTS_DIR}/helpers
    )

    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_subdirectory(orm)
