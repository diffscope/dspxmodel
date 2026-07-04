#ifndef DSPXMODEL_ORMBINDING_P_H
#define DSPXMODEL_ORMBINDING_P_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

#include <dini/change.h>
#include <dini/query.h>
#include <dini/schema.h>
#include <dini/types.h>
#include <dini/value.h>

#include <dspxmodelORM/private/ORMUtils_p.h>

namespace dspx {

    class ModelPrivate;
    class Clip;
    class KeySignature;
    class Label;
    class Note;
    class Tempo;
    class TimeSignature;
    class Track;

    namespace orm {

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

        template <typename Object, typename Member, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> intField(dini::ColumnHandle column, Member Object::*member, Notify notify) {
            return ColumnBinding<Object> {
                .column = std::move(column),
                .apply = [member](Object *object, const dini::Value &value) {
                    const auto newValue = static_cast<Member>(value.asInt64());
                    const bool changed = object->*member != newValue;
                    object->*member = newValue;
                    return changed;
                },
                .notify = std::move(notify),
            };
        }

        template <typename Object, typename Private, typename Member>
        ColumnBinding<Object> intField(dini::ColumnHandle column, Member Private::*member, void (Object::*notify)(Member)) {
            return intField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Member, typename Emitter>
        ColumnBinding<Object> intField(dini::ColumnHandle column, Member Object::*member, void (Emitter::*notify)(Member), Emitter *(Object::*emitter)()) {
            return intField<Object>(std::move(column), member, [member, notify, emitter](Object *object) {
                ((object->*emitter)()->*notify)(object->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> doubleField(dini::ColumnHandle column, double Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return value.asDouble(); }, std::move(notify));
        }

        template <typename Object, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> doubleField(dini::ColumnHandle column, double Object::*member, Notify notify) {
            return ColumnBinding<Object> {
                .column = std::move(column),
                .apply = [member](Object *object, const dini::Value &value) {
                    const auto newValue = value.asDouble();
                    const bool changed = object->*member != newValue;
                    object->*member = newValue;
                    return changed;
                },
                .notify = std::move(notify),
            };
        }

        template <typename Object, typename Private>
        ColumnBinding<Object> doubleField(dini::ColumnHandle column, double Private::*member, void (Object::*notify)(double)) {
            return doubleField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Emitter>
        ColumnBinding<Object> doubleField(dini::ColumnHandle column, double Object::*member, void (Emitter::*notify)(double), Emitter *(Object::*emitter)()) {
            return doubleField<Object>(std::move(column), member, [member, notify, emitter](Object *object) {
                ((object->*emitter)()->*notify)(object->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> boolField(dini::ColumnHandle column, bool Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return value.asBool(); }, std::move(notify));
        }

        template <typename Object, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> boolField(dini::ColumnHandle column, bool Object::*member, Notify notify) {
            return ColumnBinding<Object> {
                .column = std::move(column),
                .apply = [member](Object *object, const dini::Value &value) {
                    const auto newValue = value.asBool();
                    const bool changed = object->*member != newValue;
                    object->*member = newValue;
                    return changed;
                },
                .notify = std::move(notify),
            };
        }

        template <typename Object, typename Private>
        ColumnBinding<Object> boolField(dini::ColumnHandle column, bool Private::*member, void (Object::*notify)(bool)) {
            return boolField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Emitter>
        ColumnBinding<Object> boolField(dini::ColumnHandle column, bool Object::*member, void (Emitter::*notify)(bool), Emitter *(Object::*emitter)()) {
            return boolField<Object>(std::move(column), member, [member, notify, emitter](Object *object) {
                ((object->*emitter)()->*notify)(object->*member);
            });
        }

        template <typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> stringField(dini::ColumnHandle column, QString Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return stringFromValue(value); }, std::move(notify));
        }

        template <typename Object, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> stringField(dini::ColumnHandle column, QString Object::*member, Notify notify) {
            return ColumnBinding<Object> {
                .column = std::move(column),
                .apply = [member](Object *object, const dini::Value &value) {
                    const auto newValue = stringFromValue(value);
                    const bool changed = object->*member != newValue;
                    object->*member = newValue;
                    return changed;
                },
                .notify = std::move(notify),
            };
        }

        template <typename Object, typename Private>
        ColumnBinding<Object> stringField(dini::ColumnHandle column, QString Private::*member, void (Object::*notify)(const QString &)) {
            return stringField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Private>
        ColumnBinding<Object> stringField(dini::ColumnHandle column, QString Private::*member, void (Object::*notify)(QString)) {
            return stringField<Object, Private>(std::move(column), member, [member, notify](Object *object) {
                (object->*notify)(Private::get(object)->*member);
            });
        }

        template <typename Object, typename Emitter>
        ColumnBinding<Object> stringField(dini::ColumnHandle column, QString Object::*member, void (Emitter::*notify)(const QString &), Emitter *(Object::*emitter)()) {
            return stringField<Object>(std::move(column), member, [member, notify, emitter](Object *object) {
                ((object->*emitter)()->*notify)(object->*member);
            });
        }

        template <typename Enum, typename Object, typename Private, typename Notify>
            requires (!std::is_member_function_pointer_v<std::decay_t<Notify>>)
        ColumnBinding<Object> enumField(dini::ColumnHandle column, Enum Private::*member, Notify notify) {
            return field<Object, Private>(std::move(column), member, [](const dini::Value &value) { return static_cast<Enum>(value.asInt64()); }, std::move(notify));
        }

        template <typename Enum, typename Object, typename Private>
        ColumnBinding<Object> enumField(dini::ColumnHandle column, Enum Private::*member, void (Object::*notify)(Enum)) {
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

        template <typename Object, typename Private, typename Related>
        ColumnBinding<Object> previousNextField(dini::ColumnHandle column, Handle Private::*handleMember, Related *Private::*objectMember, void (Object::*notify)(Related *)) {
            return previousNextField<Object, Private, Related>(std::move(column), handleMember, objectMember, [objectMember, notify](Object *object) {
                (object->*notify)(Private::get(object)->*objectMember);
            });
        }

        struct TableBinding {
            dini::TableHandle table;
            std::function<void(ModelPrivate &, const dini::ItemInsertedChange &)> itemInserted;
            std::function<void(ModelPrivate &, const dini::ItemSnapshot &, bool)> itemRemoved;
            std::function<void(ModelPrivate &, const dini::ColumnUpdatedChange &)> columnUpdated;
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
                .columnUpdated = [spec, containsColumn](ModelPrivate &model, const dini::ColumnUpdatedChange &change) {
                    const bool membership = containsColumn(spec.membershipColumns, change.column);
                    const bool order = containsColumn(spec.orderColumns, change.column);
                    auto *item = spec.find(model, handleFromId(change.itemId));
                    auto *oldOwner = spec.ownerForChange ? spec.ownerForChange(model, change, true) : nullptr;
                    auto *newOwner = spec.ownerForChange ? spec.ownerForChange(model, change, false) : nullptr;

                    if (!item) {
                        if ((membership || order) && spec.refreshOwner) {
                            refreshUniqueOwner(oldOwner, newOwner, spec.refreshOwner, true);
                        }
                        return;
                    }

                    if (membership) {
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

                    spec.applyColumn(item, change.column, change.newValue, true);

                    if ((membership || order) && spec.refreshOwner) {
                        refreshUniqueOwner(oldOwner, newOwner, spec.refreshOwner, true);
                    }

                    if (membership) {
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

        template <typename Factory>
        TableBinding makePolymorphicTableBinding(Factory factory) {
            return factory();
        }

        template <typename Factory>
        ListBinding makePolymorphicListBinding(Factory factory) {
            return factory();
        }

        const TableBinding &modelTableBinding();
        const TableBinding &labelTableBinding();
        const TableBinding &keySignatureTableBinding();
        const TableBinding &tempoTableBinding();
        const TableBinding &timeSignatureTableBinding();
        const TableBinding &clipTableBinding();
        const TableBinding &noteTableBinding();
        const ListBinding &trackListBinding();

        void syncClipColumns(Clip *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyClipColumn(Clip *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncLabelColumns(Label *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyLabelColumn(Label *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncNoteColumns(Note *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyNoteColumn(Note *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncKeySignatureColumns(KeySignature *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyKeySignatureColumn(KeySignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncTempoColumns(Tempo *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyTempoColumn(Tempo *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncTimeSignatureColumns(TimeSignature *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyTimeSignatureColumn(TimeSignature *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);
        void syncTrackColumns(Track *item, const dini::ItemSnapshot &snapshot, bool notify);
        bool applyTrackColumn(Track *item, const dini::ColumnHandle &column, const dini::Value &value, bool notify);

        const OrderSpec &labelOrderSpec();
        const OrderSpec &keySignatureOrderSpec();
        const OrderSpec &tempoOrderSpec();
        const OrderSpec &timeSignatureOrderSpec();
        const OrderSpec &clipOrderSpec();
        const OrderSpec &noteOrderSpec();

    }

}

#endif // DSPXMODEL_ORMBINDING_P_H
