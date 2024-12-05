#pragma once
#include <glm/glm.hpp>
#include <imgui.h>

namespace RetroRenderer
{

struct Config
{
    enum class AAType
    {
        NONE,
        MSAA,
        FXAA,
    };

    struct RendererSettings
    {
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
        bool cullFace = true;
        bool depthTest = true;
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

    struct RasterizerSettings
	{
        float pointSize = 1.0f;
		float lineWidth = 1.0f;
        ImVec4 lineColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        bool basicLineColors = false; // Display triangle edges as RGB colors
        RasterizationLineMode lineMode = RasterizationLineMode::BRESENHAM;
		RasterizationPolygonMode polygonMode = RasterizationPolygonMode::FILL;
    };

    EnvironmentSettings environment;
    CullSettings cull;
    RendererSettings renderer;
    RasterizerSettings rasterizer;
    bool showConfigPanel = true;
};

}

