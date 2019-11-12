/*
#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <sstream>

#include <map>
#include <functional>
*/

#include "NodeFunctions.h"
#include "comLib.h"
#pragma comment(lib, "Project1.lib")
ComLib comLib("shaderMemory", 50, CONSUMER);

#include <Windows.h>

// ===========================================================
// ================ Structs and ENUMs ========================
// ===========================================================
/* 
enum NODE_TYPE {
	TRANSFORM,
	MESH,
	LIGHT,
	CAMERA,
	MATERIAL
};

enum CMDTYPE {
	DEFAULT = 1000,

	NEW_NODE = 1001,
	UPDATE_NODE = 1002,

	UPDATE_MATRIX = 1003,
	UPDATE_NAME = 1004,

	NEW_MATERIAL = 1005,
	UPDATE_MATERIAL = 1006,

	DELETE_NODE = 1007,
	TRANSFORM_UPDATE = 1008
};

struct MsgHeader {
	CMDTYPE   cmdType;
	NODE_TYPE nodeType;
	char objName[64];
	int msgSize;
	int nameLen;
};

struct msgMesh {
	int vtxCount;
	int trisCount;
	int normalCount;
	int UVcount;
};

// Structs for RayLib ===============
struct cameraFromMaya {
	Vector3 up;
	Vector3 forward;
	Vector3 pos;
	int type;
	float fov;
};

struct lightFromMaya {
	Vector3 lightPos;
	float radius;
	Color color;
};

struct modelFromMaya {
	Model model;
	int index;
	std::string name;
	Matrix modelMatrix;
	Color color;
	std::string materialName;
	Mesh mesh; 
};

struct materialMaya {
	int type;					//color = 0, texture = 1 
	char materialName[64];
	char fileTextureName[64];
	int matNameLen;
	int textureNameLen;
	Color color;
};

struct transformFromMaya {
	char childName[64];
	int childNameLen;
};

struct modelPos {
	Model model;
	Matrix modelMatrix;
};

*/
// ===========================================================
// =============== FUNCTION DECLARATIONS =====================
// ===========================================================
/* 

void addNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void updateNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void deleteNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);

void updateNodeName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void updateNodeMatrix(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);

void updateMaterial(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void updateMaterialName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);


typedef void(*FnPtr)(std::vector<modelFromMaya>&, std::vector<lightFromMaya>&, std::vector<cameraFromMaya>&, std::vector<materialMaya>&, char*, int, Shader, int*, int*,  int*);

*/
void recvFromMaya(char* buffer, std::map<CMDTYPE, FnPtr> functionMap, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials, std::vector<Texture2D> &textureArr);

// ==================================================================================
// ==================================================================================
// ================================= MAIN ===========================================
// ==================================================================================
// ==================================================================================

