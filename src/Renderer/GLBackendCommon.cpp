#include "GLBackendCommon.h"

namespace RetroRenderer {

GLuint ToGLHandle(ShaderHandle handle) {
    return static_cast<GLuint>(handle.value);
}

GLuint ToGLHandle(TextureHandle handle) {
    return static_cast<GLuint>(handle.value);
}

const FrameMaterialState* ResolveFrameMaterial(const RenderPacket& packet, FrameMaterialId materialId) {
    if (materialId == kInvalidFrameMaterialId || materialId >= packet.materials.size()) {
        return nullptr;
    }
    return &packet.materials[materialId];
}

const Texture* ResolveFrameTexture(const RenderPacket& packet, FrameTextureId textureId) {
    if (textureId == kInvalidFrameTextureId || textureId >= packet.textures.size()) {
        return nullptr;
    }

    return packet.textures[textureId].get();
}

const GLMeshUniformLocations& GLProgramUniformCache::GetMeshProgram(GLuint program) {
    auto [it, inserted] = m_MeshPrograms.try_emplace(program);
    if (inserted) {
        it->second.modelMatrix = glGetUniformLocation(program, "u_ModelMatrix");
        it->second.mvp = glGetUniformLocation(program, "u_MVP");
        it->second.normalMatrix = glGetUniformLocation(program, "u_NormalMatrix");
        it->second.lightPos = glGetUniformLocation(program, "u_LightPos");
        it->second.viewPos = glGetUniformLocation(program, "u_ViewPos");
        it->second.lightColor = glGetUniformLocation(program, "u_LightColor");
        it->second.shininess = glGetUniformLocation(program, "u_Shininess");
        it->second.ambientStrength = glGetUniformLocation(program, "u_AmbientStrength");
        it->second.specularStrength = glGetUniformLocation(program, "u_SpecularStrength");
        it->second.texture = glGetUniformLocation(program, "u_Texture");
    }
    return it->second;
}

const GLSkyboxUniformLocations& GLProgramUniformCache::GetSkyboxProgram(GLuint program) {
    auto [it, inserted] = m_SkyboxPrograms.try_emplace(program);
    if (inserted) {
        it->second.viewMatrix = glGetUniformLocation(program, "u_ViewMatrix");
        it->second.projectionMatrix = glGetUniformLocation(program, "u_ProjectionMatrix");
        it->second.skyboxSampler = glGetUniformLocation(program, "skybox");
    }
    return it->second;
}

const GLGridUniformLocations& GLProgramUniformCache::GetGridProgram(GLuint program) {
    auto [it, inserted] = m_GridPrograms.try_emplace(program);
    if (inserted) {
        it->second.mvp = glGetUniformLocation(program, "u_MVP");
    }
    return it->second;
}

void GLProgramUniformCache::Clear() {
    m_MeshPrograms.clear();
    m_SkyboxPrograms.clear();
    m_GridPrograms.clear();
}

} // namespace RetroRenderer
