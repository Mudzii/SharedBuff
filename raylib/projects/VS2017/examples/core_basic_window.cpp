#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <map>
#include <sstream>
#include <iostream>
#include <functional>

#include <Windows.h>

//#include <C:\Users\BTH\Desktop\shared\Project1\ComLib.h>
#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>

#pragma comment(lib, "Project1.lib")

// ===========================================
enum CMDTYPE {
	DEFAULT = 1000,
	NEW_NODE = 1001,
	VTX = 1002

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
	CMDTYPE type;
	int nrOf;
	int nameLen;
	int msgSize;
	char objName[64];
};

// ===========================================

void addModel(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index);
void updateModel(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index);


void addModel2(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index);
void updateModel2(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index);

typedef void(*FnPtr)(std::vector<modelFromMaya>&, char*, int, Shader, int*, int*);
// ===========================================
std::string recvFromMaya(char* buffer);

CMDTYPE recvFromMaya2(char* buffer);

// ===========================================
ComLib comLib("sharedBuff2", 100, CONSUMER);

// ===========================================
int main() {

	SetTraceLog(LOG_WARNING);

	// OUR STUFF ====================================
	std::vector<modelFromMaya> modelsFromMaya;

	std::map<CMDTYPE, FnPtr> funcMap;
	funcMap[NEW_NODE] = addModel2;
	funcMap[VTX]	  = updateModel2;

	/*std::map<std::string, FnPtr> funcMap;
	funcMap["addModel"]	   = addModel;
	funcMap["updateModel"] = updateModel;*/

	Model cube;
	int modelIndex = 0;
	int nrOfObj = 0;
	size_t tempArraySize = 0;

	int sometingToRead = false;
	bool checkIfModelLoaded = false;
	bool isNextMsgSometing = false;
	// =============================================

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

	// triangle by hand
	float vtx[3 * 3]{
		1, 0, 1,
		1, 0, 0,
		0, 0, 1
	};

	float norm[9]{ 0, 1, 0, 0, 1, 0, 0, 1, 0 };
	unsigned short indices[3] = { 0, 1, 2 }; // remove to use vertices only
	unsigned char colors[12] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };

	std::cout << "-----------------" << std::endl;

	Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
	SetCameraMode(camera, CAMERA_FREE);         // Set an orbital camera mode
	SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second

	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc = GetShaderLocation(shader1, "view");
	int projLoc = GetShaderLocation(shader1, "projection");

	int lightLoc = GetShaderLocation(shader1, "lightPos");
	Vector3 lightPos = { 4,1,4 };


	Mesh mesh2 = {};
	mesh2.vertexCount = 3;
	mesh2.triangleCount = 1;
	mesh2.vertices = new float[9];
	mesh2.normals = new float[9];
	mesh2.indices = new unsigned short[3]; // remove to use vertices only
	mesh2.colors = new unsigned char[12];

	memcpy(mesh2.vertices, vtx, sizeof(float) * 9);
	memcpy(mesh2.normals, norm, sizeof(float) * 9);
	memcpy(mesh2.indices, indices, sizeof(unsigned short) * 3); // remove to use vertices only
	memcpy(mesh2.colors, colors, sizeof(unsigned char) * 12); // remove to use vertices only

	rlLoadMesh(&mesh2, false);

	Model tri = LoadModelFromMesh(mesh2);
	tri.material.shader = shader1;
	flatScene.push_back({ tri, MatrixTranslate(2,0,2) });



	// TEST CUBE ===========================

	/*int nr = comLib.nextSize();
	float* arrayVtx = new float[nr]();
	int lengthVtxArray = 0;
	float* arrayVtx2;

	recvFromMaya(arrayVtx, nr, &lengthVtxArray);*/

	//std::cout << "lengthVtxArray: " << lengthVtxArray << std::endl;

	/*for (int i = 0; i < lengthVtxArray; i++) {
		std::cout << "array[i]: " << arrayVtx[i] << std::endl;
	}*/


	// Main game loop
	while (!WindowShouldClose()) {               // Detect window close button or ESC key

		// Update
		//----------------------------------------------------------------------------------
		UpdateCamera(&camera);                  // Update camera
		//----------------------------------------------------------------------------------
		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		SetShaderValueMatrix(shader1, viewLoc, GetCameraMatrix(camera));
		SetShaderValue(shader1, lightLoc, Vector3ToFloat(lightPos), 3);

		ClearBackground(RAYWHITE);

		if (IsKeyPressed(KEY_D)) {
			Vector3 pos = { (rand() / (float)RAND_MAX) * 10.0f, 0.0f, (rand() / (float)RAND_MAX) * 10.0f };

			static int i = 0;

			// matrices are transposed later, so order here would be [Translate*Scale*vector]
			Matrix modelmatrix = MatrixMultiply(MatrixMultiply(
				MatrixRotate({ 0,1,0 }, i / (float)5),
				MatrixScale(0.1, 0.1, 0.1)),
				MatrixTranslate(pos.x, pos.y, pos.z));

			Model copy = model1;
			//flatScene.push_back({copy, modelmatrix});
			i++;
		}

		BeginMode3D(camera);
		// ===================================

		///////////////////////// TEST FOR DRAW MODEL UNDER RUNTIME
		tempArraySize = comLib.nextSize();
		char* tempArray = new char[tempArraySize];

		CMDTYPE type = CMDTYPE::DEFAULT;
		type = recvFromMaya2(tempArray);

		if (type != CMDTYPE::DEFAULT) {

			//std::cout << "TYPE: " << type << std::endl;
			funcMap[type](modelsFromMaya, tempArray, tempArraySize, shader1, &nrOfObj, &modelIndex);
		}

		/*
		std::string funcToCall = "";
		funcToCall = recvFromMaya(tempArray);

		if (funcToCall != "") {
			funcMap[funcToCall](modelsFromMaya, tempArray, tempArraySize, shader1, &nrOfObj, &modelIndex);
		}
		*/

		int i = 0;
		i++;
		//////////////////////////////

		//std::cout << "modelsFromMaya.size(): " << modelsFromMaya.size() << std::endl;

		for (int i = 0; i < modelsFromMaya.size(); i++) {
			auto m = modelsFromMaya[i];
			auto l = GetShaderLocation(m.model.material.shader, "model");
			SetShaderValueMatrix(m.model.material.shader, modelLoc, m.modelMatrix);
			DrawModel(m.model, {}, 1.0, RED);
		}


		/*	nr = comLib.nextSize();
		arrayVtx = new float[nr];
		lengthVtxArray = 0;
		recvFromMaya(arrayVtx, nr, &lengthVtxArray);*/

		/*
		int nr2 = comLib.nextSize();
		arrayVtx2 = new float[nr2]();
		int lengthVtxArray2 = 0;


		sometingToRead = recvFromMaya(arrayVtx2, nr2, &lengthVtxArray2);
		if (sometingToRead == true) {

			if (checkIfModelLoaded == true)	{
				checkIfModelLoaded = false;
				cubeExists = false;
				flatScene.pop_back();

			}

			if(checkIfModelLoaded == false) {

			Mesh cubeTest2 = {};
			cubeTest2.vertexCount = lengthVtxArray2;
			cubeTest2.vertices = new float[lengthVtxArray2];
			memcpy(cubeTest2.vertices, arrayVtx2, sizeof(float) * cubeTest2.vertexCount);
			rlLoadMesh(&cubeTest2, false);

			cube = LoadModelFromMesh(cubeTest2);
			cube.material.shader = shader1;
			flatScene.push_back({ cube, MatrixTranslate(1,0,1) });
			checkIfModelLoaded = true;
			cubeExists = true;

			/*for (int i = 0; i < cubeTest2.vertexCount; i++) {

				std::cout << "VTX: " << cubeTest2.vertices[i]  << std::endl;
			}
			std::cout << "==================================" << std::endl;

			}
		}
		*/

		// ===================================
		/*
		for (int i = 0; i < flatScene.size(); i++)
		{
			auto m = flatScene[i];
			auto l = GetShaderLocation(m.model.material.shader, "model");
			SetShaderValueMatrix( m.model.material.shader, modelLoc, m.modelMatrix);
			DrawModel(m.model,{}, 1.0, RED);
		}
		*/
		DrawGrid(10, 1.0f);     // Draw a grid

		DrawSphere(lightPos, 0.1, RED);

		EndMode3D();

		DrawTextRL("(c) Watermill 3D model by Alberto Cano", screenWidth - 210, screenHeight - 20, 10, GRAY);
		DrawTextRL(FormatText("Camera position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z), 600, 20, 10, BLACK);
		DrawTextRL(FormatText("Camera target: (%.2f, %.2f, %.2f)", camera.target.x, camera.target.y, camera.target.z), 600, 40, 10, GRAY);
		DrawFPS(10, 10);

		EndDrawing();
		delete[] tempArray;
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	UnloadShader(shader1);       // Unload shader
	UnloadTexture(texture1);     // Unload texture
	UnloadModel(model1);
	UnloadModel(tri);

	//UnloadModel(cube);

	CloseWindowRL();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------


	return 0;
}

std::string recvFromMaya(char* buffer) {

	std::string lineOut = "";

	size_t nr = comLib.nextSize();
	size_t oldBuffSize = nr;

	char* buff = (char*)malloc(oldBuffSize);

	if (nr > oldBuffSize) {
		free(buff);
		buff = (char*)malloc(nr);
		oldBuffSize = nr;
	}

	bool test = comLib.recv(buff, nr);
	if (test == true) {

		std::string reference;
		std::string testString(buff, nr);
		std::stringstream stringStream;
		stringStream << testString;

		std::string temp;
		stringStream >> temp;
		if (std::stringstream(temp) >> reference) {
			lineOut = temp;
			std::cout << lineOut << std::endl;
		}
	}

	memcpy(buffer, buff, nr);
	free(buff);
	return lineOut;
}

void addModel(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index)
{
	float exampleInt;	//use to find int type in string array
	char examplechar;
	float* arrayVtx = new float[bufferSize];
	int lengthVtxArray = 0;
	std::string tempName = "";

	int check = -1;

	std::string testString(buffer, bufferSize);
	std::stringstream stringStream;
	stringStream << testString;

	std::string reference;
	std::string temp;

	stringStream >> temp;
	if (std::stringstream(temp) >> reference) {
		//is command, discard
	}

	int i = 0;
	while (!stringStream.eof()) {

		stringStream >> temp;
		if (std::stringstream(temp) >> exampleInt) {
			if (check == 0) {
				arrayVtx[i] = (float)std::stof(temp);
				lengthVtxArray++;
			}

			if (check == -1) {
				tempName.append(temp);
			}

			i++;
			temp = "";
		}

		if (std::stringstream(temp) >> examplechar) {

			if (examplechar == '|') {
				check = 0;
				i = 0;
			}

			if (check == -1) {
				tempName.append(temp);
			}
		}
	}

	bool exists = false;
	for (int i = 0; i < *nrObjs; i++) {
		if (objNameArray[i].name == tempName) {
			exists = true;
		}
	}

	if (exists == false) {
		Mesh tempMeshToAdd = {};
		tempMeshToAdd.vertexCount = lengthVtxArray;
		tempMeshToAdd.vertices = new float[lengthVtxArray];
		memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);
		rlLoadMesh(&tempMeshToAdd, false);

		Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
		tempModelToAdd.material.shader = shader1;

		objNameArray.push_back({ tempModelToAdd, *index, tempName, MatrixTranslate(2,0,2) });
		*index = *index + 1;
		*nrObjs = *nrObjs + 1;
	}
}

void updateModel(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index)
{
	float exampleInt;	//use to find int type in string array
	char examplechar;
	float* arrayVtx = new float[bufferSize];
	int lengthVtxArray = 0;
	std::string tempName = "";

	int check = -1;

	std::string testString(buffer, bufferSize);
	std::stringstream stringStream;
	stringStream << testString;

	std::string reference;
	std::string temp;

	stringStream >> temp;
	if (std::stringstream(temp) >> reference)
	{
		//is command, discard
	}

	int i = 0;
	while (!stringStream.eof())
	{
		stringStream >> temp;
		if (std::stringstream(temp) >> exampleInt)
		{
			if (check == 0)
			{
				arrayVtx[i] = (float)std::stof(temp);
				lengthVtxArray++;
			}

			if (check == -1)
			{
				tempName.append(temp);
			}
			i++;
			temp = "";
		}
		if (std::stringstream(temp) >> examplechar)
		{
			if (examplechar == '|')
			{
				check = 0;
				i = 0;
			}

			if (check == -1)
			{
				tempName.append(temp);
			}
		}
	}

	for (int i = 0; i < *nrObjs; i++)
	{
		if (objNameArray[i].name == tempName)
		{
			int tempIndex = objNameArray[i].index;

			Mesh tempMeshToAdd = {};
			tempMeshToAdd.vertexCount = lengthVtxArray;
			tempMeshToAdd.vertices = new float[lengthVtxArray];
			memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);
			rlLoadMesh(&tempMeshToAdd, false);

			Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
			tempModelToAdd.material.shader = shader1;

			objNameArray.erase(objNameArray.begin() + i);
			objNameArray.insert(objNameArray.begin() + i, { tempModelToAdd, tempIndex, tempName, MatrixTranslate(2,0,2) });
		}
	}
}


