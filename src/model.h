#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <array>
#include <glm/ext/vector_float3.hpp>

struct Face {
	std::array<int, 3> positionIndices;
	std::array<int, 3> texIndices;
	std::array<int, 3> normalIndices;
};

class Model {
private:
	std::vector<glm::vec3> mVerts;
	std::vector<Face> mFaces;
public:

	Model();
	Model(const std::string_view filepath);
	~Model();
	int nVerts();
	int nFaces();
	glm::vec3 vert(int ix);
	Face face(int ix);
	std::vector<Face> faces();
};

