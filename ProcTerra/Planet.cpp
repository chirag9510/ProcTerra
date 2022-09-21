#include "Planet.h"
#include <OgreMeshLodGenerator.h>
#include <OgreLodConfig.h>

Planet::Planet(Ogre::SceneManager* mSceneMgr, MeshType meshType, std::string strName, Ogre::uint32 visibilityMask) :
    mSceneMgr(mSceneMgr),
    strName(strName),
    meshType(meshType),
    nSections(120),
    iDiaMultiplier(15),
    fYaw(0.2f),
    fPitch(0.2f),
    fRoll(0.2f),
    bYaw(false),
    bPitch(false),
    bRoll(false),
    lightType(LightType::AMBIENT),
    bRenderElevation(true),
    visibilityMask(visibilityMask),
    bAutoLodGeneration(false)
{
}

void Planet::update(const float& fDeltaTime)
{
    for (auto e : vecFaceNodes)
    {
        if(bYaw)
            e->yaw(Ogre::Radian(fDeltaTime * fYaw), Ogre::Node::TS_WORLD);
        if(bPitch)
            e->pitch(Ogre::Radian(fDeltaTime * fPitch), Ogre::Node::TS_WORLD);
        if(bRoll)
            e->roll(Ogre::Radian(fDeltaTime * fRoll), Ogre::Node::TS_WORLD);
    }
}

void Planet::init()
{
    //first read from .dat file
    if (!readDATFile())
    {
        resetToDefaultNoiseValues();
        resetToDefaultBiomeValues();

        //resize to initialize 3 ring objects
        vecRings.resize(3);
        resetToDefaultRingValues();
    }
    initMeshValues();

    //generate the mesh first time
    //create a generic material so that it may set the diffuse color
    material = Ogre::MaterialManager::getSingleton().create(strName + "DiffuseMtr", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    //use if directional lighting is disabled and only ambient light is used
    if (lightType == LightType::AMBIENT)                                              //gradient mesh will always have ambient light
    {
        material->getTechnique(0)->getPass(0)->setVertexColourTracking(Ogre::TVC_AMBIENT);
    }
    else
    {
        material->getTechnique(0)->getPass(0)->setVertexColourTracking(Ogre::TVC_DIFFUSE);
        material->getTechnique(0)->getPass(0)->setLightingEnabled(true);
    }

    createDefaultFaceVerticesAndIndices();

    //create 6 meshes for each of the faces 
    //and them convert them to spheres
    vecFaces.reserve(6);
    vecFaceNodes.reserve(6);
    vecFaces.emplace_back(createNormalisedFace(Ogre::Vector3::UNIT_Y,strName + "PlaneY", strName + "FaceY"));
    vecFaces.emplace_back(createNormalisedFace(Ogre::Vector3::UNIT_X, strName + "PlaneX", strName + "FaceX"));
    vecFaces.emplace_back(createNormalisedFace(Ogre::Vector3::UNIT_Z, strName + "PlaneZ", strName + "FaceZ"));
    vecFaces.emplace_back(createNormalisedFace(Ogre::Vector3::NEGATIVE_UNIT_Y, strName + "PlaneNY", strName + "FaceNY"));
    vecFaces.emplace_back(createNormalisedFace(Ogre::Vector3::NEGATIVE_UNIT_X, strName + "PlaneNX", strName + "FaceNX"));
    vecFaces.emplace_back(createNormalisedFace(Ogre::Vector3::NEGATIVE_UNIT_Z, strName + "PlaneNZ", strName + "FaceNZ"));

    //create ring mesh, not for gradient planet
    if (meshType == MeshType::NORMAL_BIOMES)
    {
        //init all 3 rings
        short i = 0;
        for (auto& ring : vecRings)
        {
            ring.mshY = createRing(Ogre::Vector3::UNIT_Y, &ring.entityY, &ring.sceneNodeY, ring.colorInner, ring.colorOuter, strName + "PlaneRingY" + std::to_string(i), strName + "RingFaceY" + std::to_string(i));
            ring.mshNY = createRing(Ogre::Vector3::NEGATIVE_UNIT_Y, &ring.entityNY, &ring.sceneNodeNY, ring.colorInner, ring.colorOuter, strName + "PlaneRingNY" + std::to_string(i), strName + "RingFaceNY" + std::to_string(i++));
            ring.entityY->setVisible(ring.bVisible);
            ring.entityNY->setVisible(ring.bVisible);
            ring.sceneNodeY->pitch(Ogre::Radian(ring.fPitch), Ogre::Node::TS_WORLD);
            ring.sceneNodeY->yaw(Ogre::Radian(ring.fYaw), Ogre::Node::TS_WORLD);
            ring.sceneNodeY->roll(Ogre::Radian(ring.fRoll), Ogre::Node::TS_WORLD);
            ring.sceneNodeNY->pitch(Ogre::Radian(ring.fPitch), Ogre::Node::TS_WORLD);
            ring.sceneNodeNY->yaw(Ogre::Radian(ring.fYaw), Ogre::Node::TS_WORLD);
            ring.sceneNodeNY->roll(Ogre::Radian(ring.fRoll), Ogre::Node::TS_WORLD);

        }
    }


    //generate mesh with noise for first time, also update rings
    generate();

}


void Planet::generate()
{
    //update noise object with values from gui
    setValuesToNoiseObject();

    //update each face mesh by applying noise algo into each of thier vertices at world position
    updateMesh(vecFaces[0].get(), Ogre::Vector3::UNIT_Y);
    updateMesh(vecFaces[1].get(), Ogre::Vector3::UNIT_X);
    updateMesh(vecFaces[2].get(), Ogre::Vector3::UNIT_Z);
    updateMesh(vecFaces[3].get(), Ogre::Vector3::NEGATIVE_UNIT_Y);
    updateMesh(vecFaces[4].get(), Ogre::Vector3::NEGATIVE_UNIT_X);
    updateMesh(vecFaces[5].get(), Ogre::Vector3::NEGATIVE_UNIT_Z);

    //gradient planets dont have rings
    if (meshType == MeshType::NORMAL_BIOMES)
    {
        for (auto& ring : vecRings)
        {
            updateRing(ring.mshY.get(), Ogre::Vector3::UNIT_Y, ring.fOuterRingDia, ring.fInnerThickness, ring.colorInner, ring.colorOuter);
            updateRing(ring.mshNY.get(), Ogre::Vector3::NEGATIVE_UNIT_Y, ring.fOuterRingDia, ring.fInnerThickness, ring.colorInner, ring.colorOuter);
        }
    }
}




void Planet::updateMesh(const Ogre::Mesh* const mesh, const Ogre::Vector3 vFace)
{
    //how planet generation will work -
    // create a new sphere using the default mesh plane values createDefaultFaceVerticesAndIndices() 6 times just the way it was created in init()
    // only difference is the values for vertices once they are rotated to thier appropriate face position, and then normalized to form a sphere,
    // are then sent to the noise generation algo to create peaks and valleys for the planet where the vertex will set its distance from center according to its range
    // the world position values for the mesh are retrieved when the mesh can be recreated into its default sphere coordinates, to form a planet with peaks and valleys, fresh from the ground up
    //default for Ogre::Vector3::NEGATIVE_UNIT_Y
    Ogre::Quaternion vertexRot(Ogre::Degree(0), Ogre::Vector3::UNIT_X);
    //rotate the plane so it may face the correct direction according to its face
    if (vFace == Ogre::Vector3::UNIT_Y)
        vertexRot = Ogre::Quaternion(Ogre::Degree(180), Ogre::Vector3::UNIT_X);     //pitch 180
    else if (vFace == Ogre::Vector3::UNIT_X)
        vertexRot = Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z);     //roll 90
    else if (vFace == Ogre::Vector3::UNIT_Z)
        vertexRot = Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_X);     //pitch 90
    else if (vFace == Ogre::Vector3::NEGATIVE_UNIT_X)
        vertexRot = Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_Z);     //roll 90
    else if (vFace == Ogre::Vector3::NEGATIVE_UNIT_Z)
        vertexRot = Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_X);     //pitch 90

    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    int vertex_count = 0, index_count = 0;
    Ogre::SubMesh* submesh = mesh->getSubMesh(0);
    // We only need to add the shared vertices once
    if (submesh->useSharedVertices)
    {
        if (!added_shared)
        {
            vertex_count += mesh->sharedVertexData->vertexCount;
            added_shared = true;
        }
    }
    else
    {
        vertex_count += submesh->vertexData->vertexCount;
    }
    // Add the indices
    index_count += submesh->indexData->indexCount;

    // Allocate space for the vertices and indices
    auto vertices = new Ogre::Vector3[vertex_count];
    auto indices = new unsigned long[index_count];
    added_shared = false;

    Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
    Ogre::VertexData* color_data = vertex_data;
    if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
    {
        if (submesh->useSharedVertices)
        {
            added_shared = true;
            shared_offset = current_offset;
        }

        //vertex position
        float* pVertexPosition;
        Ogre::Vector3 v, vMesh;
        const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
        Ogre::HardwareVertexBufferSharedPtr vbufPos = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
        unsigned char* vertex = static_cast<unsigned char*>(vbufPos->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

        //diffuse
        Ogre::RGBA* pColorValue;
        const Ogre::VertexElement* colorElem = color_data->vertexDeclaration->findElementBySemantic(Ogre::VES_COLOUR);
        Ogre::HardwareVertexBufferSharedPtr vbufColor = color_data->vertexBufferBinding->getBuffer(colorElem->getSource());
        unsigned char* color = static_cast<unsigned char*>(vbufColor->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

        //update mesh
        float e = 0.f;
        float fDistFromCenter = fSideLength / 2.f;
        float fNoiseDist = fPerFrequencyHeight * fDistFromCenter;
        float eMinDepth = -1.f;
        if (meshType == MeshType::NORMAL_BIOMES)
            eMinDepth = indexMinBiomeDepth ? vecBiomes[indexMinBiomeDepth - 1].e : -1.0f;
        else if (meshType == MeshType::GRADIENT && bRenderElevation == false)
            eMinDepth = 1.f;
        float fMinBiomeDistFromCenter = fDistFromCenter + fNoiseDist * eMinDepth;
        for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbufPos->getVertexSize(), color += vbufColor->getVertexSize())
        {
            //VERTEX
             //gets the pointer to the vertex !!!!
            posElem->baseVertexPointerToElement(vertex, &pVertexPosition);

            v = vertexRot * vecVertices[j];
            v.normalise();
            vMesh = Ogre::Vector3(v.x * fDistFromCenter, v.y * fDistFromCenter, v.z * fDistFromCenter);
            if (bDomainWarp)
                domainWarp.DomainWarp(vMesh.x, vMesh.y, vMesh.z);
            e = (noise.GetNoise(vMesh.x, vMesh.y, vMesh.z) +
                0.5f * noise.GetNoise(2.0f * vMesh.x, 2.0f * vMesh.y, 2.0f * vMesh.z) +
                0.25f * noise.GetNoise(4.0f * vMesh.x, 4.0f * vMesh.y, 4.f * vMesh.z)) / 1.75f;
            //clamp 
            e = std::clamp(e, -1.f, 1.f);

            //now set the position after applying e
            if (e < eMinDepth)
                fDistFromCenter = fMinBiomeDistFromCenter;
            else
                fDistFromCenter = fDistFromCenter + e * fNoiseDist;
            pVertexPosition[0] = v.x * fDistFromCenter;
            pVertexPosition[1] = v.y * fDistFromCenter;
            pVertexPosition[2] = v.z * fDistFromCenter;
            fDistFromCenter = fSideLength / 2.f;

            //COLOUR
            //now pointer to the color value
            colorElem->baseVertexPointerToElement(color, &pColorValue);
            if (meshType == MeshType::NORMAL_BIOMES)
            {
                //set the color according the the biome, for primary planet only
                for (auto iter = vecBiomes.begin(); iter != vecBiomes.end(); iter++)
                {
                    if (e < iter->e)
                    {
                        if (iter != vecBiomes.begin() && iter + 1 != vecBiomes.end() && interpolationType != InterpolationType::Sharp)
                            *pColorValue = biomeColorInterpolation(e, iter).getAsBYTE();
                        else
                            *pColorValue = iter->color.getAsBYTE();
                        break;
                    }
                }
            }
            else
            {
                //black n white for gradient
                *pColorValue = Ogre::ColourValue(0.5f + e * 0.5f, 0.5f + e * 0.5f, 0.5f + e * 0.5f).getAsBYTE();
            }
        }

        vbufPos->unlock();
        vbufColor->unlock();
    }

    delete vertices;
    delete indices;
}

