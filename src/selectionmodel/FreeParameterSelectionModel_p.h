#ifndef DSPXMODEL_FREEPARAMETERSELECTIONMODEL_P_H
#define DSPXMODEL_FREEPARAMETERSELECTIONMODEL_P_H

#include <dspxmodelSelectionModel/FreeParameterSelectionModel.h>

#include <QMetaObject>

namespace dspx {

    class Track;

    class FreeParameterSelectionModelPrivate {
        Q_DECLARE_PUBLIC(FreeParameterSelectionModel)

    public:
        struct State {
            SingingClip *singingClip = nullptr;
            QString parameterId;
            FreeValueDataArray::FreeValueRole role = FreeValueDataArray::Edited;
            int start = 0;
            int end = 0;
        };

        FreeParameterSelectionModel *q_ptr = nullptr;
        SelectionModel *selectionModel = nullptr;
        State state;
        QMetaObject::Connection clipDestroyedConnection;
        QMetaObject::Connection clipSequenceChangedConnection;
        QMetaObject::Connection trackListChangedConnection;

        Track *attachedTrack(SingingClip *clip) const;
        void connectTrack(Track *track);
        void setContext(SingingClip *clip, const QString &parameterId, FreeValueDataArray::FreeValueRole role);
        void notifyChanges(const State &previous);
    };

}

#endif // DSPXMODEL_FREEPARAMETERSELECTIONMODEL_P_H
