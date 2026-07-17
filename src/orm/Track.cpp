#include "Track.h"
#include "Track_p.h"

#include <cstdint>
#include <utility>

#include <dini/transaction.h>
#include <opendspx/model.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/TrackList.h>
#include <dspxmodelORM/private/ClipSequence_p.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<Track>> &trackColumnBindings() {
            static const std::vector<orm::ColumnBinding<Track>> bindings {
                orm::intFieldWithSignal<Track, TrackPrivate>(Schema::trackColorIdColumn(), &TrackPrivate::colorId, &Track::colorIdChanged, &Track::colorIdChangedAfterCommit),
                orm::doubleFieldWithSignal<Track, TrackPrivate>(Schema::trackHeightColumn(), &TrackPrivate::height, &Track::heightChanged, &Track::heightChangedAfterCommit),
                orm::stringFieldWithSignal<Track, TrackPrivate>(Schema::trackNameColumn(), &TrackPrivate::name, &Track::nameChanged, &Track::nameChangedAfterCommit),
                orm::doubleFieldWithSignal<Track, TrackPrivate>(Schema::trackGainColumn(), &TrackPrivate::gain, &Track::gainChanged, &Track::gainChangedAfterCommit),
                orm::doubleFieldWithSignal<Track, TrackPrivate>(Schema::trackPanColumn(), &TrackPrivate::pan, &Track::panChanged, &Track::panChangedAfterCommit),
                orm::boolFieldWithSignal<Track, TrackPrivate>(Schema::trackMuteColumn(), &TrackPrivate::mute, &Track::muteChanged, &Track::muteChangedAfterCommit),
                orm::boolFieldWithSignal<Track, TrackPrivate>(Schema::trackSoloColumn(), &TrackPrivate::solo, &Track::soloChanged, &Track::soloChangedAfterCommit),
                orm::boolFieldWithSignal<Track, TrackPrivate>(Schema::trackRecordColumn(), &TrackPrivate::record, &Track::recordChanged, &Track::recordChangedAfterCommit),
                orm::binaryField<Track, TrackPrivate>(Schema::trackWorkspaceColumn(), &TrackPrivate::workspaceData, nullptr, nullptr),
                {Schema::trackParent().column(), [](Track *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = TrackPrivate::get(q);
                     auto *newList = model->isModelValue(value) ? model->tracks : nullptr;
                     const bool changed = d->trackList != newList;
                     d->trackList = newList;
                     return changed;
                 }, [](Track *q) {
                     emit q->trackListChanged(TrackPrivate::get(q)->trackList);
                 }, [](Track *q) {
                     emit q->trackListChangedAfterCommit(TrackPrivate::get(q)->trackList);
                 }},
            };
            return bindings;
        }

    }

    TrackPrivate::TrackPrivate(Track *q) : q_ptr(q) {
    }

    void TrackPrivate::setTrackList(TrackList *newList, bool notify) {
        Q_Q(Track);
        if (trackList == newList) {
            return;
        }
        trackList = newList;
        if (notify) {
            emit q->trackListChanged(trackList);
        }
    }

    dini::ByteArray TrackPrivate::workspace() const {
        return workspaceData;
    }

    void TrackPrivate::setWorkspace(dini::ByteArray workspace) {
        Q_Q(Track);
        ModelPrivate::get(q->model())->update(q->handle(), Schema::trackWorkspaceColumn(), dini::Value(std::move(workspace)));
    }

    Track::Track(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new TrackPrivate(this)) {
        Q_D(Track);
        d->clips = ClipSequencePrivate::create(this);
        ClipSequencePrivate::get(d->clips)->refresh(false);
    }

    Track::~Track() = default;

    int Track::colorId() const {
        Q_D(const Track);
        return d->colorId;
    }

    void Track::setColorId(int colorId) {
        ModelPrivate::get(model())->update(handle(), Schema::trackColorIdColumn(), dini::Value(static_cast<std::int64_t>(colorId)));
    }

    double Track::height() const {
        Q_D(const Track);
        return d->height;
    }

    void Track::setHeight(double height) {
        ModelPrivate::get(model())->update(handle(), Schema::trackHeightColumn(), dini::Value(height));
    }

    QString Track::name() const {
        Q_D(const Track);
        return d->name;
    }

    void Track::setName(const QString &name) {
        ModelPrivate::get(model())->update(handle(), Schema::trackNameColumn(), orm::valueFromString(name));
    }

    double Track::gain() const {
        Q_D(const Track);
        return d->gain;
    }

    void Track::setGain(double gain) {
        ModelPrivate::get(model())->update(handle(), Schema::trackGainColumn(), dini::Value(gain));
    }

    double Track::pan() const {
        Q_D(const Track);
        return d->pan;
    }

    void Track::setPan(double pan) {
        ModelPrivate::get(model())->update(handle(), Schema::trackPanColumn(), dini::Value(pan));
    }

    bool Track::mute() const {
        Q_D(const Track);
        return d->mute;
    }

    void Track::setMute(bool mute) {
        ModelPrivate::get(model())->update(handle(), Schema::trackMuteColumn(), dini::Value(mute));
    }

    bool Track::solo() const {
        Q_D(const Track);
        return d->solo;
    }

    void Track::setSolo(bool solo) {
        ModelPrivate::get(model())->update(handle(), Schema::trackSoloColumn(), dini::Value(solo));
    }

    bool Track::record() const {
        Q_D(const Track);
        return d->record;
    }

    void Track::setRecord(bool record) {
        ModelPrivate::get(model())->update(handle(), Schema::trackRecordColumn(), dini::Value(record));
    }

    ClipSequence *Track::clips() const {
        Q_D(const Track);
        return d->clips;
    }

    TrackList *Track::trackList() const {
        Q_D(const Track);
        return d->trackList;
    }

    opendspx::Track Track::toOpenDSPX() const {
        Q_D(const Track);
        opendspx::Track target;
        target.name = name().toStdString();
        target.control = {
            .gain = conv::toDecibel(gain()),
            .pan = pan(),
            .mute = mute(),
            .solo = solo(),
        };
        target.clips = clips()->toOpenDSPX();
        target.workspace = conv::deserializeWorkspace(d->workspace());
        auto &diffscope = conv::ensureObject(target.workspace["diffscope"]);
        diffscope["colorId"] = colorId();
        diffscope["height"] = height();
        diffscope["record"] = record();
        OpenDSPXConversion::convertTrackToOpenDSPX(this, target);
        return target;
    }

    void Track::fromOpenDSPX(const opendspx::Track &track) {
        Q_D(Track);
        d->setWorkspace(conv::serializeWorkspace(track.workspace));
        setName(QString::fromStdString(track.name));
        setGain(conv::fromDecibel(track.control.gain));
        setPan(track.control.pan);
        setMute(track.control.mute);
        setSolo(track.control.solo);
        clips()->fromOpenDSPX(track.clips);
        if (auto it = track.workspace.find("diffscope"); it != track.workspace.end()) {
            const auto &workspace = it->second;
            if (auto v = conv::optionalChain(workspace, "colorId"); v.is_number_integer()) {
                setColorId(v.get<int>());
            }
            if (auto v = conv::optionalChain(workspace, "height"); v.is_number()) {
                setHeight(v.get<double>());
            }
            if (auto v = conv::optionalChain(workspace, "record"); v.is_boolean()) {
                setRecord(v.get<bool>());
            }
        }
        OpenDSPXConversion::convertTrackFromOpenDSPX(this, track);
    }

    namespace orm {

        void syncTrackColumns(Track *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(trackColumnBindings(), item, snapshot, notify);
        }

        bool applyTrackColumn(Track *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(trackColumnBindings(), item, column, value, notify);
        }

    }

}


#include "moc_Track.cpp"
