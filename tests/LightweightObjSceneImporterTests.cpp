#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "Scene/ImportedSceneData.h"
#include "Scene/LightweightObjSceneImporter.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace RetroRenderer {
namespace {
bool LoadObjText(const std::string& objText, ImportedSceneData& outSceneData) {
    LightweightObjSceneImporter importer;
    return importer.LoadFromMemory(reinterpret_cast<const uint8_t*>(objText.data()), objText.size(), outSceneData);
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

} // namespace RetroRenderer
