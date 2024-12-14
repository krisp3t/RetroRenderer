#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

namespace RetroRenderer
{

struct Config
{
    struct WindowSettings
    {
		int width = 1280;
		int height = 720;
		bool fullscreen = false;
		bool enableVsync = true;
		bool showFPS = true;
		bool showConfigPanel = true;
    };

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
        RendererType selectedRenderer = RendererType::SOFTWARE;
        AAType aaType = AAType::NONE;
        bool showWireframe = false;
        bool enablePerspectiveCorrect = true;
        ImVec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        int viewportResolution[2] = {1280, 720};
    };

    struct EnvironmentSettings
    {
        bool showSkybox = false;
        bool showGrid = true;
        bool showFloor = true;
        bool shadowMap = true;
    };

    struct CullSettings
    {
		bool backfaceCulling = false;
        bool depthTest = true;
		bool rasterClip = true;
        bool geometricClip = true;
    };

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

