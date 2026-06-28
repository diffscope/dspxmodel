project(dspxmodelCore
    VERSION ${DSPXMODEL_VERSION}
    LANGUAGES CXX
)

file(GLOB_RECURSE _src *.h *.cpp)

dspxmodel_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    QT_LINKS Core
    QT_INCLUDE_PRIVATE Core
    INCLUDE_PRIVATE *
    PREFIX DSPXMODEL_CORE
    FEATURES cxx_std_20
)
