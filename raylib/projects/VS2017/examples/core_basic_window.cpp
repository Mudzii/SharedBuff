#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <sstream>
#include "comLib.h"

#include <map>
#include <functional>
#pragma comment(lib, "Project1.lib")

#include <Windows.h>
ComLib comLib("shaderMemory", 50, CONSUMER);

// ===========================================================
// ================ Structs and ENUMs ========================
// ===========================================================

enum NODE_TYPE {
	TRANSFORM,
	MESH,
	LIGHT,
	CAMERA
};

enum CMDTYPE {
	DEFAULT				= 1000,

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


// ===========================================================
// =============== FUNCTION DECLARATIONS =====================
// ===========================================================

void addNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void updateNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void deleteNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);

void updateNodeName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void updateNodeMatrix(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);

void updateMaterial(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);
void updateMaterialName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);


typedef void(*FnPtr)(std::vector<modelFromMaya>&, std::vector<lightFromMaya>&, std::vector<cameraFromMaya>&, std::vector<materialMaya>&, char*, int, Shader, int*, int*,  int*);

void recvFromMaya(char* buffer, std::map<CMDTYPE, FnPtr> functionMap, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials);

// ==================================================================================
// ==================================================================================
// ================================= MAIN ===========================================
// ==================================================================================
// ==================================================================================

int main() {
	SetTraceLog(LOG_WARNING);

	// vectors with objects from Maya ====== 
	std::vector<materialMaya> materialArray;
	std::vector<cameraFromMaya> cameraArray;
	std::vector<modelFromMaya> modelArray;
	std::vector<lightFromMaya> lightsArray;


	// create a light ====== 
	Vector3 lightPos = { 0,0,0 };
	float radius = 0.1;
	Color color = { 1 * 255, 1 * 255, 1 * 255, 1 * 255 };
	lightsArray.push_back({ lightPos, radius, color });

	// map specific commands to a specific function
	std::map<CMDTYPE, FnPtr> funcMap;

	funcMap[NEW_NODE]			 = addNode; 
	funcMap[UPDATE_NODE]		 = updateNode;
	funcMap[UPDATE_MATRIX]		 = updateNodeMatrix;
	funcMap[UPDATE_NAME]		 = updateNodeName;
	funcMap[NEW_MATERIAL]	     = updateMaterial;
	funcMap[UPDATE_MATERIAL]     = updateMaterialName;
	funcMap[DELETE_NODE]		 = deleteNode;


	// create a simple cube
	Model cube;
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
	camera.position = { 4.0f, 4.0f, 4.0f };
	camera.target	= { 0.0f, 1.0f, -1.0f };
	camera.up		= { 0.0f, 1.0f, 0.0f };
	camera.fovy		= 45.0f;
	camera.type		= CAMERA_PERSPECTIVE;

	cameraArray.push_back({ camera.up, camera.target, camera.position });

	// real models rendered each frame
	std::vector<modelPos> flatScene;

	Material material1 = LoadMaterialDefault();
	Texture2D texture1 = LoadTexture("resources/models/watermill_diffuse.png");
	material1.maps[MAP_DIFFUSE].texture = texture1;

	Shader shader1 = LoadShader("resources/shaders/glsl330/phong.vs",
		"resources/shaders/glsl330/phong.fs");   // Load model shader
	material1.shader = shader1;

	Mesh mesh1   = LoadMesh("resources/models/watermill.obj");
	Model model1 = LoadModelFromMesh(mesh1);
	model1.material = material1;                     // Set shader effect to 3d model

	unsigned char colors[12] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };

	std::cout << "-----------------" << std::endl;

	Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
	SetCameraMode(camera, CAMERA_FREE);         // Set an orbital camera mode
	SetTargetFPS(130);                           // Set our game to run at 60 frames-per-second
	//initializing camera!!!!

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

		// Get what CMDTYPE the message sent from maya is 
		tempArraySize = comLib.nextSize();
		tempArray	  = new char[tempArraySize];

		recvFromMaya(tempArray, funcMap, modelArray, lightsArray, cameraArray, materialArray, tempArraySize, shader1, &nrOfObj, &modelIndex, &nrMaterials);
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
			DrawSphere(lightPos, 0.1, color);
		}

		SetShaderValue(shader1, lightLoc, Vector3ToFloat(lightPos), 1);

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

		DrawTextRL("(c) Watermill 3D model by Alberto Cano", screenWidth - 210, screenHeight - 20, 10, GRAY);
		DrawTextRL(FormatText("Camera position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z), 600, 20, 10, BLACK);
		DrawTextRL(FormatText("Camera target: (%.2f, %.2f, %.2f)", camera.target.x, camera.target.y, camera.target.z), 600, 40, 10, GRAY);
		DrawFPS(10, 10);
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	UnloadShader(shader1);       // Unload shader
	UnloadTexture(texture1);     // Unload texture
	UnloadModel(model1);
	
	for (int i = 0; i < modelArray.size(); i++) {
		UnloadModel(modelArray[i].model);
	}
	
	//UnloadModel(tri);
	//UnloadModel(cubeMod);
	//UnloadModel(cube);

	CloseWindowRL();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
	return 0;
}


// ==================================================================================
// ==================================================================================
// ================================= FUNCTIONS ======================================
// ==================================================================================
// ==================================================================================



// ==================================================================================
// ================= FUNCTION TO REVIECE MSG FROM MAYA ==============================
// ==================================================================================

void recvFromMaya(char* buffer, std::map<CMDTYPE, FnPtr> functionMap, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

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

		if (msgHeader.nodeType == NODE_TYPE::TRANSFORM) {
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials);
		}

		if (msgHeader.nodeType == NODE_TYPE::MESH) {

			//std::cout << "RCV MESH" << std::endl;
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials);

		}

		if (msgHeader.nodeType == NODE_TYPE::CAMERA) {
			//std::cout << "RCV CAM" << std::endl;
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials);

		}

		if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
			functionMap[msgHeader.cmdType](modelArray, lightsArray, cameraArray, materialArray, buffer, bufferSize, shader, nrObjs, index, nrMaterials);
		}
		

	}

	delete[] buffer; 
	
}

