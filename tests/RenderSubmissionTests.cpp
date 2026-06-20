#include "Renderer/CpuFrame.h"
#include "Renderer/FrameSubmission.h"
#include "Renderer/UiRenderPacket.h"
#include "Window/ImGuiTexture.h"
#include <catch2/catch_test_macros.hpp>
#include <imgui.h>
#include <memory>

namespace RetroRenderer {

TEST_CASE("UiRenderPacket owns captured ImGui geometry", "[renderer][submission]") {
    ImDrawList source(nullptr);
    source.VtxBuffer.resize(3);
    source.IdxBuffer.resize(3);
    source.IdxBuffer[0] = 0;
    source.IdxBuffer[1] = 1;
    source.IdxBuffer[2] = 2;
    source.CmdBuffer.resize(2);
    source.CmdBuffer[0].ClipRect = ImVec4(0.0f, 0.0f, 640.0f, 480.0f);
    source.CmdBuffer[0].TextureId = ToImTextureID(PresentationTexture::Output);
    source.CmdBuffer[0].ElemCount = 3;
    source.CmdBuffer[1].UserCallback = ImDrawCallback_ResetRenderState;
    ImDrawData drawData;
    drawData.Valid = true;
    drawData.DisplaySize = ImVec2(640.0f, 480.0f);
    drawData.FramebufferScale = ImVec2(1.0f, 1.0f);
    drawData.CmdLists.push_back(&source);
    drawData.CmdListsCount = 1;
    drawData.TotalVtxCount = 3;
    drawData.TotalIdxCount = 3;

    UiRenderPacket packet = UiRenderPacket::Capture(&drawData);
    REQUIRE(packet.valid);
    REQUIRE_FALSE(packet.drawLists.empty());
    const size_t capturedVertexCount = packet.drawLists.front().vertices.size();
    REQUIRE(capturedVertexCount > 0);
    REQUIRE(packet.drawLists.front().commands.size() == 2);
    CHECK(packet.drawLists.front().commands.front().texture.value == PresentationTexture::Output.value);
    CHECK(packet.drawLists.front().commands.back().resetRenderState);

    source.VtxBuffer.clear();
    source.IdxBuffer.clear();
    source.CmdBuffer.clear();
    CHECK(packet.drawLists.front().vertices.size() == capturedVertexCount);
}

TEST_CASE("FrameSubmission retains immutable CPU frame ownership", "[renderer][submission]") {
    auto mutableFrame = std::make_shared<CpuFrame>();
    mutableFrame->width = 1;
    mutableFrame->height = 1;
    mutableFrame->pitch = sizeof(Pixel);
    mutableFrame->pixels.push_back(Pixel{1, 2, 3, 4});

    FrameSubmission submission{};
    submission.softwareFrame = mutableFrame;
    mutableFrame.reset();

    REQUIRE(submission.softwareFrame);
    REQUIRE(submission.softwareFrame->pixels.size() == 1);
    CHECK(submission.softwareFrame->pixels.front().r == 1);
}

} // namespace RetroRenderer
