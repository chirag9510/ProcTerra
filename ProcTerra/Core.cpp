#include "Core.h"
#include <Ogre.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <OgreFileSystemLayer.h>
#include <RTShaderSystem/OgreRTShaderSystem.h>
#include <Bites/OgreSGTechniqueResolverListener.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayContainer.h>
#include <OgreTextAreaOverlayElement.h>
#include <imgui.h>

Core::Core() :
	OgreBites::ApplicationContext("Procedural Terra"),
	lightType(LightType::AMBIENT),
	colorAmbient(1.f, 1.f, 1.f),
	colorSLDiffuse(.85f, 0.86, 0.86, 0.95),
	fSLPowerScale(1.512f),
	vSLDirection(Ogre::Vector3(.55f, .2f, -.75)),
	fWindowSize(.25f),
	colorMiniScreen(Ogre::ColourValue(0.f, 0.f, 0.f)),
	colorPrimary(Ogre::ColourValue::Black),
	bWireFrame(false),
	bToggleFreelook(false),
	bToggleSkybox(true)
{
}

//run
bool Core::keyPressed(const OgreBites::KeyboardEvent& evt)
{
	if(!mListenerChain.keyPressed(evt))
		cameraMan->keyPressed(evt);

	if (evt.keysym.sym == OgreBites::SDLK_PAGEUP)
	{
		resetCameraPosition();
		bToggleFreelook = !bToggleFreelook;
		if (bToggleFreelook)
		{
			cameraMan->setStyle(OgreBites::CS_FREELOOK);
			//mouse movement for imgui will be disabled in free look mode, set camera in orbit mode to use imgui
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		else
		{
			cameraMan->setStyle(OgreBites::CS_ORBIT);
			//mouse movement for imgui enabled again
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}
	}
	else if (evt.keysym.sym == OgreBites::SDLK_HOME)
		resetCameraPosition();
	else if (evt.keysym.sym == OgreBites::SDLK_ESCAPE)
	{
		mRoot->queueEndRendering();
	}

	return true;
}

bool Core::keyReleased(const OgreBites::KeyboardEvent& evt)
{
	if(!mListenerChain.keyReleased(evt))
		cameraMan->keyReleased(evt);
	return true;
}

bool Core::mouseMoved(const OgreBites::MouseMotionEvent& evt)
{
	if (mListenerChain.mouseMoved(evt))
	{
		if (cameraMan->getStyle() == OgreBites::CS_FREELOOK)
			SDL_ShowCursor(SDL_TRUE);
	}
	else
	{
		cameraMan->mouseMoved(evt);
		if (cameraMan->getStyle() == OgreBites::CS_FREELOOK)
			SDL_ShowCursor(SDL_FALSE);
	}
	return true;
}

bool Core::mouseWheelRolled(const OgreBites::MouseWheelEvent& evt)
{
	if(!mListenerChain.mouseWheelRolled(evt))
		cameraMan->mouseWheelRolled(evt);
	return true;
}

bool Core::mousePressed(const OgreBites::MouseButtonEvent& evt)
{
	if (!mListenerChain.mousePressed(evt))
	{
		//disabled right click camera movement for CS_ORBIT camera type 
		if (cameraMan->getStyle() == OgreBites::CS_ORBIT && evt.button == OgreBites::BUTTON_RIGHT)
			return true;

		cameraMan->mousePressed(evt);
	}
	return true;
}

bool Core::mouseReleased(const OgreBites::MouseButtonEvent& evt)
{
	if(!mListenerChain.mouseReleased(evt))
		cameraMan->mouseReleased(evt);
	return true;
}

bool Core::textInput(const OgreBites::TextInputEvent& evt) 
{
	return mListenerChain.textInput(evt); 
}


bool Core::frameStarted(const Ogre::FrameEvent& evt)
{
	OgreBites::ApplicationContext::frameStarted(evt);

	//update planet rotation
	planet->update(evt.timeSinceLastFrame);
	planetGradient->update(evt.timeSinceLastFrame);
	
	Ogre::ImGuiOverlay::NewFrame();
	
	//enable mouse cursor when hovering over an imgui window if on freelook
	if (cameraMan->getStyle() == OgreBites::CS_FREELOOK)
	{
		if (ImGui::GetIO().WantCaptureMouse)
		{
			if(SDL_GetRelativeMouseMode() == SDL_TRUE)
				SDL_SetRelativeMouseMode(SDL_FALSE);
		}
		else if(SDL_GetRelativeMouseMode() == SDL_FALSE)
			SDL_SetRelativeMouseMode(SDL_TRUE);
	}

	//popup main menu
	if (ImGui::BeginPopupContextVoid())
	{
		if (ImGui::MenuItem("Generate!!"))
		{
			planet->generate();
			planetGradient->generate();
		}
		ImGui::MenuItem("Presets", nullptr, &bSelected[0]);
		ImGui::MenuItem("Noise", nullptr, &bSelected[1]);
		ImGui::MenuItem("Biomes", nullptr, &bSelected[2]);
		ImGui::MenuItem("Rings", nullptr, &bSelected[3]);
		ImGui::MenuItem("Mesh", nullptr, &bSelected[4]);
		ImGui::MenuItem("Lighting", nullptr, &bSelected[5]);
		ImGui::MenuItem("Settings", nullptr, &bSelected[6]);
		if (ImGui::MenuItem("Quit"))
			mRoot->queueEndRendering();
		ImGui::EndPopup();
	}
	//configs
	if (bSelected[0] && ImGui::Begin("Planet Presets", &bSelected[0]))
	{
		if (ImGui::Button("Rocky Moon"))
		{
			planet->setPreset(Preset::Rocky_Moon);
			planetGradient->setPreset(Preset::Rocky_Moon);
		}

		if (ImGui::Button("Swift Planet"))
		{
			planet->setPreset(Preset::Swift_Planet);
			planetGradient->setPreset(Preset::Swift_Planet);
		}

		if (ImGui::Button("Evening Star"))
		{
			planet->setPreset(Preset::Evening_Star);
			planetGradient->setPreset(Preset::Evening_Star);
		}

		if (ImGui::Button("Blue Marble"))
		{
			planet->setPreset(Preset::Blue_Marble);
			planetGradient->setPreset(Preset::Blue_Marble);
		}

		if (ImGui::Button("Rusty Planet"))
		{
			planet->setPreset(Preset::Rusty_Planet);
			planetGradient->setPreset(Preset::Rusty_Planet);
		}

		if (ImGui::Button("Ringed Giant"))
		{
			planet->setPreset(Preset::Ringed_Giant);
			planetGradient->setPreset(Preset::Ringed_Giant);
		}

		if (ImGui::Button("Ice Giant"))
		{
			planet->setPreset(Preset::Ice_Giant);
			planetGradient->setPreset(Preset::Ice_Giant);
		}
	}


	if (bSelected[1] && ImGui::Begin("Noise Configuration", &bSelected[1]))
	{
		//general
		ImGui::Text("General");
		const char* szNoiseType[] = { "OpenSimplex2", "OpenSimplex2S","Cellular","Perlin","ValueCubic","Value" };
		imSelection = planet->noiseType;
		ImGui::ListBox("Noise Type", &imSelection, szNoiseType, ARRAYSIZE(szNoiseType));
		planet->noiseType = planetGradient->noiseType = static_cast<FastNoiseLite::NoiseType>(imSelection);

		const char* szRotation3D[] = { "None", "Improve XYPlanes","Improve XZPlanes"};
		imSelection = planet->rotationType3D;
		ImGui::ListBox("Rotation Type", &imSelection, szRotation3D, ARRAYSIZE(szRotation3D));
		planet->rotationType3D = planetGradient->rotationType3D = static_cast<FastNoiseLite::RotationType3D>(imSelection);

		ImGui::InputInt("Seed", &planet->iSeed);
		planetGradient->iSeed = planet->iSeed;
		ImGui::InputFloat("Frequency", &planet->fFrequency, 0.f, 0.f, "%.6f");
		planetGradient->fFrequency = planet->fFrequency;
		ImGui::SliderFloat("Frequency Height", &planet->fPerFrequencyHeight, .01f, .50f);
		planetGradient->fPerFrequencyHeight = planet->fPerFrequencyHeight;


		//fractal
		ImGui::NewLine();
		ImGui::Text("Fractal");
		const char* szFractalType[] = { "None", "FBm","Ridged","Ping Pong" };
		imSelection = planet->fractalType;
		ImGui::ListBox("Fractal Type", &imSelection, szFractalType, ARRAYSIZE(szFractalType));
		planet->fractalType = planetGradient->fractalType = static_cast<FastNoiseLite::FractalType>(imSelection);

		if (planet->fractalType != FastNoiseLite::FractalType_None)
		{
			ImGui::InputInt("Octaves", &planet->iOctaves);
			planetGradient->iOctaves = planet->iOctaves;
			ImGui::InputFloat("Gain", &planet->fFractalGain, 0.f, 0.f, "%.6f");
			planetGradient->fFractalGain = planet->fFractalGain;
			ImGui::InputFloat("Weighted Strength", &planet->fFractalWeightedStrength, 0.f, 0.f, "%.6f");
			planetGradient->fFractalWeightedStrength = planet->fFractalWeightedStrength;
			ImGui::InputFloat("Lacunarity", &planet->fFractalLacunarity, 0.f, 0.f, "%.6f");
			planetGradient->fFractalLacunarity = planet->fFractalLacunarity;
			if (planet->fractalType == FastNoiseLite::FractalType_PingPong)
			{
				ImGui::InputFloat("Ping Pong Strength", &planet->fPingPongStrength, 0.f, 0.f, "%.6f");
				planetGradient->fPingPongStrength = planet->fPingPongStrength;
			}
		}


		//cellular
		if (planet->noiseType == FastNoiseLite::NoiseType_Cellular)
		{
			ImGui::NewLine();
			ImGui::Text("Cellular");
			const char* szDistanceFunc[] = { "Euclidean", "Euclidean Sq","Manhattan","Hybrid" };
			imSelection = planet->cellularDistanceFunction;
			ImGui::ListBox("Distance Function", &imSelection, szDistanceFunc, ARRAYSIZE(szDistanceFunc));
			planet->cellularDistanceFunction = planetGradient->cellularDistanceFunction = static_cast<FastNoiseLite::CellularDistanceFunction>(imSelection);

			const char* szReturnType[] = { "Cell Value", "Distance","Distance 2","Distance 2 Add", "Distance 2 Sub", "Distance 2 Mul", "Distance 2 Div"};
			imSelection = planet->cellularReturnType;
			ImGui::ListBox("Return Type", &imSelection, szReturnType, ARRAYSIZE(szReturnType));
			planet->cellularReturnType = planetGradient->cellularReturnType = static_cast<FastNoiseLite::CellularReturnType>(imSelection);

			ImGui::InputFloat("Jitter", &planet->fJitter, 0.f, 0.f, "%.6f");
			planetGradient->fJitter = planet->fJitter;
		}

		ImGui::NewLine();
		ImGui::Checkbox("Toggle Domain Warp", &planet->bDomainWarp);
		planetGradient->bDomainWarp = planet->bDomainWarp;

		////domain warp
		if (planet->bDomainWarp)
		{
			ImGui::NewLine();
			ImGui::Text("Domain Warp");
			const char* szDomainWarpType[] = { "Open Simplex 2","Open Simplex 2 Reduced", "Basic Grid" };
			imSelection = planet->domainWarpType;
			ImGui::ListBox("Domain Warp Type", &imSelection, szDomainWarpType, ARRAYSIZE(szDomainWarpType));
			planet->domainWarpType = planetGradient->domainWarpType = static_cast<FastNoiseLite::DomainWarpType>(imSelection);

			const char* szRotation3D[] = { "None", "Improve XYPlanes","Improve XZPlanes" };
			imSelection = planet->domainWarpRotationType3D;
			ImGui::ListBox("DW Rotation Type", &imSelection, szRotation3D, ARRAYSIZE(szRotation3D));
			planet->domainWarpRotationType3D = planetGradient->domainWarpRotationType3D = static_cast<FastNoiseLite::RotationType3D>(imSelection);

			ImGui::InputFloat("Amplitude", &planet->fDomainWarpAmplitude, 0.f, 0.f, "%.6f");
			planetGradient->fDomainWarpAmplitude = planet->fDomainWarpAmplitude;
			ImGui::InputFloat("DW Frequency", &planet->fDomainWarpFrequency, 0.f, 0.f, "%.6f");
			planetGradient->fDomainWarpFrequency = planet->fDomainWarpFrequency;
			ImGui::Separator();

			//domain warp fractal
			ImGui::NewLine();
			ImGui::Text("Domain Warp Fractal");
			const char* szDomainWarpFractalType[] = { "None","Domain Warp Progressive", "Domain Warp Independent" };
			imSelection = planet->domainWarpFractalType ? planet->domainWarpFractalType - 3 : 0;
			ImGui::ListBox("DW Fractal Type", &imSelection, szDomainWarpFractalType, ARRAYSIZE(szDomainWarpFractalType));
			imSelection = imSelection ? imSelection + 3 : 0;
			planet->domainWarpFractalType = planetGradient->domainWarpFractalType = static_cast<FastNoiseLite::FractalType>(imSelection);

			if (planet->domainWarpFractalType != FastNoiseLite::FractalType::FractalType_None)
			{
				ImGui::InputInt("DW Fractal Octaves", &planet->iDWFractalOctaves);
				planetGradient->iDWFractalOctaves = planet->iDWFractalOctaves;
				ImGui::InputFloat("DW Fractal Gain", &planet->fDWFractalGain, 0.f, 0.f, "%.6f");
				planetGradient->fDWFractalGain = planet->fDWFractalGain;
				ImGui::InputFloat("DW Fractal Lacunarity", &planet->fDWFractalLacunarity, 0.f, 0.f, "%.6f");
				planetGradient->fDWFractalLacunarity = planet->fDWFractalLacunarity;
			}
		}
		

		ImGui::NewLine();
		ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "(!) Generate to update the Planet Mesh.");

		ImGui::NewLine();
		if (ImGui::Button("Reset to Defaults"))
		{
			planet->resetToDefaultNoiseValues();
			planetGradient->resetToDefaultNoiseValues();
		}

	}
	if (bSelected[2] && ImGui::Begin("Biome Configuration", &bSelected[2]))
	{
		ImGui::Text("Biome Interpolation Type");
		imSelection = static_cast<int>(planet->interpolationType);
		const char* szInterpolationType[] = { "Smooth", "Intermediate", "Sharp"};
		ImGui::ListBox("Interpolation Type", &imSelection, szInterpolationType, ARRAYSIZE(szInterpolationType));
		planet->interpolationType = static_cast<InterpolationType>(imSelection);

		ImGui::NewLine();
		std::string strBiome = "";
		imSelection = 0;
		for (auto iter = planet->vecBiomes.begin(); iter != planet->vecBiomes.end(); iter++)
		{

			fColor[0] = iter->color.r, fColor[1] = iter->color.g, fColor[2] = iter->color.b;
			ImGui::SliderFloat(std::string("Biome " + std::to_string(imSelection)).c_str(), &iter->e, -1.0f, 1.0f);
			if (iter + 1 == planet->vecBiomes.end())
				ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Recommended: Keep it to 1.f");
			ImGui::ColorEdit3(std::string("Color " + std::to_string(imSelection++)).c_str(), fColor);
			iter->color = Ogre::ColourValue(fColor[0], fColor[1], fColor[2]);
			ImGui::NewLine();
		}

		const char* szBiomes[] = { "None", "Biome 0", "Biome 1","Biome 2","Biome 3","Biome 4","Biome 5", "Biome 6", "Biome 7", "Biome 8" };
		ImGui::ListBox("Minimum Biome Depth", &planet->indexMinBiomeDepth, szBiomes, ARRAYSIZE(szBiomes));
		
		ImGui::NewLine();
		ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "(!) Generate to update the Planet Mesh.");

		ImGui::NewLine();
		ImGui::Text("Planet Rotation");
		ImGui::Checkbox("Set Yaw", &planet->bYaw);
		planetGradient->bYaw = planet->bYaw;
		if (planet->bYaw)
		{
			ImGui::InputFloat("Yaw", &planet->fYaw);
			ImGui::SliderFloat("Slider Yaw", &planet->fYaw, -5.0f, 5.0f);
			planetGradient->fYaw = planet->fYaw;
		}
		ImGui::Checkbox("Set Pitch", &planet->bPitch);
		planetGradient->bPitch = planet->bPitch;
		if (planet->bPitch)
		{
			ImGui::InputFloat("Pitch", &planet->fPitch);
			ImGui::SliderFloat("Slider Pitch", &planet->fPitch, -5.0f, 5.0f);
			planetGradient->fPitch = planet->fPitch;
		}
		ImGui::Checkbox("Set Roll", &planet->bRoll);
		planetGradient->bRoll = planet->bRoll;
		if (planet->bRoll)
		{
			ImGui::InputFloat("Roll", &planet->fRoll);
			ImGui::SliderFloat("Slider Roll", &planet->fRoll, -5.0f, 5.0f);
			planetGradient->fRoll = planet->fRoll;
		}


		ImGui::NewLine();
		if (ImGui::Button("Reset to Defaults"))
			planet->resetToDefaultBiomeValues();
	}
	if (bSelected[3] && ImGui::Begin("Rings Configuration", &bSelected[3]))
	{
		imSelection = 0;
		for (auto& ring : planet->vecRings)
		{
			if (ImGui::Checkbox(std::string("Toggle Ring " + std::to_string(imSelection)).c_str(), &ring.bVisible))
			{
				ring.entityY->setVisible(ring.bVisible);
				ring.entityNY->setVisible(ring.bVisible);
			}
			
			ImGui::InputFloat(std::string("Size Ring " + std::to_string(imSelection)).c_str(), &ring.fOuterRingDia);
			ring.fOuterRingDia = std::clamp(ring.fOuterRingDia, 1.f, MaxOuterRingDia);
			ImGui::SliderFloat(std::string("Size Slider Ring " + std::to_string(imSelection)).c_str(), &ring.fOuterRingDia, 1.f, 4.f);

			ImGui::InputFloat(std::string("Width Ring " + std::to_string(imSelection)).c_str(), &ring.fInnerThickness);
			ring.fInnerThickness = std::clamp(ring.fInnerThickness, 0.f, 1.f);
			ImGui::SliderFloat(std::string("Width Slider Ring " + std::to_string(imSelection)).c_str(), &ring.fInnerThickness, 0.f, 1.f);

			fColor[0] = ring.colorOuter.r, fColor[1] = ring.colorOuter.g, fColor[2] = ring.colorOuter.b;
			ImGui::ColorEdit3(std::string("Color Outer Ring " + std::to_string(imSelection)).c_str(), fColor);
			ring.colorOuter = Ogre::ColourValue(fColor[0], fColor[1], fColor[2]);

			fColor[0] = ring.colorInner.r, fColor[1] = ring.colorInner.g, fColor[2] = ring.colorInner.b;
			ImGui::ColorEdit3(std::string("Color Inner Ring " + std::to_string(imSelection)).c_str(), fColor);
			ring.colorInner = Ogre::ColourValue(fColor[0], fColor[1], fColor[2]);

			fSelection = ring.fYaw;
			ImGui::InputFloat(std::string("Yaw Ring " + std::to_string(imSelection)).c_str(), &ring.fYaw);
			ring.fYaw = std::clamp(ring.fYaw, -1.f, 1.0f);
			ImGui::SliderFloat(std::string("Yaw Slider Ring " + std::to_string(imSelection)).c_str(), &ring.fYaw, -1.f, 1.f);
			ring.sceneNodeY->yaw(Ogre::Radian(ring.fYaw - fSelection), Ogre::Node::TS_WORLD);
			ring.sceneNodeNY->yaw(Ogre::Radian(ring.fYaw - fSelection), Ogre::Node::TS_WORLD);

			fSelection = ring.fPitch;
			ImGui::InputFloat(std::string("Pitch Ring " + std::to_string(imSelection)).c_str(), &ring.fPitch);
			ring.fPitch = std::clamp(ring.fPitch, -1.f, 1.0f);
			ImGui::SliderFloat(std::string("Pitch Slider Ring " + std::to_string(imSelection)).c_str(), &ring.fPitch, -1.f, 1.f);
			ring.sceneNodeY->pitch(Ogre::Radian(ring.fPitch - fSelection), Ogre::Node::TS_WORLD);
			ring.sceneNodeNY->pitch(Ogre::Radian(ring.fPitch - fSelection), Ogre::Node::TS_WORLD);

			fSelection = ring.fRoll;
			ImGui::InputFloat(std::string("Roll Ring " + std::to_string(imSelection)).c_str(), &ring.fRoll);
			ring.fRoll = std::clamp(ring.fRoll, -1.f, 1.0f);
			ImGui::SliderFloat(std::string("Roll Slider Ring " + std::to_string(imSelection++)).c_str(), &ring.fRoll, -1.f, 1.f);
			ring.sceneNodeY->roll(Ogre::Radian(ring.fRoll - fSelection), Ogre::Node::TS_WORLD);
			ring.sceneNodeNY->roll(Ogre::Radian(ring.fRoll - fSelection), Ogre::Node::TS_WORLD);

			ImGui::NewLine();

		}

		ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "(!) Generate to update the Ring Meshes.");

		ImGui::NewLine();
		if (ImGui::Button("Reset to Defaults"))
			planet->resetToDefaultRingValues();
	}
	if (bSelected[4] && ImGui::Begin("Mesh Configuration", &bSelected[4]))
	{
		ImGui::SliderInt("Segments", &imSections, MinSections, MaxSections);													//label is Segments, value bieng changed is Sections
		ImGui::SliderInt("Diameter Multiplier", &imDiaMultiplier, MinDiaMultiplier, MaxDiaMultiplier);
		ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "(!) Restart app for these changes to take effect.");

	}
	if (bSelected[5] && ImGui::Begin("Lighting Configuration", &bSelected[5]))
	{
		imSelection = static_cast<int>(lightType);
		ImGui::RadioButton("Ambient Light", &imSelection, 0);
		ImGui::RadioButton("Sunlight", &imSelection, 1);
		lightType = static_cast<LightType>(imSelection);

		if (lightType == planet->getLightType())
		{
			if (lightType == LightType::AMBIENT)
			{
				//ambient colors
				fColor4[0] = colorAmbient.r, fColor4[1] = colorAmbient.g, fColor4[2] = colorAmbient.b, fColor4[3] = colorAmbient.a;
				ImGui::ColorEdit4("Ambient Color", fColor4);
				colorAmbient = Ogre::ColourValue(fColor4[0], fColor4[1], fColor4[2], fColor4[3]);
				mSceneMgr->setAmbientLight(colorAmbient);
			}
			else if (lightType == LightType::DIRECTIONAL)
			{
				//sunlight colors
				fColor4[0] = colorSLDiffuse.r, fColor4[1] = colorSLDiffuse.g, fColor4[2] = colorSLDiffuse.b, fColor4[3] = colorSLDiffuse.a;
				ImGui::ColorEdit4("Diffuse", fColor4);
				colorSLDiffuse = Ogre::ColourValue(fColor4[0], fColor4[1], fColor4[2], fColor4[3]);

				ImGui::SliderFloat("Power Scale", &fSLPowerScale, 0.f, 5.f);

				ImGui::Text("Light Direction");
				ImGui::SliderFloat("X", &vSLDirection.x, -1.0f, 1.0f);
				ImGui::SliderFloat("Y", &vSLDirection.y, -1.0f, 1.0f);
				ImGui::SliderFloat("Z", &vSLDirection.z, -1.0f, 1.0f);

				sunlight->setDiffuseColour(colorSLDiffuse);
				sunlight->setPowerScale(fSLPowerScale);
				sunlightNode->setDirection(vSLDirection);
			}

		}
		else
			ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "(!) Restart app for change in Lighting Type to take effect.");

	}
	if (bSelected[6] && ImGui::Begin("Application Settings", &bSelected[6]))
	{
		//Camera settings
		ImGui::Text("Camera Settings");
		if(ImGui::Checkbox("Enable Freelook", &bToggleFreelook))
			resetCameraPosition();
		if (bToggleFreelook && cameraMan->getStyle() == OgreBites::CS_ORBIT)
		{
			cameraMan->setStyle(OgreBites::CS_FREELOOK);
			//mouse movement for imgui will be disabled in free look mode, set camera in orbit mode to use imgui
			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
		else if(!bToggleFreelook && cameraMan->getStyle() == OgreBites::CS_FREELOOK)
		{
			cameraMan->setStyle(OgreBites::CS_ORBIT);
			//mouse movement for imgui enabled again
			SDL_SetRelativeMouseMode(SDL_FALSE);
		}

		ImGui::Checkbox("Enable Wireframe Mode", &bWireFrame);
		if (bWireFrame && mCamera->getPolygonMode() == Ogre::PM_SOLID)
		{
			mCamera->setPolygonMode(Ogre::PM_WIREFRAME);
			//disable the mini screen since the texture also turns to wireframe making it useless
			recMiniScreen->setCorners(-1.f, -1.f, -1.f, -1.f);
		}
		else if (!bWireFrame && mCamera->getPolygonMode() == Ogre::PM_WIREFRAME)
		{
			mCamera->setPolygonMode(Ogre::PM_SOLID);
			recMiniScreen->setCorners(-1.f, -1.f + (2.f * fWindowSize), -1.f + (2.f * fWindowSize),  -1.f);
		}
		if (ImGui::Button("Reset Camera Position"))
			resetCameraPosition();

		ImGui::NewLine();

		//main window
		ImGui::Text("Main Window Settings");
		if (ImGui::Checkbox("Toggle Skybox", &bToggleSkybox))
			mSceneMgr->setSkyBoxEnabled(bToggleSkybox);
		if (!bToggleSkybox)
		{
			fColor[0] = colorPrimary.r, fColor[1] = colorPrimary.g, fColor[2] = colorPrimary.b;
			ImGui::ColorEdit3("Color Main", fColor);
			colorPrimary = Ogre::ColourValue(fColor[0], fColor[1], fColor[2]);
			vpPrimary->setBackgroundColour(colorPrimary);
		}
		ImGui::NewLine();

		//Mini Screen settings
		ImGui::Text("Mini Window Settings");
		fColor[0] = colorMiniScreen.r, fColor[1] = colorMiniScreen.g, fColor[2] = colorMiniScreen.b;
		ImGui::ColorEdit3("Color Mini", fColor);
		colorMiniScreen = Ogre::ColourValue(fColor[0], fColor[1], fColor[2]);
		vpMiniScreen->setBackgroundColour(colorMiniScreen);

		ImGui::SliderFloat("Window Size", &fWindowSize, 0.f, 1.f, "%.2f");
		recMiniScreen->setCorners(-1.f, -1.f + (2.f * fWindowSize), -1.f + (2.f * fWindowSize), -1.f);

	
		ImGui::NewLine();
		ImGui::Checkbox("Toggle Auto LOD Generation (for slower GPUs)", &planet->bAutoLodGeneration);
		//planetGradient->bAutoLodGeneration = planet->bAutoLodGeneration;
		ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "(!) Restart app for LOD Generation to take effect.");

		ImGui::NewLine();
		ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "//chirag 2022");

	}
	
	ImGui::EndFrame();


	return true;
}

