#include "KeySignature.h"
#include "KeySignature_p.h"

#include <cstdint>

#include <dini//engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/private/ConversionUtils_p.h>
#include <dspxmodelORM/private/KeySignatureSequence_p.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <nlohmann/json.hpp>

namespace dspx {

    namespace {

        const std::vector<orm::ColumnBinding<KeySignature>> &keySignatureColumnBindings() {
            static const std::vector<orm::ColumnBinding<KeySignature>> bindings {
                orm::intFieldWithSignal<KeySignature, KeySignaturePrivate>(Schema::keySignaturePositionColumn(), &KeySignaturePrivate::position, &KeySignature::positionChanged),
                orm::intFieldWithSignal<KeySignature, KeySignaturePrivate>(Schema::keySignatureModeColumn(), &KeySignaturePrivate::mode, &KeySignature::modeChanged),
                orm::intFieldWithSignal<KeySignature, KeySignaturePrivate>(Schema::keySignatureTonalityColumn(), &KeySignaturePrivate::tonality, &KeySignature::tonalityChanged),
                orm::enumFieldWithSignal<KeySignature::AccidentalType, KeySignature, KeySignaturePrivate>(Schema::keySignatureAccidentalTypeColumn(), &KeySignaturePrivate::accidentalType, &KeySignature::accidentalTypeChanged),
                orm::previousNextFieldWithSignal<KeySignature, KeySignaturePrivate>(Schema::keySignaturePreviousItemColumn(), &KeySignaturePrivate::previousHandle, &KeySignaturePrivate::previous, &KeySignature::previousItemChanged),
                orm::previousNextFieldWithSignal<KeySignature, KeySignaturePrivate>(Schema::keySignatureNextItemColumn(), &KeySignaturePrivate::nextHandle, &KeySignaturePrivate::next, &KeySignature::nextItemChanged),
                {Schema::keySignatureParent().column(), [](KeySignature *q, const dini::Value &value) {
                     auto *model = ModelPrivate::get(q->model());
                     auto *d = KeySignaturePrivate::get(q);
                     auto *newSequence = model->isModelValue(value) ? model->keySignatures : nullptr;
                     const bool changed = d->sequence != newSequence;
                     d->sequence = newSequence;
                     return changed;
                 }, [](KeySignature *q) {
                     emit q->keySignatureSequenceChanged(KeySignaturePrivate::get(q)->sequence);
                 }},
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &keySignatureOrderSpec() {
            static const OrderSpec spec {{Schema::keySignaturePositionColumn()}, false};
            return spec;
        }

        const TableBinding &keySignatureTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<KeySignature, KeySignatureSequence>({
                .table = Schema::keySignatureTable(),
                .membershipColumns = {Schema::keySignatureParent().column()},
                .orderColumns = {Schema::keySignaturePositionColumn()},
                .moveSemantics = MoveSemantics::None,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<KeySignature>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<KeySignature>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.keySignatureObjects.remove(handle); },
                .sync = [](KeySignature *item, const dini::ItemSnapshot &snapshot, bool notify) { syncColumnBindings(keySignatureColumnBindings(), item, snapshot, notify); },
                .applyColumn = [](KeySignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyColumnBinding(keySignatureColumnBindings(), item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return model.isModelValue(orm::snapshotValue(snapshot, Schema::keySignatureParent().column())) ? model.keySignatures : nullptr;
                },
                .ownerForChange = [](ModelPrivate &model, const dini::ColumnUpdatedChange &change, bool oldValue) {
                    if (change.column == Schema::keySignatureParent().column()) {
                        return model.isModelValue(oldValue ? change.oldValue : change.newValue) ? model.keySignatures : nullptr;
                    }
                    return model.keySignatures;
                },
                .setOwner = [](KeySignature *item, KeySignatureSequence *owner, bool notify) { KeySignaturePrivate::get(item)->setSequence(owner, notify); },
                .refreshOwner = [](KeySignatureSequence *owner, bool notify) { KeySignatureSequencePrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](KeySignatureSequence *owner, KeySignature *item, KeySignatureSequence *) { emit owner->itemAboutToInsert(item); },
                .itemInserted = [](KeySignatureSequence *owner, KeySignature *item, KeySignatureSequence *) { emit owner->itemInserted(item); },
                .itemAboutToRemove = [](KeySignatureSequence *owner, KeySignature *item, KeySignatureSequence *) { emit owner->itemAboutToRemove(item); },
                .itemRemoved = [](KeySignatureSequence *owner, KeySignature *item, KeySignatureSequence *) { emit owner->itemRemoved(item); },
            });
            return binding;
        }

        void syncKeySignatureColumns(KeySignature *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(keySignatureColumnBindings(), item, snapshot, notify);
        }

        bool applyKeySignatureColumn(KeySignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            return applyColumnBinding(keySignatureColumnBindings(), item, column, value, notify);
        }

    }

