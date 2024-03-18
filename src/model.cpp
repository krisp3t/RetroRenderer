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
            // we only support triangles (not quads)
            // each vertex is made of 3 indices: position, texture, normal
            std::array<int, 3> positionIndices, texIndices, normalIndices;
			int ixPosition, ixTexture, ixNormal;
			iss >> trash;
            int count = 0;
			while (iss >> ixPosition >> trash >> ixTexture >> trash >> ixNormal) {
                if (count >= 3) {
					std::cerr << "Error: renderer only supports triangles in obj file" << std::endl;
					return;
                }
                // wavefront indices start at 1, not 0
                positionIndices[count] = --ixPosition;
                texIndices[count] = --ixTexture;
                normalIndices[count] = --ixNormal;
                count++;
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
