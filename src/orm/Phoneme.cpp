#include "Phoneme.h"
#include "Phoneme_p.h"

#include <cstdint>
#include <vector>

#include <dini/engine.h>
#include <opendspx/phoneme.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/PhonemeSequence.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/Note_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>
#include <dspxmodelORM/private/PhonemeSequence_p.h>

namespace dspx {

    namespace {

        PhonemeSequence *phonemeOwnerFromAssociationValue(ModelPrivate &model, const dini::Value &value) {
            const auto relationHandle = orm::handleFromValue(value);
            if (!relationHandle || !model.engine->contains(orm::idFromHandle(relationHandle))) {
                return nullptr;
            }
            const auto relation = model.engine->read(orm::idFromHandle(relationHandle));
            if (!orm::isContainer(relation, Schema::notePhonemeRelationTable())) {
                return nullptr;
            }
            const auto noteHandle = orm::handleFromValue(orm::snapshotValue(relation, Schema::notePhonemeRelationParent().column()));
            const auto roleValue = orm::snapshotValue(relation, Schema::notePhonemeRelationRoleColumn());
            if (!noteHandle || roleValue.isNull()) {
                return nullptr;
            }
            auto *note = model.ensure<Note>(noteHandle);
            if (!note) {
                return nullptr;
            }
            const auto role = static_cast<PhonemeSequence::PhonemeRole>(roleValue.asInt64());
            if (role == PhonemeSequence::Original) {
                return note->originalPhonemes();
            }
            if (role == PhonemeSequence::Edited) {
                return note->editedPhonemes();
            }
            return nullptr;
        }

        PhonemeSequence *phonemeOwnerFromSnapshot(ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
            return phonemeOwnerFromAssociationValue(model, orm::snapshotValue(snapshot, Schema::phonemeParent().column()));
        }

        const std::vector<orm::ColumnBinding<Phoneme>> &phonemeColumnBindings() {
            static const std::vector<orm::ColumnBinding<Phoneme>> bindings {
                orm::stringFieldWithSignal<Phoneme, PhonemePrivate>(Schema::phonemeLanguageColumn(), &PhonemePrivate::language, &Phoneme::languageChanged, &Phoneme::languageChangedAfterCommit),
                orm::intFieldWithSignal<Phoneme, PhonemePrivate>(Schema::phonemeStartColumn(), &PhonemePrivate::start, &Phoneme::startChanged, &Phoneme::startChangedAfterCommit),
                orm::stringFieldWithSignal<Phoneme, PhonemePrivate>(Schema::phonemeTokenColumn(), &PhonemePrivate::token, &Phoneme::tokenChanged, &Phoneme::tokenChangedAfterCommit),
                orm::boolFieldWithSignal<Phoneme, PhonemePrivate>(Schema::phonemeOnsetColumn(), &PhonemePrivate::onset, &Phoneme::onsetChanged, &Phoneme::onsetChangedAfterCommit),
                orm::previousNextFieldWithSignal<Phoneme, PhonemePrivate>(Schema::phonemePreviousItemColumn(), &PhonemePrivate::previousHandle, &PhonemePrivate::previous, &Phoneme::previousItemChanged, &Phoneme::previousItemChangedAfterCommit),
                orm::previousNextFieldWithSignal<Phoneme, PhonemePrivate>(Schema::phonemeNextItemColumn(), &PhonemePrivate::nextHandle, &PhonemePrivate::next, &Phoneme::nextItemChanged, &Phoneme::nextItemChangedAfterCommit),
            };
            return bindings;
        }

    }

    namespace orm {

        const OrderSpec &phonemeOrderSpec() {
            static const OrderSpec spec {{Schema::phonemeStartColumn()}, true};
            return spec;
        }