// TEST===========================================
CMDTYPE recvFromMaya2(char* buffer) {

	std::string lineOut = "";
	CMDTYPE type = CMDTYPE::DEFAULT;

	MsgHeader msgHeader = {};

	size_t nr = comLib.nextSize();
	size_t oldBuffSize = nr;

	char* buff = (char*)malloc(oldBuffSize);


	std::string objectName = "";

	if (nr > oldBuffSize) {
		free(buff);
		buff = (char*)malloc(nr);
		oldBuffSize = nr;
	}

	bool test = comLib.recv(buff, nr);
	if (test == true) {
		memcpy(buffer, buff, nr);
		memcpy((char*)&msgHeader, buff, sizeof(MsgHeader));


		char* msg = new char[msgHeader.msgSize];
		memcpy((char*)msg, buff + sizeof(MsgHeader), msgHeader.msgSize);

		//char* testMsg = new char[172]; 
		//memcpy((char*)&testMsg, buff, 171);

		std::cout << "header type: " << msgHeader.type << std::endl;
		std::cout << "header nrOf: " << msgHeader.nrOf << std::endl;
		std::cout << "header objName: " << msgHeader.objName << std::endl;
		std::cout << "header nameLen: " << msgHeader.nameLen << std::endl;
		std::cout << "header nameLen: " << msgHeader.msgSize << std::endl;

		objectName = msgHeader.objName;
		objectName = objectName.substr(0, msgHeader.nameLen);
		std::cout << "objectName: " << objectName << std::endl;
		std::cout << "MSG: " << msg << std::endl;

		//std::cout << "testMsg: " << testMsg << std::endl;

		/*

		int typeInt = 0;

		std::string msgString(buff, nr);
		std::stringstream ss(msgString);
		ss >> typeInt;

		if (MSGTYPE::VTX == typeInt) {
			type = MSGTYPE::VTX;

		}

		else if (MSGTYPE::NEW_NODE == typeInt) {
			type = MSGTYPE::NEW_NODE;
		}
		*/

		//free(testMsg); 
	}

	free(buff);

	return type;
}

