cmake_minimum_required(VERSION 3.31)
set(PROJECT_NAME "UnhideNPCs")
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

string(TIMESTAMP UNPC_VER_YEAR "%Y")
string(TIMESTAMP UNPC_VER_MONTH "%m")
string(TIMESTAMP UNPC_VER_DAY "%d")
set(UNPC_VER_BUILD "1")
set(UNPC_VER_STRING "${UNPC_VER_YEAR}.${UNPC_VER_MONTH}.${UNPC_VER_DAY}.${UNPC_VER_BUILD}")

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
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.80
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

# change XInput to xinput because Linux is case sensitive and MingW provides xinput not XInput
set(IMGUI_WIN32_BACKEND "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp")
file(READ "${IMGUI_WIN32_BACKEND}" IMGUI_WIN32_CONTENT)
string(REPLACE "#include <XInput.h>" "#include <xinput.h>" IMGUI_WIN32_CONTENT "${IMGUI_WIN32_CONTENT}")
file(WRITE "${IMGUI_WIN32_BACKEND}" "${IMGUI_WIN32_CONTENT}")

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

add_library(${PROJECT_NAME} SHARED src/dllmain.cpp ${PROJECT_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Os
        -ffunction-sections
        -fdata-sections
        -fno-strict-aliasing
)

target_link_options(${PROJECT_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
        -Wl,--gc-sections
        "${CMAKE_SOURCE_DIR}/src/proxy/d3d9.def"
        "${CMAKE_SOURCE_DIR}/src/proxy/dxgi.def"
        "${CMAKE_SOURCE_DIR}/src/proxy/midimap.def"
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.hpp")
target_include_directories(${PROJECT_NAME} PRIVATE
        "src"
        "${fmt_SOURCE_DIR}"
        "${minhook_SOURCE_DIR}"
        "${nexus_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

if (BUILDING_ON_GITHUB)
    target_include_directories(${PROJECT_NAME} PRIVATE "${UnhideNPCs_RE_SOURCE_DIR}")
    file(GLOB SRC_RE "${UnhideNPCs_RE_SOURCE_DIR}/*.cpp")
else ()
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../UnhideNPCs-RE")
    file(GLOB SRC_RE "${CMAKE_CURRENT_SOURCE_DIR}/../UnhideNPCs-RE/*.cpp")
endif ()
target_sources(${PROJECT_NAME} PRIVATE ${SRC_RE})

target_link_libraries(${PROJECT_NAME} PRIVATE
        minhook
        fmt
        imgui
        d3d11
        dxgi
        d3dcompiler
        xinput
        gdi32
        user32)

#add_executable(Injector src/injector.cpp)

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#COMMAND ${CMAKE_COMMAND} -E copy_if_different
#$<TARGET_FILE:${PROJECT_NAME}>
#"C:/Users/user/Nextcloud/user files/Guild Wars 2/UnhideNPCs.dll"
#COMMAND ${CMAKE_COMMAND} -E copy_if_different
#$<TARGET_FILE:${PROJECT_NAME}>
#"C:/Program Files/Guild Wars 2/addons/UnhideNPCs.dll"
#COMMAND ${CMAKE_COMMAND} -E copy_if_different
#$<TARGET_FILE:${PROJECT_NAME}>
#"C:/Program Files (x86)/Steam/steamapps/common/Guild Wars 2/d33d9.dll"
#)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${PROJECT_NAME}> $<TARGET_FILE:${PROJECT_NAME}>.debug
        COMMAND ${CMAKE_STRIP} --strip-debug $<TARGET_FILE:${PROJECT_NAME}>
        COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:${PROJECT_NAME}>.debug $<TARGET_FILE:${PROJECT_NAME}>
)