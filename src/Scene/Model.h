#pragma once
#include "Mesh.h"

namespace RetroRenderer
{

class Model
{
public:
    Model(Mesh* mesh) : m_Mesh(mesh) {}
    ~Model() = default;
    Mesh* GetMesh() const { return m_Mesh; }
private:
    Mesh* m_Mesh = nullptr;
};

}
