#include "NodeFunctions.h"

void addNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

	std::cout << std::endl;
	std::cout << "=======================================" << std::endl;
	std::cout << "ADD NODE FUNCTION" << std::endl;

}

void updateNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

}
void deleteNode(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

}

void updateNodeName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

}

void updateNodeMatrix(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

}

void updateMaterial(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

}

void updateMaterialName(std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int bufferSize, Shader shader, int* nrObjs, int* index, int* nrMaterials) {

}



// ==================================================================================
// ================================= OTHER ==========================================
// ==================================================================================

/*
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
*/

/*
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
*/

/*
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


		else if(tempCam.type == 1) {
			std::cout << "Ortographic cam" << std::endl;

		}


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
*/

/*
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
*/

/*
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
*/

/*
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
*/

/*
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


		if (tempMat.type == 0) {
			std::cout << "is color" << std::endl;
		}


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
*/
