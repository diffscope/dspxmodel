#include "Phoneme.h"
#include "Phoneme_p.h"

#include <cstdint>
#include <optional>

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

        std::optional<PhonemeSequence::PhonemeRole> roleFromValue(const dini::Value &value) {
            if (value.isNull()) {
                return {};
            }
            return static_cast<PhonemeSequence::PhonemeRole>(value.asInt64());
        }

        PhonemeSequence *phonemeOwnerFromPlacement(ModelPrivate &model, Handle noteHandle, std::optional<PhonemeSequence::PhonemeRole> role) {
            if (!noteHandle || !role.has_value()) {
                return nullptr;
            }
            auto *note = model.ensure<Note>(noteHandle);
            if (!note) {
                return nullptr;
            }
            return *role == PhonemeSequence::Edited ? note->editedPhonemes() : note->originalPhonemes();
        }

        PhonemeSequence *phonemeOwnerFromSnapshot(ModelPrivate &model, const dini::ItemSnapshot &snapshot) {
            return phonemeOwnerFromPlacement(model,
                                             orm::handleFromValue(orm::snapshotValue(snapshot, Schema::phonemeParent().column())),
                                             roleFromValue(orm::snapshotValue(snapshot, Schema::phonemeRoleColumn())));
        }

        const std::vector<orm::ColumnBinding<Phoneme>> &phonemeColumnBindings() {
            static const std::vector<orm::ColumnBinding<Phoneme>> bindings {
                orm::stringField<Phoneme, PhonemePrivate>(Schema::phonemeLanguageColumn(), &PhonemePrivate::language, &Phoneme::languageChanged),
                orm::intField<Phoneme, PhonemePrivate>(Schema::phonemeStartColumn(), &PhonemePrivate::start, &Phoneme::startChanged),
                orm::stringField<Phoneme, PhonemePrivate>(Schema::phonemeTokenColumn(), &PhonemePrivate::token, &Phoneme::tokenChanged),
                orm::boolField<Phoneme, PhonemePrivate>(Schema::phonemeOnsetColumn(), &PhonemePrivate::onset, &Phoneme::onsetChanged),
                orm::previousNextField<Phoneme, PhonemePrivate>(Schema::phonemePreviousItemColumn(), &PhonemePrivate::previousHandle, &PhonemePrivate::previous, &Phoneme::previousItemChanged),
                orm::previousNextField<Phoneme, PhonemePrivate>(Schema::phonemeNextItemColumn(), &PhonemePrivate::nextHandle, &PhonemePrivate::next, &Phoneme::nextItemChanged),
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
                .membershipColumns = {Schema::phonemeParent().column(), Schema::phonemeRoleColumn()},
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
                .refreshOwner = [](PhonemeSequence *owner, bool notify) { PhonemeSequencePrivate::get(owner)->refresh(notify); },
                .itemAboutToInsert = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedFrom) { emit owner->itemAboutToInsert(item, movedFrom); },
                .itemInserted = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedFrom) { emit owner->itemInserted(item, movedFrom); },
                .itemAboutToRemove = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedTo) { emit owner->itemAboutToRemove(item, movedTo); },
                .itemRemoved = [](PhonemeSequence *owner, Phoneme *item, PhonemeSequence *movedTo) { emit owner->itemRemoved(item, movedTo); },
            });
            return binding;
        }

        void syncPhonemeColumns(Phoneme *item, const dini::ItemSnapshot &snapshot, bool notify) {
            syncColumnBindings(phonemeColumnBindings(), item, snapshot, notify);
            PhonemePrivate::get(item)->setPlacement(
                orm::handleFromValue(orm::snapshotValue(snapshot, Schema::phonemeParent().column())),
                roleFromValue(orm::snapshotValue(snapshot, Schema::phonemeRoleColumn())),
                notify);
        }

        bool applyPhonemeColumn(Phoneme *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            auto *d = PhonemePrivate::get(item);
            if (column == Schema::phonemeParent().column()) {
                d->setPlacement(orm::handleFromValue(value), d->role, notify);
                return true;
            }
            if (column == Schema::phonemeRoleColumn()) {
                d->setPlacement(d->parentHandle, roleFromValue(value), notify);
                return true;
            }
            return applyColumnBinding(phonemeColumnBindings(), item, column, value, notify);
        }

    }

    PhonemePrivate::PhonemePrivate(Phoneme *q) : q_ptr(q) {
    }

    void PhonemePrivate::setPlacement(Handle newParent, std::optional<PhonemeSequence::PhonemeRole> newRole, bool notify) {
        Q_Q(Phoneme);
        auto *modelData = ModelPrivate::get(q->model());
        auto *newSequence = phonemeOwnerFromPlacement(*modelData, newParent, newRole);
        const bool changed = parentHandle != newParent || role != newRole || sequence != newSequence;
        parentHandle = newParent;
        role = newRole;
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
