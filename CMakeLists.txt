cmake_minimum_required(VERSION 3.31)
set(PROJECT_NAME "UnhideNPCs")
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

string(TIMESTAMP UNPC_VER_YEAR "%Y" UTC)
string(TIMESTAMP UNPC_VER_MONTH "%m" UTC)
string(REGEX REPLACE "^0" "" UNPC_VER_MONTH "${UNPC_VER_MONTH}")
string(TIMESTAMP UNPC_VER_DAY "%d" UTC)
string(REGEX REPLACE "^0" "" UNPC_VER_DAY "${UNPC_VER_DAY}")
string(TIMESTAMP UNPC_VER_HOUR "%H" UTC)
string(REGEX REPLACE "^0" "" UNPC_VER_HOUR "${UNPC_VER_HOUR}")
string(TIMESTAMP UNPC_VER_MINUTE "%M" UTC)
set(UNPC_VER_BUILD "${UNPC_VER_HOUR}${UNPC_VER_MINUTE}")
set(UNPC_VER_STRING "${UNPC_VER_YEAR}.${UNPC_VER_MONTH}.${UNPC_VER_DAY}.${UNPC_VER_BUILD}")

file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${UNPC_VER_STRING}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/version.hpp @ONLY)

if (DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
    set(BUILDING_ON_GITHUB TRUE)
    add_compile_definitions(BUILDING_ON_GITHUB)
else ()
    set(BUILDING_ON_GITHUB FALSE)
endif ()
message(STATUS "Building on GitHub Actions: ${BUILDING_ON_GITHUB}")

include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
        GIT_TAG "e424e3f2e607da02742f73db84873b8084fc714c" # 12.0.0
        GIT_SHALLOW TRUE
        CXX_STANDARD 17
)
FetchContent_MakeAvailable(fmt)
target_compile_options(fmt PRIVATE "-w")

if (POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif ()

FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG c3fcafdc10146beb5919319d0683e44e3c30d537
        GIT_SHALLOW TRUE
)

FetchContent_GetProperties(minhook)
if (NOT minhook_POPULATED)
    FetchContent_Populate(minhook)
endif ()

add_library(minhook STATIC
        ${minhook_SOURCE_DIR}/src/buffer.c
        ${minhook_SOURCE_DIR}/src/hook.c
        ${minhook_SOURCE_DIR}/src/trampoline.c
        ${minhook_SOURCE_DIR}/src/hde/hde64.c
)

target_include_directories(minhook PUBLIC
        ${minhook_SOURCE_DIR}/include
)