Ogre::ColourValue Planet::biomeColorInterpolation(const float& e, std::vector<Biome>::iterator& iter)
{
    float eBiome = iter->e;
    Ogre::ColourValue colorBiome = iter->color;
    iter--;
    float eBiomePrev = iter->e;
    Ogre::ColourValue colorBiomePrev = iter->color;
    float eDeltaPercent = (e - eBiomePrev) / (eBiome - eBiomePrev);

    //smooth by default
    if (interpolationType == InterpolationType::Intermediate)
    {
        if (eDeltaPercent < .075f)
            eDeltaPercent = 0.f;
        else if (eDeltaPercent < .33f)
            eDeltaPercent = .33f;
        else if (eDeltaPercent < .66f)
            eDeltaPercent = .66f;
        else
            eDeltaPercent = 1.f;
    }

    colorBiome.r = colorBiomePrev.r + (colorBiome.r - colorBiomePrev.r) * eDeltaPercent;
    colorBiome.g = colorBiomePrev.g + (colorBiome.g - colorBiomePrev.g) * eDeltaPercent;
    colorBiome.b = colorBiomePrev.b + (colorBiome.b - colorBiomePrev.b) * eDeltaPercent;

    
    return colorBiome;

}


Ogre::MeshPtr Planet::createNormalisedFace(const Ogre::Vector3 vFace, const std::string strItem, const std::string strEntity)
{
    //default for Ogre::Vector3::NEGATIVE_UNIT_Y
    Ogre::Quaternion vertexRot(Ogre::Degree(0), Ogre::Vector3::UNIT_X);

    //rotate the plane so it may face the correct direction according to its face
    if (vFace == Ogre::Vector3::UNIT_Y)
        vertexRot = Ogre::Quaternion(Ogre::Degree(180), Ogre::Vector3::UNIT_X);     //pitch 180
    else if (vFace == Ogre::Vector3::UNIT_X)
        vertexRot = Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z);     //roll 90
    else if (vFace == Ogre::Vector3::UNIT_Z)
        vertexRot = Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_X);     //pitch 90
    else if (vFace == Ogre::Vector3::NEGATIVE_UNIT_X)
        vertexRot = Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_Z);     //roll 90
    else if (vFace == Ogre::Vector3::NEGATIVE_UNIT_Z)
        vertexRot = Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_X);     //pitch 90


    /// Create the mesh via the MeshManager
    Ogre::MeshPtr msh = Ogre::MeshManager::getSingleton().createManual(strItem, "General");
    /// Create one submesh
    Ogre::SubMesh* sub = msh->createSubMesh();

    //v buffer
    std::vector<float> vertices;
    vertices.reserve(vBufCount);
    //convert the plane coordinates to sphere right now during mesh initialization
    //apply rotation to each vertex according to the direction of the face
    //index order for mesh triangles dont change
    //https://forums.ogre3d.org/viewtopic.php?t=77080
    float fDistFromCenter = fSideLength / 2.f;
    Ogre::Vector3 vertex;
    for (auto defVertex : vecVertices)
    {
        vertex = vertexRot * defVertex;
        vertex.normalise();
        vertices.emplace_back(vertex.x * fDistFromCenter);
        vertices.emplace_back(vertex.y * fDistFromCenter);
        vertices.emplace_back(vertex.z * fDistFromCenter);

        //normals
        vertices.emplace_back(vertex.x);
        vertices.emplace_back(vertex.y);
        vertices.emplace_back(vertex.z);
    }

    // convert to RGBA
    std::vector<Ogre::RGBA> colours;
    colours.reserve(nVertices);
    for (int i = 0; i < nVertices; i++)
        colours[i] = Ogre::ColourValue(1.0, 0.0, 0.0).getAsBYTE(); //0 colour

    /// Create vertex data structure for 8 vertices shared between submeshes
    msh->sharedVertexData = new Ogre::VertexData();
    msh->sharedVertexData->vertexCount = nVertices;

    /// Create declaration (memory format) of vertex data
    Ogre::VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    // 1st buffer
    decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
    /// and bytes per vertex (offset)
    Ogre::HardwareVertexBufferSharedPtr vbuf =
        Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, msh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), static_cast<void*>(vertices.data()), true);

    /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    Ogre::VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, vbuf);

    // 2nd buffer   VET_UBYTE4_NORM
    offset = 0;
    decl->addElement(1, offset, Ogre::VET_UBYTE4_NORM, Ogre::VES_COLOUR);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_UBYTE4_NORM);
    /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
    /// and bytes per vertex (offset)
    vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
        offset, msh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), static_cast<void*>(colours.data()), true);

    /// Set vertex buffer binding so buffer 1 is bound to our colour buffer
    bind->setBinding(1, vbuf);

    /// Allocate index buffer of the requested number of vertices (ibufCount) 
    Ogre::HardwareIndexBufferSharedPtr ibuf = Ogre::HardwareBufferManager::getSingleton().
        createIndexBuffer(
            Ogre::HardwareIndexBuffer::IT_16BIT,
            iBufCount,
            Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    ibuf->writeData(0, ibuf->getSizeInBytes(), static_cast<void*>(vecIndices.data()), true);

    /// Set parameters of the submesh
    sub->useSharedVertices = true;
    sub->indexData->indexBuffer = ibuf;
    sub->indexData->indexCount = iBufCount;
    sub->indexData->indexStart = 0;

    /// Set bounding information (for culling)
    msh->_setBounds(Ogre::AxisAlignedBox(-fSideLength, -fSideLength, -fSideLength, fSideLength, fSideLength, fSideLength));
    msh->_setBoundingSphereRadius(Ogre::Math::Sqrt(3 * fSideLength * fSideLength));

    /// Notify -Mesh object that it has been loaded
    msh->load();

    //now spawn it 
    Ogre::Entity* entity = mSceneMgr->createEntity(strEntity, strItem);
    entity->setMaterialName(strName + "DiffuseMtr");
    entity->setVisibilityFlags(visibilityMask);
    Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->attachObject(entity);
    vecFaceNodes.emplace_back(node);

    //auto lod on mesh
    if (bAutoLodGeneration)
    {
        if (!Ogre::MeshLodGenerator::getSingletonPtr())
            new Ogre::MeshLodGenerator();
        Ogre::MeshLodGenerator::getSingletonPtr()->generateAutoconfiguredLodLevels(msh);
    }
  

    return msh;
}