void addModel2(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index) {

	MsgHeader header;

	std::cout << "==================================" << std::endl;
	std::cout << "IN ADD MODEL" << std::endl;

	int check = -1;

	int typeInt = 0;
	int nrOfElements = 0;

	float* arrayVtx = new float[bufferSize];
	int lengthVtxArray = 0;

	std::string msgString(buffer, bufferSize);
	std::istringstream ss(msgString);

	std::string objName = "";
	std::string temp;


	//get header elements
	if (check == -1) {
		ss >> typeInt >> header.nrOf >> header.nameLen >> header.objName;

		if (typeInt == CMDTYPE::NEW_NODE)
			header.type = CMDTYPE::NEW_NODE;

		std::cout << "TYPE: " << header.type << std::endl;
		std::cout << "NR OF: " << header.nrOf << std::endl;
		std::cout << "objName: " << header.objName << std::endl;
		std::cout << "nameLen: " << header.nameLen << std::endl;

		objName = header.objName;
		objName = objName.substr(0, header.nameLen);
		std::cout << "objName: " << objName << std::endl;


		check = 0;
	}

	int element = 0;
	float tempFloat = 0.0f;
	nrOfElements = header.nrOf * 3 * 2; //for each vtx * [x,y,z]

	while (!ss.eof()) {

		// get x y z ?? 
		ss >> temp;

		if (element >= nrOfElements) {
			check = 1;
			std::cout << "Last element fount " << std::endl;
			break;

		}

		if (std::stringstream(temp) >> tempFloat) {
			std::cout << "TEMP: " << temp << std::endl;
			arrayVtx[element] = (float)std::stof(temp);

			lengthVtxArray++;
			element++;
		}

	}


	bool exists = false;
	for (int i = 0; i < *nrObjs; i++) {
		if (objNameArray[i].name == objName) {
			exists = true;
		}
	}

	if (exists == false) {

		Mesh tempMeshToAdd = {};
		tempMeshToAdd.vertexCount = lengthVtxArray;
		tempMeshToAdd.vertices = new float[lengthVtxArray];
		memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);
		rlLoadMesh(&tempMeshToAdd, false);

		Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
		tempModelToAdd.material.shader = shader1;

		objNameArray.push_back({ tempModelToAdd, *index, objName, MatrixTranslate(2,0,2) });
		*index = *index + 1;
		*nrObjs = *nrObjs + 1;
	}

}

