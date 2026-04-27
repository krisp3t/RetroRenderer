#pragma once

#include "../Base/Color.h"
#include "../include/kris_glheaders.h"
#include "Buffer.h"
#include <cstddef>

namespace RetroRenderer {

class GLFramePresenter {
  public:
    GLFramePresenter() = default;
    ~GLFramePresenter();
    GLFramePresenter(const GLFramePresenter&) = delete;
    GLFramePresenter& operator=(const GLFramePresenter&) = delete;

    bool Init(int width, int height, bool nearestNeighbor);
    bool Resize(int width, int height, bool nearestNeighbor);
    void Destroy();

    bool Upload(const Buffer<Pixel>& buffer);
    bool UploadPixels(const Pixel* pixels, size_t width, size_t height);

    [[nodiscard]] GLuint GetTextureHandle() const {
        return m_TextureId;
    }

  private:
    bool CreateTexture(int width, int height, bool nearestNeighbor);
    void ApplyFiltering(bool nearestNeighbor);

    GLuint m_TextureId = 0;
    int m_Width = 0;
    int m_Height = 0;
    bool m_NearestNeighbor = false;
};

} // namespace RetroRenderer