Ogre::MeshPtr Planet::createRing(const Ogre::Vector3 vFace, Ogre::Entity** entity, Ogre::SceneNode** node, const Ogre::ColourValue colorInner, const Ogre::ColourValue colorOuter, const std::string strItem, const std::string strEntity)
{
    //rotate the plane so it may face the correct direction according to its face
    //default for Ogre::Vector3::NEGATIVE_UNIT_Y
    Ogre::Quaternion vertexRot(Ogre::Degree(0), Ogre::Vector3::UNIT_X);
    if (vFace == Ogre::Vector3::UNIT_Y)
        vertexRot = Ogre::Quaternion(Ogre::Degree(180), Ogre::Vector3::UNIT_X);     //pitch 180


    /// Create the mesh via the MeshManager
    Ogre::MeshPtr msh = Ogre::MeshManager::getSingleton().createManual(strItem, "General");
    /// Create one submesh
    Ogre::SubMesh* sub = msh->createSubMesh();

    //v buffer
    std::vector<float> vertices;
    vertices.reserve(vRingBufCount);
    //convert the plane coordinates to sphere right now during mesh initialization
    //apply rotation to each vertex according to the direction of the face
    //index order for mesh triangles dont change
    //https://forums.ogre3d.org/viewtopic.php?t=77080
    float fDistFromCenter = fSideLength / 2.f;
    Ogre::Vector3 vertex;
    for (auto defVertex : vecRingVertices)
    {
        vertex = vertexRot * defVertex;
        vertex.normalise();
        vertices.emplace_back(vertex.x * fDistFromCenter);
        vertices.emplace_back(vertex.y * fDistFromCenter);
        vertices.emplace_back(vertex.z * fDistFromCenter);

        //normals
        vertices.emplace_back(vertex.x);
        vertices.emplace_back(vertex.y);
        vertices.emplace_back(vertex.z);
    }

    // convert to RGBA
    std::vector<Ogre::RGBA> colours;
    colours.reserve(nRingVertices);
    for (int i = 0; i < nRingVertices; i++)
    {
        if(i >= nRingVertices / 2.f)
            colours[i] = colorInner.getAsBYTE(); //0 colour
        else
            colours[i] = colorOuter.getAsBYTE(); //0 colour
    }

    /// Create vertex data structure for 8 vertices shared between submeshes
    msh->sharedVertexData = new Ogre::VertexData();
    msh->sharedVertexData->vertexCount = nRingVertices;

    /// Create declaration (memory format) of vertex data
    Ogre::VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    // 1st buffer
    decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
    /// and bytes per vertex (offset)
    Ogre::HardwareVertexBufferSharedPtr vbuf =
        Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, msh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), static_cast<void*>(vertices.data()), true);

    /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    Ogre::VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, vbuf);

    // 2nd buffer   VET_UBYTE4_NORM
    offset = 0;
    decl->addElement(1, offset, Ogre::VET_UBYTE4_NORM, Ogre::VES_COLOUR);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_UBYTE4_NORM);
    /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
    /// and bytes per vertex (offset)
    vbuf = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
        offset, msh->sharedVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), static_cast<void*>(colours.data()), true);

    /// Set vertex buffer binding so buffer 1 is bound to our colour buffer
    bind->setBinding(1, vbuf);

    /// Allocate index buffer of the requested number of vertices (ibufCount) 
    Ogre::HardwareIndexBufferSharedPtr ibuf = Ogre::HardwareBufferManager::getSingleton().
        createIndexBuffer(
            Ogre::HardwareIndexBuffer::IT_16BIT,
            iRingBufCount,
            Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    ibuf->writeData(0, ibuf->getSizeInBytes(), static_cast<void*>(vecRingIndices.data()), true);

    /// Set parameters of the submesh
    sub->useSharedVertices = true;
    sub->indexData->indexBuffer = ibuf;
    sub->indexData->indexCount = iRingBufCount;
    sub->indexData->indexStart = 0;

    /// Set bounding information (for culling)
    msh->_setBounds(Ogre::AxisAlignedBox(-fSideLength, -fSideLength, -fSideLength, fSideLength, fSideLength, fSideLength));
    msh->_setBoundingSphereRadius(Ogre::Math::Sqrt(3 * fSideLength * fSideLength));

    /// Notify -Mesh object that it has been loaded
    msh->load();


    //now spawn it 
    Ogre::Entity* entityR = mSceneMgr->createEntity(strEntity, strItem);
    entityR->setMaterialName(strName + "DiffuseMtr");
    entityR->setVisibilityFlags(visibilityMask);
    Ogre::SceneNode* nodeR = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    nodeR->attachObject(entityR);

    *entity = entityR;
    *node = nodeR;

    return msh;
}


