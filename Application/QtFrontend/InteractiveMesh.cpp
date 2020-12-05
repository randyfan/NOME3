#include "InteractiveMesh.h"
#include "FrontendContext.h"
#include "MaterialParser.h"
#include "MeshToQGeometry.h"
#include "FaceToQGeometry.h" // Randy added on 10/12
#include "Nome3DView.h"
#include "ResourceMgr.h"
#include <Matrix3x4.h>
#include <Scene/Mesh.h>

#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QObjectPicker>
#include <Qt3DRender/QPickEvent>

namespace Nome
{

CInteractiveMesh::CInteractiveMesh(Scene::CSceneTreeNode* node)
    : SceneTreeNode(node)
    , PointEntity {}
    , PointMaterial {}
    , PointGeometry {}
    , PointRenderer {}
{
    
    // UpdateTransform();

    //CreateInteractiveFaces(); // Randy added this on 10/12. can use the Mesh's transform for each face. Commented out on 12/2
    //UpdateFaceGeometries(true); // currently material logic is in here also. Commented out on 12/2. Too laggy
    // UpdatePointGeometries();// Randy added this . // Randy commented out on 11/26 to remove vertices at load

    // // // // / /UpdateFaceMaterials(); // test this later. default should be gray face
    // probably don't need InitInteractiosn again?



    UpdateGeometry();
    UpdateMaterial(false); // don't show facets by default
    InitInteractions();
}

// Randy added this on 10/12
void CInteractiveMesh::CreateInteractiveFaces() {
    auto* entity = SceneTreeNode->GetInstanceEntity();
    if (!entity)
    {
        entity = SceneTreeNode->GetOwner()->GetEntity();
        
    }
    if (entity)
    {
        auto* meshInstance = dynamic_cast<Scene::CMeshInstance*>(entity);
        if (meshInstance)
        {
            auto openmesh = meshInstance->GetMeshImpl();
            CMeshImpl::FaceIter fIter, fEnd = openmesh.faces_end();

            for (fIter = openmesh.faces_sbegin(); fIter != fEnd; ++fIter)
            {
                auto * interactiveface = new Qt3DCore::QEntity(this); // change to "this" as root fixed bug 
                                                                //auto facegeometry = new Qt3DRender::QGeometry();
                //auto facerenderer = new Qt3DRender::QGeometryRenderer();
                //auto facematerial = new Qt3DRender::QMaterial();
                interactivefaces.push_back(interactiveface); // need individual faces to allow for face selection
                //facegeometries.push_back(facegeometry);
                //facerenderers.push_back(facerenderer);
                //facematerials.push_back(facematerial);
            }
        }
        else
        {
            // The entity is not a mesh instance, we don't know how to handle it. For example, if
            // you try to instanciate a face, it'll generate this placeholder sphere.

        }
    }

}


void CInteractiveMesh::UpdatePointGeometries(bool PickVertexBool) { 
 // Update or create the entity for drawing vertices


    auto* entity = SceneTreeNode->GetInstanceEntity();
    if (!entity)
    {
        entity = SceneTreeNode->GetOwner()->GetEntity();
    }
    if (entity)
    {
        auto* meshInstance = dynamic_cast<Scene::CMeshInstance*>(entity);
        auto openmesh = meshInstance->GetMeshImpl();

        auto selectedfacehandles = meshInstance->GetSelectedFaceHandles(); // Added on 12/2. Probably not needed for point geometry, but adding because it is a required argument
        CMeshToQGeometry meshToQGeometry(meshInstance->GetMeshImpl(), selectedfacehandles, true); // This may be causing bugs Randy 
        if (!PointEntity || PickVertexBool)
        {
            std::string xmlPath = "";
            PointEntity = new Qt3DCore::QEntity(this);
            if (PickVertexBool)
            {
                delete PointRenderer;
                delete PointGeometry;
                xmlPath = CResourceMgr::Get().Find(
                    "DebugDrawLinev2.xml"); // vert box size = 6
            }
            else {
                xmlPath = CResourceMgr::Get().Find(
                    "DebugDrawLine.xml"); // this uses instanceColor, and also uses
                                            // LineShading.frag (which is used for polylines) for
                                            // final color
            }
            auto* lineMat = new CXMLMaterial(QString::fromStdString(xmlPath));
            PointMaterial = lineMat;
            PointMaterial->setParent(this);
            PointEntity->addComponent(PointMaterial);
        }
        else
        {
            std::string xmlPath = "";
            PointEntity = new Qt3DCore::QEntity(this);
            delete PointRenderer;
            delete PointGeometry;
            xmlPath = CResourceMgr::Get().Find(
                "DebugDrawLine.xml"); // this uses instanceColor, and also uses
                                      // LineShading.frag (which is used for polylines) for
                                      // final color
            auto* lineMat = new CXMLMaterial(QString::fromStdString(xmlPath));
            PointMaterial = lineMat;
            PointMaterial->setParent(this);
            PointEntity->addComponent(PointMaterial);

        }
        PointGeometry = meshToQGeometry.GetPointGeometry();
        PointGeometry->setParent(PointEntity);
        PointRenderer = new Qt3DRender::QGeometryRenderer(PointEntity);
        PointRenderer->setGeometry(PointGeometry);
        PointRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Points);
        PointEntity->addComponent(PointRenderer);
    }
}

