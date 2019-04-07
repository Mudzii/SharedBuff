#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <sstream>
#include <map>
#include <functional>

#include <Windows.h>

//#include <C:\Users\BTH\Desktop\SharedBuff-master\shared\Project1\ComLib.h>
#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>

#pragma comment(lib, "Project1.lib")
ComLib ourComLib("buffer2", 50, CONSUMER);

enum NODE_TYPE {
	TRANSFORM,
	MESH,
	LIGHT,
	CAMERA
};

enum CMDTYPE {
	DEFAULT = 1000,
	NEW_NODE = 1001,
	UPDATE_NODE = 1002,
	UPDATE_MATRIX = 1003,
	UPDATE_NAME = 1004
};

struct cameraFromMaya {
	Vector3 up;
	Vector3 forward;
	Vector3 pos;
	int type;
	float FOVy;
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
};

struct modelPos {
	Model model;
	Matrix modelMatrix;
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
};

//model
void addNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
void updateNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
void updateNodeMatrix(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
void updateNodeName(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);

////light
//void addLight(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
//void updateLightColor(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
//void updateLightMatrix(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
////void updateNameLight(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya);
//
////camera
//void updateCamera(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya);
//

typedef void(*FnPtr)(std::vector<modelFromMaya>&, char*, int, Shader, int*, int*, std::vector<lightFromMaya>&, std::vector<cameraFromMaya>&);

CMDTYPE recvFromMaya(char* buffer);

int main() {

	SetTraceLog(LOG_WARNING);

	/////// OUR STUFF
	std::vector<modelFromMaya> modelsFromMaya;
	std::vector<lightFromMaya> lightsFromMaya;
	std::vector<cameraFromMaya> cameraMaya;

	Vector3 lightPos = { 4,1,4 };
	float radius = 0.1;
	Color color = { 1 * 255, 1 * 255, 1 * 255, 1 * 255 };
	lightsFromMaya.push_back({ lightPos, radius, color });

	std::map<CMDTYPE, FnPtr> funcMap;
	funcMap[NEW_NODE] = addNode;
	funcMap[UPDATE_NODE] = updateNode;
	funcMap[UPDATE_MATRIX] = updateNodeMatrix;
	funcMap[UPDATE_NAME] = updateNodeName;


	int modelIndex = 0;
	int nrOfObj = 0;
	Model cube;
	size_t tempArraySize = 0;
	//////

	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth = 1200;
	int screenHeight = 800;

	SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screenWidth, screenHeight, "demo");

	// Define the camera to look into our 3d world
	Camera camera = { 0 };
	camera.position = { 4.0f, 4.0f, 4.0f };
	camera.target = { 0.0f, 1.0f, -1.0f };
	camera.up = { 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.type = CAMERA_PERSPECTIVE;

	cameraMaya.push_back({ camera.up, camera.target, camera.position });

	// real models rendered each frame
	std::vector<modelPos> flatScene;

	Material material1 = LoadMaterialDefault();
	Texture2D texture1 = LoadTexture("resources/models/watermill_diffuse.png");
	material1.maps[MAP_DIFFUSE].texture = texture1;

	Shader shader1 = LoadShader("resources/shaders/glsl330/phong.vs",
		"resources/shaders/glsl330/phong.fs");   // Load model shader
	material1.shader = shader1;

	Mesh mesh1 = LoadMesh("resources/models/watermill.obj");
	Model model1 = LoadModelFromMesh(mesh1);
	model1.material = material1;                     // Set shader effect to 3d model


	unsigned char colors[12] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };

	std::cout << "-----------------" << std::endl;

	Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
	SetCameraMode(camera, CAMERA_FREE);         // Set an orbital camera mode
	SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second
	//initializing camera!!!!

	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc = GetShaderLocation(shader1, "view");
	int projLoc = GetShaderLocation(shader1, "projection");

	int lightLoc = GetShaderLocation(shader1, "lightPos");

	// =====================================

	// Main game loop
	while (!WindowShouldClose())                // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		UpdateCamera(&camera);                  // Update camera
		//----------------------------------------------------------------------------------
		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		///////////////////////// TEST FOR DRAW MODEL UNDER RUNTIME
		tempArraySize = ourComLib.nextSize();
		char* tempArray = new char[tempArraySize];

		CMDTYPE messageType = CMDTYPE::DEFAULT;
		messageType = recvFromMaya(tempArray);

		if (messageType != CMDTYPE::DEFAULT) {
			funcMap[messageType](modelsFromMaya, tempArray, tempArraySize, shader1, &nrOfObj, &modelIndex, lightsFromMaya, cameraMaya);
		}


		int i = 0;
		i++;
		//////////////////////////////
		camera.target = cameraMaya[0].forward;
		camera.position = cameraMaya[0].pos;
		camera.fovy = cameraMaya[0].FOVy;
		camera.type = cameraMaya[0].type;
		camera.up = cameraMaya[0].up;



		SetShaderValueMatrix(shader1, viewLoc, GetCameraMatrix(camera));

		SetShaderValue(shader1, lightLoc, Vector3ToFloat(lightsFromMaya[0].lightPos), 1);

		ClearBackground(RAYWHITE);

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

		BeginMode3D(camera);

		for (int i = 0; i < modelsFromMaya.size(); i++)
		{
			auto m = modelsFromMaya[i];
			auto l = GetShaderLocation(m.model.material.shader, "model");
			SetShaderValueMatrix(m.model.material.shader, modelLoc, m.modelMatrix);
			DrawModel(m.model, {}, 1.0, lightsFromMaya[0].color);
		}

		DrawGrid(10, 1.0f);     // Draw a grid
		DrawSphere(lightsFromMaya[0].lightPos, 0.1, lightsFromMaya[0].color);

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
	//UnloadModel(tri);
	//UnloadModel(cubeMod);
	//UnloadModel(cube);

	CloseWindowRL();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------
	return 0;
}




