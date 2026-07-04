#ifndef DSPXMODEL_CONVERSIONUTILS_P_H
#define DSPXMODEL_CONVERSIONUTILS_P_H

#include <cmath>

#include <nlohmann/json.hpp>

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

}

#endif //DSPXMODEL_CONVERSIONUTILS_P_H
