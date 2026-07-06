project(dspxmodelORM
    VERSION ${DSPXMODEL_VERSION}
    LANGUAGES CXX
)

file(GLOB_RECURSE _src *.h *.cpp)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(opendspx CONFIG REQUIRED)

dspxmodel_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS dspxmodelCore opendspx::opendspxmodel
    QT_LINKS Core
    QT_INCLUDE_PRIVATE Core
    INCLUDE_PRIVATE *
    PREFIX DSPXMODEL_ORM
    FEATURES cxx_std_20
)
