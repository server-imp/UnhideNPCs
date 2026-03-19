#include "settings.hpp"

#include "logger.hpp"
#include "util.hpp"

nlohmann::ordered_json fw::Settings::serialize() const
{
    nlohmann::ordered_json json;

    if (!_name.empty())
    {
        json["name"] = _name;
    }

    for (const auto& setting : _settings)
    {
        std::visit(
            [&json](auto&& value)
            {
                json[value->name()] = value->get();
            },
            setting
        );
    }

    if (_children.empty())
    {
        return json;
    }

    json["children"] = nlohmann::json::array();
    for (const auto& child : _children)
    {
        auto j = child->serialize();
        json["children"].push_back(j);
    }

    return json;
}

void fw::Settings::deserialize(const nlohmann::ordered_json& json)
{
    auto setKey = [&](const std::string& key, const nlohmann::json& value)
    {
        for (auto& setting : _settings)
        {
            bool stop = false;
            std::visit(
                [&](auto&& s)
                {
                    if (util::equalsIgnoreCase(s->name(), key))
                    {
                        s->set(value, false);
                        stop = true;
                    }
                },
                setting
            );
            if (stop)
            {
                return;
            }
        }
    };

    auto updateExistingChild = [&](const std::string& name, const nlohmann::ordered_json& json)
    {
        for (const auto& child : _children)
        {
            if (child->_name.empty())
            {
                continue;
            }

            if (util::equalsIgnoreCase(child->_name, name))
            {
                child->deserialize(json);
                return true;
            }
        }

        return false;
    };

    if (json.contains("children") && json["children"].is_array())
    {
        for (auto& childJson : json["children"])
        {
            auto name = childJson.value("name", "");
            if (name.empty())
            {
                continue;
            }

            if (!updateExistingChild(name, childJson))
            {
                auto child = createChild(childJson.value("name", ""));
                child->deserialize(childJson);
                _children.push_back(std::move(child));
            }
        }
    }

    for (auto it = json.begin(); it != json.end(); ++it)
    {
        if (it.key() == "children" || it.key() == "name")
            continue;
        setKey(it.key(), it.value());
    }
}

fw::Settings::Settings(Settings* owner, std::string name, std::filesystem::path filePath)
{
    _owner    = owner;
    _name     = std::move(name);
    _filePath = std::move(filePath);
}

void fw::Settings::needSave()
{
    std::lock_guard lock(_mutex);

    if (_owner && _owner != this)
    {
        _owner->needSave();
        return;
    }

    _needSave = true;
}

void fw::Settings::save(const bool force)
{
    std::lock_guard lock(_mutex);

    if (!_needSave && !force)
    {
        return;
    }

    _needSave = false;

    if (_owner && _owner != this)
    {
        _owner->save(force);
        return;
    }

    std::ofstream ofs(_filePath, std::ios::trunc | std::ios::out);
    ofs << serialize().dump(2);
}

void fw::Settings::load()
{
    std::unique_lock lock(_mutex);

    _loaded = false;
    if (_owner && _owner != this)
    {
        lock.unlock();
        _owner->load();
        lock.lock();
        _loaded = _owner->loaded();
        return;
    }

    std::ifstream ifs(_filePath);
    if (!ifs.is_open())
    {
        LOG_WARN("Failed to open settings file, re-saving current state");
        lock.unlock();
        save(true);
        _loaded = true;
        return;
    }

    const auto  json = nlohmann::ordered_json::parse(ifs, nullptr, false);
    if (json.is_discarded() || !json.is_object() || json.empty() || json.is_null())
    {
        LOG_WARN("Failed to parse settings file, re-saving current state");
        lock.unlock();
        save(true);
        _loaded = true;
        return;
    }

    try
    {
        deserialize(json);
        LOG_INFO("Loaded settings");
        _loaded = true;
    }
    catch (const std::exception& e)
    {
        LOG_WARN("Failed to deserialize settings, re-saving current state");
        lock.unlock();
        save(true);
        _loaded = true;
    }
}

bool fw::Settings::loaded() const
{
    return _loaded;
}

const std::string& fw::Settings::name() const
{
    return _name;
}

void fw::Settings::setName(const std::string& name)
{
    _name = name;
    needSave();
}

std::vector<fw::SettingVariant>& fw::Settings::settings()
{
    return _settings;
}

std::vector<std::unique_ptr<fw::Settings>>& fw::Settings::children()
{
    return _children;
}
