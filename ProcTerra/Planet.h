#pragma once
#include <Ogre.h>
#include <vector>
#include "FastNoiseLite.h"

constexpr int MaxDiaMultiplier = 50;
constexpr int MinDiaMultiplier = 4;
constexpr size_t MaxSections = 250;
constexpr size_t MinSections = 20;
constexpr int MaxBiomesIndex = 8;
constexpr float MinOuterRingDia = 1.f;
constexpr float MaxOuterRingDia = 4.f;

enum class Preset
{
	Swift_Planet,									//mercury
	Evening_Star,
	Rocky_Moon,										//moon                                               
	Blue_Marble,									//earth
	Ringed_Giant,									//saturn
	Ice_Giant,										//uranus/neptune
	Rusty_Planet									//mars
};

//biome color interpolation 
enum class InterpolationType
{
	Smooth,
	Intermediate,
	Sharp
};

//ambient or sunlight
enum class LightType
{
	AMBIENT,
	DIRECTIONAL
};

enum class MeshType
{
	NORMAL_BIOMES,											//normal mesh with biomes, the main planet
	GRADIENT												//gradient with colors from white to black used as reference for main planet in the mini viewport
};

struct Biome
{
	float e;													//elevation coefficient
	Ogre::ColourValue color;
	Biome(float e = 1.f, Ogre::ColourValue color = Ogre::ColourValue(1.f, 1.f, 1.f))
		: e(e), color(color)
	{
	}
};

struct Ring
{
	bool bVisible;											//only render if visible
	float fOuterRingDia, fInnerThickness;												//ring diameter (outer ring) is from 1.f to 3.f wrt planet dia and thickness (inner ring) is 0.f to 1.0f wrt outer ring 
	float fYaw, fPitch, fRoll;
	Ogre::ColourValue colorInner, colorOuter;
	Ogre::MeshPtr mshY, mshNY;								//UNIT_Y / NEGATIVE_UNIT_Y facing mesh faces
	Ogre::Entity* entityY, *entityNY;
	Ogre::SceneNode* sceneNodeY, *sceneNodeNY;
	Ring() :
		fOuterRingDia(1.f), fInnerThickness(0.15f),
		colorInner(0.15f, 0.15f, 0.15f),
		colorOuter(Ogre::ColourValue(0.7f, 0.7f, 0.7f)), 
		bVisible(false),
		fYaw(0.f), fPitch(0.f), fRoll(0.f),
		entityY(nullptr), entityNY(nullptr),
		sceneNodeY(nullptr), sceneNodeNY(nullptr)
	{}
};

class Planet
{
	Ogre::SceneManager* mSceneMgr;
	std::string strName;									//planet name also associated with names of meshes
	MeshType meshType;										//normal_biome for the primary planet, gradient for the gradient one in the corner viewport, gradient also doesnt have rings
	Ogre::MaterialPtr material;

	//sunlight / ambient light	
	LightType lightType;								//0 is ambient 1 is sunlight, should always be ambient for gradient mesh
	
	//these vertex and index positions will used 6 times for each face only rotated when setting the actual position of the vertex for the mesh vertex buffer
	//rotation will be applied according to the direction the face will face
	std::vector<Ogre::Vector3> vecVertices;
	std::vector<unsigned short> vecIndices;
	std::vector<Ogre::MeshPtr> vecFaces;					//all 6 faces 
	std::vector<Ogre::SceneNode*> vecFaceNodes;

	//2 faces for the ring meshes	+y and -y
	size_t nRingVertices, vRingBufCount, iRingBufCount;
	std::vector<Ogre::Vector3> vecRingVertices;										//starting from outer to inner ring
	std::vector<unsigned short> vecRingIndices;										//starting from outer to inner ring

