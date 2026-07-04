#include "TimeSignatureSequence.h"
#include "TimeSignatureSequence_p.h"

#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/timesignature.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/TimeSignature_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedTimeSignatureQuery(Handle modelHandle, dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::timeSignatureParent(), modelHandle),
                .sortKeys = orm::sortKeys(orm::timeSignatureOrderSpec(), direction),
            };
        }

        QList<TimeSignature *> timeSignaturesFromView(ModelPrivate *model, const dini::View &view) {
            QList<TimeSignature *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<TimeSignature>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    TimeSignatureSequencePrivate::TimeSignatureSequencePrivate(TimeSignatureSequence *q, Model *model) : q_ptr(q), model(model) {
    }

    void TimeSignatureSequencePrivate::refresh(bool notify) {
        auto *modelData = ModelPrivate::get(model);
        const auto view = modelData->engine->query(Schema::timeSignatureTable(), orderedTimeSignatureQuery(modelData->modelHandle));
        const auto newSize = static_cast<int>(view.count());
        TimeSignature *newFirst = nullptr;
        TimeSignature *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<TimeSignature>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::timeSignatureTable(), orderedTimeSignatureQuery(modelData->modelHandle, dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<TimeSignature>(*lastSnapshot);
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
        auto *q = q_func();
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

    Handle TimeSignatureSequencePrivate::itemAtIndex(int index, Handle except) const {
        auto *modelData = ModelPrivate::get(model);
        const auto result = modelData->engine->query(
            Schema::timeSignatureTable(),
            dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::timeSignatureParent(), modelData->modelHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::timeSignatureIndexColumn()), dini::Value(static_cast<std::int64_t>(index))),
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

    TimeSignatureSequence::TimeSignatureSequence(Model *model)
        : QObject(model), d_ptr(new TimeSignatureSequencePrivate(this, model)) {
    }

    TimeSignatureSequence::~TimeSignatureSequence() = default;

    int TimeSignatureSequence::size() const {
        Q_D(const TimeSignatureSequence);
        return d->size;
    }

    TimeSignature *TimeSignatureSequence::firstItem() const {
        Q_D(const TimeSignatureSequence);
        return d->first;
    }

    TimeSignature *TimeSignatureSequence::lastItem() const {
        Q_D(const TimeSignatureSequence);
        return d->last;
    }

    QList<TimeSignature *> TimeSignatureSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        auto *modelData = ModelPrivate::get(model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::timeSignatureParent(), modelData->modelHandle),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::timeSignatureIndexColumn()), dini::ComparisonOperator::GreaterOrEqual, dini::Value(static_cast<std::int64_t>(position)))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::timeSignatureIndexColumn()), dini::ComparisonOperator::Less, dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::timeSignatureTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::timeSignatureOrderSpec()),
        });
        return timeSignaturesFromView(modelData, view);
    }

    bool TimeSignatureSequence::contains(TimeSignature *item) const {
        return item && item->timeSignatureSequence() == this;
    }

    bool TimeSignatureSequence::insertItem(TimeSignature *item) {
        if (!item || item->timeSignatureSequence()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(model());
        const auto conflict = TimeSignatureSequencePrivate::get(this)->itemAtIndex(item->index(), item->handle());
        if (conflict) {
            modelData->update(conflict, Schema::timeSignatureParent().column(), dini::Value::null());
        }
        modelData->update(item->handle(), Schema::timeSignatureParent().column(), orm::valueFromHandle(modelData->modelHandle));
        return true;
    }

    bool TimeSignatureSequence::removeItem(TimeSignature *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(model())->update(item->handle(), Schema::timeSignatureParent().column(), dini::Value::null());
        return true;
    }

    Model *TimeSignatureSequence::model() const {
        return TimeSignatureSequencePrivate::get(this)->model;
    }

    std::vector<opendspx::TimeSignature> TimeSignatureSequence::toOpenDSPX() const {
        std::vector<opendspx::TimeSignature> result;
        for (auto timeSignature = firstItem(); timeSignature; timeSignature = timeSignature->nextItem()) {
            result.push_back(timeSignature->toOpenDSPX());
        }
        return result;
    }

    void TimeSignatureSequence::fromOpenDSPX(const std::vector<opendspx::TimeSignature> &timeSignatures) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        for (const auto &source : timeSignatures) {
            auto timeSignature = model()->createTimeSignature();
            timeSignature->fromOpenDSPX(source);
            insertItem(timeSignature);
        }
    }

}
