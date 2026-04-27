#pragma once

#include "TextureHandle.h"
#include <cstddef>

namespace RetroRenderer {

template <typename T> struct Buffer;
struct Pixel;

class IFramePresenter {
  public:
    IFramePresenter() = default;
    virtual ~IFramePresenter() = default;

    IFramePresenter(const IFramePresenter&) = delete;
    IFramePresenter& operator=(const IFramePresenter&) = delete;

    virtual bool Init(int width, int height, bool nearestNeighbor) = 0;
    virtual bool Resize(int width, int height, bool nearestNeighbor) = 0;
    virtual void Destroy() = 0;

    virtual bool Upload(const Buffer<Pixel>& buffer) = 0;
    virtual bool UploadPixels(const Pixel* pixels, size_t width, size_t height) = 0;

    [[nodiscard]] virtual TextureHandle GetTextureHandle() const = 0;
};

} // namespace RetroRenderer
