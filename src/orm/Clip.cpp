#include "Clip.h"
#include "Clip_p.h"

#include <cstdint>
#include <memory>
#include <utility>

#include <dini/transaction.h>
#include <opendspx/audioclip.h>
#include <opendspx/clip.h>
#include <opendspx/singingclip.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/ClipSequence.h>
#include <dspxmodelORM/OpenDSPXConversion.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/private/AudioClip_p.h>
#include <dspxmodelORM/private/ClipSequence_p.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Track_p.h>

namespace dspx {

    namespace {

        ClipSequence *clipOwnerFromValue(ModelPrivate &model, const dini::Value &value) {
            const auto trackHandle = orm::handleFromValue(value);
            auto *track = trackHandle ? model.ensure<Track>(trackHandle) : nullptr;
            return track ? track->clips() : nullptr;
        }

        const std::vector<orm::ColumnBinding<Clip>> &clipColumnBindings() {
            static const std::vector<orm::ColumnBinding<Clip>> bindings {
                orm::stringFieldWithSignal<Clip, ClipPrivate>(Schema::clipNameColumn(), &ClipPrivate::name, &Clip::nameChanged, &Clip::nameChangedAfterCommit),
                orm::doubleFieldWithSignal<Clip, ClipPrivate>(Schema::clipGainColumn(), &ClipPrivate::gain, &Clip::gainChanged, &Clip::gainChangedAfterCommit),
                orm::doubleFieldWithSignal<Clip, ClipPrivate>(Schema::clipPanColumn(), &ClipPrivate::pan, &Clip::panChanged, &Clip::panChangedAfterCommit),
                orm::boolFieldWithSignal<Clip, ClipPrivate>(Schema::clipMuteColumn(), &ClipPrivate::mute, &Clip::muteChanged, &Clip::muteChangedAfterCommit),
                orm::intField<Clip, ClipPrivate>(Schema::clipPositionColumn(), &ClipPrivate::position, [](Clip *q) {
                    auto *d = ClipPrivate::get(q);
                    emit q->positionChanged(d->position);
                    emit q->startChanged(q->start());
                }, [](Clip *q) {
                    emit q->positionChangedAfterCommit(ClipPrivate::get(q)->position);
                    emit q->startChangedAfterCommit(q->start());
                }),
                orm::intFieldWithSignal<Clip, ClipPrivate>(Schema::clipLengthColumn(), &ClipPrivate::length, &Clip::lengthChanged, &Clip::lengthChangedAfterCommit),
                orm::intField<Clip, ClipPrivate>(Schema::clipClipStartColumn(), &ClipPrivate::clipStart, [](Clip *q) {
                    auto *d = ClipPrivate::get(q);
                    emit q->clipStartChanged(d->clipStart);
                    emit q->startChanged(q->start());
                }, [](Clip *q) {
                    emit q->clipStartChangedAfterCommit(ClipPrivate::get(q)->clipStart);
                    emit q->startChangedAfterCommit(q->start());
                }),
                orm::intFieldWithSignal<Clip, ClipPrivate>(Schema::clipClipLengthColumn(), &ClipPrivate::clipLength, &Clip::clipLengthChanged, &Clip::clipLengthChangedAfterCommit),
                orm::previousNextFieldWithSignal<Clip, ClipPrivate>(Schema::clipPreviousItemColumn(), &ClipPrivate::previousHandle, &ClipPrivate::previous, &Clip::previousItemChanged, &Clip::previousItemChangedAfterCommit),
                orm::previousNextFieldWithSignal<Clip, ClipPrivate>(Schema::clipNextItemColumn(), &ClipPrivate::nextHandle, &ClipPrivate::next, &Clip::nextItemChanged, &Clip::nextItemChangedAfterCommit),
                {Schema::clipOverlappedCountColumn(), [](Clip *q, const dini::Value &value) {
                     auto *d = ClipPrivate::get(q);
                     const auto oldOverlapped = d->overlappedCount > 0;
                     d->overlappedCount = static_cast<int>(value.asInt64());
                     return oldOverlapped != (d->overlappedCount > 0);
                 }, [](Clip *q) {
                     emit q->overlappedChanged(q->overlapped());
                 }, [](Clip *q) {
                     emit q->overlappedChangedAfterCommit(q->overlapped());
                 }},
                orm::binaryField<Clip, ClipPrivate>(Schema::clipWorkspaceColumn(), &ClipPrivate::workspaceData, nullptr, nullptr),
                {Schema::clipParent().column(), [](Clip *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = ClipPrivate::get(q);
                     auto *newSequence = clipOwnerFromValue(*model, value);
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](Clip *q) {
                     emit q->clipSequenceChanged(ClipPrivate::get(q)->sequence);
                 }, [](Clip *q) {
                     emit q->clipSequenceChangedAfterCommit(ClipPrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &clipOrderSpec() {
            static const OrderSpec spec {{Schema::clipPositionColumn(), Schema::clipClipLengthColumn()}, true};
            return spec;
        }

        const TableBinding &clipTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<Clip, ClipSequence>({
                .table = Schema::clipTable(),
                .membershipColumns = {Schema::clipParent().column()},
                .orderColumns = {Schema::clipPositionColumn(), Schema::clipClipLengthColumn()},
                .moveSemantics = MoveSemantics::BetweenOwners,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Clip>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Clip>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) {
                    model.clipObjects.remove(handle);
                    model.audioClipObjects.remove(handle);
                    model.singingClipObjects.remove(handle);
                },
                .sync = [](Clip *item, const dini::ItemSnapshot &snapshot, bool notify) { syncClipColumns(item, snapshot, notify); },
                .applyColumn = [](Clip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyClipColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return clipOwnerFromValue(model, orm::snapshotValue(snapshot, Schema::clipParent().column()));
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::clipParent().column()) {
                        return clipOwnerFromValue(model, oldValue ? change.oldValue : change.newValue);
                    }
                    auto *clip = model.find<Clip>(orm::handleFromId(change.itemId));
                    return clip ? clip->clipSequence() : nullptr;
                },
                .setOwner = [](Clip *item, ClipSequence *owner, bool notify) { ClipPrivate::get(item)->setSequence(owner, notify); },
                .ownerChangedAfterCommit = [](Clip *item, ClipSequence *owner) { emit item->clipSequenceChangedAfterCommit(owner); },
                .refreshOwner = [](ClipSequence *owner, bool notify) { ClipSequencePrivate::get(owner)->refresh(notify); },
                .refreshOwnerAfterCommit = [](ClipSequence *owner, bool sizeChanged, bool orderChanged) {
                    if (sizeChanged) emit owner->sizeChangedAfterCommit(owner->size());
                    if (orderChanged) {
                        emit owner->firstItemChangedAfterCommit(owner->firstItem());
                        emit owner->lastItemChangedAfterCommit(owner->lastItem());
                    }
                },
                .itemAboutToInsert = [](ClipSequence *owner, Clip *item, ClipSequence *movedFrom) { emit owner->itemAboutToInsert(item, movedFrom); },
                .itemInserted = [](ClipSequence *owner, Clip *item, ClipSequence *movedFrom) { emit owner->itemInserted(item, movedFrom); },
                .itemAboutToRemove = [](ClipSequence *owner, Clip *item, ClipSequence *movedTo) { emit owner->itemAboutToRemove(item, movedTo); },
                .itemRemoved = [](ClipSequence *owner, Clip *item, ClipSequence *movedTo) { emit owner->itemRemoved(item, movedTo); },
                .itemAboutToInsertAfterCommit = [](ClipSequence *owner, Clip *item, ClipSequence *movedFrom) { emit owner->itemAboutToInsertAfterCommit(item, movedFrom); },
                .itemInsertedAfterCommit = [](ClipSequence *owner, Clip *item, ClipSequence *movedFrom) { emit owner->itemInsertedAfterCommit(item, movedFrom); },
                .itemAboutToRemoveAfterCommit = [](ClipSequence *owner, Clip *item, ClipSequence *movedTo) { emit owner->itemAboutToRemoveAfterCommit(item, movedTo); },
                .itemRemovedAfterCommit = [](ClipSequence *owner, Clip *item, ClipSequence *movedTo) { emit owner->itemRemovedAfterCommit(item, movedTo); },
            });
            return binding;
        }

        void syncClipColumns(Clip *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(clipColumnBindings(), item, snapshot, notify);
            if (auto *audio = qobject_cast<AudioClip *>(item)) {
                syncAudioClipColumns(audio, snapshot, notify);
            }
        }

        bool applyClipColumn(Clip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            if (applyColumnBinding(clipColumnBindings(), item, column, value, notify)) {
                return true;
            }
            if (auto *audio = qobject_cast<AudioClip *>(item)) {
                return applyAudioClipColumn(audio, column, value, notify);
            }
            return false;
        }

    }

    ClipPrivate::ClipPrivate(Clip *q) : q_ptr(q) {
    }

    void ClipPrivate::setSequence(ClipSequence *newSequence, bool notify) {
        Q_Q(Clip);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->clipSequenceChanged(sequence);
        }
    }

