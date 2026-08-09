dspxmodel_add_qt_test(tst_piece_division
    tst_piece_division.cpp
)

target_link_libraries(tst_piece_division PRIVATE
    dspxmodelPiece
)
