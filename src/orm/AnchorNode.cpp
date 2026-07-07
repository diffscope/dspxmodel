#include "AnchorNode.h"
#include "AnchorNode_p.h"

#include <cstdint>
#include <vector>

#include <dini/engine.h>
#include <opendspx/anchornode.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/AnchorNodeSequence.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/private/AnchorNodeSequence_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        AnchorNodeSequence *anchorNodeOwnerFromAssociationValue(ModelPrivate &model, const dini::Value &value) {
            const auto relationHandle = orm::handleFromValue(value);
            if (!relationHandle || !model.engine->contains(orm::idFromHandle(relationHandle))) {
                return nullptr;
            }
            const auto relation = model.engine->read(orm::idFromHandle(relationHandle));
            if (!orm::isContainer(relation, Schema::parameterAnchorNodeRelationTable())) {
                return nullptr;
            }
            const auto parameterHandle = orm::handleFromValue(orm::snapshotValue(relation, Schema::parameterAnchorNodeRelationParent().column()));
            const auto roleValue = orm::snapshotValue(relation, Schema::parameterAnchorNodeRelationRoleColumn());
            if (!parameterHandle || roleValue.isNull()) {
                return nullptr;
            }
            auto *parameter = model.ensure<Parameter>(parameterHandle);
            if (!parameter) {
                return nullptr;
            }
            const auto role = static_cast<AnchorNodeSequence::AnchorNodeRole>(roleValue.asInt64());
            if (role == AnchorNodeSequence::Transform) {
                return parameter->anchorTransform();
            }
            if (role == AnchorNodeSequence::Edited) {
                return parameter->anchorEdited();
            }
            return nullptr;
        }

        AnchorNodeSequence *anchorNodeOwnerFromSnapshot(ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
            return anchorNodeOwnerFromAssociationValue(model, orm::snapshotValue(snapshot, Schema::anchorNodeParent().column()));
        }

        const std::vector<orm::ColumnBinding<AnchorNode>> &anchorNodeColumnBindings() {
            static const std::vector<orm::ColumnBinding<AnchorNode>> bindings {
                orm::enumFieldWithSignal<AnchorNode::InterpolationMode, AnchorNode, AnchorNodePrivate>(
                    Schema::anchorNodeInterpolationModeColumn(), &AnchorNodePrivate::interpolationMode, &AnchorNode::interpolationModeChanged),
                orm::intFieldWithSignal<AnchorNode, AnchorNodePrivate>(Schema::anchorNodeXColumn(), &AnchorNodePrivate::x, &AnchorNode::xChanged),
                orm::intFieldWithSignal<AnchorNode, AnchorNodePrivate>(Schema::anchorNodeYColumn(), &AnchorNodePrivate::y, &AnchorNode::yChanged),
                orm::previousNextFieldWithSignal<AnchorNode, AnchorNodePrivate>(Schema::anchorNodePreviousItemColumn(), &AnchorNodePrivate::previousHandle, &AnchorNodePrivate::previous, &AnchorNode::previousItemChanged),
                orm::previousNextFieldWithSignal<AnchorNode, AnchorNodePrivate>(Schema::anchorNodeNextItemColumn(), &AnchorNodePrivate::nextHandle, &AnchorNodePrivate::next, &AnchorNode::nextItemChanged),
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &anchorNodeOrderSpec() {
            static const OrderSpec spec {{Schema::anchorNodeXColumn()}, true};
            return spec;
        }

        const TableBinding &anchorNodeTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<AnchorNode, AnchorNodeSequence>({
                .table = Schema::anchorNodeTable(),
                .membershipColumns = {Schema::anchorNodeParent().column()},
                .orderColumns = {Schema::anchorNodeXColumn()},
                .moveSemantics = MoveSemantics::BetweenOwners,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<AnchorNode>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<AnchorNode>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.anchorNodeObjects.remove(handle); },
                .sync = [](AnchorNode *item, const dini::ItemSnapshot &snapshot, bool notify) { syncAnchorNodeColumns(item, snapshot, notify); },
                .applyColumn = [](AnchorNode *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyAnchorNodeColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return anchorNodeOwnerFromSnapshot(model, snapshot);
                },
                .setOwner = [](AnchorNode *item, AnchorNodeSequence *owner, bool notify) { AnchorNodePrivate::get(item)->setSequence(owner, notify); },
                .refreshOwner = [](AnchorNodeSequence *owner, bool notify) { AnchorNodeSequencePrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](AnchorNodeSequence *owner, AnchorNode *item, AnchorNodeSequence *movedFrom) { emit owner->itemAboutToInsert(item, movedFrom); },
                .itemInserted = [](AnchorNodeSequence *owner, AnchorNode *item, AnchorNodeSequence *movedFrom) { emit owner->itemInserted(item, movedFrom); },
                .itemAboutToRemove = [](AnchorNodeSequence *owner, AnchorNode *item, AnchorNodeSequence *movedTo) { emit owner->itemAboutToRemove(item, movedTo); },
                .itemRemoved = [](AnchorNodeSequence *owner, AnchorNode *item, AnchorNodeSequence *movedTo) { emit owner->itemRemoved(item, movedTo); },
            });
            return binding;
        }

        void syncAnchorNodeColumns(AnchorNode *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(anchorNodeColumnBindings(), item, snapshot, notify);
            AnchorNodePrivate::get(item)->setPlacement(
                orm::handleFromValue(orm::snapshotValue(snapshot, Schema::anchorNodeParent().column())),
                notify);
        }

        bool applyAnchorNodeColumn(AnchorNode *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            auto *d = AnchorNodePrivate::get(item);
            if (column == Schema::anchorNodeParent().column()) {
                d->setPlacement(orm::handleFromValue(value), notify);
                return true;
            }
            return applyColumnBinding(anchorNodeColumnBindings(), item, column, value, notify);
        }

    }

    AnchorNodePrivate::AnchorNodePrivate(AnchorNode *q) : q_ptr(q) {
    }

    void AnchorNodePrivate::setPlacement(Handle newRelation, bool notify) {
        Q_Q(AnchorNode);
        auto *modelData = ModelPrivate::get(q->model());
        auto *newSequence = anchorNodeOwnerFromAssociationValue(*modelData, orm::valueFromHandle(newRelation));
        const bool changed = relationHandle != newRelation || sequence != newSequence;
        relationHandle = newRelation;
        sequence = newSequence;
        if (notify && changed) {
            emit q->anchorNodeSequenceChanged(sequence);
        }
    }

    void AnchorNodePrivate::setSequence(AnchorNodeSequence *newSequence, bool notify) {
        Q_Q(AnchorNode);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->anchorNodeSequenceChanged(sequence);
        }
    }

    AnchorNode::AnchorNode(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new AnchorNodePrivate(this)) {
    }

    AnchorNode::~AnchorNode() = default;

    AnchorNode::InterpolationMode AnchorNode::interpolationMode() const {
        Q_D(const AnchorNode);
        return d->interpolationMode;
    }

    void AnchorNode::setInterpolationMode(InterpolationMode interpolationMode) {
        ModelPrivate::get(model())->update(handle(), Schema::anchorNodeInterpolationModeColumn(), dini::Value(static_cast<std::int64_t>(interpolationMode)));
    }

    int AnchorNode::x() const {
        Q_D(const AnchorNode);
        return d->x;
    }

    void AnchorNode::setX(int x) {
        auto *modelData = ModelPrivate::get(model());
        Q_D(const AnchorNode);
        if (d->sequence) {
            const auto conflict = AnchorNodeSequencePrivate::get(d->sequence)->itemAtPosition(x, handle());
            if (conflict) {
                modelData->update(conflict, Schema::anchorNodeParent().column(), dini::Value::null());
            }
        }
        modelData->update(handle(), Schema::anchorNodeXColumn(), dini::Value(static_cast<std::int64_t>(x)));
    }

    int AnchorNode::y() const {
        Q_D(const AnchorNode);
        return d->y;
    }

    void AnchorNode::setY(int y) {
        ModelPrivate::get(model())->update(handle(), Schema::anchorNodeYColumn(), dini::Value(static_cast<std::int64_t>(y)));
    }

    AnchorNode *AnchorNode::previousItem() const {
        Q_D(const AnchorNode);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<AnchorNode>(d->previousHandle);
        }
        return d->previous;
    }

    AnchorNode *AnchorNode::nextItem() const {
        Q_D(const AnchorNode);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<AnchorNode>(d->nextHandle);
        }
        return d->next;
    }

    AnchorNodeSequence *AnchorNode::anchorNodeSequence() const {
        Q_D(const AnchorNode);
        return d->sequence;
    }

    opendspx::AnchorNode AnchorNode::toOpenDSPX() const {
        opendspx::AnchorNode target;
        target.x = x();
        target.y = y();
        switch (interpolationMode()) {
            case None:
                target.interp = opendspx::AnchorNode::Interpolation::None;
                break;
            case Linear:
                target.interp = opendspx::AnchorNode::Interpolation::Linear;
                break;
            case Hermite:
                target.interp = opendspx::AnchorNode::Interpolation::Hermite;
                break;
        }
        return target;
    }

    void AnchorNode::fromOpenDSPX(const opendspx::AnchorNode &node) {
        switch (node.interp) {
            case opendspx::AnchorNode::Interpolation::None:
                setInterpolationMode(None);
                break;
            case opendspx::AnchorNode::Interpolation::Linear:
                setInterpolationMode(Linear);
                break;
            case opendspx::AnchorNode::Interpolation::Hermite:
                setInterpolationMode(Hermite);
                break;
        }
        setX(node.x);
        setY(node.y);
    }

}

#include "moc_AnchorNode.cpp"