	FastNoiseLite noise, domainWarp;

public:
	//Mesh properties
	////dimensions, vertices and index order for each face of the cube
	size_t nSegments, nSections, nIndices, nVertices, vBufCount, iBufCount;								//num vertices of each side
	int iDiaMultiplier;																					//fSideLength is calculated using fDiaMultiplier * nSegments
	float fSideLength;																					//length of each side
	float fSectionLength;																				//length of each section = side / (segments - 1)
	float fPerFrequencyHeight;																			//1% to 50% (.01 to 0.5) the radius of planet. added to planet radius which makes mountains etc. possible
	bool bRenderElevation;																				//for gradient mesh only. normal mesh renders elevation based on biomes
	Ogre::uint32 visibilityMask;
	//the 9 biomes
	std::vector<Biome> vecBiomes;
	InterpolationType interpolationType;																//not for gradient type planet
	int indexMinBiomeDepth;																				//the index of the biome from which minimum biome height is calculated
	//auto lod
	bool bAutoLodGeneration;
	//rotation
	bool bYaw, bPitch, bRoll;
	float fYaw, fPitch, fRoll;

	//Noise properties
	FastNoiseLite::NoiseType noiseType;																	//general
	FastNoiseLite::RotationType3D rotationType3D;
	int iSeed;
	float fFrequency;
	FastNoiseLite::FractalType fractalType;																//fractal
	int iOctaves;
	float fFractalGain, fFractalWeightedStrength, fFractalLacunarity, fPingPongStrength;
	FastNoiseLite::CellularDistanceFunction cellularDistanceFunction;									//cellular
	FastNoiseLite::CellularReturnType cellularReturnType;
	float fJitter;
	//domain warp properties
	bool bDomainWarp;																					//adds domain warp if true to the points
	FastNoiseLite::DomainWarpType domainWarpType;														//domain warp
	FastNoiseLite::RotationType3D domainWarpRotationType3D;
	float fDomainWarpAmplitude, fDomainWarpFrequency;
	FastNoiseLite::FractalType domainWarpFractalType;													//domain warp fractal
	int iDWFractalOctaves;
	float fDWFractalLacunarity, fDWFractalGain;

	//rings
	std::vector<Ring> vecRings;

	//MAX Sections allowed = 250 or else everything will be destroyed
	Planet(Ogre::SceneManager* mSceneMgr, MeshType meshType, std::string strName, Ogre::uint32 visibilityMask);
	void init();
	void update(const float& fDeltaTime);																//planet rotation update etc.
	void generate();																					//update the planet mesh and generate the planet

	void initMeshValues();																				//set num of vertices, indices etc. 
	void resetToDefaultNoiseValues();																	//reset to default noise values
	void resetToDefaultBiomeValues();																	//reset to default biome colors
	void resetToDefaultRingValues();

	bool readDATFile();
	void writeDATFile();

	LightType getLightType() const { return lightType; };
	FastNoiseLite getNoise() { return noise; };
	void setPreset(const Preset preset);
	void setNoise(const FastNoiseLite fn) { this->noise = fn; };
	void setLightType(const LightType lightType) { this->lightType = lightType; };
	
private:
	void setValuesToNoiseObject();																		//sets the noise varialbes to the FastNoiseLite object

	void createDefaultFaceVerticesAndIndices();															//for both planet mesh and rings	
	//for planet mesh
	Ogre::MeshPtr createNormalisedFace(const Ogre::Vector3 vFace, const std::string strItem, const std::string strEntity);
	void updateMesh(const Ogre::Mesh* const mesh, const Ogre::Vector3 vFace);
	Ogre::ColourValue biomeColorInterpolation(const float& e, std::vector<Biome>::iterator& iter);

	//for rings
	Ogre::MeshPtr createRing(const Ogre::Vector3 vFace, Ogre::Entity** entity, Ogre::SceneNode** node, const Ogre::ColourValue colorInner, const Ogre::ColourValue colorOuter, const std::string strItem, const std::string strEntity);
	void updateRing(const Ogre::Mesh* const mesh, const Ogre::Vector3 vFace, const float fOuterRingDia, const float fInnerThickness, const Ogre::ColourValue colorInner, const Ogre::ColourValue colorOuter);

};

