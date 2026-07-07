project(dspxmodelPropertyMapper
    VERSION ${DSPXMODEL_VERSION}
    LANGUAGES CXX
)

qm_import(Qml)

file(GLOB_RECURSE _src *.h *.cpp)

dspxmodel_add_library(${PROJECT_NAME} AUTOGEN
    SOURCES ${_src}
    LINKS dspxmodelSelectionModel dspxmodelORM dspxmodelCore
    QT_LINKS Core Qml
    QT_INCLUDE_PRIVATE Core Qml
    INCLUDE_PRIVATE *
    PREFIX DSPXMODEL_PROPERTYMAPPER
    FEATURES cxx_std_20
)

if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE _qml_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.qml *.js *.mjs)

qt_add_qml_module(${PROJECT_NAME}
    URI DiffScope.DspxModel.PropertyMapper
    QML_FILES ${_qml_files}
    NAMESPACE dspx
    OUTPUT_DIRECTORY ${QMSETUP_BUILD_DIR}/qml/DiffScope/DspxModel/PropertyMapper
)

qm_install_qml_modules(${PROJECT_NAME})
