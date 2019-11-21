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

int findMaterial(std::vector<materialFromMaya>& matArr, std::string MatName) {

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

void updateCamera(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {


	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer + sizeof(messageType), sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	if (msgHeader.nodeType == NODE_TYPE::CAMERA) {

		cameraFromMaya camInfo = {};
		memcpy((char*)&camInfo, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(cameraFromMaya));

		cameraArray.at(0) = camInfo;

	}
}

// function that adds a new node
void addNode(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "ADD NODE FUNCTION" << std::endl;
	*/

	int matOffset = 0;
	size_t meshOffsetSize = 0;

	for (int i = 0; i < nrMsg; i++) {

		size_t totalOffset = meshOffsetSize + ((sizeof(MsgHeader) + sizeof(msgMaterial)) * matOffset);

		MsgHeader msgHeader = {}; 
		memcpy((char*)&msgHeader, buffer + sizeof(messageType) + totalOffset, sizeof(MsgHeader));

		std::string objectName = msgHeader.objName;
		objectName = objectName.substr(0, msgHeader.nameLen);

		if (msgHeader.nodeType == NODE_TYPE::MESH) {

			// check of mesh already exists. If it doesn't - add it
			int modelindex = findMesh(modelArray, objectName);
			if (modelindex == -1) {

				// get mesh info send over
				msgMesh meshInfo = {};
				memcpy((char*)&meshInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + totalOffset, sizeof(msgMesh));

				// create arrays to store information & get info from buffer
				float* meshUVs  = new float[meshInfo.UVcount];
				float* meshVtx  = new float[meshInfo.vtxCount * 3];
				float* meshNorm = new float[meshInfo.normalCount * 3];

				memcpy((char*)meshVtx, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + totalOffset, (sizeof(float) * meshInfo.vtxCount * 3));
				memcpy((char*)meshNorm, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + totalOffset, (sizeof(float) * meshInfo.normalCount * 3));
				memcpy((char*)meshUVs, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3) + totalOffset, (sizeof(float) * meshInfo.UVcount));

				// get material info sent over
				msgMaterial materialInfo = {};
				memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3) + (sizeof(float) * meshInfo.UVcount) + totalOffset, sizeof(msgMaterial));


				std::string materialName = materialInfo.materialName;
				materialName = materialName.substr(0, materialInfo.matNameLen);

				std::string texturePath = materialInfo.fileTextureName;
				texturePath = texturePath.substr(0, materialInfo.textureNameLen);

				// get matrix info sent over 
				Matrix matrixInfo = {};
				memcpy((char*)&matrixInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3) + (sizeof(float) * meshInfo.UVcount) + sizeof(msgMaterial) + totalOffset, sizeof(Matrix));

				// Check if material already exists, else push back new mat
				materialFromMaya tempMaterial = {};
				tempMaterial.texturePath = texturePath;
				tempMaterial.materialName = materialName;
				tempMaterial.type = materialInfo.type;
				tempMaterial.matColor = materialInfo.color;

				int materialIndex = findMaterial(materialArray, materialName);
				if (materialIndex >= 0) {
					tempMaterial.matShader = materialArray[materialIndex].matShader;
					materialArray[materialIndex] = { tempMaterial };
				}

				else {
					tempMaterial.matShader = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");
					materialArray.push_back({ tempMaterial });
					materialIndex = materialArray.size() - 1;
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
				tempModel.material.shader = materialArray[materialIndex].matShader;


				// if materal has texture, load it
				if (materialInfo.type == 1 && texturePath.length() > 0) {
					tempMaterial.matTexture = LoadTexture(texturePath.c_str());
					tempModel.material.maps[MAP_DIFFUSE].texture = tempMaterial.matTexture;

				}

				// pushback the new node
				modelindex = modelArray.size() - 1;
				if (modelArray.size() == 0)
					modelindex = 0;



				modelArray.push_back({ modelindex, objectName, tempMesh, tempModel, MatrixTranslate(0,0,0), materialInfo.color, materialName });

				// delete any allocated arrays
				delete[] meshVtx;
				delete[] meshNorm;
				delete[] meshUVs;

				/*std::cout << "Mesh does not exist: " << objectName << std::endl;
				std::cout << "VTX CNT: " << meshInfo.vtxCount << std::endl;
				std::cout << "materialName: " << materialName << std::endl;
				std::cout << "texturePath: " << texturePath << std::endl;*/

			}

			meshOffsetSize += msgHeader.msgSize; 
		}

		else if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

			int materialIndex = findMaterial(materialArray, objectName);
			if (materialIndex == -1) {

				std::cout << "Add material " << objectName << std::endl; 


				msgMaterial materialInfo = {};
				memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + totalOffset, sizeof(msgMaterial));

				std::string matName = materialInfo.materialName;
				matName = matName.substr(0, materialInfo.matNameLen);

				std::string textureName = materialInfo.fileTextureName;
				textureName = textureName.substr(0, materialInfo.textureNameLen);

				materialFromMaya tempMat = {}; 
				tempMat.materialName = matName; 
				tempMat.texturePath  = textureName; 
				tempMat.type		 = materialInfo.type; 
				tempMat.matColor	 = materialInfo.color; 
				tempMat.matShader	 = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");

				if (tempMat.type == 1 && textureName.length() > 0) {
					tempMat.matTexture = LoadTexture(textureName.c_str());
				}


				materialArray.push_back({ tempMat });

				materialIndex = materialArray.size() - 1;
				if (materialArray.size() == 0)
					materialIndex = 0; 


				// check if objects have material and update them
				for (int i = 0; i < modelArray.size(); i++) {
					if (modelArray[i].materialName == matName) {

						// reaload mesh & model
						rlLoadMesh(&modelArray[i].mesh, false);
						modelArray[i].model = LoadModelFromMesh(modelArray[i].mesh);
						modelArray[i].model.material.shader = materialArray[materialIndex].matShader;

						// if material has texture, reload it
						if (materialInfo.type == 1 && textureName.length() > 0) {

							materialArray[materialIndex].matTexture = LoadTexture(textureName.c_str());
							modelArray[i].model.material.maps[MAP_DIFFUSE].texture = materialArray[materialIndex].matTexture;
					
						}
						

						// update model 
						modelArray[i].color = materialInfo.color;
						modelArray[i].materialName = materialInfo.materialName;

			
					}
	 

				}

				//std::cout << "Mat name: " << matName << std::endl; 
				//std::cout << "textureName: " << textureName << std::endl;

			}

			matOffset++; 
		}

	}


	/* 
	if (msgHeader.nodeType == NODE_TYPE::LIGHT) {
		
		// get light info send over
		lightFromMaya lightInfo = {};
		memcpy((char*)&lightInfo, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(lightFromMaya));

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

			memcpy(lightInfo.lightName, lightName.c_str(), lightInfo.lightNameLen);
			lightsArray.push_back({ lightInfo });
		}

	}

	*/
	
}

