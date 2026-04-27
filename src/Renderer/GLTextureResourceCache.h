#pragma once

#include "../Scene/Texture.h"
#include "../include/kris_glheaders.h"
#include <KrisLogger/Logger.h>
#include <cstdint>
#include <unordered_map>

namespace RetroRenderer {

class GLTextureResourceCache {
  public:
    GLTextureResourceCache() = default;
    ~GLTextureResourceCache() = default;
    GLTextureResourceCache(const GLTextureResourceCache&) = delete;
    GLTextureResourceCache& operator=(const GLTextureResourceCache&) = delete;

    GLuint GetOrCreate(const Texture& texture) {
        if (!texture.HasCpuPixels()) {
            return 0;
        }

        auto it = m_Resources.find(&texture);
        if (it != m_Resources.end()) {
            if (it->second.revision == texture.GetRevision()) {
                return it->second.textureId;
            }
            DeleteTexture(it->second.textureId);
            m_Resources.erase(it);
        }

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            texture.GetWidth(),
            texture.GetHeight(),
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            texture.GetPixels().data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_Resources.emplace(&texture, TextureGpuResource{textureId, texture.GetRevision()});
        LOGD("Created GL texture resource: %d x %d", texture.GetWidth(), texture.GetHeight());
        return textureId;
    }

    void Clear() {
        for (auto& entry : m_Resources) {
            DeleteTexture(entry.second.textureId);
        }
        m_Resources.clear();
    }

  private:
    struct TextureGpuResource {
        GLuint textureId = 0;
        uint64_t revision = 0;
    };

    static void DeleteTexture(GLuint& textureId) {
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
    }

    std::unordered_map<const Texture*, TextureGpuResource> m_Resources;
};

} // namespace RetroRenderer
