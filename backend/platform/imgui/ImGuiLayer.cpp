#include "ImGuiLayer.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>

#ifndef IM_OFFSETOF
#define IM_OFFSETOF(_TYPE,_MEMBER) offsetof(_TYPE, _MEMBER)
#endif

namespace {
ImGuiLayer* g_ActiveImGuiLayer = nullptr;

void ConfigureDefaultUiFont(ImGuiIO& io) {
    const std::array<const char*, 4> fontCandidates = {
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/msyhbd.ttc",
        "C:/Windows/Fonts/msyhbd.ttf"
    };

    ImFont* loadedFont = nullptr;
    constexpr float kFontSize = 24.0f;
    for (const char* candidate : fontCandidates) {
        if (!std::filesystem::exists(candidate)) {
            continue;
        }

        loadedFont = io.Fonts->AddFontFromFileTTF(
            candidate,
            kFontSize,
            nullptr,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon()
        );
        if (loadedFont != nullptr) {
            break;
        }
    }

    if (loadedFont != nullptr) {
        io.FontDefault = loadedFont;
    } else {
        io.Fonts->AddFontDefault();
    }
}

int KeyToImGuiKey(int key) {
    switch (key) {
    case GLFW_KEY_TAB: return ImGuiKey_Tab;
    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
    case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
    case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
    case GLFW_KEY_HOME: return ImGuiKey_Home;
    case GLFW_KEY_END: return ImGuiKey_End;
    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
    case GLFW_KEY_SPACE: return ImGuiKey_Space;
    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
    case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
    case GLFW_KEY_COMMA: return ImGuiKey_Comma;
    case GLFW_KEY_MINUS: return ImGuiKey_Minus;
    case GLFW_KEY_PERIOD: return ImGuiKey_Period;
    case GLFW_KEY_SLASH: return ImGuiKey_Slash;
    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
    case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
    case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
    case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
    case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
    case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
    case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
    case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
    case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
    case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
    case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
    case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
    case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
    case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
    case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
    case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
    case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
    case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
    case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
    case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
    case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
    case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
    case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
    case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
    case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
    case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
    case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
    case GLFW_KEY_MENU: return ImGuiKey_Menu;
    case GLFW_KEY_0: return ImGuiKey_0;
    case GLFW_KEY_1: return ImGuiKey_1;
    case GLFW_KEY_2: return ImGuiKey_2;
    case GLFW_KEY_3: return ImGuiKey_3;
    case GLFW_KEY_4: return ImGuiKey_4;
    case GLFW_KEY_5: return ImGuiKey_5;
    case GLFW_KEY_6: return ImGuiKey_6;
    case GLFW_KEY_7: return ImGuiKey_7;
    case GLFW_KEY_8: return ImGuiKey_8;
    case GLFW_KEY_9: return ImGuiKey_9;
    case GLFW_KEY_A: return ImGuiKey_A;
    case GLFW_KEY_B: return ImGuiKey_B;
    case GLFW_KEY_C: return ImGuiKey_C;
    case GLFW_KEY_D: return ImGuiKey_D;
    case GLFW_KEY_E: return ImGuiKey_E;
    case GLFW_KEY_F: return ImGuiKey_F;
    case GLFW_KEY_G: return ImGuiKey_G;
    case GLFW_KEY_H: return ImGuiKey_H;
    case GLFW_KEY_I: return ImGuiKey_I;
    case GLFW_KEY_J: return ImGuiKey_J;
    case GLFW_KEY_K: return ImGuiKey_K;
    case GLFW_KEY_L: return ImGuiKey_L;
    case GLFW_KEY_M: return ImGuiKey_M;
    case GLFW_KEY_N: return ImGuiKey_N;
    case GLFW_KEY_O: return ImGuiKey_O;
    case GLFW_KEY_P: return ImGuiKey_P;
    case GLFW_KEY_Q: return ImGuiKey_Q;
    case GLFW_KEY_R: return ImGuiKey_R;
    case GLFW_KEY_S: return ImGuiKey_S;
    case GLFW_KEY_T: return ImGuiKey_T;
    case GLFW_KEY_U: return ImGuiKey_U;
    case GLFW_KEY_V: return ImGuiKey_V;
    case GLFW_KEY_W: return ImGuiKey_W;
    case GLFW_KEY_X: return ImGuiKey_X;
    case GLFW_KEY_Y: return ImGuiKey_Y;
    case GLFW_KEY_Z: return ImGuiKey_Z;
    case GLFW_KEY_F1: return ImGuiKey_F1;
    case GLFW_KEY_F2: return ImGuiKey_F2;
    case GLFW_KEY_F3: return ImGuiKey_F3;
    case GLFW_KEY_F4: return ImGuiKey_F4;
    case GLFW_KEY_F5: return ImGuiKey_F5;
    case GLFW_KEY_F6: return ImGuiKey_F6;
    case GLFW_KEY_F7: return ImGuiKey_F7;
    case GLFW_KEY_F8: return ImGuiKey_F8;
    case GLFW_KEY_F9: return ImGuiKey_F9;
    case GLFW_KEY_F10: return ImGuiKey_F10;
    case GLFW_KEY_F11: return ImGuiKey_F11;
    case GLFW_KEY_F12: return ImGuiKey_F12;
    default: return ImGuiKey_None;
    }
}
}

