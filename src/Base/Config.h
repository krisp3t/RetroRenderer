#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

namespace RetroRenderer
{

struct Config
{
    // Window
    struct WindowSettings
    {
		glm::ivec2 size = { 1280, 720 };
		glm::ivec2 outputWindowSize = { 1280, 720 }; // Size of the window that shows rendered image (can be different from image size)
		bool fullscreen = false;
		bool enableVsync = true;
		bool showFPS = true;
		bool showConfigPanel = true;
    };

    // Renderer
    enum class AAType
    {
        NONE,
        MSAA,
        FXAA,
    };
    enum class RendererType
    {
        SOFTWARE,
        GL
    };
    struct RendererSettings
    {
		glm::ivec2 resolution = { 1280, 720 }; // Resolution of the render target (can have different size than window, stretching will occur)
        float resolutionScale = 1.0f;
        bool resolutionAutoResize = false;
        RendererType selectedRenderer = RendererType::SOFTWARE;
        AAType aaType = AAType::NONE;
        bool showWireframe = false;
        bool enablePerspectiveCorrect = true;
        ImVec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    };

	// Environment
    struct EnvironmentSettings
    {
        bool showSkybox = false;
        bool showGrid = true;
        bool showFloor = true;
        bool shadowMap = true;
    };

	// Culling
    struct CullSettings
    {
		bool backfaceCulling = false;
        bool depthTest = true;
		bool rasterClip = true;
        bool geometricClip = true;
    };

    // Rasterizer
	enum class RasterizationLineMode
	{
		DDA,
        BRESENHAM
	};
    enum class RasterizationPolygonMode
    {
        POINT,  // GL_POINT 
        LINE,   // wireframe = GL_LINE
		FILL,   // default   = GL_FILL
    };
    enum class RasterizationFillMode
    {
        SCANLINE,
        BARYCENTRIC,
        PINEDA
    };
    struct RasterizerSettings
	{
        float pointSize = 1.0f;
		float lineWidth = 1.0f;
        ImVec4 lineColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool basicLineColors = true; // Display triangle edges as RGB colors
        RasterizationLineMode lineMode = RasterizationLineMode::BRESENHAM;
		RasterizationPolygonMode polygonMode = RasterizationPolygonMode::FILL;
		RasterizationFillMode fillMode = RasterizationFillMode::SCANLINE;
    };

    WindowSettings window;
    EnvironmentSettings environment;
    CullSettings cull;
    RendererSettings renderer;
    RasterizerSettings rasterizer;
};

}

