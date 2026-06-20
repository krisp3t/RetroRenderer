#pragma once

#include "../Base/Config.h"
#include "../Scene/Texture.h"
#include "../include/kris_glheaders.h"
#include <KrisLogger/Logger.h>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace RetroRenderer {

class GLTextureResourceCache {
  public:
    GLTextureResourceCache() = default;
    ~GLTextureResourceCache() = default;
    GLTextureResourceCache(const GLTextureResourceCache&) = delete;
    GLTextureResourceCache& operator=(const GLTextureResourceCache&) = delete;

    GLuint GetOrCreate(const Texture& texture, Config::GLTextureSampling sampling) {
        if (!texture.HasCpuPixels()) {
            return 0;
        }

        const TextureCacheKey key{&texture, sampling};
        auto it = m_Resources.find(key);
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
        switch (sampling) {
        case Config::GLTextureSampling::FILTERED_MIPS:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case Config::GLTextureSampling::RETRO_NEAREST:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        }
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
        if (sampling == Config::GLTextureSampling::FILTERED_MIPS) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        m_Resources.emplace(key, TextureGpuResource{textureId, texture.GetRevision()});
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
    struct TextureCacheKey {
        const Texture* texture = nullptr;
        Config::GLTextureSampling sampling = Config::GLTextureSampling::FILTERED_MIPS;

        bool operator==(const TextureCacheKey& other) const {
            return texture == other.texture && sampling == other.sampling;
        }
    };

    struct TextureCacheKeyHash {
        size_t operator()(const TextureCacheKey& key) const {
            const size_t textureHash = std::hash<const Texture*>{}(key.texture);
            const size_t samplingHash = std::hash<int>{}(static_cast<int>(key.sampling));
            return textureHash ^ (samplingHash << 1);
        }
    };

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

    std::unordered_map<TextureCacheKey, TextureGpuResource, TextureCacheKeyHash> m_Resources;
};

} // namespace RetroRenderer
