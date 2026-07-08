#include "AnchorNodeSequence.h"
#include "AnchorNodeSequence_p.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <dini/engine.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AnchorNode.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec anchorNodeRelationQuery(Handle parameterHandle, AnchorNodeSequence::AnchorNodeRole role) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::parameterAnchorNodeRelationParent(), parameterHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::parameterAnchorNodeRelationRoleColumn()),
                                     dini::Value(static_cast<std::int64_t>(role))),
                }),
            };
        }

        dini::QuerySpec orderedAnchorNodeQuery(Handle relationHandle,
                                               dini::SortDirection direction = dini::SortDirection::Ascending) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::anchorNodeParent(), relationHandle),
                .sortKeys = orm::sortKeys(orm::anchorNodeOrderSpec(), direction),
            };
        }

        QList<AnchorNode *> anchorNodesFromView(ModelPrivate *model, const dini::View &view) {
            QList<AnchorNode *> result;
            for (const auto &snapshot : view.toVector()) {
                if (auto *item = model->ensure<AnchorNode>(snapshot)) {
                    result.append(item);
                }
            }
            return result;
        }

    }

    AnchorNodeSequencePrivate::AnchorNodeSequencePrivate(AnchorNodeSequence *q, Parameter *parameter, AnchorNodeSequence::AnchorNodeRole role)
        : q_ptr(q), parameter(parameter), role(role), jsIterable(new JSIterable(q, JSIterable::Sequence)) {
    }

    Handle AnchorNodeSequencePrivate::relationHandle() const {
        auto *modelData = ModelPrivate::get(parameter->model());
        if (auto relation = orm::firstSnapshot(modelData->engine->query(Schema::parameterAnchorNodeRelationTable(),
                                                                        anchorNodeRelationQuery(parameter->handle(), role)))) {
            return orm::handleFromId(relation->id);
        }
        return {};
    }

    dini::Value AnchorNodeSequencePrivate::associationValue() const {
        return orm::valueFromHandle(relationHandle());
    }

    void AnchorNodeSequencePrivate::refresh(bool notify) {
        Q_Q(AnchorNodeSequence);
        auto *modelData = ModelPrivate::get(parameter->model());
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
        const auto view = modelData->engine->query(Schema::anchorNodeTable(), orderedAnchorNodeQuery(relation));
        const auto newSize = static_cast<int>(view.count());
        AnchorNode *newFirst = nullptr;
        AnchorNode *newLast = nullptr;
        if (auto firstSnapshot = orm::firstSnapshot(view)) {
            newFirst = modelData->ensure<AnchorNode>(*firstSnapshot);
        }
        if (newSize > 0) {
            const auto lastView = modelData->engine->query(Schema::anchorNodeTable(), orderedAnchorNodeQuery(relation, dini::SortDirection::Descending));
            if (auto lastSnapshot = orm::firstSnapshot(lastView)) {
                newLast = modelData->ensure<AnchorNode>(*lastSnapshot);
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

    AnchorNodeSequence::AnchorNodeSequence(Parameter *parameter, AnchorNodeRole role)
        : QObject(parameter), d_ptr(new AnchorNodeSequencePrivate(this, parameter, role)) {
    }

    AnchorNodeSequence::~AnchorNodeSequence() = default;

    int AnchorNodeSequence::size() const {
        Q_D(const AnchorNodeSequence);
        return d->size;
    }

    AnchorNode *AnchorNodeSequence::firstItem() const {
        Q_D(const AnchorNodeSequence);
        return d->first;
    }

    AnchorNode *AnchorNodeSequence::lastItem() const {
        Q_D(const AnchorNodeSequence);
        return d->last;
    }

    QList<AnchorNode *> AnchorNodeSequence::slice(int position, int length) const {
        if (position < 0 || length < 0) {
            return {};
        }
        Q_D(const AnchorNodeSequence);
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(parameter()->model());
        auto filter = dini::FilterExpression::all({
            orm::parentFilter(Schema::anchorNodeParent(), relation),
            dini::FilterExpression(dini::Filter(dini::FieldRef::column(Schema::anchorNodeXColumn()),
                                                dini::ComparisonOperator::Less,
                                                dini::Value(static_cast<std::int64_t>(position + length)))),
        });
        const auto view = modelData->engine->query(Schema::anchorNodeTable(), {
            .filter = std::move(filter),
            .sortKeys = orm::sortKeys(orm::anchorNodeOrderSpec()),
        });
        auto nodes = anchorNodesFromView(modelData, view);
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [position](AnchorNode *node) {
            return !node || node->x() < position;
        }), nodes.end());
        return nodes;
    }

    bool AnchorNodeSequence::contains(AnchorNode *item) const {
        return item && item->anchorNodeSequence() == this;
    }

    bool AnchorNodeSequence::insertItem(AnchorNode *item) {
        if (!item || item->model() != parameter()->model() || item->anchorNodeSequence()) {
            return false;
        }
        Q_D(const AnchorNodeSequence);
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(parameter()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::anchorNodeParent().column(), .value = associationValue},
        });
        return true;
    }

    bool AnchorNodeSequence::removeItem(AnchorNode *item) {
        if (!contains(item)) {
            return false;
        }
        ModelPrivate::get(parameter()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::anchorNodeParent().column(), .value = dini::Value::null()},
        });
        return true;
    }

    bool AnchorNodeSequence::moveItem(AnchorNode *item, AnchorNodeSequence *sequence) {
        if (!contains(item) || !sequence || sequence->parameter()->model() != parameter()->model() || sequence->contains(item)) {
            return false;
        }
        const auto associationValue = AnchorNodeSequencePrivate::get(sequence)->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(parameter()->model())->update(item->handle(), {
            dini::ColumnValue {.column = Schema::anchorNodeParent().column(), .value = associationValue},
        });
        return true;
    }

    AnchorNodeSequence::AnchorNodeRole AnchorNodeSequence::role() const {
        Q_D(const AnchorNodeSequence);
        return d->role;
    }

    Parameter *AnchorNodeSequence::parameter() const {
        Q_D(const AnchorNodeSequence);
        return d->parameter;
    }

}

#include "moc_AnchorNodeSequence.cpp"
