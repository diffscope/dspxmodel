#include "DynamicMixingAnchorSequence.h"
#include "DynamicMixingAnchorSequence_p.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include <dini/engine.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/DynamicMixingAnchor.h>
#include <dspxmodelORM/Sources.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedDynamicMixingAnchorQuery(Handle sourcesHandle,
                                                        dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::dynamicMixingAnchorParent(), sourcesHandle),
                .sortKeys = orm::sortKeys(orm::dynamicMixingAnchorOrderSpec(), direction),
            };
        }

        QList<DynamicMixingAnchor *> dynamicMixingAnchorsFromView(ModelPrivate *model, const dini::View &view) {
            QList<DynamicMixingAnchor *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<DynamicMixingAnchor>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    DynamicMixingAnchorSequencePrivate::DynamicMixingAnchorSequencePrivate(DynamicMixingAnchorSequence *q,
                                                                           Sources *sources)
        : q_ptr(q), sources(sources) {
    }

    void DynamicMixingAnchorSequencePrivate::refresh(bool notify) {
        Q_Q(DynamicMixingAnchorSequence);
        auto *modelData = ModelPrivate::get(sources->model());
        const auto view = modelData->engine->query(Schema::dynamicMixingAnchorTable(),
                                                   orderedDynamicMixingAnchorQuery(sources->handle()));
        const auto newSize = static_cast<int>(view.count());
        DynamicMixingAnchor *newFirst = nullptr;
        DynamicMixingAnchor *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<DynamicMixingAnchor>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(
                Schema::dynamicMixingAnchorTable(),
                orderedDynamicMixingAnchorQuery(sources->handle(), dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<DynamicMixingAnchor>(*lastSnapshot);
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

    Handle DynamicMixingAnchorSequencePrivate::itemAtPosition(int position, Handle except) const {
        auto *modelData = ModelPrivate::get(sources->model());
        const auto result = modelData->engine->query(
            Schema::dynamicMixingAnchorTable(),
            dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::dynamicMixingAnchorParent(), sources->handle()),
                    orm::equalFilter(dini::FieldRef::column(Schema::dynamicMixingAnchorPositionColumn()),
                                     dini::Value(static_cast<std::int64_t>(position))),
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

    DynamicMixingAnchorSequence::DynamicMixingAnchorSequence(Sources *sources)
        : QObject(sources), d_ptr(new DynamicMixingAnchorSequencePrivate(this, sources)) {
    }

    DynamicMixingAnchorSequence::~DynamicMixingAnchorSequence() = default;

    int DynamicMixingAnchorSequence::size() const {
        Q_D(const DynamicMixingAnchorSequence);
        return d->size;
    }

    DynamicMixingAnchor *DynamicMixingAnchorSequence::firstItem() const {
        Q_D(const DynamicMixingAnchorSequence);
        return d->first;
    }

    DynamicMixingAnchor *DynamicMixingAnchorSequence::lastItem() const {
        Q_D(const DynamicMixingAnchorSequence);
        return d->last;
    }

    QList<DynamicMixingAnchor *> DynamicMixingAnchorSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        auto *modelData = ModelPrivate::get(sources()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::dynamicMixingAnchorParent(), sources()->handle()),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::dynamicMixingAnchorPositionColumn()),
                                                dini::ComparisonOperator::GreaterOrEqual,
                                                dini::Value(static_cast<std::int64_t>(position)))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::dynamicMixingAnchorPositionColumn()),
                                                dini::ComparisonOperator::Less,
                                                dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::dynamicMixingAnchorTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::dynamicMixingAnchorOrderSpec()),
        });
        return dynamicMixingAnchorsFromView(modelData, view);
    }

    bool DynamicMixingAnchorSequence::contains(DynamicMixingAnchor *item) const {
        return item && item->dynamicMixingAnchorSequence() == this;
    }

    bool DynamicMixingAnchorSequence::insertItem(DynamicMixingAnchor *item) {
        if (!item || item->model() != sources()->model() || item->dynamicMixingAnchorSequence()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(sources()->model());
        const auto conflict = DynamicMixingAnchorSequencePrivate::get(this)->itemAtPosition(item->position(), item->handle());
        if (conflict) {
            modelData->update(conflict, Schema::dynamicMixingAnchorParent().column(), dini::Value::null());
        }
        modelData->update(item->handle(), Schema::dynamicMixingAnchorParent().column(), orm::valueFromHandle(sources()->handle()));
        return true;
    }

    bool DynamicMixingAnchorSequence::removeItem(DynamicMixingAnchor *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(sources()->model())->update(item->handle(), Schema::dynamicMixingAnchorParent().column(), dini::Value::null());
        return true;
    }

    bool DynamicMixingAnchorSequence::moveItem(DynamicMixingAnchor *item, DynamicMixingAnchorSequence *sequence) {
        if (!contains(item) || !sequence || sequence->sources()->model() != sources()->model() || sequence->contains(item)) {
            return false;
        }
        auto *modelData = ModelPrivate::get(sources()->model());
        const auto conflict = DynamicMixingAnchorSequencePrivate::get(sequence)->itemAtPosition(item->position(), item->handle());
        if (conflict) {
            modelData->update(conflict, Schema::dynamicMixingAnchorParent().column(), dini::Value::null());
        }
        modelData->update(item->handle(), Schema::dynamicMixingAnchorParent().column(), orm::valueFromHandle(sequence->sources()->handle()));
        return true;
    }

    Sources *DynamicMixingAnchorSequence::sources() const {
        Q_D(const DynamicMixingAnchorSequence);
        return d->sources;
    }

}


#include "moc_DynamicMixingAnchorSequence.cpp"
