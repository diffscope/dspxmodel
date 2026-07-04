#include "KeySignatureSequence.h"
#include "KeySignatureSequence_p.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/KeySignature_p.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <nlohmann/json.hpp>

namespace dspx {

    namespace {

        dini::QuerySpec orderedKeySignatureQuery(Handle modelHandle, dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::keySignatureParent(), modelHandle),
                .sortKeys = orm::sortKeys(orm::keySignatureOrderSpec(), direction),
            };
        }

        QList<KeySignature *> keySignaturesFromView(ModelPrivate *model, const dini::View &view) {
            QList<KeySignature *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<KeySignature>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    KeySignatureSequencePrivate::KeySignatureSequencePrivate(KeySignatureSequence *q, Model *model) : q_ptr(q), model(model) {
    }

    void KeySignatureSequencePrivate::refresh(bool notify) {
        Q_Q(KeySignatureSequence);
        auto *modelData = ModelPrivate::get(model);
        const auto view =
            modelData->engine->query(Schema::keySignatureTable(), orderedKeySignatureQuery(modelData->modelHandle));
        const auto newSize = static_cast<int>(view.count());
        KeySignature *newFirst = nullptr;
        KeySignature *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<KeySignature>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::keySignatureTable(), orderedKeySignatureQuery(modelData->modelHandle, dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<KeySignature>(*lastSnapshot);
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

    Handle KeySignatureSequencePrivate::itemAtPosition(int position, Handle except) const {
        auto *modelData = ModelPrivate::get(model);
        const auto result = modelData->engine->query(
            Schema::keySignatureTable(),
            dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::keySignatureParent(), modelData->modelHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::keySignaturePositionColumn()), dini::Value(static_cast<std::int64_t>(position))),
                }),
            }).toVector();
        for (const auto &snapshot : result) {
            const auto handle = orm::handleFromId(snapshot.id);
            if (handle != except) {
                return handle;
            }
        }
        return {};
    }

    KeySignatureSequence::KeySignatureSequence(Model *model)
        : QObject(model), d_ptr(new KeySignatureSequencePrivate(this, model)) {
    }

    KeySignatureSequence::~KeySignatureSequence() = default;

    int KeySignatureSequence::size() const {
        Q_D(const KeySignatureSequence);
        return d->size;
    }

    KeySignature *KeySignatureSequence::firstItem() const {
        Q_D(const KeySignatureSequence);
        return d->first;
    }

    KeySignature *KeySignatureSequence::lastItem() const {
        Q_D(const KeySignatureSequence);
        return d->last;
    }

    QList<KeySignature *> KeySignatureSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        auto *modelData = ModelPrivate::get(model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::keySignatureParent(), modelData->modelHandle),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::keySignaturePositionColumn()), dini::ComparisonOperator::GreaterOrEqual, dini::Value(static_cast<std::int64_t>(position)))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::keySignaturePositionColumn()), dini::ComparisonOperator::Less, dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::keySignatureTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::keySignatureOrderSpec()),
        });
        return keySignaturesFromView(modelData, view);
    }

    bool KeySignatureSequence::contains(KeySignature *item) const {
        return item && item->keySignatureSequence() == this;
    }

    bool KeySignatureSequence::insertItem(KeySignature *item) {
        if (!item || item->keySignatureSequence()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(model());
        const auto conflict = KeySignatureSequencePrivate::get(this)->itemAtPosition(item->position(), item->handle());
        if (conflict) {
            modelData->update(conflict, Schema::keySignatureParent().column(), dini::Value::null());
        }
        modelData->update(item->handle(), Schema::keySignatureParent().column(), orm::valueFromHandle(modelData->modelHandle));
        return true;
    }

    bool KeySignatureSequence::removeItem(KeySignature *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(model())->update(item->handle(), Schema::keySignatureParent().column(), dini::Value::null());
        return true;
    }

    Model *KeySignatureSequence::model() const {
        return KeySignatureSequencePrivate::get(this)->model;
    }

    nlohmann::json KeySignatureSequence::toOpenDSPX() const {
        nlohmann::json result = nlohmann::json::array();
        for (auto keySignature = firstItem(); keySignature; keySignature = keySignature->nextItem()) {
            result.push_back(keySignature->toOpenDSPX());
        }
        return result;
    }

    void KeySignatureSequence::fromOpenDSPX(const nlohmann::json &keySignatures) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        if (!keySignatures.is_array()) {
            return;
        }
        for (const auto &source : keySignatures) {
            auto keySignature = model()->createKeySignature();
            keySignature->fromOpenDSPX(source);
            insertItem(keySignature);
        }
    }

}
