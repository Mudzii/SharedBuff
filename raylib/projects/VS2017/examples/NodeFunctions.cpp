#include "NodeFunctions.h"

// ==============================================

int findMesh(std::vector<modelFromMaya>& modelArr, std::string MeshName) {

	int objIndex = -1; 
	for (int i = 0; i < modelArr.size(); i++) {
		if (modelArr[i].name == MeshName) {

			objIndex = i;
			break;
		}
	}
	
	return objIndex;
}

int findMaterial(std::vector<materialMaya>& matArr, std::string MatName) {

	int matIndex = -1;
	for (int i = 0; i < matArr.size(); i++) {
		if (matArr[i].materialName == MatName) {

			matIndex = i;
			break;
		}
	}

	return matIndex;
}

// ==============================================


// function that adds a new node
void addNode(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	//std::cout << std::endl;
	//std::cout << "=======================================" << std::endl;
	//std::cout << "ADD NODE FUNCTION" << std::endl;

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {


		// check of mesh already exists
		int modelindex = findMesh(modelArray, objectName);
		if (modelindex == -1) {

			// get mesh info send over
			msgMesh meshInfo = {};
			memcpy((char*)&meshInfo, buffer + sizeof(MsgHeader), sizeof(msgMesh));

			// create arrays to store information & get info from buffer
			float* meshUVs  = new float[meshInfo.UVcount];
			float* meshVtx  = new float[meshInfo.vtxCount * 3];
			float* meshNorm = new float[meshInfo.normalCount * 3];

			memcpy((char*)meshVtx,  buffer + sizeof(MsgHeader) + sizeof(msgMesh),  (sizeof(float) * meshInfo.vtxCount * 3));
			memcpy((char*)meshNorm, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3), (sizeof(float) * meshInfo.normalCount * 3));
			memcpy((char*)meshUVs,  buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3), (sizeof(float) * meshInfo.UVcount));

			// get material info sent over
			materialMaya materialInfo = {};
			memcpy((char*)&materialInfo, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3) + (sizeof(float) * meshInfo.UVcount), sizeof(materialMaya));

			std::string materialName = materialInfo.materialName;
			materialName = materialName.substr(0, materialInfo.matNameLen);

			std::string texturePath = materialInfo.fileTextureName;
			texturePath = texturePath.substr(0, materialInfo.textureNameLen);

			// get matrix info sent over 
			Matrix matrixInfo = {};
			memcpy((char*)&matrixInfo, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3) + (sizeof(float) * meshInfo.UVcount) + sizeof(materialMaya), sizeof(Matrix));

			//Shader tempShader; 
			// Check if material already exists, else push back new mat
			int materialIndex = findMaterial(materialArray, materialName);
			if (materialIndex >= 0) {
				materialArray[materialIndex] = materialInfo;
				//tempShader = shaderArr[materialIndex];
			}

			else {
				materialArray.push_back({ materialInfo });
				materialIndex = materialArray.size() - 1; 
				//tempShader = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs"); 
				//shaderArr.push_back(tempShader);
			}

			// create a temp mesh and fill with info
			Mesh tempMesh = {};

			// vtx information
			tempMesh.vertexCount = meshInfo.vtxCount;
			tempMesh.vertices = new float[meshInfo.vtxCount * 3];
			memcpy(tempMesh.vertices, meshVtx, (sizeof(float) * tempMesh.vertexCount * 3));

			// normal information
			tempMesh.normals = new float[meshInfo.normalCount * 3];
			memcpy(tempMesh.normals, meshNorm, (sizeof(float) * meshInfo.normalCount * 3));

			// UV information
			tempMesh.texcoords = new float[meshInfo.UVcount];
			memcpy(tempMesh.texcoords, meshUVs, (sizeof(float) * meshInfo.UVcount));

			// load mesh and create material
			rlLoadMesh(&tempMesh, false);
			Model tempModel = LoadModelFromMesh(tempMesh);
			
			//tempModel.material.shader = shaderArr[materialIndex]; 
			tempModel.material.shader = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");
			shaderArr.push_back(tempModel.material.shader);

			// if materal has texture, load it
			if (materialInfo.type == 1 && texturePath.length() > 0) {
				Texture2D texture = LoadTexture(texturePath.c_str());
				tempModel.material.maps[MAP_DIFFUSE].texture = texture;

				textureArr.push_back(texture);
			}

			// pushback the new node
			modelArray.push_back({ *index, objectName, tempMesh, tempModel, matrixInfo, materialInfo.color, materialName });
			*index = *index + 1;


			// delete any allocated arrays
			delete[] meshVtx;
			delete[] meshNorm;
			delete[] meshUVs;
			
		}

	}
	
	if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
		
		// get light info send over
		lightFromMaya lightInfo = {};
		memcpy((char*)&lightInfo, buffer + sizeof(MsgHeader), sizeof(lightFromMaya));

		std::string lightName = lightInfo.lightName;
		lightName = lightName.substr(0, lightInfo.lightNameLen);
		

		// check if light exists, otherwise add new
		int lightIndex = -1; 
		for (int i = 0; i < lightsArray.size(); i++) {
			if (lightsArray[i].lightName == lightName) {
				lightIndex = i; 
				break; 
			}
		}

		if (lightIndex == -1) {

			/*lightFromMaya tempLight = {};

			tempLight.color    = lightInfo.color;
			tempLight.lightPos = lightInfo.lightPos;
			tempLight.lightNameLen	  = lightInfo.lightNameLen;
			tempLight.intensityRadius = lightInfo.intensityRadius;
			memcpy(tempLight.lightName, lightName.c_str(), tempLight.lightNameLen);*/

			memcpy(lightInfo.lightName, lightName.c_str(), lightInfo.lightNameLen);
			lightsArray.push_back({ lightInfo });
		}

	}
	
}