// function updates an existing node
void updateNode(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE NODE FUNCTION" << std::endl;
	*/
	

	int matOffset		  = 0;
	int meshMatOffset	  = 0; 
	size_t meshOffsetSize = 0;

	for (int i = 0; i < nrMsg; i++) {

		//std::cout <<  std::endl;
		//std::cout << "---------------" << std::endl;

		size_t totalOffset = meshOffsetSize + ((sizeof(MsgHeader) + sizeof(msgMaterial)) * matOffset) + ((sizeof(MsgHeader) + sizeof(msgMesh) + sizeof(msgMaterial)) * meshMatOffset);

		MsgHeader msgHeader = {};
		memcpy((char*)&msgHeader, buffer + sizeof(messageType) + totalOffset, sizeof(MsgHeader));

		std::string objectName = msgHeader.objName;
		objectName = objectName.substr(0, msgHeader.nameLen);

		if (msgHeader.nodeType == NODE_TYPE::MESH) {

			// check of mesh already exists. If it doesn't - add it
			int modelIndex = findMesh(modelArray, objectName);
			if (modelIndex >= 0) {

				//std::cout << "Mesh exists. Updating  " << objectName << std::endl;

				// get mesh info from buffer
				msgMesh meshInfo = {};
				memcpy((char*)&meshInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + totalOffset, sizeof(msgMesh));

				// create arrays to store information
				float* meshUVs  = new float[meshInfo.UVcount];
				float* meshVtx  = new float[meshInfo.vtxCount * 3];
				float* meshNorm = new float[meshInfo.normalCount * 3];


				memcpy((char*)meshVtx, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + totalOffset, (sizeof(float) * meshInfo.vtxCount * 3));
				memcpy((char*)meshNorm, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + totalOffset, (sizeof(float) * meshInfo.normalCount * 3));
				memcpy((char*)meshUVs, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + (sizeof(float) * meshInfo.vtxCount * 3) + (sizeof(float) * meshInfo.normalCount * 3) + totalOffset, (sizeof(float) * meshInfo.UVcount));
				
				Mesh tempMesh = {};
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

				
				int materialIndex = findMaterial(materialArray, modelArray[modelIndex].materialName);

				if (materialIndex == -1) {
					materialFromMaya tempMat = {};
					tempMat.type = 0;
					tempMat.materialName = "tempMat";
					tempMat.texturePath = "NA";
					tempMat.matColor = { 0, 0, 0, 255};
					tempMat.matShader = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");

					materialArray.push_back(tempMat); 
					materialIndex = materialArray.size() - 1; 
					if (materialArray.size() == 0)
						materialIndex = 0; 
				}
		
				rlLoadMesh(&tempMesh, false);
				modelArray[modelIndex].model = LoadModelFromMesh(tempMesh);
				modelArray[modelIndex].model.material.shader = materialArray[materialIndex].matShader;

				if (materialArray[materialIndex].type == 1 && materialArray[materialIndex].texturePath.length() > 0) {

					materialArray[materialIndex].matColor = { 255, 255, 255, 255 };
					materialArray[materialIndex].matTexture = LoadTexture(materialArray[materialIndex].texturePath.c_str());
					modelArray[modelIndex].model.material.maps[MAP_DIFFUSE].texture = materialArray[materialIndex].matTexture;

				}
				

				// replace with new values
			//modelArray[modelindex] = { modelArray[modelindex].index, objectName, tempMesh, modelArray[modelindex].model, modelArray[modelindex].modelMatrix, modelArray[modelindex].color, modelArray[modelindex].materialName };

				modelArray[modelIndex].mesh = tempMesh;
				modelArray[modelIndex].color = materialArray[materialIndex].matColor;
				modelArray[modelIndex].materialName = materialArray[materialIndex].materialName;

				// delete the arrays
				delete[] meshVtx;
				delete[] meshNorm;
				delete[] meshUVs;


				meshOffsetSize += msgHeader.msgSize;
			}
		}


		else if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {


			int matIndex = findMaterial(materialArray, objectName);
			if (matIndex >= 0) {

				msgMaterial materialInfo = {};
				memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + totalOffset, sizeof(msgMaterial));

				std::string materialName = materialInfo.materialName;
				materialName = materialName.substr(0, materialInfo.matNameLen);

				std::string texturePath = materialInfo.fileTextureName;
				texturePath = texturePath.substr(0, materialInfo.textureNameLen);

				/* 
				std::cout << "Update material " << objectName << std::endl;
				std::cout << "materialName " << materialName << std::endl;
				std::cout << "texturePath " << texturePath << std::endl;
				*/

				// update material in array
				materialArray[matIndex].materialName = materialName;
				materialArray[matIndex].texturePath  = texturePath;
				materialArray[matIndex].type		 = materialInfo.type; 
				materialArray[matIndex].matColor	 = materialInfo.color;
			
				// check if objects have material and update them
				for (int i = 0; i < modelArray.size(); i++) {
					if (modelArray[i].materialName == materialName) {

						//std::cout << "Mesh with material found " << modelArray[i].name << std::endl;


						// reaload mesh & model
						rlLoadMesh(&modelArray[i].mesh, false);
						modelArray[i].model = LoadModelFromMesh(modelArray[i].mesh);
						modelArray[i].model.material.shader = materialArray[matIndex].matShader;

						// if material has texture, reload it
						if (materialInfo.type == 1 && texturePath.length() > 0) {

							materialArray[matIndex].matTexture = LoadTexture(texturePath.c_str());
							modelArray[i].model.material.maps[MAP_DIFFUSE].texture = materialArray[matIndex].matTexture;
						}

						
						// update model 
						modelArray[i].color		   = materialArray[matIndex].matColor;
						modelArray[i].materialName = materialArray[matIndex].materialName;
			

					}
				}


			}



			matOffset++; 
		}


		else if (msgHeader.nodeType == NODE_TYPE::MESH_MATERIAL) {

			std::cout << "Update material for " << objectName << std::endl;

			int modelIndex = findMesh(modelArray, objectName);
			if (modelIndex >= 0) {

				msgMesh meshInfo = {};
				memcpy((char*)&meshInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + totalOffset, sizeof(msgMesh));

				msgMaterial materialInfo = {};
				memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh) + totalOffset, sizeof(msgMaterial));

				std::string materialName = materialInfo.materialName;
				materialName = materialName.substr(0, materialInfo.matNameLen);

				std::string texturePath = materialInfo.fileTextureName;
				texturePath = texturePath.substr(0, materialInfo.textureNameLen);


				int materialIndex = findMaterial(materialArray, materialName);
				if (materialIndex == -1) {

					std::cout << "Material not found... " << objectName << std::endl;


					materialFromMaya tempMaterial = {}; 
					tempMaterial.texturePath  = texturePath; 
					tempMaterial.materialName = materialName;
					tempMaterial.type		  = materialInfo.type; 
					tempMaterial.matColor	  = materialInfo.color; 

					tempMaterial.matShader	  = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");
				
					if (tempMaterial.type == 1 && texturePath.length() > 0) {
						tempMaterial.matTexture = LoadTexture(texturePath.c_str());
					}

					materialArray.push_back({ tempMaterial });
					materialIndex = materialArray.size() - 1;

					if (materialArray.size() == 0)
						materialIndex = 0;

				}
				
				// reload model and shader
				rlLoadMesh(&modelArray[modelIndex].mesh, false);
				modelArray[modelIndex].model = LoadModelFromMesh(modelArray[modelIndex].mesh);
				modelArray[modelIndex].model.material.shader = materialArray[materialIndex].matShader;


				materialArray[modelIndex].texturePath  = texturePath;
				materialArray[modelIndex].materialName = materialName;
				materialArray[modelIndex].type		   = materialInfo.type;

				/* 
				std::cout << "Update material for " << objectName << std::endl;
				std::cout << "materialName " << materialName << std::endl;
				std::cout << "texturePath " << texturePath << std::endl;
				std::cout << "materialIndex " << materialIndex << std::endl;
				std::cout << "materialArray[materialIndex].type " << materialArray[materialIndex].type << std::endl;
				*/

				if (materialArray[modelIndex].type == 1 && texturePath.length() > 0) {
					
					//std::cout << "material has texture " << texturePath << std::endl; 

					materialInfo.color = { 255,255,255,255 };
					
					materialArray[materialIndex].matTexture = LoadTexture(texturePath.c_str());
					modelArray[modelIndex].model.material.maps[MAP_DIFFUSE].texture = materialArray[materialIndex].matTexture;
				
				}

				// update material and mesh
				materialArray[modelIndex].matColor	= materialInfo.color;
				modelArray[modelIndex].color		= materialInfo.color; 
				modelArray[modelIndex].materialName = materialName; 
				


		

			}


			meshMatOffset++; 
		}


	}



	
	/* 
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

			

			lightsArray[lightIndex] = { lightInfo };
			
		}
	}
	*/

}

