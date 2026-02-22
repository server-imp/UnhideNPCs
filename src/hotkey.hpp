#ifndef UNHIDENPCS_HOTKEY_HPP
#define UNHIDENPCS_HOTKEY_HPP

#include "nlohmann/json.hpp"

struct Hotkey
{
    std::string label {};
    int32_t vkCode {};

    bool ctrl {};
    bool shift {};
    bool alt {};

    bool active {};

    [[nodiscard]] std::string toString() const;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hotkey, vkCode, ctrl, shift, alt)
};

class HotkeyManager
{
private:
    struct
    {
        bool down[256]{};
        bool pressed[256]{};
    } _keyStates {};

    std::vector<std::pair<std::string, Hotkey>> _hotkeys {};
    std::vector<std::function<void(const std::string&)>> _callbacks {};

    std::string _requiredWndClassName {};
    bool _wndClassActive = true;

    std::string _hotkeyCapturing {};
    bool _captureJustStarted {};

    std::filesystem::path _filePath;
    bool _needSave = false;

    bool isHotkeyPressed(Hotkey& hotkey) const;

public:
    explicit HotkeyManager(const std::string& requiredWndClassName = "", std::filesystem::path filePath = "");

    void setRequiredWndClassName(const std::string& className);
    void registerHotkey(const std::string& id, const std::string& label);
    void unregisterHotkey(const std::string& id);
    void triggerCallbacks(const std::string& id) const;
    void registerCallback(const std::function<void(const std::string&)>& callback);
    void tick();
    bool captureHotkey(Hotkey& hotkey);
    void renderHotkey(const std::string& id, Hotkey& hotkey);
    void renderHotkeys();
    void stopCapturing(bool clearHotkey = false);
    [[nodiscard]] bool isCapturing() const;

    [[nodiscard]] Hotkey* getHotkey(const std::string& id);

    void save();
    void load();
};

#endif //UNHIDENPCS_HOTKEY_HPP