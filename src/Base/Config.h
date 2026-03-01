#pragma once

#include "../Base/Color.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <imgui.h>

namespace RetroRenderer {

struct Config {
    // Window
    struct WindowSettings {
        glm::ivec2 size = {1280, 720};
        bool fullscreen = false;
        bool enableVsync = true;
        bool showFPS = false;
        bool showConfigPanel = true;
        bool showControls = false;
    };

    // Renderer
    enum class AAType {
        NONE,
        MSAA,
        FXAA,
    };

    enum class RendererType {
        SOFTWARE,
        GL
    };

    enum class RenderPreset {
        CUSTOM,
        DEFAULT,
        PICO8,
        PICOCAD
    };

    enum class PaletteType {
        NONE,
        PICO8
    };

    struct BaseRendererSettings {
        glm::ivec2 resolution = {1280, 720}; // Resolution of the render target
        float resolutionScale = 1.0f;
        RendererType selectedRenderer = RendererType::GL;
        AAType aaType = AAType::NONE;
        bool enablePerspectiveCorrect = true;
        bool nearestNeighborPresentation = false;
        ImVec4 clearColor = Color::DefaultBackground().ToImVec4();
    };

    struct SoftwareRendererSettings {
        bool showNormals = false;
        bool showTangents = false;
        bool showBitangents = false;
        bool showBoundingBox = false;
        bool showOctree = false;
        bool showBVH = false;
    };

    // Environment
    struct EnvironmentSettings {
        bool showSkybox = false;
        bool showGrid = true;
        bool showFloor = true;
        bool shadowMap = true;
        glm::vec3 lightPosition = {0.0f, 0.0f, 5.0f};
    };

    // Culling
    struct CullSettings {
        bool backfaceCulling = false;
        bool depthTest = true;
        bool rasterClip = true;
        bool geometricClip = true;
        bool frustumCull = true;
    };

    // Rasterizer
    enum class RasterizationLineMode {
        DDA,
        BRESENHAM
    };

    enum class RasterizationPolygonMode {
        POINT, // GL_POINT
        LINE, // wireframe = GL_LINE
        FILL, // default   = GL_FILL
    };

    enum class RasterizationFillMode {
        SCANLINE,
        BARYCENTRIC,
        PINEDA
    };

    struct SoftwareRasterizerSettings {
        float pointSize = 1.0f;
        float lineWidth = 1.0f;
        ImVec4 lineColor = {1.0f, 1.0f, 1.0f, 1.0f};
        bool basicLineColors = true; // Display triangle edges as RGB colors
        RasterizationLineMode lineMode = RasterizationLineMode::BRESENHAM;
        RasterizationPolygonMode polygonMode = RasterizationPolygonMode::FILL;
        RasterizationFillMode fillMode = RasterizationFillMode::SCANLINE;
    };

    struct GLRasterizerSettings {
        RasterizationPolygonMode polygonMode = RasterizationPolygonMode::FILL;
    };

    struct SoftwareSpecifics {
        SoftwareRendererSettings renderer;
        SoftwareRasterizerSettings rasterizer;
    };

    struct GLSpecifics {
        GLRasterizerSettings rasterizer;
    };

    struct RetroStyleSettings {
        RenderPreset preset = RenderPreset::DEFAULT;
        PaletteType palette = PaletteType::NONE;
        bool enablePalette = false;
        bool enableColorRamps = false;
        bool enableOrderedDithering = false;
        int lightingBands = 0;
        bool snapVertices = false;
        bool affineTextureMapping = false;
    };

    // Shared between rendering modes
    WindowSettings window;
    EnvironmentSettings environment;
    CullSettings cull;
    BaseRendererSettings renderer;
    // Specific to software / GL
    SoftwareSpecifics software;
    GLSpecifics gl;
    RetroStyleSettings retro;

    static constexpr const char* RenderPresetLabel(RenderPreset preset) {
        switch (preset) {
        case RenderPreset::CUSTOM:
            return "Custom";
        case RenderPreset::DEFAULT:
            return "Default";
        case RenderPreset::PICO8:
            return "PICO-8";
        case RenderPreset::PICOCAD:
            return "picoCAD";
        }
        return "Unknown";
    }

    static constexpr const char* PaletteLabel(PaletteType palette) {
        switch (palette) {
        case PaletteType::NONE:
            return "None";
        case PaletteType::PICO8:
            return "PICO-8";
        }
        return "Unknown";
    }