void updateModel2(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader1, int* nrObjs, int* index) {

	MsgHeader header;

	//std::cout << "==================================" << std::endl;
	//std::cout << "IN UPDATE MODEL" << std::endl;

	int check = -1;

	int typeInt = 0;
	int nrOfElements = 0;

	float* arrayVtx = new float[bufferSize];
	int lengthVtxArray = 0;

	std::string msgString(buffer, bufferSize);
	std::istringstream ss(msgString);

	std::string objName = "";
	std::string temp;

	memcpy((char*)&header, buffer, sizeof(MsgHeader));

	std::cout << "header type: " << header.type << std::endl;

	//get header elements
	if (check == -1) {
		ss >> typeInt >> header.nrOf >> header.nameLen >> header.objName;


		if (typeInt == CMDTYPE::VTX)
			header.type = CMDTYPE::VTX;

		std::cout << "TYPE: " << header.type << std::endl;
		std::cout << "NR OF: " << header.nrOf << std::endl;
		std::cout << "objName: " << header.objName << std::endl;
		std::cout << "nameLen: " << header.nameLen << std::endl;

		objName = header.objName;
		objName = objName.substr(0, header.nameLen);
		std::cout << "objName: " << objName << std::endl;

		check = 0;
	}

	int element = 0;
	float tempFloat = 0.0f;
	nrOfElements = header.nrOf * 3 * 2; //for each vtx * [x,y,z]

	while (!ss.eof()) {

		ss >> temp;
		//std::cout << "TEMP: " << temp << std::endl;

		if (element >= nrOfElements) {
			check = 1;
			//std::cout << "Last element fount " << std::endl;
			break;

		}

		if (std::stringstream(temp) >> tempFloat) {
			arrayVtx[element] = (float)std::stof(temp);

			lengthVtxArray++;
			element++;
		}

	}


	for (int i = 0; i < *nrObjs; i++) {

		if (objNameArray[i].name == objName) {

			int tempIndex = objNameArray[i].index;

			Mesh tempMeshToAdd = {};
			tempMeshToAdd.vertexCount = lengthVtxArray;
			tempMeshToAdd.vertices = new float[lengthVtxArray];
			memcpy(tempMeshToAdd.vertices, arrayVtx, sizeof(float) * tempMeshToAdd.vertexCount);
			rlLoadMesh(&tempMeshToAdd, false);

			Model tempModelToAdd = LoadModelFromMesh(tempMeshToAdd);
			tempModelToAdd.material.shader = shader1;

			objNameArray.erase(objNameArray.begin() + i);
			objNameArray.insert(objNameArray.begin() + i, { tempModelToAdd, tempIndex, objName, MatrixTranslate(2,0,2) });
		}
	}

}


