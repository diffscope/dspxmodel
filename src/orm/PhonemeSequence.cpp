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

        dini::QuerySpec orderedPhonemeQuery(Handle noteHandle,
                                            PhonemeSequence::PhonemeRole role,
                                            dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::phonemeParent(), noteHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::phonemeRoleColumn()), dini::Value(static_cast<std::int64_t>(role))),
                }),
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

        dini::ColumnValue phonemeRoleValue(PhonemeSequence::PhonemeRole role) {
            return dini::ColumnValue {
                .column = Schema::phonemeRoleColumn(),
                .value = dini::Value(static_cast<std::int64_t>(role)),
            };
        }

    }

    PhonemeSequencePrivate::PhonemeSequencePrivate(PhonemeSequence *q, Note *note, PhonemeSequence::PhonemeRole role)
        : q_ptr(q), note(note), role(role) {
    }

    void PhonemeSequencePrivate::refresh(bool notify) {
        Q_Q(PhonemeSequence);
        auto *modelData = ModelPrivate::get(note->model());
        const auto view = modelData->engine->query(Schema::phonemeTable(), orderedPhonemeQuery(note->handle(), role));
        const auto newSize = static_cast<int>(view.count());
        Phoneme *newFirst = nullptr;
        Phoneme *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Phoneme>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::phonemeTable(), orderedPhonemeQuery(note->handle(), role, dini::SortDirection::Descending));
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
        auto *modelData = ModelPrivate::get(note()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::phonemeParent(), note()->handle()),
            orm::equalFilter(dini::FieldRef::column(Schema::phonemeRoleColumn()), dini::Value(static_cast<std::int64_t>(role()))),
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
        if (!item || item->phonemeSequence()) {
            return false;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = orm::valueFromHandle(note()->handle())},
            phonemeRoleValue(role()),
        });
        return true;
    }

    bool PhonemeSequence::removeItem(Phoneme *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = dini::Value::null()},
            dini::ColumnValue {.column = Schema::phonemeRoleColumn(), .value = dini::Value::null()},
        });
        return true;
    }

    bool PhonemeSequence::moveItem(Phoneme *item, PhonemeSequence *sequence) {
        if (!contains(item) || !sequence || sequence->contains(item)) {
            return false;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = orm::valueFromHandle(sequence->note()->handle())},
            phonemeRoleValue(sequence->role()),
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