// function updates an existing node
void updateNode(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {


	//std::cout << std::endl;
	//std::cout << "=======================================" << std::endl;
	//std::cout << "UPDATE NODE FUNCTION" << std::endl;

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// get mesh info from buffer
		msgMesh meshInfo = {};
		memcpy((char*)&meshInfo, buffer + sizeof(MsgHeader), sizeof(msgMesh));

		// create arrays to store information
		float* meshUVs  = new float[meshInfo.UVcount];
		float* meshVtx  = new float[meshInfo.vtxCount * 3];
		float* meshNorm = new float[meshInfo.normalCount * 3];

		memcpy((char*)meshVtx,  buffer + sizeof(MsgHeader) + sizeof(msgMesh),  (sizeof(float) * meshInfo.vtxCount * 3));
		memcpy((char*)meshNorm, buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3),  (sizeof(float) * meshInfo.normalCount * 3));
		memcpy((char*)meshUVs,  buffer + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3), (sizeof(float) * meshInfo.UVcount));
	
		// Check if mesh already exists
		int modelindex = findMesh(modelArray, objectName);
		// get mesh material

		if (modelindex >= 0) {

			Color tempColor   = modelArray[modelindex].color;
			Matrix tempMatrix = modelArray[modelindex].modelMatrix;
			std::string tempMaterialName = modelArray[modelindex].materialName;
			
			Mesh tempMesh = {};
			int tempIndex = modelArray[modelindex].index;

			// vtx
			tempMesh.vertexCount = meshInfo.vtxCount;
			tempMesh.vertices = new float[meshInfo.vtxCount * 3];
			memcpy(tempMesh.vertices, meshVtx, (sizeof(float) * tempMesh.vertexCount * 3));

			// normals
			tempMesh.normals = new float[meshInfo.normalCount * 3];
			memcpy(tempMesh.normals, meshNorm, (sizeof(float) * meshInfo.normalCount * 3));

			// UVs
			tempMesh.texcoords = new float[meshInfo.UVcount];
			memcpy(tempMesh.texcoords, meshUVs, (sizeof(float) * meshInfo.UVcount));

			int materialIndex = findMaterial(materialArray, tempMaterialName);

			rlLoadMesh(&tempMesh, false);
			modelArray[modelindex].model = LoadModelFromMesh(tempMesh);
			modelArray[modelindex].model.material.shader = shaderArr[modelindex];
			//modelArray[modelindex].model.material.shader = LoadShader("resources/shaders/glsl330/phong.vs",
				//"resources/shaders/glsl330/phong.fs");
			//shaderArr.push_back(modelArray[modelindex].model.material.shader);

			if (materialIndex >= 0) {
				if (materialArray[materialIndex].type == 1 && materialArray[materialIndex].textureNameLen > 0) {

					tempColor		  = { 255, 255, 255, 255 };
					Texture2D texture = LoadTexture(materialArray[materialIndex].fileTextureName);
					modelArray[modelindex].model.material.maps[MAP_DIFFUSE].texture = texture;
					textureArr.push_back(texture);
				}

			}

			// replace with new values
			modelArray[modelindex] = { tempIndex, objectName, tempMesh, modelArray[modelindex].model, tempMatrix, tempColor, tempMaterialName };

		}

		// delete the arrays
		delete[] meshVtx;
		delete[] meshNorm;
		delete[] meshUVs;
		
		
	}
	
	if (msgHeader.nodeType == NODE_TYPE::CAMERA) {

		cameraFromMaya camInfo = {};
		memcpy((char*)&camInfo, buffer + sizeof(MsgHeader), sizeof(cameraFromMaya));

		cameraArray.at(0) = camInfo;

	}
	
	if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
		
		// get mesh info send over
		lightFromMaya lightInfo = {};
		memcpy((char*)&lightInfo, buffer + sizeof(MsgHeader), sizeof(lightFromMaya));

		std::string lightName = lightInfo.lightName;
		lightName = lightName.substr(0, lightInfo.lightNameLen);
		

		int lightIndex = -1; 
		for (int i = 0; i < lightsArray.size(); i++) {
			if (lightsArray[i].lightName == lightName) {
				lightIndex = i; 
				break; 
			}
		}

		if (lightIndex >= 0) {

			memcpy(lightInfo.lightName, lightName.c_str(), lightInfo.lightNameLen); 

			/*std::cout << "UPDATING LIGHT" << std::endl;

			std::cout << "CLR LIGHT " << (int)lightInfo.color.r << "  " << (int)lightInfo.color.g << "  " << (int)lightInfo.color.b << std::endl;
			lightFromMaya tempLight = {}; 

			tempLight.color	   = lightInfo.color; 
			tempLight.lightPos = lightInfo.lightPos; 
			tempLight.lightNameLen	  = lightInfo.lightNameLen; 
			tempLight.intensityRadius = lightInfo.intensityRadius; 
			memcpy(tempLight.lightName, lightName.c_str(), tempLight.lightNameLen);*/

			lightsArray[lightIndex] = { lightInfo };
			
		}
	}
}