int main() {
	SetTraceLog(LOG_WARNING);

	// vectors with objects from Maya ====== 
	std::vector<Texture2D> textureArray;
	std::vector<modelFromMaya> modelArray;
	std::vector<lightFromMaya> lightsArray;
	std::vector<cameraFromMaya> cameraArray;
	std::vector<materialMaya> materialArray;


	// create a light ====== 
	lightFromMaya tempLight = {};
	tempLight.lightNameLen = 5; 
	tempLight.lightPos = { 0,0,0 };
	tempLight.intensityRadius = 0.1;
	tempLight.color = { 1 * 255, 1 * 255, 1 * 255, 1 * 255 };
	memcpy(tempLight.lightName, "light", tempLight.lightNameLen);
	lightsArray.push_back(tempLight);

	// map specific commands to a specific function
	
	std::map<CMDTYPE, FnPtr> funcMap;

	funcMap[NEW_NODE]			  = addNode; 
	funcMap[UPDATE_NODE]		  = updateNode;
	funcMap[UPDATE_MATRIX]		  = updateNodeMatrix;
	funcMap[UPDATE_NAME]		  = updateNodeName;
	funcMap[NEW_MATERIAL]	      = newMaterial;
	funcMap[UPDATE_MATERIAL]      = updateMaterial;
	funcMap[DELETE_NODE]		  = deleteNode;
	funcMap[UPDATE_NODE_MATERIAL] = updateNodeMaterial;

	// create a simple cube
	//Model cube;
	int modelIndex  = 0;
	int nrOfObj		= 0;
	int nrMaterials = 0;
	size_t tempArraySize = 0;
	
	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth = 1200;
	int screenHeight = 800;

	SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	//SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "demo");

	// Define the camera to look into our 3d world
	Camera camera	= { 0 };
	/* 
	camera.position = { 4.0f, 4.0f, 4.0f };
	camera.target	= { 0.0f, 1.0f, -1.0f };
	camera.up		= { 0.0f, 1.0f, 0.0f };
	camera.fovy		= 45.0f;
	camera.type		= CAMERA_PERSPECTIVE;
	*/

	cameraFromMaya tempCamera = {}; 
	tempCamera.fov  = 45.0f;
	tempCamera.type = CAMERA_PERSPECTIVE;
	tempCamera.up   = { 0.0f, 1.0f, 0.0f };
	tempCamera.pos  = { 4.0f, 4.0f, 4.0f }; 
	tempCamera.forward = { 0.0f, 1.0f, -1.0f };
	cameraArray.push_back(tempCamera);

	// real models rendered each frame
	std::vector<modelPos> flatScene;

	Material material1 = LoadMaterialDefault();
	Texture2D texture1 = LoadTexture("resources/models/watermill_diffuse.png");
	material1.maps[MAP_DIFFUSE].texture = texture1;

	Shader shader1 = LoadShader("resources/shaders/glsl330/phong.vs",
		"resources/shaders/glsl330/phong.fs");   // Load model shader
	material1.shader = shader1;

	/* 
	Mesh mesh1   = LoadMesh("resources/models/watermill.obj");
	Model model1 = LoadModelFromMesh(mesh1);
	model1.material = material1;                     // Set shader effect to 3d model

	unsigned char colors[12] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };
	*/

	std::cout << "-----------------" << std::endl;

	Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
	SetCameraMode(camera, CAMERA_FREE);         // Set an orbital camera mode
	SetTargetFPS(130);                           // Set our game to run at 60 frames-per-second
	
	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc  = GetShaderLocation(shader1, "view");
	int projLoc  = GetShaderLocation(shader1, "projection");
	int lightLoc = GetShaderLocation(shader1, "lightPos");


	char* tempArray; 
	// =====================================

		// Main game loop
	while (!WindowShouldClose())                // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		//UpdateCamera(&camera);                  // Update camera
		//----------------------------------------------------------------------------------
		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		// Get the messages sent from maya
		//tempArraySize = comLib.nextSize();
		tempArray	  = new char[1];
		recvFromMaya(tempArray, funcMap, modelArray, lightsArray, cameraArray, materialArray, tempArraySize, shader1, &nrOfObj, &modelIndex, &nrMaterials, textureArray);
		delete[] tempArray; 

		// set camera
		if (cameraArray.size() > 0) {

			camera.up   = cameraArray[0].up;
			camera.fovy = cameraArray[0].fov;
			camera.type = cameraArray[0].type;
			camera.position = cameraArray[0].pos;
			camera.target	= cameraArray[0].forward;
		}
		
		// draw lights from maya
		for (int i = 0; i < lightsArray.size(); i++) {
			DrawSphere(tempLight.lightPos, 0.1, tempLight.color);
		}

		SetShaderValue(shader1, lightLoc, Vector3ToFloat(tempLight.lightPos), 1);

		ClearBackground(RAYWHITE);
		/* 
		if (IsKeyPressed(KEY_D))
		{
			Vector3 pos = { (rand() / (float)RAND_MAX) * 10.0f, 0.0f, (rand() / (float)RAND_MAX) * 10.0f };

			static int i = 0;

			// matrices are transposed later, so order here would be [Translate*Scale*vector]
			Matrix modelmatrix = MatrixMultiply(MatrixMultiply(
				MatrixRotate({ 0,1,0 }, i / (float)5),
				MatrixScale(0.1, 0.1, 0.1)),
				MatrixTranslate(pos.x, pos.y, pos.z));

			Model copy = cube;
			flatScene.push_back({ copy, modelmatrix });

			i++;
		}

		*/

		BeginMode3D(camera);

		
		// draw models from maya
		for (int i = 0; i < modelArray.size(); i++) {
			
			auto m = modelArray[i];
			auto l = GetShaderLocation(m.model.material.shader, "model");
			SetShaderValueMatrix(m.model.material.shader, modelLoc, m.modelMatrix);

			Color finalColor = { (lightsArray[0].color.r + m.color.r),(lightsArray[0].color.g + m.color.g),(lightsArray[0].color.b + m.color.b),(lightsArray[0].color.a + m.color.a) };
			DrawModel(m.model, {}, 1.0, finalColor);

		}
		
		DrawGrid(10, 1.0f);     // Draw a grid

		EndMode3D();

		DrawTextRL("Maya API level editor", screenWidth - 210, screenHeight - 20, 10, GRAY);
		DrawTextRL(FormatText("Camera position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z), 600, 20, 10, BLACK);
		DrawTextRL(FormatText("Camera target: (%.2f, %.2f, %.2f)", camera.target.x, camera.target.y, camera.target.z), 600, 40, 10, GRAY);
		DrawFPS(10, 10);
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	UnloadShader(shader1);       // Unload shader
	UnloadTexture(texture1);     // Unload texture
	
	for (int i = 0; i < textureArray.size(); i++) {
		UnloadTexture(textureArray[i]);
	}
	
	for (int i = 0; i < modelArray.size(); i++) {
		UnloadModel(modelArray[i].model);
	}
	
	/* 
	UnloadModel(model1);
	UnloadModel(tri);
	UnloadModel(cubeMod);
	UnloadModel(cube);
	*/

	CloseWindowRL();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
	return 0;
}


// ==================================================================================
// ================= FUNCTION TO REVIECE MSG FROM MAYA ==============================
// ==================================================================================
void recvFromMaya(char* buffer, std::map<CMDTYPE, FnPtr> functionMap, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials, std::vector<Texture2D> &textureArr) {

	MsgHeader msgHeader = {}; 
	msgHeader.cmdType   = CMDTYPE::DEFAULT;

	size_t nr = comLib.nextSize();
	size_t oldBuffSize = nr;

	buffer = new char[oldBuffSize];
	if (nr > oldBuffSize) {
		delete[] buffer;
		buffer = new char[nr];
		oldBuffSize = nr;
	}

	// get message from Maya ======
	if (comLib.recv(buffer, nr)) {
		memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

		//std::cout << "NODE TYPE " << msgHeader.nodeType << std::endl; 
		functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials, textureArr);

		/* 
		if (msgHeader.nodeType == NODE_TYPE::MESH) {
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials, textureArr);
		}
	
		if (msgHeader.nodeType == NODE_TYPE::CAMERA) {
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials, textureArr);
		}
		
		if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials, textureArr);
		}

		if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {
		
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials, textureArr);
		}
		*/
		

	}

	delete[] buffer; 
		
}