#include "FreeParameterSelectionModel.h"
#include "FreeParameterSelectionModel_p.h"

#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelSelectionModel/SelectionModel.h>

namespace dspx {

    Track *FreeParameterSelectionModelPrivate::attachedTrack(SingingClip *clip) const {
        if (!clip || clip->model() != selectionModel->model()) {
            return nullptr;
        }
        auto *sequence = clip->clipSequence();
        auto *track = sequence ? sequence->track() : nullptr;
        return track && track->trackList() == selectionModel->model()->tracks() ? track : nullptr;
    }

    void FreeParameterSelectionModelPrivate::connectTrack(Track *track) {
        QObject::disconnect(trackListChangedConnection);
        trackListChangedConnection = QObject::connect(track, &Track::trackListChanged, q_ptr, [this] {
            if (!attachedTrack(state.singingClip)) {
                selectionModel->clearFreeParameterContext();
            }
        });
    }

    void FreeParameterSelectionModelPrivate::setContext(SingingClip *clip, const QString &parameterId, FreeValueDataArray::FreeValueRole role) {
        if (state.singingClip == clip && state.parameterId == parameterId && state.role == role) {
            return;
        }
        auto *previousClip = state.singingClip;
        state = {clip, parameterId, role};
        if (previousClip == clip) {
            return;
        }
        QObject::disconnect(clipDestroyedConnection);
        QObject::disconnect(clipSequenceChangedConnection);
        if (!clip) {
            QObject::disconnect(trackListChangedConnection);
            return;
        }
        clipDestroyedConnection = QObject::connect(clip, &QObject::destroyed, q_ptr, [this] {
            selectionModel->clearFreeParameterContext();
        });
        clipSequenceChangedConnection = QObject::connect(clip, &Clip::clipSequenceChanged, q_ptr, [this] {
            if (auto *track = attachedTrack(state.singingClip)) {
                connectTrack(track);
            } else {
                selectionModel->clearFreeParameterContext();
            }
        });
        connectTrack(attachedTrack(clip));
    }

    void FreeParameterSelectionModelPrivate::notifyChanges(const State &previous) {
        if (previous.singingClip != state.singingClip || previous.parameterId != state.parameterId || previous.role != state.role) {
            Q_EMIT q_ptr->contextChanged();
        }
        if (previous.start != state.start || previous.end != state.end) {
            Q_EMIT q_ptr->rangeChanged();
        }
        if ((previous.start < previous.end) != q_ptr->hasSelection()) {
            Q_EMIT q_ptr->hasSelectionChanged();
        }
    }

    FreeParameterSelectionModel::FreeParameterSelectionModel(SelectionModel *parent)
        : QObject(parent), d_ptr(new FreeParameterSelectionModelPrivate) {
        Q_D(FreeParameterSelectionModel);
        d->q_ptr = this;
        d->selectionModel = parent;
    }

    FreeParameterSelectionModel::~FreeParameterSelectionModel() = default;

    SingingClip *FreeParameterSelectionModel::singingClip() const {
        Q_D(const FreeParameterSelectionModel);
        return d->state.singingClip;
    }

    QString FreeParameterSelectionModel::parameterId() const {
        Q_D(const FreeParameterSelectionModel);
        return d->state.parameterId;
    }

    FreeValueDataArray::FreeValueRole FreeParameterSelectionModel::role() const {
        Q_D(const FreeParameterSelectionModel);
        return d->state.role;
    }

    int FreeParameterSelectionModel::start() const {
        Q_D(const FreeParameterSelectionModel);
        return d->state.start;
    }

    int FreeParameterSelectionModel::end() const {
        Q_D(const FreeParameterSelectionModel);
        return d->state.end;
    }

    bool FreeParameterSelectionModel::hasSelection() const {
        Q_D(const FreeParameterSelectionModel);
        return d->state.start < d->state.end;
    }

}

#include "moc_FreeParameterSelectionModel.cpp"
