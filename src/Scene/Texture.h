#pragma once
#include <SDL_image.h>

#ifdef __ANDROID__
#include <GLES3/gl3.h> // For OpenGL ES 3.0
#else
#include <glad/glad.h>
#endif

namespace RetroRenderer
{
    class Texture
    {
    public:
        Texture();
        ~Texture();
        bool LoadFromFile(const char* filePath);
        void Bind(GLuint unit = 0);
        GLuint GetID() const;
    private:
        GLuint LoadTextureFromFile(const char* filePath);
        GLuint m_TextureID;
    };
}