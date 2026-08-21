#include "PhonemeSequence.h"
#include "PhonemeSequence_p.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

#include <dini/engine.h>
#include <opendspx/phoneme.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/Phoneme.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Phoneme_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedPhonemeQuery(Handle noteHandle,
                                            dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::phonemeParent(), noteHandle),
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
        return role == PhonemeSequence::Edited ? note->handle() : Handle {};
    }

    dini::Value PhonemeSequencePrivate::associationValue() const {
        return orm::valueFromHandle(relationHandle());
    }

    void PhonemeSequencePrivate::refresh(bool notify) {
        if (role == PhonemeSequence::Original) {
            refreshOriginal(notify);
            return;
        }
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

    void PhonemeSequencePrivate::addOriginalItem(Phoneme *item) {
        Q_Q(PhonemeSequence);
        originalItems.emplace(std::make_pair(item->start(), item->handle().d), item);
        originalStartConnections.emplace(
            item,
            QObject::connect(item, &Phoneme::startChanged, q, [this, item](int) {
                originalStartChanged(item);
            }));
    }

    void PhonemeSequencePrivate::removeOriginalItem(Phoneme *item) {
        const auto itemIt = std::find_if(originalItems.begin(), originalItems.end(), [item](const auto &entry) {
            return entry.second == item;
        });
        if (itemIt != originalItems.end()) {
            originalItems.erase(itemIt);
        }
        const auto connectionIt = originalStartConnections.find(item);
        if (connectionIt != originalStartConnections.end()) {
            QObject::disconnect(connectionIt->second);
            originalStartConnections.erase(connectionIt);
        }
    }

    void PhonemeSequencePrivate::originalStartChanged(Phoneme *item) {
        const auto itemIt = std::find_if(originalItems.begin(), originalItems.end(), [item](const auto &entry) {
            return entry.second == item;
        });
        if (itemIt == originalItems.end()) {
            return;
        }
        originalItems.erase(itemIt);
        originalItems.emplace(std::make_pair(item->start(), item->handle().d), item);
        refreshOriginal(true);
    }

    void PhonemeSequencePrivate::refreshOriginal(bool notify) {
        Q_Q(PhonemeSequence);
        const auto newSize = static_cast<int>(originalItems.size());
        auto *newFirst = originalItems.empty() ? nullptr : originalItems.begin()->second;
        auto *newLast = originalItems.empty() ? nullptr : originalItems.rbegin()->second;
        const bool sizeChanged = size != newSize;
        const bool firstChanged = first != newFirst;
        const bool lastChanged = last != newLast;
        size = newSize;
        first = newFirst;
        last = newLast;

        for (auto it = originalItems.begin(); it != originalItems.end(); ++it) {
            auto *item = it->second;
            auto *previous = it == originalItems.begin() ? nullptr : std::prev(it)->second;
            auto next = std::next(it);
            auto *nextItem = next == originalItems.end() ? nullptr : next->second;
            auto *itemData = PhonemePrivate::get(item);
            itemData->setPreviousItem(previous, notify);
            itemData->setNextItem(nextItem, notify);
        }

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

    void PhonemeSequencePrivate::clearOriginalItemsForModelDestruction() {
        for (const auto &entry : originalStartConnections) {
            QObject::disconnect(entry.second);
        }
        originalStartConnections.clear();
        originalItems.clear();
        size = 0;
        first = nullptr;
        last = nullptr;
    }

    void PhonemeSequencePrivate::destroyOriginalItems() {
        Q_Q(PhonemeSequence);
        auto *modelData = ModelPrivate::get(note->model());
        while (!originalItems.empty()) {
            auto *item = originalItems.begin()->second;
            q->removeItem(item);
            modelData->phonemeObjects.remove(item->handle());
            item->deleteLater();
        }
    }

    PhonemeSequence::PhonemeSequence(Note *note, PhonemeRole role) : QObject(note), d_ptr(new PhonemeSequencePrivate(this, note, role)) {
    }

    PhonemeSequence::~PhonemeSequence() {
        Q_D(PhonemeSequence);
        if (d->role != Original || d->originalItems.empty()) {
            return;
        }
        auto *modelData = ModelPrivate::get(d->note->model());
        QList<Phoneme *> items;
        items.reserve(static_cast<qsizetype>(d->originalItems.size()));
        for (const auto &entry : d->originalItems) {
            items.append(entry.second);
        }
        for (const auto &entry : d->originalStartConnections) {
            QObject::disconnect(entry.second);
        }
        d->originalItems.clear();
        d->originalStartConnections.clear();
        if (modelData->destroying) {
            return;
        }
        for (auto *item : items) {
            auto *itemData = PhonemePrivate::get(item);
            itemData->setSequence(nullptr, false);
            itemData->setPreviousItem(nullptr, false);
            itemData->setNextItem(nullptr, false);
            modelData->phonemeObjects.remove(item->handle());
            item->deleteLater();
        }
    }

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
        if (position < 0 || length <= 0) {
            return {};
        }
        const auto queryEnd = static_cast<std::int64_t>(position) + static_cast<std::int64_t>(length);
        Q_D(const PhonemeSequence);
        if (d->role == Original) {
            QList<Phoneme *> result;
            for (auto it = d->originalItems.lower_bound(std::make_pair(position, quint64 {0}));
                 it != d->originalItems.end() && it->first.first < queryEnd;
                 ++it) {
                result.append(it->second);
            }
            return result;
        }
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(note()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::phonemeParent(), relation),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::phonemeStartColumn()),
                                                dini::ComparisonOperator::GreaterOrEqual,
                                                dini::Value(static_cast<std::int64_t>(position)))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::phonemeStartColumn()),
                                                dini::ComparisonOperator::Less,
                                                dini::Value(queryEnd))),
        });
        const auto view = modelData->engine->query(Schema::phonemeTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::phonemeOrderSpec()),
        });
        return phonemesFromView(modelData, view);
    }

    bool PhonemeSequence::contains(Phoneme *item) const {
        return item && item->phonemeSequence() == this;
    }

    bool PhonemeSequence::insertItem(Phoneme *item) {
        Q_D(PhonemeSequence);
        const auto itemRole = d->role == Original ? Phoneme::Original : Phoneme::Edited;
        if (!item || item->model() != note()->model() || item->phonemeSequence() ||
            item->role() != itemRole) {
            return false;
        }
        if (d->role == Original) {
            if (ModelPrivate::get(note()->model())->find<Phoneme>(item->handle()) != item) {
                return false;
            }
            emit itemAboutToInsert(item, nullptr);
            d->addOriginalItem(item);
            PhonemePrivate::get(item)->setSequence(this, true);
            d->refreshOriginal(true);
            emit itemInserted(item, nullptr);
            return true;
        }
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
        Q_D(PhonemeSequence);
        if (d->role == Original) {
            emit itemAboutToRemove(item, nullptr);
            d->removeOriginalItem(item);
            auto *itemData = PhonemePrivate::get(item);
            itemData->setSequence(nullptr, true);
            itemData->setPreviousItem(nullptr, true);
            itemData->setNextItem(nullptr, true);
            d->refreshOriginal(true);
            emit itemRemoved(item, nullptr);
            return true;
        }
        ModelPrivate::get(note()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::phonemeParent().column(), .value = dini::Value::null()},
        });
        return true;
    }

    bool PhonemeSequence::moveItem(Phoneme *item, PhonemeSequence *sequence) {
        if (!contains(item) || !sequence || sequence->note()->model() != note()->model() || sequence->contains(item) ||
            sequence->role() != role()) {
            return false;
        }
        Q_D(PhonemeSequence);
        if (d->role == Original) {
            auto *targetData = PhonemeSequencePrivate::get(sequence);
            emit itemAboutToRemove(item, sequence);
            emit sequence->itemAboutToInsert(item, this);
            d->removeOriginalItem(item);
            targetData->addOriginalItem(item);
            PhonemePrivate::get(item)->setSequence(sequence, true);
            d->refreshOriginal(true);
            targetData->refreshOriginal(true);
            emit itemRemoved(item, sequence);
            emit sequence->itemInserted(item, this);
            return true;
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
            auto *phoneme = role() == Original ? note()->model()->createOriginalPhoneme()
                                               : note()->model()->createPhoneme();
            phoneme->fromOpenDSPX(source);
            insertItem(phoneme);
        }
    }

}


#include "moc_PhonemeSequence.cpp"
