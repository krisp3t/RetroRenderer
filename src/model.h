#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <array>
#include <glm/ext/vector_float3.hpp>

class Model {
private:
	std::vector<glm::vec3> mVerts;
	std::vector<std::vector<std::array<int, 3>>> mFaces;
public:
	Model();
	Model(const std::string_view filepath);
	~Model();
	int nVerts();
	int nFaces();
	glm::vec3 vert(int ix);
	std::vector<std::array<int, 3>> face(int ix);
};