void CInteractiveMesh::UpdateFaceGeometries(bool wireframe)
{
    auto* entity = SceneTreeNode->GetInstanceEntity();
    if (!entity)
        entity = SceneTreeNode->GetOwner()->GetEntity();

    if (entity)
    {
        auto* meshInstance = dynamic_cast<Scene::CMeshInstance*>(entity);
        auto openmesh = meshInstance->GetMeshImpl();
        if (meshInstance)
        {
            delete GeometryRenderer; // I don't think is needed? this is not used here
            delete Geometry;  // I dont think this is needed? this is not used here
            CMeshImpl::FaceIter fIter, fEnd = openmesh.faces_end();
            int counter = 0;

            for (auto facerender : facerenderers)
                delete facerender;
            facerenderers.clear();
            for (auto facegeometry : facegeometries)
                delete facegeometry;

            facegeometries.clear();



            //Get face color
            QVector3D instanceColor {1.0f, 0.5f, 0.0f}; // orange color
               // If the scene tree node is not within a group, then we can directly use its
            // surface color
            if (!SceneTreeNode->GetParent()->GetOwner()->IsGroup())
            {
                if (auto surface = SceneTreeNode->GetOwner()->GetSurface())
                {
                    instanceColor.setX(surface->ColorR.GetValue(1.0f));
                    instanceColor.setY(surface->ColorG.GetValue(1.0f));
                    instanceColor.setZ(surface->ColorB.GetValue(1.0f));
                }
            }
            else // else, the scenetreenode is within a group, and we keep bubbling up from
                 // where we are (going up the tree) until we get to an instance scene node
                 // that has a surface color
            {
                bool setColor = false;
                auto currNode = SceneTreeNode;
                while (currNode->GetParent()->GetOwner()->IsGroup())
                { // while currNode is within a group
                    if (auto surface = currNode->GetOwner()->GetSurface())
                    { // if the currNode itself is assigned a surface color, then this color
                      // is prioritzed. we set the color and break.
                        instanceColor.setX(surface->ColorR.GetValue(1.0f));
                        instanceColor.setY(surface->ColorG.GetValue(1.0f));
                        instanceColor.setZ(surface->ColorB.GetValue(1.0f));
                        setColor = true;
                        break;
                    }
                    currNode = currNode->GetParent();
                }

                if (!setColor) // If the surface color hasn't been set yet
                {
                    currNode = currNode->GetParent(); // here, currNode's parent is
                                                      // guaranteed to be a instance scene
                                                      // tree node due to previous while loop

                    if (auto surface = currNode->GetOwner()->GetSurface())
                    {
                        instanceColor.setX(surface->ColorR.GetValue(1.0f));
                        instanceColor.setY(surface->ColorG.GetValue(1.0f));
                        instanceColor.setZ(surface->ColorB.GetValue(1.0f));
                    }
                }
            }

            
            for (fIter = openmesh.faces_sbegin(); fIter != fEnd; ++fIter)
            {
                auto interactiveface = interactivefaces[counter];

                // Commenting this out fixed the transform bug? Why is this not needed?
                //auto * facetransform = new Qt3DCore::QTransform(this); 
                //interactiveface->addComponent(facetransform);
                //const auto& tf = SceneTreeNode->L2WTransform.GetValue(tc::Matrix3x4::IDENTITY);
                //QMatrix4x4 qtf { tf.ToMatrix4().Data() };
                //facetransform->setMatrix(qtf);

                auto fH = *fIter;
                auto selectedfacehandles = meshInstance->GetSelectedFaceHandles();

                // If this fH is selected, create a unique material for it
                auto iter = std::find(selectedfacehandles.begin(), selectedfacehandles.end(), fH);
                if (iter != selectedfacehandles.end()) {
                    NomeFace::CFaceToQGeometry faceToQGeometry(openmesh, fH, false);// must set to false to avoid generating all mesh points for every face
                    auto* facegeometry = faceToQGeometry.GetGeometry();
                    facegeometries.push_back(facegeometry); // Randy added this so can keep track of facegeometry and clear at the end
                    facegeometry->setParent(interactiveface);
                    auto* facerender = new Qt3DRender::QGeometryRenderer(interactiveface);
                    facerenderers.push_back(facerender); // Randy added this so can keep track of
                                                            // facerenders and clear at the end
                    facerender->setGeometry(facegeometry);
                    facerender->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
                    interactiveface->addComponent(facerender); // adding geometry data to interactive face
                    std::string xmlPath;
                    if (wireframe)
                        xmlPath = CResourceMgr::Get().Find("WireFrameLit.xml");
                    else
                        xmlPath = CResourceMgr::Get().Find("RandyLit.xml");
                    auto* mat = new CXMLMaterial(QString::fromStdString(xmlPath));
                    interactiveface->addComponent(mat);
                    mat->FindParameterByName("kd")->setValue(QVector3D(1, 0, 1)); // Pink if selected
                }
                else // else, use the instanceColor
                {
                    NomeFace::CFaceToQGeometry faceToQGeometry(openmesh, fH, false); // must set to false to avoid generating all mesh points for every face
                    auto* facegeometry = faceToQGeometry.GetGeometry();
                    facegeometry->setParent(interactiveface);
                    auto* facerender = new Qt3DRender::QGeometryRenderer(interactiveface);
                    facerender->setGeometry(facegeometry);
                    facerender->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
                    interactiveface->addComponent(facerender); // adding geometry data to interactive face
                    std::string xmlPath;
                    if (wireframe)
                    {
                        xmlPath = CResourceMgr::Get().Find("WireFrameLit.xml");
                    }
                    else
                    {
                        xmlPath = CResourceMgr::Get().Find("RandyLit.xml");
                    }
                    auto* mat = new CXMLMaterial(QString::fromStdString(xmlPath));
                    interactiveface->addComponent(mat);

                    mat->FindParameterByName("kd")->setValue(instanceColor); // color rgb ranges from 0 to 1
                        //mat->FindParameterByName("kd")->setValue(QVector3D(1 - .15 * counter, 0, 0)); // (20*counter, 0, 255*counter));
                }
                counter += 1;
            }
        }
    }
}

