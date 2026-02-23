#include "hotkey.hpp"

#include "imgui.h"
#include "ui.hpp"
#include "unpc.hpp"
#include "fw/logger.hpp"

std::string Hotkey::toString() const
{
    if (vkCode == 0)
    {
        return "None";
    }

    std::string s;

    if (ctrl)
    {
        s += "Ctrl + ";
    }
    if (shift)
    {
        s += "Shift + ";
    }
    if (alt)
    {
        s += "Alt + ";
    }

    static char name[64];
    const UINT  scan = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    GetKeyNameTextA(scan << 16, name, 64);

    s += name;

    return s;
}

bool HotkeyManager::isHotkeyPressed(Hotkey& hotkey) const
{
    if (!_wndClassActive)
    {
        return false;
    }

    if (hotkey.vkCode == 0)
    {
        return false;
    }

    const bool pressed = _keyStates.down[hotkey.vkCode] && _keyStates.down[VK_CONTROL] == hotkey.ctrl && _keyStates.down[VK_SHIFT] == hotkey.shift && _keyStates
        .down[VK_MENU] == hotkey.alt;

    if (pressed && !hotkey.active)
    {
        hotkey.active = true;
        return true;
    }

    if (!pressed)
    {
        hotkey.active = false;
    }

    return false;
}

HotkeyManager::HotkeyManager(const std::string& requiredWndClassName, std::filesystem::path filePath)
{
    setRequiredWndClassName(requiredWndClassName);
    _filePath = std::move(filePath);
}

uintptr_t HotkeyManager::onWndProc(HWND hWnd, const UINT msg, const WPARAM wParam, LPARAM lParam)
{
    if (msg != WM_KEYDOWN)
    {
        return msg;
    }

    if (wParam == VK_ESCAPE && isCapturing())
    {
        unpc::hotkeyManager.stopCapturing(true);
        return 0;
    }

    const auto vkCode = wParam;

    if (vkCode == VK_LBUTTON || vkCode == VK_RBUTTON || vkCode == VK_MBUTTON || vkCode == VK_XBUTTON1 || vkCode == VK_XBUTTON2 || vkCode == VK_BACK || vkCode ==
        VK_TAB || vkCode == VK_RETURN || vkCode == VK_PAUSE || vkCode == VK_CAPITAL || vkCode == VK_NONAME)
    {
        return msg;
    }

    if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL || vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT || vkCode ==
        VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU)
    {
        return msg;
    }

    const bool ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool alt   = (GetKeyState(VK_MENU) & 0x8000) != 0;

    static char name[64];
    const UINT  scan = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    GetKeyNameTextA(scan << 16, name, 64);
    LOG_INFO("onWndProc: {} [{}, {}, {}]", name, ctrl, shift, alt);

    if (!_hotkeyCapturing.empty())
    {
        const auto hotkey = getHotkey(_hotkeyCapturing);
        if (!hotkey)
        {
            _hotkeyCapturing = "";
            return 0;
        }

        LOG_INFO("Updated hotkey \"{}\" to {}", _hotkeyCapturing, hotkey->toString());
        hotkey->vkCode = vkCode;
        hotkey->ctrl   = ctrl;
        hotkey->shift  = shift;
        hotkey->alt    = alt;
        hotkey->active   = true; // prevent it from activating immediately
        _hotkeyCapturing = "";
        _needSave        = true;

        return 0;
    }

    for (auto& [id, hotkey] : _hotkeys)
    {
        if (hotkey.vkCode != vkCode)
        {
            continue;
        }

        if (hotkey.ctrl && !ctrl)
        {
            continue;
        }

        if (hotkey.shift && !shift)
        {
            continue;
        }

        if (hotkey.alt && !alt)
        {
            continue;
        }

        triggerCallbacks(id);
    }

    return msg;
}

void HotkeyManager::setRequiredWndClassName(const std::string& className)
{
    _requiredWndClassName = className;
}

void HotkeyManager::registerHotkey(const std::string& id, const std::string& label)
{
    if (getHotkey(id))
    {
        LOG_WARN("Hotkey \"{}\" already registered", id);
        return;
    }

    _hotkeys.emplace_back(id, Hotkey { .label = label });
}

void HotkeyManager::unregisterHotkey(const std::string& id)
{
    _hotkeys.erase(
        std::remove_if(
            _hotkeys.begin(),
            _hotkeys.end(),
            [&](const auto& pair)
            {
                return pair.first == id;
            }
        ),
        _hotkeys.end()
    );
}

void HotkeyManager::triggerCallbacks(const std::string& id) const
{
    for (const auto& callback : _callbacks)
    {
        callback(id);
    }
}

void HotkeyManager::registerCallback(const std::function<void(const std::string&)>& callback)
{
    _callbacks.push_back(callback);
}

void HotkeyManager::tick()
{
    /*if (!_requiredWndClassName.empty())
    {
        auto hWnd = GetForegroundWindow();
        if (!hWnd)
        {
            _wndClassActive = false;
        }
        else
        {
            char className[256] {};
            if (!GetClassNameA(hWnd, className, sizeof(className)))
            {
                _wndClassActive = false;
            }
            else
            {
                _wndClassActive = strcmp(className, _requiredWndClassName.c_str()) == 0;
            }
        }
    }

    if (!_wndClassActive)
    {
        return;
    }

    for (int vk = 0; vk < 256; vk++)
    {
        const SHORT state = GetAsyncKeyState(vk);

        _keyStates.down[vk]    = (state & 0x8000) != 0;
        _keyStates.pressed[vk] = (state & 1) != 0;
    }

    if (!unpc::settings)
    {
        return;
    }

    if (!_hotkeyCapturing.empty())
    {
        const auto hotkey = getHotkey(_hotkeyCapturing);
        if (hotkey)
        {
            if (captureHotkey(*hotkey))
            {
                LOG_INFO("Updated hotkey \"{}\" to {}", _hotkeyCapturing, hotkey->toString());
                _hotkeyCapturing = "";
                hotkey->active   = true; // prevent it from activating immediately
                _needSave        = true;
            }
        }
    }

    for (auto& [id, hotkey] : _hotkeys)
    {
        if (isHotkeyPressed(hotkey))
        {
            triggerCallbacks(id);

            LOG_INFO("Hotkey \"{}\" [{}] triggered", id, hotkey.toString());
        }
    }*/

    if (_needSave)
    {
        save();
    }
}

