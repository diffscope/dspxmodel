#include "TempoSequence.h"
#include "TempoSequence_p.h"

#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/tempo.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/Tempo_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedTempoQuery(Handle modelHandle, dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::tempoParent(), modelHandle),
                .sortKeys = orm::sortKeys(orm::tempoOrderSpec(), direction),
            };
        }

        QList<Tempo *> temposFromView(ModelPrivate *model, const dini::View &view) {
            QList<Tempo *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Tempo>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    TempoSequencePrivate::TempoSequencePrivate(TempoSequence *q, Model *model) : q_ptr(q), model(model), jsIterable(new JSIterable(q, JSIterable::Sequence)) {
    }

    void TempoSequencePrivate::refresh(bool notify) {
        Q_Q(TempoSequence);
        auto *modelData = ModelPrivate::get(model);
        const auto view = modelData->engine->query(Schema::tempoTable(), orderedTempoQuery(modelData->modelHandle));
        const auto newSize = static_cast<int>(view.count());
        Tempo *newFirst = nullptr;
        Tempo *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Tempo>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::tempoTable(), orderedTempoQuery(modelData->modelHandle, dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<Tempo>(*lastSnapshot);
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

    TempoSequence::TempoSequence(Model *model) : QObject(model), d_ptr(new TempoSequencePrivate(this, model)) {
    }

    TempoSequence::~TempoSequence() = default;

    int TempoSequence::size() const {
        Q_D(const TempoSequence);
        return d->size;
    }

    Tempo *TempoSequence::firstItem() const {
        Q_D(const TempoSequence);
        return d->first;
    }

    Tempo *TempoSequence::lastItem() const {
        Q_D(const TempoSequence);
        return d->last;
    }

    QList<Tempo *> TempoSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        auto *modelData = ModelPrivate::get(model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::tempoParent(), modelData->modelHandle),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::tempoPositionColumn()), dini::ComparisonOperator::GreaterOrEqual, dini::Value(static_cast<std::int64_t>(position)))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::tempoPositionColumn()), dini::ComparisonOperator::Less, dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::tempoTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::tempoOrderSpec()),
        });
        return temposFromView(modelData, view);
    }

    bool TempoSequence::contains(Tempo *item) const {
        return item && item->tempoSequence() == this;
    }

    bool TempoSequence::insertItem(Tempo *item) {
        if (!item || item->tempoSequence()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(model());
        modelData->update(item->handle(), Schema::tempoParent().column(), orm::valueFromHandle(modelData->modelHandle));
        return true;
    }

    bool TempoSequence::removeItem(Tempo *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(model())->update(item->handle(), Schema::tempoParent().column(), dini::Value::null());
        return true;
    }

    Model *TempoSequence::model() const {
        return TempoSequencePrivate::get(this)->model;
    }

    std::vector<opendspx::Tempo> TempoSequence::toOpenDSPX() const {
        std::vector<opendspx::Tempo> result;
        for (auto tempo = firstItem(); tempo; tempo = tempo->nextItem()) {
            result.push_back(tempo->toOpenDSPX());
        }
        return result;
    }

    void TempoSequence::fromOpenDSPX(const std::vector<opendspx::Tempo> &tempos) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        for (const auto &source : tempos) {
            auto tempo = model()->createTempo();
            tempo->fromOpenDSPX(source);
            insertItem(tempo);
        }
    }

}


#include "moc_TempoSequence.cpp"
