#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Base/Config.h"
#include "Renderer/Buffer.h"
#include "Renderer/MaterialRuntime.h"
#include "Renderer/Software/Rasterizer.h"
#include "Scene/Texture.h"
#include "Scene/Vertex.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

namespace RetroRenderer {
namespace {
bool PixelsEqual(const Pixel& lhs, const Pixel& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

size_t CountPixels(const Buffer<Pixel>& framebuffer, const Pixel& color) {
    size_t count = 0;
    const size_t pixelCount = framebuffer.width * framebuffer.height;
    for (size_t i = 0; i < pixelCount; i++) {
        if (PixelsEqual(framebuffer.data[i], color)) {
            count++;
        }
    }
    return count;
}

bool BuffersEqual(const Buffer<Pixel>& lhs, const Buffer<Pixel>& rhs) {
    if (lhs.width != rhs.width || lhs.height != rhs.height) {
        return false;
    }
    const size_t pixelCount = lhs.width * lhs.height;
    for (size_t i = 0; i < pixelCount; i++) {
        if (!PixelsEqual(lhs.data[i], rhs.data[i])) {
            return false;
        }
    }
    return true;
}

template <typename TPredicate>
size_t CountPixelsIf(const Buffer<Pixel>& framebuffer, TPredicate predicate) {
    size_t count = 0;
    const size_t pixelCount = framebuffer.width * framebuffer.height;
    for (size_t i = 0; i < pixelCount; i++) {
        if (predicate(framebuffer.data[i])) {
            count++;
        }
    }
    return count;
}

Vertex MakeVertex(float x, float y, float z) {
    Vertex vertex{};
    vertex.position = glm::vec4(x, y, z, 1.0f);
    vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertex.texCoords = glm::vec2(0.0f);
    vertex.color = glm::vec3(1.0f);
    return vertex;
}

RasterVertex MakeLitRasterVertex(float x, float y, float ndcZ, float worldZ) {
    RasterVertex vertex{};
    vertex.position = glm::vec3(x, y, ndcZ);
    vertex.cullPosition = glm::vec3(x, -y, ndcZ);
    vertex.worldPosition = glm::vec3(x, y, worldZ);
    vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
    vertex.color = glm::vec4(1.0f);
    vertex.clipW = 1.0f;
    return vertex;
}

SoftwareMaterialState MakeVertexColorPhongMaterialState() {
    auto material = std::make_shared<CompiledMaterialTemplate>();
    material->pipelineState.shadingModel = MaterialShadingModel::PHONG;
    material->usesVertexColor = true;

    MaterialInstruction color{};
    color.opcode = MaterialOpcode::SEMANTIC;
    color.resultType = MaterialDataType::VEC4;
    color.semantic = MaterialSemantic::COLOR0;
    color.componentCount = 4;
    color.dstRegister = 0;
    material->fragmentProgram.instructions.push_back(color);
    material->fragmentProgram.registerTypes.push_back(MaterialDataType::VEC4);
    material->fragmentProgram.registerCount = 1;
    material->fragmentOutputs.baseColorRegister = 0;

    const auto appendFloatConstant = [&](float value) {
        MaterialInstruction instruction{};
        instruction.opcode = MaterialOpcode::CONSTANT;
        instruction.resultType = MaterialDataType::FLOAT1;
        instruction.immediate = glm::vec4(value, 0.0f, 0.0f, 0.0f);
        instruction.componentCount = 1;
        instruction.dstRegister = material->fragmentProgram.registerCount;
        material->fragmentProgram.instructions.push_back(instruction);
        material->fragmentProgram.registerTypes.push_back(MaterialDataType::FLOAT1);
        material->fragmentProgram.registerCount++;
        return instruction.dstRegister;
    };

    material->fragmentOutputs.alphaRegister = appendFloatConstant(1.0f);
    material->fragmentOutputs.ambientStrengthRegister = appendFloatConstant(0.0f);
    material->fragmentOutputs.specularStrengthRegister = appendFloatConstant(0.0f);
    material->fragmentOutputs.shininessRegister = appendFloatConstant(32.0f);

    SoftwareMaterialState state{};
    state.compiledTemplate = std::move(material);
    state.pipelineState = state.compiledTemplate->pipelineState;
    return state;
}

SoftwareMaterialState MakeVertexColorUnlitMaterialState() {
    auto material = std::make_shared<CompiledMaterialTemplate>();
    material->pipelineState.shadingModel = MaterialShadingModel::UNLIT;
    material->usesVertexColor = true;

    MaterialInstruction color{};
    color.opcode = MaterialOpcode::SEMANTIC;
    color.resultType = MaterialDataType::VEC4;
    color.semantic = MaterialSemantic::COLOR0;
    color.componentCount = 4;
    color.dstRegister = 0;
    material->fragmentProgram.instructions.push_back(color);
    material->fragmentProgram.registerTypes.push_back(MaterialDataType::VEC4);
    material->fragmentProgram.registerCount = 1;
    material->fragmentOutputs.baseColorRegister = 0;

    MaterialInstruction alpha{};
    alpha.opcode = MaterialOpcode::CONSTANT;
    alpha.resultType = MaterialDataType::FLOAT1;
    alpha.immediate = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    alpha.componentCount = 1;
    alpha.dstRegister = 1;
    material->fragmentProgram.instructions.push_back(alpha);
    material->fragmentProgram.registerTypes.push_back(MaterialDataType::FLOAT1);
    material->fragmentProgram.registerCount = 2;
    material->fragmentOutputs.alphaRegister = 1;

    SoftwareMaterialState state{};
    state.compiledTemplate = std::move(material);
    state.pipelineState = state.compiledTemplate->pipelineState;
    return state;
}

std::shared_ptr<CompiledMaterialTemplate> MakeTexturedUnlitMaterialTemplate() {
    auto material = std::make_shared<CompiledMaterialTemplate>();
    material->pipelineState.shadingModel = MaterialShadingModel::UNLIT;
    material->usesTextureSampling = true;
    material->samplers.push_back(MaterialSamplerDesc{
        .name = "albedo",
        .filter = MaterialFilterMode::NEAREST,
        .wrapU = MaterialWrapMode::REPEAT,
        .wrapV = MaterialWrapMode::REPEAT,
    });

    MaterialInstruction uv{};
    uv.opcode = MaterialOpcode::SEMANTIC;
    uv.resultType = MaterialDataType::VEC2;
    uv.semantic = MaterialSemantic::UV0;
    uv.componentCount = 2;
    uv.dstRegister = 0;
    material->fragmentProgram.instructions.push_back(uv);
    material->fragmentProgram.registerTypes.push_back(MaterialDataType::VEC2);
    material->fragmentProgram.registerCount = 1;

    MaterialInstruction sample{};
    sample.opcode = MaterialOpcode::SAMPLE_TEXTURE;
    sample.resultType = MaterialDataType::VEC4;
    sample.samplerIndex = 0;
    sample.srcRegisters[0] = 0;
    sample.componentCount = 4;
    sample.dstRegister = 1;
    material->fragmentProgram.instructions.push_back(sample);
    material->fragmentProgram.registerTypes.push_back(MaterialDataType::VEC4);
    material->fragmentProgram.registerCount = 2;
    material->fragmentOutputs.baseColorRegister = 1;

    MaterialInstruction alpha{};
    alpha.opcode = MaterialOpcode::CONSTANT;
    alpha.resultType = MaterialDataType::FLOAT1;
    alpha.immediate = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    alpha.componentCount = 1;
    alpha.dstRegister = 2;
    material->fragmentProgram.instructions.push_back(alpha);
    material->fragmentProgram.registerTypes.push_back(MaterialDataType::FLOAT1);
    material->fragmentProgram.registerCount = 3;
    material->fragmentOutputs.alphaRegister = 2;
    return material;
}

std::filesystem::path MakeTempDir(const char* name) {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "retrorenderer_rasterizer_tests" / name;
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}

std::filesystem::path WriteRedBmp(const std::filesystem::path& directory) {
    static constexpr uint8_t kRedBmpBytes[] = {
        0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00,
        0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00,
        0x13, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xFF, 0x00,
    };

    const std::filesystem::path texturePath = directory / "red.bmp";
    std::ofstream file(texturePath, std::ios::binary);
    file.write(reinterpret_cast<const char*>(kRedBmpBytes), sizeof(kRedBmpBytes));
    return texturePath;
}

std::array<Vertex, 3> MakeLargeTriangle(float ndcZ) {
    return {MakeVertex(-0.8f, -0.8f, ndcZ), MakeVertex(0.8f, -0.8f, ndcZ), MakeVertex(0.0f, 0.8f, ndcZ)};
}

Config MakeBarycentricFillConfig() {
    Config config{};
    config.cull.backfaceCulling = false;
    config.cull.depthTest = true;
    config.cull.rasterClip = true;
    config.software.rasterizer.polygonMode = Config::RasterizationPolygonMode::FILL;
    config.software.rasterizer.fillMode = Config::RasterizationFillMode::BARYCENTRIC;
    return config;
}

Config MakeLineConfig(Config::RasterizationLineMode lineMode) {
    Config config{};
    config.cull.rasterClip = true;
    config.software.rasterizer.lineMode = lineMode;
    return config;
}

int PixelLuminance(const Pixel& pixel) {
    return static_cast<int>(pixel.r) + static_cast<int>(pixel.g) + static_cast<int>(pixel.b);
}

} // namespace

TEST_CASE("NDCToViewport maps canonical coordinates with subpixel offset", "[rasterizer]") {
    const glm::vec2 topLeft = Rasterizer::NDCToViewport(glm::vec2(-1.0f, 1.0f), 640, 480);
    REQUIRE(topLeft.x == Catch::Approx(0.5f));
    REQUIRE(topLeft.y == Catch::Approx(0.5f));

    const glm::vec2 center = Rasterizer::NDCToViewport(glm::vec2(0.0f, 0.0f), 640, 480);
    REQUIRE(center.x == Catch::Approx(320.5f));
    REQUIRE(center.y == Catch::Approx(240.5f));
}

TEST_CASE("DrawTriangle fills framebuffer in barycentric mode", "[rasterizer]") {
    Buffer<Pixel> framebuffer(32, 32);
    Buffer<float> depthBuffer(32, 32);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    Config config = MakeBarycentricFillConfig();
    std::array<Vertex, 3> triangle = MakeLargeTriangle(0.0f);
    const Pixel fillColor{17, 34, 51, 255};

    Rasterizer::DrawTriangle(framebuffer, depthBuffer, triangle, config, fillColor);

    REQUIRE(CountPixels(framebuffer, fillColor) > 0);

    const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
    REQUIRE(PixelsEqual(framebuffer.data[centerIndex], fillColor));
    REQUIRE(depthBuffer.data[centerIndex] < 1.0f);
}

TEST_CASE("Shared-edge triangles partition interior without gaps or overlap", "[rasterizer][edge-rules]") {
    Buffer<Pixel> framebufferA(32, 32);
    Buffer<Pixel> framebufferB(32, 32);
    Buffer<float> depthA(32, 32);
    Buffer<float> depthB(32, 32);
    framebufferA.Clear(Pixel{0, 0, 0, 0});
    framebufferB.Clear(Pixel{0, 0, 0, 0});
    depthA.Clear(1.0f);
    depthB.Clear(1.0f);

    Config config = MakeBarycentricFillConfig();
    config.cull.depthTest = false;
    std::array<Vertex, 3> triangleA = {MakeVertex(-0.5f, -0.5f, 0.0f), MakeVertex(0.5f, -0.5f, 0.0f),
                                        MakeVertex(0.5f, 0.5f, 0.0f)};
    std::array<Vertex, 3> triangleB = {MakeVertex(-0.5f, -0.5f, 0.0f), MakeVertex(0.5f, 0.5f, 0.0f),
                                        MakeVertex(-0.5f, 0.5f, 0.0f)};
    const Pixel colorA{255, 40, 40, 255};
    const Pixel colorB{40, 255, 40, 255};

    Rasterizer::DrawTriangle(framebufferA, depthA, triangleA, config, colorA);
    Rasterizer::DrawTriangle(framebufferB, depthB, triangleB, config, colorB);

    for (int y = 9; y <= 23; y++) {
        for (int x = 9; x <= 23; x++) {
            const size_t index = static_cast<size_t>(y) * framebufferA.width + static_cast<size_t>(x);
            const bool inA = PixelsEqual(framebufferA.data[index], colorA);
            const bool inB = PixelsEqual(framebufferB.data[index], colorB);
            REQUIRE(inA != inB);
        }
    }
}

TEST_CASE("Raster clipping handles fully and partially outside triangles", "[rasterizer][edge-rules]") {
    Buffer<Pixel> framebuffer(32, 32);
    Buffer<float> depthBuffer(32, 32);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    Config clipOnConfig = MakeBarycentricFillConfig();
    clipOnConfig.cull.depthTest = false;
    clipOnConfig.cull.rasterClip = true;

    std::array<Vertex, 3> fullyOutside =
        {MakeVertex(1.2f, -0.3f, 0.0f), MakeVertex(1.5f, 0.0f, 0.0f), MakeVertex(1.3f, 0.7f, 0.0f)};
    const Pixel outsideColor{250, 200, 20, 255};
    Rasterizer::DrawTriangle(framebuffer, depthBuffer, fullyOutside, clipOnConfig, outsideColor);
    REQUIRE(CountPixels(framebuffer, outsideColor) == 0);

    std::array<Vertex, 3> partiallyOutside =
        {MakeVertex(0.2f, -0.2f, 0.0f), MakeVertex(1.3f, -0.2f, 0.0f), MakeVertex(0.7f, 0.8f, 0.0f)};
    const Pixel partialColor{20, 200, 250, 255};
    Rasterizer::DrawTriangle(framebuffer, depthBuffer, partiallyOutside, clipOnConfig, partialColor);
    REQUIRE(CountPixels(framebuffer, partialColor) > 0);

    Buffer<Pixel> clipOnFramebuffer(32, 32);
    Buffer<Pixel> clipOffFramebuffer(32, 32);
    Buffer<float> clipOnDepth(32, 32);
    Buffer<float> clipOffDepth(32, 32);
    clipOnFramebuffer.Clear(Pixel{0, 0, 0, 0});
    clipOffFramebuffer.Clear(Pixel{0, 0, 0, 0});
    clipOnDepth.Clear(1.0f);
    clipOffDepth.Clear(1.0f);

    std::array<Vertex, 3> fullyInside = MakeLargeTriangle(0.0f);
    const Pixel insideColor{90, 120, 255, 255};
    Config clipOffConfig = clipOnConfig;
    clipOffConfig.cull.rasterClip = false;

    Rasterizer::DrawTriangle(clipOnFramebuffer, clipOnDepth, fullyInside, clipOnConfig, insideColor);
    Rasterizer::DrawTriangle(clipOffFramebuffer, clipOffDepth, fullyInside, clipOffConfig, insideColor);

    REQUIRE(CountPixels(clipOnFramebuffer, insideColor) > 0);
    REQUIRE(BuffersEqual(clipOnFramebuffer, clipOffFramebuffer));
}

TEST_CASE("Backface culling rejects backfacing and degenerate triangles", "[rasterizer][edge-rules]") {
    Config config = MakeBarycentricFillConfig();
    config.cull.backfaceCulling = true;
    config.cull.depthTest = false;

    const Pixel drawColor{255, 200, 10, 255};

    Buffer<Pixel> frontBuffer(32, 32);
    Buffer<float> frontDepth(32, 32);
    frontBuffer.Clear(Pixel{0, 0, 0, 0});
    frontDepth.Clear(1.0f);
    std::array<Vertex, 3> frontFacing = {MakeVertex(-0.6f, -0.5f, 0.0f), MakeVertex(0.0f, 0.6f, 0.0f),
                                          MakeVertex(0.6f, -0.5f, 0.0f)};
    Rasterizer::DrawTriangle(frontBuffer, frontDepth, frontFacing, config, drawColor);
    REQUIRE(CountPixels(frontBuffer, drawColor) > 0);

    Buffer<Pixel> backBuffer(32, 32);
    Buffer<float> backDepth(32, 32);
    backBuffer.Clear(Pixel{0, 0, 0, 0});
    backDepth.Clear(1.0f);
    std::array<Vertex, 3> backFacing = {MakeVertex(-0.6f, -0.5f, 0.0f), MakeVertex(0.6f, -0.5f, 0.0f),
                                         MakeVertex(0.0f, 0.6f, 0.0f)};
    Rasterizer::DrawTriangle(backBuffer, backDepth, backFacing, config, drawColor);
    REQUIRE(CountPixels(backBuffer, drawColor) == 0);

    Buffer<Pixel> degenerateBuffer(32, 32);
    Buffer<float> degenerateDepth(32, 32);
    degenerateBuffer.Clear(Pixel{0, 0, 0, 0});
    degenerateDepth.Clear(1.0f);
    std::array<Vertex, 3> degenerate = {MakeVertex(-0.8f, -0.8f, 0.0f), MakeVertex(0.0f, 0.0f, 0.0f),
                                         MakeVertex(0.8f, 0.8f, 0.0f)};
    Rasterizer::DrawTriangle(degenerateBuffer, degenerateDepth, degenerate, config, drawColor);
    REQUIRE(CountPixels(degenerateBuffer, drawColor) == 0);
}

TEST_CASE("DDA and Bresenham draw connected horizontal, vertical, and diagonal lines", "[rasterizer][line]") {
    const Pixel lineColor{200, 200, 255, 255};
    const Pixel clearColor{0, 0, 0, 0};
    const Config::RasterizationLineMode modes[] = {Config::RasterizationLineMode::DDA,
                                                    Config::RasterizationLineMode::BRESENHAM};

    for (Config::RasterizationLineMode mode : modes) {
        INFO(((mode == Config::RasterizationLineMode::DDA) ? "DDA" : "Bresenham"));
        Config config = MakeLineConfig(mode);

        Buffer<Pixel> horizontal(32, 32);
        horizontal.Clear(clearColor);
        Rasterizer::DrawLine(horizontal, glm::vec2(2.0f, 10.0f), glm::vec2(20.0f, 10.0f), config, lineColor);
        for (size_t x = 2; x <= 20; x++) {
            REQUIRE(PixelsEqual(horizontal.data[10 * horizontal.width + x], lineColor));
        }

        Buffer<Pixel> vertical(32, 32);
        vertical.Clear(clearColor);
        Rasterizer::DrawLine(vertical, glm::vec2(12.0f, 3.0f), glm::vec2(12.0f, 21.0f), config, lineColor);
        for (size_t y = 3; y <= 21; y++) {
            REQUIRE(PixelsEqual(vertical.data[y * vertical.width + 12], lineColor));
        }

        Buffer<Pixel> diagonal(32, 32);
        diagonal.Clear(clearColor);
        Rasterizer::DrawLine(diagonal, glm::vec2(4.0f, 4.0f), glm::vec2(18.0f, 18.0f), config, lineColor);
        for (size_t step = 4; step <= 18; step++) {
            REQUIRE(PixelsEqual(diagonal.data[step * diagonal.width + step], lineColor));
        }
    }
}

TEST_CASE("Line rasterization is endpoint-order invariant", "[rasterizer][line]") {
    const Pixel lineColor{220, 120, 40, 255};
    const Pixel clearColor{0, 0, 0, 0};
    const Config::RasterizationLineMode modes[] = {Config::RasterizationLineMode::DDA,
                                                    Config::RasterizationLineMode::BRESENHAM};

    for (Config::RasterizationLineMode mode : modes) {
        INFO(((mode == Config::RasterizationLineMode::DDA) ? "DDA" : "Bresenham"));
        Config config = MakeLineConfig(mode);

        Buffer<Pixel> forward(32, 32);
        Buffer<Pixel> reverse(32, 32);
        forward.Clear(clearColor);
        reverse.Clear(clearColor);

        const glm::vec2 p0(3.0f, 6.0f);
        const glm::vec2 p1(25.0f, 17.0f);
        Rasterizer::DrawLine(forward, p0, p1, config, lineColor);
        Rasterizer::DrawLine(reverse, p1, p0, config, lineColor);

        REQUIRE(BuffersEqual(forward, reverse));
    }
}

TEST_CASE("Depth testing preserves the nearest triangle", "[rasterizer]") {
    Buffer<Pixel> framebuffer(32, 32);
    Buffer<float> depthBuffer(32, 32);
    framebuffer.Clear(Pixel{0, 0, 0, 0});
    depthBuffer.Clear(1.0f);

    Config config = MakeBarycentricFillConfig();
    std::array<Vertex, 3> nearTriangle = MakeLargeTriangle(-0.5f);
    std::array<Vertex, 3> farTriangle = MakeLargeTriangle(0.8f);
    const Pixel nearColor{255, 0, 0, 255};
    const Pixel farColor{0, 0, 255, 255};

    Rasterizer::DrawTriangle(framebuffer, depthBuffer, nearTriangle, config, nearColor);
    Rasterizer::DrawTriangle(framebuffer, depthBuffer, farTriangle, config, farColor);

    const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
    REQUIRE(PixelsEqual(framebuffer.data[centerIndex], nearColor));
    REQUIRE(depthBuffer.data[centerIndex] == Catch::Approx(0.25f).margin(0.02f));
    REQUIRE(CountPixels(framebuffer, farColor) == 0);
}

TEST_CASE("Depth test resolves partial overlap per pixel", "[rasterizer][depth]") {
    Config config = MakeBarycentricFillConfig();
    config.cull.depthTest = true;

    const Pixel nearColor{240, 20, 20, 255};
    const Pixel farColor{20, 20, 240, 255};
    const Pixel clearColor{0, 0, 0, 0};
    std::array<Vertex, 3> nearTriangle = {MakeVertex(-0.8f, -0.6f, -0.8f), MakeVertex(0.2f, -0.6f, -0.8f),
                                           MakeVertex(-0.3f, 0.7f, -0.8f)};
    std::array<Vertex, 3> farTriangle = {MakeVertex(-0.2f, -0.6f, 0.8f), MakeVertex(0.8f, -0.6f, 0.8f),
                                          MakeVertex(0.3f, 0.7f, 0.8f)};

    Buffer<Pixel> nearOnly(32, 32);
    Buffer<Pixel> farOnly(32, 32);
    Buffer<float> nearDepth(32, 32);
    Buffer<float> farDepth(32, 32);
    nearOnly.Clear(clearColor);
    farOnly.Clear(clearColor);
    nearDepth.Clear(1.0f);
    farDepth.Clear(1.0f);

    Rasterizer::DrawTriangle(nearOnly, nearDepth, nearTriangle, config, nearColor);
    Rasterizer::DrawTriangle(farOnly, farDepth, farTriangle, config, farColor);

    std::vector<size_t> overlapIndices;
    std::vector<size_t> farOnlyIndices;
    const size_t pixelCount = nearOnly.width * nearOnly.height;
    for (size_t i = 0; i < pixelCount; i++) {
        const bool nearCovered = PixelsEqual(nearOnly.data[i], nearColor);
        const bool farCovered = PixelsEqual(farOnly.data[i], farColor);
        if (nearCovered && farCovered) {
            overlapIndices.push_back(i);
        } else if (!nearCovered && farCovered) {
            farOnlyIndices.push_back(i);
        }
    }
    REQUIRE(!overlapIndices.empty());
    REQUIRE(!farOnlyIndices.empty());

    Buffer<Pixel> combined(32, 32);
    Buffer<float> combinedDepth(32, 32);
    combined.Clear(clearColor);
    combinedDepth.Clear(1.0f);
    Rasterizer::DrawTriangle(combined, combinedDepth, nearTriangle, config, nearColor);
    Rasterizer::DrawTriangle(combined, combinedDepth, farTriangle, config, farColor);

    for (size_t index : overlapIndices) {
        REQUIRE(PixelsEqual(combined.data[index], nearColor));
    }
    for (size_t index : farOnlyIndices) {
        REQUIRE(PixelsEqual(combined.data[index], farColor));
    }
}

TEST_CASE("Disabling depth test makes later triangle overwrite overlap", "[rasterizer][depth]") {
    Config config = MakeBarycentricFillConfig();
    config.cull.depthTest = false;

    const Pixel nearColor{255, 60, 60, 255};
    const Pixel farColor{60, 60, 255, 255};
    const Pixel clearColor{0, 0, 0, 0};
    std::array<Vertex, 3> nearTriangle = {MakeVertex(-0.8f, -0.6f, -0.8f), MakeVertex(0.2f, -0.6f, -0.8f),
                                           MakeVertex(-0.3f, 0.7f, -0.8f)};
    std::array<Vertex, 3> farTriangle = {MakeVertex(-0.2f, -0.6f, 0.8f), MakeVertex(0.8f, -0.6f, 0.8f),
                                          MakeVertex(0.3f, 0.7f, 0.8f)};

    Buffer<Pixel> nearOnly(32, 32);
    Buffer<Pixel> farOnly(32, 32);
    Buffer<float> nearDepth(32, 32);
    Buffer<float> farDepth(32, 32);
    nearOnly.Clear(clearColor);
    farOnly.Clear(clearColor);
    nearDepth.Clear(1.0f);
    farDepth.Clear(1.0f);

    Rasterizer::DrawTriangle(nearOnly, nearDepth, nearTriangle, config, nearColor);
    Rasterizer::DrawTriangle(farOnly, farDepth, farTriangle, config, farColor);

    std::vector<size_t> overlapIndices;
    const size_t pixelCount = nearOnly.width * nearOnly.height;
    for (size_t i = 0; i < pixelCount; i++) {
        if (PixelsEqual(nearOnly.data[i], nearColor) && PixelsEqual(farOnly.data[i], farColor)) {
            overlapIndices.push_back(i);
        }
    }
    REQUIRE(!overlapIndices.empty());

    Buffer<Pixel> combined(32, 32);
    Buffer<float> combinedDepth(32, 32);
    combined.Clear(clearColor);
    combinedDepth.Clear(1.0f);
    Rasterizer::DrawTriangle(combined, combinedDepth, nearTriangle, config, nearColor);
    Rasterizer::DrawTriangle(combined, combinedDepth, farTriangle, config, farColor);

    for (size_t index : overlapIndices) {
        REQUIRE(PixelsEqual(combined.data[index], farColor));
    }
}

TEST_CASE("Point light attenuation darkens identical surfaces with distance", "[rasterizer][lighting]") {
    const auto renderAtDepth = [](float worldZ) {
        Buffer<Pixel> framebuffer(32, 32);
        Buffer<float> depthBuffer(32, 32);
        framebuffer.Clear(Pixel{0, 0, 0, 0});
        depthBuffer.Clear(1.0f);

        Config config = MakeBarycentricFillConfig();
        config.cull.depthTest = false;
        config.environment.lightPosition = glm::vec3(0.0f, 0.0f, 5.0f);

        const SoftwareMaterialState materialState = MakeVertexColorPhongMaterialState();

        std::array<RasterVertex, 3> triangle = {
            MakeLitRasterVertex(-0.6f, -0.5f, 0.0f, worldZ),
            MakeLitRasterVertex(0.0f, 0.6f, 0.0f, worldZ),
            MakeLitRasterVertex(0.6f, -0.5f, 0.0f, worldZ),
        };
        const std::vector<LightSnapshot> noLights;
        Rasterizer::DrawTriangle(
            framebuffer, depthBuffer, triangle, config, noLights, materialState, glm::vec3(0.0f, 0.0f, 3.0f), nullptr);
        const size_t centerIndex = (framebuffer.height / 2) * framebuffer.width + (framebuffer.width / 2);
        return framebuffer.data[centerIndex];
    };

    const Pixel nearPixel = renderAtDepth(0.0f);
    const Pixel midPixel = renderAtDepth(-5.0f);
    const Pixel farPixel = renderAtDepth(-10.0f);

    REQUIRE(nearPixel.a == 255);
    REQUIRE(midPixel.a == 255);
    REQUIRE(farPixel.a == 255);
    REQUIRE(PixelLuminance(nearPixel) > PixelLuminance(midPixel));
    REQUIRE(PixelLuminance(midPixel) > PixelLuminance(farPixel));
}

TEST_CASE("Software material rendering uses the material graph for scanline fill mode", "[rasterizer][material]") {
    Buffer<Pixel> framebuffer(32, 32);
    Buffer<float> depthBuffer(32, 32);
    const Pixel clearColor{0, 0, 0, 0};
    const std::vector<LightSnapshot> noLights;
    const glm::vec3 viewPosition(0.0f, 0.0f, 3.0f);

    Config config{};
    config.cull.backfaceCulling = false;
    config.cull.depthTest = false;
    config.cull.rasterClip = true;
    config.software.rasterizer.polygonMode = Config::RasterizationPolygonMode::FILL;
    config.software.rasterizer.fillMode = Config::RasterizationFillMode::SCANLINE;

    SECTION("vertex color material") {
        framebuffer.Clear(clearColor);
        depthBuffer.Clear(1.0f);

        SoftwareMaterialState materialState = MakeVertexColorUnlitMaterialState();
        MaterialFragmentStageInput fragmentInput{};
        fragmentInput.color0 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        const MaterialFragmentStageOutput fragmentOutput =
            EvaluateMaterialFragmentStage(*materialState.compiledTemplate, materialState.parameterValues, materialState.samplers, fragmentInput);
        REQUIRE(fragmentOutput.baseColor.g == Catch::Approx(1.0f));
        REQUIRE(fragmentOutput.baseColor.r == Catch::Approx(0.0f));
        REQUIRE(fragmentOutput.baseColor.b == Catch::Approx(0.0f));

        std::array<RasterVertex, 3> triangle = {
            MakeLitRasterVertex(-0.7f, -0.6f, 0.0f, 0.0f),
            MakeLitRasterVertex(0.0f, 0.7f, 0.0f, 0.0f),
            MakeLitRasterVertex(0.7f, -0.6f, 0.0f, 0.0f),
        };
        for (RasterVertex& vertex : triangle) {
            vertex.color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }

        Rasterizer::DrawTriangle(framebuffer, depthBuffer, triangle, config, noLights, materialState, viewPosition, nullptr);

        const size_t drawnPixels = CountPixelsIf(framebuffer, [&](const Pixel& pixel) {
            return !PixelsEqual(pixel, clearColor);
        });
        const size_t greenPixels = CountPixelsIf(framebuffer, [](const Pixel& pixel) {
            return pixel.g > 200 && pixel.r < 32 && pixel.b < 32;
        });
        REQUIRE(drawnPixels > 0);
        REQUIRE(greenPixels > 0);
    }

    SECTION("textured material") {
        framebuffer.Clear(clearColor);
        depthBuffer.Clear(1.0f);

        const std::filesystem::path tempDir = MakeTempDir("textured_material");
        const std::filesystem::path texturePath = WriteRedBmp(tempDir);
        Texture texture;
        REQUIRE(texture.LoadFromFile(texturePath.string().c_str()));

        SoftwareMaterialState materialState{};
        materialState.compiledTemplate = MakeTexturedUnlitMaterialTemplate();
        materialState.pipelineState = materialState.compiledTemplate->pipelineState;
        materialState.samplers.push_back(ResolvedMaterialSampler{
            .texture = &texture,
            .filter = MaterialFilterMode::NEAREST,
            .wrapU = MaterialWrapMode::REPEAT,
            .wrapV = MaterialWrapMode::REPEAT,
        });
        MaterialFragmentStageInput fragmentInput{};
        fragmentInput.uv0 = glm::vec2(0.5f, 0.5f);
        const MaterialFragmentStageOutput fragmentOutput =
            EvaluateMaterialFragmentStage(*materialState.compiledTemplate, materialState.parameterValues, materialState.samplers, fragmentInput);
        REQUIRE(fragmentOutput.baseColor.r == Catch::Approx(1.0f).margin(0.01f));
        REQUIRE(fragmentOutput.baseColor.g == Catch::Approx(0.0f).margin(0.01f));
        REQUIRE(fragmentOutput.baseColor.b == Catch::Approx(0.0f).margin(0.01f));

        std::array<RasterVertex, 3> triangle = {
            MakeLitRasterVertex(-0.7f, -0.6f, 0.0f, 0.0f),
            MakeLitRasterVertex(0.0f, 0.7f, 0.0f, 0.0f),
            MakeLitRasterVertex(0.7f, -0.6f, 0.0f, 0.0f),
        };
        triangle[0].texCoords = glm::vec2(0.0f, 0.0f);
        triangle[1].texCoords = glm::vec2(0.5f, 1.0f);
        triangle[2].texCoords = glm::vec2(1.0f, 0.0f);

        Rasterizer::DrawTriangle(framebuffer, depthBuffer, triangle, config, noLights, materialState, viewPosition, nullptr);

        const size_t drawnPixels = CountPixelsIf(framebuffer, [&](const Pixel& pixel) {
            return !PixelsEqual(pixel, clearColor);
        });
        const size_t redPixels = CountPixelsIf(framebuffer, [](const Pixel& pixel) {
            return pixel.r > 200 && pixel.g < 32 && pixel.b < 32;
        });
        REQUIRE(drawnPixels > 0);
        REQUIRE(redPixels > 0);
    }
}

} // namespace RetroRenderer