// function to delete a node
void deleteNode(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "DELETE NODE FUNCTION" << std::endl;
	*/
	

	int Offset = 0;
	for (int i = 0; i < nrMsg; i++) {
	

		MsgHeader msgHeader = {};
		memcpy((char*)&msgHeader, buffer + sizeof(messageType) + (sizeof(MsgHeader) * Offset), sizeof(MsgHeader));

		std::string objectName = msgHeader.objName;
		objectName = objectName.substr(0, msgHeader.nameLen);

	
		if (msgHeader.nodeType == NODE_TYPE::MESH) {

			int modelIndex = findMesh(modelArray, objectName);
			if (modelIndex >= 0) {

				UnloadMesh(&modelArray[modelIndex].model.mesh);
				modelArray.erase(modelArray.begin() + modelIndex);
			}
		}
		
	
		else if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

			int matIndex = findMaterial(materialArray, objectName);
			if (matIndex >= 0) {

				bool meshHasMat = false;
				for (int i = 0; i < modelArray.size(); i++) {
					if (modelArray[i].materialName == objectName) {
						meshHasMat = true;
						break;
					}
				}

				if (!meshHasMat) {
			
					//Unload shader and texture and remove material
					if (materialArray[matIndex].type == 1)
						UnloadTexture(materialArray[matIndex].matTexture);

					UnloadShader(materialArray[matIndex].matShader);
					materialArray.erase(materialArray.begin() + matIndex);
				}
			}

		}


		Offset++; 
	}
	

	
}