bool Core::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	OgreBites::ApplicationContext::frameRenderingQueued(evt);
	cameraMan->frameRendered(evt);
	
	return true;
}

bool Core::frameEnded(const Ogre::FrameEvent& evt)
{
	OgreBites::ApplicationContext::frameEnded(evt);

	return true;
}

void Core::preRenderTargetUpdate(const Ogre::RenderTargetEvent& rte)
{
	recMiniScreen->setVisible(false);
}

void Core::postRenderTargetUpdate(const Ogre::RenderTargetEvent& rte)
{
	recMiniScreen->setVisible(true);
}

void Core::setup()
{
	OgreBites::ApplicationContext::setup();
	addInputListener(this);
	mSceneMgr = mRoot->createSceneManager();

	// register our scene with the RTSS
	Ogre::RTShader::ShaderGenerator* shadergen = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
	shadergen->addSceneManager(mSceneMgr);

	//and the overlay system
	mSceneMgr->addRenderQueueListener(Ogre::OverlaySystem::getSingletonPtr());

	//auto lod
	meshLodGenerator = OGRE_NEW Ogre::MeshLodGenerator();


	initLevel();
	initImGui();

	//lighting
	if (lightType == LightType::AMBIENT)                                             
	{
		mSceneMgr->setAmbientLight(Ogre::ColourValue(1.f, 1.f, 1.f));
	}
	else
	{
		//use if directional light is enabled
		sunlight = mSceneMgr->createLight();
		sunlight->setType(Ogre::Light::LT_DIRECTIONAL);
		sunlight->setDiffuseColour(colorSLDiffuse);
		sunlight->setSpecularColour(Ogre::ColourValue(0.1f, 0.1f, 0.1f));
		sunlight->setPowerScale(fSLPowerScale);
		sunlightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
		sunlightNode->setDirection(vSLDirection);
		sunlightNode->attachObject(sunlight);
	}

}