void CInteractiveMesh::UpdateTransform()
{
    if (!Transform)
    {
        Transform = new Qt3DCore::QTransform(this);
        this->addComponent(Transform);
    }
    const auto& tf = SceneTreeNode->L2WTransform.GetValue(tc::Matrix3x4::IDENTITY);
    QMatrix4x4 qtf { tf.ToMatrix4().Data() };
    Transform->setMatrix(qtf);
}

void CInteractiveMesh::UpdateGeometry()
{


    auto* entity = SceneTreeNode->GetInstanceEntity();
    if (!entity)
    {
        entity = SceneTreeNode->GetOwner()->GetEntity();
    }

    if (entity)
    {
        auto* meshInstance = dynamic_cast<Scene::CMeshInstance*>(entity);
        if (meshInstance)
        {
            delete GeometryRenderer; // Randy note: this may not be needed 
            delete Geometry; // Randy note: this may not be needed
            // A Qt3DRender::QGeometry class is used to group a list of Qt3DRender::QAttribute objects together to form a geometric shape Qt3D is able to render using Qt3DRender::QGeometryRenderer. 
            auto selectedfacehandles = meshInstance->GetSelectedFaceHandles(); // Randy added on 12/3
            CMeshToQGeometry meshToQGeometry(meshInstance->GetMeshImpl(), selectedfacehandles, true); // Randy added 2nd argument on 12/3
            Geometry = meshToQGeometry.GetGeometry();
            Geometry->setParent(this);
            GeometryRenderer = new Qt3DRender::QGeometryRenderer(this);
            GeometryRenderer->setGeometry(Geometry);
            GeometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
            this->addComponent(GeometryRenderer); // adding geometry data to interactive mesh

            // Update or create the entity for drawing vertices
            if (!PointEntity) 
            {
                PointEntity = new Qt3DCore::QEntity(this);

                auto xmlPath = CResourceMgr::Get().Find("DebugDrawLine.xml"); //this uses instanceColor, and also uses LineShading.frag (which is used for polylines) for final color
                auto* lineMat = new CXMLMaterial(QString::fromStdString(xmlPath));
                PointMaterial = lineMat;
                PointMaterial->setParent(this);
                PointEntity->addComponent(PointMaterial);
            }
            else
            {
                delete PointRenderer;
                delete PointGeometry;
            }
            PointGeometry = meshToQGeometry.GetPointGeometry();
            PointGeometry->setParent(PointEntity);
            PointRenderer = new Qt3DRender::QGeometryRenderer(PointEntity);
            PointRenderer->setGeometry(PointGeometry);
            PointRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Points);
            PointEntity->addComponent(PointRenderer);
        }
        else
        {
            // The entity is not a mesh instance, we don't know how to handle it. For example, if you try to instanciate a face, it'll generate this placeholder sphere.
            auto* vPlaceholder = new Qt3DExtras::QSphereMesh(this);
            vPlaceholder->setRadius(1.0f);
            vPlaceholder->setRings(16);
            vPlaceholder->setSlices(16);
            this->addComponent(vPlaceholder);
        }
    }
}