// function to delete a node
void deleteNode(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "DELETE NODE FUNCTION" << std::endl;
	*/
	
	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		int modelIndex = findMesh(modelArray, objectName); 
		if (modelIndex >= 0) {

			UnloadShader(shaderArr[modelIndex]);
			UnloadShader(modelArray[modelIndex].model.material.shader);
			shaderArr.erase(shaderArr.begin() + modelIndex);

			UnloadModel(modelArray[modelIndex].model);
			modelArray.erase(modelArray.begin() + modelIndex);
			*index  = *index - 1;
		}
		
	}

	if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

		int matIndex = findMaterial(materialArray, objectName); 
		if (matIndex >= 0) {

			for (int i = 0; i < modelArray.size(); i++) {
				if (modelArray[i].materialName == objectName) {
					modelArray[i].color = { 0,0,0,255 }; 
				}
			}

			materialArray.erase(materialArray.begin() + matIndex);
		}		
	}

	if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
		int lightIndex = -1; 
		for (int i = 0; i < lightsArray.size(); i++) {
			if (lightsArray[i].lightName == objectName) {
				lightIndex = i; 
			}
		}

		if (lightIndex >= 0) {
			lightsArray.erase(lightsArray.begin() + lightIndex);
		}
	}


}

// function that updates the name of a node
void updateNodeName(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	/*
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE NODE NAME FUNCTION" << std::endl;
	*/

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	NodeRenamed renamedInfo = {}; 
	memcpy((char*)&renamedInfo, buffer + sizeof(MsgHeader), sizeof(NodeRenamed));

	std::string oldName = renamedInfo.nodeOldName;
	oldName = oldName.substr(0, renamedInfo.nodeOldNameLen);

	std::string newName = renamedInfo.nodeName;
	newName = newName.substr(0, renamedInfo.nodeNameLen);

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// check if mesh exists in array and rename it
		int meshIndex = findMesh(modelArray, oldName);
		if (meshIndex >= 0) {
			modelArray[meshIndex].name = newName; 

		}
	}

	if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

		int matIndex = findMaterial(materialArray, oldName);
		if (matIndex >= 0) {
			memcpy(materialArray[matIndex].materialName, newName.c_str(), renamedInfo.nodeNameLen);
		}
			
	}

}