void Planet::updateRing(const Ogre::Mesh* const mesh, const Ogre::Vector3 vFace, const float fOuterRingDia, const float fInnerThickness, const Ogre::ColourValue colorInner, const Ogre::ColourValue colorOuter)
{
    //default for Ogre::Vector3::NEGATIVE_UNIT_Y
    Ogre::Quaternion vertexRot(Ogre::Degree(0), Ogre::Vector3::UNIT_X);
    //rotate the plane so it may face the correct direction according to its face
    if (vFace == Ogre::Vector3::UNIT_Y)
        vertexRot = Ogre::Quaternion(Ogre::Degree(180), Ogre::Vector3::UNIT_X);     //pitch 180

    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    int vertex_count = 0, index_count = 0;
    Ogre::SubMesh* submesh = mesh->getSubMesh(0);
    // We only need to add the shared vertices once
    if (submesh->useSharedVertices)
    {
        if (!added_shared)
        {
            vertex_count += mesh->sharedVertexData->vertexCount;
            added_shared = true;
        }
    }
    else
    {
        vertex_count += submesh->vertexData->vertexCount;
    }
    // Add the indices
    index_count += submesh->indexData->indexCount;

    // Allocate space for the vertices and indices
    auto vertices = new Ogre::Vector3[vertex_count];
    auto indices = new unsigned long[index_count];
    added_shared = false;

    Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
    Ogre::VertexData* color_data = vertex_data;
    if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
    {
        if (submesh->useSharedVertices)
        {
            added_shared = true;
            shared_offset = current_offset;
        }

        //vertex position
        float* pVertexPosition;
        Ogre::Vector3 v, vMesh;
        const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
        Ogre::HardwareVertexBufferSharedPtr vbufPos = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
        unsigned char* vertex = static_cast<unsigned char*>(vbufPos->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));

        //diffuse
        Ogre::RGBA* pColorValue;
        const Ogre::VertexElement* colorElem = color_data->vertexDeclaration->findElementBySemantic(Ogre::VES_COLOUR);
        Ogre::HardwareVertexBufferSharedPtr vbufColor = color_data->vertexBufferBinding->getBuffer(colorElem->getSource());
        unsigned char* color = static_cast<unsigned char*>(vbufColor->lock(Ogre::HardwareBuffer::HBL_WRITE_ONLY));
           
        float fDistFromCenter = fOuterRingDia * fSideLength / 2.f;
        float fInnerRingDist = fDistFromCenter * (1.f - fInnerThickness);
        for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbufPos->getVertexSize(), color += vbufColor->getVertexSize())
        {
            //VERTEX
             //gets the pointer to the vertex !!!!
            posElem->baseVertexPointerToElement(vertex, &pVertexPosition);

            v = vertexRot * vecRingVertices[j];
            v.normalise();

            //check if position is for inner ring or outer
            if (j >= nRingVertices / 2)
                fDistFromCenter = fInnerRingDist;

            pVertexPosition[0] = v.x * fDistFromCenter;
            pVertexPosition[1] = v.y * fDistFromCenter;
            pVertexPosition[2] = v.z * fDistFromCenter;

            //COLOUR
            //now pointer to the color value
            colorElem->baseVertexPointerToElement(color, &pColorValue);
            if (j >= nRingVertices / 2)
                *pColorValue = colorInner.getAsBYTE();
            else
                *pColorValue = colorOuter.getAsBYTE();

        }

        vbufPos->unlock();
        vbufColor->unlock();
    }

    delete vertices;
    delete indices;
}



void Planet::createDefaultFaceVerticesAndIndices()
{
    vecVertices.reserve(vBufCount);
    vecIndices.reserve(iBufCount);
    //standard vertex positions at default NEGATIVE_UNIT_Y direction starting from -x, -y, -z
    //the first vertex of the face
    float fX = -fSideLength / 2.f;        //X position which will be used to reset the vertex
    float x = fX;
    float fZ = fX;
    //the rest
    for (int i = 1; i <= nVertices; i++)
    {
        vecVertices.emplace_back(Ogre::Vector3(x, fX, fZ));
        x += fSectionLength;

        if (i % nSegments == 0)
        {
            fZ += fSectionLength;
            x = fX;
        }
    }
    //Indices for the vertices above to form a face
    unsigned short index = 0, startingIndex = 0;
    for (size_t row = 0; row < nSections; row++)
    {
        index = startingIndex = row * nSegments;
        for (size_t i = 0; i < nSections; i++)
        {
            //lower
            vecIndices.emplace_back(index);
            index += (nSegments + 1);
            vecIndices.emplace_back(index);
            index--;
            vecIndices.emplace_back(index);

            index = startingIndex;
            //upper
            vecIndices.emplace_back(index++);
            vecIndices.emplace_back(index);
            index += nSegments;
            vecIndices.emplace_back(index);
            index = ++startingIndex;
        }
    }


    //Ring System
    vecRingVertices.reserve(vRingBufCount);
    vecRingIndices.reserve(iRingBufCount);
    //standard vertex positions at default NEGATIVE_UNIT_Y direction starting from -x, -y, -z
    //the first vertex of the face (ring)
    fX = -fSideLength / 2.f;        //X position which will be used to reset the vertex
    x = fX;
    fZ = fX;
    float fVertDirectionX = 1.f, fVertDirectionZ = 0.f;
    int iDir = 0;
    //the rest
    for (int i = 1; i <= nRingVertices; i++)
    {
        vecRingVertices.emplace_back(Ogre::Vector3(x, 0.f, fZ));
        x += fVertDirectionX * fSectionLength;
        fZ += fVertDirectionZ * fSectionLength;

        if (i % nSections == 0)
        {
            if (iDir % 4 == 0)
            {
                fVertDirectionX = 0.f;
                fVertDirectionZ = 1.f;
            }
            else if (iDir % 4 == 1)
            {
                fVertDirectionX = -1.f;
                fVertDirectionZ = 0.f;
            }
            else if (iDir % 4 == 2)
            {
                fVertDirectionX = 0.f;
                fVertDirectionZ = -1.f;
            }
            else if (iDir % 4 == 3)
            {
                fVertDirectionX = 1.f;
                fVertDirectionZ = 0.f;
            }

            iDir++;
        }
    }

    //Indices for the vertices above to form a face
    index = 0, startingIndex = 0;
    size_t outerIndex = nSections * 4;
    for (size_t i = 0; i < iRingBufCount; i++)
    {
        //lower 
        vecRingIndices.emplace_back(index);
        index += (outerIndex + 1);
        vecRingIndices.emplace_back(index);
        index--;
        vecRingIndices.emplace_back(index);

        index = startingIndex;
        //upper
        vecRingIndices.emplace_back(index++);
        vecRingIndices.emplace_back(index);
        index += outerIndex;
        vecRingIndices.emplace_back(index);
        index = ++startingIndex;

        //if last 2 tri are to be drawn
        if (index == outerIndex - 1)
        {
            vecRingIndices.emplace_back(index++);
            vecRingIndices.emplace_back(index);
            index = nRingVertices - 1;
            vecRingIndices.emplace_back(index);

            index = outerIndex - 1;
            vecRingIndices.emplace_back(index++);
            vecRingIndices.emplace_back(0);
            vecRingIndices.emplace_back(index);

            break;
        }
    }
}


