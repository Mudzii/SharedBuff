#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <sstream>

#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>
#pragma comment(lib, "Project1.lib")

#include <map>
#include <functional>

#include <Windows.h>
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
	UPDATE_NAME = 1004,
	UPDATE_MATERIAL = 1005,
	UPDATE_MATERIALNAME = 1006
};

struct cameraFromMaya
{
	Vector3 up;
	Vector3 forward;
	Vector3 pos;
	int type;
	float fov;
};

struct lightFromMaya
{
	Vector3 lightPos;
	float radius;
	Color color;
};

struct modelFromMaya
{
	Model model;
	int index;
	std::string name;
	Matrix modelMatrix;
	Color color;
	std::string materialName;
};

struct materialMaya {
	std::string fileTexuteName;
	Color color;
	std::string ColorOrTexture;
	std::string name;
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
void addNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials);
void updateNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials);
void updateNodeMatrix(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials);
void updateNodeName(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials);
void updateMaterial(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials);
void updateMaterialName(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials);

typedef void(*FnPtr)(std::vector<modelFromMaya>&, char*, int, Shader, int*, int*, std::vector<lightFromMaya>&, std::vector<cameraFromMaya>&, std::vector<materialMaya>&, int*);

CMDTYPE recvFromMaya(char* buffer);

int main()
{
	SetTraceLog(LOG_WARNING);

	/////// OUR STUFF
	std::vector<modelFromMaya> modelsFromMaya;
	std::vector<lightFromMaya> lightsFromMaya;
	std::vector<cameraFromMaya> cameraMaya;
	std::vector<materialMaya> materialMaya;


	Vector3 lightPos = { 0,0,0 };
	float radius = 0.1;
	Color color = { 1 * 255, 1 * 255, 1 * 255, 1 * 255 };
	lightsFromMaya.push_back({ lightPos, radius, color });

	std::map<CMDTYPE, FnPtr> funcMap;
	funcMap[NEW_NODE] = addNode;
	funcMap[UPDATE_NODE] = updateNode;
	funcMap[UPDATE_MATRIX] = updateNodeMatrix;
	funcMap[UPDATE_NAME] = updateNodeName;
	funcMap[UPDATE_MATERIAL] = updateMaterial;
	funcMap[UPDATE_MATERIALNAME] = updateMaterialName;


	int modelIndex = 0;
	int nrOfObj = 0;
	int nrMaterials = 0;
	Model cube;
	size_t tempArraySize = 0;
	//

	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth = 1200;
	int screenHeight = 800;

	SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	//SetConfigFlags(FLAG_VSYNC_HINT);
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
	SetTargetFPS(130);                           // Set our game to run at 60 frames-per-second
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
		//UpdateCamera(&camera);                  // Update camera
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
			funcMap[messageType](modelsFromMaya, tempArray, tempArraySize, shader1, &nrOfObj, &modelIndex, lightsFromMaya, cameraMaya, materialMaya, &nrMaterials);
		}


		int i = 0;
		i++;

		if (tempArray != NULL) {
			delete[] tempArray;
		}
		//////////////////////////////
		camera.target = cameraMaya[0].forward;
		camera.position = cameraMaya[0].pos;
		camera.fovy = cameraMaya[0].fov;
		camera.type = cameraMaya[0].type;
		camera.up = cameraMaya[0].up;



		float *test = Vector3ToFloat(lightsFromMaya[0].lightPos);
		SetShaderValueMatrix(shader1, viewLoc, GetCameraMatrix(camera));

		//std::cout << "Before: " << lightsFromMaya[0].lightPos.x << " " << lightsFromMaya[0].lightPos.y << " " << lightsFromMaya[0].lightPos.z << std::endl;
		SetShaderValue(shader1, lightLoc, Vector3ToFloat(lightsFromMaya[0].lightPos), 1);
		//std::cout << "After: " << lightsFromMaya[0].lightPos.x << " " << lightsFromMaya[0].lightPos.y << " " << lightsFromMaya[0].lightPos.z << std::endl;

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

		DrawSphere(lightsFromMaya[0].lightPos, 0.1, lightsFromMaya[0].color);

		for (int i = 0; i < modelsFromMaya.size(); i++)
		{
			auto m = modelsFromMaya[i];
			auto l = GetShaderLocation(m.model.material.shader, "model");
			SetShaderValueMatrix(m.model.material.shader, modelLoc, m.modelMatrix);

			Color finalColor = { (lightsFromMaya[0].color.r + m.color.r),(lightsFromMaya[0].color.g + m.color.g),(lightsFromMaya[0].color.b + m.color.b),(lightsFromMaya[0].color.a + m.color.a) };
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
	for (int i = 0; i < modelsFromMaya.size(); i++) {
		UnloadModel(modelsFromMaya[i].model);
	}
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
		if (msg != NULL) {
			delete[] msg;
		}
	}

	if (msgBuff != NULL) {
		delete[] msgBuff;
	}
	return msgHeader.cmdType;
}

