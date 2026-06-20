#include "GLUiRenderer.h"
#include <KrisLogger/Logger.h>
#include <cstddef>

namespace RetroRenderer {
namespace {

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
constexpr const char* kShaderVersion = "#version 300 es\n";
constexpr const char* kVertexPrecision = "precision highp float;\n";
constexpr const char* kFragmentPrecision = "precision mediump float;\n";
#elif defined(__APPLE__)
constexpr const char* kShaderVersion = "#version 150\n";
constexpr const char* kVertexPrecision = "";
constexpr const char* kFragmentPrecision = "";
#else
constexpr const char* kShaderVersion = "#version 330 core\n";
constexpr const char* kVertexPrecision = "";
constexpr const char* kFragmentPrecision = "";
#endif

constexpr const char* kVertexShader = R"(
uniform mat4 u_Projection;
in vec2 a_Position;
in vec2 a_Uv;
in vec4 a_Color;
out vec2 v_Uv;
out vec4 v_Color;
void main() {
    v_Uv = a_Uv;
    v_Color = a_Color;
    gl_Position = u_Projection * vec4(a_Position, 0.0, 1.0);
}
)";

constexpr const char* kFragmentShader = R"(
uniform sampler2D u_Texture;
in vec2 v_Uv;
in vec4 v_Color;
out vec4 o_Color;
void main() {
    o_Color = v_Color * texture(u_Texture, v_Uv);
}
)";

bool CompileShader(GLuint shader, const char* precision, const char* source) {
    const char* sources[] = {kShaderVersion, precision, source};
    glShaderSource(shader, 3, sources, nullptr);
    glCompileShader(shader);
    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) {
        return true;
    }

    GLchar message[1024]{};
    glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
    LOGE("Failed to compile detached ImGui shader: %s", message);
    return false;
}

} // namespace

bool GLUiRenderer::Init() {
    Destroy();
    return CreateDeviceObjects();
}

bool GLUiRenderer::CreateDeviceObjects() {
    m_VertexShader = glCreateShader(GL_VERTEX_SHADER);
    m_FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!CompileShader(m_VertexShader, kVertexPrecision, kVertexShader) ||
        !CompileShader(m_FragmentShader, kFragmentPrecision, kFragmentShader)) {
        Destroy();
        return false;
    }

    m_ShaderProgram = glCreateProgram();
    glAttachShader(m_ShaderProgram, m_VertexShader);
    glAttachShader(m_ShaderProgram, m_FragmentShader);
    glBindAttribLocation(m_ShaderProgram, 0, "a_Position");
    glBindAttribLocation(m_ShaderProgram, 1, "a_Uv");
    glBindAttribLocation(m_ShaderProgram, 2, "a_Color");
    glLinkProgram(m_ShaderProgram);
    GLint status = GL_FALSE;
    glGetProgramiv(m_ShaderProgram, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLchar message[1024]{};
        glGetProgramInfoLog(m_ShaderProgram, sizeof(message), nullptr, message);
        LOGE("Failed to link detached ImGui program: %s", message);
        Destroy();
        return false;
    }

    m_TextureLocation = glGetUniformLocation(m_ShaderProgram, "u_Texture");
    m_ProjectionLocation = glGetUniformLocation(m_ShaderProgram, "u_Projection");
    glGenBuffers(1, &m_VertexBuffer);
    glGenBuffers(1, &m_IndexBuffer);
    glGenVertexArrays(1, &m_VertexArray);
    glBindVertexArray(m_VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<void*>(offsetof(ImDrawVert, pos)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<void*>(offsetof(ImDrawVert, uv)));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), reinterpret_cast<void*>(offsetof(ImDrawVert, col)));
    glBindVertexArray(0);
    return true;
}