    dini::ByteArray ClipPrivate::workspace() const {
        return workspaceData;
    }

    void ClipPrivate::setWorkspace(dini::ByteArray workspace) {
        Q_Q(Clip);
        ModelPrivate::get(q->model())->update(q->handle(), Schema::clipWorkspaceColumn(), dini::Value(std::move(workspace)));
    }

    Clip::Clip(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new ClipPrivate(this)) {
    }

    Clip::~Clip() = default;

    QString Clip::name() const {
        Q_D(const Clip);
        return d->name;
    }

    void Clip::setName(const QString &name) {
        ModelPrivate::get(model())->update(handle(), Schema::clipNameColumn(), orm::valueFromString(name));
    }

    double Clip::gain() const {
        Q_D(const Clip);
        return d->gain;
    }

    void Clip::setGain(double gain) {
        ModelPrivate::get(model())->update(handle(), Schema::clipGainColumn(), dini::Value(gain));
    }

    double Clip::pan() const {
        Q_D(const Clip);
        return d->pan;
    }

    void Clip::setPan(double pan) {
        ModelPrivate::get(model())->update(handle(), Schema::clipPanColumn(), dini::Value(pan));
    }

    bool Clip::mute() const {
        Q_D(const Clip);
        return d->mute;
    }

    void Clip::setMute(bool mute) {
        ModelPrivate::get(model())->update(handle(), Schema::clipMuteColumn(), dini::Value(mute));
    }

