#include "Label.h"
#include "Label_p.h"

#include <cstdint>

#include <dini/transaction.h>
#include <opendspx/model.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/LabelSequence_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<Label>> &labelColumnBindings() {
            static const std::vector<orm::ColumnBinding<Label>> bindings {
                orm::intField<Label, LabelPrivate>(Schema::labelPositionColumn(), &LabelPrivate::position, &Label::positionChanged),
                orm::stringField<Label, LabelPrivate>(Schema::labelTextColumn(), &LabelPrivate::text, &Label::textChanged),
                orm::previousNextField<Label, LabelPrivate>(Schema::labelPreviousItemColumn(), &LabelPrivate::previousHandle, &LabelPrivate::previous, &Label::previousItemChanged),
                orm::previousNextField<Label, LabelPrivate>(Schema::labelNextItemColumn(), &LabelPrivate::nextHandle, &LabelPrivate::next, &Label::nextItemChanged),
                {Schema::labelParent().column(), [](Label *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = LabelPrivate::get(q);
                     auto *newSequence = model->isModelValue(value) ? model->labels : nullptr;
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](Label *q) {
                     emit q->labelSequenceChanged(LabelPrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &labelOrderSpec() {
            static const OrderSpec spec {{Schema::labelPositionColumn()}, true};
            return spec;
        }

        const TableBinding &labelTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<Label, LabelSequence>({
                .table = Schema::labelTable(),
                .membershipColumns = {Schema::labelParent().column()},
                .orderColumns = {Schema::labelPositionColumn()},
                .moveSemantics = MoveSemantics::None,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Label>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Label>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.labelObjects.remove(handle); },
                .sync = [](Label *item, const dini::ItemSnapshot &snapshot, bool notify) { syncLabelColumns(item, snapshot, notify); },
                .applyColumn = [](Label *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyLabelColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return model.isModelValue(orm::snapshotValue(snapshot, Schema::labelParent().column())) ? model.labels : nullptr;
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::labelParent().column()) {
                        return model.isModelValue(oldValue ? change.oldValue : change.newValue) ? model.labels : nullptr;
                    }
                    return model.labels;
                },
                .setOwner = [](Label *item, LabelSequence *owner, bool notify) { LabelPrivate::get(item)->setSequence(owner, notify); },
                .refreshOwner = [](LabelSequence *owner, bool notify) { LabelSequencePrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](LabelSequence *owner, Label *item, LabelSequence *) { emit owner->itemAboutToInsert(item); },
                .itemInserted = [](LabelSequence *owner, Label *item, LabelSequence *) { emit owner->itemInserted(item); },
                .itemAboutToRemove = [](LabelSequence *owner, Label *item, LabelSequence *) { emit owner->itemAboutToRemove(item); },
                .itemRemoved = [](LabelSequence *owner, Label *item, LabelSequence *) { emit owner->itemRemoved(item); },
            });
            return binding;
        }

        void syncLabelColumns(Label *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(labelColumnBindings(), item, snapshot, notify);
        }

        bool applyLabelColumn(Label *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(labelColumnBindings(), item, column, value, notify);
        }

    }

    LabelPrivate::LabelPrivate(Label *q) : q_ptr(q) {
    }

    void LabelPrivate::setSequence(LabelSequence *newSequence, bool notify) {
        Q_Q(Label);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->labelSequenceChanged(sequence);
        }
    }

    Label::Label(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new LabelPrivate(this)) {
    }

    Label::~Label() = default;

    int Label::position() const {
        Q_D(const Label);
        return d->position;
    }

    void Label::setPosition(int position) {
        ModelPrivate::get(model())->update(handle(), Schema::labelPositionColumn(), dini::Value(static_cast<std::int64_t>(position)));
    }

    QString Label::text() const {
        Q_D(const Label);
        return d->text;
    }

    void Label::setText(const QString &text) {
        ModelPrivate::get(model())->update(handle(), Schema::labelTextColumn(), orm::valueFromString(text));
    }

    Label *Label::previousItem() const {
        Q_D(const Label);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<Label>(d->previousHandle);
        }
        return d->previous;
    }

    Label *Label::nextItem() const {
        Q_D(const Label);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<Label>(d->nextHandle);
        }
        return d->next;
    }

    LabelSequence *Label::labelSequence() const {
        Q_D(const Label);
        return d->sequence;
    }

    opendspx::Label Label::toOpenDSPX() const {
        return {
            .pos = position(),
            .text = text().toStdString(),
        };
    }

    void Label::fromOpenDSPX(const opendspx::Label &label) {
        setPosition(label.pos);
        setText(QString::fromStdString(label.text));
    }

}
