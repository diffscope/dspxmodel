dspxmodel_add_qt_test(tst_propertymapper
    tst_propertymapper.cpp
)

target_link_libraries(tst_propertymapper PRIVATE
    dspxmodelPropertyMapper
)