// function that updates the name of a node
void updateNodeName(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	 /* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE NODE NAME FUNCTION" << std::endl;
	 */
	
	int msgOffset = 0;
	for (int i = 0; i < nrMsg; i++) {

		MsgHeader msgHeader = {};
		memcpy((char*)&msgHeader, buffer + sizeof(messageType) + ((sizeof(MsgHeader) + sizeof(NodeRenamed)) * msgOffset), sizeof(MsgHeader));

		std::string objectName = msgHeader.objName;
		objectName = objectName.substr(0, msgHeader.nameLen);

		NodeRenamed renamedInfo = {};
		memcpy((char*)&renamedInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + ((sizeof(MsgHeader) + sizeof(NodeRenamed)) * msgOffset), sizeof(NodeRenamed));

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

				materialArray[matIndex].materialName = newName;
			}
		}


		msgOffset++;
	}


}

// function that updates the matrix of a node
void updateNodeMatrix(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	/* 
	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATRIX" << std::endl;
	std::cout << "nr msg " << nrMsg << std::endl;
	*/


	int msgOffset = 0; 
	for (int i = 0; i < nrMsg; i++) {

		MsgHeader msgHeader = {};
		memcpy((char*)&msgHeader, buffer + sizeof(messageType) + ((sizeof(MsgHeader) + sizeof(Matrix)) * msgOffset), sizeof(MsgHeader));

		std::string objectName = msgHeader.objName;
		objectName = objectName.substr(0, msgHeader.nameLen);

		Matrix tempMatrix = {};
		memcpy((char*)&tempMatrix, buffer + sizeof(messageType) + sizeof(MsgHeader) + ((sizeof(MsgHeader) + sizeof(Matrix)) * msgOffset), sizeof(Matrix));


		if (msgHeader.nodeType == NODE_TYPE::MESH) {

			// check if mesh exists in scene and update the matrix
			int modelIndex = findMesh(modelArray, objectName);
			if (modelIndex >= 0) {

				modelArray[modelIndex].modelMatrix = tempMatrix;
			}
		}

		msgOffset += 1; 
	}
	

	/* 
	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader + sizeof(MsgHeader), buffer + sizeof(int), sizeof(MsgHeader));

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

	*/
	
}


	/* 
// function that updates when a "new" material is connected to a mesh
void updateNodeMaterial(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATERIAL FOR NODE" << std::endl;
	
	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer + sizeof(messageType), sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//if (msgHeader.nodeType == NODE_TYPE::MESH) {

		// get mesh info send over
		msgMesh meshInfo = {};
		memcpy((char*)&meshInfo, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(msgMesh));

		msgMaterial materialInfo = {};
		memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader) + sizeof(msgMesh), sizeof(msgMaterial));

		std::string materialName = materialInfo.materialName;
		materialName = materialName.substr(0, materialInfo.matNameLen);

		std::string texturePath = materialInfo.fileTextureName;
		texturePath = texturePath.substr(0, materialInfo.textureNameLen);

		int modelIndex = findMesh(modelArray, objectName);
		if (modelIndex >= 0) {

			int matIndex = findMaterial(materialArray, materialName);
			if (matIndex == -1) {

				materialFromMaya tempMaterial = {}; 
				tempMaterial.texturePath  = texturePath; 
				tempMaterial.materialName = materialName;
				tempMaterial.type		  = materialInfo.type; 
				tempMaterial.matColor	  = materialInfo.color; 

				tempMaterial.matShader	  = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");
				
				if (tempMaterial.type == 1 && texturePath.length() > 0) {
					tempMaterial.matTexture = LoadTexture(materialArray[matIndex].texturePath.c_str());
				}

				materialArray.push_back({ tempMaterial });
				matIndex = materialArray.size() - 1; 

				if (materialArray.size() == 0)
					matIndex = 0; 

			}


			rlLoadMesh(&modelArray[modelIndex].mesh, false);
			modelArray[modelIndex].model = LoadModelFromMesh(modelArray[modelIndex].mesh);
			modelArray[modelIndex].model.material.shader = materialArray[matIndex].matShader; 
		

			if (materialArray[matIndex].type == 1 && texturePath.length() > 0) {
				materialInfo.color = { 255,255,255,255 };

				materialArray[matIndex].matTexture = LoadTexture(materialArray[matIndex].texturePath.c_str());
				modelArray[modelIndex].model.material.maps[MAP_DIFFUSE].texture = materialArray[matIndex].matTexture;
				
			}

			materialArray[matIndex].texturePath  = texturePath; 
			materialArray[matIndex].materialName = materialName; 
			materialArray[matIndex].type		 = materialInfo.type;
			materialArray[matIndex].matColor	 = materialInfo.color;

			// update material and mesh
			modelArray[modelIndex].color = materialInfo.color; 
			modelArray[modelIndex].materialName = materialName; 


		}

	//}

	
}
	*/

	/* 
// function that updates a material
void updateMaterial(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "UPDATE MATERIAL" << std::endl;
	std::cout << "nr msg " << nrMsg << std::endl;
	
	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

		msgMaterial materialInfo = {};
		memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(msgMaterial));
		
		std::string matName = materialInfo.materialName;
		matName = matName.substr(0, materialInfo.matNameLen);

		std::string textureName = materialInfo.fileTextureName;
		textureName = textureName.substr(0, materialInfo.textureNameLen);


		int matIndex = findMaterial(materialArray, matName);
		if (matIndex >= 0) {

			materialArray[matIndex].materialName = matName; 
			materialArray[matIndex].texturePath  = textureName; 
			materialArray[matIndex].type		 = materialInfo.type; 
			materialArray[matIndex].matColor	 = materialInfo.color;

			// check if objects have material and update them
			for (int i = 0; i < modelArray.size(); i++) {
				if (modelArray[i].materialName == matName) {


					// reaload mesh & model
					rlLoadMesh(&modelArray[i].mesh, false);
					modelArray[i].model = LoadModelFromMesh(modelArray[i].mesh);
					modelArray[i].model.material.shader = materialArray[matIndex].matShader;

					// if material has texture, reload it
					if (materialInfo.type == 1 && textureName.length() > 0) {

						materialArray[matIndex].matTexture = LoadTexture(textureName.c_str());
						modelArray[i].model.material.maps[MAP_DIFFUSE].texture = materialArray[matIndex].matTexture;
					}

					// update model 
					modelArray[i].color		   = materialArray[matIndex].matColor;
					modelArray[i].materialName = materialArray[matIndex].materialName;


				}
			}

		}

	
}
	*/

	/* 
// function that adds a new material
void newMaterial(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "NEW MATERIAL" << std::endl;
	
	MsgHeader msgHeader = {};
	memcpy((char*)&msgHeader, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(MsgHeader));

	std::string objectName = msgHeader.objName;
	objectName = objectName.substr(0, msgHeader.nameLen);

	//if (msgHeader.nodeType == NODE_TYPE::MATERIAL) {

	msgMaterial materialInfo = {};
	memcpy((char*)&materialInfo, buffer + sizeof(messageType) + sizeof(MsgHeader), sizeof(msgMaterial));

	std::string matName = materialInfo.materialName;
	matName = matName.substr(0, materialInfo.matNameLen);

	std::string textureName = materialInfo.fileTextureName;
	textureName = textureName.substr(0, materialInfo.textureNameLen);


	int matIndex = findMaterial(materialArray, matName);
	if (matIndex == -1) {

		

		materialFromMaya tempMat = {}; 
		tempMat.type = materialInfo.type; 
		tempMat.materialName = matName; 
		tempMat.texturePath = textureName; 
		tempMat.matColor = materialInfo.color; 
		tempMat.matShader = LoadShader("resources/shaders/glsl330/phong.vs", "resources/shaders/glsl330/phong.fs");

		if (tempMat.type == 1 && textureName.length() > 0) {
			tempMat.matTexture = LoadTexture(textureName.c_str());
		}


		materialArray.push_back({ tempMat });
		matIndex = materialArray.size() - 1; 

		if (materialArray.size() == 0)
			matIndex = 0; 

		
		// check if objects have material and update them
		for (int i = 0; i < modelArray.size(); i++) {
			if (modelArray[i].materialName == matName) {

				

				// reaload mesh & model
				rlLoadMesh(&modelArray[i].mesh, false);
				modelArray[i].model = LoadModelFromMesh(modelArray[i].mesh);
				modelArray[i].model.material.shader = materialArray[matIndex].matShader;

				// if material has texture, reload it
				if (materialInfo.type == 1 && textureName.length() > 0) {

					materialArray[matIndex].matTexture = LoadTexture(textureName.c_str());
					modelArray[i].model.material.maps[MAP_DIFFUSE].texture = materialArray[matIndex].matTexture;
					
				}

				// update model 
				modelArray[i].color = materialInfo.color;
				modelArray[i].materialName = materialInfo.materialName;

			}
		}
	 

	}

	
}
	*/

