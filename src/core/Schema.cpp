#include "Schema.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <dini/engine.h>
#include <dini/errors.h>
#include <dini/schema.h>
#include <dini/transaction.h>

namespace dspx {

    namespace {

        dini::Value itemValue(const dini::ItemSnapshot &item, const dini::ColumnHandle &column) {
            for (const auto &columnValue : item.values) {
                if (columnValue.column == column) {
                    return columnValue.value;
                }
            }
            return dini::Value::null();
        }

        bool hasItemValue(const dini::ItemSnapshot &item, const dini::ColumnHandle &column) {
            return std::any_of(item.values.begin(), item.values.end(), [&](const auto &columnValue) {
                return columnValue.column == column;
            });
        }

        void setItemValue(dini::ItemSnapshot &item, const dini::ColumnHandle &column, dini::Value value) {
            for (auto &columnValue : item.values) {
                if (columnValue.column == column) {
                    columnValue.value = std::move(value);
                    return;
                }
            }
            item.values.push_back({
                .column = column,
                .value = std::move(value),
            });
        }

        bool checkRatioBinary(const dini::Value &value) {
            const auto &bytes = value.asBinary();
            if (bytes.size() % sizeof(double) != 0) {
                return false;
            }

            double sum = 0.0;
            for (std::size_t i = 0; i < bytes.size(); i += sizeof(double)) {
                double item = 0.0;
                std::memcpy(&item, bytes.data() + i, sizeof(double));
                if (!std::isfinite(item) || item < 0.0 || item > 1.0) {
                    return false;
                }
                sum += item;
                if (sum > 1.0) {
                    return false;
                }
            }
            return true;
        }

