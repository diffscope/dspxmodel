#include "FreeValueDataArray.h"
#include "FreeValueDataArray_p.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <utility>
#include <variant>
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

        struct DataArrayChangeGroup {
            dini::Value associationValue;
            std::vector<std::size_t> operationIndexes;
            bool hasSplice = false;
        };

        void reverseRotation(std::vector<dini::ItemSnapshot> &items, const dini::ListRotation &rotation) {
            if (rotation.count == 0 || rotation.startIndex + rotation.count > items.size()) {
                return;
            }
            const auto count = static_cast<std::ptrdiff_t>(rotation.count);
            const auto normalized = ((rotation.offset % count) + count) % count;
            const auto inverse = normalized == 0 ? 0 : count - normalized;
            auto first = items.begin() + static_cast<std::ptrdiff_t>(rotation.startIndex);
            std::rotate(first, first + inverse, first + count);
        }

        void reverseDataArrayChanges(std::vector<dini::ItemSnapshot> &items,
                                     const dini::ChangeSet &changeSet,
                                     const DataArrayChangeGroup &group) {
            const auto &operations = changeSet.operations();
            for (auto it = group.operationIndexes.rbegin(); it != group.operationIndexes.rend(); ++it) {
                const auto &payload = operations[*it].payload();
                if (const auto *change = std::get_if<dini::ListInsertedChange>(&payload)) {
                    auto item = items.end();
                    if (change->index < items.size() && items[change->index].id == change->item.id) {
                        item = items.begin() + static_cast<std::ptrdiff_t>(change->index);
                    } else {
                        item = std::find_if(items.begin(), items.end(), [change](const dini::ItemSnapshot &snapshot) {
                            return snapshot.id == change->item.id;
                        });
                    }
                    if (item != items.end()) {
                        items.erase(item);
                    }
                } else if (const auto *change = std::get_if<dini::ListRemovedChange>(&payload)) {
                    const auto index = std::min(change->index, items.size());
                    items.insert(items.begin() + static_cast<std::ptrdiff_t>(index), change->item);
                } else if (const auto *change = std::get_if<dini::ListRotatedChange>(&payload)) {
                    reverseRotation(items, change->rotation);
                }
            }
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

        void collectFreeValueDataArrayChanges(ModelPrivate &model,
                                              const dini::ChangeSet &changeSet,
                                              std::vector<bool> &handledOperations,
                                              std::vector<std::function<void()>> &notifications) {
            const auto list = Schema::freeValueList();
            const auto &operations = changeSet.operations();
            std::vector<DataArrayChangeGroup> groups;
            auto appendOperation = [&groups](const dini::Value &associationValue, std::size_t index, bool isSplice) {
                auto group = std::find_if(groups.begin(), groups.end(), [&associationValue](const DataArrayChangeGroup &candidate) {
                    return candidate.associationValue == associationValue;
                });
                if (group == groups.end()) {
                    groups.push_back(DataArrayChangeGroup {
                        .associationValue = associationValue,
                        .operationIndexes = {index},
                        .hasSplice = isSplice,
                    });
                    return;
                }
                group->operationIndexes.push_back(index);
                group->hasSplice = group->hasSplice || isSplice;
            };

            for (std::size_t i = 0; i < operations.size(); ++i) {
                const auto &payload = operations[i].payload();
                if (const auto *change = std::get_if<dini::ListInsertedChange>(&payload);
                    change && change->list == list) {
                    appendOperation(change->associationValue, i, true);
                } else if (const auto *change = std::get_if<dini::ListRemovedChange>(&payload);
                           change && change->list == list) {
                    appendOperation(change->associationValue, i, true);
                } else if (const auto *change = std::get_if<dini::ListRotatedChange>(&payload);
                           change && change->list == list) {
                    appendOperation(change->associationValue, i, false);
                }
            }

            for (const auto &group : groups) {
                if (!group.hasSplice) {
                    continue;
                }
                for (const auto operationIndex : group.operationIndexes) {
                    handledOperations[operationIndex] = true;
                }

                auto *owner = freeValueOwnerFromAssociationValue(model, group.associationValue);
                if (!owner) {
                    continue;
                }
                auto newItems = model.engine->query(list, freeValueQuery(orm::handleFromValue(group.associationValue))).toVector();
                auto oldItems = newItems;
                reverseDataArrayChanges(oldItems, changeSet, group);

                std::size_t prefix = 0;
                while (prefix < oldItems.size() && prefix < newItems.size() &&
                       oldItems[prefix].id == newItems[prefix].id) {
                    ++prefix;
                }
                std::size_t suffix = 0;
                while (suffix < oldItems.size() - prefix && suffix < newItems.size() - prefix &&
                       oldItems[oldItems.size() - suffix - 1].id == newItems[newItems.size() - suffix - 1].id) {
                    ++suffix;
                }

                const auto removedCount = oldItems.size() - prefix - suffix;
                QList<QVariant> insertedValues;
                const auto insertedEnd = newItems.size() - suffix;
                for (auto i = prefix; i < insertedEnd; ++i) {
                    insertedValues.append(variantFromSnapshot(newItems[i]));
                }
                if (removedCount == 0 && insertedValues.isEmpty()) {
                    continue;
                }

                const auto signalIndex = static_cast<int>(prefix);
                const auto signalLength = static_cast<int>(removedCount);
                notifications[group.operationIndexes.front()] = [owner, signalIndex, signalLength, insertedValues] {
                    auto *ownerData = FreeValueDataArrayPrivate::get(owner);
                    if (ownerData->suppressNotifications) {
                        return;
                    }
                    emit owner->aboutToSplice(signalIndex, signalLength, insertedValues);
                    ownerData->refresh(true, true);
                    emit owner->spliced(signalIndex, signalLength, insertedValues);
                };
            }
        }

    }

    FreeValueDataArrayPrivate::FreeValueDataArrayPrivate(FreeValueDataArray *q, Parameter *parameter, FreeValueDataArray::FreeValueRole role)
        : q_ptr(q), parameter(parameter), role(role), jsIterable(new JSIterable(q, JSIterable::DataArray)) {
    }

    Handle FreeValueDataArrayPrivate::relationHandle() const {
        if (role == FreeValueDataArray::Original) {
            return {};
        }
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
        int newSize = 0;
        if (role == FreeValueDataArray::Original) {
            newSize = static_cast<int>(originalItems.size());
        } else {
            const auto relation = relationHandle();
            auto *modelData = ModelPrivate::get(parameter->model());
            newSize = relation ? static_cast<int>(modelData->engine->listLength(Schema::freeValueList(), orm::valueFromHandle(relation))) : 0;
        }
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
        if (d->role == Original) {
            return d->originalItems;
        }
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
        if (d->role == Original) {
            return d->originalItems.mid(index, length);
        }
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
        if (d->role == Original) {
            emit aboutToSplice(index, length, values);
            const auto insertedCount = static_cast<int>(values.size());
            if (insertedCount < length) {
                d->originalItems.remove(index + insertedCount, length - insertedCount);
            } else if (insertedCount > length) {
                const auto oldSize = d->originalItems.size();
                d->originalItems.resize(oldSize + insertedCount - length);
                std::ranges::move_backward(d->originalItems.begin() + index + length,
                                           d->originalItems.begin() + oldSize,
                                           d->originalItems.end());
            }
            if (insertedCount > 0) {
                std::ranges::copy_n(values.cbegin(), insertedCount, d->originalItems.begin() + index);
            }
            d->refresh(true, true);
            emit spliced(index, length, values);
            return true;
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }

        auto *transaction = ModelPrivate::get(parameter()->model())->requireTransaction();
        emit aboutToSplice(index, length, values);
        d->suppressNotifications = true;
        try {
            const auto list = Schema::freeValueList();
            const auto oldSize = static_cast<std::size_t>(d->size);
            const auto spliceIndex = static_cast<std::size_t>(index);
            const auto removedCount = static_cast<std::size_t>(length);
            const auto insertedCount = static_cast<std::size_t>(values.size());
            const auto suffixCount = oldSize - spliceIndex - removedCount;

            if (removedCount > 0 && suffixCount > 0) {
                transaction->rotate(list,
                                    associationValue,
                                    dini::ListRotation {
                                        .startIndex = spliceIndex,
                                        .count = removedCount + suffixCount,
                                        .offset = static_cast<std::ptrdiff_t>(removedCount),
                                    });
            }
            if (removedCount > 0) {
                transaction->removeAt(list, associationValue, oldSize - removedCount, removedCount);
            }
            if (insertedCount > 0) {
                std::vector<dini::ListInsertItem> insertedItems(insertedCount);
                std::ranges::transform(values, insertedItems.begin(), [](const QVariant &value) {
                    return dini::ListInsertItem {.values = freeValueValues(value)};
                });
                transaction->insert(list,
                                    associationValue,
                                    oldSize - removedCount,
                                    std::move(insertedItems));
            }
            if (insertedCount > 0 && suffixCount > 0) {
                transaction->rotate(list,
                                    associationValue,
                                    dini::ListRotation {
                                        .startIndex = spliceIndex,
                                        .count = suffixCount + insertedCount,
                                        .offset = static_cast<std::ptrdiff_t>(suffixCount),
                                    });
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
        if (d->role == Original) {
            emit aboutToRotate(leftIndex, middleIndex, rightIndex);
            std::rotate(d->originalItems.begin() + leftIndex,
                        d->originalItems.begin() + middleIndex,
                        d->originalItems.begin() + rightIndex);
            emit itemsChanged();
            emit rotated(leftIndex, middleIndex, rightIndex);
            return true;
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
