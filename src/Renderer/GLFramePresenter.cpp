#include "GLFramePresenter.h"
#include "../Base/Color.h"
#include "Buffer.h"
#include <KrisLogger/Logger.h>

namespace RetroRenderer {
namespace {
GLint TextureFilter(bool nearestNeighbor) {
    return nearestNeighbor ? GL_NEAREST : GL_LINEAR;
}

GLint TextureInternalFormat() {
    return GL_RGBA8;
}

void ClearPendingGlErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
} // namespace

GLFramePresenter::~GLFramePresenter() {
    Destroy();
}

bool GLFramePresenter::Init(int width, int height, bool nearestNeighbor) {
    Destroy();
    return CreateTexture(width, height, nearestNeighbor);
}

bool GLFramePresenter::Resize(int width, int height, bool nearestNeighbor) {
    if (m_TextureId != 0 &&
        m_Width == width &&
        m_Height == height) {
        ApplyFiltering(nearestNeighbor);
        m_NearestNeighbor = nearestNeighbor;
        return true;
    }

    Destroy();
    return CreateTexture(width, height, nearestNeighbor);
}

void GLFramePresenter::Destroy() {
    if (m_TextureId != 0) {
        glDeleteTextures(1, &m_TextureId);
        m_TextureId = 0;
    }
    m_Width = 0;
    m_Height = 0;
}

bool GLFramePresenter::Upload(const Buffer<Pixel>& buffer) {
    return UploadPixels(buffer.data, buffer.width, buffer.height);
}

bool GLFramePresenter::UploadPixels(const Pixel* pixels, size_t width, size_t height) {
    if (m_TextureId == 0) {
        LOGE("Cannot upload software frame: presenter texture is not initialized");
        return false;
    }
    if (width == 0 || height == 0 || pixels == nullptr) {
        return false;
    }
    if (static_cast<int>(width) != m_Width || static_cast<int>(height) != m_Height) {
        LOGE("Software frame size %zu x %zu does not match presenter texture %d x %d",
             width,
             height,
             m_Width,
             m_Height);
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

uint64_t GLFramePresenter::EstimateResidentMemory() const {
    if (m_TextureId == 0 || m_Width <= 0 || m_Height <= 0) {
        return 0;
    }
    return static_cast<uint64_t>(m_Width) * static_cast<uint64_t>(m_Height) * 4ull;
}

bool GLFramePresenter::CreateTexture(int width, int height, bool nearestNeighbor) {
    if (width <= 0 || height <= 0) {
        LOGE("Cannot create frame presenter texture with invalid size %d x %d", width, height);
        return false;
    }

    glGenTextures(1, &m_TextureId);
    if (m_TextureId == 0) {
        LOGE("Failed to create frame presenter texture");
        return false;
    }

    m_Width = width;
    m_Height = height;
    m_NearestNeighbor = nearestNeighbor;

    ClearPendingGlErrors();
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    const GLint filter = TextureFilter(nearestNeighbor);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        TextureInternalFormat(),
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    const GLenum textureError = glGetError();
    if (textureError != GL_NO_ERROR) {
        LOGE("Failed to allocate frame presenter texture %d x %d (error=0x%x)", width, height, textureError);
        glBindTexture(GL_TEXTURE_2D, 0);
        Destroy();
        return false;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void GLFramePresenter::ApplyFiltering(bool nearestNeighbor) {
    if (m_TextureId == 0) {
        return;
    }

    const GLint filter = TextureFilter(nearestNeighbor);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace RetroRenderer