/////////////////////////
//					   //
//		MODEL		   //
//					   //
/////////////////////////

void addNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials)
{
	std::cout << "ADD" << std::endl;
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	std::cout << "in add node and printing cmdType " << msgHeader.cmdType << " " << std::endl;
	std::cout << "in add node and printing node_TYPE " << msgHeader.nodeType << " " << std::endl;

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "============================" << std::endl;
		std::cout << "ADD MODEL: " << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct

		//char* msgElements = new char[msgHeader.msgSize];
		//memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		char* verticeArray = new char[sizeof(char) * mesh.vtxCount * 3];
		memcpy((char*)verticeArray, buffer + sizeof(msgMesh) + sizeof(MsgHeader) , sizeof(char) * mesh.vtxCount * 3);

		

		std::cout << "mesh trisCount: " << mesh.trisCount << "____" << std::endl;
		std::cout << std::endl; 
		std::cout << "mesh trisCount: " << mesh.trisCount << "____" << std::endl;
		std::cout << std::endl;
		
		for (int x = 0; x < mesh.vtxCount * 3; x++) {
			std::cout << "ARR: " << verticeArray[x] << std::endl;
		}

		// setup stringstream
		//std::string msgString(msgElements, msgHeader.msgSize);
		//std::istringstream ss(msgString);



		
		//std::cout << "vtxArrTest: " << vtxArrTest << " _ " << std::endl;

		/* 
		int nrOfElements = mesh.trisCount * 3 * 3; //for each tris, add each vtx and then [x,y,z]

		int nrVtx;
		int vtxCheck = 0;
		int nrNorm;
		int normCheck = 0;


		//ss >> nrVtx;
		
		float* arrayVtx = new float[nrVtx * 3];
		int lengthVtxArr = 0;

		std::string tempX, tempY, tempZ = "";
		int element = 0;
		float tempFloat = 0.0f;

		std::string tempNormX, tempNormY, tempNormZ = "";
		int lengthNormArr = 0;
		float* arrayNorm;

		std::string tempU, tempV = "";
		int nrUV;
		int UVCheck = 0;
		int lenghtUVArr = 0;
		float* arrayUV;

		std::string colorOrTexture;
		Vector4 newColor = { 255, 255, 255,255 };
		std::string materialName;
		Texture2D texture;
		std::string texturePath;
		

		*/

		/* 
		
		while (!ss.eof()) {

			while (vtxCheck < nrVtx)
			{
				ss >> tempX >> tempY >> tempZ;

				if (element >= nrOfElements) {
					std::cout << "Last element fount " << std::endl;
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
				ss >> tempNormX >> tempNormY >> tempNormZ;

				if (element >= nrOfElements) {
					std::cout << "Last element fount " << std::endl;
					break;
				}

				arrayNorm[element] = (float)std::stof(tempNormX);
				arrayNorm[element + 1] = (float)std::stof(tempNormY);
				arrayNorm[element + 2] = (float)std::stof(tempNormZ);

				lengthNormArr = lengthNormArr + 3;
				element = element + 3;

				normCheck++;

			}

			ss >> nrUV;
			std::cout << "nrUV: " << nrUV << std::endl;
			arrayUV = new float[nrUV * 2];
			element = 0;

			while (UVCheck < nrUV)
			{
				std::cout << UVCheck << " UV's" << std::endl;
				ss >> tempU >> tempV;

				if (element >= nrOfElements) {
					//std::cout << "Last element fount " << std::endl;
					break;
				}

				arrayUV[element] = (float)std::stof(tempU);
				arrayUV[element + 1] = 1.0f - (float)std::stof(tempV);

				//std::cout << "TEMP U:" << arrayUV[element] << std::endl;
				//std::cout << "TEMP V:" << arrayUV[element + 1] << std::endl;
				//std::cout << " " << std::endl;

				lenghtUVArr = lenghtUVArr + 2;
				element = element + 2;
				UVCheck++;
			}

			std::cout << "arrayUV: " << std::endl;
			//for(int i = 0; i < lenghtUVArr; i++)
				//std::cout << arrayUV << std::endl;

			//Material stuff
			ss >> materialName;
			ss >> colorOrTexture;
			std::cout << "materialName: " << materialName << std::endl;
			std::cout << "colorOrTexture: " << colorOrTexture << std::endl;
			if (colorOrTexture == "color")
			{
				ss >> newColor.x >> newColor.y >> newColor.z;
				std::cout << "�newColor: " << newColor.x << " " << newColor.y << " " << newColor.z << " " << std::endl;
			}
			else if (colorOrTexture == "texture") {
				ss >> texturePath;
				std::cout << "texturePath: " << texturePath << std::endl;
			}



			//check if alreay exists
			bool exists = false;
			for (int i = 0; i < *nrMaterials; i++) {
				if (materialMaya[i].name == materialName) {
					exists = true;
					materialMaya.erase(materialMaya.begin() + i);

					if (colorOrTexture == "color")
					{
						Color tempColor = { newColor.x * 255, newColor.y * 255, newColor.z * 255, 255 };
						std::string textureCheck = "noTexture";
						materialMaya.insert(materialMaya.begin() + i, { textureCheck, tempColor, colorOrTexture, materialName });
					}
					else {
						Color tempColor = { newColor.x * 255, newColor.y * 255, newColor.z * 255, 255 };
						materialMaya.insert(materialMaya.begin() + i, { texturePath, tempColor, colorOrTexture, materialName });
					}
				}
			}

			if (exists == false)
			{
				if (colorOrTexture == "color")
				{
					Color tempColor = { newColor.x * 255, newColor.y * 255, newColor.z * 255, 255 };
					std::string textureCheck = "noTexture";
					materialMaya.push_back({ textureCheck, tempColor, colorOrTexture, materialName });
					*nrMaterials = *nrMaterials + 1;
				}
				else {
					Color tempColor = { newColor.x * 255, newColor.y * 255, newColor.z * 255, 255 };
					materialMaya.push_back({ texturePath, tempColor, colorOrTexture, materialName });
					*nrMaterials = *nrMaterials + 1;
				}
			}

			break;
		}
		

		//std::cout << arrayNorm << std::endl;
		

		bool exists = false;
		for (int i = 0; i < *nrObjs; i++) {
			if (objNameArray[i].name == objectName) {
				exists = true;
			}
		}


		if (exists == false) {

			Mesh tempMeshToAdd = {};
			tempMeshToAdd.vertexCount = lengthVtxArr;
			tempMeshToAdd.vertices = new float[lengthVtxArr];
			memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);

			tempMeshToAdd.normals = new float[lengthNormArr];
			memcpy(tempMeshToAdd.normals, arrayNorm, sizeof(float) * lengthNormArr);

			tempMeshToAdd.texcoords = new float[lenghtUVArr];
			memcpy(tempMeshToAdd.texcoords, arrayUV, sizeof(float) * lenghtUVArr);

			rlLoadMesh(&tempMeshToAdd, false);

			Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
			tempModelToAdd.material.shader = shader1;

			Color tempColor = { newColor.x * 255, newColor.y * 255, newColor.z * 255, 255 };

			std::cout << "colorOrTexture2: " << colorOrTexture << std::endl;

			if (colorOrTexture == "color")
			{
				std::cout << "newColor: " << newColor.x << " " << newColor.y << " " << newColor.z << " " << std::endl;
				objNameArray.push_back({ tempModelToAdd, *index, objectName, MatrixTranslate(0,0,0), tempColor, materialName });
			}
			else if (colorOrTexture == "texture")
			{
				std::cout << "adding Texture" << std::endl;
				texture = LoadTexture(texturePath.c_str());
				tempModelToAdd.material.maps[MAP_DIFFUSE].texture = texture;
				objNameArray.push_back({ tempModelToAdd, *index, objectName, MatrixTranslate(0,0,0), {255,255,255,255}, materialName });
			}


			*index = *index + 1;
			*nrObjs = *nrObjs + 1;
		}


		//int lengthString = msgStringForAddMaterial.length() + 1;
		//cstr = new char[lengthString];
		//strcpy(cstr, msgStringForAddMaterial.c_str());
		//std::cout << "cstr:" << cstr << std::endl;

		 
		if (msgElements != NULL) {
			delete[] msgElements;
		}
		if (arrayVtx != NULL) {
			delete[] arrayVtx;
		}
		if (arrayNorm != NULL) {
			delete[] arrayNorm;
		}
		if (arrayUV != NULL) {
			delete[] arrayUV;
		}
		*/
	
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


		int check = 0;

		Vector3 posFromString;
		Vector4 tempLight;

		ss >> posFromString.x >> posFromString.y >> posFromString.z >> tempLight.x >> tempLight.y >> tempLight.z >> tempLight.w;

		//std::cout << "lightPos: " << lightPos[0] << " " << lightPos[1] << " " << lightPos[2] << "lightColor: " << lightColor[0] << " " << lightColor[1] << " " << lightColor[2] << " " << lightColor[3] << std::endl;

		Color tempColor = { tempLight.x * 255, tempLight.y * 255, tempLight.z * 255, tempLight.w * 255 };
		Vector3 tempPos = { posFromString.x, posFromString.y, posFromString.z };

		lightsFromMaya.erase(lightsFromMaya.begin());
		lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });

		if (msgElements != NULL) {
			delete[] msgElements;
		}

	}
	
}