    static constexpr bool IsRetroPreset(RenderPreset preset) {
        return preset == RenderPreset::PICO8 || preset == RenderPreset::PICOCAD;
    }

    static glm::ivec2 MakeAspectAwareResolution(const glm::ivec2& windowSize, int targetHeight) {
        const int safeWidth = std::max(windowSize.x, 1);
        const int safeHeight = std::max(windowSize.y, 1);
        const float aspect = static_cast<float>(safeWidth) / static_cast<float>(safeHeight);
        const int width = std::max(1, static_cast<int>(std::lround(static_cast<float>(targetHeight) * aspect)));
        return {width, std::max(targetHeight, 1)};
    }

    static void ApplyRenderPreset(Config& config, RenderPreset preset) {
        if (preset == RenderPreset::CUSTOM) {
            config.retro.preset = preset;
            return;
        }

        config.retro.preset = preset;
        config.retro.palette = PaletteType::NONE;
        config.retro.enablePalette = false;
        config.retro.enableColorRamps = false;
        config.retro.enableOrderedDithering = false;
        config.retro.lightingBands = 0;
        config.retro.snapVertices = false;
        config.retro.affineTextureMapping = false;

        config.renderer.resolutionScale = 1.0f;
        config.renderer.aaType = AAType::NONE;
        config.renderer.enablePerspectiveCorrect = true;
        config.renderer.nearestNeighborPresentation = false;
        config.renderer.clearColor = Color::DefaultBackground().ToImVec4();

        config.environment.showSkybox = false;
        config.environment.showGrid = true;
        config.environment.showFloor = true;
        config.environment.shadowMap = true;

        config.cull.backfaceCulling = false;
        config.cull.depthTest = true;
        config.cull.rasterClip = true;
        config.cull.geometricClip = true;
        config.cull.frustumCull = true;

        config.software.rasterizer.polygonMode = RasterizationPolygonMode::FILL;
        config.software.rasterizer.fillMode = RasterizationFillMode::SCANLINE;
        config.gl.rasterizer.polygonMode = RasterizationPolygonMode::FILL;

        switch (preset) {
        case RenderPreset::DEFAULT:
            config.renderer.selectedRenderer = RendererType::GL;
            config.renderer.resolution = {std::max(config.window.size.x, 1), std::max(config.window.size.y, 1)};
            break;
        case RenderPreset::PICO8:
            config.renderer.selectedRenderer = RendererType::SOFTWARE;
            config.renderer.resolution = MakeAspectAwareResolution(config.window.size, 128);
            config.renderer.enablePerspectiveCorrect = false;
            config.renderer.nearestNeighborPresentation = true;
            config.renderer.clearColor = ImVec4(29.0f / 255.0f, 43.0f / 255.0f, 83.0f / 255.0f, 1.0f);
            config.environment.showGrid = false;
            config.environment.showFloor = false;
            config.environment.shadowMap = false;
            config.cull.backfaceCulling = true;
            config.retro.palette = PaletteType::PICO8;
            config.retro.enablePalette = true;
            config.retro.enableColorRamps = true;
            config.retro.enableOrderedDithering = true;
            config.retro.lightingBands = 4;
            break;
        case RenderPreset::PICOCAD:
            config.renderer.selectedRenderer = RendererType::SOFTWARE;
            config.renderer.resolution = MakeAspectAwareResolution(config.window.size, 180);
            config.renderer.enablePerspectiveCorrect = false;
            config.renderer.nearestNeighborPresentation = true;
            config.renderer.clearColor = ImVec4(29.0f / 255.0f, 43.0f / 255.0f, 83.0f / 255.0f, 1.0f);
            config.environment.showGrid = false;
            config.environment.showFloor = false;
            config.environment.shadowMap = false;
            config.cull.backfaceCulling = true;
            config.software.rasterizer.fillMode = RasterizationFillMode::BARYCENTRIC;
            config.retro.palette = PaletteType::PICO8;
            config.retro.enablePalette = true;
            config.retro.enableColorRamps = true;
            config.retro.enableOrderedDithering = true;
            config.retro.lightingBands = 3;
            config.retro.snapVertices = true;
            config.retro.affineTextureMapping = true;
            break;
        case RenderPreset::CUSTOM:
            break;
        }
    }
};

} // namespace RetroRenderer