/*
bool recvFromMaya(float* arrayFloat, int sizeOfArray, int* nrOut) {

	bool result = false;

	size_t nr = comLib.nextSize();

	size_t oldBuffSize = nr;
	char* buff = (char*)malloc(oldBuffSize);

	if (nr > oldBuffSize){
		free(buff);
		buff = (char*)malloc(nr);
		oldBuffSize = nr;
	}

	bool test = comLib.recv(buff, nr);
	if (test == true){

		if (test == true) {
			std::cout << std::string((char*)buff, nr) << std::endl;
		}

		else {
			buff = "notWorking";
		}

		float exampleFloat;	//use to find int type in string array
		char examplechar;

		float* arrayVtx = new float[nr]();
		int lengthVtxArray = 0;

		//int* arrayIndex = new int[nr]();
		//int lengthIndexArray = 0;

		//int check = -1;

		//std::cout << "nr: " << nr << std::endl;

		std::string testString(buff, nr);
		std::stringstream stringStream;
		stringStream << testString;
		//std::cout << "testStrubg: " << testString << std::endl;

		int i = 0;
		std::string temp;

		while (!stringStream.eof())
		{
			stringStream >> temp;
			if (std::stringstream(temp) >> exampleFloat)
			{
				//std::cout << i << std::endl;
				arrayVtx[i] = (float)std::stof(temp);
				//std::cout << "arrayVtx: " << arrayVtx[i] << std::endl;
				lengthVtxArray++;

				//if (check == 0)
				//{
				//	arrayIndex[i] = (int)std::stoi(temp);
				//	//std::cout << "arrayIndex: " << arrayIndex[i] << std::endl;
				//	lengthIndexArray++;
				//}

				i++;
				temp = "";

			}
			//if (std::stringstream(temp) >> examplechar)
			//{
			//	if (examplechar == 'a')
			//	{
			//		check = 0;
			//		//std::cout << "found a!!" << std::endl;
			//		i = 0;
			//	}
			//}
		}

		float* arrayVtx2 = new float[lengthVtxArray];
		int* arrayIndex2 = new int[lengthIndexArray];

		for (int i = 0; i < lengthVtxArray; i++)
		{
			arrayVtx2[i] = arrayVtx[i];
			std::cout << "arrayvtx: " << arrayVtx2[i] << std::endl;
		}
		delete[] arrayVtx;

		for (int i = 0; i < lengthIndexArray; i++)
		{
			arrayIndex2[i] = arrayIndex[i];
			std::cout << "arrayIndex: " << arrayIndex2[i] << std::endl;
		}
		delete[] arrayIndex;

		std::cout << "arrayVtxLength: " << lengthVtxArray << std::endl;
		//std::cout << "lengthVtxArray: " << lengthVtxArray << std::endl;

		for (int i = 0; i < lengthVtxArray; i++) {
			arrayFloat[i] = arrayVtx[i];
		}

		*nrOut = lengthVtxArray;
		 result = true;
		delete[] arrayVtx;
	}

	free(buff);
	return result;
}
*/