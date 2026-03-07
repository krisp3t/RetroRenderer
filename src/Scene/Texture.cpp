#include "Texture.h"
#include "../Renderer/RetroPalette.h"
#include "KrisLogger/Logger.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace RetroRenderer {
namespace {
struct PaletteCandidate {
    uint32_t count = 0;
    uint32_t sumR = 0;
    uint32_t sumG = 0;
    uint32_t sumB = 0;
    Pixel average{};
};

constexpr size_t QuantizedRgb5Index(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<size_t>(r >> 3) << 10) |
           (static_cast<size_t>(g >> 3) << 5) |
           static_cast<size_t>(b >> 3);
}

int WeightedColorDistanceSq(const Pixel& a, const Pixel& b) {
    const int dr = static_cast<int>(a.r) - static_cast<int>(b.r);
    const int dg = static_cast<int>(a.g) - static_cast<int>(b.g);
    const int db = static_cast<int>(a.b) - static_cast<int>(b.b);
    return dr * dr * 3 + dg * dg * 4 + db * db * 2;
}

float QuantizeUnitToBands(float value, int bandCount) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    if (bandCount <= 0) {
        return clamped;
    }
    if (bandCount == 1) {
        return clamped >= 0.5f ? 1.0f : 0.0f;
    }

    const int safeBandCount = std::max(bandCount, 2);
    return std::round(clamped * static_cast<float>(safeBandCount - 1)) / static_cast<float>(safeBandCount - 1);
}

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
    m_HasAutoPalette = other.m_HasAutoPalette;
    m_AutoPalette = other.m_AutoPalette;
    m_AutoRampPixels = other.m_AutoRampPixels;
    m_AutoDitherPatterns = other.m_AutoDitherPatterns;
    m_AutoPaletteNearestIndexLut = other.m_AutoPaletteNearestIndexLut;
    other.m_Width = 0;
    other.m_Height = 0;
    other.m_HasAutoPalette = false;
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
        m_HasAutoPalette = other.m_HasAutoPalette;
        m_AutoPalette = other.m_AutoPalette;
        m_AutoRampPixels = other.m_AutoRampPixels;
        m_AutoDitherPatterns = other.m_AutoDitherPatterns;
        m_AutoPaletteNearestIndexLut = other.m_AutoPaletteNearestIndexLut;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_HasAutoPalette = false;
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
    RebuildAutoPaletteCaches();
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
    RebuildAutoPaletteCaches();
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