bool HotkeyManager::captureHotkey(Hotkey& hotkey)
{
    std::lock_guard lock(_mutex);

    if (!_wndClassActive)
    {
        return false;
    }

    if (_captureJustStarted)
    {
        _captureJustStarted = false;
        return false;
    }

    for (int vk = 1; vk < 256; vk++)
    {
        if (!_keyStates.down[vk])
        {
            continue;
        }

        if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2 || vk == VK_BACK || vk == VK_TAB || vk ==
            VK_RETURN || vk == VK_PAUSE || vk == VK_CAPITAL || vk == VK_NONAME)
        {
            continue;
        }

        if (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_MENU || vk ==
            VK_LMENU || vk == VK_RMENU)
        {
            continue;
        }

        const bool ctrl = _keyStates.down[VK_LCONTROL] || _keyStates.down[VK_RCONTROL];

        const bool shift = _keyStates.down[VK_LSHIFT] || _keyStates.down[VK_RSHIFT];

        const bool alt = _keyStates.down[VK_LMENU] || _keyStates.down[VK_RMENU];

        hotkey.vkCode = vk;
        hotkey.ctrl   = ctrl;
        hotkey.shift  = shift;
        hotkey.alt    = alt;

        LOG_INFO("Captured hotkey: {} [{}]", hotkey.toString(), vk);
        return true;
    }

    return false;
}

void HotkeyManager::renderHotkey(const std::string& id, Hotkey& hotkey)
{
    std::lock_guard lock(_mutex);

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    bool capturing = _hotkeyCapturing == id;
    ImGui::TextUnformatted(capturing ? "Esc to cancel" : hotkey.label.c_str());
    ImGui::TableSetColumnIndex(1);

    if (capturing)
    {
        ImGui::PushID((id + "_capturing").c_str());
        if (ImGui::Button("Press Keys...", ImVec2(-FLT_MIN, 0)))
        {
            _hotkeyCapturing    = "";
            _captureJustStarted = false;
            hotkey.vkCode       = 0;
            LOG_INFO("Updated hotkey \"{}\" to {}", id, hotkey.toString());
            _needSave = true;
        }
    }
    else
    {
        ImGui::PushID(id.c_str());
        if (ImGui::Button(hotkey.toString().c_str(), ImVec2(-FLT_MIN, 0)))
        {
            _hotkeyCapturing    = id;
            _captureJustStarted = true;
        }
    }

    ImGui::PopID();
}

void HotkeyManager::renderHotkeys()
{
    if (ImGui::BeginTable("hotkey_table", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY, ImVec2(0, 300)))
    {
        ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Button", ImGuiTableColumnFlags_WidthFixed, ui::fieldWidth);
        ImGui::TableNextRow();
        for (auto& [id, hotkey] : _hotkeys)
        {
            ImGui::TableNextRow();
            renderHotkey(id, hotkey);
        }
        ImGui::EndTable();
    }
}

void HotkeyManager::stopCapturing(const bool clearHotkey)
{
    std::lock_guard lock(_mutex);

    if (!_hotkeyCapturing.empty())
    {
        if (clearHotkey)
        {
            const auto hotkey = getHotkey(_hotkeyCapturing);
            if (hotkey)
            {
                hotkey->vkCode = 0;
            }
        }

        _hotkeyCapturing = "";
    }
}

bool HotkeyManager::isCapturing() const
{
    return !_hotkeyCapturing.empty();
}

Hotkey* HotkeyManager::getHotkey(const std::string& id)
{
    for (auto& [key, hotkey] : _hotkeys)
    {
        if (key == id)
        {
            return &hotkey;
        }
    }

    return nullptr;
}

void HotkeyManager::save()
{
    const std::filesystem::path dir = _filePath.parent_path();
    if (!exists(dir))
    {
        create_directories(dir);
    }

    std::ofstream file(_filePath);
    if (!file.is_open())
    {
        LOG_ERR("Failed to open {} for writing", _filePath.string());
        return;
    }

    const nlohmann::json json = _hotkeys;

    file << std::setw(4) << json << '\n';

    _needSave = false;
}

void HotkeyManager::load()
{
    if (!exists(_filePath))
    {
        return;
    }

    std::ifstream file(_filePath);
    if (!file.is_open())
    {
        LOG_ERR("Failed to open {} for reading", _filePath.string());
        return;
    }

    nlohmann::json json;
    file >> json;

    std::vector<std::pair<std::string, Hotkey>> hotkeys = json;
    for (auto& [id, hotkey] : hotkeys)
    {
        auto hk = getHotkey(id);
        if (!hk)
        {
            LOG_WARN("Unknown hotkey \"{}\"", id);
            continue;
        }

        const Hotkey temp = hotkey;

        hk->vkCode = temp.vkCode;
        hk->ctrl   = temp.ctrl;
        hk->shift  = temp.shift;
        hk->alt    = temp.alt;
        LOG_INFO("Loaded hotkey \"{}\": {}", id, hk->toString());
    }
}
