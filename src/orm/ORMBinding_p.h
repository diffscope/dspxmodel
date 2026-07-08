#ifndef DSPXMODEL_ORMBINDING_P_H
#define DSPXMODEL_ORMBINDING_P_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include <QList>
#include <QString>

#include <dini/change.h>
#include <dini/query.h>
#include <dini/schema.h>
#include <dini/types.h>
#include <dini/value.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class ModelPrivate;
    class AnchorNode;
    class Clip;
    class DynamicMixingAnchor;
    class FreeValueDataArray;
    class KeySignature;
    class Label;
    class Note;
    class Parameter;
    class ParameterMap;
    class Phoneme;
    class Singer;
    class Sources;
    class Tempo;
    class TimeSignature;
    class Track;
    class VibratoPointDataArray;

    namespace orm {

        dini::DocumentEngine *getModelEngine(ModelPrivate &model);
        const dini::ItemSnapshot *currentEventSnapshot(ModelPrivate &model, dini::ItemId itemId);

        struct OrderSpec {
            std::vector<dini::ColumnHandle> columns;
            bool tieBreakById = false;
        };

        inline std::vector<dini::SortKey> sortKeys(const OrderSpec &spec, dini::SortDirection direction = dini::SortDirection::Ascending) {
            std::vector<dini::SortKey> result;
            result.reserve(spec.columns.size() + (spec.tieBreakById ? 1 : 0));
            for (const auto &column : spec.columns) {
                result.push_back(dini::SortKey {.field = dini::FieldRef::column(column), .direction = direction});
            }
            if (spec.tieBreakById) {
                result.push_back(dini::SortKey {.field = dini::FieldRef::id(), .direction = direction});
            }
            return result;
        }

        template <typename Object>
        struct ColumnBinding {
            dini::ColumnHandle column;
            std::function<bool(Object *, const dini::Value &)> apply;
            std::function<void(Object *)> notify;
        };

        template <typename Related, typename Object>
        Related *resolvePreviousNextObject(Object *object, Handle handle);

        template <typename Object>
        bool applyColumnBinding(const std::vector<ColumnBinding<Object>> &bindings, Object *object, const dini::ColumnHandle &column, const dini::Value &value, bool notify) {
            for (const auto &binding : bindings) {
                if (binding.column == column) {
                    const bool changed = binding.apply(object, value);
                    if (notify && changed && binding.notify) {
                        binding.notify(object);
                    }
                    return true;
                }
            }
            return false;
        }

        template <typename Object>
        void syncColumnBindings(const std::vector<ColumnBinding<Object>> &bindings, Object *object, const dini::ItemSnapshot &snapshot, bool notify) {
            std::vector<const ColumnBinding<Object> *> changedBindings;
            if (notify) {
                changedBindings.reserve(bindings.size());
            }
            for (const auto &binding : bindings) {
                const bool changed = binding.apply(object, snapshotValue(snapshot, binding.column));
                if (notify && changed && binding.notify) {
                    changedBindings.push_back(&binding);
                }
            }
            for (const auto *binding : changedBindings) {
                binding->notify(object);
            }
        }

        template <typename Object, typename Private, typename Member, typename Decode, typename Notify>
        ColumnBinding<Object> field(dini::ColumnHandle column, Member Private::*member, Decode decode, Notify notify) {
            return ColumnBinding<Object> {
                .column = std::move(column),
                .apply = [member, decode = std::move(decode)](Object *object, const dini::Value &value) {
                    auto *d = Private::get(object);
                    const auto newValue = decode(value);
                    const bool changed = d->*member != newValue;
                    d->*member = newValue;
                    return changed;
                },
                .notify = std::move(notify),
            };
        }

        template <typename Object, typename Private, typename Member, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> intField(dini::ColumnHandle column, Member Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return static_cast<Member>(value.asInt64()); }, std::move(notify));
        }

        template <typename Object, typename Private, typename Member, typename Signal>
            requires std::is_member_function_pointer_v<std::decay_t<Signal>>
        ColumnBinding<Object> intFieldWithSignal(dini::ColumnHandle column, Member Private::*member, Signal notify) {
            return intField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> doubleField(dini::ColumnHandle column, double Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return value.asDouble(); }, std::move(notify));
        }

        template <typename Object, typename Private, typename Signal>
            requires std::is_member_function_pointer_v<std::decay_t<Signal>>
        ColumnBinding<Object> doubleFieldWithSignal(dini::ColumnHandle column, double Private::*member, Signal notify) {
            return doubleField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> boolField(dini::ColumnHandle column, bool Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return value.asBool(); }, std::move(notify));
        }

        template <typename Object, typename Private, typename Signal>
            requires std::is_member_function_pointer_v<std::decay_t<Signal>>
        ColumnBinding<Object> boolFieldWithSignal(dini::ColumnHandle column, bool Private::*member, Signal notify) {
            return boolField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> stringField(dini::ColumnHandle column, QString Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return stringFromValue(value); }, std::move(notify));
        }

        template <typename Object, typename Private, typename Signal>
            requires std::is_member_function_pointer_v<std::decay_t<Signal>>
        ColumnBinding<Object> stringFieldWithSignal(dini::ColumnHandle column, QString Private::*member, Signal notify) {
            return stringField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> binaryField(dini::ColumnHandle column, dini::ByteArray Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return binaryFromValue(value); }, std::move(notify));
        }

        template <typename Enum, typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> enumField(dini::ColumnHandle column, Enum Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return static_cast<Enum>(value.asInt64()); }, std::move(notify));
        }

        template <typename Enum, typename Object, typename Private, typename Signal>
            requires std::is_member_function_pointer_v<std::decay_t<Signal>>
        ColumnBinding<Object> enumFieldWithSignal(dini::ColumnHandle column, Enum Private::*member, Signal notify) {
            return enumField<Enum, Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Private, typename Related, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> previousNextField(dini::ColumnHandle column, Handle Private::*handleMember, Related *Private::*objectMember, Notify notify) {
            return ColumnBinding<Object> {
                .column = std::move(column),
                .apply = [handleMember, objectMember](Object *object, const dini::Value &value) {
                    auto *d = Private::get(object);
                    const auto newHandle = handleFromValue(value);
                    auto *newObject = resolvePreviousNextObject<Related>(object, newHandle);
                    const bool changed = d->*handleMember != newHandle || d->*objectMember != newObject;
                    d->*handleMember = newHandle;
                    d->*objectMember = newObject;
                    return changed;
                },
                .notify = std::move(notify),
            };
        }

        template <typename Object, typename Private, typename Related, typename Signal>
            requires std::is_member_function_pointer_v<std::decay_t<Signal>>
        ColumnBinding<Object> previousNextFieldWithSignal(dini::ColumnHandle column, Handle Private::*handleMember, Related *Private::*objectMember, Signal notify) {
            return previousNextField<Object, Private, Related>(std::move(column), handleMember, objectMember, [objectMember, notify](Object *object) {
                (object->*notify)(Private::get(object)->*objectMember);
            });
        }

        struct TableBinding {
            dini::TableHandle table;
            std::function<void(ModelPrivate &, const dini::ItemInsertedChange &)> itemInserted;
            std::function<void(ModelPrivate &, const dini::ItemSnapshot &, bool)> itemRemoved;
            std::function<void(ModelPrivate &, const dini::ColumnUpdatedChange &)> columnUpdated;
            std::function<void(ModelPrivate &, const std::vector<dini::ColumnUpdatedChange> &)> columnUpdates;
            std::function<void(ModelPrivate &, const dini::ComputedColumnUpdatedChange &)> computedColumnUpdated;
        };

        struct ListBinding {
            dini::ListHandle list;
            std::function<void(ModelPrivate &, const dini::ItemInsertedChange &)> itemInserted;
            std::function<void(ModelPrivate &, const dini::ItemSnapshot &, bool)> itemRemoved;
            std::function<void(ModelPrivate &, const dini::ListInsertedChange &)> listInserted;
            std::function<void(ModelPrivate &, const dini::ListRemovedChange &)> listRemoved;
            std::function<void(ModelPrivate &, const dini::ColumnUpdatedChange &)> columnUpdated;
            std::function<void(ModelPrivate &, const dini::ComputedColumnUpdatedChange &)> computedColumnUpdated;
            std::function<void(ModelPrivate &, const dini::ListRotatedChange &)> listRotated;
        };

        enum class MoveSemantics {
            None,
            BetweenOwners,
        };

        enum class PositionConflictPolicy {
            AllowDuplicate,
            DetachExistingAtSamePosition,
        };

        template <typename Object, typename Owner>
        struct AssociatedTableBindingSpec {
            dini::TableHandle table;
            std::vector<dini::ColumnHandle> membershipColumns;
            std::vector<dini::ColumnHandle> orderColumns;
            MoveSemantics moveSemantics = MoveSemantics::None;

            std::function<Object *(ModelPrivate &, const dini::ItemSnapshot &)> ensure;
            std::function<Object *(ModelPrivate &, Handle)> find;
            std::function<void(ModelPrivate &, Handle)> removeObject;
            std::function<void(Object *, const dini::ItemSnapshot &, bool)> sync;
            std::function<bool(Object *, const dini::ColumnHandle &, const dini::Value &, bool)> applyColumn;

            std::function<Owner *(ModelPrivate &, const dini::ItemSnapshot &)> ownerForSnapshot;
            std::function<Owner *(ModelPrivate &, const dini::ColumnUpdatedChange &, bool)> ownerForChange;
            std::function<void(Object *, Owner *, bool)> setOwner;
            std::function<void(Owner *, bool)> refreshOwner;

            std::function<void(Owner *, Object *, Owner *)> itemAboutToInsert;
            std::function<void(Owner *, Object *, Owner *)> itemInserted;
            std::function<void(Owner *, Object *, Owner *)> itemAboutToRemove;
            std::function<void(Owner *, Object *, Owner *)> itemRemoved;
        };

        template <typename T, typename Refresh>
        void refreshUniqueOwner(T *first, T *second, Refresh refresh, bool notify) {
            if (first) {
                refresh(first, notify);
            }
            if (second && second != first) {
                refresh(second, notify);
            }
        }

        template <typename Object, typename Owner>
        TableBinding makeAssociatedTableBinding(AssociatedTableBindingSpec<Object, Owner> spec) {
            auto containsColumn = [](const std::vector<dini::ColumnHandle> &columns, const dini::ColumnHandle &column) {
                return std::find(columns.begin(), columns.end(), column) != columns.end();
            };
            struct UpdateGroup {
                dini::ItemId itemId = 0;
                dini::ItemSnapshot oldSnapshot;
                dini::ItemSnapshot newSnapshot;
                std::vector<dini::ColumnUpdatedChange> changes;
                bool membership = false;
                bool order = false;
            };
            auto groupForChange = [](std::vector<UpdateGroup> &groups, dini::ItemId itemId) -> UpdateGroup * {
                auto it = std::find_if(groups.begin(), groups.end(), [itemId](const auto &group) {
                    return group.itemId == itemId;
                });
                return it == groups.end() ? nullptr : &*it;
            };
            auto applyValue = [](dini::ItemSnapshot &snapshot, const dini::ColumnHandle &column, const dini::Value &value) {
                for (auto &columnValue : snapshot.values) {
                    if (columnValue.column == column) {
                        columnValue.value = value;
                        return;
                    }
                }
                snapshot.values.push_back({
                    .column = column,
                    .value = value,
                });
            };
            auto hasChangeForColumn = [](const UpdateGroup &group, const dini::ColumnHandle &column) {
                return std::find_if(group.changes.begin(), group.changes.end(), [&column](const auto &change) {
                    return change.column == column;
                }) != group.changes.end();
            };
            auto processColumnUpdates = [spec, containsColumn, groupForChange, applyValue, hasChangeForColumn](ModelPrivate &model, const std::vector<dini::ColumnUpdatedChange> &changes) {
                std::vector<UpdateGroup> groups;
                for (const auto &change : changes) {
                    const bool membership = containsColumn(spec.membershipColumns, change.column);
                    const bool order = containsColumn(spec.orderColumns, change.column);
                    auto *group = groupForChange(groups, change.itemId);
                    if (!group) {
                        const auto *eventSnapshot = currentEventSnapshot(model, change.itemId);
                        if (!eventSnapshot && !getModelEngine(model)->contains(change.itemId)) {
                            continue;
                        }
                        auto snapshot = eventSnapshot ? *eventSnapshot : getModelEngine(model)->read(change.itemId);
                        groups.push_back({
                            .itemId = change.itemId,
                            .oldSnapshot = snapshot,
                            .newSnapshot = std::move(snapshot),
                        });
                        group = &groups.back();
                    }
                    if (!hasChangeForColumn(*group, change.column)) {
                        applyValue(group->oldSnapshot, change.column, change.oldValue);
                    }
                    applyValue(group->newSnapshot, change.column, change.newValue);
                    group->changes.push_back(change);
                    group->membership = group->membership || membership;
                    group->order = group->order || order;
                }

                for (const auto &group : groups) {
                    auto *item = spec.find(model, handleFromId(group.itemId));
                    auto *oldOwner = spec.ownerForSnapshot ? spec.ownerForSnapshot(model, group.oldSnapshot) : nullptr;
                    auto *newOwner = spec.ownerForSnapshot ? spec.ownerForSnapshot(model, group.newSnapshot) : nullptr;

                    if (!item) {
                        if ((group.membership || group.order) && spec.refreshOwner) {
                            refreshUniqueOwner(oldOwner, newOwner, spec.refreshOwner, true);
                        }
                        continue;
                    }

                    if (group.membership) {
                        if (oldOwner && !newOwner && spec.itemAboutToRemove) {
                            spec.itemAboutToRemove(oldOwner, item, nullptr);
                        } else if (!oldOwner && newOwner && spec.itemAboutToInsert) {
                            spec.itemAboutToInsert(newOwner, item, nullptr);
                        } else if (oldOwner && newOwner && oldOwner != newOwner && spec.moveSemantics == MoveSemantics::BetweenOwners) {
                            if (spec.itemAboutToRemove) {
                                spec.itemAboutToRemove(oldOwner, item, newOwner);
                            }
                            if (spec.itemAboutToInsert) {
                                spec.itemAboutToInsert(newOwner, item, oldOwner);
                            }
                        }
                    }

                    spec.sync(item, group.newSnapshot, true);

                    if ((group.membership || group.order) && spec.refreshOwner) {
                        refreshUniqueOwner(oldOwner, newOwner, spec.refreshOwner, true);
                    }

                    if (group.membership) {
                        if (oldOwner && !newOwner && spec.itemRemoved) {
                            spec.itemRemoved(oldOwner, item, nullptr);
                        } else if (!oldOwner && newOwner && spec.itemInserted) {
                            spec.itemInserted(newOwner, item, nullptr);
                        } else if (oldOwner && newOwner && oldOwner != newOwner && spec.moveSemantics == MoveSemantics::BetweenOwners) {
                            if (spec.itemRemoved) {
                                spec.itemRemoved(oldOwner, item, newOwner);
                            }
                            if (spec.itemInserted) {
                                spec.itemInserted(newOwner, item, oldOwner);
                            }
                        }
                    }
                }
            };
            return TableBinding {
                .table = spec.table,
                .itemInserted = [spec](ModelPrivate &model, const dini::ItemInsertedChange &change) {
                    auto *item = spec.ensure(model, change.item);
                    auto *owner = item ? spec.ownerForSnapshot(model, change.item) : nullptr;
                    if (!item || !owner) {
                        return;
                    }
                    if (spec.itemAboutToInsert) {
                        spec.itemAboutToInsert(owner, item, nullptr);
                    }
                    spec.sync(item, change.item, true);
                    if (spec.refreshOwner) {
                        spec.refreshOwner(owner, true);
                    }
                    if (spec.itemInserted) {
                        spec.itemInserted(owner, item, nullptr);
                    }
                },
                .itemRemoved = [spec](ModelPrivate &model, const dini::ItemSnapshot &snapshot, bool) {
                    auto *item = spec.ensure(model, snapshot);
                    if (!item) {
                        return;
                    }
                    auto *owner = spec.ownerForSnapshot(model, snapshot);
                    if (owner && spec.itemAboutToRemove) {
                        spec.itemAboutToRemove(owner, item, nullptr);
                    }
                    if (spec.setOwner) {
                        spec.setOwner(item, nullptr, true);
                    }
                    if (spec.removeObject) {
                        spec.removeObject(model, handleFromId(snapshot.id));
                    }
                    if (owner && spec.refreshOwner) {
                        spec.refreshOwner(owner, true);
                    }
                    if (owner && spec.itemRemoved) {
                        spec.itemRemoved(owner, item, nullptr);
                    }
                    item->deleteLater();
                },
                .columnUpdated = [processColumnUpdates](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    processColumnUpdates(model, {change});
                },
                .columnUpdates = [processColumnUpdates](ModelPrivate &model, const std::vector<dini::ColumnUpdatedChange> &changes) {
                    processColumnUpdates(model, changes);
                },
                .computedColumnUpdated = [spec](ModelPrivate &model, const dini::ComputedColumnUpdatedChange &change) {
                    if (auto *item = spec.find(model, handleFromId(change.itemId))) {
                        spec.applyColumn(item, change.column, change.newValue, true);
                    }
                },
            };
        }

        template <typename Object, typename Owner>
        struct KeyedAssociatedTableBindingSpec {
            dini::TableHandle table;
            dini::ColumnHandle associationColumn;
            dini::ColumnHandle keyColumn;

            std::function<Object *(ModelPrivate &, const dini::ItemSnapshot &)> ensure;
            std::function<Object *(ModelPrivate &, Handle)> find;
            std::function<void(ModelPrivate &, Handle)> removeObject;
            std::function<void(Object *, const dini::ItemSnapshot &, bool)> sync;
            std::function<bool(Object *, const dini::ColumnHandle &, const dini::Value &, bool)> applyColumn;

            std::function<Owner *(ModelPrivate &, const dini::ItemSnapshot &)> ownerForSnapshot;
            std::function<QString(const dini::ItemSnapshot &)> keyForSnapshot;
            std::function<void(Object *, Owner *, QString, bool)> setPlacement;
            std::function<void(Owner *, bool)> refreshOwner;

            std::function<void(Owner *, const QString &, Object *, Owner *)> itemAboutToInsert;
            std::function<void(Owner *, const QString &, Object *, Owner *)> itemInserted;
            std::function<void(Owner *, const QString &, Object *, Owner *)> itemAboutToRemove;
            std::function<void(Owner *, const QString &, Object *, Owner *)> itemRemoved;
        };

        template <typename Object, typename Owner>
        TableBinding makeKeyedAssociatedTableBinding(KeyedAssociatedTableBindingSpec<Object, Owner> spec) {
            struct UpdateGroup {
                dini::ItemId itemId = 0;
                dini::ItemSnapshot oldSnapshot;
                dini::ItemSnapshot newSnapshot;
                std::vector<dini::ColumnUpdatedChange> changes;
                bool placement = false;
            };
            auto groupForChange = [](std::vector<UpdateGroup> &groups, dini::ItemId itemId) -> UpdateGroup * {
                auto it = std::find_if(groups.begin(), groups.end(), [itemId](const auto &group) {
                    return group.itemId == itemId;
                });
                return it == groups.end() ? nullptr : &*it;
            };
            auto applyValue = [](dini::ItemSnapshot &snapshot, const dini::ColumnHandle &column, const dini::Value &value) {
                for (auto &columnValue : snapshot.values) {
                    if (columnValue.column == column) {
                        columnValue.value = value;
                        return;
                    }
                }
                snapshot.values.push_back({
                    .column = column,
                    .value = value,
                });
            };
            auto hasChangeForColumn = [](const UpdateGroup &group, const dini::ColumnHandle &column) {
                return std::find_if(group.changes.begin(), group.changes.end(), [&column](const auto &change) {
                    return change.column == column;
                }) != group.changes.end();
            };
            auto emitAboutToMove = [spec](Owner *oldOwner,
                                          const QString &oldKey,
                                          Owner *newOwner,
                                          const QString &newKey,
                                          Object *item) {
                const bool placementChanged = oldOwner != newOwner || oldKey != newKey;
                if (!placementChanged) {
                    return;
                }
                if (oldOwner && spec.itemAboutToRemove) {
                    spec.itemAboutToRemove(oldOwner, oldKey, item, newOwner);
                }
                if (newOwner && spec.itemAboutToInsert) {
                    spec.itemAboutToInsert(newOwner, newKey, item, oldOwner);
                }
            };
            auto emitMoved = [spec](Owner *oldOwner,
                                    const QString &oldKey,
                                    Owner *newOwner,
                                    const QString &newKey,
                                    Object *item) {
                const bool placementChanged = oldOwner != newOwner || oldKey != newKey;
                if (!placementChanged) {
                    return;
                }
                if (oldOwner && spec.itemRemoved) {
                    spec.itemRemoved(oldOwner, oldKey, item, newOwner);
                }
                if (newOwner && spec.itemInserted) {
                    spec.itemInserted(newOwner, newKey, item, oldOwner);
                }
            };
            auto processColumnUpdates = [spec, groupForChange, applyValue, hasChangeForColumn, emitAboutToMove, emitMoved](
                                            ModelPrivate &model,
                                            const std::vector<dini::ColumnUpdatedChange> &changes) {
                std::vector<UpdateGroup> groups;
                for (const auto &change : changes) {
                    auto *group = groupForChange(groups, change.itemId);
                    if (!group) {
                        const auto *eventSnapshot = currentEventSnapshot(model, change.itemId);
                        if (!eventSnapshot && !getModelEngine(model)->contains(change.itemId)) {
                            continue;
                        }
                        auto snapshot = eventSnapshot ? *eventSnapshot : getModelEngine(model)->read(change.itemId);
                        groups.push_back({
                            .itemId = change.itemId,
                            .oldSnapshot = snapshot,
                            .newSnapshot = std::move(snapshot),
                        });
                        group = &groups.back();
                    }
                    if (!hasChangeForColumn(*group, change.column)) {
                        applyValue(group->oldSnapshot, change.column, change.oldValue);
                    }
                    applyValue(group->newSnapshot, change.column, change.newValue);
                    group->changes.push_back(change);
                    group->placement = group->placement || change.column == spec.associationColumn || change.column == spec.keyColumn;
                }

                for (const auto &group : groups) {
                    auto *item = spec.find(model, handleFromId(group.itemId));
                    auto *oldOwner = spec.ownerForSnapshot ? spec.ownerForSnapshot(model, group.oldSnapshot) : nullptr;
                    auto *newOwner = spec.ownerForSnapshot ? spec.ownerForSnapshot(model, group.newSnapshot) : nullptr;
                    const auto oldKey = spec.keyForSnapshot ? spec.keyForSnapshot(group.oldSnapshot) : QString();
                    const auto newKey = spec.keyForSnapshot ? spec.keyForSnapshot(group.newSnapshot) : QString();

                    if (!item) {
                        if (group.placement && spec.refreshOwner) {
                            refreshUniqueOwner(oldOwner, newOwner, spec.refreshOwner, true);
                        }
                        continue;
                    }

                    if (group.placement) {
                        emitAboutToMove(oldOwner, oldKey, newOwner, newKey, item);
                    }

                    spec.sync(item, group.newSnapshot, true);

                    if (group.placement && spec.refreshOwner) {
                        refreshUniqueOwner(oldOwner, newOwner, spec.refreshOwner, true);
                    }

                    if (group.placement) {
                        emitMoved(oldOwner, oldKey, newOwner, newKey, item);
                    }
                }
            };

            return TableBinding {
                .table = spec.table,
                .itemInserted = [spec](ModelPrivate &model, const dini::ItemInsertedChange &change) {
                    auto *item = spec.ensure(model, change.item);
                    auto *owner = item ? spec.ownerForSnapshot(model, change.item) : nullptr;
                    if (!item || !owner) {
                        return;
                    }
                    const auto key = spec.keyForSnapshot(change.item);
                    if (spec.itemAboutToInsert) {
                        spec.itemAboutToInsert(owner, key, item, nullptr);
                    }
                    spec.sync(item, change.item, true);
                    if (spec.refreshOwner) {
                        spec.refreshOwner(owner, true);
                    }
                    if (spec.itemInserted) {
                        spec.itemInserted(owner, key, item, nullptr);
                    }
                },
                .itemRemoved = [spec](ModelPrivate &model, const dini::ItemSnapshot &snapshot, bool) {
                    auto *item = spec.ensure(model, snapshot);
                    if (!item) {
                        return;
                    }
                    auto *owner = spec.ownerForSnapshot(model, snapshot);
                    const auto key = spec.keyForSnapshot(snapshot);
                    if (owner && spec.itemAboutToRemove) {
                        spec.itemAboutToRemove(owner, key, item, nullptr);
                    }
                    if (spec.setPlacement) {
                        spec.setPlacement(item, nullptr, QString(), true);
                    }
                    if (spec.removeObject) {
                        spec.removeObject(model, handleFromId(snapshot.id));
                    }
                    if (owner && spec.refreshOwner) {
                        spec.refreshOwner(owner, true);
                    }
                    if (owner && spec.itemRemoved) {
                        spec.itemRemoved(owner, key, item, nullptr);
                    }
                    item->deleteLater();
                },
                .columnUpdated = [processColumnUpdates](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    processColumnUpdates(model, {change});
                },
                .columnUpdates = [processColumnUpdates](ModelPrivate &model, const std::vector<dini::ColumnUpdatedChange> &changes) {
                    processColumnUpdates(model, changes);
                },
                .computedColumnUpdated = [spec](ModelPrivate &model, const dini::ComputedColumnUpdatedChange &change) {
                    if (auto *item = spec.find(model, handleFromId(change.itemId))) {
                        spec.applyColumn(item, change.column, change.newValue, true);
                    }
                },
            };
        }

        template <typename Object, typename Owner>
        struct IndexedListBindingSpec {
            dini::ListHandle list;
            dini::ColumnHandle associationColumn;

            std::function<Object *(ModelPrivate &, const dini::ItemSnapshot &)> ensure;
            std::function<Object *(ModelPrivate &, Handle)> find;
            std::function<void(ModelPrivate &, Handle)> removeObject;
            std::function<void(Object *, const dini::ItemSnapshot &, bool)> sync;
            std::function<bool(Object *, const dini::ColumnHandle &, const dini::Value &, bool)> applyColumn;

            std::function<Owner *(ModelPrivate &, const dini::Value &)> ownerForAssociationValue;
            std::function<Owner *(ModelPrivate &, const dini::ItemSnapshot &)> ownerForSnapshot;
            std::function<void(Object *, Owner *, bool)> setOwner;
            std::function<void(Owner *, bool, bool)> refreshOwner;

            std::function<void(Owner *, int, Object *, Owner *)> itemAboutToInsert;
            std::function<void(Owner *, int, Object *, Owner *)> itemInserted;
            std::function<void(Owner *, int, Object *, Owner *)> itemAboutToRemove;
            std::function<void(Owner *, int, Object *, Owner *)> itemRemoved;
            std::function<void(Owner *, int, int, int)> aboutToRotate;
            std::function<void(Owner *, int, int, int)> rotated;
        };

        template <typename Object, typename Owner>
        ListBinding makeIndexedListBinding(IndexedListBindingSpec<Object, Owner> spec) {
            return ListBinding {
                .list = spec.list,
                .itemInserted = [spec](ModelPrivate &model, const dini::ItemInsertedChange &change) {
                    if (auto *item = spec.ensure(model, change.item)) {
                        spec.sync(item, change.item, true);
                    }
                },
                .itemRemoved = [spec](ModelPrivate &model, const dini::ItemSnapshot &snapshot, bool) {
                    auto *item = spec.ensure(model, snapshot);
                    if (!item) {
                        return;
                    }
                    auto *owner = spec.ownerForSnapshot(model, snapshot);
                    const auto index = static_cast<int>(snapshot.listIndex.value_or(0));
                    if (owner && spec.itemAboutToRemove) {
                        spec.itemAboutToRemove(owner, index, item, nullptr);
                    }
                    if (spec.setOwner) {
                        spec.setOwner(item, nullptr, true);
                    }
                    spec.removeObject(model, handleFromId(snapshot.id));
                    if (owner && spec.refreshOwner) {
                        spec.refreshOwner(owner, true, true);
                    }
                    if (owner && spec.itemRemoved) {
                        spec.itemRemoved(owner, index, item, nullptr);
                    }
                    item->deleteLater();
                },
                .listInserted = [spec](ModelPrivate &model, const dini::ListInsertedChange &change) {
                    auto *owner = spec.ownerForAssociationValue(model, change.associationValue);
                    auto *item = spec.ensure(model, change.item);
                    if (!owner || !item) {
                        return;
                    }
                    const auto index = static_cast<int>(change.index);
                    if (spec.itemAboutToInsert) {
                        spec.itemAboutToInsert(owner, index, item, nullptr);
                    }
                    spec.sync(item, change.item, true);
                    if (spec.refreshOwner) {
                        spec.refreshOwner(owner, true, true);
                    }
                    if (spec.itemInserted) {
                        spec.itemInserted(owner, index, item, nullptr);
                    }
                },
                .listRemoved = [spec](ModelPrivate &model, const dini::ListRemovedChange &change) {
                    auto *owner = spec.ownerForAssociationValue(model, change.associationValue);
                    auto *item = spec.ensure(model, change.item);
                    if (!item) {
                        return;
                    }
                    const auto index = static_cast<int>(change.index);
                    if (owner && spec.itemAboutToRemove) {
                        spec.itemAboutToRemove(owner, index, item, nullptr);
                    }
                    if (spec.setOwner) {
                        spec.setOwner(item, nullptr, true);
                    }
                    spec.removeObject(model, handleFromId(change.item.id));
                    if (owner && spec.refreshOwner) {
                        spec.refreshOwner(owner, true, true);
                    }
                    if (owner && spec.itemRemoved) {
                        spec.itemRemoved(owner, index, item, nullptr);
                    }
                    item->deleteLater();
                },
                .columnUpdated = [spec](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    auto *item = spec.find(model, handleFromId(change.itemId));
                    if (change.column != spec.associationColumn) {
                        if (item) {
                            spec.applyColumn(item, change.column, change.newValue, true);
                        }
                        return;
                    }

                    auto *oldOwner = spec.ownerForAssociationValue(model, change.oldValue);
                    auto *newOwner = spec.ownerForAssociationValue(model, change.newValue);
                    if (!item) {
                        if (spec.refreshOwner) {
                            refreshUniqueOwner(oldOwner, newOwner, [spec](Owner *owner, bool notify) {
                                spec.refreshOwner(owner, notify, true);
                            }, true);
                        }
                        return;
                    }

                    const int oldIndex = static_cast<int>(change.oldListIndex.value_or(0));
                    const int newIndex = static_cast<int>(change.associationOptions.targetIndex.value_or(0));
                    if (oldOwner && !newOwner && spec.itemAboutToRemove) {
                        spec.itemAboutToRemove(oldOwner, oldIndex, item, nullptr);
                    } else if (!oldOwner && newOwner && spec.itemAboutToInsert) {
                        spec.itemAboutToInsert(newOwner, newIndex, item, nullptr);
                    } else if (oldOwner && newOwner) {
                        if (spec.itemAboutToRemove) {
                            spec.itemAboutToRemove(oldOwner, oldIndex, item, newOwner);
                        }
                        if (spec.itemAboutToInsert) {
                            spec.itemAboutToInsert(newOwner, newIndex, item, oldOwner);
                        }
                    }

                    spec.applyColumn(item, change.column, change.newValue, true);
                    if (spec.refreshOwner) {
                        refreshUniqueOwner(oldOwner, newOwner, [spec](Owner *owner, bool notify) {
                            spec.refreshOwner(owner, notify, true);
                        }, true);
                    }

                    if (oldOwner && !newOwner && spec.itemRemoved) {
                        spec.itemRemoved(oldOwner, oldIndex, item, nullptr);
                    } else if (!oldOwner && newOwner && spec.itemInserted) {
                        spec.itemInserted(newOwner, newIndex, item, nullptr);
                    } else if (oldOwner && newOwner) {
                        if (spec.itemRemoved) {
                            spec.itemRemoved(oldOwner, oldIndex, item, newOwner);
                        }
                        if (spec.itemInserted) {
                            spec.itemInserted(newOwner, newIndex, item, oldOwner);
                        }
                    }
                },
                .computedColumnUpdated = [spec](ModelPrivate &model, const dini::ComputedColumnUpdatedChange &change) {
                    if (auto *item = spec.find(model, handleFromId(change.itemId))) {
                        spec.applyColumn(item, change.column, change.newValue, true);
                    }
                },
                .listRotated = [spec](ModelPrivate &model, const dini::ListRotatedChange &change) {
                    auto *owner = spec.ownerForAssociationValue(model, change.associationValue);
                    if (!owner) {
                        return;
                    }
                    const auto normalized = change.rotation.count == 0 ? 0 : ((change.rotation.offset % static_cast<std::ptrdiff_t>(change.rotation.count)) + static_cast<std::ptrdiff_t>(change.rotation.count)) % static_cast<std::ptrdiff_t>(change.rotation.count);
                    const auto left = static_cast<int>(change.rotation.startIndex);
                    const auto middle = static_cast<int>(change.rotation.startIndex + static_cast<std::size_t>(normalized));
                    const auto right = static_cast<int>(change.rotation.startIndex + change.rotation.count);
                    if (spec.aboutToRotate) {
                        spec.aboutToRotate(owner, left, middle, right);
                    }
                    if (spec.refreshOwner) {
                        spec.refreshOwner(owner, true, true);
                    }
                    if (spec.rotated) {
                        spec.rotated(owner, left, middle, right);
                    }
                },
            };
        }

        template <typename Owner, typename ItemValue>
        struct DataArrayListBindingSpec {
            dini::ListHandle list;
            dini::ColumnHandle associationColumn;

            std::function<Owner *(ModelPrivate &, const dini::Value &)> ownerForAssociationValue;
            std::function<void(Owner *, bool, bool)> refreshOwner;
            std::function<ItemValue(const dini::ItemSnapshot &)> decodeItem;
            std::function<bool(Owner *)> notificationsSuppressed;

            std::function<void(Owner *, int, int, const QList<ItemValue> &)> aboutToSplice;
            std::function<void(Owner *, int, int, const QList<ItemValue> &)> spliced;
            std::function<void(Owner *, int, int, int)> aboutToRotate;
            std::function<void(Owner *, int, int, int)> rotated;
        };

        template <typename Owner, typename ItemValue>
        ListBinding makeDataArrayListBinding(DataArrayListBindingSpec<Owner, ItemValue> spec) {
            auto suppressed = [spec](Owner *owner) {
                return owner && spec.notificationsSuppressed && spec.notificationsSuppressed(owner);
            };
            auto refresh = [spec, suppressed](Owner *owner, bool itemsChanged) {
                if (owner && spec.refreshOwner && !suppressed(owner)) {
                    spec.refreshOwner(owner, true, itemsChanged);
                }
            };
            auto oneValue = [spec](const dini::ItemSnapshot &item) {
                QList<ItemValue> values;
                values.append(spec.decodeItem(item));
                return values;
            };
            auto emitSplice = [spec, suppressed, refresh](Owner *owner, int index, int length, const QList<ItemValue> &values) {
                if (!owner) {
                    return;
                }
                const bool shouldNotify = !suppressed(owner);
                if (shouldNotify && spec.aboutToSplice) {
                    spec.aboutToSplice(owner, index, length, values);
                }
                refresh(owner, true);
                if (shouldNotify && spec.spliced) {
                    spec.spliced(owner, index, length, values);
                }
            };
            return ListBinding {
                .list = spec.list,
                .itemInserted = [spec, refresh](ModelPrivate &model, const dini::ItemInsertedChange &change) {
                    if (change.item.listAssociationValue.has_value()) {
                        refresh(spec.ownerForAssociationValue(model, change.item.listAssociationValue.value()), true);
                    }
                },
                .itemRemoved = [spec, emitSplice](ModelPrivate &model, const dini::ItemSnapshot &snapshot, bool) {
                    if (!snapshot.listAssociationValue.has_value()) {
                        return;
                    }
                    auto *owner = spec.ownerForAssociationValue(model, snapshot.listAssociationValue.value());
                    const auto index = static_cast<int>(snapshot.listIndex.value_or(0));
                    emitSplice(owner, index, 1, QList<ItemValue>());
                },
                .listInserted = [spec, emitSplice, oneValue](ModelPrivate &model, const dini::ListInsertedChange &change) {
                    auto *owner = spec.ownerForAssociationValue(model, change.associationValue);
                    emitSplice(owner, static_cast<int>(change.index), 0, oneValue(change.item));
                },
                .listRemoved = [spec, emitSplice](ModelPrivate &model, const dini::ListRemovedChange &change) {
                    auto *owner = spec.ownerForAssociationValue(model, change.associationValue);
                    emitSplice(owner, static_cast<int>(change.index), 1, QList<ItemValue>());
                },
                .columnUpdated = [spec, emitSplice, oneValue, refresh](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    if (change.column == spec.associationColumn) {
                        auto *oldOwner = spec.ownerForAssociationValue(model, change.oldValue);
                        auto *newOwner = spec.ownerForAssociationValue(model, change.newValue);
                        emitSplice(oldOwner, static_cast<int>(change.oldListIndex.value_or(0)), 1, QList<ItemValue>());
                        const auto *eventSnapshot = currentEventSnapshot(model, change.itemId);
                        if (newOwner && (eventSnapshot || getModelEngine(model)->contains(change.itemId))) {
                            const auto item = eventSnapshot ? *eventSnapshot : getModelEngine(model)->read(change.itemId);
                            emitSplice(newOwner, static_cast<int>(change.associationOptions.targetIndex.value_or(0)), 0, oneValue(item));
                        }
                        return;
                    }
                    const auto *eventSnapshot = currentEventSnapshot(model, change.itemId);
                    if (!eventSnapshot && !getModelEngine(model)->contains(change.itemId)) {
                        return;
                    }
                    const auto item = eventSnapshot ? *eventSnapshot : getModelEngine(model)->read(change.itemId);
                    if (item.listAssociationValue.has_value()) {
                        refresh(spec.ownerForAssociationValue(model, item.listAssociationValue.value()), true);
                    }
                },
                .computedColumnUpdated = [spec, refresh](ModelPrivate &model, const dini::ComputedColumnUpdatedChange &change) {
                    const auto *eventSnapshot = currentEventSnapshot(model, change.itemId);
                    if (!eventSnapshot && !getModelEngine(model)->contains(change.itemId)) {
                        return;
                    }
                    const auto item = eventSnapshot ? *eventSnapshot : getModelEngine(model)->read(change.itemId);
                    if (item.listAssociationValue.has_value()) {
                        refresh(spec.ownerForAssociationValue(model, item.listAssociationValue.value()), true);
                    }
                },
                .listRotated = [spec, suppressed, refresh](ModelPrivate &model, const dini::ListRotatedChange &change) {
                    auto *owner = spec.ownerForAssociationValue(model, change.associationValue);
                    if (!owner) {
                        return;
                    }
                    const auto normalized = change.rotation.count == 0 ? 0 : ((change.rotation.offset % static_cast<std::ptrdiff_t>(change.rotation.count)) + static_cast<std::ptrdiff_t>(change.rotation.count)) % static_cast<std::ptrdiff_t>(change.rotation.count);
                    const auto left = static_cast<int>(change.rotation.startIndex);
                    const auto middle = static_cast<int>(change.rotation.startIndex + static_cast<std::size_t>(normalized));
                    const auto right = static_cast<int>(change.rotation.startIndex + change.rotation.count);
                    const bool shouldNotify = !suppressed(owner);
                    if (shouldNotify && spec.aboutToRotate) {
                        spec.aboutToRotate(owner, left, middle, right);
                    }
                    refresh(owner, true);
                    if (shouldNotify && spec.rotated) {
                        spec.rotated(owner, left, middle, right);
                    }
                },
            };
        }

        template <typename Factory>
        TableBinding makePolymorphicTableBinding(Factory factory) {
            return factory();
        }

        template <typename Factory>
        ListBinding makePolymorphicListBinding(Factory factory) {
            return factory();
        }

        const TableBinding &modelTableBinding();
        const TableBinding &anchorNodeTableBinding();
        const TableBinding &labelTableBinding();
        const TableBinding &keySignatureTableBinding();
        const TableBinding &tempoTableBinding();
        const TableBinding &timeSignatureTableBinding();
        const TableBinding &clipTableBinding();
        const TableBinding &dynamicMixingAnchorTableBinding();
        const TableBinding &noteTableBinding();
        const TableBinding &parameterTableBinding();
        const TableBinding &phonemeTableBinding();
        const TableBinding &sourcesTableBinding();
        const ListBinding &trackListBinding();
        const ListBinding &freeValueDataArrayBinding();
        const ListBinding &singerListBinding();
        const ListBinding &vibratoPointDataArrayBinding();

        void syncAnchorNodeColumns(AnchorNode *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyAnchorNodeColumn(AnchorNode *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncClipColumns(Clip *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyClipColumn(Clip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncDynamicMixingAnchorColumns(DynamicMixingAnchor *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyDynamicMixingAnchorColumn(DynamicMixingAnchor *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncLabelColumns(Label *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyLabelColumn(Label *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncNoteColumns(Note *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyNoteColumn(Note *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncParameterColumns(Parameter *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyParameterColumn(Parameter *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncPhonemeColumns(Phoneme *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyPhonemeColumn(Phoneme *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncSingerColumns(Singer *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applySingerColumn(Singer *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncSourcesColumns(Sources *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applySourcesColumn(Sources *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncKeySignatureColumns(KeySignature *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyKeySignatureColumn(KeySignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncTempoColumns(Tempo *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyTempoColumn(Tempo *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncTimeSignatureColumns(TimeSignature *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyTimeSignatureColumn(TimeSignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncTrackColumns(Track *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyTrackColumn(Track *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);

        const OrderSpec &labelOrderSpec();
        const OrderSpec &anchorNodeOrderSpec();
        const OrderSpec &keySignatureOrderSpec();
        const OrderSpec &tempoOrderSpec();
        const OrderSpec &timeSignatureOrderSpec();
        const OrderSpec &clipOrderSpec();
        const OrderSpec &dynamicMixingAnchorOrderSpec();
        const OrderSpec &noteOrderSpec();
        const OrderSpec &phonemeOrderSpec();

    }

}

#endif // DSPXMODEL_ORMBINDING_P_H