void GLUiRenderer::SetupRenderState(const UiRenderPacket& packet, int framebufferWidth, int framebufferHeight) {
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    const float left = packet.displayPosition.x;
    const float right = packet.displayPosition.x + packet.displaySize.x;
    const float top = packet.displayPosition.y;
    const float bottom = packet.displayPosition.y + packet.displaySize.y;
    const float projection[4][4] = {
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(right + left) / (left - right), (top + bottom) / (bottom - top), 0.0f, 1.0f},
    };

    glUseProgram(m_ShaderProgram);
    glUniform1i(m_TextureLocation, 0);
    glUniformMatrix4fv(m_ProjectionLocation, 1, GL_FALSE, &projection[0][0]);
    glBindVertexArray(m_VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
}

void GLUiRenderer::Render(const UiRenderPacket& packet, const TextureResolver& resolveTexture) {
    if (!packet.valid || packet.displaySize.x <= 0.0f || packet.displaySize.y <= 0.0f) {
        return;
    }
    const int framebufferWidth = static_cast<int>(packet.displaySize.x * packet.framebufferScale.x);
    const int framebufferHeight = static_cast<int>(packet.displaySize.y * packet.framebufferScale.y);
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    SetupRenderState(packet, framebufferWidth, framebufferHeight);

    const ImVec2 clipOffset = packet.displayPosition;
    const ImVec2 clipScale = packet.framebufferScale;
    for (const UiDrawList& list : packet.drawLists) {
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(list.vertices.size() * sizeof(ImDrawVert)),
                     list.vertices.data(),
                     GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(list.indices.size() * sizeof(ImDrawIdx)),
                     list.indices.data(),
                     GL_STREAM_DRAW);

        for (const UiDrawCommand& command : list.commands) {
            if (command.resetRenderState) {
                SetupRenderState(packet, framebufferWidth, framebufferHeight);
                continue;
            }

            const ImVec4 clipRect{
                (command.clipRect.x - clipOffset.x) * clipScale.x,
                (command.clipRect.y - clipOffset.y) * clipScale.y,
                (command.clipRect.z - clipOffset.x) * clipScale.x,
                (command.clipRect.w - clipOffset.y) * clipScale.y,
            };
            if (clipRect.x >= framebufferWidth || clipRect.y >= framebufferHeight || clipRect.z <= 0.0f ||
                clipRect.w <= 0.0f) {
                continue;
            }

            glScissor(static_cast<int>(clipRect.x),
                      static_cast<int>(framebufferHeight - clipRect.w),
                      static_cast<int>(clipRect.z - clipRect.x),
                      static_cast<int>(clipRect.w - clipRect.y));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, resolveTexture(command.texture));
            const void* indexOffset =
                reinterpret_cast<void*>(static_cast<uintptr_t>(command.indexOffset * sizeof(ImDrawIdx)));
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
            glDrawElements(GL_TRIANGLES,
                           static_cast<GLsizei>(command.elementCount),
                           sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                           indexOffset);
#else
            glDrawElementsBaseVertex(GL_TRIANGLES,
                                     static_cast<GLsizei>(command.elementCount),
                                     sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                     indexOffset,
                                     static_cast<GLint>(command.vertexOffset));
#endif
        }
    }

    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GLUiRenderer::Destroy() {
    if (m_VertexBuffer != 0) {
        glDeleteBuffers(1, &m_VertexBuffer);
        m_VertexBuffer = 0;
    }
    if (m_IndexBuffer != 0) {
        glDeleteBuffers(1, &m_IndexBuffer);
        m_IndexBuffer = 0;
    }
    if (m_VertexArray != 0) {
        glDeleteVertexArrays(1, &m_VertexArray);
        m_VertexArray = 0;
    }
    if (m_ShaderProgram != 0) {
        glDeleteProgram(m_ShaderProgram);
        m_ShaderProgram = 0;
    }
    if (m_VertexShader != 0) {
        glDeleteShader(m_VertexShader);
        m_VertexShader = 0;
    }
    if (m_FragmentShader != 0) {
        glDeleteShader(m_FragmentShader);
        m_FragmentShader = 0;
    }
}

} // namespace RetroRenderer
