#include "model.h"

Model::Model() {
	mVerts = std::vector<glm::vec3>();
    mFaces = std::vector<Face>();
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
            mVerts.emplace_back(std::move(vertex));
        }
        // Faces
		else if (!line.compare(0, 2, "f ")) {
            // face is made of usually 3 (triangle) or 4 vertices (quad)
            // each vertex is made of 3 indices: position, texture, normal
			std::vector<int> positionIndices, texIndices, normalIndices;
			int ixPosition, ixTexture, ixNormal;
			iss >> trash;
			while (iss >> ixPosition >> trash >> ixTexture >> trash >> ixNormal) {
                ixPosition--; ixTexture--; ixNormal--; // wavefront indices start at 1, not 0
                positionIndices.push_back(ixPosition);
                texIndices.push_back(ixTexture);
                normalIndices.push_back(ixNormal);
			}
            mFaces.emplace_back(std::move(positionIndices), std::move(texIndices), std::move(normalIndices));
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

Face Model::face(int idx) {
    return mFaces[idx];
}

std::vector<Face> Model::faces() {
    return mFaces;
}

glm::vec3 Model::vert(int i) {
    return mVerts[i];
}
