#pragma once
#include <OgreRoot.h>
#include <OgreSceneNode.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreOverlaySystem.h>
#include <OgreApplicationContext.h>
#include <OgreCameraMan.h>
#include <OgreImGuiOverlay.h>
#include <OgreImGuiInputListener.h>
#include <OgreMeshLodGenerator.h>
#include <memory>

#include "Planet.h"


class Core : public OgreBites::ApplicationContext, public Ogre::FrameListener, public OgreBites::InputListener, public Ogre::RenderTargetListener
{
	Ogre::SceneManager* mSceneMgr;						
	Ogre::Viewport* vpPrimary, *vpMiniScreen;			
	Ogre::ColourValue colorPrimary, colorMiniScreen;								//bkg colors for both viewports
	
	Ogre::MeshLodGenerator* meshLodGenerator;

	bool bWireFrame, bToggleFreelook, bToggleSkybox;
	Ogre::Camera* mCamera;
	std::unique_ptr<OgreBites::CameraMan> cameraMan;
	
	//mini screen
	float fWindowSize;									//size multiplier wrt main window size;
	Ogre::Rectangle2D* recMiniScreen;

	//std::unique_ptr<ImguiListener> mImguiListener;
	std::unique_ptr<OgreBites::ImGuiInputListener> mImguiListener;
	OgreBites::InputListenerChain mListenerChain;

	//lighting 
	float fSLPowerScale;
	LightType lightType;											//0 = ambient, 1 = sun light, used to store value from imgui
	Ogre::Light* sunlight;
	Ogre::SceneNode* sunlightNode;
	Ogre::ColourValue colorAmbient;
	Ogre::ColourValue colorSLDiffuse;
	Ogre::Vector3 vSLDirection;

	//imgui menu interaction
	int imSelection, imSections, imDiaMultiplier;
	float fSelection, fColor[3], fColor4[4];
	bool bSelected[7];

	//planets
	std::unique_ptr<Planet> planet;
	std::unique_ptr<Planet> planetGradient;

public:
	Core();	
	void setup();
	bool frameStarted(const Ogre::FrameEvent& evt);
	bool frameRenderingQueued(const Ogre::FrameEvent& evt);
	bool frameEnded(const Ogre::FrameEvent& evt);
	bool keyPressed(const OgreBites::KeyboardEvent& evt);
	bool keyReleased(const OgreBites::KeyboardEvent& evt);
	bool mouseMoved(const OgreBites::MouseMotionEvent& evt);
	bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt);
	bool mousePressed(const OgreBites::MouseButtonEvent& evt);
	bool mouseReleased(const OgreBites::MouseButtonEvent& evt);
	bool textInput(const OgreBites::TextInputEvent& evt);
	//stop the viewport from having another viewport
	virtual void preRenderTargetUpdate(const Ogre::RenderTargetEvent& rte) override;
	virtual void postRenderTargetUpdate(const Ogre::RenderTargetEvent& rte) override;

	void destroy();

private:
	void initLevel();
	void initImGui();
	void resetCameraPosition();

};