void Planet::setPreset(const Preset preset)
{
    switch (preset)
    {
    case Preset::Rocky_Moon:
        indexMinBiomeDepth = 2;
        vecBiomes[0] = Biome(0.635f, Ogre::ColourValue(0.408f, 0.412f, 0.427f));
        vecBiomes[1] = Biome(0.685f, Ogre::ColourValue(0.469f, 0.472f, 0.485f));
        vecBiomes[2] = Biome(0.751f, Ogre::ColourValue(0.549f, 0.549f, .580f));
        vecBiomes[3] = Biome(0.801f, Ogre::ColourValue(0.581f, .581f, .608f));
        vecBiomes[4] = Biome(0.851f, Ogre::ColourValue(.632f, .632f, .632f));
        vecBiomes[5] = Biome(0.890f, Ogre::ColourValue(0.672f, 0.668f, 0.668f));
        vecBiomes[6] = Biome(0.917f, Ogre::ColourValue(0.662f, 0.650f, 0.642f));
        vecBiomes[7] = Biome(0.934f, Ogre::ColourValue(.906f, .91f, .925f));
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(.906f, .91f, .925f));

        noiseType = FastNoiseLite::NoiseType_ValueCubic;
        rotationType3D = FastNoiseLite::RotationType3D_None;
        iSeed = 678;
        fFrequency = 0.001640f;
        fPerFrequencyHeight = .100f;

        fractalType = FastNoiseLite::FractalType_Ridged;
        iOctaves = 7;
        fFractalGain = 0.21500f;
        fFractalWeightedStrength = 0.f;
        fFractalLacunarity = 5.f;

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();

        for (auto& ring : vecRings)
        {
            ring.bVisible = false;
            //cause gradient planets dont generate rings
            if (ring.entityY != nullptr)
            {
                ring.entityY->setVisible(false);
                ring.entityNY->setVisible(false);
            }
        }


        break;

    case Preset::Swift_Planet:
        indexMinBiomeDepth = 0;
        vecBiomes[0] = Biome(-.320f, Ogre::ColourValue(0.408f, 0.412f, 0.427f));
        vecBiomes[1] = Biome(-.166, Ogre::ColourValue(0.469f, 0.472f, 0.485f));
        vecBiomes[2] = Biome(-.083f, Ogre::ColourValue(0.549f, 0.549f, .580f));
        vecBiomes[3] = Biome(0.033f, Ogre::ColourValue(0.581f, .581f, .608f));
        vecBiomes[4] = Biome(0.099f, Ogre::ColourValue(.632f, .632f, .632f));
        vecBiomes[5] = Biome(0.210f, Ogre::ColourValue(0.672f, 0.668f, 0.668f));
        vecBiomes[6] = Biome(0.348f, Ogre::ColourValue(0.662f, 0.650f, 0.642f));
        vecBiomes[7] = Biome(0.597f, Ogre::ColourValue(.906f, .91f, .925f));
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(.906f, .91f, .925f));

        noiseType = FastNoiseLite::NoiseType_Cellular;
        rotationType3D = FastNoiseLite::RotationType3D_ImproveXZPlanes;
        iSeed = 1157;
        fFrequency = 0.002500f;
        fPerFrequencyHeight = .005f;

        fractalType = FastNoiseLite::FractalType_PingPong;
        iOctaves = 4;
        fFractalGain = 0.800f;
        fFractalWeightedStrength = 0.25f;
        fFractalLacunarity = 3.f;
        fPingPongStrength = 2.f;

        cellularDistanceFunction = FastNoiseLite::CellularDistanceFunction_Manhattan;
        cellularReturnType = FastNoiseLite::CellularReturnType_Distance2Div;
        fJitter = 3.f;

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();

        for (auto& ring : vecRings)
        {
            ring.bVisible = false;
            //cause gradient planets dont generate rings
            if (ring.entityY != nullptr)
            {
                ring.entityY->setVisible(false);
                ring.entityNY->setVisible(false);
            }
        }

        break;

    case Preset::Evening_Star:
        indexMinBiomeDepth = 4;
        vecBiomes[0] = Biome(-0.831f, Ogre::ColourValue(1.f, 1.f, 0.557f));
        vecBiomes[1] = Biome(-.645f, Ogre::ColourValue(1.f, 0.796f, 0.208f));
        vecBiomes[2] = Biome(-.532f, Ogre::ColourValue(1.f, 0.655f, .188f));
        vecBiomes[3] = Biome(-.411f, Ogre::ColourValue(0.996f, .498f, .137f));
        vecBiomes[4] = Biome(-.298f, Ogre::ColourValue(.886f, .329f, .086f));
        vecBiomes[5] = Biome(-.153f, Ogre::ColourValue(0.843f, 0.263f, 0.114f));
        vecBiomes[6] = Biome(0.024f, Ogre::ColourValue(0.604f, 0.184f, 0.071f));
        vecBiomes[7] = Biome(0.379f, Ogre::ColourValue(.443f, .106f, .031f));
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(.390f, .09f, .025f));

        noiseType = FastNoiseLite::NoiseType_Perlin;
        rotationType3D = FastNoiseLite::RotationType3D_None;
        iSeed = 432;
        fFrequency = 0.000150f;
        fPerFrequencyHeight = .105f;

        fractalType = FastNoiseLite::FractalType_PingPong;
        iOctaves = 7;
        fFractalGain = 0.500f;
        fFractalWeightedStrength = 0.f;
        fFractalLacunarity = 6.f;
        fPingPongStrength = 2.1f;

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();
        
        for (auto& ring : vecRings)
        {
            ring.bVisible = false;
            //cause gradient planets dont generate rings
            if (ring.entityY != nullptr)
            {
                ring.entityY->setVisible(false);
                ring.entityNY->setVisible(false);
            }
        }


        break;

    case Preset::Blue_Marble:
        indexMinBiomeDepth = 1;
        if (vecBiomes.size() == 0)
            vecBiomes.resize(9);
        vecBiomes[0] = Biome(0.089f, Ogre::ColourValue(0.067f, 0.236f, 0.358f));                        //deep water
        vecBiomes[1] = Biome(0.137f, Ogre::ColourValue(0.184f, 0.373f, .212f));                        //water
        vecBiomes[2] = Biome(0.403f, Ogre::ColourValue(0.307f, 0.461f, .307f));                        //shallow water
        vecBiomes[3] = Biome(0.508f, Ogre::ColourValue(.449f, .534f, .382f));                        //beach
        vecBiomes[4] = Biome(0.548f, Ogre::ColourValue(.706f, .678f, .600f));                        //grass
        vecBiomes[5] = Biome(0.710f, Ogre::ColourValue(0.733f, 0.561f, 0.518f));                        //tall grass
        vecBiomes[6] = Biome(0.815f, Ogre::ColourValue(0.733f, 0.561f, 0.518f));                        //forest
        vecBiomes[7] = Biome(0.871f, Ogre::ColourValue(.451f, .533f, .384f));                        //mountains
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(.306f, .463f, .306f));                        //snowy peaks

        noiseType = FastNoiseLite::NoiseType_Cellular;
        rotationType3D = FastNoiseLite::RotationType3D_None;
        iSeed = 428;
        fFrequency = 0.000195f;
        fPerFrequencyHeight = .175f;

        fractalType = FastNoiseLite::FractalType_PingPong;
        iOctaves = 6;
        fFractalGain = 0.350f;
        fFractalWeightedStrength = 0.f;
        fFractalLacunarity = 5.f;
        fPingPongStrength = 2.1f;

        cellularDistanceFunction = FastNoiseLite::CellularDistanceFunction_Hybrid;
        cellularReturnType = FastNoiseLite::CellularReturnType_Distance;
        fJitter = 1.f;

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();

        for (auto& ring : vecRings)
        {
            ring.bVisible = false;
            //cause gradient planets dont generate rings
            if (ring.entityY != nullptr)
            {
                ring.entityY->setVisible(false);
                ring.entityNY->setVisible(false);
            }
        }

        break;

    case Preset::Ringed_Giant:
        indexMinBiomeDepth = 9;
        vecBiomes[0] = Biome(-.185f, Ogre::ColourValue(0.946f, 0.812f, 0.570f));
        vecBiomes[1] = Biome(0.258f, Ogre::ColourValue(0.971f, 0.738f, 0.319f));
        vecBiomes[2] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));
        vecBiomes[3] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));
        vecBiomes[4] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));
        vecBiomes[5] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));
        vecBiomes[6] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));
        vecBiomes[7] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(1.f, 0.839f, .549f));

        noiseType = FastNoiseLite::NoiseType_Perlin;
        rotationType3D = FastNoiseLite::RotationType3D_None;
        iSeed = 428;
        fFrequency = 0.0010f;
        fPerFrequencyHeight = .100f;

        fractalType = FastNoiseLite::FractalType_FBm;
        iOctaves = 2;
        fFractalGain = 0.500f;
        fFractalWeightedStrength = 0.f;
        fFractalLacunarity = 0.f;

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();

        //cause gradient planets dont generate rings
        if (meshType != MeshType::GRADIENT)
        {
            vecRings[0].fOuterRingDia = 2.724f;
            vecRings[0].fInnerThickness = 0.078f;
            vecRings[0].colorOuter = Ogre::ColourValue(0.500f, 0.448f, 0.387f);
            vecRings[0].colorInner = Ogre::ColourValue(0.490f, 0.430f, 0.350f);
            vecRings[1].fOuterRingDia = 2.410f;
            vecRings[1].fInnerThickness = 0.224f;
            vecRings[1].colorOuter = Ogre::ColourValue(0.694f, 0.627f, 0.557f);
            vecRings[1].colorInner = Ogre::ColourValue(0.451f, 0.396f, 0.353f);
            vecRings[2].fOuterRingDia = 1.806f;
            vecRings[2].fInnerThickness = 0.119f;
            vecRings[2].colorOuter = Ogre::ColourValue(0.161f, 0.161f, 0.153f);
            vecRings[2].colorInner = Ogre::ColourValue(0.076f, 0.076f, 0.076f);
            for (auto& ring : vecRings)
            {
                ring.bVisible = ring.bVisible = ring.bVisible = true;
                ring.entityY->setVisible(true);
                ring.entityY->setVisible(true);
                ring.entityY->setVisible(true);
                ring.entityNY->setVisible(true);
                ring.entityNY->setVisible(true);
                ring.entityNY->setVisible(true);
                ring.fYaw = 0.f;
                ring.fRoll = 0.f;
                ring.fPitch = 0.f;
                ring.sceneNodeNY->resetOrientation();
                ring.sceneNodeY->resetOrientation();
            }

        }
        

        break;

    case Preset::Ice_Giant:
        indexMinBiomeDepth = 9;
        vecBiomes[0] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[1] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[2] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[3] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[4] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[5] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[6] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[7] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(0.686f, 0.828f, 0.828f));

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();

        //cause gradient planets dont generate rings
        if (meshType != MeshType::GRADIENT)
        {
            vecRings[0].fOuterRingDia = 2.556f;
            vecRings[0].fInnerThickness = 0.030f;
            vecRings[0].colorOuter = Ogre::ColourValue(0.9f, 0.9f, 0.9f);
            vecRings[0].colorInner = Ogre::ColourValue(0.550f, 0.550f, 0.550f);
            vecRings[1].fOuterRingDia = 2.422f;
            vecRings[1].fInnerThickness = 0.011f;
            vecRings[1].colorOuter = Ogre::ColourValue(0.550f, 0.550f, 0.550f);
            vecRings[1].colorInner = Ogre::ColourValue(0.250f, 0.250f, 0.250f);
            vecRings[2].fOuterRingDia = 1.806f;
            vecRings[2].fInnerThickness = 0.f;
            //vecRings[2].colorOuter = Ogre::ColourValue(0.161f, 0.161f, 0.153f);
            //vecRings[2].colorInner = Ogre::ColourValue(0.076f, 0.076f, 0.076f);
            for (auto& ring : vecRings)
            {
                ring.bVisible = ring.bVisible = ring.bVisible = true;
                ring.entityY->setVisible(true);
                ring.entityY->setVisible(true);
                ring.entityY->setVisible(true);
                ring.entityNY->setVisible(true);
                ring.entityNY->setVisible(true);
                ring.entityNY->setVisible(true);
                ring.fYaw = 0.f;
                ring.fRoll = -1.f;
                ring.fPitch = -0.955f;
                ring.sceneNodeNY->resetOrientation();
                ring.sceneNodeY->resetOrientation();

                ring.sceneNodeY->yaw(Ogre::Radian(ring.fYaw), Ogre::Node::TS_WORLD);
                ring.sceneNodeY->roll(Ogre::Radian(ring.fRoll), Ogre::Node::TS_WORLD);
                ring.sceneNodeY->pitch(Ogre::Radian(ring.fPitch), Ogre::Node::TS_WORLD);
                ring.sceneNodeNY->yaw(Ogre::Radian(ring.fYaw), Ogre::Node::TS_WORLD);
                ring.sceneNodeNY->roll(Ogre::Radian(ring.fRoll), Ogre::Node::TS_WORLD);
                ring.sceneNodeNY->pitch(Ogre::Radian(ring.fPitch), Ogre::Node::TS_WORLD);
            }

        }

        break;



        break;

    case Preset::Rusty_Planet:
        indexMinBiomeDepth = 7;
        vecBiomes[0] = Biome(-.798f, Ogre::ColourValue(0.090f, 0.157f, 0.196f));
        vecBiomes[1] = Biome(-.565f, Ogre::ColourValue(0.267f, 0.259f, 0.259f));
        vecBiomes[2] = Biome(-.444f, Ogre::ColourValue(0.369f, 0.302f, .271f));
        vecBiomes[3] = Biome(-0.298f, Ogre::ColourValue(0.506f, .408f, .341f));
        vecBiomes[4] = Biome(-0.169f, Ogre::ColourValue(.631f, .478f, .353f));
        vecBiomes[5] = Biome(-0.081f, Ogre::ColourValue(0.761f, 0.565f, 0.396f));
        vecBiomes[6] = Biome(0.258, Ogre::ColourValue(0.639f, 0.404f, 0.216f));
        vecBiomes[7] = Biome(0.379f, Ogre::ColourValue(.682f, .455f, .267f));
        vecBiomes[8] = Biome(1.f, Ogre::ColourValue(.855f, .875f, .910f));


        noiseType = FastNoiseLite::NoiseType_Cellular;
        rotationType3D = FastNoiseLite::RotationType3D_None;
        iSeed = 428;
        fFrequency = 0.0010f;
        fPerFrequencyHeight = .100f;

        fractalType = FastNoiseLite::FractalType_PingPong;
        iOctaves = 4;
        fFractalGain = 0.500f;
        fFractalWeightedStrength = 2.f;
        fFractalLacunarity = 7.f;
        fPingPongStrength = 2.f;

        cellularDistanceFunction = FastNoiseLite::CellularDistanceFunction_EuclideanSq;
        cellularReturnType = FastNoiseLite::CellularReturnType_Distance;
        fJitter = 1.f;

        bDomainWarp = false;

        for (auto& face : vecFaceNodes)
            face->resetOrientation();

        for (auto& ring : vecRings)
        {
            ring.bVisible = false;
            //cause gradient planets dont generate rings
            if (ring.entityY != nullptr)
            {
                ring.entityY->setVisible(false);
                ring.entityNY->setVisible(false);
            }
        }

        break;

    }

    //call generate 
    generate();
}


