#pragma once

#include "../Base/Config.h"
#include "MaterialTypes.h"
#include "../Scene/Texture.h"
#include "../include/kris_glheaders.h"
#include <KrisLogger/Logger.h>
#include <algorithm>
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

    GLuint GetOrCreate(const Texture& texture,
                       const MaterialSamplerDesc& samplerDesc,
                       Config::GLTextureSampling samplingOverride) {
        if (!texture.HasCpuPixels()) {
            return 0;
        }

        const TextureCacheKey key{&texture, samplingOverride, samplerDesc.filter, samplerDesc.wrapU, samplerDesc.wrapV};
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
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S,
                        samplerDesc.wrapU == MaterialWrapMode::CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T,
                        samplerDesc.wrapV == MaterialWrapMode::CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        const bool useNearest = samplingOverride == Config::GLTextureSampling::RETRO_NEAREST ||
                                samplerDesc.filter == MaterialFilterMode::NEAREST;
        if (useNearest) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
        if (!useNearest) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        m_Resources.emplace(key, TextureGpuResource{textureId, texture.GetRevision(), EstimateTextureBytes(texture, useNearest)});
        LOGD("Created GL texture resource: %d x %d", texture.GetWidth(), texture.GetHeight());
        return textureId;
    }

    void Clear() {
        for (auto& entry : m_Resources) {
            DeleteTexture(entry.second.textureId);
        }
        m_Resources.clear();
    }

    [[nodiscard]] uint64_t EstimateResidentMemory() const {
        uint64_t totalBytes = 0;
        for (const auto& entry : m_Resources) {
            totalBytes += entry.second.estimatedBytes;
        }
        return totalBytes;
    }

  private:
    struct TextureCacheKey {
        const Texture* texture = nullptr;
        Config::GLTextureSampling samplingOverride = Config::GLTextureSampling::FILTERED_MIPS;
        MaterialFilterMode filter = MaterialFilterMode::LINEAR;
        MaterialWrapMode wrapU = MaterialWrapMode::REPEAT;
        MaterialWrapMode wrapV = MaterialWrapMode::REPEAT;

        bool operator==(const TextureCacheKey& other) const {
            return texture == other.texture &&
                   samplingOverride == other.samplingOverride &&
                   filter == other.filter &&
                   wrapU == other.wrapU &&
                   wrapV == other.wrapV;
        }
    };

    struct TextureCacheKeyHash {
        size_t operator()(const TextureCacheKey& key) const {
            const size_t textureHash = std::hash<const Texture*>{}(key.texture);
            const size_t samplingHash = std::hash<int>{}(static_cast<int>(key.samplingOverride));
            const size_t filterHash = std::hash<int>{}(static_cast<int>(key.filter));
            const size_t wrapUHash = std::hash<int>{}(static_cast<int>(key.wrapU));
            const size_t wrapVHash = std::hash<int>{}(static_cast<int>(key.wrapV));
            return textureHash ^ (samplingHash << 1) ^ (filterHash << 2) ^ (wrapUHash << 3) ^ (wrapVHash << 4);
        }
    };

    struct TextureGpuResource {
        GLuint textureId = 0;
        uint64_t revision = 0;
        uint64_t estimatedBytes = 0;
    };

    static uint64_t EstimateTextureBytes(const Texture& texture, bool useNearest) {
        const uint64_t baseBytes =
            static_cast<uint64_t>(std::max(texture.GetWidth(), 0)) *
            static_cast<uint64_t>(std::max(texture.GetHeight(), 0)) *
            4ull;
        if (useNearest) {
            return baseBytes;
        }
        return baseBytes + baseBytes / 3ull;
    }

    static void DeleteTexture(GLuint& textureId) {
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
    }

    std::unordered_map<TextureCacheKey, TextureGpuResource, TextureCacheKeyHash> m_Resources;
};

} // namespace RetroRenderer