// ==================================================================================
// ================================= OTHER ==========================================
// ==================================================================================


//add node adds a new node to the rayLib
void addNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "ADD NODE FUNCTION" << std::endl; 

	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader)); 

	std::string objectName = msgHeader.objName; 
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// get mesh info send over 
		msgMesh meshInfo = {};
		memcpy((char*)&meshInfo, buffer + sizeof(MsgHeader), sizeof(msgMesh));

		// create arrays to store information 
		int vtxCount	   = meshInfo.trisCount * 3;
		float* meshVtx	   = new float[vtxCount * 3];
		float* meshNormals = new float[vtxCount * 3];
		float* meshUVs	   = new float[vtxCount * 2];

		memcpy((char*)meshVtx,	   buffer + sizeof(MsgHeader) + sizeof(msgMesh), (sizeof(float) * vtxCount * 3));
		memcpy((char*)meshNormals, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * vtxCount * 3), (sizeof(float) * vtxCount * 3));
		memcpy((char*)meshUVs,	   buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * vtxCount * 3) + (sizeof(float) * vtxCount * 3), (sizeof(float) * vtxCount * 2));
		
		materialMaya tempMat = {}; 
		memcpy((char*)&tempMat, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * vtxCount * 3) + (sizeof(float) * vtxCount * 3) + (sizeof(float) * vtxCount * 2), sizeof(materialMaya));
		Color tempColor = { tempMat.color.r, tempMat.color.g, tempMat.color.b, tempMat.color.a};

		std::string materialName = tempMat.materialName;
		materialName = materialName.substr(0, tempMat.matNameLen);

		std::string texturePath = tempMat.fileTextureName;
		texturePath = texturePath.substr(0, tempMat.textureNameLen);


		std::cout << "textureName: " << texturePath << std::endl;
		std::cout << "materialName: " << materialName << std::endl;
		std::cout << "tempColor: " << (int)tempColor.r << " : " << (int)tempColor.g << " : " << (int)tempColor.b << " : " << (int)tempColor.a << " : " << std::endl;


		// get mesh material 
		Texture2D texture; 
		Color newColor; 
		materialMaya newMaterial = {}; 
		newMaterial.type = tempMat.type; 
		memcpy(newMaterial.materialName, materialName.c_str(), tempMat.matNameLen);
		memcpy(newMaterial.fileTextureName, texturePath.c_str(), tempMat.textureNameLen);


		// Check if material already exists
		bool materialExists = false; 
		for (int i = 0; i < *nrMaterials; i++) {
			if (materialArray[i].materialName == materialName) {

				// if it exists, remove it and replace it
				materialExists = true;
				materialArray.erase(materialArray.begin() + i);

				if (newMaterial.type == 0) {
					newMaterial.color = tempColor;
				
					materialArray.insert(materialArray.begin() + i, { newMaterial });
				}
				
				else if (newMaterial.type == 1) {
					newMaterial.color = { 255, 255, 255, 255 };
					materialArray.insert(materialArray.begin() + i, { newMaterial });

				}
				

				
			}
		}

		//if material doesn't exist, add it to array
		if (materialExists == false) {

			if (newMaterial.type == 0) {
				newMaterial.color = tempColor;
				materialArray.push_back({ newMaterial });
				*nrMaterials = *nrMaterials + 1;
			}

			else if (newMaterial.type == 1) {
				newMaterial.color = { 255, 255, 255, 255 };
				materialArray.push_back({ newMaterial });
				*nrMaterials = *nrMaterials + 1;
			}
		}


		// check of mesh already exists
		bool meshExists = false; 
		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].name == objectName) {
				meshExists = true;
			}
		}
		
		if (meshExists == false) {

			Mesh tempMesh = {};
			
			// vtx
			tempMesh.vertexCount = vtxCount; 
			tempMesh.vertices	 = new float[vtxCount * 3];
			memcpy(tempMesh.vertices, meshVtx, sizeof(float) * tempMesh.vertexCount * 3);

			// normals
			tempMesh.normals = new float[vtxCount * 3];
			memcpy(tempMesh.normals, meshNormals, sizeof(float) * tempMesh.vertexCount * 3);

			// UVs
			tempMesh.texcoords = new float[vtxCount * 2];
			memcpy(tempMesh.texcoords, meshUVs, sizeof(float) * tempMesh.vertexCount * 2);

			
			rlLoadMesh(&tempMesh, false);
			Model tempModel = LoadModelFromMesh(tempMesh);
			tempModel.material.shader = shader;

			Texture2D texture;
			Color newColor = { tempMat.color.r, tempMat.color.g, tempMat.color.b, 255 }; 

			if (tempMat.type == 0) {
				modelArray.push_back({ tempModel, *index, objectName, MatrixTranslate(0,0,0), newColor, materialName, tempMesh });

			}

			else if (tempMat.type == 1) {
				newColor = { 255, 255, 255, 255 };
				texture  = LoadTexture(texturePath.c_str());
				tempModel.material.maps[MAP_DIFFUSE].texture = texture;
				modelArray.push_back({ tempModel, *index, objectName, MatrixTranslate(0,0,0), newColor, materialName, tempMesh });
			}


			*index  = *index + 1;
			*nrObjs = *nrObjs + 1;

		}
	


		// delete the arrays
		delete[] meshVtx;
		delete[] meshNormals;
		delete[] meshUVs;
	}

	if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
		std::cout << "Light node to add" << std::endl;

		lightFromMaya lightInfo = {}; 
		memcpy((char*)&lightInfo, buffer + sizeof(MsgHeader), sizeof(lightFromMaya));

		std::cout << "LIGHT POS: " << lightInfo.lightPos.x << " : " << lightInfo.lightPos.y << " : " << lightInfo.lightPos.z << std::endl;
		std::cout << "LIGHT INS: " << lightInfo.radius << std::endl;

		lightsArray.erase(lightsArray.begin());
		lightsArray.insert(lightsArray.begin(), { lightInfo });
	}

}

void deleteNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "DELETE NODE FUNCTION" << std::endl;

	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].name == objectName) {

				int tempIndex = modelArray[i].index;
				modelArray.erase(modelArray.begin() + i);
				*nrObjs = *nrObjs - 1;
				*index = *index - 1;
			}
		}

	}

}

//update node "updates" an already existing node by removing it and replacing it with a new one
void updateNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {


	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {
		std::cout << "=======================================" << std::endl;
		std::cout << "UPDATE NODE FUNCTION" << std::endl;

		msgMesh meshInfo = {};
		memcpy((char*)&meshInfo, buffer + sizeof(MsgHeader), sizeof(msgMesh));

		int vtxCount = meshInfo.trisCount * 3;
		int uvCount = meshInfo.UVcount; 

		float* meshVtx	   = new float[vtxCount * 3];
		float* meshNormals = new float[vtxCount * 3];
		float* meshUVs	   = new float[vtxCount * 2];

		memcpy((char*)meshVtx, buffer + sizeof(MsgHeader) + sizeof(msgMesh), (sizeof(float) * vtxCount * 3));
		memcpy((char*)meshNormals, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * vtxCount * 3), (sizeof(float) * vtxCount * 3));
		memcpy((char*)meshUVs, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * vtxCount * 3) + (sizeof(float) * vtxCount * 3), (sizeof(float) * vtxCount * 2));


		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].name == objectName) {
				
				Mesh tempMesh = {};
				int tempIndex = modelArray[i].index;

				// vtx
				tempMesh.vertexCount = vtxCount;
				tempMesh.vertices = new float[vtxCount * 3];
				memcpy(tempMesh.vertices, meshVtx, sizeof(float) * tempMesh.vertexCount * 3);

				// normals
				tempMesh.normals = new float[vtxCount * 3];
				memcpy(tempMesh.normals, meshNormals, sizeof(float) * tempMesh.vertexCount * 3);

				// UVs
				tempMesh.texcoords = new float[vtxCount * 2];
				memcpy(tempMesh.texcoords, meshUVs, sizeof(float) * tempMesh.vertexCount * 2);

				rlLoadMesh(&tempMesh, false);
				Model tempModel = LoadModelFromMesh(tempMesh);
				tempModel.material.shader = shader;
				
				Texture2D tempTexture; 
				Color tempColor = modelArray[i].color;
				std::string tempMaterialName = modelArray[i].materialName;		
				Matrix tempMatrix = modelArray[i].modelMatrix;

				// erase old model and replace with new
				modelArray.erase(modelArray.begin() + i);

				for (int i = 0; i < materialArray.size(); i++) {
					if (materialArray[i].materialName == tempMaterialName) {

						if (materialArray[i].type == 0) {

							std::cout << "Updating color" << std::endl;
							tempColor = materialArray[i].color;
						}

						else if (materialArray[i].type == 1) {

							std::cout << "Updating material" << std::endl;

							tempColor = {255, 255, 255, 255};
							tempTexture = LoadTexture(materialArray[i].fileTextureName); 
							tempModel.material.maps[MAP_DIFFUSE].texture = tempTexture;

						}

					}

					else {
						std::cout << "MAT DOES NOT EXIST" << std::endl;

					}
				}


				// replace with new			
				modelArray.insert(modelArray.begin() + i, {tempModel, tempIndex, objectName, tempMatrix, tempColor, tempMaterialName, tempMesh });

			}

		}


		// delete the arrays
		delete[] meshVtx;
		delete[] meshNormals;
		delete[] meshUVs;
	}

	if (msgHeader.nodeType == NODE_TYPE::TRANSFORM) {

		transformFromMaya transfInfo = {};
		Matrix transfMatrix = {}; 

		memcpy((char*)&transfInfo, buffer + sizeof(MsgHeader), sizeof(transformFromMaya));
		memcpy((char*)&transfMatrix, buffer + sizeof(MsgHeader) + sizeof(transformFromMaya), sizeof(Matrix));

		std::string childName = transfInfo.childName;
		childName = childName.substr(0, transfInfo.childNameLen);

		//std::cout << "transfInform child: " << childName << std::endl;

	}

	if (msgHeader.nodeType == NODE_TYPE::CAMERA) {
		
		cameraFromMaya tempCam = {}; 
		memcpy((char*)&tempCam, buffer + sizeof(MsgHeader), sizeof(cameraFromMaya));

		// if perspective camera, change FOV from radians to degrees
		if (tempCam.type == 0) {
			float degreesFOV = tempCam.fov * (180.0 / 3.141592653589793238463);
			tempCam.fov = degreesFOV;
		}

		/* 
		else if(tempCam.type == 1) {
			std::cout << "Ortographic cam" << std::endl;

		}
		*/

		//erase the old camera and set the new one
		cameraArray.erase(cameraArray.begin());
		cameraArray.insert(cameraArray.begin(), { tempCam.up, tempCam.forward, tempCam.pos, tempCam.type, tempCam.fov });

	}

	if (msgHeader.nodeType == NODE_TYPE::LIGHT) {

		std::cout << "UPDATE LIGHT!" << std::endl; 
		lightFromMaya tempLight = {};
		memcpy((char*)&tempLight, buffer + sizeof(MsgHeader), sizeof(lightFromMaya));

		//erase the old camera and set the new one
		lightsArray.erase(lightsArray.begin());
		lightsArray.insert(lightsArray.begin(), {tempLight});


	}

}

