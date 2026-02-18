#ifndef UNHIDENPCS_MUMBLELINK_HPP
#define UNHIDENPCS_MUMBLELINK_HPP
#pragma once
#include "pch.hpp"

enum class MapType : uint32_t
{
    Redirect,
    CharacterCreation,
    CompetitivePvP,
    GvG,
    Instances,
    Public,
    Tournament,
    Tutorial,
    UserTournament,
    EternalBattlegrounds,
    BlueBorderlands,
    GreenBorderlands,
    RedBorderlands,
    WvWReward,
    ObsidianSanctum,
    EdgeOfTheMists,
    PublicMini,
    BigBattle,
    ArmisticeBastion
};

#pragma pack(push, 1)
struct MumbleContext
{
    unsigned char serverAddress[28]; // contains sockaddr_in or sockaddr_in6
    uint32_t      mapId;
    MapType       mapType;
    uint32_t      shardId;
    uint32_t      instance;
    uint32_t      buildId;
    // Additional data beyond the <context_len> bytes the game instructs Mumble to use to distinguish between instances.
    uint32_t uiState;
    // Bitmask: Bit 1 = IsMapOpen, Bit 2 = IsCompassTopRight, Bit 3 = DoesCompassHaveRotationEnabled, Bit 4 = Game has focus, Bit 5 = Is in Competitive game mode, Bit 6 = Textbox has focus, Bit 7 = Is in Combat
    uint16_t compassWidth;    // pixels
    uint16_t compassHeight;   // pixels
    float    compassRotation; // radians
    float    playerX;         // continentCoords
    float    playerY;         // continentCoords
    float    mapCenterX;      // continentCoords
    float    mapCenterY;      // continentCoords
    float    mapScale;
    uint32_t processId;
    uint8_t  mountIndex;

    [[nodiscard]] bool isMapOpen() const
    {
        return uiState & (1 << 0);
    }

    [[nodiscard]] bool isCompassTopRight() const
    {
        return uiState & (1 << 1);
    }

    [[nodiscard]] bool isCompassRotationEnabled() const
    {
        return uiState & (1 << 2);
    }

    [[nodiscard]] bool hasGameFocus() const
    {
        return uiState & (1 << 3);
    }

    [[nodiscard]] bool isCompetitiveMode() const
    {
        return uiState & (1 << 4);
    }

    [[nodiscard]] bool isTextboxFocused() const
    {
        return uiState & (1 << 5);
    }

    [[nodiscard]] bool isInCombat() const
    {
        return uiState & (1 << 6);
    }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct MumbleLink
{
    uint32_t uiVersion;
    uint32_t uiTick;
    float    fAvatarPosition[3];
    float    fAvatarFront[3];
    float    fAvatarTop[3];
    wchar_t  name[256];
    float    fCameraPosition[3];
    float    fCameraFront[3];
    float    fCameraTop[3];
    wchar_t  identity[256];
    uint32_t contextLen;
    // Despite the actual context containing more data, this value is currently 48. See "context" section below.
    unsigned char context[256];
    wchar_t       description[2048];

    const MumbleContext& getContext()
    {
        return *reinterpret_cast<MumbleContext*>(&context);
    }
};
#pragma pack(pop)

inline MumbleLink* getMumbleLink()
{
    std::string mappingName = util::getStartupArgValue("-mumble");
    if (mappingName.empty())
    {
        mappingName = "MumbleLink";
    }

    auto hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, mappingName.c_str());
    if (!hMap)
    {
        LOG_DBG("OpenFileMappingW failed: \"{}\", {}", mappingName, GetLastError());
        hMap = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            sizeof(MumbleLink),
            mappingName.c_str()
        );

        if (!hMap)
        {
            LOG_DBG("CreateFileMappingW failed: {}", GetLastError());
            return nullptr;
        }

        LOG_DBG("CreateFileMappingW succeeded");
    }

    const auto ptr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MumbleLink));
    if (!ptr)
    {
        LOG_DBG("MapViewOfFile failed: {}", GetLastError());
        CloseHandle(hMap);
        return nullptr;
    }

    // The game won't initialize Mumble Link if we close this handle
    // CloseHandle(hMap);

    return static_cast<MumbleLink*>(ptr);
}
#endif //UNHIDENPCS_MUMBLELINK_HPP