void Planet::initMeshValues()
{
    /// Define the vertices (8 vertices, each have 3 floats for position and 3 for normal)
    nSegments = nSections + 1;
    nVertices = nSegments * nSegments;                                     //square of vertices on each side
    vBufCount = 3 * 2 * nVertices;
    //nSections = nSegments - 1;
    iBufCount = nSections * nSections * 6;
    fSideLength = static_cast<float>(iDiaMultiplier * (nSections));
    fSectionLength = fSideLength / nSections;

    //for the rings
    nRingVertices = nSections * 4 * 2;                          //2 for inner and outer rings
    vRingBufCount = 3 * 2 * nRingVertices;
    iRingBufCount = nSections * 4 * 3 * 2;
}

void Planet::setValuesToNoiseObject()
{
    noise.SetNoiseType(noiseType);
    noise.SetRotationType3D(rotationType3D);
    noise.SetSeed(iSeed);
    noise.SetFrequency(fFrequency);

    noise.SetFractalType(fractalType);
    noise.SetFractalOctaves(iOctaves);
    noise.SetFractalGain(fFractalGain);
    noise.SetFractalWeightedStrength(fFractalWeightedStrength);
    noise.SetFractalLacunarity(fFractalLacunarity);
    noise.SetFractalPingPongStrength(fPingPongStrength);

    noise.SetCellularDistanceFunction(cellularDistanceFunction);
    noise.SetCellularReturnType(cellularReturnType);
    noise.SetCellularJitter(fJitter);

    //domain warp stuff
    domainWarp.SetDomainWarpType(domainWarpType);
    domainWarp.SetRotationType3D(domainWarpRotationType3D);
    domainWarp.SetDomainWarpAmp(fDomainWarpAmplitude);
    domainWarp.SetFrequency(fDomainWarpFrequency);

    domainWarp.SetFractalType(domainWarpFractalType);
    domainWarp.SetFractalOctaves(iDWFractalOctaves);
    domainWarp.SetFractalLacunarity(fDWFractalLacunarity);
    domainWarp.SetFractalGain(fDWFractalGain);


}