void CInteractiveMesh::UpdateMaterial(bool showFacets)
{
    QVector3D instanceColor { 1.0f, 0.5f, 0.1f }; // orange color

    // If the scene tree node is not within a group, then we can directly use its surface color
    if (!SceneTreeNode->GetParent()->GetOwner()->IsGroup()) 
    {
        if (auto surface = SceneTreeNode->GetOwner()->GetSurface()) 
        {
            instanceColor.setX(surface->ColorR.GetValue(1.0f));
            instanceColor.setY(surface->ColorG.GetValue(1.0f));
            instanceColor.setZ(surface->ColorB.GetValue(1.0f));
        }
    }
    else // else, the scenetreenode is within a group, and we keep bubbling up from where we are (going up the tree) until we get to an instance scene node that has a surface color
    {
        bool setColor = false;
        auto currNode = SceneTreeNode;
        while(currNode->GetParent()->GetOwner()->IsGroup()) { //while currNode is within a group
            if (auto surface = currNode->GetOwner()->GetSurface()) {  // if the currNode itself is assigned a surface color, then this color is prioritzed. we set the color and break.
                instanceColor.setX(surface->ColorR.GetValue(1.0f));
                instanceColor.setY(surface->ColorG.GetValue(1.0f));
                instanceColor.setZ(surface->ColorB.GetValue(1.0f));
                setColor = true;
                break;
            }
            currNode = currNode->GetParent();
        }

        if (!setColor) // If the surface color hasn't been set yet
        {
            currNode = currNode->GetParent(); //here, currNode's parent is guaranteed to be a instance scene tree node due to previous while loop

            if (auto surface = currNode->GetOwner()->GetSurface())
            {
                instanceColor.setX(surface->ColorR.GetValue(1.0f));
                instanceColor.setY(surface->ColorG.GetValue(1.0f));
                instanceColor.setZ(surface->ColorB.GetValue(1.0f));
            }
        }
    }

    if (!Material)
    {
        auto xmlPath = CResourceMgr::Get().Find("WireframeLit.xml");
        auto* mat = new CXMLMaterial(QString::fromStdString(xmlPath));
        this->addComponent(mat);
        Material = mat;
    }
    auto* mat = dynamic_cast<CXMLMaterial*>(Material);

    mat->FindParameterByName("kd")->setValue(instanceColor); 
    if (showFacets) {
        mat->FindParameterByName("showFacets")->setValue(1);
    }
    else
    {
        mat->FindParameterByName("showFacets")->setValue(0);
    }


    // Use non-default line color only if the instance has a surface
    auto surface = SceneTreeNode->GetOwner()->GetSurface();
    if (LineMaterial && surface)
    {
        auto* lineMat = dynamic_cast<CXMLMaterial*>(LineMaterial);
        lineMat->FindParameterByName("instanceColor")->setValue(instanceColor);
    }
}

