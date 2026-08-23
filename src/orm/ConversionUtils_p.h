#ifndef DSPXMODEL_CONVERSIONUTILS_P_H
#define DSPXMODEL_CONVERSIONUTILS_P_H

#include <cmath>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <stdcorelib/support/json.h>

#include <opendspx/workspace.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>

#include <dini/types.h>
#include <dini/value.h>

namespace dspx::conv {

    namespace detail {

        template <typename T>
        struct always_false : std::false_type {};

        template <typename T>
        inline constexpr bool always_false_v = always_false<T>::value;

        template <typename T>
        struct is_string_like {
        private:
            using D = std::decay_t<T>;

        public:
            static constexpr bool value =
                std::is_same_v<D, std::string> ||
                std::is_same_v<D, std::string_view> ||
                std::is_convertible_v<T, const char *>;
        };

        template <typename T>
        inline constexpr bool is_string_like_v = is_string_like<T>::value;

        template <typename T>
        bool optionalChainStep(const stdc::JsonValue *&cur, T &&arg) {
            using D = std::decay_t<T>;

            if constexpr (is_string_like_v<T>) {
                if (!cur || !cur->isObject()) {
                    return false;
                }

                if constexpr (std::is_convertible_v<T, const char *>
                           && !std::is_same_v<D, std::string>
                           && !std::is_same_v<D, std::string_view>) {
                    const char *key = arg;
                    if (!key) {
                        return false;
                    }

                    const auto &object = cur->toObject();
                    auto it = object.find(key);
                    if (it == object.end()) {
                        return false;
                    }

                    cur = &(it->second);
                    return true;
                } else {
                    std::string key(arg);

                    const auto &object = cur->toObject();
                    auto it = object.find(key);
                    if (it == object.end()) {
                        return false;
                    }

                    cur = &(it->second);
                    return true;
                }
            } else if constexpr (std::is_integral_v<D> && !std::is_same_v<D, bool>) {
                if (!cur || !cur->isArray()) {
                    return false;
                }

                if constexpr (std::is_signed_v<D>) {
                    if (arg < 0) {
                        return false;
                    }
                }

                const auto index = static_cast<std::size_t>(arg);
                const auto &array = cur->toArray();
                if (index >= array.size()) {
                    return false;
                }

                cur = &(array[index]);
                return true;
            } else {
                static_assert(
                    always_false_v<T>,
                    "optionalChain only supports string-like object keys and integer array indexes"
                );
                return false;
            }
        }

    }

    inline double toDecibel(double value) {
        return 20.0 * std::log10(value);
    }

    inline double fromDecibel(double value) {
        return std::pow(10.0, value / 20.0);
    }

    inline dini::Value valueFromRatio(const QList<double> &ratio) {
        dini::ByteArray bytes;
        bytes.resize(static_cast<std::size_t>(ratio.size()) * sizeof(double));
        auto *dst = bytes.data();
        for (const auto item : ratio) {
            std::memcpy(dst, &item, sizeof(double));
            dst += sizeof(double);
        }
        return dini::Value(std::move(bytes));
    }

    inline QList<double> ratioFromValue(const dini::Value &value) {
        QList<double> result;
        if (value.isNull()) {
            return result;
        }
        const auto &bytes = value.asBinary();
        result.reserve(static_cast<int>(bytes.size() / sizeof(double)));
        for (std::size_t i = 0; i + sizeof(double) <= bytes.size(); i += sizeof(double)) {
            double item = 0.0;
            std::memcpy(&item, bytes.data() + i, sizeof(double));
            result.append(item);
        }
        return result;
    }

    template <typename ...Args>
    stdc::JsonValue optionalChain(const stdc::JsonValue &json, Args &&...args) {
        const stdc::JsonValue *cur = &json;
        bool ok = (detail::optionalChainStep(cur, std::forward<Args>(args)) && ...);
        if (!ok || !cur) {
            return stdc::JsonValue();
        }
        return *cur;
    }

    inline dini::ByteArray serializeWorkspace(const stdc::JsonObject &workspace) {
        return stdc::JsonValue(workspace).toCbor();
    }

    inline stdc::JsonObject workspaceToJson(const opendspx::Workspace &workspace) {
        stdc::JsonObject result;
        for (const auto &[key, value] : workspace) {
            result[key] = value;
        }
        return result;
    }

    inline opendspx::Workspace workspaceFromJson(const stdc::JsonObject &workspace) {
        opendspx::Workspace result;
        for (const auto &[key, value] : workspace) {
            result[key] = value.toObject();
        }
        return result;
    }

    inline dini::ByteArray serializeWorkspace(const opendspx::Workspace &workspace) {
        return serializeWorkspace(workspaceToJson(workspace));
    }

    inline opendspx::Workspace deserializeWorkspace(const dini::ByteArray &workspace) {
        if (workspace.empty()) {
            return {};
        }
        auto result = stdc::JsonValue::fromCbor(workspace);
        if (result.isObject()) {
            return workspaceFromJson(result.toObject());
        }
        return {};
    }

    inline stdc::JsonValue jsonFromQJsonValue(const QJsonValue &value) {
        switch (value.type()) {
            case QJsonValue::Null:
            case QJsonValue::Undefined:
                return stdc::JsonValue();
            case QJsonValue::Bool:
                return value.toBool();
            case QJsonValue::Double:
                return value.toDouble();
            case QJsonValue::String:
                return value.toString().toStdString();
            case QJsonValue::Array: {
                stdc::JsonArray result;
                const auto array = value.toArray();
                for (const auto &item : array) {
                    result.push_back(jsonFromQJsonValue(item));
                }
                return result;
            }
            case QJsonValue::Object: {
                stdc::JsonObject result;
                const auto object = value.toObject();
                for (auto it = object.begin(); it != object.end(); ++it) {
                    result[it.key().toStdString()] = jsonFromQJsonValue(it.value());
                }
                return result;
            }
        }
        return stdc::JsonValue();
    }

    inline QJsonValue qJsonValueFromJson(const stdc::JsonValue &json) {
        switch (json.type()) {
            case stdc::JsonValue::Null:
            case stdc::JsonValue::Binary:
                return {};
            case stdc::JsonValue::Bool:
                return json.toBool();
            case stdc::JsonValue::Double:
            case stdc::JsonValue::Int:
                return json.toDouble();
            case stdc::JsonValue::String:
                return QString::fromStdString(json.toString());
            case stdc::JsonValue::Array: {
                QJsonArray result;
                for (const auto &item : json.toArray()) {
                    result.append(qJsonValueFromJson(item));
                }
                return result;
            }
            case stdc::JsonValue::Object: {
                QJsonObject result;
                for (const auto &[key, value] : json.toObject()) {
                    result.insert(QString::fromStdString(key), qJsonValueFromJson(value));
                }
                return result;
            }
        }
        return {};
    }

}

#endif // DSPXMODEL_CONVERSIONUTILS_P_H