CMDTYPE recvFromMaya(char* buffer)
{
	MsgHeader msgHeader = {};
	msgHeader.cmdType = CMDTYPE::DEFAULT;

	size_t nr = ourComLib.nextSize();
	size_t oldBuffSize = nr;

	char* msgBuff = new char[oldBuffSize];

	if (nr > oldBuffSize) {
		delete[] msgBuff;
		msgBuff = new char[nr];
		oldBuffSize = nr;
	}

	bool receive = ourComLib.recv(msgBuff, nr);

	if (receive == true) {

		memcpy((char*)&msgHeader, msgBuff, sizeof(MsgHeader));

		char* msg = new char[msgHeader.msgSize];

		if (msgHeader.nodeType == NODE_TYPE::MESH) {

			msgMesh mesh = {};

			memcpy((char*)&mesh, msgBuff + sizeof(MsgHeader), sizeof(msgMesh));
			memcpy((char*)msg, msgBuff + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);


			memcpy(buffer, msgBuff, sizeof(MsgHeader));		//HEADER FIRST
			memcpy(buffer + sizeof(MsgHeader), msgBuff + sizeof(MsgHeader), sizeof(msgMesh));	//MESH STRUCT
			memcpy(buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgBuff + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);		//THE MSG

		}

		else {
			memcpy((char*)msg, msgBuff + sizeof(MsgHeader), msgHeader.msgSize);

			memcpy(buffer, msgBuff, sizeof(MsgHeader));		//HEADER
			memcpy(buffer + sizeof(MsgHeader), msgBuff + sizeof(MsgHeader), msgHeader.msgSize);	//MSG


		}

		//std::cout << "print msg: " << msg << "_" << std::endl;
		delete[] msg;
	}

	delete[] msgBuff;
	return msgHeader.cmdType;
}

/////////////////////////
//					   //
//		MODEL		   //
//					   //
/////////////////////////

void addNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
{
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";

	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//std::cout << "in add node and printing cmdType " << msgHeader.cmdType << " " << std::endl;
	//::cout << "in add node and printing node_TYPE " << msgHeader.nodeType << " " << std::endl;

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "============================" << std::endl;
		std::cout << "ADD MODEL: " << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		//float* arrayVtx = new float[msgHeader.msgSize];
		//int lengthVtxArr = 0;

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		std::string tempX, tempY, tempZ = "";
		std::string tempNormX, tempNormY, tempNormZ = "";

		int nrVtx;
		int vtxCheck = 0;
		int nrNorm;
		int normCheck = 0;

		int lengthVtxArr = 0;
		int lengthNormArr = 0;

		int element = 0;
		float tempFloat = 0.0f;

		int nrOfElements = mesh.trisCount * 3 * 3; //for each tris, add each vtx and then [x,y,z]

		ss >> nrVtx;

		float* arrayVtx = new float[nrVtx * 3];
		float* arrayNorm;

		while (!ss.eof()) {

			while (vtxCheck < nrVtx)
			{
				//std::cout << "VERTEX" << std::endl;
				ss >> tempX >> tempY >> tempZ;

				if (element >= nrOfElements) {
					//std::cout << "Last element fount " << std::endl;
					break;
				}

				if (std::stringstream(tempX) >> tempFloat && std::stringstream(tempY) >> tempFloat && std::stringstream(tempZ) >> tempFloat) {
					//std::cout << "TEMP: " << tempX << " : " << tempY << " : " << tempX << std::endl;

					arrayVtx[element] = (float)std::stof(tempX);
					arrayVtx[element + 1] = (float)std::stof(tempY);
					arrayVtx[element + 2] = (float)std::stof(tempZ);

					lengthVtxArr = lengthVtxArr + 3;
					element = element + 3;
				}
				vtxCheck++;
			}

			ss >> nrNorm;

			arrayNorm = new float[nrNorm * 3];

			element = 0;

			while (normCheck < nrNorm) {
				std::cout << "NORMALS" << std::endl;
				ss >> tempNormX >> tempNormY >> tempNormZ;

				if (element >= nrOfElements) {
					//std::cout << "Last element fount " << std::endl;
					break;
				}

				arrayNorm[element] = (float)std::stof(tempNormX);
				//std::cout << "NORM X:" << arrayNorm[element] << std::endl;
				arrayNorm[element + 1] = (float)std::stof(tempNormY);
				//std::cout << "NORM Y:" << arrayNorm[element + 1] << std::endl;
				arrayNorm[element + 2] = (float)std::stof(tempNormZ);
				//std::cout << "NORM Z" << arrayNorm[element + 2] << std::endl;

				lengthNormArr = lengthNormArr + 3;
				element = element + 3;

				normCheck++;

			}

			break;
		}

		std::cout << arrayNorm << std::endl;

		bool exists = false;
		for (int i = 0; i < *nrObjs; i++) {
			if (objNameArray[i].name == objectName) {
				exists = true;
			}
		}

		exists == false;

		if (exists == false) {

			Mesh tempMeshToAdd = {};
			tempMeshToAdd.vertexCount = lengthVtxArr;
			tempMeshToAdd.vertices = new float[lengthVtxArr];
			memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);

			tempMeshToAdd.normals = new float[lengthNormArr];
			memcpy(tempMeshToAdd.normals, arrayNorm, sizeof(float) * lengthNormArr);

			rlLoadMesh(&tempMeshToAdd, false);

			Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
			tempModelToAdd.material.shader = shader1;

			objNameArray.push_back({ tempModelToAdd, *index, objectName, MatrixTranslate(2,0,2) });
			*index = *index + 1;
			*nrObjs = *nrObjs + 1;
		}

		delete[] msgElements;
		delete[] arrayVtx;
	}

	if (msgHeader.nodeType == NODE_TYPE::LIGHT)
	{
		std::cout << "============================" << std::endl;
		std::cout << "ADD LIGHT: " << std::endl;

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		float lightPos[3];
		float lightColor[4];
		int check = 0;

		ss >> lightPos[0] >> lightPos[1] >> lightPos[2] >> lightColor[0] >> lightColor[1] >> lightColor[2] >> lightColor[3];

		//std::cout << "lightPos: " << lightPos[0] << " " << lightPos[1] << " " << lightPos[2] << "lightColor: " << lightColor[0] << " " << lightColor[1] << " " << lightColor[2] << " " << lightColor[3] << std::endl;

		Color tempColor = { lightColor[0] * 255, lightColor[1] * 255, lightColor[2] * 255, lightColor[3] * 255 };
		Vector3 tempPos = { lightPos[0], lightPos[1], lightPos[2] };

		lightsFromMaya.erase(lightsFromMaya.begin());
		lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });

		delete[] msgElements;
	}
}

void updateNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya) {
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//std::cout << "in update and printing cmdType " << msgHeader.cmdType << " " << std::endl;
	//std::cout << "in update and printing node_TYPE " << msgHeader.nodeType << " " << std::endl;

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "============================" << std::endl;
		std::cout << "UPDATE MODEL: " << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct
		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		//float* arrayVtx = new float[msgHeader.msgSize];
		//int lengthVtxArr = 0;

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		int nrVtx;
		int vtxCheck = 0;
		int nrNorm;
		int normCheck = 0;

		// setup stringstream

		ss >> nrVtx;

		float* arrayNorm;
		int lengthNormArr = 0;
		float* arrayVtx = new float[nrVtx * 3];
		int lengthVtxArr = 0;

		std::string tempX, tempY, tempZ = "";
		int element = 0;
		float tempFloat = 0.0f;

		int nrOfElements = mesh.trisCount * 3 * 3; //for each tris, add each vtx and then [x,y,z]

		std::string tempNormX, tempNormY, tempNormZ = "";


		while (!ss.eof()) {

			while (vtxCheck < nrVtx)
			{
				ss >> tempX >> tempY >> tempZ;

				if (element >= nrOfElements) {
					//std::cout << "Last element fount " << std::endl;
					break;
				}

				if (std::stringstream(tempX) >> tempFloat && std::stringstream(tempY) >> tempFloat && std::stringstream(tempZ) >> tempFloat) {
					//std::cout << "TEMP: " << tempX << " : " << tempY << " : " << tempX << std::endl;

					arrayVtx[element] = (float)std::stof(tempX);
					arrayVtx[element + 1] = (float)std::stof(tempY);
					arrayVtx[element + 2] = (float)std::stof(tempZ);

					lengthVtxArr = lengthVtxArr + 3;
					element = element + 3;
				}
				vtxCheck++;
			}

			ss >> nrNorm;

			arrayNorm = new float[nrNorm * 3];

			element = 0;

			while (normCheck < nrNorm)
			{
				std::cout << "NORMALS" << std::endl;
				ss >> tempNormX >> tempNormY >> tempNormZ;

				if (element >= nrOfElements) {
					//std::cout << "Last element fount " << std::endl;
					break;
				}

				arrayNorm[element] = (float)std::stof(tempNormX);
				//std::cout << "NORM X:" << arrayNorm[element] << std::endl;
				arrayNorm[element + 1] = (float)std::stof(tempNormY);
				//std::cout << "NORM Y:" << arrayNorm[element + 1] << std::endl;
				arrayNorm[element + 2] = (float)std::stof(tempNormZ);
				//std::cout << "NORM Z" << arrayNorm[element + 2] << std::endl;

				lengthNormArr = lengthNormArr + 3;
				element = element + 3;

				normCheck++;

			}

			break;

		}

		for (int i = 0; i < *nrObjs; i++)
		{
			if (objNameArray[i].name == objectName)
			{
				int tempIndex = objNameArray[i].index;

				Mesh tempMeshToAdd = {};
				tempMeshToAdd.vertexCount = lengthVtxArr;
				tempMeshToAdd.vertices = new float[lengthVtxArr];
				memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);

				tempMeshToAdd.normals = new float[lengthNormArr];
				memcpy(tempMeshToAdd.normals, arrayNorm, sizeof(float) * lengthNormArr);

				rlLoadMesh(&tempMeshToAdd, false);

				Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
				tempModelToAdd.material.shader = shader1;

				Matrix tempMat = objNameArray[i].modelMatrix;

				objNameArray.erase(objNameArray.begin() + i);
				objNameArray.insert(objNameArray.begin() + i, { tempModelToAdd, tempIndex, objectName, tempMat });
			}
		}

		delete[] msgElements;
		delete[] arrayVtx;
	}

	if (msgHeader.nodeType == NODE_TYPE::LIGHT)
	{
		std::cout << "============================" << std::endl;
		std::cout << "UPDATE LIGHT: " << std::endl;

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		float lightColor[4] = { 0, 0, 0, 255 };

		ss >> lightColor[0] >> lightColor[1] >> lightColor[2] >> lightColor[3];

		Color tempColor = { lightColor[0] * 255, lightColor[1] * 255, lightColor[2] * 255, lightColor[3] * 255 };
		Vector3 tempPos = lightsFromMaya[0].lightPos;

		lightsFromMaya.erase(lightsFromMaya.begin());
		lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });

		delete[] msgElements;
	}

	if (msgHeader.nodeType == NODE_TYPE::CAMERA) {
		Vector3 up = { 0,0,0 };
		Vector3 foward = { 0,0,0 };
		Vector3 pos = { 0,0,0 };
		float FOVy;

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		int camType;
		std::string camTypeString;
		ss >> camTypeString >> up.x >> up.y >> up.z >> foward.x >> foward.y >> foward.z >> pos.x >> pos.y >> pos.z >> FOVy;

		float degreesFOV = FOVy * (180.0 / 3.141592653589793238463);

		if (camTypeString == "|persp|perspShape") {
			camType = CAMERA_PERSPECTIVE;
		}

		else {
			camType = CAMERA_ORTHOGRAPHIC;
		}


		cameraFromMaya.erase(cameraFromMaya.begin());
		cameraFromMaya.insert(cameraFromMaya.begin(), { up, foward, pos, camType, degreesFOV });
	}
}

