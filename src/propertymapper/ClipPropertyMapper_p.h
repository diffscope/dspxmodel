#ifndef DSPXMODEL_CLIPPROPERTYMAPPER_P_H
#define DSPXMODEL_CLIPPROPERTYMAPPER_P_H

#include "ClipPropertyMapper.h"

#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelSelectionModel/AnchorNodeSelectionModel.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelSelectionModel/ClipSelectionModel.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelSelectionModel/NoteSelectionModel.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>

#include "PropertyMapperData_p.h"

namespace dspx {
    class SelectionModel;
    class ClipPropertyMapperPrivate : public PropertyMapperData<
        ClipPropertyMapper,
        ClipPropertyMapperPrivate,
        dspx::Clip,
        PropertyMetadata<dspx::Clip, &dspx::Clip::name, &dspx::Clip::setName, decltype(&dspx::Clip::nameChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::type, nullptr, std::nullptr_t>,
        PropertyMetadata<dspx::Clip,
            [](const dspx::Clip *clip) { return clip->clipSequence() ? clip->clipSequence()->track() : nullptr; },
            [](dspx::Clip *clip, dspx::Track *track) { if (track && clip->clipSequence()) clip->clipSequence()->moveItem(clip, track->clips()); },
            decltype(&dspx::Clip::clipSequenceChanged)
        >,
        PropertyMetadata<dspx::Clip, &dspx::Clip::mute, &dspx::Clip::setMute, decltype(&dspx::Clip::muteChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::gain, &dspx::Clip::setGain, decltype(&dspx::Clip::gainChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::pan, &dspx::Clip::setPan, decltype(&dspx::Clip::panChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::position, &dspx::Clip::setPosition, decltype(&dspx::Clip::positionChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::clipStart, &dspx::Clip::setClipStart, decltype(&dspx::Clip::clipStartChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::clipLength, &dspx::Clip::setClipLength, decltype(&dspx::Clip::clipLengthChanged)>,
        PropertyMetadata<dspx::Clip, &dspx::Clip::length, &dspx::Clip::setLength, decltype(&dspx::Clip::lengthChanged)>
    > {
        Q_DECLARE_PUBLIC(ClipPropertyMapper)
    public:
        ClipPropertyMapperPrivate() : PropertyMapperData(
            {&dspx::Clip::nameChanged},
            {nullptr},
            {&dspx::Clip::clipSequenceChanged},
            {&dspx::Clip::muteChanged},
            {&dspx::Clip::gainChanged},
            {&dspx::Clip::panChanged},
            {&dspx::Clip::positionChanged},
            {&dspx::Clip::clipStartChanged},
            {&dspx::Clip::clipLengthChanged},
            {&dspx::Clip::lengthChanged}
        ) {}

        dspx::SelectionModel *selectionModel = nullptr;
        dspx::ClipSelectionModel *clipSelectionModel = nullptr;
        dspx::NoteSelectionModel *noteSelectionModel = nullptr;
        dspx::AnchorNodeSelectionModel *anchorNodeSelectionModel = nullptr;

        dspx::NoteSequence *noteSequenceWithSelectedItems = nullptr;
        dspx::AnchorNodeSequence *anchorNodeSequenceWithSelectedItems = nullptr;
        dspx::Parameter *parameterWithSelectedItems = nullptr;
        dspx::ParameterMap *parameterMapWithSelectedItems = nullptr;
        dspx::SingingClip *noteSingingClipWithSelectedItems = nullptr;
        dspx::SingingClip *anchorSingingClipWithSelectedItems = nullptr;

        QMetaObject::Connection noteClipSequenceConnection;
        QMetaObject::Connection anchorParameterMapConnection;

        void setSelectionModel(dspx::SelectionModel *selectionModel_);
        void attachSelectionModel();
        void detachSelectionModel();
        void rebindSelectionModels();
        void unbindSelectionModels();

        void rebuildFromSelection();
        void rebuildFromClipSelection();
        void rebuildFromNoteSelection();
        void rebuildFromAnchorSelection();

        void setNoteSequenceWatcher(dspx::NoteSequence *noteSequence);
        void resetNoteWatchers();

        void setAnchorSequenceWatcher(dspx::AnchorNodeSequence *anchorNodeSequence);
        void setAnchorParameterWatcher(dspx::Parameter *parameter);
        void setAnchorParameterMapWatcher(dspx::ParameterMap *parameterMap);
        void setAnchorSingingClipWatcher(dspx::SingingClip *singingClip);
        void resetAnchorWatchers();

        dspx::Clip *clipFromNoteSequence(dspx::NoteSequence *noteSequence) const;
        dspx::Clip *clipFromAnchorNodeSequence(dspx::AnchorNodeSequence *anchorNodeSequence) const;

        enum {
            NameProperty = 0,
            TypeProperty = 1,
            AssociatedTrackProperty = 2,
            MuteProperty = 3,
            GainProperty = 4,
            PanProperty = 5,
            PositionProperty = 6,
            ClipStartProperty = 7,
            ClipLengthProperty = 8,
            LengthProperty = 9
        };

        template<int i>
        void notifyValueChange() {
            Q_Q(ClipPropertyMapper);
            if constexpr (i == NameProperty) {
                q->nameChanged();
            } else if constexpr (i == TypeProperty) {
                q->typeChanged();
            } else if constexpr (i == AssociatedTrackProperty) {
                q->associatedTrackChanged();
            } else if constexpr (i == MuteProperty) {
                q->muteChanged();
            } else if constexpr (i == GainProperty) {
                q->gainChanged();
            } else if constexpr (i == PanProperty) {
                q->panChanged();
            } else if constexpr (i == PositionProperty) {
                q->positionChanged();
            } else if constexpr (i == ClipStartProperty) {
                q->clipStartChanged();
            } else if constexpr (i == ClipLengthProperty) {
                q->clipLengthChanged();
            } else if constexpr (i == LengthProperty) {
                q->lengthChanged();
            }
        }
    };
}

#endif // DSPXMODEL_CLIPPROPERTYMAPPER_P_H