// function that updates the matrix of a node
void updateNodeMatrix(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	Matrix tempMatrix = {};
	memcpy((char*)&tempMatrix, buffer + sizeof(MsgHeader), sizeof(Matrix));

	if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// check if mesh exists in scene and update the matrix
		int modelIndex = findMesh(modelArray, objectName);
		if (modelIndex >= 0) {
			
			modelArray[modelIndex].modelMatrix = tempMatrix; 
		}
	}

	
}

// function that updates when a "new" material is connected to a mesh
void updateNodeMaterial(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATERIAL FOR NODE" << std::endl;
	*/

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// get mesh info send over
		msgMesh meshInfo = {};
		memcpy((char*)&meshInfo, buffer + sizeof(MsgHeader), sizeof(msgMesh));

		materialMaya materialInfo = {}; 
		memcpy((char*)&materialInfo, buffer + sizeof(MsgHeader) + sizeof(msgMesh), sizeof(materialMaya));

		std::string materialName = materialInfo.materialName;
		materialName = materialName.substr(0, materialInfo.matNameLen);

		std::string texturePath = materialInfo.fileTextureName;
		texturePath = texturePath.substr(0, materialInfo.textureNameLen);
		
		int modelIndex = findMesh(modelArray, objectName);
		if (modelIndex >= 0) {

			rlLoadMesh(&modelArray[modelIndex].mesh, false);
			modelArray[modelIndex].model = LoadModelFromMesh(modelArray[modelIndex].mesh);
			modelArray[modelIndex].model.material.shader = shaderArr[modelIndex];
			//modelArray[modelIndex].model.material.shader = LoadShader("resources/shaders/glsl330/phong.vs",
				//"resources/shaders/glsl330/phong.fs");
			//shaderArr.push_back(modelArray[modelIndex].model.material.shader);
		
			int matIndex = findMaterial(materialArray, materialName);
			if (matIndex == -1) {
				materialArray.push_back({ materialInfo });
				matIndex = materialArray.size() - 1; 
			}

			if (matIndex >= 0) {

				if (materialArray[matIndex].type == 1 && texturePath.length() > 0) {
					materialInfo.color = { 255,255,255,255 };

					Texture2D texture = LoadTexture(materialArray[matIndex].fileTextureName);
					modelArray[modelIndex].model.material.maps[MAP_DIFFUSE].texture = texture;
					textureArr.push_back(texture);
				}

				// update material and mesh
				materialArray[matIndex] = { materialInfo };
				modelArray[modelIndex].color = materialInfo.color; 
				modelArray[modelIndex].materialName = materialName; 
		
			}

			
		}

	//}

}

// function that updates a material
void updateMaterial(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATERIAL" << std::endl;
	*/
	
	
	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

		materialMaya materialInfo = {}; 
		memcpy((char*)&materialInfo, buffer + sizeof(MsgHeader), sizeof(materialMaya));
		
		std::string matName = materialInfo.materialName;
		matName = matName.substr(0, materialInfo.matNameLen);

		std::string textureName = materialInfo.fileTextureName;
		textureName = textureName.substr(0, materialInfo.textureNameLen);

		int matIndex = findMaterial(materialArray, matName);
		if (matIndex>=0) {
			materialArray[matIndex] = { materialInfo };

		/*else {
			materialArray.push_back({ materialInfo });
			matIndex = materialArray.size() - 1; 
		}*/

		// check if objects have material and update them
		for (int i = 0; i < modelArray.size(); i++) {
			if (modelArray[i].materialName == matName) {

				// reaload mesh & model
				rlLoadMesh(&modelArray[i].mesh, false);
				modelArray[i].model = LoadModelFromMesh(modelArray[i].mesh);
				modelArray[i].model.material.shader = shaderArr[i]; 
				//modelArray[i].model.material.shader = LoadShader("resources/shaders/glsl330/phong.vs",
					//"resources/shaders/glsl330/phong.fs");
				//shaderArr.push_back(modelArray[i].model.material.shader);

				// if material has texture, reload it
				if (materialInfo.type == 1 && textureName.length() > 0) {

					Texture2D texture = LoadTexture(textureName.c_str());
					modelArray[i].model.material.maps[MAP_DIFFUSE].texture = texture;
					textureArr.push_back(texture);
				}

				// update model 
				modelArray[i].color		   = materialInfo.color; 
				modelArray[i].materialName = materialInfo.materialName;

				/* 
				Mesh tempMesh = modelArray[i].mesh;
				Color tempModelColor = materialInfo.color;
				Model tempModel = modelArray[i].model;
				int tempModelIndex = modelArray[i].index;
				std::string tempModelName = modelArray[i].name;
				Matrix tempModelMatrix = modelArray[i].modelMatrix;
				std::string tempModelMaterialName = materialInfo.materialName;

				//modelArray.erase(modelArray.begin() + i);
				//std::cout << "MAT TEXTURE 2: " << textureName << std::endl;

				modelArray[i] = { tempModelIndex, tempModelName, tempMesh, tempModel, tempModelMatrix, tempModelColor, tempModelMaterialName };

				*/

			}
		}

		}

	//}
}

