#include "MeshMerger.h"
#include "OffsetRefiner.h"
#include "Subdivision.h"

#include <unordered_map>

namespace Nome::Scene
{
DEFINE_META_OBJECT(CMeshMerger)
{
    BindNamedArgument(&CMeshMerger::Level, "sd_level", 0);
    BindNamedArgument(&CMeshMerger::Height, "height", 0);
    BindNamedArgument(&CMeshMerger::Width, "width", 0);
}

inline static const float Epsilon = 0.01f;

void CMeshMerger::UpdateEntity()
{
    if (!IsDirty())
        return;
    subdivisionLevel = Level.GetValue(0);

    Super::UpdateEntity();

    // Update is manual, so this entity has a dummy update method

    SetValid(true);
}

void CMeshMerger::Catmull()
{

    bool needSubdivision = subdivisionLevel != 0;

    bool needOffset = (Width.GetValue(0) != 0 || Height.GetValue(0) != 0); // ithink MergedMesh is currMesh
    std::cout << " needSubdiv bool: " << needSubdivision << " needOffset bool: "
        << needOffset << " currMesh size: " << currMesh.vertList.size() << " MergedMesh size: " <<  MergedMesh.vertList.size() << " " << std::endl; // so the bug is that the MergedMesh vert list is 0
    if ((!needSubdivision && !needOffset) || MergedMesh.vertList.size() == 0)//.vertices_empty()) Randy changed the commented out method
    {
        return;
    }

    currMesh.clear();

    DSMesh otherMesh = MergedMesh.randymakeCopy();  // MergedMesh; // Randy question: are we passing our own mesh here
                                 // to subdivde/offset?

    std::cout << otherMesh.faceList.size() << std::endl;
    // catmull.attach(otherMesh);
    // prepare(otherMesh);

    if (needSubdivision)
    {
        //subdivide(otherMesh, subdivisionLevel, isSharp); Randy commented this out for now. add back asap 
        std::cout << "Apply catmullclark subdivision, may take a few minutes or so" << std::endl;
    }
    if (needOffset)
    {

        std::cout << "Apply offset, may take a few minutes or so" << std::endl;
        offset(otherMesh);
    }

    currMesh = otherMesh.randymakeCopy();

    //currMesh.buildBoundary(); Performed in randymakeCopy()
    // currMesh.computeNormals();
}

void CMeshMerger::MergeClear()
{
    MergedMesh.clear(); //  Project AddOffset can we remove this?
    currMesh.clear(); // Project AddOffset Randy note: is this correct?
    //Mesh.clear();
}

void CMeshMerger::MergeIn(const CMeshInstance& meshInstance)
{
    auto tf = meshInstance.GetSceneTreeNode()->L2WTransform.GetValue(
        tc::Matrix3x4::IDENTITY); // The transformation matrix is the identity matrix by default
    auto& otherMesh = meshInstance.GetDSMesh(); // Getting OpeshMesh implementation of a mesh. This
                                                // allows us to traverse the mesh's vertices/faces

    auto meshClass =
        meshInstance.GetSceneTreeNode()->GetOwner()->GetEntity()->GetMetaObject().ClassName();
    if (meshClass == "CPolyline")
    {
        std::cout << "found Polyline entity" << std::endl;
        return; // skip for now, dont merge polyline entities
    }
    if (meshClass == "CBSpline")
    {
        std::cout << "found Bspline entity" << std::endl;
        return; // skip for now, dont merge polyline related entities
    }

    // Copy over all the vertices and check for overlapping
    std::unordered_map<Vertex*, Vertex*> vertMap;
    for (auto otherVert :
         otherMesh.vertList) // Iterate through all the vertices in the mesh (the non-merger mesh,
                             // aka the one you're trying copy vertices from)
    {
        Vector3 localPos = otherVert->position; // localPos is position before transformations
        Vector3 worldPos = tf * localPos; // worldPos is the actual position you see in the grid
        auto [closestVert, distance] = FindClosestVertex(
            worldPos); // Find closest vertex already IN MERGER mesh, not the actual mesh. This is
                       // to prevent adding two merger vertices in the same location!

        if (distance < Epsilon)
        { // this is to check for cases where there is an overlap (two vertices lie in the exact
          // same world space coordinate). We only want to create one merger vertex at this
          // location!
            vertMap[otherVert] =
                closestVert; // just set vi to the closestVert (which is a merger vertex
                             // in the same location added in a previous iteration)
        }
        else // Else, we haven't added a vertex at this location yet. So lets add_vertex to the
             // merger mesh.
        {
            Vertex* copiedVert =
                new Vertex(worldPos.x, worldPos.y, worldPos.z, MergedMesh.nameToVert.size()); // project add offset
            copiedVert->name = "copiedVert" + std::to_string(MergedMesh.nameToVert.size()); // Randy this was causing the bug!!!!!!! the name
                                                 // was the same. so nameToVert remained size == 1
            MergedMesh.addVertex(copiedVert); // Project AddOffset
            vertMap[otherVert] = copiedVert; // Map actual mesh vertex to merged vertex.This
                                             // dictionary is useful for add face later.
            std::string vName = "v" + std::to_string(VertCount);
            ++VertCount; // VertCount is an attribute for this merger mesh. Starts at 0.
        }
    }

    // Add faces and create a face mesh for each
    for (auto otherFace :
         otherMesh.faceList) // Iterate through all the faces in the mesh (that is, the non-merger
                             // mesh, aka the one you're trying to copy faces from)
    {
        std::vector<Vertex*> verts;
        for (auto vert : otherFace->vertices) // otherMesh vertices
        { // iterate through all the vertices on this face
            auto temp = vertMap[vert];
            verts.emplace_back(vertMap[vert]);
        } // Add the vertex handles
        // auto fnew =
        // Mesh.add_face(verts); // add_face processes the merger vertex handles and adds the face
        // into the merger mesh (Mesh refers to the merger mesh here)
        Face* copiedFace = new Face(verts);
        MergedMesh.addPolygonFace(verts); // Project AddOffset
        std::string fName = "v" + std::to_string(FaceCount);
        FaceCount++;
    }
    MergedMesh.buildBoundary();
    MergedMesh.computeNormals();
    currMesh = MergedMesh.randymakeCopy();

}



// Find closest vertex in current mesh's vertices
std::pair<Vertex*, float> CMeshMerger::FindClosestVertex(const tc::Vector3& pos)
{
    Vertex* result;
    float minDist = std::numeric_limits<float>::max();
    // TODO: linear search for the time being
    for (const auto& v : MergedMesh.vertList) // Project AddOffset
    {
        Vector3 pp = v->position;
        float dist = pos.DistanceToPoint(pp);
        if (dist < minDist)
        {
            minDist = dist;
            result = v;
        }
    }
    return { result, minDist };
}


// offset only added here
// Randy changed it to use DSMesh
bool CMeshMerger::offset(DSMesh & _m)
{
    std::cout << "Here is _m face size " << _m.faceList.size() << std::endl;
    float height = Height.GetValue(0.0f);
    float width = Width.GetValue(0.0f);
    if (height <= 0 && width <= 0)
    {
        return true;
    }
    COffsetRefiner offsetRefiner(_m, offsetFlag);
    offsetRefiner.Refine(height, width);
    _m.clear(); // TODO: is this not doing anyhting???

    

    std::vector<Vertex*> vertices = offsetRefiner.GetVertices();    
    std::vector<Face*> faces = offsetRefiner.GetFaces();  


    // Print vertices
    printf("============ output vertices ======\n");

    // Print faces
    printf("============ output faces ======\n"); // TODO: debug below...
    //for (int index = 0; index < faces.size(); index++)
    for (auto face : faces)
    {
        std::vector<Vertex*> newVerts;
        for (auto vert : face->vertices) {
            Vertex* newVert = new Vertex(vert->position.x, vert->position.y, vert->position.z, _m.vertList.size());
            _m.addVertex(newVert);
            newVerts.push_back(newVert);
        }
        _m.addPolygonFace(newVerts);
    }

    _m.buildBoundary(); // Randy added this on 2/26
    _m.computeNormals();
    return true;
}

bool CMeshMerger::subdivide(CMeshImpl& _m, unsigned int n, bool isSharp)
{
    // Project AddOffset - reimplement this later
    return true;
}

void CMeshMerger::split_face(CMeshImpl& _m, const CMeshImpl::FaceHandle& _fh)
{
    // Project AddOffset - reimplement this later
}

void CMeshMerger::split_edge(CMeshImpl& _m, const CMeshImpl::EdgeHandle& _eh)
{
    // Project AddOffset - reimplement this later
}

void CMeshMerger::compute_midpoint(CMeshImpl& _m, const CMeshImpl::EdgeHandle& _eh,
                                   const bool _update_points)
{
    // Project AddOffset - reimplement this later
}

void CMeshMerger::update_vertex(CMeshImpl& _m, const CMeshImpl::VertexHandle& _vh)
{ // Project AddOffset - reimplement this later
}

}