void updateNode(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials)
{
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

		int nrVtx;
		int vtxCheck = 0;
		int nrNorm;
		int normCheck = 0;

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		ss >> nrVtx;

		float* arrayNorm;
		int lengthNormArr = 0;
		float* arrayVtx = new float[nrVtx * 3];
		int lengthVtxArr = 0;

		std::string tempX, tempY, tempZ = "";
		int element = 0;
		float tempFloat = 0.0f;

		std::string tempU, tempV = "";
		int nrUV;
		int UVCheck = 0;
		int lenghtUVArr = 0;
		float* arrayUV;

		int nrOfElements = mesh.trisCount * 3 * 3; //for each tris, add each vtx and then [x,y,z]

		std::string tempNormX, tempNormY, tempNormZ = "";


		while (!ss.eof()) {

			while (vtxCheck < nrVtx)
			{
				ss >> tempX >> tempY >> tempZ;

				if (element >= nrOfElements) {
					std::cout << "Last element fount " << std::endl;
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
				ss >> tempNormX >> tempNormY >> tempNormZ;

				if (element >= nrOfElements) {
					std::cout << "Last element fount " << std::endl;
					break;
				}

				arrayNorm[element] = (float)std::stof(tempNormX);
				arrayNorm[element + 1] = (float)std::stof(tempNormY);
				arrayNorm[element + 2] = (float)std::stof(tempNormZ);

				lengthNormArr = lengthNormArr + 3;
				element = element + 3;

				normCheck++;

			}

			ss >> nrUV;
			std::cout << "nrUV: " << nrUV << std::endl;
			arrayUV = new float[nrUV * 2];
			element = 0;

			while (UVCheck < nrUV)
			{
				std::cout << UVCheck << " UV's" << std::endl;
				ss >> tempU >> tempV;

				if (element >= nrOfElements) {
					//std::cout << "Last element fount " << std::endl;
					break;
				}

				arrayUV[element] = (float)std::stof(tempU);
				arrayUV[element + 1] = 1.0f - (float)std::stof(tempV);

				//std::cout << "TEMP U:" << arrayUV[element] << std::endl;
				//std::cout << "TEMP V:" << arrayUV[element + 1] << std::endl;
				//std::cout << " " << std::endl;

				lenghtUVArr = lenghtUVArr + 2;
				element = element + 2;
				UVCheck++;
			}

			std::cout << "arrayUV: " << std::endl;

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

				tempMeshToAdd.texcoords = new float[lenghtUVArr];
				memcpy(tempMeshToAdd.texcoords, arrayUV, sizeof(float) * lenghtUVArr);

				rlLoadMesh(&tempMeshToAdd, false);

				Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
				tempModelToAdd.material.shader = shader1;
				Color tempColor = objNameArray[i].color;
				Matrix tempMat = objNameArray[i].modelMatrix;
				std::string tempMaterialName = objNameArray[i].materialName;

				std::cout << "mtName: " << tempMaterialName << std::endl;
				objNameArray.erase(objNameArray.begin() + i);

				std::cout << "mtName: " << materialMaya.size() << std::endl;
				for (int i = 0; i < materialMaya.size(); i++)
				{
					std::cout << "materialMaya: " << materialMaya[i].name << std::endl;
					if (materialMaya[i].name == tempMaterialName)
					{
						if (materialMaya[i].ColorOrTexture == "color")
						{
							std::cout << "clor: " << std::endl;
							//objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, tempColor, tempMaterialName });
							tempColor = materialMaya[i].color;
						}
						else if (materialMaya[i].ColorOrTexture == "texture")
						{
							std::cout << "text: " << std::endl;
							//std::cout << "texture stuff: " << texture.height << std::endl;
							Texture2D tempTexture = LoadTexture(materialMaya[i].fileTexuteName.c_str());
							tempModelToAdd.material.maps[MAP_DIFFUSE].texture = tempTexture;
							//objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, {255,255,255,255}, tempMaterialName });
						}
					}
				}

				objNameArray.insert(objNameArray.begin() + i, { tempModelToAdd, tempIndex, objectName, tempMat, tempColor, tempMaterialName });
			}
		}

		if (msgElements != NULL) {
			delete[] msgElements;
		}
		if (arrayVtx != NULL) {
			delete[] arrayVtx;
		}
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

		Vector4 lightColor = { 0, 0, 0, 255 };

		ss >> lightColor.x >> lightColor.y >> lightColor.z >> lightColor.w;

		Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, lightColor.w * 255 };
		Vector3 tempPos = lightsFromMaya[0].lightPos;

		lightsFromMaya.erase(lightsFromMaya.begin());
		lightsFromMaya.insert(lightsFromMaya.begin(), { tempPos, 0.1, tempColor });

		if (msgElements != NULL) {
			delete[] msgElements;
		}
	}
	if (msgHeader.nodeType == NODE_TYPE::CAMERA)
	{
		Vector3 upDir = { 0,0,0 };
		Vector3 viewDir = { 0,0,0 };
		Vector3 pos = { 0,0,0 };
		int type = 0;

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		std::string cameraName;
		ss >> cameraName;


		float fov;

		ss >> upDir.x >> upDir.y >> upDir.z >> viewDir.x >> viewDir.y >> viewDir.z >> pos.x >> pos.y >> pos.z >> fov;

		//std::cout << "camer astuff" << upDir.x << " " << upDir.y << " " << upDir.z << " " << viewDir.x << " " << viewDir.y << " " << viewDir.z << " " << pos.x << " " << pos.y << " " << pos.z << " " << fov << std::endl;

		float FieldOfView = 0;

		if (cameraName == "|persp|perspShape") {
			type = CAMERA_PERSPECTIVE;
			//std::cout << "perspective" << std::endl;
			//radians to degrees
			float degreesFOV = fov * (180.0 / 3.141592653589793238463);
			FieldOfView = degreesFOV;
		}
		else {
			type = CAMERA_ORTHOGRAPHIC;
			//std::cout << "orto" << std::endl; 
			FieldOfView = fov;
		}


		cameraFromMaya.erase(cameraFromMaya.begin());
		cameraFromMaya.insert(cameraFromMaya.begin(), { upDir, viewDir, pos, type, FieldOfView });
	}

	//	std::cout << "============================" << std::endl;
}