void Core::initLevel()
{
	mSceneMgr->setSkyBox(true, "spaceSkybox");

	//primary viewport
	//create a camera
	mCamera = mSceneMgr->createCamera("camera");
	mCamera->setNearClipDistance(5);
	mCamera->setAutoAspectRatio(true);
	//mCamera->setFarClipDistance(planet->fSideLength * 10.f);
	Ogre::SceneNode* cameraNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	cameraNode->attachObject(mCamera);
	cameraNode->setPosition(0, 0, 0);
	cameraNode->lookAt(Ogre::Vector3(0, 0, 0), Ogre::Node::TransformSpace::TS_WORLD);
	// and tell it to render into the main window
	vpPrimary = getRenderWindow()->addViewport(mCamera, 0, 0.f, 0.f, 1.f, 1.f);
	vpPrimary->setVisibilityMask(0xFFFFFF00);
	vpPrimary->setBackgroundColour(colorPrimary);
	//the planet
	planet = std::make_unique<Planet>(mSceneMgr, MeshType::NORMAL_BIOMES, "main", 0xF00);
	planet->init();	
	cameraMan = std::make_unique<OgreBites::CameraMan>(cameraNode);
	resetCameraPosition();

	//mini screen for gradient planet
	Ogre::TexturePtr rttTexture =
		Ogre::TextureManager::getSingleton().createManual(
			"RttTex",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			Ogre::TEX_TYPE_2D,
			getRenderWindow()->getWidth(), getRenderWindow()->getHeight(),
			0,
			Ogre::PF_R8G8B8,
			Ogre::TU_RENDERTARGET);
	Ogre::RenderTarget* renderTexture = rttTexture->getBuffer()->getRenderTarget();
	vpMiniScreen = renderTexture->addViewport(mCamera);
	vpMiniScreen->setClearEveryFrame(true);
	vpMiniScreen->setBackgroundColour(colorMiniScreen);
	vpMiniScreen->setOverlaysEnabled(false);
	vpMiniScreen->setSkiesEnabled(false);
	vpMiniScreen->setVisibilityMask(0xFFFF0F0);
	recMiniScreen = OGRE_NEW Ogre::Rectangle2D(true);
	recMiniScreen->setCorners(-1.f, -.5f, -.5f, -1.f);
	recMiniScreen->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
	Ogre::SceneNode* miniScreenNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	miniScreenNode->attachObject(recMiniScreen);

	Ogre::MaterialPtr renderMaterial =
		Ogre::MaterialManager::getSingleton().create(
			"RttMat",
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	renderMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);
	renderMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("RttTex");
	recMiniScreen->setMaterial(renderMaterial);
	renderTexture->addListener(this);

	//gradient planet using the second SM
	planetGradient = std::make_unique<Planet>(mSceneMgr, MeshType::GRADIENT, "sec", 0xF0);
	planetGradient->init();
}

