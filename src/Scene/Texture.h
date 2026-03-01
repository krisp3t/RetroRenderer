#pragma once
#include "../Base/Color.h"
#include "../include/kris_glheaders.h"
#include <SDL_image.h>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

namespace RetroRenderer {
class Texture {
  public:
    Texture();
    ~Texture();
    // Disable copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    // Enable moving
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool LoadFromFile(const char* filePath);
    bool LoadFromMemory(const uint8_t* data, const size_t size);
    void Bind(GLuint unit = 0) const;
    GLuint GetID() const {
        return m_TextureID;
    }
    bool IsValid() const {
        return m_TextureID != 0;
    }
    bool HasCpuPixels() const {
        return !m_Pixels.empty() && m_Width > 0 && m_Height > 0;
    }
    const std::string& GetPath() const {
        return m_Path;
    }
    int GetWidth() const {
        return m_Width;
    }
    int GetHeight() const {
        return m_Height;
    }
    Pixel SampleNearestRepeat(const glm::vec2& uv) const;

  private:
    bool LoadTextureFromFile(const char* filePath, GLuint& outTextureID, std::vector<Pixel>& outPixels, int& outWidth, int& outHeight);
    bool LoadTextureFromMemory(const uint8_t* data,
                               const size_t size,
                               GLuint& outTextureID,
                               std::vector<Pixel>& outPixels,
                               int& outWidth,
                               int& outHeight);
    GLuint m_TextureID;
    std::string m_Path;
    int m_Width = 0;
    int m_Height = 0;
    std::vector<Pixel> m_Pixels;
};
} // namespace RetroRenderer
