#include "Tempo.h"
#include "Tempo_p.h"

#include <cstdint>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/tempo.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/TempoSequence_p.h>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<Tempo>> &tempoColumnBindings() {
            static const std::vector<orm::ColumnBinding<Tempo>> bindings {
                orm::intFieldWithSignal<Tempo, TempoPrivate>(Schema::tempoPositionColumn(), &TempoPrivate::position, &Tempo::positionChanged),
                orm::doubleFieldWithSignal<Tempo, TempoPrivate>(Schema::tempoValueColumn(), &TempoPrivate::value, &Tempo::valueChanged),
                orm::previousNextFieldWithSignal<Tempo, TempoPrivate>(Schema::tempoPreviousItemColumn(), &TempoPrivate::previousHandle, &TempoPrivate::previous, &Tempo::previousItemChanged),
                orm::previousNextFieldWithSignal<Tempo, TempoPrivate>(Schema::tempoNextItemColumn(), &TempoPrivate::nextHandle, &TempoPrivate::next, &Tempo::nextItemChanged),
                {Schema::tempoParent().column(), [](Tempo *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = TempoPrivate::get(q);
                     auto *newSequence = model->isModelValue(value) ? model->tempos : nullptr;
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](Tempo *q) {
                     emit q->tempoSequenceChanged(TempoPrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &tempoOrderSpec() {
            static const OrderSpec spec {{Schema::tempoPositionColumn()}, false};
            return spec;
        }

        const TableBinding &tempoTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<Tempo, TempoSequence>({
                .table = Schema::tempoTable(),
                .membershipColumns = {Schema::tempoParent().column()},
                .orderColumns = {Schema::tempoPositionColumn()},
                .moveSemantics = MoveSemantics::None,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Tempo>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Tempo>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.tempoObjects.remove(handle); },
                .sync = [](Tempo *item, const dini::ItemSnapshot &snapshot, bool notify) { syncTempoColumns(item, snapshot, notify); },
                .applyColumn = [](Tempo *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyTempoColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return model.isModelValue(orm::snapshotValue(snapshot, Schema::tempoParent().column())) ? model.tempos : nullptr;
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::tempoParent().column()) {
                        return model.isModelValue(oldValue ? change.oldValue : change.newValue) ? model.tempos : nullptr;
                    }
                    return model.tempos;
                },
                .setOwner = [](Tempo *item, TempoSequence *owner, bool notify) { TempoPrivate::get(item)->setSequence(owner, notify); },
                .refreshOwner = [](TempoSequence *owner, bool notify) { TempoSequencePrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](TempoSequence *owner, Tempo *item, TempoSequence *) { emit owner->itemAboutToInsert(item); },
                .itemInserted = [](TempoSequence *owner, Tempo *item, TempoSequence *) { emit owner->itemInserted(item); },
                .itemAboutToRemove = [](TempoSequence *owner, Tempo *item, TempoSequence *) { emit owner->itemAboutToRemove(item); },
                .itemRemoved = [](TempoSequence *owner, Tempo *item, TempoSequence *) { emit owner->itemRemoved(item); },
            });
            return binding;
        }

        void syncTempoColumns(Tempo *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(tempoColumnBindings(), item, snapshot, notify);
        }

        bool applyTempoColumn(Tempo *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(tempoColumnBindings(), item, column, value, notify);
        }

    }

    TempoPrivate::TempoPrivate(Tempo *q) : q_ptr(q) {
    }

    void TempoPrivate::setSequence(TempoSequence *newSequence, bool notify) {
        Q_Q(Tempo);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->tempoSequenceChanged(sequence);
        }
    }

    Tempo::Tempo(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new TempoPrivate(this)) {
    }

    Tempo::~Tempo() = default;

    int Tempo::position() const {
        Q_D(const Tempo);
        return d->position;
    }

    void Tempo::setPosition(int position) {
        auto *modelData = ModelPrivate::get(model());
        const auto itemId = orm::idFromHandle(handle());
        const auto snapshot = modelData->engine->read(itemId);
        const auto currentPosition = static_cast<int>(orm::snapshotValue(snapshot, Schema::tempoPositionColumn()).asInt64());
        if (currentPosition == position) {
            return;
        }
        const bool contained = modelData->isModelValue(orm::snapshotValue(snapshot, Schema::tempoParent().column()));
        if (contained && position >= 0) {
            const auto conflict = TempoSequencePrivate::get(modelData->tempos)->itemAtPosition(position, handle());
            if (conflict) {
                modelData->update(conflict, Schema::tempoParent().column(), dini::Value::null());
            }
        }
        modelData->update(handle(), Schema::tempoPositionColumn(), dini::Value(static_cast<std::int64_t>(position)));
    }

    double Tempo::value() const {
        Q_D(const Tempo);
        return d->value;
    }

    void Tempo::setValue(double value) {
        ModelPrivate::get(model())->update(handle(), Schema::tempoValueColumn(), dini::Value(value));
    }

    Tempo *Tempo::previousItem() const {
        Q_D(const Tempo);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<Tempo>(d->previousHandle);
        }
        return d->previous;
    }

    Tempo *Tempo::nextItem() const {
        Q_D(const Tempo);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<Tempo>(d->nextHandle);
        }
        return d->next;
    }

    TempoSequence *Tempo::tempoSequence() const {
        Q_D(const Tempo);
        return d->sequence;
    }

    opendspx::Tempo Tempo::toOpenDSPX() const {
        return {
            .pos = position(),
            .value = value(),
        };
    }

    void Tempo::fromOpenDSPX(const opendspx::Tempo &tempo) {
        setPosition(tempo.pos);
        setValue(tempo.value);
    }

}


#include "moc_Tempo.cpp"