void Planet::resetToDefaultNoiseValues()
{
    bDomainWarp = false;

    //the default values of the FastNoiseLite object
    noiseType = FastNoiseLite::NoiseType_Perlin;
    rotationType3D = FastNoiseLite::RotationType3D_None;
    iSeed = 428;
    fFrequency = 0.00095f;
    fPerFrequencyHeight = .1f;

    fractalType = FastNoiseLite::FractalType_PingPong;
    iOctaves = 4;
    fFractalGain = .215f;
    fFractalWeightedStrength = 0.f;
    fFractalLacunarity = 5.f;
    fPingPongStrength = 2.1f;

    cellularDistanceFunction = FastNoiseLite::CellularDistanceFunction_EuclideanSq;
    cellularReturnType = FastNoiseLite::CellularReturnType_Distance;
    fJitter = 1.f;

    domainWarpType = FastNoiseLite::DomainWarpType_OpenSimplex2;
    domainWarpRotationType3D = FastNoiseLite::RotationType3D_None;
    fDomainWarpAmplitude = 30.f;
    fDomainWarpFrequency = 0.005f;

    domainWarpFractalType = FastNoiseLite::FractalType_None;
    iDWFractalOctaves = 5;
    fDWFractalLacunarity = 2.f;
    fDWFractalGain = 0.50f;

}

void Planet::resetToDefaultBiomeValues()
{
    interpolationType = InterpolationType::Smooth;
    indexMinBiomeDepth = 3;

    if (vecBiomes.size() == 0)
        vecBiomes.resize(9);
    vecBiomes[0] = Biome(-0.45f, Ogre::ColourValue(0.f, 0.f, 0.5f));                        //deep water
    vecBiomes[1] = Biome(-0.2f, Ogre::ColourValue(0.f, 0.f, 1.f));                        //water
    vecBiomes[2] = Biome(-0.1f, Ogre::ColourValue(0.f, 0.74f, 1.f));                        //shallow water
    vecBiomes[3] = Biome(0.f, Ogre::ColourValue(1.f, .66f, .4f));                        //beach
    vecBiomes[4] = Biome(0.11f, Ogre::ColourValue(.2f, .65f, .2f));                        //grass
    vecBiomes[5] = Biome(0.38f, Ogre::ColourValue(0.f, 0.55f, 0.f));                        //tall grass
    vecBiomes[6] = Biome(0.50f, Ogre::ColourValue(0.f, 0.38f, 0.f));                        //forest
    vecBiomes[7] = Biome(0.65f, Ogre::ColourValue(.5f, .5f, .5f));                        //mountains
    vecBiomes[8] = Biome(1.f, Ogre::ColourValue(1.f, 1.f, 1.f));                        //snowy peaks
}

void Planet::resetToDefaultRingValues()
{
    //reset rings
    vecRings[0].fOuterRingDia = 3.5;
    vecRings[0].fInnerThickness = 0.040;
    vecRings[1].fOuterRingDia = 3.14;
    vecRings[1].fInnerThickness = 0.25f;
    vecRings[2].fOuterRingDia = 2.175;
    vecRings[2].fInnerThickness = 0.097f;
    for (auto& ring : vecRings)
    {
        ring.colorInner = Ogre::ColourValue(0.076f, 0.076f, 0.076f);
        ring.colorOuter = Ogre::ColourValue(0.55f, 0.55f, 0.55f);
        ring.fRoll = ring.fYaw = ring.fPitch = 0.f;
        if (ring.sceneNodeY != nullptr)
        {
            ring.sceneNodeY->resetOrientation();
            ring.sceneNodeNY->resetOrientation();
        }
    }
}