void updateNodeMatrix(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials)
{
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	std::cout << "in matrix and printing cmdType " << msgHeader.cmdType << " " << std::endl;
	std::cout << "in matrix and printing node_TYPE " << msgHeader.nodeType << " " << std::endl;

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

		Vector4 row1;
		Vector4 row2;
		Vector4 row3;
		Vector4 row4;

		ss >> row1.x >> row1.y >> row1.z >> row1.w >> row2.x >> row2.y >> row2.z >> row2.w >> row3.x >> row3.y >> row3.z >> row3.w >> row4.x >> row4.y >> row4.z >> row4.w;

		Matrix tempMatrix = { row1.x ,row1.y ,row1.z , row1.w ,
		row2.x ,row2.y ,row2.z ,row2.w ,
		row3.x ,row3.y ,row3.z , row3.w ,
		row4.x ,row4.y ,row4.z, row4.w
		};

		for (int i = 0; i < *nrObjs; i++)
		{
			std::cout << "objName: " << objNameArray[i].name << std::endl;
			std::cout << "tempName: " << objectName << std::endl;
			if (objNameArray[i].name == objectName)
			{
				int tempIndex = objNameArray[i].index;
				Model tempModel = objNameArray[i].model;
				int tempIndex2 = objNameArray[i].index;
				std::string tempName2 = objNameArray[i].name;
				Matrix newModelMatrix = tempMatrix;
				Color tempColor = objNameArray[i].color;
				std::string tempMaterialName = objNameArray[i].materialName;

				objNameArray.erase(objNameArray.begin() + i);
				objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, tempColor, tempMaterialName });
			}
		}

		if (msgElements != NULL) {
			delete[] msgElements;
		}
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

		Vector3 lightPos;

		ss >> lightPos.x >> lightPos.y >> lightPos.z;

		std::cout << lightPos.x << " " << lightPos.y << " " << lightPos.z << " " << std::endl;

		Color tempColor = lightsFromMaya[0].color;

		lightsFromMaya.erase(lightsFromMaya.begin());
		lightsFromMaya.insert(lightsFromMaya.begin(), { lightPos, 0.1, tempColor });

		if (msgElements != NULL) {
			delete[] msgElements;
		}
	}

}

