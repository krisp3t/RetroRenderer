#include "Texture.h"
#include "KrisLogger/Logger.h"

namespace RetroRenderer
{
    Texture::Texture() : m_TextureID(0)
    {
    }
    Texture::~Texture()
    {
        if (m_TextureID)
        {
            glDeleteTextures(1, &m_TextureID);
            m_TextureID = 0;
        }
    }
    Texture::Texture(Texture&& other) noexcept
    : m_TextureID(other.m_TextureID)
    {
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
        }
        return *this;
    }

    GLuint Texture::LoadTextureFromFile(const char* filePath)
    {
        IMG_Init(IMG_INIT_PNG); // TODO: don't hardcode
        SDL_Surface* surface = IMG_Load(filePath);
        if (!surface)
        {
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
        GLenum format = GL_RGBA;
        if (surface->format->BytesPerPixel == 3) {
            format = GL_RGB;
        }

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0,
                     format, GL_UNSIGNED_BYTE, surface->pixels);

        glGenerateMipmap(GL_TEXTURE_2D);

        SDL_FreeSurface(surface);

        return textureID;
    }

    bool Texture::LoadFromFile(const char* filePath)
    {
        if (m_TextureID) {
            glDeleteTextures(1, &m_TextureID);
            m_TextureID = 0;
        }
        m_TextureID = LoadTextureFromFile(filePath);
        if (!m_TextureID) {
            LOGE("Failed to load texture: %s", filePath);
            return false;
        }
        return true;
    }

    void Texture::Bind(GLuint unit) const
    {
        if (!IsValid())
        {
            LOGW("Tried to bind empty texture");
            return;
        }
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_TextureID);
    }
}
