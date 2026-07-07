#ifndef DSPXMODEL_SELECTIONMODEL_P_H
#define DSPXMODEL_SELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    class SelectionModelPrivate {
        Q_DECLARE_PUBLIC(SelectionModel)
    public:
        SelectionModel *q_ptr;

        Model *model;

        SelectionModel::SelectionType selectionType{SelectionModel::ST_None};
        AnchorNodeSelectionModel *anchorNodeSelectionModel;
        ClipSelectionModel *clipSelectionModel;
        KeySignatureSelectionModel *keySignatureSelectionModel;
        LabelSelectionModel *labelSelectionModel;
        NoteSelectionModel *noteSelectionModel;
        TempoSelectionModel *tempoSelectionModel;
        TrackSelectionModel *trackSelectionModel;

    };

}

#endif // DSPXMODEL_SELECTIONMODEL_P_H
