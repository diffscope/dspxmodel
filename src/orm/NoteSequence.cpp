#include "NoteSequence.h"
#include "NoteSequence_p.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/singingclip.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/SingingClip.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/Note_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedNoteQuery(Handle singingClipHandle, dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::noteParent(), singingClipHandle),
                .sortKeys = orm::sortKeys(orm::noteOrderSpec(), direction),
            };
        }

        QList<Note *> notesFromView(ModelPrivate *model, const dini::View &view) {
            QList<Note *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Note>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    NoteSequencePrivate::NoteSequencePrivate(NoteSequence *q, SingingClip *singingClip) : q_ptr(q), singingClip(singingClip), jsIterable(new JSIterable(q, JSIterable::Sequence)) {
    }

    void NoteSequencePrivate::refresh(bool notify) {
        Q_Q(NoteSequence);
        auto *modelData = ModelPrivate::get(singingClip->model());
        const auto view = modelData->engine->query(Schema::noteTable(), orderedNoteQuery(singingClip->handle()));
        const auto newSize = static_cast<int>(view.count());
        Note *newFirst = nullptr;
        Note *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Note>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::noteTable(), orderedNoteQuery(singingClip->handle(), dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<Note>(*lastSnapshot);
            }
        }

        const bool sizeChanged = size != newSize;
        const bool firstChanged = first != newFirst;
        const bool lastChanged = last != newLast;
        size = newSize;
        first = newFirst;
        last = newLast;

        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (firstChanged) {
            emit q->firstItemChanged(first);
        }
        if (lastChanged) {
            emit q->lastItemChanged(last);
        }
    }

    NoteSequence::NoteSequence(SingingClip *singingClip) : QObject(singingClip), d_ptr(new NoteSequencePrivate(this, singingClip)) {
    }

    NoteSequence::~NoteSequence() = default;

    int NoteSequence::size() const {
        Q_D(const NoteSequence);
        return d->size;
    }

    Note *NoteSequence::firstItem() const {
        Q_D(const NoteSequence);
        return d->first;
    }

    Note *NoteSequence::lastItem() const {
        Q_D(const NoteSequence);
        return d->last;
    }

    QList<Note *> NoteSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        auto *modelData = ModelPrivate::get(singingClip()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::noteParent(), singingClip()->handle()),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::notePositionColumn()),
                                                dini::ComparisonOperator::Less,
                                                dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::noteTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::noteOrderSpec()),
        });
        auto notes = notesFromView(modelData, view);
        notes.erase(std::remove_if(notes.begin(), notes.end(), [position](Note *note) {
            return !note || note->position() + note->length() <= position;
        }), notes.end());
        return notes;
    }

    bool NoteSequence::contains(Note *item) const {
        return item && item->noteSequence() == this;
    }

    bool NoteSequence::insertItem(Note *item) {
        if (!item || item->noteSequence()) {
            return false;
        }
        ModelPrivate::get(singingClip()->model())->update(item->handle(), Schema::noteParent().column(), orm::valueFromHandle(singingClip()->handle()));
        return true;
    }

    bool NoteSequence::removeItem(Note *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(singingClip()->model())->update(item->handle(), Schema::noteParent().column(), dini::Value::null());
        return true;
    }

    bool NoteSequence::moveItem(Note *item, NoteSequence *sequence) {
        if (!contains(item) || !sequence || sequence->contains(item)) {
            return false;
        }
        ModelPrivate::get(singingClip()->model())->update(item->handle(), Schema::noteParent().column(), orm::valueFromHandle(sequence->singingClip()->handle()));
        return true;
    }

    SingingClip *NoteSequence::singingClip() const {
        Q_D(const NoteSequence);
        return d->singingClip;
    }

    std::vector<opendspx::Note> NoteSequence::toOpenDSPX() const {
        std::vector<opendspx::Note> result;
        for (auto note = firstItem(); note; note = note->nextItem()) {
            result.push_back(note->toOpenDSPX());
        }
        return result;
    }

    void NoteSequence::fromOpenDSPX(const std::vector<opendspx::Note> &notes) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        for (const auto &source : notes) {
            auto note = singingClip()->model()->createNote();
            note->fromOpenDSPX(source);
            insertItem(note);
        }
    }

}


#include "moc_NoteSequence.cpp"
