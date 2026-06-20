#pragma once

#include "../include/kris_glheaders.h"
#include "IFramePresenter.h"
#include <cstdint>

namespace RetroRenderer {

class GLFramePresenter : public IFramePresenter {
  public:
    GLFramePresenter() = default;
    ~GLFramePresenter() override;
    GLFramePresenter(const GLFramePresenter&) = delete;
    GLFramePresenter& operator=(const GLFramePresenter&) = delete;

    bool Init(int width, int height, bool nearestNeighbor) override;
    bool Resize(int width, int height, bool nearestNeighbor) override;
    void Destroy() override;

    bool Upload(const Buffer<Pixel>& buffer) override;
    bool UploadPixels(const Pixel* pixels, size_t width, size_t height) override;

    [[nodiscard]] TextureHandle GetTextureHandle() const override {
        return TextureHandle{static_cast<uintptr_t>(m_TextureId)};
    }
    [[nodiscard]] uint64_t EstimateResidentMemory() const;

  private:
    bool CreateTexture(int width, int height, bool nearestNeighbor);
    void ApplyFiltering(bool nearestNeighbor);

    GLuint m_TextureId = 0;
    int m_Width = 0;
    int m_Height = 0;
    bool m_NearestNeighbor = false;
};

} // namespace RetroRenderer
