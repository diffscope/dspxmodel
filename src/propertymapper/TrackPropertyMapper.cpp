#include "TrackPropertyMapper.h"
#include "TrackPropertyMapper_p.h"

#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    TrackPropertyMapper::TrackPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new TrackPropertyMapperPrivate) {
        Q_D(TrackPropertyMapper);
        d->q_ptr = this;
    }

    TrackPropertyMapper::~TrackPropertyMapper() = default;

    dspx::SelectionModel *TrackPropertyMapper::selectionModel() const {
        Q_D(const TrackPropertyMapper);
        return d->selectionModel;
    }

    void TrackPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(TrackPropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant TrackPropertyMapper::name() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::NameProperty>();
    }

    void TrackPropertyMapper::setName(const QVariant &name) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::NameProperty>(name);
    }

    QVariant TrackPropertyMapper::colorId() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::ColorIdProperty>();
    }

    void TrackPropertyMapper::setColorId(const QVariant &colorId) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::ColorIdProperty>(colorId);
    }

    QVariant TrackPropertyMapper::height() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::HeightProperty>();
    }

    void TrackPropertyMapper::setHeight(const QVariant &height) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::HeightProperty>(height);
    }

    QVariant TrackPropertyMapper::mute() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::MuteProperty>();
    }

    void TrackPropertyMapper::setMute(const QVariant &mute) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::MuteProperty>(mute);
    }

    QVariant TrackPropertyMapper::solo() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::SoloProperty>();
    }

    void TrackPropertyMapper::setSolo(const QVariant &solo) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::SoloProperty>(solo);
    }

    QVariant TrackPropertyMapper::record() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::RecordProperty>();
    }

    void TrackPropertyMapper::setRecord(const QVariant &record) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::RecordProperty>(record);
    }

    QVariant TrackPropertyMapper::gain() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::GainProperty>();
    }

    void TrackPropertyMapper::setGain(const QVariant &gain) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::GainProperty>(gain);
    }

    QVariant TrackPropertyMapper::pan() const {
        Q_D(const TrackPropertyMapper);
        return d->value<TrackPropertyMapperPrivate::PanProperty>();
    }

    void TrackPropertyMapper::setPan(const QVariant &pan) {
        Q_D(TrackPropertyMapper);
        d->setValue<TrackPropertyMapperPrivate::PanProperty>(pan);
    }

    void TrackPropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        rebuildFromSelection();
    }

    void TrackPropertyMapperPrivate::attachSelectionModel() {
        Q_Q(TrackPropertyMapper);
        if (!selectionModel) {
            return;
        }

        // Listen to selection type changes
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, q, [this] {
            rebindSelectionModels();
        });

        rebindSelectionModels();
    }

    void TrackPropertyMapperPrivate::rebindSelectionModels() {
        Q_Q(TrackPropertyMapper);
        if (!selectionModel) {
            return;
        }

        // Disconnect all existing connections
        unbindSelectionModels();

        const auto selectionType = selectionModel->selectionType();

        // Bind track selection model
        if (selectionType == dspx::SelectionModel::ST_Track) {
            trackSelectionModel = selectionModel->trackSelectionModel();
            if (trackSelectionModel) {
                QObject::connect(trackSelectionModel, &dspx::TrackSelectionModel::itemSelected, q, [this](dspx::Track *track, bool selected) {
                    handleItemSelected(track, selected);
                });
            }
        }

        // Bind clip selection model
        if (selectionType == dspx::SelectionModel::ST_Clip) {
            clipSelectionModel = selectionModel->clipSelectionModel();
            if (clipSelectionModel) {
                QObject::connect(clipSelectionModel, &dspx::ClipSelectionModel::itemSelected, q, [this](dspx::Clip *clip, bool selected) {
                    Q_UNUSED(selected);
                    Q_UNUSED(clip);
                    rebuildFromSelection();
                });
                QObject::connect(clipSelectionModel, &dspx::ClipSelectionModel::clipSequencesWithSelectedItemsChanged, q, [this] {
                    rebuildFromSelection();
                });
            }
        }

        // Bind note selection model
        if (selectionType == dspx::SelectionModel::ST_Note) {
            noteSelectionModel = selectionModel->noteSelectionModel();
            if (noteSelectionModel) {
                QObject::connect(noteSelectionModel, &dspx::NoteSelectionModel::itemSelected, q, [this](dspx::Note *note, bool selected) {
                    Q_UNUSED(selected);
                    Q_UNUSED(note);
                    rebuildFromSelection();
                });
                QObject::connect(noteSelectionModel, &dspx::NoteSelectionModel::noteSequenceWithSelectedItemsChanged, q, [this] {
                    rebuildFromSelection();
                });
            }
        }

        // Bind anchor node selection model
        if (selectionType == dspx::SelectionModel::ST_AnchorNode) {
            anchorNodeSelectionModel = selectionModel->anchorNodeSelectionModel();
            if (anchorNodeSelectionModel) {
                QObject::connect(anchorNodeSelectionModel, &dspx::AnchorNodeSelectionModel::anchorNodeSequenceWithSelectedItemsChanged, q, [this] {
                    rebuildFromSelection();
                });
            }
        }

        rebuildFromSelection();
    }

    void TrackPropertyMapperPrivate::unbindSelectionModels() {
        if (trackSelectionModel) {
            QObject::disconnect(trackSelectionModel, nullptr, q_ptr, nullptr);
            trackSelectionModel = nullptr;
        }
        if (clipSelectionModel) {
            QObject::disconnect(clipSelectionModel, nullptr, q_ptr, nullptr);
            clipSelectionModel = nullptr;
        }
        if (noteSelectionModel) {
            QObject::disconnect(noteSelectionModel, nullptr, q_ptr, nullptr);
            noteSelectionModel = nullptr;
        }
        if (anchorNodeSelectionModel) {
            QObject::disconnect(anchorNodeSelectionModel, nullptr, q_ptr, nullptr);
            anchorNodeSelectionModel = nullptr;
        }
    }

    void TrackPropertyMapperPrivate::detachSelectionModel() {
        resetNoteWatchers();
        resetAnchorWatchers();

        if (selectionModel) {
            QObject::disconnect(selectionModel, nullptr, q_ptr, nullptr);
        }

        unbindSelectionModels();
        clear();

        selectionModel = nullptr;
    }

    void TrackPropertyMapperPrivate::rebuildFromSelection() {
        clear();
        rebuildFromTrackSelection();
        rebuildFromClipSelection();
        rebuildFromNoteSelection();
        rebuildFromAnchorSelection();
        refreshCache();
    }

    void TrackPropertyMapperPrivate::rebuildFromTrackSelection() {
        if (!trackSelectionModel) {
            return;
        }
        const auto tracks = trackSelectionModel->selectedItems();
        for (auto *track : tracks) {
            addItem(track);
        }
    }

    void TrackPropertyMapperPrivate::rebuildFromClipSelection() {
        if (!clipSelectionModel) {
            return;
        }
        for (auto *clipSequence : clipSelectionModel->clipSequencesWithSelectedItems()) {
            if (!clipSequence) {
                continue;
            }
            if (auto *track = clipSequence->track()) {
                addItem(track);
            }
        }
    }

    dspx::Track *TrackPropertyMapperPrivate::trackFromNoteSequence(dspx::NoteSequence *noteSequence) const {
        if (!noteSequence) {
            return nullptr;
        }
        if (auto *singingClip = noteSequence->singingClip()) {
            if (auto *clipSequence = singingClip->clipSequence()) {
                return clipSequence->track();
            }
        }
        return nullptr;
    }

    dspx::Track *TrackPropertyMapperPrivate::trackFromAnchorNodeSequence(dspx::AnchorNodeSequence *anchorNodeSequence) const {
        if (!anchorNodeSequence) {
            return nullptr;
        }
        if (auto *parameter = anchorNodeSequence->parameter()) {
            if (auto *parameterMap = parameter->parameterMap()) {
                if (auto *singingClip = parameterMap->singingClip()) {
                    if (auto *clipSequence = singingClip->clipSequence()) {
                        return clipSequence->track();
                    }
                }
            }
        }
        return nullptr;
    }

    void TrackPropertyMapperPrivate::rebuildFromNoteSelection() {
        resetNoteWatchers();
        if (!noteSelectionModel) {
            return;
        }
        noteSequenceWithSelectedItems = noteSelectionModel->noteSequenceWithSelectedItems();
        setNoteSequenceWatcher(noteSequenceWithSelectedItems);
        if (auto *track = trackFromNoteSequence(noteSequenceWithSelectedItems)) {
            addItem(track);
        }
    }

    void TrackPropertyMapperPrivate::rebuildFromAnchorSelection() {
        resetAnchorWatchers();
        if (!anchorNodeSelectionModel) {
            return;
        }
        anchorNodeSequenceWithSelectedItems = anchorNodeSelectionModel->anchorNodeSequenceWithSelectedItems();
        setAnchorSequenceWatcher(anchorNodeSequenceWithSelectedItems);
        setAnchorParameterWatcher(anchorNodeSequenceWithSelectedItems ? anchorNodeSequenceWithSelectedItems->parameter() : nullptr);
        setAnchorParameterMapWatcher(parameterWithSelectedItems ? parameterWithSelectedItems->parameterMap() : nullptr);
        setAnchorSingingClipWatcher(parameterMapWithSelectedItems ? parameterMapWithSelectedItems->singingClip() : nullptr);
        if (auto *track = trackFromAnchorNodeSequence(anchorNodeSequenceWithSelectedItems)) {
            addItem(track);
        }
    }

    void TrackPropertyMapperPrivate::setNoteSequenceWatcher(dspx::NoteSequence *noteSequence) {
        if (!noteSequence) {
            return;
        }
        noteSingingClipWithSelectedItems = noteSequence->singingClip();
        if (!noteSingingClipWithSelectedItems) {
            return;
        }
        noteClipSequenceConnection = QObject::connect(noteSingingClipWithSelectedItems, &dspx::Clip::clipSequenceChanged, q_ptr, [this] {
            rebuildFromSelection();
        });
    }

    void TrackPropertyMapperPrivate::resetNoteWatchers() {
        if (noteClipSequenceConnection) {
            QObject::disconnect(noteClipSequenceConnection);
        }
        noteClipSequenceConnection = {};
        noteSequenceWithSelectedItems = nullptr;
        noteSingingClipWithSelectedItems = nullptr;
    }

    void TrackPropertyMapperPrivate::setAnchorSequenceWatcher(dspx::AnchorNodeSequence *anchorNodeSequence) {
        if (!anchorNodeSequence) {
            return;
        }
        parameterWithSelectedItems = anchorNodeSequence->parameter();
        if (parameterWithSelectedItems) {
            anchorParameterMapConnection = QObject::connect(parameterWithSelectedItems, &dspx::Parameter::parameterMapChanged, q_ptr, [this] {
                rebuildFromSelection();
            });
        }
    }

    void TrackPropertyMapperPrivate::setAnchorParameterWatcher(dspx::Parameter *parameter) {
        Q_UNUSED(parameter);
    }

    void TrackPropertyMapperPrivate::setAnchorParameterMapWatcher(dspx::ParameterMap *parameterMap) {
        parameterMapWithSelectedItems = parameterMap;
        if (!parameterMapWithSelectedItems) {
            return;
        }
        anchorSingingClipWithSelectedItems = parameterMapWithSelectedItems->singingClip();
        if (!anchorSingingClipWithSelectedItems) {
            return;
        }
        anchorClipSequenceConnection = QObject::connect(anchorSingingClipWithSelectedItems, &dspx::Clip::clipSequenceChanged, q_ptr, [this] {
            rebuildFromSelection();
        });
    }

    void TrackPropertyMapperPrivate::setAnchorSingingClipWatcher(dspx::SingingClip *singingClip) {
        anchorSingingClipWithSelectedItems = singingClip;
    }

    void TrackPropertyMapperPrivate::resetAnchorWatchers() {
        if (anchorParameterMapConnection) {
            QObject::disconnect(anchorParameterMapConnection);
        }
        if (anchorClipSequenceConnection) {
            QObject::disconnect(anchorClipSequenceConnection);
        }
        anchorParameterMapConnection = {};
        anchorClipSequenceConnection = {};
        anchorNodeSequenceWithSelectedItems = nullptr;
        parameterWithSelectedItems = nullptr;
        parameterMapWithSelectedItems = nullptr;
        anchorSingingClipWithSelectedItems = nullptr;
    }
}

#include "moc_TrackPropertyMapper.cpp"
