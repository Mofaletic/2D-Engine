cmake_minimum_required(VERSION 3.16)
project(LancelotEngine LANGUAGES C CXX)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,
        $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,
        $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LANCELOT_ENABLE_PROFILING "Enable lightweight CPU instrumentation and trace output" ON)

find_package(OpenGL REQUIRED)

if (EXISTS "${CMAKE_SOURCE_DIR}/external/SDL/CMakeLists.txt")
    add_subdirectory(external/SDL)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/external/SDL_image/CMakeLists.txt")
    add_subdirectory(external/SDL_image)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    add_subdirectory(external/glfw)
endif()

if (EXISTS "${CMAKE_SOURCE_DIR}/external/glad/CMakeLists.txt")
    add_subdirectory(external/glad)
elseif (EXISTS "${CMAKE_SOURCE_DIR}/external/glad/src/glad.c")
    add_library(glad STATIC external/glad/src/glad.c)
    target_include_directories(glad PUBLIC external/glad/include)
endif()

if (TARGET glfw)
    set(LANCELOT_GLFW_TARGET glfw)
elseif (TARGET glfw3::glfw)
    set(LANCELOT_GLFW_TARGET glfw3::glfw)
else()
    find_package(glfw3 REQUIRED)
    if (TARGET glfw)
        set(LANCELOT_GLFW_TARGET glfw)
    elseif (TARGET glfw3::glfw)
        set(LANCELOT_GLFW_TARGET glfw3::glfw)
    else()
        message(FATAL_ERROR "GLFW target was not found. Provide external/glfw or install glfw3.")
    endif()
endif()

if (TARGET glad)
    set(LANCELOT_GLAD_TARGET glad)
elseif (TARGET glad::glad)
    set(LANCELOT_GLAD_TARGET glad::glad)
else()
    find_package(glad CONFIG REQUIRED)
    if (TARGET glad)
        set(LANCELOT_GLAD_TARGET glad)
    elseif (TARGET glad::glad)
        set(LANCELOT_GLAD_TARGET glad::glad)
    else()
        message(FATAL_ERROR "GLAD target was not found. Provide external/glad or install glad.")
    endif()
endif()

add_executable(LancelotEngine
    main.cpp
    backend/core/Engine.cpp
    backend/core/GameLoop.cpp
    backend/window/WindowManager.cpp
    backend/input/InputManager.cpp
    backend/render/GraphicsContext.cpp
    backend/render/RendererAPI.cpp
    backend/render/RenderCommand.cpp
    backend/render/Renderer.cpp
    backend/render/Renderer2D.cpp
    backend/render/OrthographicCamera.cpp
    backend/render/Material.cpp
    backend/render/ShaderLibrary.cpp
    backend/render/Texture.cpp
    backend/render/Buffer.cpp
    backend/render/VertexArray.cpp
    backend/render/Shader.cpp
    backend/platform/imgui/ImGuiLayer.cpp
    backend/platform/opengl/OpenGLContext.cpp
    backend/platform/opengl/OpenGLRendererAPI.cpp
    backend/platform/opengl/OpenGLBuffer.cpp
    backend/platform/opengl/OpenGLVertexArray.cpp
    backend/platform/opengl/OpenGLShader.cpp
    backend/platform/opengl/OpenGLTexture.cpp
    backend/resource/AssetRegistry.cpp
    backend/resource/ResourceManager.cpp
    backend/resource/ResourcePathUtils.cpp
    backend/project/ProjectManager.cpp
    backend/script/ScriptRuntime.cpp
    backend/SceneSerializer.cpp
    frontend/src/editor/EditorUI.cpp
    frontend/src/editor/Panels/AssetPanel.cpp
    frontend/src/editor/Panels/ConsolePanel.cpp
    frontend/src/editor/Panels/HierarchyPanel.cpp
    frontend/src/editor/Panels/InspectorPanel.cpp
    frontend/src/editor/Panels/ScenePanel.cpp
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
)

target_include_directories(LancelotEngine PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/backend
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/SDL/include
    ${CMAKE_SOURCE_DIR}/external/SDL_image/include
    ${CMAKE_SOURCE_DIR}/external/json-develop/single_include
)

target_link_libraries(LancelotEngine PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    ${LANCELOT_GLFW_TARGET}
    ${LANCELOT_GLAD_TARGET}
    OpenGL::GL
)

if (MSVC)
    target_compile_definitions(LancelotEngine PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if (LANCELOT_ENABLE_PROFILING)
    target_compile_definitions(LancelotEngine PRIVATE LANCELOT_PROFILE_ENABLED=1)
else()
    target_compile_definitions(LancelotEngine PRIVATE LANCELOT_PROFILE_ENABLED=0)
endif()
