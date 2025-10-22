#ifndef UNHIDENPCS_MUMBLELINK_HPP
#define UNHIDENPCS_MUMBLELINK_HPP
#pragma once

struct MumbleContext
{
    unsigned char serverAddress[28]; // contains sockaddr_in or sockaddr_in6
    uint32_t      mapId;
    uint32_t      mapType;
    uint32_t      shardId;
    uint32_t      instance;
    uint32_t      buildId;
    uint32_t      uiState;
    uint16_t      compassWidth;    // pixels
    uint16_t      compassHeight;   // pixels
    float         compassRotation; // radians
    float         playerX;         // continentCoords
    float         playerY;         // continentCoords
    float         mapCenterX;      // continentCoords
    float         mapCenterY;      // continentCoords
    float         mapScale;
    uint32_t      processId;
    uint8_t       mountIndex;

    [[nodiscard]] bool IsMapOpen() const
    {
        return uiState & (1 << 0);
    }

    [[nodiscard]] bool IsCompassTopRight() const
    {
        return uiState & (1 << 1);
    }

    [[nodiscard]] bool IsCompassRotationEnabled() const
    {
        return uiState & (1 << 2);
    }

    [[nodiscard]] bool HasGameFocus() const
    {
        return uiState & (1 << 3);
    }

    [[nodiscard]] bool IsCompetitiveMode() const
    {
        return uiState & (1 << 4);
    }

    [[nodiscard]] bool IsTextboxFocused() const
    {
        return uiState & (1 << 5);
    }

    [[nodiscard]] bool IsInCombat() const
    {
        return uiState & (1 << 6);
    }
};

struct MumbleLink
{
    uint32_t      uiVersion;
    uint32_t      uiTick;
    float         fAvatarPosition[3];
    float         fAvatarFront[3];
    float         fAvatarTop[3];
    wchar_t       name[256];
    float         fCameraPosition[3];
    float         fCameraFront[3];
    float         fCameraTop[3];
    wchar_t       identity[256];
    uint32_t      context_len;
    unsigned char context[256];
    wchar_t       description[2048];

    const MumbleContext& getContext()
    {
        return *reinterpret_cast<MumbleContext*>(&context);
    }
};

inline MumbleLink* getMumbleLink()
{
    auto hMap = OpenFileMappingA(FILE_MAP_READ, FALSE, "MumbleLink");
    if (!hMap)
        return nullptr;
    const auto ptr = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMap);
    return static_cast<MumbleLink*>(ptr);
}


#endif //UNHIDENPCS_MUMBLELINK_HPP
