#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

#include <sstream>
#include <map>
#include <functional>

#include <Windows.h>

//#include <C:\Users\BTH\Desktop\shared\Project1\ComLib.h>
#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>

#pragma comment(lib, "Project1.lib")

// ===========================================
struct modelFromMaya
{
	Model model;
	int index;
	std::string name;
	Matrix modelMatrix;
};

struct modelPos { 
	Model model; 
	Matrix modelMatrix; 
};

// ===========================================

void addModel(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index);
void updateModel(std::vector<modelFromMaya>& objNameArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index);

typedef void(FnPtr)(std::vector<modelFromMaya>&, char*, int, Shader, int*, int*);
// ===========================================
std::string recvFromMaya(char* buffer);

// ===========================================
ComLib comLib("sharedBuff", 100, CONSUMER); 

// ===========================================
int main() {

	SetTraceLog(LOG_WARNING);

	// OUR STUFF ====================================
	std::vector<modelFromMaya> modelsFromMaya;

	std::map<std::string, FnPtr> funcMap;
	//funcMap["addModel"]	   = addModel;
	//funcMap["updateModel"] = updateModel;

	Model cube;
	int modelIndex = 0;
	int nrOfObj	   = 0;
	size_t tempArraySize = 0; 
	// =============================================

    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth  = 1200;
    int screenHeight = 800;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)
	SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "demo");

    // Define the camera to look into our 3d world
    Camera camera   = { 0 };
    camera.position = { 4.0f, 4.0f, 4.0f };
    camera.target   = { 0.0f, 1.0f, -1.0f };
    camera.up   = { 0.0f, 1.0f, 0.0f };
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

	Mesh mesh1	 = LoadMesh("resources/models/watermill.obj");
    Model model1 = LoadModelFromMesh(mesh1);                   
    model1.material = material1;                     // Set shader effect to 3d model


	int sometingToRead		= false;
	bool checkIfModelLoaded = false;
	bool isNextMsgSometing  = false;


	// triangle by hand
	float vtx[3*3]{ 
		1, 0, 1,
		1, 0, 0,
		0, 0, 1 
	};

	float norm[9] { 0, 1, 0, 0, 1, 0, 0, 1, 0};
	unsigned short indices[3] = { 0, 1, 2 }; // remove to use vertices only
	unsigned char colors[12]  = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };

	std::cout << "-----------------" << std::endl;

    Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
    SetCameraMode(camera, CAMERA_FREE);         // Set an orbital camera mode
    SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second

	int modelLoc = GetShaderLocation(shader1, "model");
	int viewLoc  = GetShaderLocation(shader1, "view");
	int projLoc  = GetShaderLocation(shader1, "projection");

	int lightLoc = GetShaderLocation(shader1, "lightPos");
	Vector3 lightPos = {4,1,4};


	Mesh mesh2 = {};
	mesh2.vertexCount   = 3;
	mesh2.triangleCount = 1;
	mesh2.vertices = new float[9]; 
	mesh2.normals  = new float[9];
	mesh2.indices  = new unsigned short[3]; // remove to use vertices only
	mesh2.colors   = new unsigned char[12];

	memcpy(mesh2.vertices, vtx, sizeof(float)*9);
	memcpy(mesh2.normals, norm, sizeof(float)*9);
	memcpy(mesh2.indices, indices, sizeof(unsigned short)*3); // remove to use vertices only
	memcpy(mesh2.colors, colors, sizeof(unsigned char)*12); // remove to use vertices only

	rlLoadMesh(&mesh2, false);

	Model tri = LoadModelFromMesh(mesh2);
	tri.material.shader = shader1;
	flatScene.push_back({tri, MatrixTranslate(2,0,2)});

    

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
    while (!WindowShouldClose())                // Detect window close button or ESC key
    {
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

			if (IsKeyPressed(KEY_D))
			{
				Vector3 pos = { (rand() / (float)RAND_MAX) * 10.0f, 0.0f, (rand() / (float)RAND_MAX) * 10.0f };

				static int i = 0;

				// matrices are transposed later, so order here would be [Translate*Scale*vector]
				Matrix modelmatrix = MatrixMultiply( MatrixMultiply( 
					MatrixRotate({0,1,0}, i / (float)5), 
					MatrixScale(0.1,0.1,0.1)),
					MatrixTranslate(pos.x,pos.y,pos.z));

				Model copy = model1;
				flatScene.push_back({copy, modelmatrix});
				i++;
			}

            BeginMode3D(camera);
			// ===================================

			///////////////////////// TEST FOR DRAW MODEL UNDER RUNTIME
			tempArraySize   = comLib.nextSize();
			char* tempArray = new char[tempArraySize];

			std::string funcToCall = "";
			funcToCall = recvFromMaya(tempArray);

			if (funcToCall != "") {
				//funcMap[funcToCall](modelsFromMaya, tempArray, tempArraySize, shader1, &nrOfObj, &modelIndex);
			}

			int i = 0;
			i++;
			//////////////////////////////

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

			DrawSphere(lightPos, 0.1,RED);

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

	UnloadModel(cube);

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
			if (check == 0)	{
				arrayVtx[i] = (float)std::stof(temp);
				lengthVtxArray++;
			}

			if (check == -1) {
				tempName.append(temp);
			}

			i++;
			temp = "";
		}

		if (std::stringstream(temp) >> examplechar)	{

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