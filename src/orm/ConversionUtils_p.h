#ifndef DSPXMODEL_CONVERSIONUTILS_P_H
#define DSPXMODEL_CONVERSIONUTILS_P_H

#include <cmath>
#include <utility>

#include <dini/types.h>
#include <nlohmann/json.hpp>
#include <opendspx/workspace.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

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
        bool optionalChainStep(const nlohmann::json *&cur, T &&arg) {
            using D = std::decay_t<T>;

            if constexpr (is_string_like_v<T>) {
                if (!cur || !cur->is_object()) {
                    return false;
                }

                if constexpr (std::is_convertible_v<T, const char *>
                           && !std::is_same_v<D, std::string>
                           && !std::is_same_v<D, std::string_view>) {
                    const char *key = arg;
                    if (!key) {
                        return false;
                    }

                    auto it = cur->find(key);
                    if (it == cur->end()) {
                        return false;
                    }

                    cur = &(*it);
                    return true;
                } else {
                    std::string key(arg);

                    auto it = cur->find(key);
                    if (it == cur->end()) {
                        return false;
                    }

                    cur = &(*it);
                    return true;
                }
            } else if constexpr (std::is_integral_v<D> && !std::is_same_v<D, bool>) {
                if (!cur || !cur->is_array()) {
                    return false;
                }

                if constexpr (std::is_signed_v<D>) {
                    if (arg < 0) {
                        return false;
                    }
                }

                auto index = static_cast<nlohmann::json::size_type>(arg);

                if (index >= cur->size()) {
                    return false;
                }

                cur = &((*cur)[index]);
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

    template <typename ...Args>
    nlohmann::json optionalChain(const nlohmann::json &json, Args &&...args) {
        const nlohmann::json *cur = &json;
        bool ok = (detail::optionalChainStep(cur, std::forward<Args>(args)) && ...);
        if (!ok || !cur) {
            return nullptr;
        }
        return *cur;
    }

    inline dini::ByteArray serializeWorkspace(const nlohmann::json &workspace) {
        if (!workspace.is_object()) {
            return {};
        }
        return nlohmann::json::to_msgpack(workspace);
    }

    inline nlohmann::json &ensureObject(nlohmann::json &value) {
        if (!value.is_object()) {
            value = nlohmann::json::object();
        }
        return value;
    }

    inline nlohmann::json &ensureObjectMember(nlohmann::json &value, const char *key) {
        auto &object = ensureObject(value);
        auto &member = object[key];
        return ensureObject(member);
    }

    inline nlohmann::json workspaceToJson(const opendspx::Workspace &workspace) {
        auto result = nlohmann::json::object();
        for (const auto &[key, value] : workspace) {
            result[key] = value;
        }
        return result;
    }

    inline opendspx::Workspace workspaceFromJson(const nlohmann::json &workspace) {
        opendspx::Workspace result;
        if (!workspace.is_object()) {
            return result;
        }
        for (auto it = workspace.begin(); it != workspace.end(); ++it) {
            result[it.key()] = *it;
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
        try {
            auto result = nlohmann::json::from_msgpack(workspace);
            if (result.is_object()) {
                return workspaceFromJson(result);
            }
        } catch (const nlohmann::json::exception &) {
        }
        return {};
    }

    inline nlohmann::json jsonFromQJsonValue(const QJsonValue &value) {
        switch (value.type()) {
            case QJsonValue::Null:
            case QJsonValue::Undefined:
                return nullptr;
            case QJsonValue::Bool:
                return value.toBool();
            case QJsonValue::Double:
                return value.toDouble();
            case QJsonValue::String:
                return value.toString().toStdString();
            case QJsonValue::Array: {
                auto result = nlohmann::json::array();
                const auto array = value.toArray();
                for (const auto &item : array) {
                    result.push_back(jsonFromQJsonValue(item));
                }
                return result;
            }
            case QJsonValue::Object: {
                auto result = nlohmann::json::object();
                const auto object = value.toObject();
                for (auto it = object.begin(); it != object.end(); ++it) {
                    result[it.key().toStdString()] = jsonFromQJsonValue(it.value());
                }
                return result;
            }
        }
        return nullptr;
    }

    inline QJsonValue qJsonValueFromJson(const nlohmann::json &json) {
        if (json.is_null()) {
            return {};
        }
        if (json.is_boolean()) {
            return json.get<bool>();
        }
        if (json.is_number()) {
            return json.get<double>();
        }
        if (json.is_string()) {
            return QString::fromStdString(json.get<std::string>());
        }
        if (json.is_array()) {
            QJsonArray result;
            for (const auto &item : json) {
                result.append(qJsonValueFromJson(item));
            }
            return result;
        }
        if (json.is_object()) {
            QJsonObject result;
            for (auto it = json.begin(); it != json.end(); ++it) {
                result.insert(QString::fromStdString(it.key()), qJsonValueFromJson(*it));
            }
            return result;
        }
        return {};
    }

}

#endif //DSPXMODEL_CONVERSIONUTILS_P_H
