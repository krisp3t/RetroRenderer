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
            // each vertex is made of 3 indices: position, texture, normal
            std::vector<int> positionIndices, texIndices, normalIndices;
			int ixPosition, ixTexture, ixNormal;
			iss >> trash;
            int count = 0;
			while (iss >> ixPosition >> trash >> ixTexture >> trash >> ixNormal) {
                // wavefront indices start at 1, not 0
                positionIndices.push_back(ixPosition - 1);
                texIndices.push_back(ixTexture - 1);
                normalIndices.push_back(ixNormal - 1);
                count++;
			}
            // convert non-triangle faces to triangles, store triangles
            while (count >= 3) {
				mFaces.emplace_back(
                    std::array<int, 3>{positionIndices[0], positionIndices[count - 2], positionIndices[count - 1]},
                    std::array<int, 3>{texIndices[0], texIndices[count - 2], texIndices[count - 1]},
                    std::array<int, 3>{normalIndices[0], normalIndices[count - 2], normalIndices[count - 1]});
				count--;
			}
		}
	}
    infile.close();
}

Model::~Model() {
}

int Model::nVerts() const {
    return mVerts.size();
}

int Model::nFaces() const {
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
