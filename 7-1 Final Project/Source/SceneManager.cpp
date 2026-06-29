///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	DestroyGLTextures();

	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	stbi_set_flip_vertically_on_load(true);

	std::cout << "Trying to load texture: " << filename << std::endl;

	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	if (image)
	{
		std::cout << "Successfully loaded image:" << filename
			<< ", width:" << width
			<< ", height:" << height
			<< ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (colorChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		}
		else if (colorChannels == 4)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			stbi_image_free(image);
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots. There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}

	std::cout << "Total textures bound: " << m_loadedTextures << std::endl;
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}

	m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
		{
			index++;
		}
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
		{
			index++;
		}
	}

	if (textureSlot == -1)
	{
		std::cout << "Texture tag not found: " << tag << std::endl;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;

	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command.
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for loading the texture image files
 *  into OpenGL memory before the 3D scene is rendered.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	std::cout << "LoadSceneTextures started" << std::endl;

	CreateGLTexture("textures/wood.jpg", "wood");
	CreateGLTexture("textures/black_plastic.jpg", "blackPlastic");
	CreateGLTexture("textures/black_leather.jpg", "blackLeather");
	CreateGLTexture("textures/keyboard_keys.jpg", "keyboardKeys");
	CreateGLTexture("textures/monitor_screen.jpg", "monitorScreen");
	CreateGLTexture("textures/dark_metal.jpg", "darkMetal");
	CreateGLTexture("textures/warm_light.jpg", "warmLight");

	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring material settings
 *  for the different objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL material;

	material.diffuseColor = glm::vec3(0.70f, 0.45f, 0.22f);
	material.specularColor = glm::vec3(0.30f, 0.22f, 0.15f);
	material.shininess = 12.0f;
	material.tag = "woodMaterial";
	m_objectMaterials.push_back(material);

	material.diffuseColor = glm::vec3(0.03f, 0.03f, 0.035f);
	material.specularColor = glm::vec3(0.45f, 0.45f, 0.50f);
	material.shininess = 32.0f;
	material.tag = "blackPlasticMaterial";
	m_objectMaterials.push_back(material);

	material.diffuseColor = glm::vec3(0.04f, 0.04f, 0.045f);
	material.specularColor = glm::vec3(0.18f, 0.18f, 0.20f);
	material.shininess = 8.0f;
	material.tag = "blackRubberMaterial";
	m_objectMaterials.push_back(material);

	material.diffuseColor = glm::vec3(0.10f, 0.10f, 0.11f);
	material.specularColor = glm::vec3(0.35f, 0.35f, 0.38f);
	material.shininess = 18.0f;
	material.tag = "darkMetalMaterial";
	m_objectMaterials.push_back(material);

	material.diffuseColor = glm::vec3(0.15f, 0.35f, 0.60f);
	material.specularColor = glm::vec3(0.55f, 0.65f, 0.75f);
	material.shininess = 48.0f;
	material.tag = "screenMaterial";
	m_objectMaterials.push_back(material);

	material.diffuseColor = glm::vec3(1.00f, 0.72f, 0.35f);
	material.specularColor = glm::vec3(0.80f, 0.70f, 0.45f);
	material.shininess = 40.0f;
	material.tag = "lampLightMaterial";
	m_objectMaterials.push_back(material);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	m_pShaderManager->setBoolValue("directionalLight.bActive", false);
	m_pShaderManager->setBoolValue("spotLight.bActive", false);

	m_pShaderManager->setVec3Value("viewPosition", 0.0f, 5.0f, 15.0f);

	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[0].position", 3.0f, 4.5f, -0.8f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.18f, 0.13f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.95f, 0.72f, 0.38f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.90f, 0.75f, 0.45f);

	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[1].position", -4.0f, 5.0f, 2.5f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.05f, 0.06f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.25f, 0.32f, 0.48f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.35f, 0.40f, 0.55f);

	m_pShaderManager->setBoolValue("pointLights[2].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[4].bActive", false);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering.
 ***********************************************************/
void SceneManager::PrepareScene()
{
	std::cout << "PrepareScene started" << std::endl;

	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes.
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/******************************************************************/
	// desk / table surface - tiled wood texture
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("wood");
	SetTextureUVScale(0.75f, 0.75f);
	SetShaderMaterial("woodMaterial");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// monitor frame
	scaleXYZ = glm::vec3(3.8f, 2.2f, 0.18f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 1.90f, -2.4f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	SetShaderMaterial("blackPlasticMaterial");
	m_basicMeshes->DrawBoxMesh();

	// monitor screen
	scaleXYZ = glm::vec3(3.25f, 1.72f, 0.08f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 1.94f, -2.24f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("monitorScreen");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("screenMaterial");
	m_basicMeshes->DrawBoxMesh();

	// monitor stand
	scaleXYZ = glm::vec3(0.28f, 0.85f, 0.28f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 0.85f, -2.35f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.03f, 0.03f, 0.03f, 1.0f);
	SetShaderMaterial("blackPlasticMaterial");
	m_basicMeshes->DrawBoxMesh();

	// monitor base
	scaleXYZ = glm::vec3(1.3f, 0.12f, 0.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.0f, 0.46f, -2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.03f, 0.03f, 0.03f, 1.0f);
	SetShaderMaterial("blackPlasticMaterial");
	m_basicMeshes->DrawBoxMesh();

	// keyboard base
	scaleXYZ = glm::vec3(4.9f, 0.18f, 1.15f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.0f, 0.20f, 1.35f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blackPlastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("blackPlasticMaterial");
	m_basicMeshes->DrawBoxMesh();

	// keyboard keys
	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col < 12; col++)
		{
			scaleXYZ = glm::vec3(0.25f, 0.07f, 0.16f);
			XrotationDegrees = 0.0f;
			YrotationDegrees = 0.0f;
			ZrotationDegrees = 0.0f;
			positionXYZ = glm::vec3(-3.95f + (col * 0.35f), 0.34f, 1.10f + (row * 0.25f));

			SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
			SetShaderTexture("keyboardKeys");
			SetTextureUVScale(1.0f, 1.0f);
			SetShaderMaterial("blackPlasticMaterial");
			m_basicMeshes->DrawBoxMesh();
		}
	}

	// mouse
	scaleXYZ = glm::vec3(0.48f, 0.12f, 0.68f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(1.55f, 0.19f, 1.50f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blackPlastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("blackPlasticMaterial");
	m_basicMeshes->DrawSphereMesh();

	// lamp base
	scaleXYZ = glm::vec3(0.70f, 0.08f, 0.70f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.25f, 0.10f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("darkMetal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkMetalMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// lamp lower arm
	scaleXYZ = glm::vec3(0.11f, 1.25f, 0.11f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.25f, 0.72f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("darkMetal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkMetalMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// lamp joint
	scaleXYZ = glm::vec3(0.18f, 0.18f, 0.18f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(4.25f, 1.38f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("darkMetal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkMetalMaterial");
	m_basicMeshes->DrawSphereMesh();

	// lamp upper arm
	scaleXYZ = glm::vec3(0.11f, 1.05f, 0.11f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -55.0f;
	positionXYZ = glm::vec3(3.78f, 1.70f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("darkMetal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkMetalMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// lamp shade
	scaleXYZ = glm::vec3(0.65f, 0.52f, 0.65f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;
	positionXYZ = glm::vec3(3.25f, 1.92f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("darkMetal");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("darkMetalMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// lamp light
	scaleXYZ = glm::vec3(0.45f, 0.06f, 0.45f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -45.0f;
	positionXYZ = glm::vec3(3.05f, 1.75f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("warmLight");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("lampLightMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// lamp cable
	scaleXYZ = glm::vec3(0.03f, 0.03f, 1.25f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -65.0f;
	positionXYZ = glm::vec3(4.80f, 0.11f, -0.65f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	SetShaderMaterial("blackRubberMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// headphones band
	scaleXYZ = glm::vec3(0.95f, 0.05f, 0.05f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.45f, 0.34f, 0.18f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blackLeather");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("blackRubberMaterial");
	m_basicMeshes->DrawBoxMesh();

	// headphones back band
	scaleXYZ = glm::vec3(0.95f, 0.05f, 0.05f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.45f, 0.34f, -0.20f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blackLeather");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("blackRubberMaterial");
	m_basicMeshes->DrawBoxMesh();

	// left headphone cup
	scaleXYZ = glm::vec3(0.30f, 0.16f, 0.30f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-5.95f, 0.24f, 0.00f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blackLeather");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("blackRubberMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// right headphone cup
	scaleXYZ = glm::vec3(0.30f, 0.16f, 0.30f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.95f, 0.24f, 0.00f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("blackLeather");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("blackRubberMaterial");
	m_basicMeshes->DrawCylinderMesh();

	// headphone cable
	scaleXYZ = glm::vec3(0.03f, 0.03f, 0.95f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -35.0f;
	positionXYZ = glm::vec3(-6.25f, 0.14f, 0.30f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	SetShaderMaterial("blackRubberMaterial");
	m_basicMeshes->DrawCylinderMesh();
}