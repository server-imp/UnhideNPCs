#ifndef UNHIDENPCS_CFG_HPP
#define UNHIDENPCS_CFG_HPP
#pragma once

#include <charconv>
#include <filesystem>
#include <mutex>
#include <type_traits>
#include <unordered_map>

#include "logger.hpp"
#include "util.hpp"

template <typename T>
inline constexpr auto always_false_v = false;

class property
{
private:
    std::string _comment;
    std::string _key;
    std::string _raw;

public:
    explicit property(std::string comment, std::string key, std::string raw);

    explicit property(std::string key, std::string comment = "");

    property(const property&) = default;

    property(property&&) noexcept = default;

    property& operator=(const property&) = default;

    property& operator=(property&&) noexcept = default;

    [[nodiscard]] const std::string& comment() const noexcept;

    [[nodiscard]] const std::string& key() const noexcept;

    [[nodiscard]] const std::string& raw() const noexcept;

    template <typename T>
    void set(const T& value);

    template <typename T>
    T get(const T& defaultValue = {}) const;
};

template <typename T>
void property::set(const T& value)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        std::string v = value;
        util::replace(v, "\n", "\\n");
        _raw = std::move(v);
    }
    else if constexpr (std::is_same_v<T, const char*>)
    {
        std::string v(value);
        util::replace(v, "\n", "\\n");
        _raw = std::move(v);
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        _raw = value ? "true" : "false";
    }
    else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    {
        _raw = fmt::format("{}", value);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        _raw = fmt::format("{:g}", value);
    }
    else
    {
        static_assert(always_false_v<T>, "cfg::property::set does not support this type");
    }
}

template <typename T>
T property::get(const T& defaultValue) const
{
    if (util::empty_or_whitespace(_raw))
    {
        return defaultValue;
    }

    if constexpr (std::is_same_v<T, std::string>)
    {
        return _raw;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return util::strtob(_raw, defaultValue);
    }
    else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    {
        const std::string& s     = _raw;
        const char*        begin = s.c_str();
        const char*        end   = begin + s.size();
        int                base  = 10;
        if (s.size() > 2 && (s[0] == '0') && (s[1] == 'x' || s[1] == 'X'))
            base = 16;

        T result {};
        if constexpr (std::is_signed_v<T>)
        {
            long long              tmp = 0;
            std::from_chars_result r {};
            if (base == 16)
            {
                unsigned long long utmp = 0;
                r                       = std::from_chars(begin, end, utmp, 16);
                if (r.ec == std::errc())
                    result = static_cast<T>(utmp);
                else
                    return defaultValue;
            }
            else
            {
                r = std::from_chars(begin, end, tmp, 10);
                if (r.ec == std::errc())
                    result = static_cast<T>(tmp);
                else
                    return defaultValue;
            }
        }
        else
        {
            unsigned long long utmp = 0;

            if (auto [ptr, ec] = std::from_chars(begin, end, utmp, base); ec == std::errc())
                result = static_cast<T>(utmp);
            else
                return defaultValue;
        }
        return result;
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        const char* begin  = _raw.c_str();
        char*       endptr = nullptr;
        if constexpr (std::is_same_v<T, float>)
        {
            errno   = 0;
            float v = std::strtof(begin, &endptr);
            if (endptr != begin && errno == 0)
                return v;
        }
        else
        {
            errno    = 0;
            double v = std::strtod(begin, &endptr);
            if (endptr != begin && errno == 0)
                return static_cast<T>(v);
        }
        return defaultValue;
    }
    else
    {
        static_assert(always_false_v<T>, "cfg::property::get does not support this type");
    }

    return {};
}

class Config
{
private:
    std::filesystem::path                   _filePath {};
    std::vector<property>                   _properties {}; // maintain order for save
    std::unordered_map<std::string, size_t> _index {};      // key -> index into _properties
    std::atomic_bool                        _needSave {};
    mutable std::mutex                      _mutex {};
    bool                                    _loaded {};

    static std::string normalize_key(std::string_view k);

public:
    explicit Config(std::filesystem::path filePath);

    Config(const Config&) = delete;

    Config& operator=(const Config&) = delete;

    Config(Config&&) = delete;

    Config& operator=(Config&&) = delete;

    bool load();

    bool save();

    const std::string& getComment(std::string_view key) const;

    template <typename T>
    T get(std::string_view key, const T& defaultValue = {}, std::string comment = "");

    template <typename T>
    void set(std::string_view key, const T& value);

    bool needs_save() const noexcept;
    bool loaded() const noexcept;

    const std::filesystem::path& file_path() const noexcept;
};

template <typename T>
T Config::get(const std::string_view key, const T& defaultValue, std::string comment)
{
    const std::string nkey = normalize_key(key);
    {
        std::lock_guard lock(_mutex);
        if (const auto it = _index.find(nkey); it != _index.end())
        {
            const property& p = _properties[it->second];
            return p.get<T>(defaultValue);
        }
    }

    property p(nkey, std::move(comment));
    p.set<T>(defaultValue);
    {
        std::lock_guard lock(_mutex);
        _index.emplace(p.key(), _properties.size());
        _properties.emplace_back(std::move(p));
        LOG_DBG("Default {}: {}", nkey, defaultValue);
        _needSave = true;
    }

    return defaultValue;
}

template <typename T>
void Config::set(const std::string_view key, const T& value)
{
    const std::string nkey = normalize_key(key);

    std::lock_guard lock(_mutex);
    if (const auto it = _index.find(nkey); it != _index.end())
    {
        _properties[it->second].set<T>(value);
    }
    else
    {
        property p(nkey);
        p.set<T>(value);
        _index.emplace(p.key(), _properties.size());
        _properties.emplace_back(std::move(p));
    }
    _needSave = true;
}

#define CONFIG_PROPERTY(type, name, defaultValue, comment)\
private:\
type _##name = this->get<type>(#name, defaultValue, comment);\
public:\
const std::string& getComment##name() const\
{\
return this->getComment(#name);\
}\
\
type get##name() const\
{\
return _##name;\
}\
\
void set##name(const type value)\
{\
_##name = value;\
this->set<type>(#name, value);\
this->save();\
}

#endif //UNHIDENPCS_CFG_HPP
