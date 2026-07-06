#include "FreeValueDataArray.h"
#include "FreeValueDataArray_p.h"

#include <cstdint>
#include <cstddef>
#include <vector>

#include <QMetaType>

#include <dini/engine.h>
#include <dini/transaction.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Parameter.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec freeValueRelationQuery(Handle parameterHandle, FreeValueDataArray::FreeValueRole role) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::freeValueRelationParent(), parameterHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::freeValueRelationRoleColumn()),
                                     dini::Value(static_cast<std::int64_t>(role))),
                }),
            };
        }

        dini::QuerySpec freeValueQuery(Handle relationHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::freeValueParent(), relationHandle),
            };
        }

        QVariant variantFromSnapshot(const dini::ItemSnapshot &snapshot) {
            const auto &value = orm::snapshotValue(snapshot, Schema::freeValueValueColumn());
            return value.isNull() ? QVariant() : QVariant(static_cast<int>(value.asInt64()));
        }

        QList<QVariant> variantsFromView(const dini::View &view) {
            QList<QVariant> result;
            for (const auto &snapshot : view.toVector()) {
                result.append(variantFromSnapshot(snapshot));
            }
            return result;
        }

        bool isValidFreeValue(const QVariant &value) {
            return !value.isValid() || value.userType() == QMetaType::Int;
        }

        dini::Value valueFromVariant(const QVariant &value) {
            return value.isValid() ? dini::Value(static_cast<std::int64_t>(value.toInt())) : dini::Value::null();
        }

        std::vector<dini::ColumnValue> freeValueValues(const QVariant &value) {
            return {
                dini::ColumnValue {.column = Schema::freeValueValueColumn(), .value = valueFromVariant(value)},
            };
        }

        FreeValueDataArray *freeValueOwnerFromAssociationValue(ModelPrivate &model, const dini::Value &value) {
            const auto relationHandle = orm::handleFromValue(value);
            if (!relationHandle || !model.engine->contains(orm::idFromHandle(relationHandle))) {
                return nullptr;
            }
            const auto relation = model.engine->read(orm::idFromHandle(relationHandle));
            if (!orm::isContainer(relation, Schema::parameterFreeValueRelation())) {
                return nullptr;
            }
            const auto parameterHandle = orm::handleFromValue(orm::snapshotValue(relation, Schema::freeValueRelationParent().column()));
            const auto roleValue = orm::snapshotValue(relation, Schema::freeValueRelationRoleColumn());
            if (!parameterHandle || roleValue.isNull()) {
                return nullptr;
            }
            auto *parameter = model.ensure<Parameter>(parameterHandle);
            if (!parameter) {
                return nullptr;
            }
            const auto role = static_cast<FreeValueDataArray::FreeValueRole>(roleValue.asInt64());
            if (role == FreeValueDataArray::Original) {
                return parameter->original();
            }
            if (role == FreeValueDataArray::Transform) {
                return parameter->freeTransform();
            }
            if (role == FreeValueDataArray::Edited) {
                return parameter->freeEdited();
            }
            return nullptr;
        }

    }

    namespace orm {

        const ListBinding &freeValueDataArrayBinding() {
            static const ListBinding binding = makeDataArrayListBinding<FreeValueDataArray, QVariant>({
                .list = Schema::freeValueList(),
                .associationColumn = Schema::freeValueParent().column(),
                .ownerForAssociationValue = [](ModelPrivate &model, const dini::Value &value) {
                    return freeValueOwnerFromAssociationValue(model, value);
                },
                .refreshOwner = [](FreeValueDataArray *owner, bool notify, bool itemsChanged) {
                    FreeValueDataArrayPrivate::get(owner)->refresh(notify, itemsChanged);
                },
                .decodeItem = [](const dini::ItemSnapshot &snapshot) {
                    return variantFromSnapshot(snapshot);
                },
                .notificationsSuppressed = [](FreeValueDataArray *owner) {
                    return FreeValueDataArrayPrivate::get(owner)->suppressNotifications;
                },
                .aboutToSplice = [](FreeValueDataArray *owner, int index, int length, const QList<QVariant> &values) {
                    emit owner->aboutToSplice(index, length, values);
                },
                .spliced = [](FreeValueDataArray *owner, int index, int length, const QList<QVariant> &values) {
                    emit owner->spliced(index, length, values);
                },
                .aboutToRotate = [](FreeValueDataArray *owner, int left, int middle, int right) {
                    emit owner->aboutToRotate(left, middle, right);
                },
                .rotated = [](FreeValueDataArray *owner, int left, int middle, int right) {
                    emit owner->rotated(left, middle, right);
                },
            });
            return binding;
        }

    }

    FreeValueDataArrayPrivate::FreeValueDataArrayPrivate(FreeValueDataArray *q, Parameter *parameter, FreeValueDataArray::FreeValueRole role)
        : q_ptr(q), parameter(parameter), role(role) {
    }

    Handle FreeValueDataArrayPrivate::relationHandle() const {
        auto *modelData = ModelPrivate::get(parameter->model());
        if (auto relation = orm::firstSnapshot(modelData->engine->query(Schema::parameterFreeValueRelation(),
                                                                        freeValueRelationQuery(parameter->handle(), role)))) {
            return orm::handleFromId(relation->id);
        }
        return {};
    }

    dini::Value FreeValueDataArrayPrivate::associationValue() const {
        return orm::valueFromHandle(relationHandle());
    }

    void FreeValueDataArrayPrivate::refresh(bool notify, bool itemsChanged) {
        Q_Q(FreeValueDataArray);
        const auto relation = relationHandle();
        auto *modelData = ModelPrivate::get(parameter->model());
        const auto newSize = relation ? static_cast<int>(modelData->engine->listLength(Schema::freeValueList(), orm::valueFromHandle(relation))) : 0;
        const bool sizeChanged = size != newSize;
        size = newSize;

        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        if (itemsChanged || sizeChanged) {
            emit q->itemsChanged();
        }
    }

    FreeValueDataArray::FreeValueDataArray(Parameter *parameter, FreeValueRole role)
        : QObject(parameter), d_ptr(new FreeValueDataArrayPrivate(this, parameter, role)) {
    }

    FreeValueDataArray::~FreeValueDataArray() = default;

    int FreeValueDataArray::size() const {
        Q_D(const FreeValueDataArray);
        return d->size;
    }

    QList<QVariant> FreeValueDataArray::items() const {
        Q_D(const FreeValueDataArray);
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(parameter()->model());
        return variantsFromView(modelData->engine->query(Schema::freeValueList(), freeValueQuery(relation)));
    }

    QList<QVariant> FreeValueDataArray::slice(int index, int length) const {
        if (index < 0 || length < 0) {
            return {};
        }
        Q_D(const FreeValueDataArray);
        const auto relation = d->relationHandle();
        if (!relation) {
            return {};
        }
        auto *modelData = ModelPrivate::get(parameter()->model());
        return variantsFromView(modelData->engine->query(Schema::freeValueList(), freeValueQuery(relation))
                                    .offset(static_cast<std::size_t>(index))
                                    .limit(static_cast<std::size_t>(length)));
    }

    bool FreeValueDataArray::splice(int index, int length, const QList<QVariant> &values) {
        Q_D(FreeValueDataArray);
        if (index < 0 || length < 0 || index > d->size || length > d->size - index) {
            return false;
        }
        for (const auto &value : values) {
            if (!isValidFreeValue(value)) {
                return false;
            }
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }

        auto *transaction = ModelPrivate::get(parameter()->model())->requireTransaction();
        emit aboutToSplice(index, length, values);
        d->suppressNotifications = true;
        try {
            for (int i = 0; i < length; ++i) {
                transaction->removeAt(Schema::freeValueList(), associationValue, static_cast<std::size_t>(index));
            }
            for (int i = 0; i < values.size(); ++i) {
                transaction->insert(Schema::freeValueList(),
                                    associationValue,
                                    static_cast<std::size_t>(index + i),
                                    freeValueValues(values.at(i)));
            }
        } catch (...) {
            d->suppressNotifications = false;
            throw;
        }
        d->suppressNotifications = false;
        d->refresh(true, true);
        emit spliced(index, length, values);
        return true;
    }

    bool FreeValueDataArray::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(FreeValueDataArray);
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex || rightIndex > d->size) {
            return false;
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(parameter()->model())->rotate(Schema::freeValueList(), associationValue, leftIndex, middleIndex, rightIndex);
        return true;
    }

    FreeValueDataArray::FreeValueRole FreeValueDataArray::role() const {
        Q_D(const FreeValueDataArray);
        return d->role;
    }

    Parameter *FreeValueDataArray::parameter() const {
        Q_D(const FreeValueDataArray);
        return d->parameter;
    }

}


#include "moc_FreeValueDataArray.cpp"
