#include "Sphere.h"
#include <cmath>

#undef M_PI

namespace Nome::Scene
{

DEFINE_META_OBJECT(CSphere)
{
    BindPositionalArgument(&CSphere::VerticesPerCircle, 1, 0);
    BindPositionalArgument(&CSphere::NumberOfCircles, 1, 1);
    BindPositionalArgument(&CSphere::Radius, 1, 2);
    BindPositionalArgument(&CSphere::Degrees, 1, 3);
}

void CSphere::UpdateEntity()
{
    if (!IsDirty())
        return;

    // Clear mesh
    Super::UpdateEntity();

    int n = static_cast<int>(VerticesPerCircle.GetValue(16.0f));
    int circles = static_cast<int>(NumberOfCircles.GetValue(16.0f));
    float radius = Radius.GetValue(1.0f);
    float degrees = Degrees.GetValue(0.0f);

    float height = 0.f;
    int actualVertices = 0;

    for (int circle = 0; circle < circles; circle++) {
        float height = (radius * circle) / circles;
        for (int i = 0; i < n; i++)
        {
            float theta = (float)i / n * 2.f * (float)tc::M_PI;
            if (theta > degrees)
            {
                break;
            }
            if (circle == 0) {
                AddVertex("v1_" + std::to_string(circle) + "_" + std::to_string(i),
                          { radius * cosf(theta), radius * sinf(theta), 0 });
            }
            else
            {
                float a = sqrt(pow(radius, 2) - pow(height, 2));
                AddVertex("v2_" + std::to_string(circle) + "_" + std::to_string(i),
                          { a * cosf(theta), a * sinf(theta), height });
                AddVertex("v3_" + std::to_string(circle) + "_" + std::to_string(i),
                          { a * cosf(theta), a * sinf(theta), -height });
            }
            actualVertices++;
        }
    }
    

    if (actualVertices != n) 
    {
        AddVertex("v4_0_0",
                  {0, 0, 0 });
    }

    // Create faces
    for (int circle = 0; circle < circles - 1)
    {
        for (int i = 0; i < actualVertices; i++)
        {
            // CCW winding
            // v1_i v1_next
            // v2_i v2_next
            // v3_i v3_next
            int next = (i + 1) % actualVertices;
            if (actualVertices != n && (i == actualVertices - 1)) {
                std::vector<std::string> upperFace = {
                    "v2_" + std::to_string(circle) + "_" + std::to_string(0),
                    "v2_" + std::to_string(circle + 1) + "_" + std::to_string(0), "v4_0_0"
                };
                AddFace("f3_" + std::to_string(circle) + "_" + std::to_string(0), upperFace);

                std::vector<std::string> lowerFace = {
                    "v3_" + std::to_string(circle) + "_" + std::to_string(0),
                    "v3_" + std::to_string(circle + 1) + "_" + std::to_string(0), "v4_0_0"
                };
                AddFace("f4_" + std::to_string(circle) + "_" + std::to_string(0), lowerFace);

                std::vector<std::string> upperFace = {
                    "v2_" + std::to_string(circle) + "_" + std::to_string(i),
                    "v2_" + std::to_string(circle + 1) + "_" + std::to_string(i), "v4_0_0"
                };
                AddFace("f3_" + std::to_string(circle) + "_" + std::to_string(i), upperFace);

                std::vector<std::string> lowerFace = {
                    "v3_" + std::to_string(circle) + "_" + std::to_string(i),
                    "v3_" + std::to_string(circle + 1) + "_" + std::to_string(i), "v4_0_0"
                };
                AddFace("f4_" + std::to_string(circle) + "_" + std::to_string(i), lowerFace);
            }
            else
            {
                std::vector<std::string> upperFace = {
                    "v2_" + std::to_string(circle) + "_" + std::to_string(i),
                    "v2_" + std::to_string(circle + 1) + "_" + std::to_string(i),
                    "v2_" + std::to_string(circle) + "_" + std::to_string(next),
                    "v2_" + std::to_string(circle + 1) + "_" + std::to_string(next)
                };
                AddFace("f1_" + std::to_string(circle) + "_" + std::to_string(i), upperFace);

                std::vector<std::string> lowerFace = {
                    "v3_" + std::to_string(circle) + "_" + std::to_string(i),
                    "v3_" + std::to_string(circle + 1) + "_" + std::to_string(i),
                    "v3_" + std::to_string(circle) + "_" + std::to_string(next),
                    "v3_" + std::to_string(circle + 1) + "_" + std::to_string(next)
                };
                AddFace("f2_" + std::to_string(circle) + "_" + std::to_string(i), lowerFace);
            
            }
        }
    }
    
    // Two caps
    // std::vector<std::string> upperCap, lowerCap;
    // for (int i = 0; i < n; i++)
    //{
    //    upperCap.push_back("v1_" + std::to_string(i));
    //    lowerCap.push_back("v3_" + std::to_string(n - 1 - i));
    //}
    // AddFace("top", upperCap);
    // AddFace("bottom", lowerCap);
}

}
