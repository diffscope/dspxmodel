dspxmodel_add_qt_test(tst_note_overlapping
    tst_note_overlapping.cpp
)

find_package(Qt6 REQUIRED COMPONENTS Gui)

dspxmodel_add_qt_test(tst_free_value_editing
    tst_free_value_editing.cpp
)

target_link_libraries(tst_free_value_editing PRIVATE Qt6::Gui)

dspxmodel_add_qt_test(tst_note_editing
    tst_note_editing.cpp
)

target_link_libraries(tst_note_editing PRIVATE Qt6::Gui)