Texture Texture::CloneCpuOnly() const {
    Texture clone;
    clone.m_Path = m_Path;
    clone.m_Width = m_Width;
    clone.m_Height = m_Height;
    clone.m_Pixels = m_Pixels;
    clone.m_HasAutoPalette = m_HasAutoPalette;
    clone.m_AutoPalette = m_AutoPalette;
    clone.m_AutoRampPixels = m_AutoRampPixels;
    clone.m_AutoDitherPatterns = m_AutoDitherPatterns;
    clone.m_AutoPaletteNearestIndexLut = m_AutoPaletteNearestIndexLut;
    return clone;
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

uint8_t Texture::FindNearestAutoPaletteIndex(const Color& color) const {
    return FindNearestAutoPaletteIndex(color.r, color.g, color.b);
}

uint8_t Texture::FindNearestAutoPaletteIndex(uint8_t r, uint8_t g, uint8_t b) const {
    if (!m_HasAutoPalette) {
        return 0;
    }
    return m_AutoPaletteNearestIndexLut[QuantizedRgb5Index(r, g, b)];
}

Pixel Texture::FindNearestAutoPalettePixel(const Color& color) const {
    if (!m_HasAutoPalette) {
        return color.ToPixel();
    }
    Pixel quantized = m_AutoPalette[FindNearestAutoPaletteIndex(color.r, color.g, color.b)];
    quantized.a = color.a;
    return quantized;
}

Pixel Texture::SampleAutoRampPixel(uint8_t basePaletteIndex, float value, int bandCount, uint8_t alpha) const {
    if (!m_HasAutoPalette) {
        return Pixel{255, 255, 255, alpha};
    }
    const float quantized = QuantizeUnitToBands(value, bandCount);
    const size_t slot = std::min<size_t>(
        static_cast<size_t>(std::lround(quantized * static_cast<float>(m_AutoRampPixels[0].size() - 1))),
        m_AutoRampPixels[0].size() - 1);
    Pixel pixel = m_AutoRampPixels[std::min<size_t>(basePaletteIndex, kAutoPaletteSize - 1)][slot];
    pixel.a = alpha;
    return pixel;
}

const std::array<Pixel, 16>& Texture::GetAutoDitherPattern4x4(uint8_t paletteIndex) const {
    return m_AutoDitherPatterns[std::min<size_t>(paletteIndex, kAutoPaletteSize - 1)];
}

void Texture::ClearAutoPaletteCaches() {
    m_HasAutoPalette = false;
    m_AutoPalette.fill(Pixel{255, 255, 255, 255});
    for (auto& ramp : m_AutoRampPixels) {
        ramp.fill(Pixel{255, 255, 255, 255});
    }
    for (auto& pattern : m_AutoDitherPatterns) {
        pattern.fill(Pixel{255, 255, 255, 255});
    }
    m_AutoPaletteNearestIndexLut.fill(0);
}

void Texture::RebuildAutoPaletteCaches() {
    ClearAutoPaletteCaches();
    if (!HasCpuPixels()) {
        return;
    }

    struct BucketAccum {
        uint32_t count = 0;
        uint32_t sumR = 0;
        uint32_t sumG = 0;
        uint32_t sumB = 0;
    };

    std::array<BucketAccum, kRgb5LutSize> buckets{};
    for (const Pixel& pixel : m_Pixels) {
        if (pixel.a == 0) {
            continue;
        }
        BucketAccum& bucket = buckets[QuantizedRgb5Index(pixel.r, pixel.g, pixel.b)];
        bucket.count++;
        bucket.sumR += pixel.r;
        bucket.sumG += pixel.g;
        bucket.sumB += pixel.b;
    }

    std::vector<PaletteCandidate> candidates;
    candidates.reserve(kRgb5LutSize);
    for (const BucketAccum& bucket : buckets) {
        if (bucket.count == 0) {
            continue;
        }
        PaletteCandidate candidate{};
        candidate.count = bucket.count;
        candidate.sumR = bucket.sumR;
        candidate.sumG = bucket.sumG;
        candidate.sumB = bucket.sumB;
        candidate.average = Pixel{
            static_cast<uint8_t>(bucket.sumR / bucket.count),
            static_cast<uint8_t>(bucket.sumG / bucket.count),
            static_cast<uint8_t>(bucket.sumB / bucket.count),
            255};
        candidates.push_back(candidate);
    }

    if (candidates.empty()) {
        return;
    }

    std::sort(candidates.begin(), candidates.end(), [](const PaletteCandidate& a, const PaletteCandidate& b) {
        return a.count > b.count;
    });

    std::vector<Pixel> selectedPalette;
    selectedPalette.reserve(kAutoPaletteSize);
    constexpr int kDiversityThreshold = 18 * 18 * 3;
    for (const PaletteCandidate& candidate : candidates) {
        bool distinctEnough = true;
        for (const Pixel& existing : selectedPalette) {
            if (WeightedColorDistanceSq(existing, candidate.average) < kDiversityThreshold) {
                distinctEnough = false;
                break;
            }
        }
        if (distinctEnough) {
            selectedPalette.push_back(candidate.average);
            if (selectedPalette.size() == kAutoPaletteSize) {
                break;
            }
        }
    }

    if (selectedPalette.size() < kAutoPaletteSize) {
        for (const PaletteCandidate& candidate : candidates) {
            selectedPalette.push_back(candidate.average);
            if (selectedPalette.size() == kAutoPaletteSize) {
                break;
            }
        }
    }

    while (selectedPalette.size() < kAutoPaletteSize) {
        selectedPalette.push_back(selectedPalette.back());
    }

    for (size_t i = 0; i < kAutoPaletteSize; i++) {
        m_AutoPalette[i] = selectedPalette[i];
    }

    for (size_t r = 0; r < 32; r++) {
        for (size_t g = 0; g < 32; g++) {
            for (size_t b = 0; b < 32; b++) {
                const Pixel sample{
                    static_cast<uint8_t>((r << 3) | (r >> 2)),
                    static_cast<uint8_t>((g << 3) | (g >> 2)),
                    static_cast<uint8_t>((b << 3) | (b >> 2)),
                    255};
                int bestDistance = WeightedColorDistanceSq(sample, m_AutoPalette[0]);
                uint8_t bestIndex = 0;
                for (size_t i = 1; i < kAutoPaletteSize; i++) {
                    const int distance = WeightedColorDistanceSq(sample, m_AutoPalette[i]);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestIndex = static_cast<uint8_t>(i);
                    }
                }
                m_AutoPaletteNearestIndexLut[(r << 10) | (g << 5) | b] = bestIndex;
            }
        }
    }

    m_HasAutoPalette = true;

    for (size_t paletteIndex = 0; paletteIndex < kAutoPaletteSize; paletteIndex++) {
        const Pixel& baseColor = m_AutoPalette[paletteIndex];
        for (size_t slot = 0; slot < m_AutoRampPixels[paletteIndex].size(); slot++) {
            const float factor =
                static_cast<float>(slot) / static_cast<float>(m_AutoRampPixels[paletteIndex].size() - 1);
            const Color shaded(
                Color::Uint8Tag{},
                static_cast<uint8_t>(std::lround(static_cast<float>(baseColor.r) * factor)),
                static_cast<uint8_t>(std::lround(static_cast<float>(baseColor.g) * factor)),
                static_cast<uint8_t>(std::lround(static_cast<float>(baseColor.b) * factor)),
                255);
            m_AutoRampPixels[paletteIndex][slot] = FindNearestAutoPalettePixel(shaded);
        }

        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                const float centeredThreshold = RetroPalette::GetOrderedDitherThreshold4x4(glm::ivec2{x, y}) - 0.5f;
                const int offset = static_cast<int>(std::lround(centeredThreshold * 64.0f));
                const auto applyOffset = [offset](uint8_t channel) {
                    return static_cast<uint8_t>(std::clamp(static_cast<int>(channel) + offset, 0, 255));
                };
                const Color adjusted(
                    Color::Uint8Tag{},
                    applyOffset(baseColor.r),
                    applyOffset(baseColor.g),
                    applyOffset(baseColor.b),
                    255);
                m_AutoDitherPatterns[paletteIndex][static_cast<size_t>((y << 2) | x)] = FindNearestAutoPalettePixel(adjusted);
            }
        }
    }

}
} // namespace RetroRenderer
