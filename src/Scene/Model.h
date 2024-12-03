#pragma once
#include "Mesh.h"

namespace RetroRenderer
{

class Model
{
public:
    Model(Mesh* mesh) : m_Mesh(mesh) {}
    ~Model() = default;
private:
    Mesh* m_Mesh = nullptr;
};

}
