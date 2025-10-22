#ifndef UNHIDENPCS_UI_HPP
#define UNHIDENPCS_UI_HPP
#pragma once

namespace ui
{
    void tooltip(const char* text);

    bool checkbox(const char* label, const char* id, bool& value, const char* tip);

    bool combo(const char* label, const char* id, int& value, const char* const* items, int count, const char* tip);

    bool sliderInt(const char* label, const char* id, int& value, int min, int max, const char* fmt, const char* tip);

    void renderOptions();

    void renderWindow(uint32_t not_charsel_or_loading, uint32_t hide_if_combat_or_ooc);

    bool wasKeyPressed(int vKey);
    bool wasComboPressed(const std::initializer_list<int>& combo);
}

#endif //UNHIDENPCS_UI_HPP
