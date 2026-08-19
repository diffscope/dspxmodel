#include "VibratoPointDataArray.h"
#include "VibratoPointDataArray_p.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

#include <dini/engine.h>
#include <dini/transaction.h>
#include <opendspx/controlpoint.h>

#include <dspxmodelCore/Schema.h>
#include <dspxmodelORM/Model.h>
#include <dspxmodelORM/Note.h>
#include <dspxmodelORM/private/Model_p.h>
#include <dspxmodelORM/private/ORMBinding_p.h>
#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    namespace {

        dini::QuerySpec vibratoPointRelationQuery(Handle noteHandle, VibratoPointDataArray::VibratoPointRole role) {
            return dini::QuerySpec {
                .filter = dini::FilterExpression::all({
                    orm::parentFilter(Schema::noteVibratoPointRelationParent(), noteHandle),
                    orm::equalFilter(dini::FieldRef::column(Schema::noteVibratoPointRelationRoleColumn()),
                                     dini::Value(static_cast<std::int64_t>(role))),
                }),
            };
        }

        dini::QuerySpec vibratoPointQuery(Handle relationHandle) {
            return dini::QuerySpec {
                .filter = orm::parentFilter(Schema::vibratoPointParent(), relationHandle),
            };
        }

        QPointF pointFromSnapshot(const dini::ItemSnapshot &snapshot) {
            return QPointF(orm::snapshotValue(snapshot, Schema::vibratoPointXColumn()).asDouble(),
                           orm::snapshotValue(snapshot, Schema::vibratoPointYColumn()).asDouble());
        }

        QList<QPointF> pointsFromView(const dini::View &view) {
            QList<QPointF> result;
            for (const auto &snapshot : view.toVector()) {
                result.append(pointFromSnapshot(snapshot));
            }
            return result;
        }

        std::vector<dini::ColumnValue> pointValues(const QPointF &point) {
            return {
                dini::ColumnValue {.column = Schema::vibratoPointXColumn(), .value = dini::Value(point.x())},
                dini::ColumnValue {.column = Schema::vibratoPointYColumn(), .value = dini::Value(point.y())},
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

        VibratoPointDataArray *vibratoPointOwnerFromAssociationValue(ModelPrivate &model, const dini::Value &value) {
            const auto relationHandle = orm::handleFromValue(value);
            if (!relationHandle || !model.engine->contains(orm::idFromHandle(relationHandle))) {
                return nullptr;
            }
            const auto relation = model.engine->read(orm::idFromHandle(relationHandle));
            if (!orm::isContainer(relation, Schema::noteVibratoPointRelationTable())) {
                return nullptr;
            }
            const auto noteHandle = orm::handleFromValue(orm::snapshotValue(relation, Schema::noteVibratoPointRelationParent().column()));
            const auto roleValue = orm::snapshotValue(relation, Schema::noteVibratoPointRelationRoleColumn());
            if (!noteHandle || roleValue.isNull()) {
                return nullptr;
            }
            auto *note = model.ensure<Note>(noteHandle);
            if (!note) {
                return nullptr;
            }
            const auto role = static_cast<VibratoPointDataArray::VibratoPointRole>(roleValue.asInt64());
            if (role == VibratoPointDataArray::Amplitude) {
                return note->vibratoAmplitudeControlPoints();
            }
            if (role == VibratoPointDataArray::Frequency) {
                return note->vibratoFrequencyControlPoints();
            }
            return nullptr;
        }

    }

    namespace orm {

        const ListBinding &vibratoPointDataArrayBinding() {
            static const ListBinding binding = makeDataArrayListBinding<VibratoPointDataArray, QPointF>({
                .list = Schema::vibratoPointList(),
                .associationColumn = Schema::vibratoPointParent().column(),
                .ownerForAssociationValue = [](ModelPrivate &model, const dini::Value &value) {
                    return vibratoPointOwnerFromAssociationValue(model, value);
                },
                .applySplice = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values, bool notify) {
                    VibratoPointDataArrayPrivate::get(owner)->applySplice(index, length, values, notify);
                },
                .applyRotate = [](VibratoPointDataArray *owner, int leftIndex, int middleIndex, int rightIndex, bool notify) {
                    VibratoPointDataArrayPrivate::get(owner)->applyRotate(leftIndex, middleIndex, rightIndex, notify);
                },
                .decodeItem = [](const dini::ItemSnapshot &snapshot) {
                    return pointFromSnapshot(snapshot);
                },
                .notificationsSuppressed = [](VibratoPointDataArray *owner) {
                    return VibratoPointDataArrayPrivate::get(owner)->suppressNotifications;
                },
                .aboutToSplice = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values) {
                    emit owner->aboutToSplice(index, length, values);
                },
                .spliced = [](VibratoPointDataArray *owner, int index, int length, const QList<QPointF> &values) {
                    emit owner->spliced(index, length, values);
                },
                .aboutToRotate = [](VibratoPointDataArray *owner, int left, int middle, int right) {
                    emit owner->aboutToRotate(left, middle, right);
                },
                .rotated = [](VibratoPointDataArray *owner, int left, int middle, int right) {
                    emit owner->rotated(left, middle, right);
                },
            });
            return binding;
        }

        void collectVibratoPointDataArrayChanges(ModelPrivate &model,
                                                 const dini::ChangeSet &changeSet,
                                                 std::vector<bool> &handledOperations,
                                                 std::vector<std::function<void()>> &notifications) {
            const auto list = Schema::vibratoPointList();
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

                auto *owner = vibratoPointOwnerFromAssociationValue(model, group.associationValue);
                if (!owner) {
                    continue;
                }
                auto newItems = model.engine->query(list, vibratoPointQuery(orm::handleFromValue(group.associationValue))).toVector();
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
                QList<QPointF> insertedValues;
                const auto insertedEnd = newItems.size() - suffix;
                for (auto i = prefix; i < insertedEnd; ++i) {
                    insertedValues.append(pointFromSnapshot(newItems[i]));
                }
                if (removedCount == 0 && insertedValues.isEmpty()) {
                    continue;
                }

                const auto signalIndex = static_cast<int>(prefix);
                const auto signalLength = static_cast<int>(removedCount);
                notifications[group.operationIndexes.front()] = [owner, signalIndex, signalLength, insertedValues] {
                    auto *ownerData = VibratoPointDataArrayPrivate::get(owner);
                    if (ownerData->suppressNotifications) {
                        return;
                    }
                    emit owner->aboutToSplice(signalIndex, signalLength, insertedValues);
                    ownerData->applySplice(signalIndex, signalLength, insertedValues, true);
                    emit owner->spliced(signalIndex, signalLength, insertedValues);
                };
            }
        }

    }

    VibratoPointDataArrayPrivate::VibratoPointDataArrayPrivate(VibratoPointDataArray *q,
                                                               Note *note,
                                                               VibratoPointDataArray::VibratoPointRole role)
        : q_ptr(q), note(note), role(role), jsIterable(new JSIterable(q, JSIterable::DataArray)) {
    }

    Handle VibratoPointDataArrayPrivate::relationHandle() const {
        auto *modelData = ModelPrivate::get(note->model());
        if (auto relation = orm::firstSnapshot(modelData->engine->query(Schema::noteVibratoPointRelationTable(),
                                                                        vibratoPointRelationQuery(note->handle(), role)))) {
            return orm::handleFromId(relation->id);
        }
        return {};
    }

    dini::Value VibratoPointDataArrayPrivate::associationValue() const {
        return orm::valueFromHandle(relationHandle());
    }

    // Full rebuild from the engine. Initialization only; all change paths must
    // update the cache incrementally via applySplice()/applyRotate().
    void VibratoPointDataArrayPrivate::refresh(bool notify, bool itemsChanged) {
        Q_Q(VibratoPointDataArray);
        const auto relation = relationHandle();
        auto *modelData = ModelPrivate::get(note->model());
        items = relation ? pointsFromView(modelData->engine->query(Schema::vibratoPointList(), vibratoPointQuery(relation))) : QList<QPointF> {};
        const auto newSize = static_cast<int>(items.size());
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

    void VibratoPointDataArrayPrivate::applySplice(int index, int length, const QList<QPointF> &values, bool notify) {
        Q_Q(VibratoPointDataArray);
        if (index < 0 || length < 0 || index > items.size() || length > items.size() - index) {
            return;
        }
        const auto insertedCount = static_cast<int>(values.size());
        if (insertedCount < length) {
            items.remove(index + insertedCount, length - insertedCount);
        } else if (insertedCount > length) {
            const auto oldSize = items.size();
            items.resize(oldSize + insertedCount - length);
            std::ranges::move_backward(items.begin() + index + length,
                                       items.begin() + oldSize,
                                       items.end());
        }
        if (insertedCount > 0) {
            std::ranges::copy_n(values.cbegin(), insertedCount, items.begin() + index);
        }
        const auto newSize = static_cast<int>(items.size());
        const bool sizeChanged = size != newSize;
        size = newSize;
        if (!notify) {
            return;
        }
        if (sizeChanged) {
            emit q->sizeChanged(size);
        }
        emit q->itemsChanged();
    }

    void VibratoPointDataArrayPrivate::applyRotate(int leftIndex, int middleIndex, int rightIndex, bool notify) {
        Q_Q(VibratoPointDataArray);
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex || rightIndex > items.size()) {
            return;
        }
        std::rotate(items.begin() + leftIndex,
                    items.begin() + middleIndex,
                    items.begin() + rightIndex);
        if (notify) {
            emit q->itemsChanged();
        }
    }

    VibratoPointDataArray::VibratoPointDataArray(Note *note, VibratoPointRole role)
        : QObject(note), d_ptr(new VibratoPointDataArrayPrivate(this, note, role)) {
    }

    VibratoPointDataArray::~VibratoPointDataArray() = default;

    int VibratoPointDataArray::size() const {
        Q_D(const VibratoPointDataArray);
        return d->size;
    }

    QList<QPointF> VibratoPointDataArray::items() const {
        Q_D(const VibratoPointDataArray);
        return d->items;
    }

    QList<QPointF> VibratoPointDataArray::slice(int index, int length) const {
        if (index < 0 || length < 0) {
            return {};
        }
        Q_D(const VibratoPointDataArray);
        return d->items.mid(index, length);
    }

    bool VibratoPointDataArray::splice(int index, int length, const QList<QPointF> &values) {
        Q_D(VibratoPointDataArray);
        if (index < 0 || length < 0 || index > d->size || length > d->size - index) {
            return false;
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }

        auto *transaction = ModelPrivate::get(note()->model())->requireTransaction();
        emit aboutToSplice(index, length, values);
        d->suppressNotifications = true;
        try {
            const auto list = Schema::vibratoPointList();
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
                std::ranges::transform(values, insertedItems.begin(), [](const QPointF &value) {
                    return dini::ListInsertItem {.values = pointValues(value)};
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
        d->applySplice(index, length, values, true);
        emit spliced(index, length, values);
        return true;
    }

    bool VibratoPointDataArray::rotate(int leftIndex, int middleIndex, int rightIndex) {
        Q_D(VibratoPointDataArray);
        if (leftIndex < 0 || middleIndex < leftIndex || rightIndex < middleIndex || rightIndex > d->size) {
            return false;
        }
        const auto associationValue = d->associationValue();
        if (associationValue.isNull()) {
            return false;
        }
        ModelPrivate::get(note()->model())->rotate(Schema::vibratoPointList(), associationValue, leftIndex, middleIndex, rightIndex);
        return true;
    }

    VibratoPointDataArray::VibratoPointRole VibratoPointDataArray::role() const {
        Q_D(const VibratoPointDataArray);
        return d->role;
    }

    Note *VibratoPointDataArray::note() const {
        Q_D(const VibratoPointDataArray);
        return d->note;
    }

    std::vector<opendspx::ControlPoint> VibratoPointDataArray::toOpenDSPX() const {
        std::vector<opendspx::ControlPoint> result;
        const auto source = items();
        result.reserve(static_cast<std::size_t>(source.size()));
        for (const auto &point : source) {
            result.push_back(opendspx::ControlPoint {
                .x = point.x(),
                .y = point.y(),
            });
        }
        return result;
    }

    void VibratoPointDataArray::fromOpenDSPX(const std::vector<opendspx::ControlPoint> &points) {
        QList<QPointF> target;
        target.reserve(static_cast<qsizetype>(points.size()));
        for (const auto &point : points) {
            target.append(QPointF(point.x, point.y));
        }
        splice(0, size(), target);
    }

}


#include "moc_VibratoPointDataArray.cpp"
