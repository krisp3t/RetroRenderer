#include "model.h"

Model::Model() {
	mVerts = std::vector<glm::vec3>();
	mFaces = std::vector<std::vector<std::array<int, 3>>>();
}
Model::Model(const std::string_view filepath) {
    std::ifstream infile(filepath.data());
    if (!infile.good()) {
		std::cerr << "Error: cannot open file " << filepath << std::endl;
		return;
	}
	std::string line;
	while (std::getline(infile, line)) {
        std::istringstream iss(line);
        char trash;
        // Vertices
        if (!line.compare(0, 2, "v ")) {
            glm::vec3 vertex;
            iss >> trash >> vertex.x >> vertex.y >> vertex.z;
            mVerts.push_back(vertex);
        }
        // Faces
		else if (!line.compare(0, 2, "f ")) {
            // face is made of usually 3 (triangle) or 4 vertices (quad)
            // each vertex is made of 3 indices: position, texture, normal
			std::vector<std::array<int, 3>> vertices; 
			int ixPosition, ixTexture, ixNormal;
			iss >> trash;
			while (iss >> ixPosition >> trash >> ixTexture >> trash >> ixNormal) {
				ixPosition--; // wavefront indices start at 1, not 0
                ixTexture--;
                ixNormal--;
                std::array<int, 3> vertex { ixPosition, ixTexture, ixNormal };
                vertices.push_back(vertex);
			}
            mFaces.push_back(vertices);
		}
	}
    infile.close();
}

Model::~Model() {
}

int Model::nVerts() {
    return mVerts.size();
}

int Model::nFaces() {
    return mFaces.size();
}

std::vector<std::array<int, 3>> Model::face(int idx) {
    return mFaces[idx];
}

glm::vec3 Model::vert(int i) {
    return mVerts[i];
}
