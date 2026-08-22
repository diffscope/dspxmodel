#include "ClipSequence.h"
#include "ClipSequence_p.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/audioclip.h>
#include <opendspx/singingclip.h>
#include <opendspx/track.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AudioClip.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/Track.h>
#include <dspxmodelORM/private/Clip_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedClipQuery(Handle trackHandle, dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::clipParent(), trackHandle),
                .sortKeys = orm::sortKeys(orm::clipOrderSpec(), direction),
            };
        }

        QList<Clip *> clipsFromView(ModelPrivate *model, const dini::View &view) {
            QList<Clip *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Clip>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    ClipSequencePrivate::ClipSequencePrivate(ClipSequence *q, Track *track) : q_ptr(q), track(track), jsIterable(new JSIterable(q, JSIterable::Sequence)) {
    }

    void ClipSequencePrivate::refresh(bool notify) {
        Q_Q(ClipSequence);
        auto *modelData = ModelPrivate::get(track->model());
        const auto view = modelData->engine->query(Schema::clipTable(), orderedClipQuery(track->handle()));
        const auto newSize = static_cast<int>(view.count());
        const auto trackId = orm::idFromHandle(track->handle());
        const auto newAudioClipCount = static_cast<int>(modelData->engine->read(trackId, Schema::trackAudioClipCountColumn()).asInt64());
        const auto newSingingClipCount = static_cast<int>(modelData->engine->read(trackId, Schema::trackSingingClipCountColumn()).asInt64());
        Clip *newFirst = nullptr;
        Clip *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Clip>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::clipTable(), orderedClipQuery(track->handle(), dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<Clip>(*lastSnapshot);
            }
        }

        const bool sizeChanged = size != newSize;
        const bool audioClipCountChanged = audioClipCount != newAudioClipCount;
        const bool singingClipCountChanged = singingClipCount != newSingingClipCount;
        const bool firstChanged = first != newFirst;
        const bool lastChanged = last != newLast;
        size = newSize;
        audioClipCount = newAudioClipCount;
        singingClipCount = newSingingClipCount;
        first = newFirst;
        last = newLast;

        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (audioClipCountChanged) {
            emit q->audioClipCountChanged(audioClipCount);
        }
        if (singingClipCountChanged) {
            emit q->singingClipCountChanged(singingClipCount);
        }
        if (firstChanged) {
            emit q->firstItemChanged(first);
        }
        if (lastChanged) {
            emit q->lastItemChanged(last);
        }
    }

    ClipSequence::ClipSequence(Track *track) : QObject(track), d_ptr(new ClipSequencePrivate(this, track)) {
    }

    ClipSequence::~ClipSequence() = default;

    int ClipSequence::size() const {
        Q_D(const ClipSequence);
        return d->size;
    }

    int ClipSequence::audioClipCount() const {
        Q_D(const ClipSequence);
        return d->audioClipCount;
    }

    int ClipSequence::singingClipCount() const {
        Q_D(const ClipSequence);
        return d->singingClipCount;
    }

    Clip *ClipSequence::firstItem() const {
        Q_D(const ClipSequence);
        return d->first;
    }

    Clip *ClipSequence::lastItem() const {
        Q_D(const ClipSequence);
        return d->last;
    }

    QList<Clip *> ClipSequence::slice(int position, int length) const {
        if (position < 0 || length <= 0) {
            return {};
        }
        const auto queryEnd = static_cast<std::int64_t>(position) + static_cast<std::int64_t>(length);
        auto *modelData = ModelPrivate::get(track()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::clipParent(), track()->handle()),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::clipPositionColumn()),
                                                dini::ComparisonOperator::Less,
                                                dini::Value(queryEnd))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::clipEndColumn()),
                                                dini::ComparisonOperator::Greater,
                                                dini::Value(static_cast<std::int64_t>(position)))),
        });
        const auto view = modelData->engine->query(Schema::clipTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::clipOrderSpec()),
        });
        return clipsFromView(modelData, view);
    }

    bool ClipSequence::contains(Clip *item) const {
        return item && item->clipSequence() == this;
    }

    bool ClipSequence::insertItem(Clip *item) {
        if (!item || item->clipSequence()) {
            return false;
        }
        ModelPrivate::get(track()->model())->update(item->handle(), Schema::clipParent().column(), orm::valueFromHandle(track()->handle()));
        return true;
    }

    bool ClipSequence::removeItem(Clip *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(track()->model())->update(item->handle(), Schema::clipParent().column(), dini::Value::null());
        return true;
    }

    bool ClipSequence::moveItem(Clip *item, ClipSequence *sequence) {
        if (!contains(item) || !sequence || sequence->contains(item)) {
            return false;
        }
        ModelPrivate::get(track()->model())->update(item->handle(), Schema::clipParent().column(), orm::valueFromHandle(sequence->track()->handle()));
        return true;
    }

    std::vector<std::shared_ptr<opendspx::Clip>> ClipSequence::toOpenDSPX() const {
        std::vector<std::shared_ptr<opendspx::Clip>> result;
        for (auto clip = firstItem(); clip; clip = clip->nextItem()) {
            if (auto converted = clip->toOpenDSPX()) {
                result.push_back(std::move(converted));
            }
        }
        return result;
    }

    void ClipSequence::fromOpenDSPX(const std::vector<std::shared_ptr<opendspx::Clip>> &clips) {
        while (size() > 0) {
            removeItem(firstItem());
        }

        for (const auto &source : clips) {
            if (!source) {
                continue;
            }
            switch (source->type) {
                case opendspx::Clip::Type::Audio: {
                    auto *clip = track()->model()->createAudioClip();
                    clip->fromOpenDSPX(*static_cast<const opendspx::AudioClip *>(source.get()));
                    insertItem(clip);
                    break;
                }
                case opendspx::Clip::Type::Singing: {
                    auto *clip = track()->model()->createSingingClip();
                    clip->fromOpenDSPX(*static_cast<const opendspx::SingingClip *>(source.get()));
                    insertItem(clip);
                    break;
                }
            }
        }
    }

    Track *ClipSequence::track() const {
        Q_D(const ClipSequence);
        return d->track;
    }

}

#include "moc_ClipSequence.cpp"