//ipdate material
void updateMaterial(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials)
{
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "MATERIAL COLOR UPDATE" << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		std::string matieralName;
		ss >> matieralName;

		std::cout << "matName from buffer: " << matieralName << std::endl;

		Texture2D texture;
		std::string colorOrTexture;
		ss >> colorOrTexture;

		Vector4 lightColor = { 255,255,255,255 };
		std::string texturePath;

		if (colorOrTexture == "color")
		{
			std::cout << "is color" << std::endl;

			ss >> lightColor.x >> lightColor.y >> lightColor.z;
			//std::cout << "tempColor: " << lightColor[0] << ", " << lightColor[1] << ", " << lightColor[2] << std::endl;
		}
		else if (colorOrTexture == "texture")
		{
			std::cout << "is texture" << std::endl;
			ss >> texturePath;
			std::cout << "texture: " << texturePath << std::endl;
			texture = LoadTexture(texturePath.c_str());
		}

		for (int i = 0; i < *nrMaterials; i++) {
			if (materialMaya[i].name == matieralName) {

				materialMaya.erase(materialMaya.begin() + i);

				if (colorOrTexture == "color")
				{
					Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
					std::string textureCheck = "noTexture";
					materialMaya.insert(materialMaya.begin() + i, { textureCheck, tempColor, colorOrTexture, matieralName });
				}
				else {
					Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
					materialMaya.insert(materialMaya.begin() + i, { texturePath, tempColor, colorOrTexture, matieralName });
				}
			}
			else {
				if (colorOrTexture == "color")
				{
					Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
					std::string textureCheck = "noTexture";
					materialMaya.push_back({ textureCheck, tempColor, colorOrTexture, matieralName });
					*nrMaterials = *nrMaterials + 1;
				}
				else {
					Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
					materialMaya.push_back({ texturePath, tempColor, colorOrTexture, matieralName });
					*nrMaterials = *nrMaterials + 1;
				}
			}
		}

		for (int i = 0; i < *nrObjs; i++)
		{
			if (objNameArray[i].materialName == matieralName)
			{
				std::cout << "matName " << objNameArray[i].materialName << std::endl;

				int tempIndex = objNameArray[i].index;
				Model tempModel = objNameArray[i].model;
				int tempIndex2 = objNameArray[i].index;
				std::string tempName2 = objNameArray[i].name;
				Matrix newModelMatrix = objNameArray[i].modelMatrix;
				Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
				std::string tempMaterialName = objNameArray[i].materialName;

				objNameArray.erase(objNameArray.begin() + i);

				if (colorOrTexture == "color")
				{
					objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, tempColor, tempMaterialName });
				}
				else if (colorOrTexture == "texture")
				{
					std::cout << "texture stuff: " << texture.height << std::endl;
					tempModel.material.maps[MAP_DIFFUSE].texture = texture;
					objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, {255,255,255,255}, tempMaterialName });
				}
			}
		}

		if (msgElements != NULL) {
			delete[] msgElements;
		}
	}
}

