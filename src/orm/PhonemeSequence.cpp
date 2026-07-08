#include "PhonemeSequence.h"
#include "PhonemeSequence_p.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/phoneme.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec phonemeRelationQuery(Handle noteHandle, PhonemeSequence::PhonemeRole role) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::notePhonemeRelationParent(), noteHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::notePhonemeRelationRoleColumn()),
                                     dini::Value(static_cast<std::int64_t>(role))),
                }),
            };
        }

        dini::QuerySpec orderedPhonemeQuery(Handle relationHandle,
                                            dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::phonemeParent(), relationHandle),
                .sortKeys = orm::sortKeys(orm::phonemeOrderSpec(), direction),
            };
        }

        QList<Phoneme *> phonemesFromView(ModelPrivate *model, const dini::View &view) {
            QList<Phoneme *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Phoneme>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    PhonemeSequencePrivate::PhonemeSequencePrivate(PhonemeSequence *q, Note *note, PhonemeSequence::PhonemeRole role)
        : q_ptr(q), note(note), role(role), jsIterable(new JSIterable(q, JSIterable::Sequence)) {
    }

    Handle PhonemeSequencePrivate::relationHandle() const {
        auto *modelData = ModelPrivate::get(note->model());
        if (auto relation = orm::firstSnapshot(modelData->engine->query(Schema::notePhonemeRelationTable(),
                                                                        phonemeRelationQuery(note->handle(), role)))) {
            return orm::handleFromId(relation->id);
        }
        return {};
    }

    dini::Value PhonemeSequencePrivate::associationValue() const {
        return orm::valueFromHandle(relationHandle());
    }

    void PhonemeSequencePrivate::refresh(bool notify) {
        Q_Q(PhonemeSequence);
        auto *modelData = ModelPrivate::get(note->model());
        const auto relation = relationHandle();
        if (!relation) {
            const bool sizeChanged = size != 0;
            const bool firstChanged = first != nullptr;
            const bool lastChanged = last != nullptr;
            size = 0;
            first = nullptr;
            last = nullptr;
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
            return;
        }
        const auto view = modelData->engine->query(Schema::phonemeTable(), orderedPhonemeQuery(relation));
        const auto newSize = static_cast<int>(view.count());
        Phoneme *newFirst = nullptr;
        Phoneme *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Phoneme>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::phonemeTable(), orderedPhonemeQuery(relation, dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<Phoneme>(*lastSnapshot);
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

    PhonemeSequence::PhonemeSequence(Note *note, PhonemeRole role) : QObject(note), d_ptr(new PhonemeSequencePrivate(this, note, role)) {
    }

    PhonemeSequence::~PhonemeSequence() = default;

    int PhonemeSequence::size() const {
        Q_D(const PhonemeSequence);
        return d->size;
    }

    Phoneme *PhonemeSequence::firstItem() const {
        Q_D(const PhonemeSequence);
        return d->first;
    }

    Phoneme *PhonemeSequence::lastItem() const {
        Q_D(const PhonemeSequence);
        return d->last;
    }

    QList<Phoneme *> PhonemeSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        Q_D(const PhonemeSequence);
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(note()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::phonemeParent(), relation),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::phonemeStartColumn()),
                                                dini::ComparisonOperator::Less,
                                                dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::phonemeTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::phonemeOrderSpec()),
        });
        auto phonemes = phonemesFromView(modelData, view);
        phonemes.erase(std::remove_if(phonemes.begin(), phonemes.end(), [position](Phoneme *phoneme) {
            return !phoneme || phoneme->start() < position;
        }), phonemes.end());
        return phonemes;
    }

    bool PhonemeSequence::contains(Phoneme *item) const {
        return item && item->phonemeSequence() == this;
    }

    bool PhonemeSequence::insertItem(Phoneme *item) {
        if (!item || item->model() != note()->model() || item->phonemeSequence()) {
            return false;
        }
        Q_D(const PhonemeSequence);
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = associationValue},
        });
        return true;
    }

    bool PhonemeSequence::removeItem(Phoneme *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = dini::Value::null()},
        });
        return true;
    }

    bool PhonemeSequence::moveItem(Phoneme *item, PhonemeSequence *sequence) {
        if (!contains(item) || !sequence || sequence->note()->model() != note()->model() || sequence->contains(item)) {
            return false;
        }
        const auto associationValue = PhonemeSequencePrivate::get(sequence)->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = associationValue},
        });
        return true;
    }

    PhonemeSequence::PhonemeRole PhonemeSequence::role() const {
        Q_D(const PhonemeSequence);
        return d->role;
    }

    Note *PhonemeSequence::note() const {
        Q_D(const PhonemeSequence);
        return d->note;
    }

    std::vector<opendspx::Phoneme> PhonemeSequence::toOpenDSPX() const {
        std::vector<opendspx::Phoneme> result;
        for (auto phoneme = firstItem(); phoneme; phoneme = phoneme->nextItem()) {
            result.push_back(phoneme->toOpenDSPX());
        }
        return result;
    }

    void PhonemeSequence::fromOpenDSPX(const std::vector<opendspx::Phoneme> &phonemes) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        for (const auto &source : phonemes) {
            auto *phoneme = note()->model()->createPhoneme();
            phoneme->fromOpenDSPX(source);
            insertItem(phoneme);
        }
    }

}


#include "moc_PhonemeSequence.cpp"
