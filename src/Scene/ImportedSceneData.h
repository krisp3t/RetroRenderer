#pragma once

#include "Vertex.h"
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace RetroRenderer {

struct ImportedMaterial {
    glm::vec3 diffuseColor = glm::vec3(1.0f);
    glm::vec3 specularColor = glm::vec3(0.0f);
};

struct ImportedMesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::optional<int> materialIndex;
};

struct ImportedNode {
    std::string name;
    glm::mat4 localTransform = glm::mat4(1.0f);
    std::vector<int> meshIndices;
    std::vector<int> childNodeIndices;
};

struct ImportedSceneData {
    std::vector<ImportedNode> nodes;
    std::vector<ImportedMesh> meshes;
    std::vector<ImportedMaterial> materials;
    int rootNodeIndex = -1;
};

} // namespace RetroRenderer
