#ifndef DSPXMODEL_TRACKPROPERTYMAPPER_P_H
#define DSPXMODEL_TRACKPROPERTYMAPPER_P_H

#include "TrackPropertyMapper.h"

#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/DynamicMixingAnchorSequence.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelSelectionModel/DynamicMixingAnchorSelectionModel.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelSelectionModel/TrackSelectionModel.h>

#include "PropertyMapperData_p.h"

namespace dspx {
    class SelectionModel;
    class TrackPropertyMapperPrivate : public PropertyMapperData<
        TrackPropertyMapper,
        TrackPropertyMapperPrivate,
        dspx::Track,
        PropertyMetadata<dspx::Track, &dspx::Track::name, &dspx::Track::setName, decltype(&dspx::Track::nameChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::colorId, &dspx::Track::setColorId, decltype(&dspx::Track::colorIdChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::height, &dspx::Track::setHeight, decltype(&dspx::Track::heightChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::mute, &dspx::Track::setMute, decltype(&dspx::Track::muteChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::solo, &dspx::Track::setSolo, decltype(&dspx::Track::soloChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::record, &dspx::Track::setRecord, decltype(&dspx::Track::recordChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::gain, &dspx::Track::setGain, decltype(&dspx::Track::gainChanged)>,
        PropertyMetadata<dspx::Track, &dspx::Track::pan, &dspx::Track::setPan, decltype(&dspx::Track::panChanged)>
    > {
        Q_DECLARE_PUBLIC(TrackPropertyMapper)
    public:
        TrackPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Track::nameChanged},
            {&dspx::Track::colorIdChanged},
            {&dspx::Track::heightChanged},
            {&dspx::Track::muteChanged},
            {&dspx::Track::soloChanged},
            {&dspx::Track::recordChanged},
            {&dspx::Track::gainChanged},
            {&dspx::Track::panChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::TrackSelectionModel *trackSelectionModel = nullptr;
        dspx::ClipSelectionModel *clipSelectionModel = nullptr;
        dspx::NoteSelectionModel *noteSelectionModel = nullptr;
        dspx::AnchorNodeSelectionModel *anchorNodeSelectionModel = nullptr;
        dspx::DynamicMixingAnchorSelectionModel *dynamicMixingAnchorSelectionModel = nullptr;

        dspx::NoteSequence *noteSequenceWithSelectedItems = nullptr;
        dspx::AnchorNodeSequence *anchorNodeSequenceWithSelectedItems = nullptr;
        dspx::Parameter *parameterWithSelectedItems = nullptr;
        dspx::ParameterMap *parameterMapWithSelectedItems = nullptr;
        dspx::SingingClip *noteSingingClipWithSelectedItems = nullptr;
        dspx::SingingClip *anchorSingingClipWithSelectedItems = nullptr;
        dspx::DynamicMixingAnchorSequence *dynamicMixingAnchorSequenceWithSelectedItems = nullptr;
        dspx::Sources *dynamicMixingSourcesWithSelectedItems = nullptr;
        dspx::SingingClip *dynamicMixingSingingClipWithSelectedItems = nullptr;

        QMetaObject::Connection noteClipSequenceConnection;
        QMetaObject::Connection anchorParameterMapConnection;
        QMetaObject::Connection anchorClipSequenceConnection;
        QMetaObject::Connection dynamicMixingSingingClipConnection;
        QMetaObject::Connection dynamicMixingClipSequenceConnection;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();
        void rebindSelectionModels();
        void unbindSelectionModels();

        void rebuildFromSelection();
        void rebuildFromTrackSelection();
        void rebuildFromClipSelection();
        void rebuildFromNoteSelection();
        void rebuildFromAnchorSelection();
        void rebuildFromDynamicMixingSelection();

        void setNoteSequenceWatcher(dspx::NoteSequence *noteSequence);
        void resetNoteWatchers();

        void setAnchorSequenceWatcher(dspx::AnchorNodeSequence *anchorNodeSequence);
        void setAnchorParameterWatcher(dspx::Parameter *parameter);
        void setAnchorParameterMapWatcher(dspx::ParameterMap *parameterMap);
        void setAnchorSingingClipWatcher(dspx::SingingClip *singingClip);
        void resetAnchorWatchers();
        void resetDynamicMixingWatchers();

        dspx::Track *trackFromNoteSequence(dspx::NoteSequence *noteSequence) const;
        dspx::Track *trackFromAnchorNodeSequence(dspx::AnchorNodeSequence *anchorNodeSequence) const;
        dspx::Track *trackFromDynamicMixingAnchorSequence(dspx::DynamicMixingAnchorSequence *sequence) const;

        enum {
            NameProperty = 0,
            ColorIdProperty = 1,
            HeightProperty = 2,
            MuteProperty = 3,
            SoloProperty = 4,
            RecordProperty = 5,
            GainProperty = 6,
            PanProperty = 7
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(TrackPropertyMapper);
            if constexpr (i == NameProperty) {
                q->nameChanged();
            } else if constexpr (i == ColorIdProperty) {
                q->colorIdChanged();
            } else if constexpr (i == HeightProperty) {
                q->heightChanged();
            } else if constexpr (i == MuteProperty) {
                q->muteChanged();
            } else if constexpr (i == SoloProperty) {
                q->soloChanged();
            } else if constexpr (i == RecordProperty) {
                q->recordChanged();
            } else if constexpr (i == GainProperty) {
                q->gainChanged();
            } else if constexpr (i == PanProperty) {
                q->panChanged();
            }
        }
    };
}

#endif // DSPXMODEL_TRACKPROPERTYMAPPER_P_H