void Core::initImGui()
{
	Ogre::ImGuiOverlay* imguiOverlay = OGRE_NEW Ogre::ImGuiOverlay();
	// handle DPI scaling
	float vpScale = Ogre::OverlayManager::getSingleton().getPixelRatio();
	ImGui::GetIO().FontGlobalScale = std::round(vpScale); // default font does not work with fractional scaling
	ImGui::GetStyle().ScaleAllSizes(vpScale);
	imguiOverlay->setZOrder(300);
	imguiOverlay->show();
	Ogre::OverlayManager::getSingleton().addOverlay(imguiOverlay); // now owned by overlaymgr
	mImguiListener = std::make_unique<OgreBites::ImGuiInputListener>();
	mListenerChain = OgreBites::InputListenerChain({ mImguiListener.get()});

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 5.3f;
	style.FrameRounding = 2.3f;
	style.ScrollbarRounding = 0;

	style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
	//style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.87f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
	//style.Colors[ImGuiCol_ComboBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.99f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.48f, 0.72f, 0.89f, 0.49f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
	//style.Colors[ImGuiCol_Column] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	//style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.70f, 0.60f, 0.60f, 1.00f);
	//style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.90f, 0.70f, 0.70f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
	/*style.Colors[ImGuiCol_CloseButton] = ImVec4(0.50f, 0.50f, 0.90f, 0.50f);
	style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.70f, 0.70f, 0.90f, 0.60f);
	style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);*/
	style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
	//style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);


	//for imgui menu interaction
	imSelection = 0;
	imSections = planet->nSections;
	imDiaMultiplier = planet->iDiaMultiplier;
	fSelection = 0.f;
	fColor[0] = fColor[1] = fColor[2] = fColor4[0] = fColor4[1] = fColor4[2] = fColor4[3] = 0.f;
	bSelected[0] = bSelected[1] = bSelected[2] = bSelected[3] = bSelected[4] = bSelected[5] = bSelected[6] = false;
	lightType = planet->getLightType();
}


void Core::resetCameraPosition()
{
	cameraMan->setStyle(OgreBites::CameraStyle::CS_ORBIT);
	cameraMan->setYawPitchDist(Ogre::Radian(0.f), Ogre::Radian(0.f), planet->fSideLength * 1.65f);
}

void Core::destroy()
{
	//write to dat file, only primary planet is neccesary
	planet->nSections = imSections;
	planet->iDiaMultiplier = imDiaMultiplier;
	planet->setLightType(lightType);
	planet->writeDATFile();

	Ogre::OverlayManager::getSingleton().destroy("ImGuiOverlay");
	//mRenderWindow->removeListener(Ogre::OverlaySystem::getSingletonPtr());

	OGRE_DELETE meshLodGenerator;
	OGRE_DELETE mOverlaySystem;
	OGRE_DELETE recMiniScreen;

	closeApp();
}




