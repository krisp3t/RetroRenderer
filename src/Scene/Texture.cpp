#include "Texture.h"
#include "KrisLogger/Logger.h"

namespace RetroRenderer {
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
    }
    return *this;
}

GLuint Texture::LoadTextureFromFile(const char* filePath) {
    IMG_Init(IMG_INIT_PNG); // TODO: don't hardcode
    SDL_Surface* surface = IMG_Load(filePath);
    if (!surface) {
        LOGE("Failed to load texture file %s: %s", filePath, IMG_GetError());
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // SDL surface format to GL format
    GLenum format;
    switch (surface->format->BytesPerPixel) {
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    default:
        LOGE("Unsupported texture format: %d BPP", surface->format->BytesPerPixel);
        SDL_FreeSurface(surface);
        IMG_Quit();
        return 0;
    }

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(surface);

    return textureID;
}

GLuint Texture::LoadTextureFromMemory(const uint8_t* data, const size_t size) {
    int flags = IMG_INIT_PNG;
    int initted = IMG_Init(flags);
    if ((initted & flags) != flags) {
        LOGE("Failed to initialize SDL_image: %s", IMG_GetError());
        return 0;
    }

    SDL_RWops* rw = SDL_RWFromConstMem(data, static_cast<int>(size));
    if (!rw) {
        LOGE("Failed to create RWops: %s", SDL_GetError());
        IMG_Quit();
        return 0;
    }

    SDL_Surface* surface = IMG_Load_RW(rw, 1); // 1 = auto-close rw after loading
    if (!surface) {
        LOGE("Failed to load texture from memory: %s", IMG_GetError());
        IMG_Quit();
        return 0;
    }

    GLenum format;
    switch (surface->format->BytesPerPixel) {
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    default:
        LOGE("Unsupported texture format: %d BPP", surface->format->BytesPerPixel);
        SDL_FreeSurface(surface);
        IMG_Quit();
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(surface);
    IMG_Quit();

    return textureID;
}

bool Texture::LoadFromFile(const char* filePath) {
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
    m_TextureID = LoadTextureFromFile(filePath);
    if (!m_TextureID) {
        LOGE("Failed to load texture: %s", filePath);
        return false;
    }
    LOGI("Loaded texture %s", filePath);
    m_Path = std::string(filePath);
    return true;
}

bool Texture::LoadFromMemory(const uint8_t* data, const size_t size) {
    if (m_TextureID) {
        glDeleteTextures(1, &m_TextureID);
        m_TextureID = 0;
    }
    m_TextureID = LoadTextureFromMemory(data, size);
    if (!m_TextureID) {
        LOGE("Failed to load texture from memory");
        return false;
    }
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
} // namespace RetroRenderer
