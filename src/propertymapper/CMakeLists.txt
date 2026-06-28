project(dspxmodelPropertyMapper
    VERSION ${DSPXMODEL_VERSION}
    LANGUAGES CXX
)

file(GLOB_RECURSE _src *.h *.cpp)

dspxmodel_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS dspxmodelSelectionModel dspxmodelORM dspxmodelCore
    QT_LINKS Core
    QT_INCLUDE_PRIVATE Core
    INCLUDE_PRIVATE *
    PREFIX DSPXMODEL_PROPERTYMAPPER
    FEATURES cxx_std_20
)
