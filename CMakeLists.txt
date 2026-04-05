cmake_minimum_required(VERSION 3.16)
project(SDLTest)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,
        $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,
        $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3 / SDL3_image
add_subdirectory(external/SDL)

set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/SDL_image)

# 头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/json-develop/single_include
)

# 可执行程序
add_executable(SDLTest
    # 主文件
    main.cpp

    # backend 文件夹（你最新的路径）
    backend/core/Engine.cpp
    backend/window/WindowManager.cpp
    backend/render/Renderer2D.cpp
    backend/resource/TextureManager.cpp
    backend/input/InputManager.cpp
    backend/core/GameLoop.cpp
    backend/project/ProjectManager.cpp
    backend/resource/ResourceManager.cpp
    backend/resource/AssetRegistry.cpp
    backend/resource/ResourcePathUtils.cpp
    backend/resource/Sprite.cpp

    # frontend 编辑器
    frontend/src/editor/EditorUI.cpp
    frontend/src/editor/Panels/ScenePanel.cpp
    frontend/src/editor/Panels/AssetPanel.cpp
    frontend/src/editor/Panels/HierarchyPanel.cpp 
    frontend/src/editor/Panels/InspectorPanel.cpp 

    # ImGui 核心 【正确写法：一行一个，不要引号！】
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp


    # ImGui SDL3 后端
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_sdlrenderer3.cpp
 "frontend/src/editor/Panels/ConsolePanel.h" "frontend/src/editor/Panels/ConsolePanel.cpp" "backend/SceneSerializer.h" "backend/SceneSerializer.cpp")

# 链接库
target_link_libraries(SDLTest PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    Ole32
)
# 复制 asset 文件夹到 exe 目录
add_custom_command(TARGET SDLTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/asset
        $<TARGET_FILE_DIR:SDLTest>/asset
)

# 自动复制 SDL3.dll 到 exe 目录
add_custom_command(TARGET SDLTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:SDLTest>
)

# 自动复制 SDL3_image.dll 到 exe 目录
add_custom_command(TARGET SDLTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3_image::SDL3_image>
        $<TARGET_FILE_DIR:SDLTest>
)
