#include "LabelSequence.h"
#include "LabelSequence_p.h"

#include <cstdint>
#include <utility>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/model.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/Label_p.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec orderedLabelQuery(Handle modelHandle, dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::labelParent(), modelHandle),
                .sortKeys = orm::sortKeys(orm::labelOrderSpec(), direction),
            };
        }

        QList<Label *> labelsFromView(ModelPrivate *model, const dini::View &view) {
            QList<Label *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<Label>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    LabelSequencePrivate::LabelSequencePrivate(LabelSequence *q, Model *model) : q_ptr(q), model(model), jsIterable(new JSIterable(q, JSIterable::Sequence)) {
    }

    void LabelSequencePrivate::refresh(bool notify) {
        Q_Q(LabelSequence);
        auto *modelData = ModelPrivate::get(model);
        const auto view = modelData->engine->query(Schema::labelTable(), orderedLabelQuery(modelData->modelHandle));
        const auto newSize = static_cast<int>(view.count());
        Label *newFirst = nullptr;
        Label *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<Label>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::labelTable(), orderedLabelQuery(modelData->modelHandle, dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<Label>(*lastSnapshot);
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

    LabelSequence::LabelSequence(Model *model) : QObject(model), d_ptr(new LabelSequencePrivate(this, model)) {
    }

    LabelSequence::~LabelSequence() = default;

    int LabelSequence::size() const {
        Q_D(const LabelSequence);
        return d->size;
    }

    Label *LabelSequence::firstItem() const {
        Q_D(const LabelSequence);
        return d->first;
    }

    Label *LabelSequence::lastItem() const {
        Q_D(const LabelSequence);
        return d->last;
    }

    QList<Label *> LabelSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        auto *modelData = ModelPrivate::get(model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::labelParent(), modelData->modelHandle),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::labelPositionColumn()), dini::ComparisonOperator::GreaterOrEqual, dini::Value(static_cast<std::int64_t>(position)))),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::labelPositionColumn()), dini::ComparisonOperator::Less, dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::labelTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::labelOrderSpec()),
        });
        return labelsFromView(modelData, view);
    }

    bool LabelSequence::contains(Label *item) const {
        return item && item->labelSequence() == this;
    }

    bool LabelSequence::insertItem(Label *item) {
        if (!item || item->labelSequence()) {
            return false;
        }
        auto *modelData = ModelPrivate::get(model());
        modelData->update(item->handle(), Schema::labelParent().column(), orm::valueFromHandle(modelData->modelHandle));
        return true;
    }

    bool LabelSequence::removeItem(Label *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(model())->update(item->handle(), Schema::labelParent().column(), dini::Value::null());
        return true;
    }

    Model *LabelSequence::model() const {
        return LabelSequencePrivate::get(this)->model;
    }

    std::vector<opendspx::Label> LabelSequence::toOpenDSPX() const {
        std::vector<opendspx::Label> result;
        for (auto label = firstItem(); label; label = label->nextItem()) {
            result.push_back(label->toOpenDSPX());
        }
        return result;
    }

    void LabelSequence::fromOpenDSPX(const std::vector<opendspx::Label> &labels) {
        while (size() > 0) {
            removeItem(firstItem());
        }
        for (const auto &source : labels) {
            auto label = model()->createLabel();
            label->fromOpenDSPX(source);
            insertItem(label);
        }
    }

}


#include "moc_LabelSequence.cpp"
