#pragma once
#include <SDL_image.h>
#include <string>
#include "../include/kris_glheaders.h"


namespace RetroRenderer
{
    class Texture
    {
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
        GLuint GetID() const { return m_TextureID; }
        bool IsValid() const { return m_TextureID != 0; }
        const std::string& GetPath() const { return m_Path; }
    private:
        GLuint LoadTextureFromFile(const char* filePath);
        GLuint LoadTextureFromMemory(const uint8_t* data, const size_t size);
        GLuint m_TextureID;
        std::string m_Path;
    };
}