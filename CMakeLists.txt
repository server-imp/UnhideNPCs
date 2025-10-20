cmake_minimum_required(VERSION 3.31)
set(PROJECT_NAME "UnhideNPCs")
project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

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

FetchContent_Declare(
        minhook
        GIT_REPOSITORY "https://github.com/TsudaKageyu/minhook.git"
        GIT_TAG "c3fcafdc10146beb5919319d0683e44e3c30d537" # 1.3.4
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(minhook)

FetchContent_Declare(
        UnhideNPCs_RE
        GIT_REPOSITORY "https://github.com/server-imp/UnhideNPCs-RE.git"
        GIT_TAG "main"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(UnhideNPCs_RE)

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
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

FetchContent_Declare(
        Nexus
        GIT_REPOSITORY "https://github.com/RaidcoreGG/RCGG-lib-nexus-api.git"
        GIT_TAG "v6.1"
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Nexus)

file(GLOB_RECURSE PROJECT_SOURCES "src/**.cpp")

add_library(${PROJECT_NAME} SHARED src/dllmain.cpp ${PROJECT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".dll")
target_precompile_headers(${PROJECT_NAME} PRIVATE "src/pch.hpp")
target_include_directories(${PROJECT_NAME} PRIVATE
        "src"
        "${fmt_SOURCE_DIR}"
        "${minhook_SOURCE_DIR}"
        "${nexus_SOURCE_DIR}"
        "${UnhideNPCs_RE_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME} PRIVATE minhook fmt imgui)

add_executable(Injector src/injector.cpp)

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#        $<TARGET_FILE:${PROJECT_NAME}>
#        "C:/Program Files/Guild Wars 2/dxgi.dll"
#)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        "C:/Program Files/Guild Wars 2/addons/UnhideNPCs.dll"
)