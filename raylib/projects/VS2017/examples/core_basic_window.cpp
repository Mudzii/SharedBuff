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
size_t oldBuffSize = 50; 

#include <Windows.h>

// ==================================================================================

void recvFromMaya(std::map<CMDTYPE, FnPtr> functionMap, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray);

// ==================================================================================
// ==================================================================================
// ================================= MAIN ===========================================
// ==================================================================================
// ==================================================================================

int main() {
	SetTraceLog(LOG_WARNING);

	// vectors with objects from Maya ====== 
	std::vector<modelFromMaya> modelArray;
	std::vector<lightFromMaya> lightsArray;
	std::vector<cameraFromMaya> cameraArray;
	std::vector<materialFromMaya> materialArray;

	// create a light ====== 
	lightFromMaya tempLight   = {};
	tempLight.lightNameLen    = 5; 
	tempLight.intensityRadius = 0.1;
	tempLight.lightPos		  = { 0,0,0 };
	tempLight.color			  = { 1 * 255, 1 * 255, 1 * 255, 1 * 255 };
	memcpy(tempLight.lightName, "light", tempLight.lightNameLen);
	

	// map specific commands to a specific function
	std::map<CMDTYPE, FnPtr> funcMap;

	funcMap[NEW_NODE]			  = addNode; 
	funcMap[UPDATE_NODE]		  = updateNode;
	funcMap[DELETE_NODE]		  = deleteNode;
	funcMap[UPDATE_CAMERA]		  = updateCamera;
	funcMap[UPDATE_NAME]		  = updateNodeName;
	funcMap[UPDATE_MATRIX]		  = updateNodeMatrix;

	//funcMap[NEW_MATERIAL]	      = newMaterial;
	//funcMap[UPDATE_MATERIAL]      = updateMaterial;
	//funcMap[UPDATE_NODE_MATERIAL] = updateNodeMaterial; 
	/* 
	int modelIndex  = 0;
	// create a simple cube
	//Model cube;
	//int nrOfObj		= 0;
	//int nrMaterials = 0;
	//size_t tempArraySize = 0;
	*/

	
	// Initialization
	//--------------------------------------------------------------------------------------
	int screenWidth  = 1200;
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

	Shader shader1 = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");  

	/* 
	// real models rendered each frame
	std::vector<modelPos> flatScene;

	// inits from rayLib
	Material material1 = LoadMaterialDefault();
	Texture2D texture1 = LoadTexture("resources/models/watermill_diffuse.png");
	material1.maps[MAP_DIFFUSE].texture = texture1;

	material1.shader = shader1;


	Mesh mesh1   = LoadMesh("resources/models/watermill.obj");
	Model model1 = LoadModelFromMesh(mesh1);
	model1.material = material1;                     // Set shader effect to 3d model

	unsigned char colors[12] = { 255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255 };
	*/
	

	//std::cout << "-----------------" << std::endl;
	Vector3 position = { 0.0f, 0.0f, 0.0f };     // Set model position
	SetCameraMode(camera, CAMERA_FREE);          // Set an orbital camera mode
	SetTargetFPS(140);                           // Set our game to run at 60 frames-per-second
	
	//int modelLoc = GetShaderLocation(shader1, "model");
	//int viewLoc  = GetShaderLocation(shader1, "view");
	//int projLoc  = GetShaderLocation(shader1, "projection");
	int lightLoc = GetShaderLocation(shader1, "lightPos");

	// MAIN GAME LOOP =====================================
	// Detect window close button or ESC key
	while (!WindowShouldClose()) {

		// Update
		//----------------------------------------------------------------------------------
		//UpdateCamera(&camera);                  // Update camera
		//----------------------------------------------------------------------------------
		// Draw
		//----------------------------------------------------------------------------------
		
		// Get the messages sent from maya
		recvFromMaya(funcMap, modelArray, lightsArray, cameraArray, materialArray);
		
		BeginDrawing();

		// set the camera
		if (cameraArray.size() > 0) {

			camera.up   = cameraArray[0].up;
			camera.fovy = cameraArray[0].fov;
			camera.type = cameraArray[0].type;
			camera.position = cameraArray[0].pos;
			camera.target	= cameraArray[0].forward;
		}
		
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


		// To draw objects, add them after this line
		BeginMode3D(camera);

		// draw the light
		DrawSphere(tempLight.lightPos, 0.1, tempLight.color);
		SetShaderValue(shader1, lightLoc, Vector3ToFloat(tempLight.lightPos), 1);

		/* 

		// draw lights from maya
		for (int i = 0; i < lightsArray.size(); i++) {
			DrawSphere(lightsArray[i].lightPos, 0.1, lightsArray[i].color);

			
			for (int j = 0; j < shaderArray.size(); j++) {
				int lightLocTemp = GetShaderLocation(shaderArray[j], "lightPos");
				SetShaderValue(shaderArray[j], lightLocTemp, Vector3ToFloat(lightsArray[i].lightPos), 1);
			}
		

		}
		*/
			
		
		// draw models from maya
		for (int i = 0; i < modelArray.size(); i++) {
			

			auto m = modelArray[i];
			auto l = GetShaderLocation(m.model.material.shader, "model");

			int modelLocTemp = GetShaderLocation(m.model.material.shader, "model");
			SetShaderValueMatrix(m.model.material.shader, modelLocTemp, m.modelMatrix);

			
			Color finalColor = { m.color.r, m.color.g, m.color.b, m.color.a }; 
			DrawModel(m.model, {}, 1.0, finalColor);
		}

		

		DrawGrid(10, 1.0f);     // Draw a grid
		EndMode3D();

		// write text 
		DrawTextRL("Maya API level editor", screenWidth - 120, screenHeight - 20, 10, GRAY);
		DrawTextRL(FormatText("Camera position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z), 600, 20, 10, BLACK);
		DrawTextRL(FormatText("Camera target: (%.2f, %.2f, %.2f)", camera.target.x, camera.target.y, camera.target.z), 600, 40, 10, GRAY);
		DrawFPS(10, 10);
		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization

	// Unload shaders + textures
	UnloadShader(shader1); 
	for (int i = 0; i < materialArray.size(); i++) {

		UnloadShader(materialArray[i].matShader);

		if(materialArray[i].type == 1)
			UnloadTexture(materialArray[i].matTexture);
	}
	
	// unload models 
	for (int i = 0; i < modelArray.size(); i++) {
		UnloadShader(modelArray[i].model.material.shader);
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

void recvFromMaya(std::map<CMDTYPE, FnPtr> functionMap, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray) {

	char* buffer; 
	messageType msgType = {}; 

	size_t nr = comLib.nextSize();
	buffer = new char[oldBuffSize];

	if (nr > oldBuffSize) {
		delete[] buffer;
		buffer = new char[nr];
		oldBuffSize = nr;
	}



	// get message from Maya ======
	if (comLib.recv(buffer, nr)) {

		memcpy((char*)&msgType, buffer, sizeof(messageType));

		if (msgType.cmdType > 1000 && msgType.msgNr > 0) {
			functionMap[msgType.cmdType](msgType.msgNr, modelArray, lightsArray, cameraArray, materialArray, buffer);
		}
	}

	delete[] buffer; 
		

}