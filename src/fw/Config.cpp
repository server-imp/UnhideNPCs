#include "config.hpp"

property::property
(std::string comment, std::string key, std::string raw) : _comment
                                                          (std::move(comment)),
                                                          _key(std::move(key)),
                                                          _raw(std::move(raw))
{
    util::trim(_raw);
    util::replace(_raw, "\\n", "\n");
}

property::property(std::string key, std::string comment) : property(std::move(comment), std::move(key), "") {}

const std::string& property::comment() const noexcept
{
    return _comment;
}

const std::string& property::key() const noexcept
{
    return _key;
}

const std::string& property::raw() const noexcept
{
    return _raw;
}

std::string Config::normalize_key(const std::string_view k)
{
    std::string s{k};
    util::trim(s);
    return s;
}

Config::Config(std::filesystem::path filePath) : _filePath(std::move(filePath))
{
    if (!std::filesystem::exists(_filePath.parent_path()))
        std::filesystem::create_directories(_filePath.parent_path());

    load();
}

bool Config::load()
{
    std::lock_guard lock(_mutex);
    _properties.clear();
    _index.clear();
    _needSave = false;
    _loaded = false;

    std::vector<std::string> lines;
    try
    {
        lines = util::readLines(_filePath.string());
    }
    catch (const std::exception& ex)
    {
        LOG_DBG("Failed to read file {}: {}", _filePath.string(), ex.what());
        return false;
    }

    std::string comment;
    std::string raw;

    for (auto line : lines)
    {
        util::trim(line);
        if (line.empty())
            continue;

        if (line.rfind('#', 0) == 0)
        {
            line = line.substr(1);
            util::ltrim(line);

            if (!comment.empty())
                comment.append("\n");
            comment.append(line);
            continue;
        }

        size_t indexPos = 0;
        if ((indexPos = line.find_first_of(':')) == std::string::npos)
        {
            LOG_DBG("Invalid line in {}: {}", _filePath.string(), line.c_str());
            return false;
        }

        std::string key = line.substr(0, indexPos);
        raw             = line.substr(indexPos + 1);
        util::trim(raw);
        util::trim(key);

        if (util::empty_or_whitespace(raw))
        {
            raw.clear();
            _needSave = true;
        }

        LOG_DBG("{}: {}", key, raw.c_str());

        _index.emplace(key, _properties.size());
        _properties.emplace_back(std::move(comment), std::move(key), std::move(raw));

        comment.clear();
        key.clear();
        raw.clear();
    }

    _loaded = true;
    return true;
}

bool Config::save()
{
    if (!_needSave)
        return true;

    std::lock_guard lock(_mutex);

    if (!_needSave)
        return true;

    std::filesystem::path tmp = _filePath;
    tmp += ".tmp";

    std::ofstream ofs(tmp, std::ios::trunc);
    if (!ofs)
    {
        LOG_DBG("Failed to open {} for writing", tmp.string());
        return false;
    }

    bool first = true;
    for (const auto& p : _properties)
    {
        if (!first)
            ofs << "\n\n";
        first = false;

        if (!p.comment().empty())
        {
            std::string comm = p.comment();
            util::replace(comm, "\n", "\n# ");
            ofs << "# " << comm << "\n";
        }

        std::string raw = p.raw();
        util::replace(raw, "\n", "\\n");
        ofs << p.key() << ": " << raw;
    }

    ofs.flush();
    ofs.close();

    std::error_code ec;
    std::filesystem::rename(tmp, _filePath, ec);
    if (ec)
    {
        LOG_DBG("Failed to replace {} with {}: {}", _filePath.string(), tmp.string(), ec.message());
        std::filesystem::remove(tmp, ec);
        return false;
    }

    _needSave = false;
    return true;
}

bool Config::needs_save() const noexcept
{
    return _needSave;
}

bool Config::loaded() const noexcept
{
    return _loaded;
}

const std::filesystem::path& Config::file_path() const noexcept
{
    return _filePath;
}