bool Planet::readDATFile()
{
    FILE* fileDat;
    if (fopen_s(&fileDat, "./mesh.dat", "r") == 0)
    {
        fscanf_s(fileDat, "%d", &bAutoLodGeneration);
        fscanf_s(fileDat, "%d", &lightType);

        int iType = 0;
        fscanf_s(fileDat, "%d", &iType);
        nSections = std::clamp(static_cast<size_t>(iType), MinSections, MaxSections);
        fscanf_s(fileDat, "%d", &iDiaMultiplier);
        iDiaMultiplier = std::clamp(iDiaMultiplier, MinDiaMultiplier, MaxDiaMultiplier);
        
        fscanf_s(fileDat, "%f", &fPerFrequencyHeight);
        fPerFrequencyHeight = std::clamp(fPerFrequencyHeight, 0.01f, 0.5f);
        fscanf_s(fileDat, "%d", &indexMinBiomeDepth);
        indexMinBiomeDepth = std::clamp(indexMinBiomeDepth, 0, MaxBiomesIndex);

        fscanf_s(fileDat, "%d", &bDomainWarp);

        //noise
        fscanf_s(fileDat, "%d", &iType);
        noiseType = static_cast<FastNoiseLite::NoiseType>(iType);
        noiseType = noiseType > FastNoiseLite::NoiseType_Value ? FastNoiseLite::NoiseType_Value : noiseType;
        fscanf_s(fileDat, "%d", &iType);
        rotationType3D = static_cast<FastNoiseLite::RotationType3D>(iType);
        rotationType3D = rotationType3D > FastNoiseLite::RotationType3D_ImproveXZPlanes ? FastNoiseLite::RotationType3D_ImproveXZPlanes : rotationType3D;
        fscanf_s(fileDat, "%d", &iSeed);
        fscanf_s(fileDat, "%f", &fFrequency);
        
        fscanf_s(fileDat, "%d", &iType);
        fractalType = static_cast<FastNoiseLite::FractalType>(iType);
        fractalType = fractalType > FastNoiseLite::FractalType_PingPong ? FastNoiseLite::FractalType_PingPong : fractalType;
        fscanf_s(fileDat, "%d", &iOctaves);
        fscanf_s(fileDat, "%f", &fFractalGain);
        fscanf_s(fileDat, "%f", &fFractalWeightedStrength);
        fscanf_s(fileDat, "%f", &fFractalLacunarity);
        fscanf_s(fileDat, "%f", &fPingPongStrength);

        fscanf_s(fileDat, "%d", &iType);
        cellularDistanceFunction = static_cast<FastNoiseLite::CellularDistanceFunction> (iType);
        cellularDistanceFunction = cellularDistanceFunction > FastNoiseLite::CellularDistanceFunction_Hybrid ? FastNoiseLite::CellularDistanceFunction_Hybrid : cellularDistanceFunction;
        fscanf_s(fileDat, "%d", &iType);
        cellularReturnType = static_cast<FastNoiseLite::CellularReturnType> (iType);
        cellularReturnType = cellularReturnType > FastNoiseLite::CellularReturnType_Distance2Div ? FastNoiseLite::CellularReturnType_Distance2Div : cellularReturnType;
        fscanf_s(fileDat, "%f", &fJitter);

        fscanf_s(fileDat, "%d", &iType);
        domainWarpType = static_cast<FastNoiseLite::DomainWarpType>(iType);
        domainWarpType = domainWarpType > FastNoiseLite::DomainWarpType_BasicGrid ? FastNoiseLite::DomainWarpType_BasicGrid : domainWarpType;
        fscanf_s(fileDat, "%d", &iType);
        domainWarpRotationType3D = static_cast<FastNoiseLite::RotationType3D>(iType);
        domainWarpRotationType3D = domainWarpRotationType3D > FastNoiseLite::RotationType3D_ImproveXZPlanes ? FastNoiseLite::RotationType3D_ImproveXZPlanes : domainWarpRotationType3D;
        fscanf_s(fileDat, "%f", &fDomainWarpAmplitude);
        fscanf_s(fileDat, "%f", &fDomainWarpFrequency);

        fscanf_s(fileDat, "%d", &iType);
        domainWarpFractalType = static_cast<FastNoiseLite::FractalType>(iType);
        domainWarpFractalType = domainWarpFractalType > FastNoiseLite::FractalType_DomainWarpIndependent ? FastNoiseLite::FractalType_DomainWarpIndependent : domainWarpFractalType;
        fscanf_s(fileDat, "%d", &iDWFractalOctaves);
        fscanf_s(fileDat, "%f", &fDWFractalLacunarity);
        fscanf_s(fileDat, "%f", &fDWFractalGain);


        //Biomes
        Ogre::ColourValue color;
        float e = 0.f;
        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));                        //deep water

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        fscanf_s(fileDat, "%f,%f,%f,%f", &color.r, &color.g, &color.b, &e);
        vecBiomes.emplace_back(Biome(e, color));

        //3 rings
        Ring ring;
        fscanf_s(fileDat, "%d", &ring.bVisible);
        fscanf_s(fileDat, "%f", &ring.fOuterRingDia);
        fscanf_s(fileDat, "%f", &ring.fInnerThickness);
        fscanf_s(fileDat, "%f", &ring.fPitch);
        fscanf_s(fileDat, "%f", &ring.fYaw);
        fscanf_s(fileDat, "%f", &ring.fRoll);
        fscanf_s(fileDat, "%f,%f,%f", &ring.colorOuter.r, &ring.colorOuter.g, &ring.colorOuter.b);
        fscanf_s(fileDat, "%f,%f,%f", &ring.colorInner.r, &ring.colorInner.g, &ring.colorInner.b);
        vecRings.emplace_back(ring);

        fscanf_s(fileDat, "%d", &ring.bVisible);
        fscanf_s(fileDat, "%f", &ring.fOuterRingDia);
        fscanf_s(fileDat, "%f", &ring.fInnerThickness);
        fscanf_s(fileDat, "%f", &ring.fPitch);
        fscanf_s(fileDat, "%f", &ring.fYaw);
        fscanf_s(fileDat, "%f", &ring.fRoll);
        fscanf_s(fileDat, "%f,%f,%f", &ring.colorOuter.r, &ring.colorOuter.g, &ring.colorOuter.b);
        fscanf_s(fileDat, "%f,%f,%f", &ring.colorInner.r, &ring.colorInner.g, &ring.colorInner.b);
        vecRings.emplace_back(ring);

        fscanf_s(fileDat, "%d", &ring.bVisible);
        fscanf_s(fileDat, "%f", &ring.fOuterRingDia);
        fscanf_s(fileDat, "%f", &ring.fInnerThickness);
        fscanf_s(fileDat, "%f", &ring.fPitch);
        fscanf_s(fileDat, "%f", &ring.fYaw);
        fscanf_s(fileDat, "%f", &ring.fRoll);
        fscanf_s(fileDat, "%f,%f,%f", &ring.colorOuter.r, &ring.colorOuter.g, &ring.colorOuter.b);
        fscanf_s(fileDat, "%f,%f,%f", &ring.colorInner.r, &ring.colorInner.g, &ring.colorInner.b);
        vecRings.emplace_back(ring);

        fclose(fileDat);
    }
    else
        return false;

    return true;
}

void Planet::writeDATFile()
{
    FILE* fileDat;
    fopen_s(&fileDat, "./mesh.dat", "w+");

    fprintf_s(fileDat, "%d\n", bAutoLodGeneration);
    fprintf_s(fileDat, "%d\n", lightType);

    fprintf_s(fileDat, "%d\n", nSections);
    fprintf_s(fileDat, "%d\n", iDiaMultiplier);

    fprintf_s(fileDat, "%f\n", fPerFrequencyHeight);
    fprintf_s(fileDat, "%d\n", indexMinBiomeDepth);
  
    fprintf_s(fileDat, "%d\n", bDomainWarp);

    fprintf_s(fileDat, "%d\n", noiseType);
    fprintf_s(fileDat, "%d\n", rotationType3D);
    fprintf_s(fileDat, "%d\n", iSeed);
    fprintf_s(fileDat, "%f\n", fFrequency);

    fprintf_s(fileDat, "%d\n", fractalType);
    fprintf_s(fileDat, "%d\n", iOctaves);
    fprintf_s(fileDat, "%f\n", fFractalGain);
    fprintf_s(fileDat, "%f\n", fFractalWeightedStrength);
    fprintf_s(fileDat, "%f\n", fFractalLacunarity);
    fprintf_s(fileDat, "%f\n", fPingPongStrength);

    fprintf_s(fileDat, "%d\n", cellularDistanceFunction);
    fprintf_s(fileDat, "%d\n", cellularReturnType);
    fprintf_s(fileDat, "%f\n", fJitter);

    fprintf_s(fileDat, "%d\n", domainWarpType);
    fprintf_s(fileDat, "%d\n", domainWarpRotationType3D);
    fprintf_s(fileDat, "%f\n", fDomainWarpAmplitude);
    fprintf_s(fileDat, "%f\n", fDomainWarpFrequency);

    fprintf_s(fileDat, "%d\n", domainWarpFractalType);
    fprintf_s(fileDat, "%d\n", iDWFractalOctaves);
    fprintf_s(fileDat, "%f\n", fDWFractalLacunarity);
    fprintf_s(fileDat, "%f\n", fDWFractalGain);


    for (auto& biome : vecBiomes)
        fprintf_s(fileDat, "%f,%f,%f,%f\n", biome.color.r, biome.color.g, biome.color.b, biome.e);

    for (auto& ring : vecRings)
    {
        fprintf_s(fileDat, "%d\n", ring.bVisible);
        fprintf_s(fileDat, "%f\n", ring.fOuterRingDia);
        fprintf_s(fileDat, "%f\n", ring.fInnerThickness);
        fprintf_s(fileDat, "%f\n", ring.fPitch);
        fprintf_s(fileDat, "%f\n", ring.fYaw);
        fprintf_s(fileDat, "%f\n", ring.fRoll);
        fprintf_s(fileDat, "%f,%f,%f\n", ring.colorOuter.r, ring.colorOuter.g, ring.colorOuter.b);
        fprintf_s(fileDat, "%f,%f,%f\n", ring.colorInner.r, ring.colorInner.g, ring.colorInner.b);
    }

    fclose(fileDat);
}