        dini::ItemSnapshot querySingle(dini::TransactionContext &ctx, const dini::TableHandle &table, quint64 id, const char *message) {
            auto result = ctx.engine().query(table, {
                .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::id(), dini::ComparisonOperator::Equal, id))
            }).toVector();
            if (result.size() != 1) {
                throw dini::ConstraintError(message);
            }
            return result.front();
        }

        dini::ItemSnapshot querySingle(dini::TransactionContext &ctx, const dini::ListHandle &list, quint64 id, const char *message) {
            auto result = ctx.engine().query(list, {
                .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::id(), dini::ComparisonOperator::Equal, id))
            }).toVector();
            if (result.size() != 1) {
                throw dini::ConstraintError(message);
            }
            return result.front();
        }

        struct ParentVariantCheckHook {

            explicit ParentVariantCheckHook(dini::TableHandle parentTable, dini::RelationHandle parentRelation, dini::VariantHandle variant)
                : parentTable(std::move(parentTable)), parentRelation(std::move(parentRelation)), variant(std::move(variant)) {

            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }
                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemInserted) {
                        const auto &inserted = std::get<dini::ItemInsertedChange>(change.payload());
                        if (const auto &parent = inserted.item.listAssociationValue; parent && !parent->isNull()) {
                            queryAndCheck(ctx, parent->asUInt64());
                        }
                    } else if (change.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                        const auto &updated = std::get<dini::ColumnUpdatedChange>(change.payload());
                        if (updated.column == parentRelation.column() && !updated.newValue.isNull()) {
                            queryAndCheck(ctx, updated.newValue.asUInt64());
                        }
                    } else if (change.kind() == dini::ChangeOperationKind::ListInserted) {
                        const auto &inserted = std::get<dini::ListInsertedChange>(change.payload());
                        if (const auto &parent = inserted.item.listAssociationValue; parent && !parent->isNull()) {
                            queryAndCheck(ctx, parent->asUInt64());
                        }
                    }
                }
            }

        private:
            void queryAndCheck(dini::TransactionContext &ctx, quint64 id) const {
                auto result = ctx.engine().query(parentTable, {
                    .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::id(), dini::ComparisonOperator::Equal, id))
                }).toVector();
                Q_ASSERT(result.size() == 1);
                const auto &item = result.front();
                Q_ASSERT(item.variant.has_value());
                if (item.variant.value() != variant) {
                    throw dini::ConstraintError("parent variant does not match");
                }
            }

            dini::TableHandle parentTable;
            dini::RelationHandle parentRelation;
            dini::VariantHandle variant;
        };

        struct SourcesAssociationCheckHook {
            explicit SourcesAssociationCheckHook(dini::TableHandle clipTable, dini::TableHandle sourcesTable,
                                                 dini::RelationHandle sourcesParent, dini::VariantHandle singingClipVariant)
                : clipTable(std::move(clipTable)), sourcesTable(std::move(sourcesTable)),
                  sourcesParent(std::move(sourcesParent)), singingClipVariant(std::move(singingClipVariant)) {
            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }
                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemInserted) {
                        const auto &inserted = std::get<dini::ItemInsertedChange>(change.payload());
                        validate(ctx, inserted.item.id, parentValue(inserted.item));
                    } else if (change.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                        const auto &updated = std::get<dini::ColumnUpdatedChange>(change.payload());
                        if (updated.column == sourcesParent.column()) {
                            validate(ctx, updated.itemId, updated.newValue);
                        }
                    }
                }
            }

        private:
            dini::Value parentValue(const dini::ItemSnapshot &item) const {
                if (item.parentId.has_value()) {
                    return item.parentId.value();
                }
                return itemValue(item, sourcesParent.column());
            }

            void validate(dini::TransactionContext &ctx, quint64 itemId, const dini::Value &parent) const {
                if (parent.isNull()) {
                    return;
                }

                const auto singingClipId = parent.asUInt64();
                const auto clip = querySingle(ctx, clipTable, singingClipId, "sources singing clip does not exist");
                if (!clip.variant.has_value() || clip.variant.value() != singingClipVariant) {
                    throw dini::ConstraintError("sources parent variant does not match");
                }

                auto result = ctx.engine().query(sourcesTable, {
                    .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::parent(sourcesParent),
                                                                  dini::ComparisonOperator::Equal,
                                                                  singingClipId))
                }).toVector();
                for (const auto &source : result) {
                    if (source.id != itemId) {
                        throw dini::ConstraintError("singing clip sources must be unique");
                    }
                }
            }

            dini::TableHandle clipTable;
            dini::TableHandle sourcesTable;
            dini::RelationHandle sourcesParent;
            dini::VariantHandle singingClipVariant;
        };

        struct MixableAssociationCheckHook {
            explicit MixableAssociationCheckHook(dini::TableHandle sourcesTable, dini::TableHandle mixableTable,
                                                 dini::ListHandle singerList, dini::ColumnHandle sourcesColumn,
                                                 dini::ColumnHandle mixedSingerColumn,
                                                 dini::VariantHandle mixedSingerVariant)
                : sourcesTable(std::move(sourcesTable)), mixableTable(std::move(mixableTable)),
                  singerList(std::move(singerList)), sourcesColumn(std::move(sourcesColumn)),
                  mixedSingerColumn(std::move(mixedSingerColumn)), mixedSingerVariant(std::move(mixedSingerVariant)) {
            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }
                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemInserted) {
                        const auto &inserted = std::get<dini::ItemInsertedChange>(change.payload());
                        validate(ctx, inserted.item);
                    } else if (change.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                        const auto &updated = std::get<dini::ColumnUpdatedChange>(change.payload());
                        if (updated.column == sourcesColumn || updated.column == mixedSingerColumn) {
                            auto item = querySingle(ctx, mixableTable, updated.itemId, "mixable item does not exist");
                            setItemValue(item, updated.column, updated.newValue);
                            validate(ctx, item);
                        }
                    }
                }
            }

        private:
            void validate(dini::TransactionContext &ctx, const dini::ItemSnapshot &item) const {
                const auto sources = itemValue(item, sourcesColumn);
                const auto mixedSinger = itemValue(item, mixedSingerColumn);
                const bool hasSources = !sources.isNull();
                const bool hasMixedSinger = !mixedSinger.isNull();
                if (hasSources == hasMixedSinger) {
                    throw dini::ConstraintError("mixable must reference exactly one virtual parent");
                }

                if (hasSources) {
                    querySingle(ctx, sourcesTable, sources.asUInt64(), "mixable sources does not exist");
                    ensureUnique(ctx, item.id, sourcesColumn, sources);
                } else {
                    const auto singer = querySingle(ctx, singerList, mixedSinger.asUInt64(), "mixable mixed singer does not exist");
                    if (!singer.variant.has_value() || singer.variant.value() != mixedSingerVariant) {
                        throw dini::ConstraintError("mixable mixed singer variant does not match");
                    }
                    ensureUnique(ctx, item.id, mixedSingerColumn, mixedSinger);
                }
            }

            void ensureUnique(dini::TransactionContext &ctx, quint64 itemId, const dini::ColumnHandle &column,
                              const dini::Value &value) const {
                auto result = ctx.engine().query(mixableTable, {
                    .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::column(column),
                                                                  dini::ComparisonOperator::Equal,
                                                                  value))
                }).toVector();
                for (const auto &mixable : result) {
                    if (mixable.id != itemId) {
                        throw dini::ConstraintError("mixable virtual parent must be unique");
                    }
                }
            }

            dini::TableHandle sourcesTable;
            dini::TableHandle mixableTable;
            dini::ListHandle singerList;
            dini::ColumnHandle sourcesColumn;
            dini::ColumnHandle mixedSingerColumn;
            dini::VariantHandle mixedSingerVariant;
        };

        struct RoleAssociationCheckHook {
            explicit RoleAssociationCheckHook(dini::TableHandle table, dini::RelationHandle association,
                                              dini::ColumnHandle roleColumn)
                : table(std::move(table)), association(std::move(association)), roleColumn(std::move(roleColumn)) {
            }

            explicit RoleAssociationCheckHook(dini::ListHandle list, dini::RelationHandle association,
                                              dini::ColumnHandle roleColumn)
                : list(std::move(list)), association(std::move(association)), roleColumn(std::move(roleColumn)) {
            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }
                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemInserted) {
                        const auto &inserted = std::get<dini::ItemInsertedChange>(change.payload());
                        validate(inserted.item);
                    } else if (change.kind() == dini::ChangeOperationKind::ListInserted) {
                        const auto &inserted = std::get<dini::ListInsertedChange>(change.payload());
                        validate(inserted.item);
                    } else if (change.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                        const auto &updated = std::get<dini::ColumnUpdatedChange>(change.payload());
                        if (updated.column == association.column()) {
                            const auto item = query(ctx, updated.itemId);
                            validate(updated.newValue, itemValue(item, roleColumn));
                        } else if (updated.column == roleColumn) {
                            const auto item = query(ctx, updated.itemId);
                            validate(associationValue(item), updated.newValue);
                        }
                    }
                }
            }

        private:
            dini::ItemSnapshot query(dini::TransactionContext &ctx, quint64 id) const {
                if (table.has_value()) {
                    return querySingle(ctx, *table, id, "role-associated item does not exist");
                }
                return querySingle(ctx, *list, id, "role-associated item does not exist");
            }

            dini::Value associationValue(const dini::ItemSnapshot &item) const {
                if (item.listAssociationValue.has_value()) {
                    return item.listAssociationValue.value();
                }
                if (item.parentId.has_value()) {
                    return item.parentId.value();
                }
                return itemValue(item, association.column());
            }

            void validate(const dini::ItemSnapshot &item) const {
                validate(associationValue(item), itemValue(item, roleColumn));
            }

            void validate(const dini::Value &associationValue, const dini::Value &roleValue) const {
                const bool hasAssociation = !associationValue.isNull();
                const bool hasRole = !roleValue.isNull();
                if (hasAssociation != hasRole) {
                    throw dini::ConstraintError("role and association must both be null or both be non-null");
                }
            }

            std::optional<dini::TableHandle> table;
            std::optional<dini::ListHandle> list;
            dini::RelationHandle association;
            dini::ColumnHandle roleColumn;
        };

        struct VirtualCascadeDeleteHook {
            enum class Mode {
                Sources,
                MixedSinger,
            };

            explicit VirtualCascadeDeleteHook(Mode mode, dini::TableHandle mixableTable,
                                              dini::ColumnHandle virtualParentColumn,
                                              dini::VariantHandle mixedSingerVariant = {})
                : mode(mode), mixableTable(std::move(mixableTable)),
                  virtualParentColumn(std::move(virtualParentColumn)),
                  mixedSingerVariant(std::move(mixedSingerVariant)) {
            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }
                for (const auto &change : changeSet.operations()) {
                    if (mode == Mode::Sources) {
                        if (change.kind() == dini::ChangeOperationKind::ItemRemoved) {
                            const auto &removed = std::get<dini::ItemRemovedChange>(change.payload());
                            removeMixables(ctx, removed.item.id);
                        } else if (change.kind() == dini::ChangeOperationKind::CascadeRemoved) {
                            const auto &removed = std::get<dini::CascadeRemovedChange>(change.payload());
                            removeMixables(ctx, removed.item.id);
                        }
                    } else {
                        removeForMixedSingerChange(ctx, change);
                    }
                }
            }

        private:
            void removeForMixedSingerChange(dini::TransactionContext &ctx, const dini::ChangeOperation &change) const {
                if (change.kind() == dini::ChangeOperationKind::ItemRemoved) {
                    const auto &removed = std::get<dini::ItemRemovedChange>(change.payload());
                    removeMixablesForSinger(ctx, removed.item);
                } else if (change.kind() == dini::ChangeOperationKind::CascadeRemoved) {
                    const auto &removed = std::get<dini::CascadeRemovedChange>(change.payload());
                    removeMixablesForSinger(ctx, removed.item, removed.ancestorId);
                } else if (change.kind() == dini::ChangeOperationKind::ListRemoved) {
                    const auto &removed = std::get<dini::ListRemovedChange>(change.payload());
                    removeMixablesForSinger(ctx, removed.item);
                }
            }

            void removeMixablesForSinger(dini::TransactionContext &ctx, const dini::ItemSnapshot &item,
                                         std::optional<quint64> skipMixableId = {}) const {
                if (!item.variant.has_value() || item.variant.value() != mixedSingerVariant) {
                    return;
                }
                removeMixables(ctx, item.id, skipMixableId);
            }

            void removeMixables(dini::TransactionContext &ctx, quint64 parentId,
                                std::optional<quint64> skipMixableId = {}) const {
                auto result = ctx.engine().query(mixableTable, {
                    .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::column(virtualParentColumn),
                                                                  dini::ComparisonOperator::Equal,
                                                                  parentId))
                }).toVector();
                for (const auto &mixable : result) {
                    if (skipMixableId.has_value() && mixable.id == skipMixableId.value()) {
                        continue;
                    }
                    ctx.remove(mixable.id);
                }
            }

            Mode mode;
            dini::TableHandle mixableTable;
            dini::ColumnHandle virtualParentColumn;
            dini::VariantHandle mixedSingerVariant;
        };

        int compareValue(const dini::Value &lhs, const dini::Value &rhs) {
            if (lhs.storage() == rhs.storage()) {
                return 0;
            }
            if (lhs.type() != rhs.type()) {
                return static_cast<int>(lhs.type()) < static_cast<int>(rhs.type()) ? -1 : 1;
            }
            switch (lhs.type()) {
                case dini::ValueType::Bool:
                    return lhs.asBool() < rhs.asBool() ? -1 : 1;
                case dini::ValueType::Int64:
                    return lhs.asInt64() < rhs.asInt64() ? -1 : 1;
                case dini::ValueType::UInt64:
                    return lhs.asUInt64() < rhs.asUInt64() ? -1 : 1;
                case dini::ValueType::Double:
                    return lhs.asDouble() < rhs.asDouble() ? -1 : 1;
                case dini::ValueType::String:
                    return lhs.asString() < rhs.asString() ? -1 : 1;
                default:
                    return 0;
            }
        }

        dini::Value itemIdValue(std::optional<dini::ItemId> id) {
            return id.has_value() ? dini::Value(static_cast<std::uint64_t>(*id)) : dini::Value::null();
        }

        thread_local int orderedLinkUpdateDepth = 0;

        struct ScopedOrderedLinkUpdate {
            ScopedOrderedLinkUpdate() {
                ++orderedLinkUpdateDepth;
            }

            ~ScopedOrderedLinkUpdate() {
                --orderedLinkUpdateDepth;
            }
        };

        bool orderedLinkUpdateAllowed() {
            return orderedLinkUpdateDepth > 0;
        }

        struct OrderedLinkColumns {
            dini::ColumnHandle previous;
            dini::ColumnHandle next;
        };

        OrderedLinkColumns addOrderedLinkColumns(dini::TableBuilder &builder) {
            return {
                .previous = builder.addColumn({
                    .debugName = "previousItem",
                    .type = dini::ValueType::UInt64,
                    .nullable = true,
                }),
                .next = builder.addColumn({
                    .debugName = "nextItem",
                    .type = dini::ValueType::UInt64,
                    .nullable = true,
                }),
            };
        }

        struct OrderedLinkHook {
            explicit OrderedLinkHook(dini::TableHandle table,
                                     dini::RelationHandle parent,
                                     std::vector<dini::ColumnHandle> orderColumns,
                                     dini::ColumnHandle previousColumn,
                                     dini::ColumnHandle nextColumn,
                                     std::optional<dini::ColumnHandle> roleColumn = {})
                : table(std::move(table)),
                  parent(std::move(parent)),
                  roleColumn(std::move(roleColumn)),
                  orderColumns(std::move(orderColumns)),
                  previousColumn(std::move(previousColumn)),
                  nextColumn(std::move(nextColumn)) {
            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }

                std::set<dini::ItemId> removedIds;
                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemRemoved) {
                        removedIds.insert(std::get<dini::ItemRemovedChange>(change.payload()).item.id);
                    } else if (change.kind() == dini::ChangeOperationKind::CascadeRemoved) {
                        removedIds.insert(std::get<dini::CascadeRemovedChange>(change.payload()).item.id);
                    }
                }

                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemInserted) {
                        const auto &inserted = std::get<dini::ItemInsertedChange>(change.payload());
                        rejectDirectInsertedLinks(inserted.item);
                        attach(ctx, inserted.item, {});
                    } else if (change.kind() == dini::ChangeOperationKind::ItemRemoved) {
                        const auto &removed = std::get<dini::ItemRemovedChange>(change.payload());
                        detach(ctx, removed.item, removedIds);
                    } else if (change.kind() == dini::ChangeOperationKind::CascadeRemoved) {
                        const auto &removed = std::get<dini::CascadeRemovedChange>(change.payload());
                        detach(ctx, removed.item, removedIds);
                    } else if (change.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                        const auto &updated = std::get<dini::ColumnUpdatedChange>(change.payload());
                        if (isLinkColumn(updated.column)) {
                            if (!orderedLinkUpdateAllowed()) {
                                throw dini::ConstraintError("ordered link columns are derived");
                            }
                            continue;
                        }
                        if (!affectsPlacement(updated.column)) {
                            continue;
                        }
                        auto oldItem = querySingle(ctx, table, updated.itemId, "ordered item does not exist");
                        auto newItem = oldItem;
                        setItemValue(oldItem, updated.column, updated.oldValue);
                        setItemValue(newItem, updated.column, updated.newValue);
                        if (samePlacement(oldItem, newItem)) {
                            continue;
                        }
                        std::set<dini::ItemId> excluded {updated.itemId};
                        detach(ctx, oldItem, excluded);
                        attach(ctx, newItem, excluded);
                    }
                }
            }

        private:
            bool isLinkColumn(const dini::ColumnHandle &column) const {
                return column == previousColumn || column == nextColumn;
            }

            bool affectsPlacement(const dini::ColumnHandle &column) const {
                if (column == parent.column()) {
                    return true;
                }
                if (roleColumn.has_value() && column == *roleColumn) {
                    return true;
                }
                return std::any_of(orderColumns.begin(), orderColumns.end(), [&](const auto &orderColumn) {
                    return column == orderColumn;
                });
            }

            bool hasSequenceKey(const dini::ItemSnapshot &item) const {
                if (itemValue(item, parent.column()).isNull()) {
                    return false;
                }
                return !roleColumn.has_value() || !itemValue(item, *roleColumn).isNull();
            }

            bool samePlacement(const dini::ItemSnapshot &lhs, const dini::ItemSnapshot &rhs) const {
                if (itemValue(lhs, parent.column()) != itemValue(rhs, parent.column())) {
                    return false;
                }
                if (roleColumn.has_value() && itemValue(lhs, *roleColumn) != itemValue(rhs, *roleColumn)) {
                    return false;
                }
                for (const auto &column : orderColumns) {
                    if (itemValue(lhs, column) != itemValue(rhs, column)) {
                        return false;
                    }
                }
                return true;
            }

            int compareOrder(const dini::ItemSnapshot &lhs, const dini::ItemSnapshot &rhs) const {
                for (const auto &column : orderColumns) {
                    const auto comparison = compareValue(itemValue(lhs, column), itemValue(rhs, column));
                    if (comparison != 0) {
                        return comparison;
                    }
                }
                if (lhs.id == rhs.id) {
                    return 0;
                }
                return lhs.id < rhs.id ? -1 : 1;
            }

            dini::FilterExpression filterExpression(std::vector<dini::FilterExpression> filters) const {
                if (filters.empty()) {
                    return {};
                }
                if (filters.size() == 1) {
                    return filters.front();
                }
                return dini::FilterExpression::all(std::move(filters));
            }

            std::vector<dini::FilterExpression> groupFilters(const dini::ItemSnapshot &item) const {
                std::vector<dini::FilterExpression> filters {
                    dini::FilterExpression(dini::Filter(dini::FieldRef::parent(parent),
                                                        dini::ComparisonOperator::Equal,
                                                        itemValue(item, parent.column()))),
                };
                if (roleColumn.has_value()) {
                    filters.push_back(dini::FilterExpression(dini::Filter(dini::FieldRef::column(*roleColumn),
                                                                          dini::ComparisonOperator::Equal,
                                                                          itemValue(item, *roleColumn))));
                }
                return filters;
            }

            void addExclusionFilters(std::vector<dini::FilterExpression> &filters,
                                     const std::set<dini::ItemId> &excludedIds) const {
                for (const auto id : excludedIds) {
                    filters.push_back(dini::FilterExpression(dini::Filter(dini::FieldRef::id(),
                                                                          dini::ComparisonOperator::NotEqual,
                                                                          dini::Value(static_cast<std::uint64_t>(id)))));
                }
            }

            std::vector<dini::SortKey> fullSort(bool descending = false) const {
                std::vector<dini::SortKey> keys;
                keys.reserve(orderColumns.size() + 1);
                const auto direction = descending ? dini::SortDirection::Descending : dini::SortDirection::Ascending;
                for (const auto &column : orderColumns) {
                    keys.push_back({.field = dini::FieldRef::column(column), .direction = direction});
                }
                keys.push_back({.field = dini::FieldRef::id(), .direction = direction});
                return keys;
            }

            std::vector<dini::SortKey> primarySort(bool descending = false) const {
                return {{
                    .field = dini::FieldRef::column(orderColumns.front()),
                    .direction = descending ? dini::SortDirection::Descending : dini::SortDirection::Ascending,
                }};
            }

            std::vector<dini::ItemSnapshot> queryExactPrimary(dini::TransactionContext &ctx,
                                                              const dini::ItemSnapshot &item,
                                                              const dini::Value &primaryValue,
                                                              const std::set<dini::ItemId> &excludedIds) const {
                auto filters = groupFilters(item);
                filters.push_back(dini::FilterExpression(dini::Filter(dini::FieldRef::column(orderColumns.front()),
                                                                      dini::ComparisonOperator::Equal,
                                                                      primaryValue)));
                addExclusionFilters(filters, excludedIds);
                return ctx.engine().query(table, {
                    .filter = filterExpression(std::move(filters)),
                    .sortKeys = fullSort(),
                }).toVector();
            }

            std::optional<dini::ItemSnapshot> queryDifferentPrimary(dini::TransactionContext &ctx,
                                                                    const dini::ItemSnapshot &item,
                                                                    dini::ComparisonOperator op,
                                                                    bool descending,
                                                                    const std::set<dini::ItemId> &excludedIds) const {
                auto filters = groupFilters(item);
                filters.push_back(dini::FilterExpression(dini::Filter(dini::FieldRef::column(orderColumns.front()),
                                                                      op,
                                                                      itemValue(item, orderColumns.front()))));
                addExclusionFilters(filters, excludedIds);
                auto result = ctx.engine().query(table, {
                    .filter = filterExpression(std::move(filters)),
                    .sortKeys = primarySort(descending),
                }).limit(1).toVector();
                if (result.empty()) {
                    return {};
                }

                const auto primaryValue = itemValue(result.front(), orderColumns.front());
                auto bucket = queryExactPrimary(ctx, item, primaryValue, excludedIds);
                if (bucket.empty()) {
                    return {};
                }
                return descending ? std::optional<dini::ItemSnapshot>(bucket.back()) : std::optional<dini::ItemSnapshot>(bucket.front());
            }

            std::optional<dini::ItemSnapshot> previous(dini::TransactionContext &ctx,
                                                       const dini::ItemSnapshot &item,
                                                       const std::set<dini::ItemId> &excludedIds) const {
                if (!hasSequenceKey(item)) {
                    return {};
                }
                auto bucket = queryExactPrimary(ctx, item, itemValue(item, orderColumns.front()), excludedIds);
                std::optional<dini::ItemSnapshot> result;
                for (const auto &candidate : bucket) {
                    if (compareOrder(candidate, item) < 0) {
                        result = candidate;
                    } else {
                        break;
                    }
                }
                if (result.has_value()) {
                    return result;
                }
                return queryDifferentPrimary(ctx, item, dini::ComparisonOperator::Less, true, excludedIds);
            }

            std::optional<dini::ItemSnapshot> next(dini::TransactionContext &ctx,
                                                   const dini::ItemSnapshot &item,
                                                   const std::set<dini::ItemId> &excludedIds) const {
                if (!hasSequenceKey(item)) {
                    return {};
                }
                auto bucket = queryExactPrimary(ctx, item, itemValue(item, orderColumns.front()), excludedIds);
                for (const auto &candidate : bucket) {
                    if (compareOrder(candidate, item) > 0) {
                        return candidate;
                    }
                }
                return queryDifferentPrimary(ctx, item, dini::ComparisonOperator::Greater, false, excludedIds);
            }

            void updateLink(dini::TransactionContext &ctx,
                            dini::ItemId itemId,
                            const dini::ColumnHandle &column,
                            dini::Value value) const {
                if (itemId == 0) {
                    return;
                }
                if (ctx.engine().contains(itemId) && ctx.engine().read(itemId, column) == value) {
                    return;
                }
                ScopedOrderedLinkUpdate allowed;
                ctx.update(itemId, column, std::move(value));
            }

            void attach(dini::TransactionContext &ctx,
                        const dini::ItemSnapshot &item,
                        const std::set<dini::ItemId> &excludedIds) const {
                if (!hasSequenceKey(item)) {
                    updateLink(ctx, item.id, previousColumn, dini::Value::null());
                    updateLink(ctx, item.id, nextColumn, dini::Value::null());
                    return;
                }

                auto previousItem = previous(ctx, item, excludedIds);
                auto nextItem = next(ctx, item, excludedIds);
                updateLink(ctx, item.id, previousColumn, itemIdValue(previousItem ? std::optional<dini::ItemId>(previousItem->id) : std::nullopt));
                updateLink(ctx, item.id, nextColumn, itemIdValue(nextItem ? std::optional<dini::ItemId>(nextItem->id) : std::nullopt));
                if (previousItem.has_value()) {
                    updateLink(ctx, previousItem->id, nextColumn, dini::Value(static_cast<std::uint64_t>(item.id)));
                }
                if (nextItem.has_value()) {
                    updateLink(ctx, nextItem->id, previousColumn, dini::Value(static_cast<std::uint64_t>(item.id)));
                }
            }

            void detach(dini::TransactionContext &ctx,
                        const dini::ItemSnapshot &item,
                        const std::set<dini::ItemId> &excludedIds) const {
                if (!hasSequenceKey(item)) {
                    return;
                }

                auto previousItem = previous(ctx, item, excludedIds);
                auto nextItem = next(ctx, item, excludedIds);
                if (previousItem.has_value()) {
                    updateLink(ctx, previousItem->id, nextColumn, itemIdValue(nextItem ? std::optional<dini::ItemId>(nextItem->id) : std::nullopt));
                }
                if (nextItem.has_value()) {
                    updateLink(ctx, nextItem->id, previousColumn, itemIdValue(previousItem ? std::optional<dini::ItemId>(previousItem->id) : std::nullopt));
                }
            }

            void rejectDirectInsertedLinks(const dini::ItemSnapshot &item) const {
                if (hasItemValue(item, previousColumn) || hasItemValue(item, nextColumn)) {
                    throw dini::ConstraintError("ordered link columns are derived");
                }
            }

            dini::TableHandle table;
            dini::RelationHandle parent;
            std::optional<dini::ColumnHandle> roleColumn;
            std::vector<dini::ColumnHandle> orderColumns;
            dini::ColumnHandle previousColumn;
            dini::ColumnHandle nextColumn;
        };

        thread_local int overlapCountUpdateDepth = 0;

        struct ScopedOverlapCountUpdate {
            ScopedOverlapCountUpdate() {
                ++overlapCountUpdateDepth;
            }

            ~ScopedOverlapCountUpdate() {
                --overlapCountUpdateDepth;
            }
        };

        bool overlapCountUpdateAllowed() {
            return overlapCountUpdateDepth > 0;
        }

        struct OverlapCountHook {
            explicit OverlapCountHook(dini::TableHandle table,
                                      dini::RelationHandle parent,
                                      dini::ColumnHandle positionColumn,
                                      dini::ColumnHandle lengthColumn,
                                      dini::ColumnHandle overlapCountColumn)
                : table(std::move(table)),
                  parent(std::move(parent)),
                  positionColumn(std::move(positionColumn)),
                  lengthColumn(std::move(lengthColumn)),
                  overlapCountColumn(std::move(overlapCountColumn)) {
            }

            void operator()(dini::TransactionContext &ctx, const dini::ChangeSet &changeSet) const {
                if (ctx.origin() != dini::EventOrigin::Normal) {
                    return;
                }

                std::set<dini::ItemId> removedIds;
                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemRemoved) {
                        removedIds.insert(std::get<dini::ItemRemovedChange>(change.payload()).item.id);
                    } else if (change.kind() == dini::ChangeOperationKind::CascadeRemoved) {
                        removedIds.insert(std::get<dini::CascadeRemovedChange>(change.payload()).item.id);
                    }
                }

                for (const auto &change : changeSet.operations()) {
                    if (change.kind() == dini::ChangeOperationKind::ItemInserted) {
                        const auto &inserted = std::get<dini::ItemInsertedChange>(change.payload());
                        refreshAffectedGroups(ctx,
                                              {itemValue(inserted.item, parent.column())},
                                              {inserted.item},
                                              removedIds);
                    } else if (change.kind() == dini::ChangeOperationKind::ItemRemoved) {
                        const auto &removed = std::get<dini::ItemRemovedChange>(change.payload());
                        refreshAffectedGroups(ctx,
                                              {itemValue(removed.item, parent.column())},
                                              {},
                                              removedIds);
                    } else if (change.kind() == dini::ChangeOperationKind::CascadeRemoved) {
                        const auto &removed = std::get<dini::CascadeRemovedChange>(change.payload());
                        refreshAffectedGroups(ctx,
                                              {itemValue(removed.item, parent.column())},
                                              {},
                                              removedIds);
                    } else if (change.kind() == dini::ChangeOperationKind::ColumnUpdated) {
                        const auto &updated = std::get<dini::ColumnUpdatedChange>(change.payload());
                        if (updated.column == overlapCountColumn) {
                            if (!overlapCountUpdateAllowed()) {
                                throw dini::ConstraintError("overlap count column is derived");
                            }
                            continue;
                        }
                        if (!affectsOverlap(updated.column)) {
                            continue;
                        }

                        auto oldItem = querySingle(ctx, table, updated.itemId, "overlap item does not exist");
                        auto newItem = oldItem;
                        setItemValue(oldItem, updated.column, updated.oldValue);
                        setItemValue(newItem, updated.column, updated.newValue);
                        if (sameOverlapPlacement(oldItem, newItem)) {
                            continue;
                        }

                        refreshAffectedGroups(ctx,
                                              {itemValue(oldItem, parent.column()), itemValue(newItem, parent.column())},
                                              {newItem},
                                              removedIds);
                    }
                }
            }

        private:
            bool affectsOverlap(const dini::ColumnHandle &column) const {
                return column == parent.column() || column == positionColumn || column == lengthColumn;
            }

            bool sameOverlapPlacement(const dini::ItemSnapshot &lhs, const dini::ItemSnapshot &rhs) const {
                return itemValue(lhs, parent.column()) == itemValue(rhs, parent.column()) &&
                       itemValue(lhs, positionColumn) == itemValue(rhs, positionColumn) &&
                       itemValue(lhs, lengthColumn) == itemValue(rhs, lengthColumn);
            }

            std::int64_t start(const dini::ItemSnapshot &item) const {
                return itemValue(item, positionColumn).asInt64();
            }

            std::int64_t end(const dini::ItemSnapshot &item) const {
                const auto itemStart = start(item);
                const auto itemLength = itemValue(item, lengthColumn).asInt64();
                if (itemLength > std::numeric_limits<std::int64_t>::max() - itemStart) {
                    return std::numeric_limits<std::int64_t>::max();
                }
                return itemStart + itemLength;
            }

            bool overlaps(const dini::ItemSnapshot &lhs, const dini::ItemSnapshot &rhs) const {
                return lhs.id != rhs.id && start(lhs) < end(rhs) && start(rhs) < end(lhs);
            }

            bool containsParentValue(const std::vector<dini::Value> &values, const dini::Value &value) const {
                return std::any_of(values.begin(), values.end(), [&](const auto &candidate) {
                    return candidate == value;
                });
            }

            bool isVirtualItem(const std::vector<dini::ItemSnapshot> &virtualItems, dini::ItemId id) const {
                return std::any_of(virtualItems.begin(), virtualItems.end(), [&](const auto &item) {
                    return item.id == id;
                });
            }

            std::vector<dini::ItemSnapshot> queryGroup(dini::TransactionContext &ctx,
                                                       const dini::Value &parentValue,
                                                       const std::vector<dini::ItemSnapshot> &virtualItems,
                                                       const std::set<dini::ItemId> &excludedIds) const {
                std::vector<dini::ItemSnapshot> items;
                if (parentValue.isNull()) {
                    return items;
                }

                auto existingItems = ctx.engine().query(table, {
                    .filter = dini::FilterExpression(dini::Filter(dini::FieldRef::parent(parent),
                                                                  dini::ComparisonOperator::Equal,
                                                                  parentValue)),
                }).toVector();
                for (const auto &item : existingItems) {
                    if (excludedIds.find(item.id) != excludedIds.end() || isVirtualItem(virtualItems, item.id)) {
                        continue;
                    }
                    items.push_back(item);
                }
                for (const auto &item : virtualItems) {
                    if (excludedIds.find(item.id) == excludedIds.end() &&
                        itemValue(item, parent.column()) == parentValue) {
                        items.push_back(item);
                    }
                }
                return items;
            }

            void updateCount(dini::TransactionContext &ctx, dini::ItemId itemId, std::int64_t count) const {
                const auto value = dini::Value(count);
                if (ctx.engine().contains(itemId) && ctx.engine().read(itemId, overlapCountColumn) == value) {
                    return;
                }
                ScopedOverlapCountUpdate allowed;
                ctx.update(itemId, overlapCountColumn, value);
            }

            void refreshGroup(dini::TransactionContext &ctx,
                              const dini::Value &parentValue,
                              const std::vector<dini::ItemSnapshot> &virtualItems,
                              const std::set<dini::ItemId> &excludedIds) const {
                auto items = queryGroup(ctx, parentValue, virtualItems, excludedIds);
                for (const auto &item : items) {
                    std::int64_t count = 0;
                    for (const auto &candidate : items) {
                        if (overlaps(item, candidate)) {
                            ++count;
                        }
                    }
                    updateCount(ctx, item.id, count);
                }
            }

            void refreshAffectedGroups(dini::TransactionContext &ctx,
                                       std::vector<dini::Value> parentValues,
                                       const std::vector<dini::ItemSnapshot> &virtualItems,
                                       const std::set<dini::ItemId> &excludedIds) const {
                for (const auto &item : virtualItems) {
                    const auto value = itemValue(item, parent.column());
                    if (!containsParentValue(parentValues, value)) {
                        parentValues.push_back(value);
                    }
                }

                std::vector<dini::Value> refreshed;
                for (const auto &parentValue : parentValues) {
                    if (parentValue.isNull() || containsParentValue(refreshed, parentValue)) {
                        continue;
                    }
                    refreshGroup(ctx, parentValue, virtualItems, excludedIds);
                    refreshed.push_back(parentValue);
                }

                for (const auto &item : virtualItems) {
                    if (itemValue(item, parent.column()).isNull()) {
                        updateCount(ctx, item.id, 0);
                    }
                }
            }

            dini::TableHandle table;
            dini::RelationHandle parent;
            dini::ColumnHandle positionColumn;
            dini::ColumnHandle lengthColumn;
            dini::ColumnHandle overlapCountColumn;
        };

        struct g {

            g() {
                buildModelTable();
                buildKeySignatureTable();
                buildLabelTable();
                buildTempoTable();
                buildTimeSignatureTable();
                buildTrackList();
                buildClipTable();
                buildSourcesMixableAndDynamicMixingAnchorTablesAndSingerList();
                buildNoteTable();
                buildVibratoPointList();
                buildPhonemeTable();
                buildParameterTable();
                buildFreeValueList();
                buildAnchorNodeTable();
            }

            void buildModelTable() {
                auto modelTableBuilder = schemaBuilder.createTable("Model");
                modelTable = modelTableBuilder.handle();
                modelProjectNameColumn = modelTableBuilder.addColumn({
                    .debugName = "projectName",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                modelProjectAuthorColumn = modelTableBuilder.addColumn({
                    .debugName = "projectAuthor",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                modelGlobalCentShiftColumn = modelTableBuilder.addColumn({
                    .debugName = "globalCentShift",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= -50 && v <= 50; }
                });
                modelMultiChannelOutputColumn = modelTableBuilder.addColumn({
                    .debugName = "multiChannelOutput",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                modelGainColumn = modelTableBuilder.addColumn({
                    .debugName = "gain",
                    .type = dini::ValueType::Double,
                    .defaultValue = 1.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0; }
                });
                modelPanColumn = modelTableBuilder.addColumn({
                    .debugName = "pan",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= -1.0 && v <= 1.0; }
                });
                modelMuteColumn = modelTableBuilder.addColumn({
                    .debugName = "mute",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                modelLoopEnabledColumn = modelTableBuilder.addColumn({
                    .debugName = "loopEnabled",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                modelLoopStartColumn = modelTableBuilder.addColumn({
                    .debugName = "loopStart",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                modelLoopLengthColumn = modelTableBuilder.addColumn({
                    .debugName = "loopLength",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(1),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v > 0; }
                });
            }

            void buildKeySignatureTable() {
                auto keySignatureTableBuilder = schemaBuilder.createTable("KeySignature");
                keySignatureTable = keySignatureTableBuilder.handle();
                keySignatureParent = keySignatureTableBuilder.addAssociation({
                    .debugName = "model",
                    .target = modelTable,
                });
                keySignaturePositionColumn = keySignatureTableBuilder.addColumn({
                    .debugName = "position",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Unique,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                keySignatureModeColumn = keySignatureTableBuilder.addColumn({
                    .debugName = "mode",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 4096; }
                });
                keySignatureTonalityColumn = keySignatureTableBuilder.addColumn({
                    .debugName = "tonality",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 12; }
                });
                keySignatureAccidentalTypeColumn = keySignatureTableBuilder.addColumn({
                    .debugName = "accidentalType",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 2; }
                });
                auto links = addOrderedLinkColumns(keySignatureTableBuilder);
                keySignaturePreviousItemColumn = links.previous;
                keySignatureNextItemColumn = links.next;
                keySignatureTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {keySignatureParent.column(), keySignaturePositionColumn},
                });
                keySignatureTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(keySignatureTable,
                                                keySignatureParent,
                                                {keySignaturePositionColumn},
                                                keySignaturePreviousItemColumn,
                                                keySignatureNextItemColumn)
                });
            }

            void buildLabelTable() {
                auto labelTableBuilder = schemaBuilder.createTable("Label");
                labelTable = labelTableBuilder.handle();
                labelParent = labelTableBuilder.addAssociation({
                    .debugName = "model",
                    .target = modelTable,
                });
                labelPositionColumn = labelTableBuilder.addColumn({
                    .debugName = "position",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                labelTextColumn = labelTableBuilder.addColumn({
                    .debugName = "text",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                auto links = addOrderedLinkColumns(labelTableBuilder);
                labelPreviousItemColumn = links.previous;
                labelNextItemColumn = links.next;
                labelTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {labelParent.column(), labelPositionColumn},
                });
                labelTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(labelTable,
                                                labelParent,
                                                {labelPositionColumn},
                                                labelPreviousItemColumn,
                                                labelNextItemColumn)
                });
            }

            void buildTempoTable() {
                auto tempoTableBuilder = schemaBuilder.createTable("Tempo");
                tempoTable = tempoTableBuilder.handle();
                tempoParent = tempoTableBuilder.addAssociation({
                    .debugName = "model",
                    .target = modelTable,
                });
                tempoPositionColumn = tempoTableBuilder.addColumn({
                    .debugName = "position",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Unique,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                tempoValueColumn = tempoTableBuilder.addColumn({
                    .debugName = "value",
                    .type = dini::ValueType::Double,
                    .defaultValue = 120.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 10.0 && v <= 1000.0; }
                });
                auto links = addOrderedLinkColumns(tempoTableBuilder);
                tempoPreviousItemColumn = links.previous;
                tempoNextItemColumn = links.next;
                tempoTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {tempoParent.column(), tempoPositionColumn},
                });
                tempoTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(tempoTable,
                                                tempoParent,
                                                {tempoPositionColumn},
                                                tempoPreviousItemColumn,
                                                tempoNextItemColumn)
                });
            }

            void buildTimeSignatureTable() {
                auto timeSignatureTableBuilder = schemaBuilder.createTable("TimeSignature");
                timeSignatureTable = timeSignatureTableBuilder.handle();
                timeSignatureParent = timeSignatureTableBuilder.addAssociation({
                    .debugName = "model",
                    .target = modelTable,
                });
                timeSignatureIndexColumn = timeSignatureTableBuilder.addColumn({
                    .debugName = "index",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Unique,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                timeSignatureNumeratorColumn = timeSignatureTableBuilder.addColumn({
                    .debugName = "numerator",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(4),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v > 0; }
                });
                timeSignatureDenominatorColumn = timeSignatureTableBuilder.addColumn({
                    .debugName = "denominator",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(4),
                    .nullable = false,
                    .check = [](const dini::Value &value) {
                        const auto v = value.asInt64();
                        return v == 1 || v == 2 || v == 4 || v == 8 || v == 16 || v == 32 || v == 64 || v == 128;
                    }
                });
                auto links = addOrderedLinkColumns(timeSignatureTableBuilder);
                timeSignaturePreviousItemColumn = links.previous;
                timeSignatureNextItemColumn = links.next;
                timeSignatureTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {timeSignatureParent.column(), timeSignatureIndexColumn},
                });
                timeSignatureTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(timeSignatureTable,
                                                timeSignatureParent,
                                                {timeSignatureIndexColumn},
                                                timeSignaturePreviousItemColumn,
                                                timeSignatureNextItemColumn)
                });
            }

            void buildTrackList() {
                auto trackListBuilder = schemaBuilder.createList("Track");
                trackList = trackListBuilder.handle();
                trackParent = trackListBuilder.setAssociation({
                    .debugName = "model",
                    .target = modelTable,
                });
                trackColorIdColumn = trackListBuilder.addColumn({
                    .debugName = "colorId",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                });
                trackHeightColumn = trackListBuilder.addColumn({
                    .debugName = "height",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                });
                trackNameColumn = trackListBuilder.addColumn({
                    .debugName = "name",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                trackGainColumn = trackListBuilder.addColumn({
                    .debugName = "gain",
                    .type = dini::ValueType::Double,
                    .defaultValue = 1.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0; }
                });
                trackPanColumn = trackListBuilder.addColumn({
                    .debugName = "pan",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= -1.0 && v <= 1.0; }
                });
                trackMuteColumn = trackListBuilder.addColumn({
                    .debugName = "mute",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                trackSoloColumn = trackListBuilder.addColumn({
                    .debugName = "solo",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                trackRecordColumn = trackListBuilder.addColumn({
                    .debugName = "record",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
            }

            void buildClipTable() {
                auto clipTableBuilder = schemaBuilder.createTable("Clip");
                clipTable = clipTableBuilder.handle();
                clipParent = clipTableBuilder.addAssociation({
                    .debugName = "track",
                    .target = trackList,
                });
                audioClipVariant = clipTableBuilder.addVariant("AudioClip");
                singingClipVariant = clipTableBuilder.addVariant("SingingClip");
                clipNameColumn = clipTableBuilder.addColumn({
                    .debugName = "name",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                clipGainColumn = clipTableBuilder.addColumn({
                    .debugName = "gain",
                    .type = dini::ValueType::Double,
                    .defaultValue = 1.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0; }
                });
                clipPanColumn = clipTableBuilder.addColumn({
                    .debugName = "pan",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= -1.0 && v <= 1.0; }
                });
                clipMuteColumn = clipTableBuilder.addColumn({
                    .debugName = "mute",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                clipPositionColumn = clipTableBuilder.addColumn({
                    .debugName = "position",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                clipLengthColumn = clipTableBuilder.addColumn({
                    .debugName = "length",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                clipClipStartColumn = clipTableBuilder.addColumn({
                    .debugName = "clipStart",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                clipClipLengthColumn = clipTableBuilder.addColumn({
                    .debugName = "clipLength",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                auto links = addOrderedLinkColumns(clipTableBuilder);
                clipPreviousItemColumn = links.previous;
                clipNextItemColumn = links.next;
                clipOverlappedCountColumn = clipTableBuilder.addColumn({
                    .debugName = "overlappedCount",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                audioClipPathColumn = clipTableBuilder.addVariantColumn({
                    .debugName = "path",
                    .variant = audioClipVariant,
                    .type = dini::ValueType::Binary,
                    .defaultValue = dini::Value(dini::ByteArray {}),
                    .nullable = false,
                });
                clipTableBuilder.addRangeIndex({
                    .debugName = "range",
                    .columns = {clipPositionColumn, clipClipLengthColumn},
                });
                clipTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {clipParent.column(), clipPositionColumn},
                });
                clipTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(clipTable,
                                                clipParent,
                                                {clipPositionColumn, clipClipLengthColumn},
                                                clipPreviousItemColumn,
                                                clipNextItemColumn)
                });
                clipTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OverlapCountHook(clipTable,
                                                 clipParent,
                                                 clipPositionColumn,
                                                 clipClipLengthColumn,
                                                 clipOverlappedCountColumn)
                });
            }

            void buildSourcesMixableAndDynamicMixingAnchorTablesAndSingerList() {
                auto sourcesTableBuilder = schemaBuilder.createTable("Sources");
                sourcesTable = sourcesTableBuilder.handle();
                sourcesParent = sourcesTableBuilder.addAssociation({
                    .debugName = "singingClip",
                    .target = clipTable,
                });
                sourcesCategoryColumn = sourcesTableBuilder.addColumn({
                    .debugName = "category",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });

                auto mixableTableBuilder = schemaBuilder.createTable("Mixable");
                mixableTable = mixableTableBuilder.handle();
                mixableSourcesColumn = mixableTableBuilder.addColumn({
                    .debugName = "sources",
                    .type = dini::ValueType::UInt64,
                    .index = dini::IndexKind::Unique,
                    .nullable = true,
                });
                mixableMixedSingerColumn = mixableTableBuilder.addColumn({
                    .debugName = "mixedSinger",
                    .type = dini::ValueType::UInt64,
                    .index = dini::IndexKind::Unique,
                    .nullable = true,
                });

                auto singerListBuilder = schemaBuilder.createList("Singer");
                singerList = singerListBuilder.handle();
                singerParent = singerListBuilder.setAssociation({
                    .debugName = "mixable",
                    .target = mixableTable,
                });
                singleSingerVariant = singerListBuilder.addVariant("SingleSinger");
                mixedSingerVariant = singerListBuilder.addVariant("MixedSinger");
                singerExtraColumn = singerListBuilder.addColumn({
                    .debugName = "extra",
                    .type = dini::ValueType::Binary,
                    .defaultValue = dini::Value(dini::ByteArray {}),
                    .nullable = false,
                });
                singleSingerIdColumn = singerListBuilder.addVariantColumn({
                    .debugName = "id",
                    .variant = singleSingerVariant,
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                mixedSingerRatioColumn = singerListBuilder.addVariantColumn({
                    .debugName = "ratio",
                    .variant = mixedSingerVariant,
                    .type = dini::ValueType::Binary,
                    .defaultValue = dini::Value(dini::ByteArray {}),
                    .nullable = false,
                    .check = checkRatioBinary,
                });

                auto dynamicMixingAnchorTableBuilder = schemaBuilder.createTable("DynamicMixingAnchor");
                dynamicMixingAnchorTable = dynamicMixingAnchorTableBuilder.handle();
                dynamicMixingAnchorParent = dynamicMixingAnchorTableBuilder.addAssociation({
                    .debugName = "sources",
                    .target = sourcesTable,
                });
                dynamicMixingAnchorPositionColumn = dynamicMixingAnchorTableBuilder.addColumn({
                    .debugName = "position",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Unique,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                dynamicMixingAnchorRatioColumn = dynamicMixingAnchorTableBuilder.addColumn({
                    .debugName = "ratio",
                    .type = dini::ValueType::Binary,
                    .defaultValue = dini::Value(dini::ByteArray {}),
                    .nullable = false,
                    .check = checkRatioBinary,
                });
                auto dynamicMixingAnchorLinks = addOrderedLinkColumns(dynamicMixingAnchorTableBuilder);
                dynamicMixingAnchorPreviousItemColumn = dynamicMixingAnchorLinks.previous;
                dynamicMixingAnchorNextItemColumn = dynamicMixingAnchorLinks.next;
                dynamicMixingAnchorTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {dynamicMixingAnchorParent.column(), dynamicMixingAnchorPositionColumn},
                });
                dynamicMixingAnchorTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(dynamicMixingAnchorTable,
                                                dynamicMixingAnchorParent,
                                                {dynamicMixingAnchorPositionColumn},
                                                dynamicMixingAnchorPreviousItemColumn,
                                                dynamicMixingAnchorNextItemColumn)
                });
                sourcesTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = SourcesAssociationCheckHook(clipTable, sourcesTable, sourcesParent, singingClipVariant)
                });
                sourcesTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = VirtualCascadeDeleteHook(VirtualCascadeDeleteHook::Mode::Sources,
                                                         mixableTable,
                                                         mixableSourcesColumn)
                });
                mixableTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = MixableAssociationCheckHook(sourcesTable,
                                                            mixableTable,
                                                            singerList,
                                                            mixableSourcesColumn,
                                                            mixableMixedSingerColumn,
                                                            mixedSingerVariant)
                });
                singerListBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = VirtualCascadeDeleteHook(VirtualCascadeDeleteHook::Mode::MixedSinger,
                                                         mixableTable,
                                                         mixableMixedSingerColumn,
                                                         mixedSingerVariant)
                });
            }

            void buildNoteTable() {
                auto noteTableBuilder = schemaBuilder.createTable("Note");
                noteTable = noteTableBuilder.handle();
                noteParent = noteTableBuilder.addAssociation({
                    .debugName = "singingClip",
                    .target = clipTable,
                });
                noteCentShiftColumn = noteTableBuilder.addColumn({
                    .debugName = "centShift",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= -50 && v <= 50; }
                });
                noteKeyNumberColumn = noteTableBuilder.addColumn({
                    .debugName = "keyNumber",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(60),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 128; }
                });
                noteLanguageColumn = noteTableBuilder.addColumn({
                    .debugName = "language",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                noteLengthColumn = noteTableBuilder.addColumn({
                    .debugName = "length",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                noteLyricColumn = noteTableBuilder.addColumn({
                    .debugName = "lyric",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                notePositionColumn = noteTableBuilder.addColumn({
                    .debugName = "position",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                noteOriginalPronunciationColumn = noteTableBuilder.addColumn({
                    .debugName = "originalPronunciation",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                noteEditedPronunciationColumn = noteTableBuilder.addColumn({
                    .debugName = "editedPronunciation",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                noteVibratoAmplitudeColumn = noteTableBuilder.addColumn({
                    .debugName = "vibratoAmplitude",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                noteVibratoEndColumn = noteTableBuilder.addColumn({
                    .debugName = "vibratoEnd",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0 && v <= 1.0; }
                });
                noteVibratoFrequencyColumn = noteTableBuilder.addColumn({
                    .debugName = "vibratoFrequency",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0; }
                });
                noteVibratoOffsetColumn = noteTableBuilder.addColumn({
                    .debugName = "vibratoOffset",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                });
                noteVibratoPhaseColumn = noteTableBuilder.addColumn({
                    .debugName = "vibratoPhase",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0 && v <= 1.0; }
                });
                noteVibratoStartColumn = noteTableBuilder.addColumn({
                    .debugName = "vibratoStart",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asDouble(); return v >= 0.0 && v <= 1.0; }
                });
                auto links = addOrderedLinkColumns(noteTableBuilder);
                notePreviousItemColumn = links.previous;
                noteNextItemColumn = links.next;
                noteOverlappedCountColumn = noteTableBuilder.addColumn({
                    .debugName = "overlappedCount",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                noteTableBuilder.addRangeIndex({
                    .debugName = "range",
                    .columns = {notePositionColumn, noteLengthColumn}
                });
                noteTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {noteParent.column(), notePositionColumn}
                });
                noteTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = ParentVariantCheckHook(clipTable, noteParent, singingClipVariant)
                });
                noteTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(noteTable,
                                                noteParent,
                                                {notePositionColumn, noteLengthColumn, noteKeyNumberColumn},
                                                notePreviousItemColumn,
                                                noteNextItemColumn)
                });
                noteTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OverlapCountHook(noteTable,
                                                 noteParent,
                                                 notePositionColumn,
                                                 noteLengthColumn,
                                                 noteOverlappedCountColumn)
                });
            }

            void buildVibratoPointList() {
                auto vibratoPointListBuilder = schemaBuilder.createList("VibratoPointList");
                vibratoPointList = vibratoPointListBuilder.handle();
                vibratoPointParent = vibratoPointListBuilder.setAssociation({
                    .debugName = "note",
                    .target = noteTable,
                    .nullable = false,
                });
                vibratoPointRoleColumn = vibratoPointListBuilder.addColumn({
                    .debugName = "role",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .nullable = true,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 2; }
                });
                vibratoPointXColumn = vibratoPointListBuilder.addColumn({
                    .debugName = "x",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                });
                vibratoPointYColumn = vibratoPointListBuilder.addColumn({
                    .debugName = "y",
                    .type = dini::ValueType::Double,
                    .defaultValue = 0.0,
                    .nullable = false,
                });
                vibratoPointListBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = RoleAssociationCheckHook(vibratoPointList, vibratoPointParent, vibratoPointRoleColumn)
                });
            }

            void buildPhonemeTable() {
                auto phonemeTableBuilder = schemaBuilder.createTable("Phoneme");
                phonemeTable = phonemeTableBuilder.handle();
                phonemeParent = phonemeTableBuilder.addAssociation({
                    .debugName = "note",
                    .target = noteTable,
                });
                phonemeRoleColumn = phonemeTableBuilder.addColumn({
                    .debugName = "role",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .nullable = true,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 2; }
                });
                phonemeLanguageColumn = phonemeTableBuilder.addColumn({
                    .debugName = "language",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                phonemeStartColumn = phonemeTableBuilder.addColumn({
                    .debugName = "start",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                });
                phonemeTokenColumn = phonemeTableBuilder.addColumn({
                    .debugName = "token",
                    .type = dini::ValueType::String,
                    .defaultValue = "",
                    .nullable = false,
                });
                phonemeOnsetColumn = phonemeTableBuilder.addColumn({
                    .debugName = "onset",
                    .type = dini::ValueType::Bool,
                    .defaultValue = false,
                    .nullable = false,
                });
                auto links = addOrderedLinkColumns(phonemeTableBuilder);
                phonemePreviousItemColumn = links.previous;
                phonemeNextItemColumn = links.next;
                phonemeTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {phonemeParent.column(), phonemeRoleColumn, phonemeStartColumn},
                });
                phonemeTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = RoleAssociationCheckHook(phonemeTable, phonemeParent, phonemeRoleColumn)
                });
                phonemeTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(phonemeTable,
                                                phonemeParent,
                                                {phonemeStartColumn},
                                                phonemePreviousItemColumn,
                                                phonemeNextItemColumn,
                                                phonemeRoleColumn)
                });
            }

            void buildParameterTable() {
                auto parameterTableBuilder = schemaBuilder.createTable("Parameter");
                parameterTable = parameterTableBuilder.handle();
                parameterParent = parameterTableBuilder.addAssociation({
                    .debugName = "singingClip",
                    .target = clipTable,
                });
                parameterKeyColumn = parameterTableBuilder.addColumn({
                    .debugName = "key",
                    .type = dini::ValueType::String,
                    .index = dini::IndexKind::Unique,
                    .nullable = true,
                });
                parameterTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = ParentVariantCheckHook(clipTable, parameterParent, singingClipVariant)
                });
            }

            void buildFreeValueList() {
                auto freeValueListBuilder = schemaBuilder.createList("FreeValueList");
                freeValueList = freeValueListBuilder.handle();
                freeValueParent = freeValueListBuilder.setAssociation({
                    .debugName = "parameter",
                    .target = parameterTable,
                    .nullable = false,
                });
                freeValueRoleColumn = freeValueListBuilder.addColumn({
                    .debugName = "role",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .nullable = true,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 3; }
                });
                freeValueValueColumn = freeValueListBuilder.addColumn({
                    .debugName = "value",
                    .type = dini::ValueType::Int64,
                    .nullable = true,
                });
                freeValueListBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = RoleAssociationCheckHook(freeValueList, freeValueParent, freeValueRoleColumn)
                });
            }

            void buildAnchorNodeTable() {
                auto anchorNodeTableBuilder = schemaBuilder.createTable("AnchorNode");
                anchorNodeTable = anchorNodeTableBuilder.handle();
                anchorNodeParent = anchorNodeTableBuilder.addAssociation({
                    .debugName = "parameter",
                    .target = parameterTable,
                });
                anchorNodeRoleColumn = anchorNodeTableBuilder.addColumn({
                    .debugName = "role",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Normal,
                    .nullable = true,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 2; }
                });
                anchorNodeInterpolationModeColumn = anchorNodeTableBuilder.addColumn({
                    .debugName = "interpolationMode",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0 && v < 3; }
                });
                anchorNodeXColumn = anchorNodeTableBuilder.addColumn({
                    .debugName = "x",
                    .type = dini::ValueType::Int64,
                    .index = dini::IndexKind::Unique,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                    .check = [](const dini::Value &value) { const auto v = value.asInt64(); return v >= 0; }
                });
                anchorNodeYColumn = anchorNodeTableBuilder.addColumn({
                    .debugName = "y",
                    .type = dini::ValueType::Int64,
                    .defaultValue = INT64_C(0),
                    .nullable = false,
                });
                auto links = addOrderedLinkColumns(anchorNodeTableBuilder);
                anchorNodePreviousItemColumn = links.previous;
                anchorNodeNextItemColumn = links.next;
                anchorNodeTableBuilder.addRangeIndex({
                    .debugName = "order",
                    .columns = {anchorNodeParent.column(), anchorNodeRoleColumn, anchorNodeXColumn},
                });
                anchorNodeTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = RoleAssociationCheckHook(anchorNodeTable, anchorNodeParent, anchorNodeRoleColumn)
                });
                anchorNodeTableBuilder.addHook({
                    .stage = dini::HookStage::BeforeApply,
                    .callback = OrderedLinkHook(anchorNodeTable,
                                                anchorNodeParent,
                                                {anchorNodeXColumn},
                                                anchorNodePreviousItemColumn,
                                                anchorNodeNextItemColumn,
                                                anchorNodeRoleColumn)
                });
            }

            dini::SchemaBuilder schemaBuilder;

            dini::TableHandle clipTable;
            dini::TableHandle dynamicMixingAnchorTable;
            dini::TableHandle keySignatureTable;
            dini::TableHandle labelTable;
            dini::TableHandle mixableTable;
            dini::TableHandle modelTable;
            dini::TableHandle noteTable;
            dini::TableHandle phonemeTable;
            dini::TableHandle parameterTable;
            dini::TableHandle sourcesTable;
            dini::TableHandle tempoTable;
            dini::TableHandle timeSignatureTable;
            dini::TableHandle anchorNodeTable;

            dini::ListHandle singerList;
            dini::ListHandle trackList;
            dini::ListHandle freeValueList;
            dini::ListHandle vibratoPointList;

            dini::RelationHandle vibratoPointParent;
            dini::RelationHandle clipParent;
            dini::RelationHandle dynamicMixingAnchorParent;
            dini::RelationHandle anchorNodeParent;
            dini::RelationHandle freeValueParent;
            dini::RelationHandle keySignatureParent;
            dini::RelationHandle labelParent;
            dini::RelationHandle noteParent;
            dini::RelationHandle phonemeParent;
            dini::RelationHandle parameterParent;
            dini::RelationHandle singerParent;
            dini::RelationHandle sourcesParent;
            dini::RelationHandle tempoParent;
            dini::RelationHandle timeSignatureParent;
            dini::RelationHandle trackParent;

            dini::VariantHandle audioClipVariant;
            dini::VariantHandle singingClipVariant;
            dini::VariantHandle singleSingerVariant;
            dini::VariantHandle mixedSingerVariant;

            dini::ColumnHandle vibratoPointRoleColumn;
            dini::ColumnHandle vibratoPointXColumn;
            dini::ColumnHandle vibratoPointYColumn;

            dini::ColumnHandle clipNameColumn;
            dini::ColumnHandle clipGainColumn;
            dini::ColumnHandle clipPanColumn;
            dini::ColumnHandle clipMuteColumn;
            dini::ColumnHandle clipPositionColumn;
            dini::ColumnHandle clipLengthColumn;
            dini::ColumnHandle clipClipStartColumn;
            dini::ColumnHandle clipClipLengthColumn;
            dini::ColumnHandle clipPreviousItemColumn;
            dini::ColumnHandle clipNextItemColumn;
            dini::ColumnHandle clipOverlappedCountColumn;
            dini::ColumnHandle audioClipPathColumn;

            dini::ColumnHandle labelPositionColumn;
            dini::ColumnHandle labelTextColumn;
            dini::ColumnHandle labelPreviousItemColumn;
            dini::ColumnHandle labelNextItemColumn;

            dini::ColumnHandle keySignaturePositionColumn;
            dini::ColumnHandle keySignatureModeColumn;
            dini::ColumnHandle keySignatureTonalityColumn;
            dini::ColumnHandle keySignatureAccidentalTypeColumn;
            dini::ColumnHandle keySignaturePreviousItemColumn;
            dini::ColumnHandle keySignatureNextItemColumn;

            dini::ColumnHandle modelProjectNameColumn;
            dini::ColumnHandle modelProjectAuthorColumn;
            dini::ColumnHandle modelGlobalCentShiftColumn;
            dini::ColumnHandle modelMultiChannelOutputColumn;
            dini::ColumnHandle modelGainColumn;
            dini::ColumnHandle modelPanColumn;
            dini::ColumnHandle modelMuteColumn;
            dini::ColumnHandle modelLoopEnabledColumn;
            dini::ColumnHandle modelLoopStartColumn;
            dini::ColumnHandle modelLoopLengthColumn;

            dini::ColumnHandle mixableSourcesColumn;
            dini::ColumnHandle mixableMixedSingerColumn;

            dini::ColumnHandle noteCentShiftColumn;
            dini::ColumnHandle noteKeyNumberColumn;
            dini::ColumnHandle noteLanguageColumn;
            dini::ColumnHandle noteLengthColumn;
            dini::ColumnHandle noteLyricColumn;
            dini::ColumnHandle notePositionColumn;
            dini::ColumnHandle noteOriginalPronunciationColumn;
            dini::ColumnHandle noteEditedPronunciationColumn;
            dini::ColumnHandle noteVibratoAmplitudeColumn;
            dini::ColumnHandle noteVibratoEndColumn;
            dini::ColumnHandle noteVibratoFrequencyColumn;
            dini::ColumnHandle noteVibratoOffsetColumn;
            dini::ColumnHandle noteVibratoPhaseColumn;
            dini::ColumnHandle noteVibratoStartColumn;
            dini::ColumnHandle notePreviousItemColumn;
            dini::ColumnHandle noteNextItemColumn;
            dini::ColumnHandle noteOverlappedCountColumn;

            dini::ColumnHandle phonemeRoleColumn;
            dini::ColumnHandle phonemeLanguageColumn;
            dini::ColumnHandle phonemeStartColumn;
            dini::ColumnHandle phonemeTokenColumn;
            dini::ColumnHandle phonemeOnsetColumn;
            dini::ColumnHandle phonemePreviousItemColumn;
            dini::ColumnHandle phonemeNextItemColumn;

            dini::ColumnHandle dynamicMixingAnchorPositionColumn;
            dini::ColumnHandle dynamicMixingAnchorRatioColumn;
            dini::ColumnHandle dynamicMixingAnchorPreviousItemColumn;
            dini::ColumnHandle dynamicMixingAnchorNextItemColumn;

            dini::ColumnHandle freeValueRoleColumn;
            dini::ColumnHandle freeValueValueColumn;

            dini::ColumnHandle parameterKeyColumn;

            dini::ColumnHandle singerExtraColumn;
            dini::ColumnHandle singleSingerIdColumn;
            dini::ColumnHandle mixedSingerRatioColumn;

            dini::ColumnHandle sourcesCategoryColumn;

            dini::ColumnHandle tempoPositionColumn;
            dini::ColumnHandle tempoValueColumn;
            dini::ColumnHandle tempoPreviousItemColumn;
            dini::ColumnHandle tempoNextItemColumn;

            dini::ColumnHandle timeSignatureIndexColumn;
            dini::ColumnHandle timeSignatureNumeratorColumn;
            dini::ColumnHandle timeSignatureDenominatorColumn;
            dini::ColumnHandle timeSignaturePreviousItemColumn;
            dini::ColumnHandle timeSignatureNextItemColumn;

            dini::ColumnHandle trackColorIdColumn;
            dini::ColumnHandle trackHeightColumn;
            dini::ColumnHandle trackNameColumn;
            dini::ColumnHandle trackGainColumn;
            dini::ColumnHandle trackPanColumn;
            dini::ColumnHandle trackMuteColumn;
            dini::ColumnHandle trackSoloColumn;
            dini::ColumnHandle trackRecordColumn;

            dini::ColumnHandle anchorNodeRoleColumn;
            dini::ColumnHandle anchorNodeInterpolationModeColumn;
            dini::ColumnHandle anchorNodeXColumn;
            dini::ColumnHandle anchorNodeYColumn;
            dini::ColumnHandle anchorNodePreviousItemColumn;
            dini::ColumnHandle anchorNodeNextItemColumn;
        } g;

    }



    dini::SchemaBuilder *Schema::schemaBuilder() {
        return &g.schemaBuilder;
    }

    dini::TableHandle Schema::clipTable() {
        return g.clipTable;
    }

    dini::TableHandle Schema::dynamicMixingAnchorTable() {
        return g.dynamicMixingAnchorTable;
    }

    dini::TableHandle Schema::keySignatureTable() {
        return g.keySignatureTable;
    }

    dini::TableHandle Schema::labelTable() {
        return g.labelTable;
    }

    dini::TableHandle Schema::mixableTable() {
        return g.mixableTable;
    }

    dini::TableHandle Schema::modelTable() {
        return g.modelTable;
    }

    dini::TableHandle Schema::noteTable() {
        return g.noteTable;
    }

    dini::TableHandle Schema::phonemeTable() {
        return g.phonemeTable;
    }

    dini::TableHandle Schema::parameterTable() {
        return g.parameterTable;
    }

    dini::TableHandle Schema::sourcesTable() {
        return g.sourcesTable;
    }

    dini::TableHandle Schema::tempoTable() {
        return g.tempoTable;
    }

    dini::TableHandle Schema::timeSignatureTable() {
        return g.timeSignatureTable;
    }

    dini::TableHandle Schema::anchorNodeTable() {
        return g.anchorNodeTable;
    }

    dini::ListHandle Schema::singerList() {
        return g.singerList;
    }

    dini::ListHandle Schema::trackList() {
        return g.trackList;
    }

    dini::ListHandle Schema::freeValueList() {
        return g.freeValueList;
    }

    dini::ListHandle Schema::vibratoPointList() {
        return g.vibratoPointList;
    }

    dini::RelationHandle Schema::vibratoPointParent() {
        return g.vibratoPointParent;
    }

    dini::RelationHandle Schema::anchorNodeParent() {
        return g.anchorNodeParent;
    }

    dini::RelationHandle Schema::clipParent() {
        return g.clipParent;
    }

    dini::RelationHandle Schema::dynamicMixingAnchorParent() {
        return g.dynamicMixingAnchorParent;
    }

    dini::RelationHandle Schema::freeValueParent() {
        return g.freeValueParent;
    }

    dini::RelationHandle Schema::keySignatureParent() {
        return g.keySignatureParent;
    }

    dini::RelationHandle Schema::labelParent() {
        return g.labelParent;
    }

    dini::RelationHandle Schema::noteParent() {
        return g.noteParent;
    }

    dini::RelationHandle Schema::phonemeParent() {
        return g.phonemeParent;
    }

    dini::RelationHandle Schema::parameterParent() {
        return g.parameterParent;
    }

    dini::RelationHandle Schema::singerParent() {
        return g.singerParent;
    }

    dini::RelationHandle Schema::sourcesParent() {
        return g.sourcesParent;
    }

    dini::RelationHandle Schema::tempoParent() {
        return g.tempoParent;
    }

    dini::RelationHandle Schema::timeSignatureParent() {
        return g.timeSignatureParent;
    }

    dini::RelationHandle Schema::trackParent() {
        return g.trackParent;
    }

    dini::VariantHandle Schema::audioClipVariant() {
        return g.audioClipVariant;
    }

    dini::VariantHandle Schema::singingClipVariant() {
        return g.singingClipVariant;
    }

    dini::VariantHandle Schema::singleSingerVariant() {
        return g.singleSingerVariant;
    }

    dini::VariantHandle Schema::mixedSingerVariant() {
        return g.mixedSingerVariant;
    }

    dini::ColumnHandle Schema::vibratoPointRoleColumn() {
        return g.vibratoPointRoleColumn;
    }

    dini::ColumnHandle Schema::vibratoPointXColumn() {
        return g.vibratoPointXColumn;
    }

    dini::ColumnHandle Schema::vibratoPointYColumn() {
        return g.vibratoPointYColumn;
    }

    dini::ColumnHandle Schema::clipNameColumn() {
        return g.clipNameColumn;
    }

    dini::ColumnHandle Schema::clipGainColumn() {
        return g.clipGainColumn;
    }

    dini::ColumnHandle Schema::clipPanColumn() {
        return g.clipPanColumn;
    }

    dini::ColumnHandle Schema::clipMuteColumn() {
        return g.clipMuteColumn;
    }

    dini::ColumnHandle Schema::clipPositionColumn() {
        return g.clipPositionColumn;
    }

    dini::ColumnHandle Schema::clipLengthColumn() {
        return g.clipLengthColumn;
    }

    dini::ColumnHandle Schema::clipClipStartColumn() {
        return g.clipClipStartColumn;
    }

    dini::ColumnHandle Schema::clipClipLengthColumn() {
        return g.clipClipLengthColumn;
    }

    dini::ColumnHandle Schema::clipPreviousItemColumn() {
        return g.clipPreviousItemColumn;
    }

    dini::ColumnHandle Schema::clipNextItemColumn() {
        return g.clipNextItemColumn;
    }

    dini::ColumnHandle Schema::clipOverlappedCountColumn() {
        return g.clipOverlappedCountColumn;
    }

    dini::ColumnHandle Schema::audioClipPathColumn() {
        return g.audioClipPathColumn;
    }

    dini::ColumnHandle Schema::dynamicMixingAnchorPositionColumn() {
        return g.dynamicMixingAnchorPositionColumn;
    }

    dini::ColumnHandle Schema::dynamicMixingAnchorRatioColumn() {
        return g.dynamicMixingAnchorRatioColumn;
    }

    dini::ColumnHandle Schema::dynamicMixingAnchorPreviousItemColumn() {
        return g.dynamicMixingAnchorPreviousItemColumn;
    }

    dini::ColumnHandle Schema::dynamicMixingAnchorNextItemColumn() {
        return g.dynamicMixingAnchorNextItemColumn;
    }

    dini::ColumnHandle Schema::freeValueRoleColumn() {
        return g.freeValueRoleColumn;
    }

    dini::ColumnHandle Schema::freeValueValueColumn() {
        return g.freeValueValueColumn;
    }

    dini::ColumnHandle Schema::labelPositionColumn() {
        return g.labelPositionColumn;
    }

    dini::ColumnHandle Schema::labelTextColumn() {
        return g.labelTextColumn;
    }

    dini::ColumnHandle Schema::labelPreviousItemColumn() {
        return g.labelPreviousItemColumn;
    }

    dini::ColumnHandle Schema::labelNextItemColumn() {
        return g.labelNextItemColumn;
    }

    dini::ColumnHandle Schema::keySignaturePositionColumn() {
        return g.keySignaturePositionColumn;
    }

    dini::ColumnHandle Schema::keySignatureModeColumn() {
        return g.keySignatureModeColumn;
    }

    dini::ColumnHandle Schema::keySignatureTonalityColumn() {
        return g.keySignatureTonalityColumn;
    }

    dini::ColumnHandle Schema::keySignatureAccidentalTypeColumn() {
        return g.keySignatureAccidentalTypeColumn;
    }

    dini::ColumnHandle Schema::keySignaturePreviousItemColumn() {
        return g.keySignaturePreviousItemColumn;
    }

    dini::ColumnHandle Schema::keySignatureNextItemColumn() {
        return g.keySignatureNextItemColumn;
    }

    dini::ColumnHandle Schema::modelProjectNameColumn() {
        return g.modelProjectNameColumn;
    }

    dini::ColumnHandle Schema::modelProjectAuthorColumn() {
        return g.modelProjectAuthorColumn;
    }

    dini::ColumnHandle Schema::modelGlobalCentShiftColumn() {
        return g.modelGlobalCentShiftColumn;
    }

    dini::ColumnHandle Schema::modelMultiChannelOutputColumn() {
        return g.modelMultiChannelOutputColumn;
    }

    dini::ColumnHandle Schema::modelGainColumn() {
        return g.modelGainColumn;
    }

    dini::ColumnHandle Schema::modelPanColumn() {
        return g.modelPanColumn;
    }

    dini::ColumnHandle Schema::modelMuteColumn() {
        return g.modelMuteColumn;
    }

    dini::ColumnHandle Schema::modelLoopEnabledColumn() {
        return g.modelLoopEnabledColumn;
    }

    dini::ColumnHandle Schema::modelLoopStartColumn() {
        return g.modelLoopStartColumn;
    }

    dini::ColumnHandle Schema::modelLoopLengthColumn() {
        return g.modelLoopLengthColumn;
    }

    dini::ColumnHandle Schema::mixableSourcesColumn() {
        return g.mixableSourcesColumn;
    }

    dini::ColumnHandle Schema::mixableMixedSingerColumn() {
        return g.mixableMixedSingerColumn;
    }

    dini::ColumnHandle Schema::noteCentShiftColumn() {
        return g.noteCentShiftColumn;
    }

    dini::ColumnHandle Schema::noteKeyNumberColumn() {
        return g.noteKeyNumberColumn;
    }

    dini::ColumnHandle Schema::noteLanguageColumn() {
        return g.noteLanguageColumn;
    }

    dini::ColumnHandle Schema::noteLengthColumn() {
        return g.noteLengthColumn;
    }

    dini::ColumnHandle Schema::noteLyricColumn() {
        return g.noteLyricColumn;
    }

    dini::ColumnHandle Schema::notePositionColumn() {
        return g.notePositionColumn;
    }

    dini::ColumnHandle Schema::noteOriginalPronunciationColumn() {
        return g.noteOriginalPronunciationColumn;
    }

    dini::ColumnHandle Schema::noteEditedPronunciationColumn() {
        return g.noteEditedPronunciationColumn;
    }

    dini::ColumnHandle Schema::noteVibratoAmplitudeColumn() {
        return g.noteVibratoAmplitudeColumn;
    }

    dini::ColumnHandle Schema::noteVibratoEndColumn() {
        return g.noteVibratoEndColumn;
    }

    dini::ColumnHandle Schema::noteVibratoFrequencyColumn() {
        return g.noteVibratoFrequencyColumn;
    }

    dini::ColumnHandle Schema::noteVibratoOffsetColumn() {
        return g.noteVibratoOffsetColumn;
    }

    dini::ColumnHandle Schema::noteVibratoPhaseColumn() {
        return g.noteVibratoPhaseColumn;
    }

    dini::ColumnHandle Schema::noteVibratoStartColumn() {
        return g.noteVibratoStartColumn;
    }

    dini::ColumnHandle Schema::notePreviousItemColumn() {
        return g.notePreviousItemColumn;
    }

    dini::ColumnHandle Schema::noteNextItemColumn() {
        return g.noteNextItemColumn;
    }

    dini::ColumnHandle Schema::noteOverlappedCountColumn() {
        return g.noteOverlappedCountColumn;
    }

    dini::ColumnHandle Schema::phonemeRoleColumn() {
        return g.phonemeRoleColumn;
    }

    dini::ColumnHandle Schema::phonemeLanguageColumn() {
        return g.phonemeLanguageColumn;
    }

    dini::ColumnHandle Schema::phonemeStartColumn() {
        return g.phonemeStartColumn;
    }

    dini::ColumnHandle Schema::phonemeTokenColumn() {
        return g.phonemeTokenColumn;
    }

    dini::ColumnHandle Schema::phonemeOnsetColumn() {
        return g.phonemeOnsetColumn;
    }

    dini::ColumnHandle Schema::phonemePreviousItemColumn() {
        return g.phonemePreviousItemColumn;
    }

    dini::ColumnHandle Schema::phonemeNextItemColumn() {
        return g.phonemeNextItemColumn;
    }

    dini::ColumnHandle Schema::parameterKeyColumn() {
        return g.parameterKeyColumn;
    }

    dini::ColumnHandle Schema::singerExtraColumn() {
        return g.singerExtraColumn;
    }

    dini::ColumnHandle Schema::singleSingerIdColumn() {
        return g.singleSingerIdColumn;
    }

    dini::ColumnHandle Schema::mixedSingerRatioColumn() {
        return g.mixedSingerRatioColumn;
    }

    dini::ColumnHandle Schema::sourcesCategoryColumn() {
        return g.sourcesCategoryColumn;
    }

    dini::ColumnHandle Schema::tempoPositionColumn() {
        return g.tempoPositionColumn;
    }

    dini::ColumnHandle Schema::tempoValueColumn() {
        return g.tempoValueColumn;
    }

    dini::ColumnHandle Schema::tempoPreviousItemColumn() {
        return g.tempoPreviousItemColumn;
    }

    dini::ColumnHandle Schema::tempoNextItemColumn() {
        return g.tempoNextItemColumn;
    }

    dini::ColumnHandle Schema::timeSignatureIndexColumn() {
        return g.timeSignatureIndexColumn;
    }

    dini::ColumnHandle Schema::timeSignatureNumeratorColumn() {
        return g.timeSignatureNumeratorColumn;
    }

    dini::ColumnHandle Schema::timeSignatureDenominatorColumn() {
        return g.timeSignatureDenominatorColumn;
    }

    dini::ColumnHandle Schema::timeSignaturePreviousItemColumn() {
        return g.timeSignaturePreviousItemColumn;
    }

    dini::ColumnHandle Schema::timeSignatureNextItemColumn() {
        return g.timeSignatureNextItemColumn;
    }

    dini::ColumnHandle Schema::trackColorIdColumn() {
        return g.trackColorIdColumn;
    }

    dini::ColumnHandle Schema::trackHeightColumn() {
        return g.trackHeightColumn;
    }

    dini::ColumnHandle Schema::trackNameColumn() {
        return g.trackNameColumn;
    }

    dini::ColumnHandle Schema::trackGainColumn() {
        return g.trackGainColumn;
    }

    dini::ColumnHandle Schema::trackPanColumn() {
        return g.trackPanColumn;
    }

    dini::ColumnHandle Schema::trackMuteColumn() {
        return g.trackMuteColumn;
    }

    dini::ColumnHandle Schema::trackSoloColumn() {
        return g.trackSoloColumn;
    }

    dini::ColumnHandle Schema::trackRecordColumn() {
        return g.trackRecordColumn;
    }

    dini::ColumnHandle Schema::anchorNodeRoleColumn() {
        return g.anchorNodeRoleColumn;
    }

    dini::ColumnHandle Schema::anchorNodeInterpolationModeColumn() {
        return g.anchorNodeInterpolationModeColumn;
    }

    dini::ColumnHandle Schema::anchorNodeXColumn() {
        return g.anchorNodeXColumn;
    }

    dini::ColumnHandle Schema::anchorNodeYColumn() {
        return g.anchorNodeYColumn;
    }

    dini::ColumnHandle Schema::anchorNodePreviousItemColumn() {
        return g.anchorNodePreviousItemColumn;
    }

    dini::ColumnHandle Schema::anchorNodeNextItemColumn() {
        return g.anchorNodeNextItemColumn;
    }

}