void updateNodeMatrix(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
{
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//std::cout << "in matrix and printing cmdType " << msgHeader.cmdType << " " << std::endl;
	//std::cout << "in matrix and printing node_TYPE " << msgHeader.nodeType << " " << std::endl;

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "============================" << std::endl;
		std::cout << "UPDATE MATRIX MODEL: " << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		float matrixPos[3] = { 0, 0, 0 };

		ss >> matrixPos[0] >> matrixPos[1] >> matrixPos[2];

		for (int i = 0; i < *nrObjs; i++)
		{
			//std::cout << "objName: " << objNameArray[i].name << std::endl;
			//std::cout << "tempName: " << objectName << std::endl;

			if (objNameArray[i].name == objectName)
			{
				int tempIndex = objNameArray[i].index;
				Model tempModel = objNameArray[i].model;
				int tempIndex2 = objNameArray[i].index;
				std::string tempName2 = objNameArray[i].name;
				Matrix newModelMatrix = MatrixTranslate(matrixPos[0], matrixPos[1], matrixPos[2]);

				objNameArray.erase(objNameArray.begin() + i);
				objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix });
			}
		}

		delete[] msgElements;
	}

	if (msgHeader.nodeType == NODE_TYPE::LIGHT)
	{
		std::cout << "============================" << std::endl;
		std::cout << "UPDATE LIGHT MATRIX" << std::endl;

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		float matrixPos[3] = { 0, 0, 0 };

		ss >> matrixPos[0] >> matrixPos[1] >> matrixPos[2];

		//std::cout << matrixPos[0] << matrixPos[1] << matrixPos[2] << std::endl;

		Color tempColor = lightsFromMaya[0].color;
		Vector3 tempPos = { matrixPos[0], matrixPos[1], matrixPos[2] };

		lightsFromMaya.erase(lightsFromMaya.begin());
		lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });

		delete[] msgElements;
	}

}

void updateNodeName(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
{
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//std::cout << "oldName: " << objectName << "_" << std::endl;

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "============================" << std::endl;
		std::cout << "UPDATE NAME MODEL: " << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct
		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		std::string newName = "";

		ss >> newName;
		newName = newName.substr(0, newName.length() - 1);


		//std::cout << "newName: " << newName << "_" << std::endl;

		for (int i = 0; i < *nrObjs; i++)
		{
			if (objNameArray[i].name == objectName)
			{
				int tempIndex = objNameArray[i].index;
				Model tempModel = objNameArray[i].model;
				Matrix tempMatrix = objNameArray[i].modelMatrix;

				objNameArray.erase(objNameArray.begin() + i);
				objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex, newName, tempMatrix });
			}
		}
		delete[] msgElements;
	}
}