// update the name of a node
void updateNodeName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE NODE NAME FUNCTION" << std::endl;


	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		int newNameLen = 0;
		memcpy((char*)&newNameLen, buffer + sizeof(MsgHeader), sizeof(int));

		char* newName = new char[newNameLen];
		memcpy((char*)newName, buffer + sizeof(MsgHeader) + sizeof(int), (sizeof(char) * newNameLen));

		std::string newObjName = newName;
		newObjName = newObjName.substr(0, newNameLen);

		// find the mesh and "replace" the name 
		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].name == objectName) {
				
				int tempIndex = modelArray[i].index;
				Model tempModel = modelArray[i].model;
				Matrix tempMatrix = modelArray[i].modelMatrix;
				Color tempColor = modelArray[i].color;
				std::string tempMaterialName = modelArray[i].materialName;

				modelArray.erase(modelArray.begin() + i);
				modelArray.insert(modelArray.begin() + i, { tempModel, tempIndex, newObjName, tempMatrix, tempColor, tempMaterialName });
			}

		}

		delete[]newName; 
	}

	if (msgHeader.nodeType == NODE_TYPE::TRANSFORM) {
	
		int newNameLen = 0;
		memcpy((char*)&newNameLen, buffer + sizeof(MsgHeader), sizeof(int));

		char* newName = new char[newNameLen];
		memcpy((char*)newName, buffer + sizeof(MsgHeader) + sizeof(int), (sizeof(char) * newNameLen));

		std::string newObjName = newName;
		newObjName = newObjName.substr(0, newNameLen);

		std::cout << "New transf. name: " << newObjName << std::endl; 

		delete[] newName; 
	}
}

