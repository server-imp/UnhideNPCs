#ifndef FW_SETTINGS_HPP
#define FW_SETTINGS_HPP

#include <utility>
#include <variant>
#include <memory>
#include <type_traits>
#include <nlohmann/json.hpp>

namespace fw
{
    class Settings;

    template <typename T>
    class Setting;

    using SettingVariant = std::variant<std::unique_ptr<Setting<int>>, std::unique_ptr<Setting<float>>, std::unique_ptr<
                                            Setting<bool>>>;
    using SettingRefVariant = std::variant<Setting<int>*, Setting<float>*, Setting<bool>*>;

    using OnSettingChangedFn = void(*)(SettingRefVariant&);

    template <typename T>
    class Setting
    {
    protected:
        Settings* _owner;

        std::atomic<T> _value {};
        std::atomic<T> _default {};
        std::string    _name {};
        std::string    _description {};

        std::function<void(SettingRefVariant&)> _onChanged {};
        std::function<void(std::atomic<T>&)>    _validator {};

    public:
        virtual ~Setting() = default;

        Setting(Settings* owner, T value, std::string name, std::string description) : _owner(owner), _value(value),
            _default(value), _name(std::move(name)), _description(std::move(description)) {}

        const std::string& name() const;
        const std::string& description() const;

        T set(T value, bool save = true);
        T get() const;
        T defaultValue() const;

        Setting<T>& onChanged(const OnSettingChangedFn callback)
        {
            _onChanged = callback;
            return *this;
        }

        Setting<T>& validator(const std::function<void(std::atomic<T>&)>& callback)
        {
            _validator = callback;
            return *this;
        }
    };

    class Settings
    {
    protected:
        Settings* _owner {};

        std::string                            _name {};
        std::vector<SettingVariant>            _settings {};
        std::vector<std::unique_ptr<Settings>> _children {};

        std::mutex _mutex {};
        bool       _needSave = false;

        std::filesystem::path _filePath;

        nlohmann::ordered_json serialize() const;
        void deserialize(const nlohmann::ordered_json& json);

        template <typename T, typename... Args>
        Setting<T>& add(Args&&... args)
        {
            auto        ptr = std::make_unique<Setting<T>>(this, std::forward<Args>(args)...);
            Setting<T>& ref = *ptr;
            _settings.emplace_back(std::move(ptr));
            return ref;
        }

        template <typename T, typename... Args>
        T& addChild(Args&&... args)
        {
            static_assert(std::is_base_of_v<Settings, T>, "Child must derive from Settings");
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            T&   ref = *ptr;
            _children.emplace_back(std::move(ptr));
            return ref;
        }

        virtual std::unique_ptr<Settings> createChild(const std::string& name)
        {
            return std::make_unique<fw::Settings>(this, name);
        }

    public:
        virtual  ~Settings() = default;
        explicit Settings(Settings* owner = nullptr, std::string name = "", std::filesystem::path filePath = "");

        void needSave();
        void save(bool force = false);
        void load();

        const std::string& name() const;
        void setName(const std::string& name);
        std::vector<SettingVariant>&            settings();
        std::vector<std::unique_ptr<Settings>>& children();
    };

    template <typename T>
    const std::string& Setting<T>::name() const
    {
        return _name;
    }

    template <typename T>
    const std::string& Setting<T>::description() const
    {
        return _description;
    }

    template <typename T>
    T Setting<T>::set(T value, const bool save)
    {
        _value.store(value, std::memory_order_relaxed);

        if (_validator)
        {
            _validator(_value);
        }

        if (save && _owner)
        {
            _owner->needSave();
        }

        if (_onChanged)
        {
            auto refVariant = fw::SettingRefVariant(this);
            _onChanged(refVariant);
        }

        return value;
    }

    template <typename T>
    T Setting<T>::get() const
    {
        if constexpr (std::is_trivially_copyable_v<T>)
        {
            return _value.load(std::memory_order_relaxed);
        }
        else
        {
            return _value;
        }
    }

    template <typename T>
    T Setting<T>::defaultValue() const
    {
        return _default;
    }
}

#endif //FW_SETTINGS_HPP