void CInteractiveMesh::InitInteractions()
{
    // Only mesh instances support vertex picking
    auto* mesh = dynamic_cast<Scene::CMeshInstance*>(SceneTreeNode->GetInstanceEntity());
    if (!mesh)
        return;

    auto* picker = new Qt3DRender::QObjectPicker(this);
    picker->setHoverEnabled(true);
    connect(picker, &Qt3DRender::QObjectPicker::pressed, [](Qt3DRender::QPickEvent* pick) {
        if (pick->button() == Qt3DRender::QPickEvent::LeftButton)
        {
            //if (pick->modifiers() & Qt::ShiftModifier)
            //{
                const auto& wi = pick->worldIntersection();
                const auto& origin = GFrtCtx->NomeView->camera()->position();
                auto dir = wi - origin;

                tc::Ray ray({ origin.x(), origin.y(), origin.z() }, { dir.x(), dir.y(), dir.z() });
     
                if (GFrtCtx->NomeView->PickVertexBool)
                    GFrtCtx->NomeView->PickVertexWorldRay(ray);
                if (GFrtCtx->NomeView->PickEdgeBool)
                    GFrtCtx->NomeView->PickEdgeWorldRay(ray); // Randy added on 10/29 for edge selection
                if (GFrtCtx->NomeView->PickFaceBool)
                    GFrtCtx->NomeView->PickFaceWorldRay(
                    ray); // Randy added on 10/10 for face selection. 
                //Warning, order affects display messages. Fix later.
            //}

        }
    });
    this->addComponent(picker);
}

void CInteractiveMesh::SetDebugDraw(const CDebugDraw* debugDraw)
{
    //std::cout << "inside set debug draw" << std::endl;
    // Check for existing lineEntity and delete
    auto* oldEntity = this->findChild<Qt3DCore::QEntity*>(QStringLiteral("lineEntity"));
    if (oldEntity && !SceneTreeNode->GetOwner()->isSelected()) // Randy added the second boolean on 11/21
    {
        auto* oldRenderer = oldEntity->findChild<Qt3DRender::QGeometryRenderer*>();
        if (oldRenderer && oldRenderer->geometry() == debugDraw->GetLineGeometry())
            return; // No work to be done if geometry stays the same
    }
    delete oldEntity;

    // printf("this=%p debugDraw=%p\n", this, debugDraw);

    auto* lineEntity = new Qt3DCore::QEntity(this);
    lineEntity->setObjectName(QStringLiteral("lineEntity"));

    if (!LineMaterial || SceneTreeNode->GetOwner()->isSelected()) // Randy added the second boolean on 11/21 to color polyline/bspline
    {
        auto xmlPath = CResourceMgr::Get().Find("DebugDrawLine.xml");
        auto* lineMat = new CXMLMaterial(QString::fromStdString(xmlPath));
        LineMaterial = lineMat;
        LineMaterial->setObjectName(QStringLiteral("lineMaterial"));
        LineMaterial->setParent(this);
        lineEntity->addComponent(LineMaterial); 
        // Randy added this on 11/21
        if (SceneTreeNode->GetOwner()->isSelected()) {
            std::cout << "You selected a polyline/bspline entity" << std::endl;
            QVector3D instanceColor;
            auto color = SceneTreeNode->GetOwner()->GetSelectSurface();
            std::cout << color.x + color.y + color.z << std::endl;
            instanceColor.setX(color.x);
            instanceColor.setY(color.y);
            instanceColor.setZ(color.z);
            lineMat->FindParameterByName("instanceColor")->setValue(instanceColor); 
            SceneTreeNode->GetOwner()->UnselectNode(); // deselect it so it won't be colored again the next time
        }
        else if (auto surface = SceneTreeNode->GetOwner()->GetSurface())
        {
            QVector3D instanceColor;
            instanceColor.setX(surface->ColorR.GetValue(1.0f));
            instanceColor.setY(surface->ColorG.GetValue(1.0f));
            instanceColor.setZ(surface->ColorB.GetValue(1.0f));
            lineMat->FindParameterByName("instanceColor")->setValue(instanceColor); 
        }
    }

    auto* lineRenderer = new Qt3DRender::QGeometryRenderer(lineEntity);
    lineRenderer->setGeometry(debugDraw->GetLineGeometry());
    lineRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
    lineEntity->addComponent(lineRenderer);
}

}