bool ImGuiLayer::Init(GLFWwindow* window) {
    m_Window = window;
    g_ActiveImGuiLayer = this;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.BackendPlatformName = "CustomGLFW";
    io.BackendRendererName = "CustomOpenGL3";
    ConfigureDefaultUiFont(io);

    m_MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    m_MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    m_MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    m_MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCharCallback(window, CharCallback);

    return CreateDeviceObjects();
}

void ImGuiLayer::SetExternalScrollCallback(std::function<void(float, float)> callback) {
    m_ExternalScrollCallback = std::move(callback);
}

void ImGuiLayer::Shutdown() {
    DestroyDeviceObjects();

    for (void*& cursor : m_MouseCursors) {
        if (cursor != nullptr) {
            glfwDestroyCursor(static_cast<GLFWcursor*>(cursor));
            cursor = nullptr;
        }
    }

    ImGui::DestroyContext();
    g_ActiveImGuiLayer = nullptr;
    m_Window = nullptr;
}

void ImGuiLayer::BeginFrame() {
    ImGuiIO& io = ImGui::GetIO();

    int width = 0;
    int height = 0;
    int displayWidth = 0;
    int displayHeight = 0;
    glfwGetWindowSize(m_Window, &width, &height);
    glfwGetFramebufferSize(m_Window, &displayWidth, &displayHeight);

    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.DisplayFramebufferScale = ImVec2(
        width > 0 ? static_cast<float>(displayWidth) / static_cast<float>(width) : 1.0f,
        height > 0 ? static_cast<float>(displayHeight) / static_cast<float>(height) : 1.0f
    );

    const double currentTime = glfwGetTime();
    io.DeltaTime = m_Time > 0.0 ? static_cast<float>(currentTime - m_Time) : (1.0f / 60.0f);
    m_Time = currentTime;

    UpdateMouseData();
    UpdateMouseCursor();

    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame() {
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    const int fbWidth = static_cast<int>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    const int fbHeight = static_cast<int>(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fbWidth <= 0 || fbHeight <= 0) {
        return;
    }

    GLint lastProgram = 0;
    GLint lastTexture = 0;
    GLint lastArrayBuffer = 0;
    GLint lastVertexArray = 0;
    GLint lastBlendSrcRgb = 0;
    GLint lastBlendDstRgb = 0;
    GLint lastBlendSrcAlpha = 0;
    GLint lastBlendDstAlpha = 0;
    GLint lastBlendEquationRgb = 0;
    GLint lastBlendEquationAlpha = 0;
    GLint lastViewport[4] = {};
    GLboolean lastEnableBlend = glIsEnabled(GL_BLEND);
    GLboolean lastEnableCullFace = glIsEnabled(GL_CULL_FACE);
    GLboolean lastEnableDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean lastEnableScissorTest = glIsEnabled(GL_SCISSOR_TEST);

    glGetIntegerv(GL_CURRENT_PROGRAM, &lastProgram);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVertexArray);
    glGetIntegerv(GL_BLEND_SRC_RGB, &lastBlendSrcRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &lastBlendDstRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &lastBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &lastBlendDstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &lastBlendEquationRgb);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &lastBlendEquationAlpha);
    glGetIntegerv(GL_VIEWPORT, lastViewport);

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glViewport(0, 0, fbWidth, fbHeight);

    const float left = drawData->DisplayPos.x;
    const float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
    const float top = drawData->DisplayPos.y;
    const float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;
    const float orthoProjection[4][4] = {
        { 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { (right + left) / (left - right), (top + bottom) / (bottom - top), 0.0f, 1.0f }
    };

    glUseProgram(m_ShaderHandle);
    glUniform1i(m_AttribLocationTex, 0);
    glUniformMatrix4fv(m_AttribLocationProjMtx, 1, GL_FALSE, &orthoProjection[0][0]);
    glBindVertexArray(m_VaoHandle);

    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        glBindBuffer(GL_ARRAY_BUFFER, m_VboHandle);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmdList->VtxBuffer.Size * sizeof(ImDrawVert)), cmdList->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmdList->IdxBuffer.Size * sizeof(ImDrawIdx)), cmdList->IdxBuffer.Data, GL_STREAM_DRAW);

        int idxOffset = 0;
        for (const ImDrawCmd& drawCommand : cmdList->CmdBuffer) {
            if (drawCommand.UserCallback != nullptr) {
                drawCommand.UserCallback(cmdList, &drawCommand);
            } else {
                const ImVec2 clipMin(
                    (drawCommand.ClipRect.x - drawData->DisplayPos.x) * drawData->FramebufferScale.x,
                    (drawCommand.ClipRect.y - drawData->DisplayPos.y) * drawData->FramebufferScale.y
                );
                const ImVec2 clipMax(
                    (drawCommand.ClipRect.z - drawData->DisplayPos.x) * drawData->FramebufferScale.x,
                    (drawCommand.ClipRect.w - drawData->DisplayPos.y) * drawData->FramebufferScale.y
                );

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
                    idxOffset += drawCommand.ElemCount;
                    continue;
                }

                glScissor(
                    static_cast<int>(clipMin.x),
                    static_cast<int>(fbHeight - clipMax.y),
                    static_cast<int>(clipMax.x - clipMin.x),
                    static_cast<int>(clipMax.y - clipMin.y)
                );

                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>((intptr_t)drawCommand.GetTexID()));
                const GLenum indexType = sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(drawCommand.ElemCount), indexType, reinterpret_cast<const void*>(static_cast<intptr_t>(idxOffset * sizeof(ImDrawIdx))));
            }

            idxOffset += drawCommand.ElemCount;
        }
    }

    glUseProgram(lastProgram);
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    glBindVertexArray(lastVertexArray);
    glBlendEquationSeparate(lastBlendEquationRgb, lastBlendEquationAlpha);
    glBlendFuncSeparate(lastBlendSrcRgb, lastBlendDstRgb, lastBlendSrcAlpha, lastBlendDstAlpha);
    if (lastEnableBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (lastEnableCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (lastEnableDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (lastEnableScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glViewport(lastViewport[0], lastViewport[1], lastViewport[2], lastViewport[3]);
}

bool ImGuiLayer::CreateDeviceObjects() {
    static const char* vertexShaderSource =
        "#version 330 core\n"
        "uniform mat4 ProjMtx;\n"
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";

    static const char* fragmentShaderSource =
        "#version 330 core\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main() {\n"
        "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    m_VertHandle = glCreateShader(GL_VERTEX_SHADER);
    m_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_VertHandle, 1, &vertexShaderSource, nullptr);
    glShaderSource(m_FragHandle, 1, &fragmentShaderSource, nullptr);
    glCompileShader(m_VertHandle);
    glCompileShader(m_FragHandle);

    m_ShaderHandle = glCreateProgram();
    glAttachShader(m_ShaderHandle, m_VertHandle);
    glAttachShader(m_ShaderHandle, m_FragHandle);
    glLinkProgram(m_ShaderHandle);

    m_AttribLocationTex = glGetUniformLocation(m_ShaderHandle, "Texture");
    m_AttribLocationProjMtx = glGetUniformLocation(m_ShaderHandle, "ProjMtx");
    m_AttribLocationVtxPos = 0;
    m_AttribLocationVtxUV = 1;
    m_AttribLocationVtxColor = 2;

    glGenBuffers(1, &m_VboHandle);
    glGenBuffers(1, &m_ElementsHandle);
    glGenVertexArrays(1, &m_VaoHandle);

    glBindVertexArray(m_VaoHandle);
    glBindBuffer(GL_ARRAY_BUFFER, m_VboHandle);
    glEnableVertexAttribArray(m_AttribLocationVtxPos);
    glEnableVertexAttribArray(m_AttribLocationVtxUV);
    glEnableVertexAttribArray(m_AttribLocationVtxColor);
    glVertexAttribPointer(m_AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)IM_OFFSETOF(ImDrawVert, pos));
    glVertexAttribPointer(m_AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (void*)IM_OFFSETOF(ImDrawVert, uv));
    glVertexAttribPointer(m_AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (void*)IM_OFFSETOF(ImDrawVert, col));

    return CreateFontsTexture();
}

void ImGuiLayer::DestroyDeviceObjects() {
    DestroyFontsTexture();

    if (m_VaoHandle != 0) {
        glDeleteVertexArrays(1, &m_VaoHandle);
        m_VaoHandle = 0;
    }
    if (m_VboHandle != 0) {
        glDeleteBuffers(1, &m_VboHandle);
        m_VboHandle = 0;
    }
    if (m_ElementsHandle != 0) {
        glDeleteBuffers(1, &m_ElementsHandle);
        m_ElementsHandle = 0;
    }
    if (m_ShaderHandle != 0 && m_VertHandle != 0) {
        glDetachShader(m_ShaderHandle, m_VertHandle);
    }
    if (m_ShaderHandle != 0 && m_FragHandle != 0) {
        glDetachShader(m_ShaderHandle, m_FragHandle);
    }
    if (m_VertHandle != 0) {
        glDeleteShader(m_VertHandle);
        m_VertHandle = 0;
    }
    if (m_FragHandle != 0) {
        glDeleteShader(m_FragHandle);
        m_FragHandle = 0;
    }
    if (m_ShaderHandle != 0) {
        glDeleteProgram(m_ShaderHandle);
        m_ShaderHandle = 0;
    }
}

bool ImGuiLayer::CreateFontsTexture() {
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    glGenTextures(1, &m_FontTexture);
    glBindTexture(GL_TEXTURE_2D, m_FontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    io.Fonts->SetTexID((ImTextureID)(intptr_t)m_FontTexture);
    return true;
}

void ImGuiLayer::DestroyFontsTexture() {
    if (m_FontTexture != 0) {
        ImGui::GetIO().Fonts->SetTexID(0);
        glDeleteTextures(1, &m_FontTexture);
        m_FontTexture = 0;
    }
}

void ImGuiLayer::UpdateMouseData() {
    ImGuiIO& io = ImGui::GetIO();

    for (int i = 0; i < 5; ++i) {
        io.AddMouseButtonEvent(i, m_MousePressed[i] || glfwGetMouseButton(m_Window, i) == GLFW_PRESS);
        m_MousePressed[i] = false;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(m_Window, &mouseX, &mouseY);
    io.AddMousePosEvent(static_cast<float>(mouseX), static_cast<float>(mouseY));
}

void ImGuiLayer::UpdateMouseCursor() const {
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;
    }

    const ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
    if (imguiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    } else {
        glfwSetCursor(m_Window, static_cast<GLFWcursor*>(m_MouseCursors[imguiCursor] != nullptr ? m_MouseCursors[imguiCursor] : m_MouseCursors[ImGuiMouseCursor_Arrow]));
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void ImGuiLayer::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    if (action == GLFW_PRESS) {
        if (ImGuiLayer* layer = g_ActiveImGuiLayer) {
            if (button >= 0 && button < 5) {
                layer->m_MousePressed[button] = true;
            }
        }
    }
}

void ImGuiLayer::ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(xOffset), static_cast<float>(yOffset));
    if (ImGuiLayer* layer = g_ActiveImGuiLayer) {
        if (layer->m_ExternalScrollCallback) {
            layer->m_ExternalScrollCallback(static_cast<float>(xOffset), static_cast<float>(yOffset));
        }
    }
}

void ImGuiLayer::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    const ImGuiKey imguiKey = static_cast<ImGuiKey>(KeyToImGuiKey(key));
    if (imguiKey != ImGuiKey_None) {
        io.AddKeyEvent(imguiKey, action != GLFW_RELEASE);
    }

    io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
}

void ImGuiLayer::CharCallback(GLFWwindow* window, unsigned int c) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}
