#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Scene/ImportedSceneData.h"
#include "Scene/LightweightObjSceneImporter.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace RetroRenderer {
namespace {
bool LoadObjText(const std::string& objText, ImportedSceneData& outSceneData) {
    LightweightObjSceneImporter importer;
    return importer.LoadFromMemory(reinterpret_cast<const uint8_t*>(objText.data()), objText.size(), outSceneData);
}

std::filesystem::path MakeTempDir(const char* name) {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "retrorenderer_obj_importer_tests" / name;
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}
} // namespace

TEST_CASE("OBJ importer loads a minimal triangle with computed normals", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));
    REQUIRE(sceneData.rootNodeIndex == 0);
    REQUIRE(sceneData.nodes.size() == 1);
    REQUIRE(sceneData.meshes.size() == 1);

    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.vertices.size() == 3);
    REQUIRE(mesh.indices.size() == 3);
    REQUIRE(sceneData.nodes[0].meshIndices.size() == 1);
    REQUIRE(sceneData.nodes[0].meshIndices[0] == 0);

    REQUIRE(mesh.vertices[0].position.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].position.y == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].position.z == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].normal.z == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[0].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].texCoords.y == Catch::Approx(0.0f));
}

TEST_CASE("OBJ importer triangulates polygon faces and preserves texcoords", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 1 1\n"
        "vt 0 1\n"
        "f 1/1 2/2 3/3 4/4\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));
    REQUIRE(sceneData.meshes.size() == 1);

    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.vertices.size() == 6);
    REQUIRE(mesh.indices.size() == 6);
    REQUIRE(mesh.indices[0] == 0);
    REQUIRE(mesh.indices[1] == 1);
    REQUIRE(mesh.indices[2] == 2);
    REQUIRE(mesh.indices[3] == 3);
    REQUIRE(mesh.indices[4] == 4);
    REQUIRE(mesh.indices[5] == 5);

    REQUIRE(mesh.vertices[0].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].texCoords.y == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[1].texCoords.x == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[1].texCoords.y == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[2].texCoords.x == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[2].texCoords.y == Catch::Approx(1.0f));
}

TEST_CASE("OBJ importer returns false when no faces exist", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n";

    ImportedSceneData sceneData{};
    REQUIRE_FALSE(LoadObjText(objText, sceneData));
}

TEST_CASE("OBJ importer supports negative indices", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "f -3/-3/-1 -2/-2/-1 -1/-1/-1\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));
    REQUIRE(sceneData.meshes.size() == 1);

    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.vertices.size() == 3);
    REQUIRE(mesh.indices.size() == 3);
    REQUIRE(mesh.vertices[0].position.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[1].position.x == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[2].position.y == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[0].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[1].texCoords.x == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[2].texCoords.y == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[0].normal.z == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[1].normal.z == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[2].normal.z == Catch::Approx(1.0f));
}

TEST_CASE("OBJ importer handles mixed face token formats", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vt 1 0\n"
        "vt 0 1\n"
        "vn 0 0 1\n"
        "f 1 2 3\n"
        "f 1/1 2/2 3/3\n"
        "f 1//1 2//1 3//1\n"
        "f 1/1/1 2/2/1 3/3/1\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));
    REQUIRE(sceneData.meshes.size() == 1);

    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.vertices.size() == 12);
    REQUIRE(mesh.indices.size() == 12);

    REQUIRE(mesh.vertices[0].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].texCoords.y == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[0].normal.z == Catch::Approx(1.0f));

    REQUIRE(mesh.vertices[3].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[4].texCoords.x == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[5].texCoords.y == Catch::Approx(1.0f));

    REQUIRE(mesh.vertices[6].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[6].texCoords.y == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[6].normal.z == Catch::Approx(1.0f));

    REQUIRE(mesh.vertices[9].texCoords.x == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[10].texCoords.x == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[11].texCoords.y == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[9].normal.z == Catch::Approx(1.0f));
}

TEST_CASE("OBJ importer skips malformed faces and can still load later valid faces", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1/1 2/bad 3/1\n"
        "f 1 2 3\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));
    REQUIRE(sceneData.meshes.size() == 1);
    REQUIRE(sceneData.meshes[0].vertices.size() == 3);
    REQUIRE(sceneData.meshes[0].indices.size() == 3);
}