        const TableBinding &phonemeTableBinding() {
            static const TableBinding binding = makeAssociatedTableBinding<Phoneme, PhonemeSequence>({
                .table = Schema::phonemeTable(),
                .membershipColumns = {Schema::phonemeParent().column()},
                .orderColumns = {Schema::phonemeStartColumn()},
                .moveSemantics = MoveSemantics::BetweenOwners,
                .ensure = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) { return model.ensure<Phoneme>(snapshot); },
                .find = [](ModelPrivate &model, Handle handle) { return model.find<Phoneme>(handle); },
                .removeObject = [](ModelPrivate &model, Handle handle) { model.phonemeObjects.remove(handle); },
                .sync = [](Phoneme *item, const dini::ItemSnapshot &snapshot, bool notify) { syncPhonemeColumns(item, snapshot, notify); },
                .applyColumn = [](Phoneme *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) { return applyPhonemeColumn(item, column, value, notify); },
                .ownerForSnapshot = [](ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
                    return phonemeOwnerFromSnapshot(model, snapshot);
                },
                .setOwner = [](Phoneme *item, PhonemeSequence *owner, bool notify) { PhonemePrivate::get(item)->setSequence(owner, notify); },
                .ownerChangedAfterCommit = [](Phoneme *item, PhonemeSequence *owner) { emit item->phonemeSequenceChangedAfterCommit(owner); },
                .refreshOwner = [](PhonemeSequence *owner, bool notify) { PhonemeSequencePrivate::get(owner)->refresh(notify); },
                .refreshOwnerAfterCommit = [](PhonemeSequence *owner, bool sizeChanged, bool orderChanged) {
                    if (sizeChanged) emit owner->sizeChangedAfterCommit(owner->size());
                    if (orderChanged) {
                        emit owner->firstItemChangedAfterCommit(owner->firstItem());
                        emit owner->lastItemChangedAfterCommit(owner->lastItem());
                    }
                },
                .itemAboutToInsert = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedFrom) { emit owner->itemAboutToInsert(item, movedFrom); },
                .itemInserted = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedFrom) { emit owner->itemInserted(item, movedFrom); },
                .itemAboutToRemove = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedTo) { emit owner->itemAboutToRemove(item, movedTo); },
                .itemRemoved = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedTo) { emit owner->itemRemoved(item, movedTo); },
                .itemAboutToInsertAfterCommit = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedFrom) { emit owner->itemAboutToInsertAfterCommit(item, movedFrom); },
                .itemInsertedAfterCommit = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedFrom) { emit owner->itemInsertedAfterCommit(item, movedFrom); },
                .itemAboutToRemoveAfterCommit = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedTo) { emit owner->itemAboutToRemoveAfterCommit(item, movedTo); },
                .itemRemovedAfterCommit = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedTo) { emit owner->itemRemovedAfterCommit(item, movedTo); },
            });
            return binding;
        }

        void syncPhonemeColumns(Phoneme *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(phonemeColumnBindings(), item, snapshot, notify);
            PhonemePrivate::get(item)->setPlacement(
                orm::handleFromValue(orm::snapshotValue(snapshot, Schema::phonemeParent().column())),
                notify);
        }

        bool applyPhonemeColumn(Phoneme *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            auto *d = PhonemePrivate::get(item);
            if (column == Schema::phonemeParent().column()) {
                if (currentNotificationStage == NotificationStage::AfterCommit) {
                    emit item->phonemeSequenceChangedAfterCommit(d->sequence);
                    return true;
                }
                d->setPlacement(orm::handleFromValue(value), notify);
                return true;
            }
            return applyColumnBinding(phonemeColumnBindings(), item, column, value, notify);
        }

    }

    PhonemePrivate::PhonemePrivate(Phoneme *q) : q_ptr(q) {
    }

    void PhonemePrivate::setPlacement(Handle newRelation, bool notify) {
        Q_Q(Phoneme);
        auto *modelData = ModelPrivate::get(q->model());
        auto *newSequence = phonemeOwnerFromAssociationValue(*modelData, orm::valueFromHandle(newRelation));
        const bool changed = relationHandle != newRelation || sequence != newSequence;
        relationHandle = newRelation;
        sequence = newSequence;
        if (notify && changed) {
            emit q->phonemeSequenceChanged(sequence);
        }
    }

    void PhonemePrivate::setSequence(PhonemeSequence *newSequence, bool notify) {
        Q_Q(Phoneme);
        if (sequence == newSequence) {
            return;
        }
        sequence = newSequence;
        if (notify) {
            emit q->phonemeSequenceChanged(sequence);
        }
    }

    Phoneme::Phoneme(Handle handle, Model *model) : EntityObject(handle, model, model), d_ptr(new PhonemePrivate(this)) {
    }

    Phoneme::~Phoneme() = default;

    QString Phoneme::language() const {
        Q_D(const Phoneme);
        return d->language;
    }

    void Phoneme::setLanguage(const QString &language) {
        ModelPrivate::get(model())->update(handle(), Schema::phonemeLanguageColumn(), orm::valueFromString(language));
    }

    int Phoneme::start() const {
        Q_D(const Phoneme);
        return d->start;
    }

    void Phoneme::setStart(int start) {
        ModelPrivate::get(model())->update(handle(), Schema::phonemeStartColumn(), dini::Value(static_cast<std::int64_t>(start)));
    }

    QString Phoneme::token() const {
        Q_D(const Phoneme);
        return d->token;
    }

    void Phoneme::setToken(const QString &token) {
        ModelPrivate::get(model())->update(handle(), Schema::phonemeTokenColumn(), orm::valueFromString(token));
    }

    bool Phoneme::onset() const {
        Q_D(const Phoneme);
        return d->onset;
    }

    void Phoneme::setOnset(bool onset) {
        ModelPrivate::get(model())->update(handle(), Schema::phonemeOnsetColumn(), dini::Value(onset));
    }

    Phoneme *Phoneme::previousItem() const {
        Q_D(const Phoneme);
        if (!d->previous && d->previousHandle) {
            d->previous = ModelPrivate::get(model())->ensure<Phoneme>(d->previousHandle);
        }
        return d->previous;
    }

    Phoneme *Phoneme::nextItem() const {
        Q_D(const Phoneme);
        if (!d->next && d->nextHandle) {
            d->next = ModelPrivate::get(model())->ensure<Phoneme>(d->nextHandle);
        }
        return d->next;
    }

    PhonemeSequence *Phoneme::phonemeSequence() const {
        Q_D(const Phoneme);
        return d->sequence;
    }

    opendspx::Phoneme Phoneme::toOpenDSPX() const {
        return opendspx::Phoneme {
            .language = language().toStdString(),
            .token = token().toStdString(),
            .start = start(),
            .onset = onset(),
        };
    }

    void Phoneme::fromOpenDSPX(const opendspx::Phoneme &phoneme) {
        setLanguage(QString::fromStdString(phoneme.language));
        setToken(QString::fromStdString(phoneme.token));
        setStart(phoneme.start);
        setOnset(phoneme.onset);
    }

}


#include "moc_Phoneme.cpp"
