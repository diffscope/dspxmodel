#include "TimeSignature.h"
#include "TimeSignature_p.h"

#include <cstdint>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/timesignature.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/TimeSignatureSequence_p.h>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<TimeSignature>> &timeSignatureColumnBindings() {
            static const std::vector<orm::ColumnBinding<TimeSignature>> bindings {
                orm::intField<TimeSignature, TimeSignaturePrivate>(Schema::timeSignatureIndexColumn(), &TimeSignaturePrivate::index, &TimeSignature::indexChanged),
                orm::intField<TimeSignature, TimeSignaturePrivate>(Schema::timeSignatureNumeratorColumn(), &TimeSignaturePrivate::numerator, &TimeSignature::numeratorChanged),
                orm::intField<TimeSignature, TimeSignaturePrivate>(Schema::timeSignatureDenominatorColumn(), &TimeSignaturePrivate::denominator, &TimeSignature::denominatorChanged),
                orm::previousNextField<TimeSignature, TimeSignaturePrivate>(Schema::timeSignaturePreviousItemColumn(), &TimeSignaturePrivate::previousHandle, &TimeSignaturePrivate::previous, &TimeSignature::previousItemChanged),
                orm::previousNextField<TimeSignature, TimeSignaturePrivate>(Schema::timeSignatureNextItemColumn(), &TimeSignaturePrivate::nextHandle, &TimeSignaturePrivate::next, &TimeSignature::nextItemChanged),
                {Schema::timeSignatureParent().column(), [](TimeSignature *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = TimeSignaturePrivate::get(q);
                     auto *newSequence = model->isModelValue(value) ? model->timeSignatures : nullptr;
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](TimeSignature *q) {
                     emit q->timeSignatureSequenceChanged(TimeSignaturePrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &timeSignatureOrderSpec() {
            static const OrderSpec spec {{Schema::timeSignatureIndexColumn()}, false};
            return spec;
        }

        const TableBinding &timeSignatureTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<TimeSignature, TimeSignatureSequence>({
                .table = Schema::timeSignatureTable(),
                .membershipColumns = {Schema::timeSignatureParent().column()},
                .orderColumns = {Schema::timeSignatureIndexColumn()},
                .moveSemantics = MoveSemantics::None,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<TimeSignature>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<TimeSignature>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.timeSignatureObjects.remove(handle); },
                .sync = [](TimeSignature *item, const dini::ItemSnapshot &snapshot, bool notify) { syncTimeSignatureColumns(item, snapshot, notify); },
                .applyColumn = [](TimeSignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyTimeSignatureColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return model.isModelValue(orm::snapshotValue(snapshot, Schema::timeSignatureParent().column())) ? model.timeSignatures : nullptr;
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::timeSignatureParent().column()) {
                        return model.isModelValue(oldValue ? change.oldValue : change.newValue) ? model.timeSignatures : nullptr;
                    }
                    return model.timeSignatures;
                },
                .setOwner = [](TimeSignature *item, TimeSignatureSequence *owner, bool notify) { TimeSignaturePrivate::get(item)->setSequence(owner, notify); },
                .refreshOwner = [](TimeSignatureSequence *owner, bool notify) { TimeSignatureSequencePrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](TimeSignatureSequence *owner, TimeSignature *item, TimeSignatureSequence *) { emit owner->itemAboutToInsert(item); },
                .itemInserted = [](TimeSignatureSequence *owner, TimeSignature *item, TimeSignatureSequence *) { emit owner->itemInserted(item); },
                .itemAboutToRemove = [](TimeSignatureSequence *owner, TimeSignature *item, TimeSignatureSequence *) { emit owner->itemAboutToRemove(item); },
                .itemRemoved = [](TimeSignatureSequence *owner, TimeSignature *item, TimeSignatureSequence *) { emit owner->itemRemoved(item); },
            });
            return binding;
        }

        void syncTimeSignatureColumns(TimeSignature *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(timeSignatureColumnBindings(), item, snapshot, notify);
        }

        bool applyTimeSignatureColumn(TimeSignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(timeSignatureColumnBindings(), item, column, value, notify);
        }

    }

    TimeSignaturePrivate::TimeSignaturePrivate(TimeSignature *q) : q_ptr(q) {
    }

    void TimeSignaturePrivate::setSequence(TimeSignatureSequence *newSequence, bool notify) {
        Q_Q(TimeSignature);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->timeSignatureSequenceChanged(sequence);
        }
    }

    TimeSignature::TimeSignature(Handle handle, Model *model)
        : EntityObject(handle, model, model), d_ptr(new TimeSignaturePrivate(this)) {
    }

    TimeSignature::~TimeSignature() = default;

    int TimeSignature::index() const {
        Q_D(const TimeSignature);
        return d->index;
    }

    void TimeSignature::setIndex(int index) {
        auto *modelData = ModelPrivate::get(model());
        const auto itemId = orm::idFromHandle(handle());
        const auto snapshot = modelData->engine->read(itemId);
        const auto currentIndex = static_cast<int>(orm::snapshotValue(snapshot, Schema::timeSignatureIndexColumn()).asInt64());
        if (currentIndex == index) {
            return;
        }
        const bool contained = modelData->isModelValue(orm::snapshotValue(snapshot, Schema::timeSignatureParent().column()));
        if (contained && index >= 0) {
            const auto conflict = TimeSignatureSequencePrivate::get(modelData->timeSignatures)->itemAtIndex(index, handle());
            if (conflict) {
                modelData->update(conflict, Schema::timeSignatureParent().column(), dini::Value::null());
            }
        }
        modelData->update(handle(), Schema::timeSignatureIndexColumn(), dini::Value(static_cast<std::int64_t>(index)));
    }

    int TimeSignature::numerator() const {
        Q_D(const TimeSignature);
        return d->numerator;
    }

    void TimeSignature::setNumerator(int numerator) {
        ModelPrivate::get(model())->update(handle(), Schema::timeSignatureNumeratorColumn(), dini::Value(static_cast<std::int64_t>(numerator)));
    }

    int TimeSignature::denominator() const {
        Q_D(const TimeSignature);
        return d->denominator;
    }

    void TimeSignature::setDenominator(int denominator) {
        ModelPrivate::get(model())->update(handle(), Schema::timeSignatureDenominatorColumn(), dini::Value(static_cast<std::int64_t>(denominator)));
    }

    TimeSignature *TimeSignature::previousItem() const {
        Q_D(const TimeSignature);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<TimeSignature>(d->previousHandle);
        }
        return d->previous;
    }

    TimeSignature *TimeSignature::nextItem() const {
        Q_D(const TimeSignature);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<TimeSignature>(d->nextHandle);
        }
        return d->next;
    }

    TimeSignatureSequence *TimeSignature::timeSignatureSequence() const {
        Q_D(const TimeSignature);
        return d->sequence;
    }

    opendspx::TimeSignature TimeSignature::toOpenDSPX() const {
        return {
            .index = index(),
            .numerator = numerator(),
            .denominator = denominator(),
        };
    }

    void TimeSignature::fromOpenDSPX(const opendspx::TimeSignature &timeSignature) {
        setIndex(timeSignature.index);
        setNumerator(timeSignature.numerator);
        setDenominator(timeSignature.denominator);
    }

}