// function that adds a new material
void newMaterial(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "NEW MATERIAL" << std::endl;
	*/
	 

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

		materialMaya materialInfo = {}; 
		memcpy((char*)&materialInfo, buffer + sizeof(MsgHeader), sizeof(materialMaya));

		std::string matName = materialInfo.materialName;
		matName = matName.substr(0, materialInfo.matNameLen);

		std::string textureName = materialInfo.fileTextureName;
		textureName = textureName.substr(0, materialInfo.textureNameLen);

		int matIndex = findMaterial(materialArray, matName);

		//Shader matShader = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");
		//shaderArr.push_back(matShader); 
		/*if(matIndex >= 0) {
			materialArray.at(matIndex) = { materialInfo };
		}*/

		/*else {
			matIndex = materialArray.size() - 1; 
		}*/

		if (matIndex == -1) {

		materialArray.push_back({ materialInfo });

		// check if objects have material and update them
		for (int i = 0; i < modelArray.size(); i++) {
			if (modelArray[i].materialName == matName) {

				// reaload mesh & model
				rlLoadMesh(&modelArray[i].mesh, false);
				modelArray[i].model = LoadModelFromMesh(modelArray[i].mesh);
				modelArray[i].model.material.shader = shaderArr[i];
				//modelArray[i].model.material.shader = LoadShader("resources/shaders/glsl330/phong.vs",
					//"resources/shaders/glsl330/phong.fs");
				//shaderArr.push_back(modelArray[i].model.material.shader);

				// if material has texture, reload it
				if (materialInfo.type == 1 && textureName.length() > 0) {

					Texture2D texture = LoadTexture(textureName.c_str());
					modelArray[i].model.material.maps[MAP_DIFFUSE].texture = texture;
					textureArr.push_back(texture);
				}

				// update model 
				modelArray[i].color = materialInfo.color;
				modelArray[i].materialName = materialInfo.materialName;

				/* 
				Color tempModelColor = materialInfo.color;
				Mesh tempMesh = modelArray[i].mesh;
				Model tempModel = modelArray[i].model;
				int tempModelIndex = modelArray[i].index;
				std::string tempModelMaterialName = matName;
				std::string tempModelName = modelArray[i].name;
				Matrix tempModelMatrix = modelArray[i].modelMatrix;

				rlLoadMesh(&tempMesh, false);
				tempModel = LoadModelFromMesh(tempMesh);
				tempModel.material.shader = shader;


				if (materialInfo.type == 1 && textureName.length() > 0) {

					Texture2D texture = LoadTexture(textureName.c_str());
					tempModel.material.maps[MAP_DIFFUSE].texture = texture;
					//modelArray.at(i) = { tempModelIndex, tempModelName, tempMesh, tempModel, tempModelMatrix, tempModelColor, tempModelMaterialName };

					textureArr.push_back(texture);
				}


				//modelArray.at(i) = { tempModelIndex, tempModelName, tempMesh, tempModel, tempModelMatrix, tempModelColor, tempModelMaterialName }; 
				*/


			}
		}

		}


	//}

}
