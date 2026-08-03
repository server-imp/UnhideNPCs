cmake_minimum_required(VERSION 3.31)
set(PROJECT_NAME "UnhideNPCs")
project(${PROJECT_NAME} LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

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

# If compiled on GitHub Actions, fetch UnhideNPCs-RE
if (BUILDING_ON_GITHUB)
    FetchContent_Declare(
            UnhideNPCs_RE
            GIT_REPOSITORY "https://github.com/server-imp/UnhideNPCs-RE.git"
            GIT_TAG "main"
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(UnhideNPCs_RE)
    FetchContent_Declare(
            fw
            GIT_REPOSITORY "https://github.com/server-imp/fw.git"
            GIT_TAG "master"
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(fw)
else ()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../fw ${CMAKE_BINARY_DIR}/_deps/fw-build)
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

file(GLOB_RECURSE PROJECT_SOURCES
        "src/**.cpp"
)

function(add_unhidenpcs_dll TARGET_NAME IMGUI_TARGET OUTPUT_NAME)
    add_library(${TARGET_NAME} SHARED
            src/dllmain.cpp
            ${PROJECT_SOURCES}
            ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    )

    if (UNPC_IMGUI_VERSION)
        target_compile_definitions(${TARGET_NAME} PRIVATE UNPC_IMGUI_VERSION=${UNPC_IMGUI_VERSION})
    endif ()

    target_compile_options(${TARGET_NAME} PRIVATE
            -O2
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${TARGET_NAME} PRIVATE
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
    endif ()

    if (MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
                /O2
                /Ob2
                /Oi
                /Ot
                /GL
                /Gw
                /Gy
                /Zi
        )

        target_link_options(${TARGET_NAME} PRIVATE
                /LTCG
                /OPT:REF
                /OPT:ICF
                /DEBUG
        )
    endif ()

    set_target_properties(${TARGET_NAME} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${OUTPUT_NAME}"
            SUFFIX ".dll"
    )

    if (BUILDING_ON_GITHUB)
        target_include_directories(${TARGET_NAME} PRIVATE
                ${UnhideNPCs_RE_SOURCE_DIR}/src
                ${fw_SOURCE_DIR}/include
        )
        file(GLOB SRC_RE "${UnhideNPCs_RE_SOURCE_DIR}/src/*.cpp")
    else ()
        target_include_directories(${TARGET_NAME} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/../UnhideNPCs-RE/src
        )
        file(GLOB SRC_RE "${CMAKE_CURRENT_SOURCE_DIR}/../UnhideNPCs-RE/src/*.cpp")
    endif ()

    target_sources(${TARGET_NAME} PRIVATE ${SRC_RE})

    target_precompile_headers(${TARGET_NAME} PRIVATE src/pch.hpp)

    target_include_directories(${TARGET_NAME} PRIVATE
            src
            ${nexus_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}
            ${fw_SOURCE_DIR}/include
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
            ${IMGUI_TARGET}
            fw
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${TARGET_NAME}> $<TARGET_FILE:${TARGET_NAME}>.debug
                COMMAND ${CMAKE_STRIP} --strip-debug $<TARGET_FILE:${TARGET_NAME}>
                COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:${TARGET_NAME}>.debug $<TARGET_FILE:${TARGET_NAME}>
        )
    endif ()
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