// update matrix of objects by removing and replacing with new 
void updateNodeMatrix(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	//std::cout << "UPDATE NODE MATRIX FUNCTION" << std::endl;

	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {
		//std::cout << "============================" << std::endl;
		//std::cout << "UPDATE MATRIX MODEL: " << std::endl;

		Matrix newMatrix = {};
		memcpy((char*)&newMatrix, buffer + sizeof(MsgHeader), sizeof(Matrix));

		// check through existing meshes to find the one to update 
		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].name == objectName) {

				// copy the existing mesh
				int tempIndex   = modelArray[i].index;
				Model tempModel = modelArray[i].model;
				Mesh tempMesh   = modelArray[i].mesh; 
				Color tempColor = modelArray[i].color;
				std::string tempMaterialName = modelArray[i].materialName;

				// erase object and replace with new one
				modelArray.erase(modelArray.begin() + i);
				modelArray.insert(modelArray.begin() + i, { tempModel, tempIndex, objectName, newMatrix, tempColor ,tempMaterialName , tempMesh });
				
			}

			
		}
	}


}

// update material connected to mesh 
void updateMaterial(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	std::cout <<  std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATERIAL FUNCTION" << std::endl;

	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// get info sent from maya 
		materialMaya tempMat = {};
		memcpy((char*)&tempMat, buffer + sizeof(MsgHeader), sizeof(materialMaya));
		Color tempColor = { tempMat.color.r, tempMat.color.g, tempMat.color.b, tempMat.color.a };

		std::string materialName = tempMat.materialName;
		materialName = materialName.substr(0, tempMat.matNameLen);

		std::string texturePath = tempMat.fileTextureName;
		texturePath = texturePath.substr(0, tempMat.textureNameLen);

		//std::cout << "tempColor: " << (int)tempColor.r << " : " << (int)tempColor.g << " : " << (int)tempColor.b << " : " << (int)tempColor.a << " : " << std::endl;

		// create new material
		materialMaya newMaterial = {};

		Texture2D texture;
		newMaterial.type		   = tempMat.type;
		newMaterial.matNameLen	   = tempMat.matNameLen; 
		newMaterial.textureNameLen = tempMat.textureNameLen; 
		memcpy(newMaterial.materialName, materialName.c_str(), tempMat.matNameLen);
		memcpy(newMaterial.fileTextureName, texturePath.c_str(), tempMat.textureNameLen);

		if (tempMat.type == 0) {
			newMaterial.color = tempColor;
		}

		else if (tempMat.type == 1) {
			newMaterial.color = { 255, 255, 255, 255 };
			texture = LoadTexture(texturePath.c_str());
		}

		//check for material and replace if found
		bool matExists = false;
		for (int i = 0; i < *nrMaterials; i++) {
			if (materialArray[i].materialName == materialName) {
				
				//std::cout << "Material exists" << std::endl;
				matExists = true;
				materialArray.erase(materialArray.begin() + i);
				materialArray.insert(materialArray.begin() + i, { newMaterial });


			}

		}

		// if not found, add to array
		if (matExists == false) {

			//std::cout << "Material doesn't exists. Adding..." << std::endl;

			materialArray.push_back({ newMaterial });
			*nrMaterials = *nrMaterials + 1;
		
		}

		// check ojects to update material
		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].materialName == materialName) {
					
				Model tempModel		 = modelArray[i].model;
				Color tempModelColor = newMaterial.color;
				int tempModelIndex   = modelArray[i].index;
				std::string tempModelName = modelArray[i].name;
				Matrix tempModelMatrix	  = modelArray[i].modelMatrix;
				std::string tempModelMaterialName = modelArray[i].materialName;
				Mesh tempMesh = modelArray[i].mesh; 

				rlLoadMesh(&tempMesh, false);
				tempModel = LoadModelFromMesh(tempMesh);
				tempModel.material.shader = shader;

				modelArray.erase(modelArray.begin() + i);

				if (tempMat.type == 0) {
					//std::cout << "Updating model w color " << std::endl;
					modelArray.insert(modelArray.begin() + i, { tempModel, tempModelIndex, tempModelName, tempModelMatrix, tempModelColor, tempModelMaterialName, tempMesh });

				}

				else if (tempMat.type == 1) {
					//std::cout << "Updating model w texture " << std::endl;
					tempModel.material.maps[MAP_DIFFUSE].texture = texture;
					modelArray.insert(modelArray.begin() + i, { tempModel, tempModelIndex, tempModelName, tempModelMatrix, tempModelColor, tempModelMaterialName, tempMesh });

				}

			}

			else {
				std::cout << "No mesh found with the material name" << std::endl;

			}
		}
		
	}

	

	std::cout << "=======================================" << std::endl;
	std::cout << std::endl;
}

