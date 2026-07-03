#include "Track.h"
#include "Track_p.h"

#include <cstdint>

#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<Track>> &trackColumnBindings() {
            static const std::vector<orm::ColumnBinding<Track>> bindings {
                orm::intField<Track, TrackPrivate>(Schema::trackColorIdColumn(), &TrackPrivate::colorId, &Track::colorIdChanged),
                orm::doubleField<Track, TrackPrivate>(Schema::trackHeightColumn(), &TrackPrivate::height, &Track::heightChanged),
                orm::stringField<Track, TrackPrivate>(Schema::trackNameColumn(), &TrackPrivate::name, &Track::nameChanged),
                orm::doubleField<Track, TrackPrivate>(Schema::trackGainColumn(), &TrackPrivate::gain, &Track::gainChanged),
                orm::doubleField<Track, TrackPrivate>(Schema::trackPanColumn(), &TrackPrivate::pan, &Track::panChanged),
                orm::boolField<Track, TrackPrivate>(Schema::trackMuteColumn(), &TrackPrivate::mute, &Track::muteChanged),
                orm::boolField<Track, TrackPrivate>(Schema::trackSoloColumn(), &TrackPrivate::solo, &Track::soloChanged),
                orm::boolField<Track, TrackPrivate>(Schema::trackRecordColumn(), &TrackPrivate::record, &Track::recordChanged),
                {Schema::trackParent().column(), [](Track *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = TrackPrivate::get(q);
                     auto *newList = model->isModelValue(value) ? model->tracks : nullptr;
                     const bool changed = d->trackList != newList;
                     d->trackList = newList;
                     return changed;
                 }, [](Track *q) {
                     emit q->trackListChanged(TrackPrivate::get(q)->trackList);
                 }},
            };
            return bindings;
        }

    }

    TrackPrivate::TrackPrivate(Track *q) : q_ptr(q) {
    }

    void TrackPrivate::setTrackList(TrackList *newList, bool notify) {
        if (trackList == newList) {
            return;
        }
        trackList = newList;
        if (notify) {
            emit q_func()->trackListChanged(trackList);
        }
    }

    Track::Track(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new TrackPrivate(this)) {
    }

    Track::~Track() = default;

    int Track::colorId() const {
        return TrackPrivate::get(this)->colorId;
    }

    void Track::setColorId(int colorId) {
        ModelPrivate::get(model())->update(handle(), Schema::trackColorIdColumn(), dini::Value(static_cast<std::int64_t>(colorId)));
    }

    double Track::height() const {
        return TrackPrivate::get(this)->height;
    }

    void Track::setHeight(double height) {
        ModelPrivate::get(model())->update(handle(), Schema::trackHeightColumn(), dini::Value(height));
    }

    QString Track::name() const {
        return TrackPrivate::get(this)->name;
    }

    void Track::setName(const QString &name) {
        ModelPrivate::get(model())->update(handle(), Schema::trackNameColumn(), orm::valueFromString(name));
    }

    double Track::gain() const {
        return TrackPrivate::get(this)->gain;
    }

    void Track::setGain(double gain) {
        ModelPrivate::get(model())->update(handle(), Schema::trackGainColumn(), dini::Value(gain));
    }

    double Track::pan() const {
        return TrackPrivate::get(this)->pan;
    }

    void Track::setPan(double pan) {
        ModelPrivate::get(model())->update(handle(), Schema::trackPanColumn(), dini::Value(pan));
    }

    bool Track::mute() const {
        return TrackPrivate::get(this)->mute;
    }

    void Track::setMute(bool mute) {
        ModelPrivate::get(model())->update(handle(), Schema::trackMuteColumn(), dini::Value(mute));
    }

    bool Track::solo() const {
        return TrackPrivate::get(this)->solo;
    }

    void Track::setSolo(bool solo) {
        ModelPrivate::get(model())->update(handle(), Schema::trackSoloColumn(), dini::Value(solo));
    }

    bool Track::record() const {
        return TrackPrivate::get(this)->record;
    }

    void Track::setRecord(bool record) {
        ModelPrivate::get(model())->update(handle(), Schema::trackRecordColumn(), dini::Value(record));
    }

    ClipSequence *Track::clips() const {
        return nullptr;
    }

    TrackList *Track::trackList() const {
        return TrackPrivate::get(this)->trackList;
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