    KeySignaturePrivate::KeySignaturePrivate(KeySignature *q) : q_ptr(q) {
    }

    void KeySignaturePrivate::setSequence(KeySignatureSequence *newSequence, bool notify) {
        Q_Q(KeySignature);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->keySignatureSequenceChanged(sequence);
        }
    }

    KeySignature::KeySignature(Handle handle, Model *model)
        : EntityObject(handle, model, model), d_ptr(new KeySignaturePrivate(this)) {
    }

    KeySignature::~KeySignature() = default;

    int KeySignature::position() const {
        Q_D(const KeySignature);
        return d->position;
    }

    void KeySignature::setPosition(int position) {
        auto *modelData = ModelPrivate::get(model());
        const auto itemId = orm::idFromHandle(handle());
        const auto snapshot = modelData->engine->read(itemId);
        const auto currentPosition = static_cast<int>(orm::snapshotValue(snapshot, Schema::keySignaturePositionColumn()).asInt64());
        if (currentPosition == position) {
            return;
        }
        modelData->update(handle(), Schema::keySignaturePositionColumn(), dini::Value(static_cast<std::int64_t>(position)));
    }

    int KeySignature::mode() const {
        Q_D(const KeySignature);
        return d->mode;
    }

    void KeySignature::setMode(int mode) {
        ModelPrivate::get(model())->update(handle(), Schema::keySignatureModeColumn(), dini::Value(static_cast<std::int64_t>(mode)));
    }

    int KeySignature::tonality() const {
        Q_D(const KeySignature);
        return d->tonality;
    }

    void KeySignature::setTonality(int tonality) {
        ModelPrivate::get(model())->update(handle(), Schema::keySignatureTonalityColumn(), dini::Value(static_cast<std::int64_t>(tonality)));
    }

    KeySignature::AccidentalType KeySignature::accidentalType() const {
        Q_D(const KeySignature);
        return d->accidentalType;
    }

    void KeySignature::setAccidentalType(AccidentalType accidentalType) {
        ModelPrivate::get(model())->update(handle(), Schema::keySignatureAccidentalTypeColumn(), dini::Value(static_cast<std::int64_t>(accidentalType)));
    }

    KeySignature *KeySignature::previousItem() const {
        Q_D(const KeySignature);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<KeySignature>(d->previousHandle);
        }
        return d->previous;
    }

    KeySignature *KeySignature::nextItem() const {
        Q_D(const KeySignature);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<KeySignature>(d->nextHandle);
        }
        return d->next;
    }

    KeySignatureSequence *KeySignature::keySignatureSequence() const {
        Q_D(const KeySignature);
        return d->sequence;
    }

    nlohmann::json KeySignature::toOpenDSPX() const {
        return nlohmann::json::object({
            {"pos", position()},
            {"mode", mode()},
            {"tonality", tonality()},
            {"accidentalType", accidentalType()},
        });
    }

    void KeySignature::fromOpenDSPX(const nlohmann::json &keySignature) {
        if (auto v = conv::optionalChain(keySignature, "pos"); v.is_number_integer() && v.get<int>() >= 0) {
            setPosition(v.get<int>());
        }
        if (auto v = conv::optionalChain(keySignature, "mode"); v.is_number_integer() && v.get<int>() >= 0 && v.get<int>() < 4096) {
            setMode(v.get<int>());
        }
        if (auto v = conv::optionalChain(keySignature, "tonality"); v.is_number_integer() && v.get<int>() >= 0 && v.get<int>() < 12) {
            setTonality(v.get<int>());
        }
        if (auto v = conv::optionalChain(keySignature, "accidentalType"); v.is_number_integer()) {
            const auto value = v.get<int>();
            if (value == Flat || value == Sharp) {
                setAccidentalType(static_cast<AccidentalType>(value));
            }
        }
    }

}


#include "moc_KeySignature.cpp"