//addMaterial
void updateMaterialName(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials)
{
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH)
	{
		std::cout << "MATERIAL" << std::endl;
		std::cout << "meshName: " << objectName << std::endl;
		msgMesh mesh = {};

		memcpy((char*)&mesh, buffer + sizeof(MsgHeader), sizeof(msgMesh));	//mesh struct

		char* msgElements = new char[msgHeader.msgSize];
		memcpy((char*)msgElements, buffer + sizeof(MsgHeader) + sizeof(msgMesh), msgHeader.msgSize);	//copy msg

		// setup stringstream
		std::string msgString(msgElements, msgHeader.msgSize);
		std::istringstream ss(msgString);

		std::cout << "msgString: " << msgString << "__________________________" << std::endl;

		std::string materialName;
		ss >> materialName;

		std::cout << "mat name: " << materialName << std::endl;

		Color newColor = { 255, 255, 255,255 };
		Texture2D texture;
		std::string colorOrTexture;
		ss >> colorOrTexture;

		std::cout << "colorOrTexture: " << colorOrTexture << std::endl;
		Vector4 lightColor = { 255,255,255,255 };
		std::string texturePath;

		if (colorOrTexture == "color")
		{

			std::cout << "is color" << std::endl;

			ss >> lightColor.x >> lightColor.y >> lightColor.z;
			//std::cout << "tempColor: " << lightColor[0] << ", " << lightColor[1] << ", " << lightColor[2] << std::endl; 

		}
		else if (colorOrTexture == "texture")
		{
			std::cout << "is texture" << std::endl;
			ss >> texturePath;
			std::cout << "texture: " << texturePath << std::endl;
			texture = LoadTexture(texturePath.c_str());
		}

		bool exists = false;
		for (int i = 0; i < *nrMaterials; i++) {
			if (materialMaya[i].name == materialName) {
				exists = true;
				materialMaya.erase(materialMaya.begin() + i);

				if (colorOrTexture == "color")
				{
					Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
					std::string textureCheck = "noTexture";
					materialMaya.insert(materialMaya.begin() + i, { textureCheck, tempColor, colorOrTexture, materialName });
				}
				else {
					Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
					materialMaya.insert(materialMaya.begin() + i, { texturePath, tempColor, colorOrTexture, materialName });
				}
			}
		}


		if (exists == false)
		{
			if (colorOrTexture == "color")
			{
				Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
				std::string textureCheck = "noTexture";
				materialMaya.push_back({ textureCheck, tempColor, colorOrTexture, materialName });
				*nrMaterials = *nrMaterials + 1;
			}
			else {
				Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
				materialMaya.push_back({ texturePath, tempColor, colorOrTexture, materialName });
				*nrMaterials = *nrMaterials + 1;
			}
		}

		for (int i = 0; i < *nrObjs; i++)
		{
			if (objNameArray[i].name == objectName)
			{
				int tempIndex = objNameArray[i].index;
				Model tempModel = objNameArray[i].model;
				int tempIndex2 = objNameArray[i].index;
				std::string tempName2 = objNameArray[i].name;
				Matrix newModelMatrix = objNameArray[i].modelMatrix;
				Color tempColor = { lightColor.x * 255, lightColor.y * 255, lightColor.z * 255, 255 };
				std::string tempMaterialName = materialName;

				std::cout << "matName: " << materialName << std::endl;

				objNameArray.erase(objNameArray.begin() + i);

				if (colorOrTexture == "color")
				{
					objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, tempColor, tempMaterialName });
					std::cout << "tempColor: " << tempColor.r << ", " << tempColor.g << ", " << tempColor.b << std::endl;
				}
				else if (colorOrTexture == "texture")
				{
					std::cout << "texture stuff: " << texture.height << std::endl;
					tempModel.material.maps[MAP_DIFFUSE].texture = texture;
					objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex2, tempName2, newModelMatrix, {255,255,255,255}, tempMaterialName });
				}
			}
		}

		if (msgElements != NULL) {
			delete[] msgElements;
		}
	}
}

void updateNodeName(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, std::vector<lightFromMaya>& lightsFromMaya, std::vector<cameraFromMaya>& cameraFromMaya, std::vector<materialMaya>& materialMaya, int* nrMaterials)
{
	// Get elements from buffer
	MsgHeader msgHeader = {};
	std::string objectName = "";
	memcpy((char*)&msgHeader, buffer, sizeof(MsgHeader));
	objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	std::cout << "oldName: " << objectName << "_" << std::endl;

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


		std::cout << "newName: " << newName << "_" << std::endl;

		for (int i = 0; i < *nrObjs; i++)
		{
			if (objNameArray[i].name == objectName)
			{
				int tempIndex = objNameArray[i].index;
				Model tempModel = objNameArray[i].model;
				Matrix tempMatrix = objNameArray[i].modelMatrix;
				Color tempColor = objNameArray[i].color;
				std::string tempMaterialName = objNameArray[i].materialName;

				objNameArray.erase(objNameArray.begin() + i);
				objNameArray.insert(objNameArray.begin() + i, { tempModel, tempIndex, newName, tempMatrix, tempColor, tempMaterialName });
			}
		}

		if (msgElements != NULL) {
			delete[] msgElements;
		}
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