///////////////////////////
////					   //
////		LIGHT		   //
////					   //
///////////////////////////
//
//void addLight(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
//{
//	float refereanceFloat;	//use to find int type in string array
//	char referenceChar;
//
//	float lightPos[3];
//	float lightColor[4];
//	int check = 0;
//
//	std::string testString(buffer, bufferSize);
//	std::stringstream stringStream;
//	stringStream << testString;
//
//	std::string reference;
//	std::string temp;
//
//	stringStream >> temp;
//	if (std::stringstream(temp) >> reference)
//	{
//		//is command, discard
//	}
//
//	int i = 0;
//	while (!stringStream.eof())
//	{
//		stringStream >> temp;
//		if (std::stringstream(temp) >> refereanceFloat)
//		{
//			if (check == 0)
//			{
//				lightPos[i] = (float)std::stof(temp);
//				i++;
//			}
//			
//			if (check == -1)
//			{
//				lightColor[i] = (float)std::stof(temp);
//				i++;
//			}
//			temp = "";
//		}
//		if (std::stringstream(temp) >> referenceChar)
//		{
//			if (referenceChar == '|')
//			{
//				check = -1;
//				i = 0;
//			}
//
//		}
//	}
//
//	std::cout << "Light Pos: " << lightPos[0] << " " << lightPos[1] << " " << lightPos[2] << std::endl;
//	std::cout << "Light Color: " << lightColor[0] << " " << lightColor[1] << " " << lightColor[2] << " " << lightColor[3] << std::endl;
//
//	Color tempColor = { lightColor[0] * 255, lightColor[1] * 255, lightColor[2] * 255, lightColor[3] * 255 };
//	Vector3 tempPos = { lightPos[0], lightPos[1], lightPos[2] };
//
//	lightsFromMaya.erase(lightsFromMaya.begin());
//	lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });
//
//}
//
//void updateLightColor(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
//{
//	float refereanceFloat;	//use to find int type in string array
//	float lightColor[4] = { 0, 0, 0, 255 };
//
//	std::string testString(buffer, bufferSize);
//	std::stringstream stringStream;
//	stringStream << testString;
//
//	std::string reference;
//	std::string temp;
//
//	stringStream >> temp;
//	if (std::stringstream(temp) >> reference)
//	{
//		//is command, discard
//	}
//
//	int i = 0;
//	while (!stringStream.eof())
//	{
//		stringStream >> temp;
//		if (std::stringstream(temp) >> refereanceFloat)
//		{
//			lightColor[i] = (float)std::stof(temp);
//			i++;
//			
//			temp = "";
//		}
//	}
//
//	Color tempColor = { lightColor[0] * 255, lightColor[1] * 255, lightColor[2] * 255, lightColor[3] * 255 };
//	Vector3 tempPos = lightsFromMaya[0].lightPos;
//
//	lightsFromMaya.erase(lightsFromMaya.begin());
//	lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });
//}
//
//void updateLightMatrix(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
//{
//	float refereanceFloat;	//use to find int type in string array
//	float lightPos[3] = { 0, 0, 0 };
//
//	std::string testString(buffer, bufferSize);
//	std::stringstream stringStream;
//	stringStream << testString;
//
//	std::string reference;
//	std::string temp;
//
//	stringStream >> temp;
//	if (std::stringstream(temp) >> reference)
//	{
//		//is command, discard
//	}
//
//	int i = 0;
//	while (!stringStream.eof())
//	{
//		stringStream >> temp;
//		if (std::stringstream(temp) >> refereanceFloat)
//		{
//			lightPos[i] = (float)std::stof(temp);
//			i++;
//
//			temp = "";
//		}
//	}
//
//	Color tempColor = lightsFromMaya[0].color;
//	Vector3 tempPos = { lightPos[0], lightPos[1], lightPos[2] };
//
//	lightsFromMaya.erase(lightsFromMaya.begin());
//	lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });
//}
//
///////////////////////////
////					   //
////		CAMERA		   //
////					   //
///////////////////////////
//
//void updateCamera(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya)
//{
//	Vector3 up = { 0,0,0 };
//	Vector3 foward = { 0,0,0 };
//	Vector3 pos = { 0,0,0 };
//
//	std::string testString(buffer, bufferSize);
//	std::istringstream stringStream(buffer);
//	//stringStream << testString;
//	std::string tempString = "";
//
//	float tempFloat;
//
//	stringStream >> tempString >> tempFloat >> tempFloat >> tempFloat >> foward.x >> foward.y >> foward.z >> up.x >> up.y >> up.z >> pos.x >> pos.y >> pos.z;
//
//	cameraFromMaya.erase(cameraFromMaya.begin());
//	cameraFromMaya.insert(cameraFromMaya.begin(), { {0,1,0 }, foward, pos});
//}