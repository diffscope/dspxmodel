#include "ClipPropertyMapper.h"
#include "ClipPropertyMapper_p.h"

#include <dspxmodelORM/Clip.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/NoteSequence.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/ParameterMap.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    ClipPropertyMapper::ClipPropertyMapper(QObject *parent)
        : QObject(parent), d_ptr(new ClipPropertyMapperPrivate) {
        Q_D(ClipPropertyMapper);
        d->q_ptr = this;
    }

    ClipPropertyMapper::~ClipPropertyMapper() = default;

    dspx::SelectionModel *ClipPropertyMapper::selectionModel() const {
        Q_D(const ClipPropertyMapper);
        return d->selectionModel;
    }

    void ClipPropertyMapper::setSelectionModel(dspx::SelectionModel *selectionModel) {
        Q_D(ClipPropertyMapper);
        if (d->selectionModel == selectionModel) {
            return;
        }
        d->setSelectionModel(selectionModel);
        Q_EMIT selectionModelChanged();
    }

    QVariant ClipPropertyMapper::name() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::NameProperty>();
    }

    void ClipPropertyMapper::setName(const QVariant &name) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::NameProperty>(name);
    }

    QVariant ClipPropertyMapper::type() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::TypeProperty>();
    }

    QVariant ClipPropertyMapper::associatedTrack() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::AssociatedTrackProperty>();
    }

    void ClipPropertyMapper::setAssociatedTrack(const QVariant &associatedTrack) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::AssociatedTrackProperty>(associatedTrack);
    }

    QVariant ClipPropertyMapper::mute() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::MuteProperty>();
    }

    void ClipPropertyMapper::setMute(const QVariant &mute) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::MuteProperty>(mute);
    }

    QVariant ClipPropertyMapper::gain() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::GainProperty>();
    }

    void ClipPropertyMapper::setGain(const QVariant &gain) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::GainProperty>(gain);
    }

    QVariant ClipPropertyMapper::pan() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::PanProperty>();
    }

    void ClipPropertyMapper::setPan(const QVariant &pan) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::PanProperty>(pan);
    }

    QVariant ClipPropertyMapper::position() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::PositionProperty>();
    }

    void ClipPropertyMapper::setPosition(const QVariant &position) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::PositionProperty>(position);
    }

    QVariant ClipPropertyMapper::clipStart() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::ClipStartProperty>();
    }

    void ClipPropertyMapper::setClipStart(const QVariant &clipStart) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::ClipStartProperty>(clipStart);
    }

    QVariant ClipPropertyMapper::clipLength() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::ClipLengthProperty>();
    }

    void ClipPropertyMapper::setClipLength(const QVariant &clipLength) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::ClipLengthProperty>(clipLength);
    }

    QVariant ClipPropertyMapper::length() const {
        Q_D(const ClipPropertyMapper);
        return d->value<ClipPropertyMapperPrivate::LengthProperty>();
    }

    void ClipPropertyMapper::setLength(const QVariant &length) {
        Q_D(ClipPropertyMapper);
        d->setValue<ClipPropertyMapperPrivate::LengthProperty>(length);
    }

    void ClipPropertyMapperPrivate::setSelectionModel(dspx::SelectionModel *selectionModel_) {
        if (selectionModel == selectionModel_) {
            return;
        }
        detachSelectionModel();
        selectionModel = selectionModel_;
        attachSelectionModel();
        rebuildFromSelection();
    }

    void ClipPropertyMapperPrivate::attachSelectionModel() {
        Q_Q(ClipPropertyMapper);
        if (!selectionModel) {
            return;
        }

        // Listen to selection type changes
        QObject::connect(selectionModel, &dspx::SelectionModel::selectionTypeChanged, q, [this] {
            rebindSelectionModels();
        });

        rebindSelectionModels();
    }

    void ClipPropertyMapperPrivate::rebindSelectionModels() {
        Q_Q(ClipPropertyMapper);
        if (!selectionModel) {
            return;
        }

        // Disconnect all existing connections
        unbindSelectionModels();

        const auto selectionType = selectionModel->selectionType();

        // Bind clip selection model
        if (selectionType == dspx::SelectionModel::ST_Clip) {
            clipSelectionModel = selectionModel->clipSelectionModel();
            if (clipSelectionModel) {
                QObject::connect(clipSelectionModel, &dspx::ClipSelectionModel::itemSelected, q, [this](dspx::Clip *clip, bool selected) {
                    handleItemSelected(clip, selected);
                });
            }
        }

        // Bind note selection model
        if (selectionType == dspx::SelectionModel::ST_Note) {
            noteSelectionModel = selectionModel->noteSelectionModel();
            if (noteSelectionModel) {
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

    void ClipPropertyMapperPrivate::unbindSelectionModels() {
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

    void ClipPropertyMapperPrivate::detachSelectionModel() {
        resetNoteWatchers();
        resetAnchorWatchers();

        if (selectionModel) {
            QObject::disconnect(selectionModel, nullptr, q_ptr, nullptr);
        }

        unbindSelectionModels();
        clear();

        selectionModel = nullptr;
    }

    void ClipPropertyMapperPrivate::rebuildFromSelection() {
        clear();
        rebuildFromClipSelection();
        rebuildFromNoteSelection();
        rebuildFromAnchorSelection();
        refreshCache();
    }

    void ClipPropertyMapperPrivate::rebuildFromClipSelection() {
        if (!clipSelectionModel) {
            return;
        }
        const auto clips = clipSelectionModel->selectedItems();
        for (auto *clip : clips) {
            addItem(clip);
        }
    }

    dspx::Clip *ClipPropertyMapperPrivate::clipFromNoteSequence(dspx::NoteSequence *noteSequence) const {
        if (!noteSequence) {
            return nullptr;
        }
        return noteSequence->singingClip();
    }

    dspx::Clip *ClipPropertyMapperPrivate::clipFromAnchorNodeSequence(dspx::AnchorNodeSequence *anchorNodeSequence) const {
        if (!anchorNodeSequence) {
            return nullptr;
        }
        if (auto *parameter = anchorNodeSequence->parameter()) {
            if (auto *parameterMap = parameter->parameterMap()) {
                return parameterMap->singingClip();
            }
        }
        return nullptr;
    }

    void ClipPropertyMapperPrivate::rebuildFromNoteSelection() {
        resetNoteWatchers();
        if (!noteSelectionModel) {
            return;
        }
        noteSequenceWithSelectedItems = noteSelectionModel->noteSequenceWithSelectedItems();
        setNoteSequenceWatcher(noteSequenceWithSelectedItems);
        if (auto *clip = clipFromNoteSequence(noteSequenceWithSelectedItems)) {
            addItem(clip);
        }
    }

    void ClipPropertyMapperPrivate::rebuildFromAnchorSelection() {
        resetAnchorWatchers();
        if (!anchorNodeSelectionModel) {
            return;
        }
        anchorNodeSequenceWithSelectedItems = anchorNodeSelectionModel->anchorNodeSequenceWithSelectedItems();
        setAnchorSequenceWatcher(anchorNodeSequenceWithSelectedItems);
        setAnchorParameterWatcher(anchorNodeSequenceWithSelectedItems ? anchorNodeSequenceWithSelectedItems->parameter() : nullptr);
        setAnchorParameterMapWatcher(parameterWithSelectedItems ? parameterWithSelectedItems->parameterMap() : nullptr);
        setAnchorSingingClipWatcher(parameterMapWithSelectedItems ? parameterMapWithSelectedItems->singingClip() : nullptr);
        if (auto *clip = clipFromAnchorNodeSequence(anchorNodeSequenceWithSelectedItems)) {
            addItem(clip);
        }
    }

    void ClipPropertyMapperPrivate::setNoteSequenceWatcher(dspx::NoteSequence *noteSequence) {
        if (!noteSequence) {
            return;
        }
        noteSingingClipWithSelectedItems = noteSequence->singingClip();
    }

    void ClipPropertyMapperPrivate::resetNoteWatchers() {
        if (noteClipSequenceConnection) {
            QObject::disconnect(noteClipSequenceConnection);
        }
        noteClipSequenceConnection = {};
        noteSequenceWithSelectedItems = nullptr;
        noteSingingClipWithSelectedItems = nullptr;
    }

    void ClipPropertyMapperPrivate::setAnchorSequenceWatcher(dspx::AnchorNodeSequence *anchorNodeSequence) {
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

    void ClipPropertyMapperPrivate::setAnchorParameterWatcher(dspx::Parameter *parameter) {
        Q_UNUSED(parameter);
    }

    void ClipPropertyMapperPrivate::setAnchorParameterMapWatcher(dspx::ParameterMap *parameterMap) {
        parameterMapWithSelectedItems = parameterMap;
        if (!parameterMapWithSelectedItems) {
            return;
        }
        anchorSingingClipWithSelectedItems = parameterMapWithSelectedItems->singingClip();
    }

    void ClipPropertyMapperPrivate::setAnchorSingingClipWatcher(dspx::SingingClip *singingClip) {
        anchorSingingClipWithSelectedItems = singingClip;
    }

    void ClipPropertyMapperPrivate::resetAnchorWatchers() {
        if (anchorParameterMapConnection) {
            QObject::disconnect(anchorParameterMapConnection);
        }
        anchorParameterMapConnection = {};
        anchorNodeSequenceWithSelectedItems = nullptr;
        parameterWithSelectedItems = nullptr;
        parameterMapWithSelectedItems = nullptr;
        anchorSingingClipWithSelectedItems = nullptr;
    }
}

#include "moc_ClipPropertyMapper.cpp"