    int Clip::position() const {
        Q_D(const Clip);
        return d->position;
    }

    void Clip::setPosition(int position) {
        ModelPrivate::get(model())->update(handle(), Schema::clipPositionColumn(), dini::Value(static_cast<std::int64_t>(position)));
    }

    int Clip::length() const {
        Q_D(const Clip);
        return d->length;
    }

    void Clip::setLength(int length) {
        ModelPrivate::get(model())->update(handle(), Schema::clipLengthColumn(), dini::Value(static_cast<std::int64_t>(length)));
    }

    int Clip::clipStart() const {
        Q_D(const Clip);
        return d->clipStart;
    }

    void Clip::setClipStart(int clipStart) {
        ModelPrivate::get(model())->update(handle(), Schema::clipClipStartColumn(), dini::Value(static_cast<std::int64_t>(clipStart)));
    }

    int Clip::clipLength() const {
        Q_D(const Clip);
        return d->clipLength;
    }

    void Clip::setClipLength(int clipLength) {
        ModelPrivate::get(model())->update(handle(), Schema::clipClipLengthColumn(), dini::Value(static_cast<std::int64_t>(clipLength)));
    }

    Clip::ClipType Clip::type() const {
        Q_D(const Clip);
        return d->type;
    }

    int Clip::start() const {
        return position() - clipStart();
    }

    Clip *Clip::previousItem() const {
        Q_D(const Clip);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<Clip>(d->previousHandle);
        }
        return d->previous;
    }

    Clip *Clip::nextItem() const {
        Q_D(const Clip);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<Clip>(d->nextHandle);
        }
        return d->next;
    }

    bool Clip::overlapped() const {
        Q_D(const Clip);
        return d->overlappedCount > 0;
    }

    ClipSequence *Clip::clipSequence() const {
        Q_D(const Clip);
        return d->sequence;
    }

    std::shared_ptr<opendspx::Clip> Clip::toOpenDSPX() const {
        switch (type()) {
            case Audio:
                if (auto *audio = qobject_cast<const AudioClip *>(this)) {
                    return std::make_shared<opendspx::AudioClip>(audio->toOpenDSPX());
                }
                break;
            case Singing:
                if (auto *singing = qobject_cast<const SingingClip *>(this)) {
                    return std::make_shared<opendspx::SingingClip>(singing->toOpenDSPX());
                }
                break;
        }
        Q_UNREACHABLE();
    }

    void Clip::fromOpenDSPX(const std::shared_ptr<opendspx::Clip> &clip) {
        switch (clip->type) {
            case opendspx::Clip::Type::Audio:
                if (auto *audio = qobject_cast<AudioClip *>(this)) {
                    audio->fromOpenDSPX(static_cast<const opendspx::AudioClip &>(*clip));
                }
                break;
            case opendspx::Clip::Type::Singing:
                if (auto *singing = qobject_cast<SingingClip *>(this)) {
                    singing->fromOpenDSPX(static_cast<const opendspx::SingingClip &>(*clip));
                }
                break;
            default:
                Q_UNREACHABLE();
        }
    }

    void Clip::fromOpenDSPXBase(const opendspx::Clip &clip) {
        Q_D(Clip);
        d->setWorkspace(conv::serializeWorkspace(clip.workspace));
        setName(QString::fromStdString(clip.name));
        setGain(conv::fromDecibel(clip.control.gain));
        setPan(clip.control.pan);
        setMute(clip.control.mute);
        setPosition(clip.time.pos);
        setLength(clip.time.length);
        setClipStart(clip.time.clipStart);
        setClipLength(clip.time.clipLen);
    }

    void Clip::toOpenDSPXBase(opendspx::Clip &clip) const {
        Q_D(const Clip);
        clip.workspace = conv::deserializeWorkspace(d->workspace());
        clip.name = name().toStdString();
        clip.control = {
            .gain = conv::toDecibel(gain()),
            .pan = pan(),
            .mute = mute(),
        };
        clip.time = {
            .pos = position(),
            .length = length(),
            .clipStart = clipStart(),
            .clipLen = clipLength(),
        };
    }

}

#include "moc_Clip.cpp"
