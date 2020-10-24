#pragma once
#include "Mesh.h"

namespace Nome::Scene
{

class CSphere : public CMesh
{
    DEFINE_INPUT(float, VerticesPerCircle) { MarkDirty(); }
    DEFINE_INPUT(float, NumberOfCircles) { MarkDirty(); }
    DEFINE_INPUT(float, Radius) { MarkDirty(); }
    DEFINE_INPUT(float, Degrees) { MarkDirty(); }

public:
    DECLARE_META_CLASS(CSphere, CMesh);
    CSphere() = default;
    CSphere(const std::string& name)
        : CMesh(std::move(name))
    {
    }

    void UpdateEntity() override;
};

}