# If compiled on GitHub Actions, fetch UnhideNPCs-RE
if (BUILDING_ON_GITHUB)
    FetchContent_Declare(
            UnhideNPCs_RE
            GIT_REPOSITORY "https://github.com/server-imp/UnhideNPCs-RE.git"
            GIT_TAG "main"
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(UnhideNPCs_RE)
endif ()

FetchContent_Declare(
        imgui_1800
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG "58075c4414b985b352d10718b02a8c43f25efd7c" # v1.80
        GIT_SHALLOW TRUE
        PATCH_COMMAND git apply --ignore-whitespace "${CMAKE_SOURCE_DIR}/patches/imgui-xinput-lowercase.patch"
)
FetchContent_MakeAvailable(imgui_1800)
add_library(imgui_1800 STATIC
        ${imgui_1800_SOURCE_DIR}/imgui.cpp
        ${imgui_1800_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_1800_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_1800_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_1800_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_1800_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_1800_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui_1800 PUBLIC ${imgui_1800_SOURCE_DIR})
target_compile_definitions(imgui_1800 PRIVATE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

FetchContent_Declare(
        imgui_19270
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG "dac07199cfd761113d966eb8ad739254e10df2fe" # v1.92.7
        GIT_SHALLOW TRUE
        #PATCH_COMMAND git apply --ignore-whitespace "${CMAKE_SOURCE_DIR}/patches/imgui-xinput-lowercase.patch"
)
FetchContent_MakeAvailable(imgui_19270)
add_library(imgui_19270 STATIC
        ${imgui_19270_SOURCE_DIR}/imgui.cpp
        ${imgui_19270_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_19270_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_19270_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_19270_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_19270_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_19270_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui_19270 PUBLIC ${imgui_19270_SOURCE_DIR})
target_compile_definitions(imgui_19270 PRIVATE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

FetchContent_Declare(
        Nexus
        GIT_REPOSITORY "https://github.com/RaidcoreGG/RCGG-lib-nexus-api.git"
        GIT_TAG "v6.1"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Nexus)

FetchContent_Declare(
        json
        GIT_REPOSITORY "https://github.com/nlohmann/json.git"
        GIT_TAG "55f93686c01528224f448c19128836e7df245f72" # 3.12.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
        zydis
        GIT_REPOSITORY "https://github.com/zyantific/zydis.git"
        GIT_TAG "a2278f1d254e492f6a6b39f6cb5d1f5d515659dc" # 4.1.1
)
FetchContent_MakeAvailable(zydis)

file(GLOB_RECURSE PROJECT_SOURCES
        "src/**.cpp"
)

function(add_unhidenpcs_dll TARGET_NAME IMGUI_TARGET OUTPUT_NAME)
    add_library(${TARGET_NAME} SHARED
            src/dllmain.cpp
            ${PROJECT_SOURCES}
            ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    )

    if(UNPC_IMGUI_VERSION)
        target_compile_definitions(${TARGET_NAME} PRIVATE UNPC_IMGUI_VERSION=${UNPC_IMGUI_VERSION})
    endif()

    target_compile_options(${TARGET_NAME} PRIVATE
            -Os
            -ffunction-sections
            -fdata-sections
            -fno-strict-aliasing
    )

    target_link_options(${TARGET_NAME} PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
            -Wl,--gc-sections
            "${CMAKE_SOURCE_DIR}/src/proxy/d3d9.def"
            "${CMAKE_SOURCE_DIR}/src/proxy/dxgi.def"
            "${CMAKE_SOURCE_DIR}/src/proxy/midimap.def"
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${OUTPUT_NAME}"
            SUFFIX ".dll"
    )

    if (BUILDING_ON_GITHUB)
        target_include_directories(${TARGET_NAME} PRIVATE
                "${UnhideNPCs_RE_SOURCE_DIR}"
        )
        file(GLOB SRC_RE "${UnhideNPCs_RE_SOURCE_DIR}/*.cpp")
    else()
        target_include_directories(${TARGET_NAME} PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/../UnhideNPCs-RE"
        )
        file(GLOB SRC_RE "${CMAKE_CURRENT_SOURCE_DIR}/../UnhideNPCs-RE/*.cpp")
    endif()

    target_sources(${TARGET_NAME} PRIVATE ${SRC_RE})

    target_precompile_headers(${TARGET_NAME} PRIVATE src/pch.hpp)

    target_include_directories(${TARGET_NAME} PRIVATE
            src
            "${fmt_SOURCE_DIR}"
            "${minhook_SOURCE_DIR}"
            "${nexus_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${json_SOURCE_DIR}/single_include"
            "${zydis_SOURCE_DIR}/include"
            "${zydis_SOURCE_DIR}/dependencies/zycore/include"
    )

    target_sources(${TARGET_NAME} PRIVATE ${SRC_RE})

    target_link_libraries(${TARGET_NAME} PRIVATE
            minhook
            fmt
            ${IMGUI_TARGET}
            d3d11
            dxgi
            d3dcompiler
            xinput
            gdi32
            user32
            dwmapi
            Zydis
    )

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${TARGET_NAME}>.debug
            COMMAND ${CMAKE_STRIP} --strip-debug $<TARGET_FILE:${TARGET_NAME}>
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:${TARGET_NAME}>.debug $<TARGET_FILE:${TARGET_NAME}>
    )
endfunction()

set(UNPC_IMGUI_VERSION 1800)
add_unhidenpcs_dll(
        UnhideNPCs
        imgui_1800
        UnhideNPCs
)

set(UNPC_IMGUI_VERSION 19270)
add_unhidenpcs_dll(
        UnhideNPCs_ArcDps
        imgui_19270
        UnhideNPCs_ArcDps
)