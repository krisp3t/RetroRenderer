#pragma once

#include "RenderPacket.h"
#include "TextureHandle.h"
#include "../include/kris_glheaders.h"
#include <unordered_map>

namespace RetroRenderer {

GLuint ToGLHandle(ShaderHandle handle);
GLuint ToGLHandle(TextureHandle handle);

const FrameMaterialState* ResolveFrameMaterial(const RenderPacket& packet, FrameMaterialId materialId);
const Texture* ResolveFrameTexture(const RenderPacket& packet, FrameTextureId textureId);

struct GLMeshUniformLocations {
    GLint modelMatrix = -1;
    GLint mvp = -1;
    GLint normalMatrix = -1;
    GLint lightPos = -1;
    GLint viewPos = -1;
    GLint lightColor = -1;
    GLint shininess = -1;
    GLint ambientStrength = -1;
    GLint specularStrength = -1;
    GLint texture = -1;
};

struct GLSkyboxUniformLocations {
    GLint viewMatrix = -1;
    GLint projectionMatrix = -1;
    GLint skyboxSampler = -1;
};

struct GLGridUniformLocations {
    GLint mvp = -1;
};

// Cache uniform locations once per linked program to keep the draw path free of string lookups.
class GLProgramUniformCache {
  public:
    const GLMeshUniformLocations& GetMeshProgram(GLuint program);
    const GLSkyboxUniformLocations& GetSkyboxProgram(GLuint program);
    const GLGridUniformLocations& GetGridProgram(GLuint program);
    void Clear();

  private:
    std::unordered_map<GLuint, GLMeshUniformLocations> m_MeshPrograms;
    std::unordered_map<GLuint, GLSkyboxUniformLocations> m_SkyboxPrograms;
    std::unordered_map<GLuint, GLGridUniformLocations> m_GridPrograms;
};

} // namespace RetroRenderer
