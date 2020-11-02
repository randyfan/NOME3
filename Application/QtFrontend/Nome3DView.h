#pragma once
#include "DebugDraw.h"
#include "InteractiveMesh.h"
#include <Ray.h>
#include <Scene/Scene.h>

#include <Qt3DExtras>

#include <unordered_map>
#include <unordered_set>

namespace Nome
{

class CNome3DView : public Qt3DExtras::Qt3DWindow
{
public:
    CNome3DView();
    ~CNome3DView() override;

    [[nodiscard]] const std::vector<std::string>& GetSelectedVertices() const
    {
        return SelectedVertices;
    }

    // Randy added on 10/14 for face selection
    [[nodiscard]] const std::vector<std::string>& GetSelectedFaces() const
    {
        return SelectedFaces;
    }

    void ClearSelectedVertices(); // Randy added on 9/27
    void ClearSelectedFaces(); // Randy added on 10/14 for deselecting faces
    void TakeScene(const tc::TAutoPtr<Scene::CScene>& scene);
    void UnloadScene();
    void PostSceneUpdate();

    void PickVertexWorldRay(const tc::Ray& ray);
    void PickFaceWorldRay(const tc::Ray& ray); // Randy added on 10/10
    void PickEdgeWorldRay(const tc::Ray& ray); // Randy added on 10/29

    bool WireFrameMode = true; // Randy added on 10/16 for choose wireframe mode or default mode
    static Qt3DCore::QEntity* MakeGridEntity(Qt3DCore::QEntity* parent);

private:
    Qt3DCore::QEntity* Root;
    tc::TAutoPtr<Scene::CScene> Scene;
    std::unordered_set<CInteractiveMesh*> InteractiveMeshes;
    std::unordered_map<Scene::CEntity*, CDebugDraw*> EntityDrawData;
    std::vector<std::string> SelectedVertices;
    std::vector<std::string> SelectedFaces; // Randy added on 10/10
    //std::vector<const & std::vector<std::string>> SelectedEdgeVertPositions; // Randy added on 10/29. Temporary solution. TODO: Introduce Edge names and handles
};

}