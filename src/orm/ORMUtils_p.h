#ifndef DSPXMODEL_ORMUTILS_P_H
#define DSPXMODEL_ORMUTILS_P_H

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <QString>

#include <dini/change.h>
#include <dini/query.h>
#include <dini/value.h>
#include <dini/view.h>

#include <dspxmodelORM/Handle.h>

#define DSPXMODEL_DECLARE_GET(Class) \
    static Class##Private *get(Class *q) { return q->d_func(); } \
    static const Class##Private *get(const Class *q) { return q->d_func(); }

#define DSPXMODEL_FORWARD_CONSTRUCTOR(Class) \
    template <typename ...Args> static Class *create(Args &&...args) { return new Class(std::forward<Args>(args)...); }

namespace dspx::orm {

    inline Handle handleFromId(dini::ItemId id) {
        return Handle {static_cast<quint64>(id)};
    }

    inline dini::ItemId idFromHandle(Handle handle) {
        return static_cast<dini::ItemId>(handle.d);
    }

    inline Handle handleFromValue(const dini::Value &value) {
        if (value.isNull()) {
            return {};
        }
        if (value.type() == dini::ValueType::UInt64) {
            return Handle {static_cast<quint64>(value.asUInt64())};
        }
        if (value.type() == dini::ValueType::Int64 && value.asInt64() >= 0) {
            return Handle {static_cast<quint64>(value.asInt64())};
        }
        return {};
    }

    inline dini::Value valueFromHandle(Handle handle) {
        return handle ? dini::Value(static_cast<std::uint64_t>(handle.d)) : dini::Value::null();
    }

    inline dini::Value valueFromString(const QString &value) {
        const auto utf8 = value.toUtf8();
        return dini::Value(std::string(utf8.constData(), static_cast<std::size_t>(utf8.size())));
    }

    inline QString stringFromValue(const dini::Value &value) {
        return QString::fromStdString(value.asString());
    }

    inline const dini::Value &snapshotValue(const dini::ItemSnapshot &snapshot, const dini::ColumnHandle &column) {
        static const dini::Value nullValue = dini::Value::null();
        for (const auto &columnValue : snapshot.values) {
            if (columnValue.column == column) {
                return columnValue.value;
            }
        }
        return nullValue;
    }

    inline bool isContainer(const dini::ItemSnapshot &snapshot, const dini::TableHandle &table) {
        return snapshot.containerKind == dini::ContainerKind::Table &&
               snapshot.containerId == table.containerId();
    }

    inline bool isContainer(const dini::ItemSnapshot &snapshot, const dini::ListHandle &list) {
        return snapshot.containerKind == dini::ContainerKind::List &&
               snapshot.containerId == list.containerId();
    }

    inline dini::FilterExpression equalFilter(dini::FieldRef field, dini::Value value) {
        return dini::FilterExpression(dini::Filter(std::move(field),
                                                   dini::ComparisonOperator::Equal,
                                                   std::move(value)));
    }

    inline dini::FilterExpression parentFilter(const dini::RelationHandle &relation, Handle parent) {
        return equalFilter(dini::FieldRef::parent(relation), valueFromHandle(parent));
    }

    inline std::optional<dini::ItemSnapshot> firstSnapshot(const dini::View &view) {
        auto values = view.limit(1).toVector();
        if (values.empty()) {
            return {};
        }
        return values.front();
    }

    template <typename... T>
    struct Overloaded : T... {
        using T::operator()...;
    };

    template <typename... T>
    Overloaded(T...) -> Overloaded<T...>;

}

#endif // DSPXMODEL_ORMUTILS_P_H

