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
    const auto  scan = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
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

    const bool justPressed = (lParam & (1 << 30)) == 0;
    if (!justPressed)
    {
        return msg;
    }

    if (unpc::mumbleLink && unpc::mumbleLink->getContext().isTextboxFocused())
    {
        return msg;
    }

    if (ImGui::GetCurrentContext())
    {
        const auto& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard || io.WantTextInput)
        {
            return msg;
        }
    }

    if (wParam == VK_ESCAPE && isCapturing())
    {
        stopCapturing(true);
        return 0;
    }

    const auto vkCode = wParam;

    switch (vkCode)
    {
    case 0:
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
    case VK_BACK:
    case VK_TAB:
    case VK_RETURN:
    case VK_PAUSE:
    case VK_CAPITAL:
    case VK_NONAME:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU: return msg;
    default: break;
    }

    const bool ctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const bool alt   = (GetKeyState(VK_MENU) & 0x8000) != 0;

    if (!_hotkeyCapturing.empty())
    {
        const auto hotkey = getHotkey(_hotkeyCapturing);
        if (!hotkey)
        {
            _hotkeyCapturing = "";
            return 0;
        }

        hotkey->vkCode   = vkCode;
        hotkey->ctrl     = ctrl;
        hotkey->shift    = shift;
        hotkey->alt      = alt;
        hotkey->active   = true; // prevent it from activating immediately

        LOG_INFO("Updated hotkey \"{}\" to {}", _hotkeyCapturing, hotkey->toString());

        _hotkeyCapturing = "";
        _needSave        = true;

        return 0;
    }

    for (auto& [id, hotkey] : _hotkeys)
    {
        if (hotkey.vkCode == 0)
        {
            continue;
        }

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

void HotkeyManager::update()
{
    if (_needSave)
    {
        save();
    }
}

void HotkeyManager::renderHotkey(const std::string& id, Hotkey& hotkey)
{
    std::lock_guard lock(_mutex);

    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    const bool capturing = _hotkeyCapturing == id;
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
    if (!_hotkeyCapturing.empty())
    {
        if (clearHotkey)
        {
            const auto hotkey = getHotkey(_hotkeyCapturing);
            if (hotkey)
            {
                hotkey->vkCode = 0;
                _needSave      = true;
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
