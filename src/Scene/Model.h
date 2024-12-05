#pragma once
#include "Mesh.h"

namespace RetroRenderer
{

class Model
{
public:
    Model(Mesh& mesh);
    ~Model() = default;
    Mesh& GetMesh() const;
private:
    Mesh* m_Mesh = nullptr;
};

}
