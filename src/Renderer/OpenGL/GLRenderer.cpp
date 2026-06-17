#include "GLRenderer.h"

#include "../GridGizmo.h"
#include "../../Base/Debug.h"
#include <KrisLogger/Logger.h>
#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace RetroRenderer {

void APIENTRY GLRenderer::DebugCallback(GLenum source,
                                        GLenum type,
                                        GLuint id,
                                        GLenum severity,
                                        GLsizei length,
                                        const GLchar* message,
                                        const void* userParam) {
    (void)source;
    (void)id;
    (void)length;
    (void)userParam;

    if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOGW("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
             type,
             severity,
             message);
    } else if (severity == GL_DEBUG_SEVERITY_HIGH) {
        LOGE("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
             type,
             severity,
             message);
    } else {
        LOGD("GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s",
             (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
             type,
             severity,
             message);
    }
#ifndef NDEBUG
    DEBUG_BREAK();
#endif
}

bool GLRenderer::Init(TextureHandle fbTex, int w, int h) {
    return InitializeBackend(fbTex, w, h, "assets/img/skybox-cubemap/Cubemap_Sky_23-512x512.png");
}

std::string_view GLRenderer::GetShaderPrefix() const {
    return "#version 330 core\nprecision highp float;\n";
}

const char* GLRenderer::GetRendererLogLabel() const {
    return "GLRenderer";
}

void GLRenderer::ConfigureBackendDebugState() {
    if (glDebugMessageCallback == nullptr) {
        LOGW("glDebugMessageCallback not supported on this platform");
        return;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(DebugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
    LOGD("OpenGL debug context initialized");
}

void GLRenderer::InitializeBackendResources() {
    constexpr const char* kGridVertexShader =
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 u_MVP;\n"
        "out vec3 vColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = u_MVP * vec4(aPos, 1.0);\n"
        "   vColor = aColor;\n"
        "}\n";
    constexpr const char* kGridFragmentShader =
        "in vec3 vColor;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(vColor, 1.0);\n"
        "}\n";

    m_GridProgram = CompileShaders(kGridVertexShader, kGridFragmentShader);
    if (!m_GridProgram.IsValid()) {
        return;
    }

    struct GridVertexGpu {
        glm::vec3 position;
        glm::vec3 color;
    };

    glGenVertexArrays(1, &m_GridVAO);
    glGenBuffers(1, &m_GridVBO);
    glBindVertexArray(m_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GridVertexGpu),
        reinterpret_cast<void*>(offsetof(GridVertexGpu, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GridVertexGpu),
        reinterpret_cast<void*>(offsetof(GridVertexGpu, color)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void GLRenderer::DestroyBackendResources() {
    if (m_GridVBO != 0) {
        glDeleteBuffers(1, &m_GridVBO);
        m_GridVBO = 0;
    }
    if (m_GridVAO != 0) {
        glDeleteVertexArrays(1, &m_GridVAO);
        m_GridVAO = 0;
    }
    if (m_GridProgram.IsValid()) {
        glDeleteProgram(ToGLHandle(m_GridProgram));
        m_GridProgram = {};
    }
}

void GLRenderer::ApplyBackendFrameState() {
    switch (m_FrameConfigSnapshot.gl.rasterizer.polygonMode) {
    case Config::RasterizationPolygonMode::POINT:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case Config::RasterizationPolygonMode::LINE:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case Config::RasterizationPolygonMode::FILL:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }
}

void GLRenderer::ResetBackendFrameState() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GLRenderer::RenderBackendOverlays() {
    if (m_FrameConfigSnapshot.environment.showGrid) {
        DrawGridGizmo();
    }
}

void GLRenderer::DrawGridGizmo() {
    if (m_ActiveCamera == nullptr || !m_GridProgram.IsValid() || m_GridVAO == 0 || m_GridVBO == 0) {
        return;
    }

    const GLuint gridProgram = ToGLHandle(m_GridProgram);
    const auto& uniforms = m_UniformCache.GetGridProgram(gridProgram);

    struct GridVertexGpu {
        glm::vec3 position;
        glm::vec3 color;
    };

    const std::vector<GridGizmoVertex> gridVertices = BuildGridGizmoVertices(m_ActiveCamera->m_Position);
    std::vector<GridVertexGpu> packedVertices;
    packedVertices.reserve(gridVertices.size());
    for (const GridGizmoVertex& vertex : gridVertices) {
        packedVertices.push_back({vertex.position, vertex.color});
    }

    const GLsizei vertexCount = static_cast<GLsizei>(packedVertices.size());
    if (vertexCount == 0) {
        return;
    }

    glUseProgram(gridProgram);
    const glm::mat4 mvp = m_ActiveCamera->m_ProjMat * m_ActiveCamera->m_ViewMat;
    if (uniforms.mvp >= 0) {
        glUniformMatrix4fv(uniforms.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
    }
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(m_GridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_GridVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(packedVertices.size() * sizeof(GridVertexGpu)),
                 packedVertices.data(),
                 GL_DYNAMIC_DRAW);
    glDrawArrays(GL_LINES, 0, vertexCount);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace RetroRenderer