TEST_CASE("OBJ importer returns false on invalid references when no valid faces remain", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 99\n";

    ImportedSceneData sceneData{};
    REQUIRE_FALSE(LoadObjText(objText, sceneData));
}

TEST_CASE("OBJ importer preserves optional vertex colors", "[importer][obj]") {
    const std::string objText =
        "v 0 0 0 1 0 0\n"
        "v 1 0 0 0 1 0\n"
        "v 0 1 0 0 0 1\n"
        "f 1 2 3\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));
    REQUIRE(sceneData.meshes.size() == 1);

    const ImportedMesh& mesh = sceneData.meshes[0];
    REQUIRE(mesh.vertices.size() == 3);
    REQUIRE(mesh.vertices[0].color.r == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[0].color.g == Catch::Approx(0.0f));
    REQUIRE(mesh.vertices[1].color.g == Catch::Approx(1.0f));
    REQUIRE(mesh.vertices[2].color.b == Catch::Approx(1.0f));
}

TEST_CASE("OBJ importer loads material libraries and splits meshes on usemtl", "[importer][obj]") {
    const std::filesystem::path tempDir = MakeTempDir("materials");
    const std::filesystem::path objPath = tempDir / "scene.obj";
    const std::filesystem::path mtlPath = tempDir / "scene.mtl";

    {
        std::ofstream objFile(objPath);
        REQUIRE(objFile.is_open());
        objFile << "mtllib scene.mtl\n"
                   "v -1 -1 0\n"
                   "v  0 -1 0\n"
                   "v  0  1 0\n"
                   "v -1  1 0\n"
                   "v  0 -1 0\n"
                   "v  1 -1 0\n"
                   "v  1  1 0\n"
                   "v  0  1 0\n"
                   "vt 0 0\n"
                   "vt 1 0\n"
                   "vt 1 1\n"
                   "vt 0 1\n"
                   "usemtl left\n"
                   "f 1/1 2/2 3/3 4/4\n"
                   "usemtl right\n"
                   "f 5/1 6/2 7/3 8/4\n";
    }

    {
        std::ofstream mtlFile(mtlPath);
        REQUIRE(mtlFile.is_open());
        mtlFile << "newmtl left\n"
                   "Kd 1.0 0.0 0.0\n"
                   "map_Kd left.bmp\n"
                   "\n"
                   "newmtl right\n"
                   "Kd 0.0 1.0 0.0\n"
                   "map_Kd right.bmp\n";
    }

    LightweightObjSceneImporter importer;
    ImportedSceneData sceneData{};
    REQUIRE(importer.LoadFromFile(objPath.string(), sceneData));

    REQUIRE(sceneData.materials.size() == 2);
    REQUIRE(sceneData.materials[0].diffuseTexturePath == "left.bmp");
    REQUIRE(sceneData.materials[1].diffuseTexturePath == "right.bmp");
    REQUIRE(sceneData.meshes.size() == 2);
    REQUIRE(sceneData.meshes[0].materialIndex.has_value());
    REQUIRE(sceneData.meshes[0].materialIndex.value() == 0);
    REQUIRE(sceneData.meshes[1].materialIndex.has_value());
    REQUIRE(sceneData.meshes[1].materialIndex.value() == 1);
    REQUIRE(sceneData.sourceDirectory == tempDir.string());
}

TEST_CASE("OBJ importer creates child nodes for named objects", "[importer][obj]") {
    const std::string objText =
        "o left\n"
        "v -1 0 0\n"
        "v  0 0 0\n"
        "v -1 1 0\n"
        "f 1 2 3\n"
        "o right\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "f 4 5 6\n";

    ImportedSceneData sceneData{};
    REQUIRE(LoadObjText(objText, sceneData));

    REQUIRE(sceneData.rootNodeIndex == 0);
    REQUIRE(sceneData.nodes.size() == 3);
    REQUIRE(sceneData.nodes[0].childNodeIndices.size() == 2);
    REQUIRE(sceneData.nodes[1].name == "left");
    REQUIRE(sceneData.nodes[2].name == "right");
    REQUIRE(sceneData.nodes[1].meshIndices.size() == 1);
    REQUIRE(sceneData.nodes[2].meshIndices.size() == 1);
}

} // namespace RetroRenderer
