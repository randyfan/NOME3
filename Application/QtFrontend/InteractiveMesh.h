#pragma once
#include "DebugDraw.h"
#include <Scene/RendererInterface.h>
#include <Scene/SceneGraph.h>

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QMaterial>

namespace Nome
{

class CInteractiveMesh : public Qt3DCore::QEntity
{
public:
    explicit CInteractiveMesh(Scene::CSceneTreeNode* node);

    [[nodiscard]] Scene::CSceneTreeNode* GetSceneTreeNode() const { return SceneTreeNode; }

    
    void CreateInteractiveFaces(); // Randy added this
    void UpdateFaceGeometries(bool wireframe); // Randy added this 
    void UpdatePointGeometries(bool PickVertexBool); // Randy added this
    void UpdateFaceMaterials(); // Randy added this. NOT USED
    void UpdateTransform();
    void UpdateGeometry();
    void UpdateMaterial(bool showFacets);
    void InitInteractions();
    void SetDebugDraw(const CDebugDraw* debugDraw);

private:
    Scene::CSceneTreeNode* SceneTreeNode = nullptr;

    Qt3DCore::QTransform* Transform = nullptr;
    Qt3DRender::QGeometry* Geometry = nullptr;
    Qt3DRender::QGeometryRenderer* GeometryRenderer = nullptr;
    Qt3DRender::QMaterial* Material = nullptr;
    Qt3DRender::QMaterial* LineMaterial = nullptr;

    Qt3DCore::QEntity* PointEntity;
    Qt3DRender::QMaterial* PointMaterial;
    Qt3DRender::QGeometry* PointGeometry;
    Qt3DRender::QGeometryRenderer* PointRenderer;

    // Randy TODO: instead of one QMaterial for entire mesh, we should have a Material for each face. This doesn't interfere with the topology and would allow specific face coloring
    std::vector<Qt3DCore::QEntity*> interactivefaces;
    std::vector<Qt3DRender::QGeometry*> facegeometries;
    std::vector<Qt3DRender::QGeometryRenderer*> facerenderers;
    std::vector<Qt3DRender::QMaterial*> facematerials;

};

}
