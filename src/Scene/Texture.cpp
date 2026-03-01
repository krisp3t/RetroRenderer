#include "Texture.h"
#include "KrisLogger/Logger.h"
#include <algorithm>
#include <cmath>

namespace RetroRenderer {
namespace {
bool PopulateTextureStorage(SDL_Surface* surface,
                            GLuint& outTextureID,
                            std::vector<Pixel>& outPixels,
                            int& outWidth,
                            int& outHeight) {
    if (!surface) {
        return false;
    }

    SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
    if (!rgbaSurface) {
        LOGE("Failed to convert texture surface to RGBA32: %s", SDL_GetError());
        return false;
    }

    outWidth = rgbaSurface->w;
    outHeight = rgbaSurface->h;
    outPixels.resize(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight));

    const auto* srcPixels = static_cast<const uint8_t*>(rgbaSurface->pixels);
    for (int y = 0; y < outHeight; y++) {
        const uint8_t* row = srcPixels + static_cast<size_t>(y) * static_cast<size_t>(rgbaSurface->pitch);
        for (int x = 0; x < outWidth; x++) {
            const size_t srcIndex = static_cast<size_t>(x) * 4;
            outPixels[static_cast<size_t>(y) * static_cast<size_t>(outWidth) + static_cast<size_t>(x)] =
                Pixel{row[srcIndex], row[srcIndex + 1], row[srcIndex + 2], row[srcIndex + 3]};
        }
    }

    glGenTextures(1, &outTextureID);
    glBindTexture(GL_TEXTURE_2D, outTextureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outWidth, outHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(rgbaSurface);
    return true;
}
} // namespace

Texture::Texture() : m_TextureID(0) {
}
Texture::~Texture() {
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
}
Texture::Texture(Texture&& other) noexcept : m_TextureID(other.m_TextureID), m_Path(std::move(other.m_Path)) {
    other.m_TextureID = 0; // Invalidate source object
    m_Width = other.m_Width;
    m_Height = other.m_Height;
    m_Pixels = std::move(other.m_Pixels);
    other.m_Width = 0;
    other.m_Height = 0;
}
Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_TextureID) {
            glDeleteTextures(1, &m_TextureID);
        }
        // Transfer ownership
        m_TextureID = other.m_TextureID;
        other.m_TextureID = 0;
        m_Path = std::move(other.m_Path);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_Pixels = std::move(other.m_Pixels);
        other.m_Width = 0;
        other.m_Height = 0;
    }
    return *this;
}

bool Texture::LoadTextureFromFile(const char* filePath,
                                  GLuint& outTextureID,
                                  std::vector<Pixel>& outPixels,
                                  int& outWidth,
                                  int& outHeight) {
    IMG_Init(IMG_INIT_PNG); // TODO: don't hardcode
    SDL_Surface* surface = IMG_Load(filePath);
    if (!surface) {
        LOGE("Failed to load texture file %s: %s", filePath, IMG_GetError());
        return false;
    }

    const bool ok = PopulateTextureStorage(surface, outTextureID, outPixels, outWidth, outHeight);
    SDL_FreeSurface(surface);
    return ok;
}

bool Texture::LoadTextureFromMemory(const uint8_t* data,
                                    const size_t size,
                                    GLuint& outTextureID,
                                    std::vector<Pixel>& outPixels,
                                    int& outWidth,
                                    int& outHeight) {
    int flags = IMG_INIT_PNG;
    int initted = IMG_Init(flags);
    if ((initted & flags) != flags) {
        LOGE("Failed to initialize SDL_image: %s", IMG_GetError());
        return false;
    }

    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(size));
    if (!rw) {
        LOGE("Failed to create RWops: %s", SDL_GetError());
        IMG_Quit();
        return false;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1); // 1 = auto-close rw after loading
    if (!surface) {
        LOGE("Failed to load texture from memory: %s", IMG_GetError());
        IMG_Quit();
        return false;
    }

    const bool ok = PopulateTextureStorage(surface, outTextureID, outPixels, outWidth, outHeight);
    SDL_FreeSurface(surface);
    IMG_Quit();

    return ok;
}

bool Texture::LoadFromFile(const char* filePath) {
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
    m_Pixels.clear();
    m_Width = 0;
    m_Height = 0;

    GLuint newTextureID = 0;
    std::vector<Pixel> newPixels;
    int newWidth = 0;
    int newHeight = 0;
    if (!LoadTextureFromFile(filePath, newTextureID, newPixels, newWidth, newHeight) || !newTextureID) {
        LOGE("Failed to load texture: %s", filePath);
        return false;
    }
    m_TextureID = newTextureID;
    m_Pixels = std::move(newPixels);
    m_Width = newWidth;
    m_Height = newHeight;
    LOGI("Loaded texture %s", filePath);
    m_Path = std::string(filePath);
    return true;
}

bool Texture::LoadFromMemory(const uint8_t* data, const size_t size) {
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
    m_Pixels.clear();
    m_Width = 0;
    m_Height = 0;

    GLuint newTextureID = 0;
    std::vector<Pixel> newPixels;
    int newWidth = 0;
    int newHeight = 0;
    if (!LoadTextureFromMemory(data, size, newTextureID, newPixels, newWidth, newHeight) || !newTextureID) {
        LOGE("Failed to load texture from memory");
        return false;
    }
    m_TextureID = newTextureID;
    m_Pixels = std::move(newPixels);
    m_Width = newWidth;
    m_Height = newHeight;
    LOGI("Loaded texture from memory");
    m_Path = "memory";
    return true;
}

void Texture::Bind(GLuint unit) const {
    if (!IsValid()) {
        LOGW("Tried to bind empty texture");
        return;
    }
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_TextureID);
}

Pixel Texture::SampleNearestRepeat(const glm::vec2& uv) const {
    if (!HasCpuPixels()) {
        return Pixel{255, 255, 255, 255};
    }

    const float wrappedU = uv.x - std::floor(uv.x);
    const float wrappedV = uv.y - std::floor(uv.y);
    const int x = std::clamp(static_cast<int>(std::floor(wrappedU * static_cast<float>(m_Width))), 0, m_Width - 1);
    const int y = std::clamp(static_cast<int>(std::floor((1.0f - wrappedV) * static_cast<float>(m_Height))), 0, m_Height - 1);
    return m_Pixels[static_cast<size_t>(y) * static_cast<size_t>(m_Width) + static_cast<size_t>(x)];
}
} // namespace RetroRenderer