// update material 
void updateMaterialName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATERIAL NAME FUNCTION" << std::endl;

	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		std::cout << "MATERIAL" << std::endl;
		//std::cout << "meshName: " << objectName << std::endl;
		
		Color tempColor; 
		Texture2D texture;

		// get material info 
		materialMaya tempMat = {};
		memcpy((char*)&tempMat, buffer + sizeof(MsgHeader), sizeof(materialMaya));
		tempColor = { tempMat.color.r, tempMat.color.g, tempMat.color.b, tempMat.color.a };

		std::string materialName = tempMat.materialName;
		materialName = materialName.substr(0, tempMat.matNameLen);

		std::string texturePath = tempMat.fileTextureName;
		texturePath = texturePath.substr(0, tempMat.textureNameLen);

		/* 
		if (tempMat.type == 0) {
			std::cout << "is color" << std::endl;
		}
		*/

		if (tempMat.type == 1) {
			//std::cout << "is texture" << std::endl;
			texture = LoadTexture(texturePath.c_str());
		}


		materialMaya newMaterial = {};
		newMaterial.type  = tempMat.type;
		newMaterial.color = tempColor;
		memcpy(newMaterial.materialName, materialName.c_str(), tempMat.matNameLen);
		memcpy(newMaterial.fileTextureName, texturePath.c_str(), tempMat.textureNameLen);

		int matIndex = -1; 
		// check if material exists
		bool matExists = false;
		for (int i = 0; i < *nrMaterials; i++) {
			if (materialArray[i].materialName == materialName) {
				
				//std::cout << "Material found" << std::endl;

				// if it exists, erase the material and insert new
				matExists = true;
				materialArray.erase(materialArray.begin() + i);
				materialArray.insert(materialArray.begin() + i, { newMaterial });

				matIndex = i; 
				
			}
		}


		//if it doesn't exist, add it
		if (matExists == false) {

			std::cout << "Material doesn't exist. Adding...." << std::endl;

			materialArray.push_back({newMaterial });
			*nrMaterials = *nrMaterials + 1;
			matIndex = *nrMaterials; 
		}

		
		// "update" mesh with material
		for (int i = 0; i < *nrObjs; i++) {
			if (modelArray[i].name == objectName) {
				

				Color newColor  = { tempColor };
				Mesh tempMesh   = modelArray[i].mesh;
				int tempIndex   = modelArray[i].index;
				Model tempModel = modelArray[i].model;
				std::string modelName = modelArray[i].name;
				Matrix newModelMatrix = modelArray[i].modelMatrix;
				std::string tempMaterialName = materialName;
				
				rlLoadMesh(&tempMesh, false);
				tempModel = LoadModelFromMesh(tempMesh);
				tempModel.material.shader = shader;
			

				modelArray.erase(modelArray.begin() + i);

				if (tempMat.type == 0) {
					modelArray.insert(modelArray.begin() + i, { tempModel, tempIndex, modelName, newModelMatrix, newColor, tempMaterialName, tempMesh });
					//std::cout << "NEW MODEL MAT: " << modelArray[i].materialName << std::endl;

				}

				else if (tempMat.type == 1) {
					tempModel.material.maps[MAP_DIFFUSE].texture = texture;
					modelArray.insert(modelArray.begin() + i, { tempModel, tempIndex, modelName, newModelMatrix, newColor, tempMaterialName, tempMesh });
					//std::cout << "NEW MODEL MAT: " << modelArray[i].materialName << std::endl;
			
				}



			}

			else {
				std::cout << "No mesh with obj name found " << std::endl; 
			}
		}

	}

	std::cout << "=======================================" << std::endl;
	std::cout << std::endl;

}
