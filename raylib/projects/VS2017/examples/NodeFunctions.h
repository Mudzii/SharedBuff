#ifndef NODEFUNCTIONS_H
#define NODEFUNCTIONS_H

#include <vector>
#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <sstream>
#include <iostream>

#include <map>
#include <functional>

#define MAX_CHAR_LEN 64


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
	TRANSFORM_UPDATE = 1008,

	UPDATE_NODE_MATERIAL = 1009
};

struct MsgHeader {
	CMDTYPE   cmdType;
	NODE_TYPE nodeType;
	char objName[64];
	int msgSize;
	int nameLen;
};

struct msgMesh {
	int meshID;

	int vtxCount;
	int trisCount;
	int normalCount;
	int UVcount;

	int matNameLen;
	char materialName[MAX_CHAR_LEN];
};

// Structs for RayLib ===============
struct modelPos {
	Model model;
	Matrix modelMatrix;
};

struct NodeDeleted {
	int nodeNameLen;
	char nodeName[MAX_CHAR_LEN];
};

struct NodeRenamed {
	int nodeNameLen;
	char nodeName[MAX_CHAR_LEN];

	int nodeOldNameLen;
	char nodeOldName[MAX_CHAR_LEN];
};

struct cameraFromMaya {
	int type;

	Vector3 up;
	Vector3 forward;
	Vector3 pos;
	float fov;
};

struct lightFromMaya {
	int lightNameLen;
	char lightName[MAX_CHAR_LEN];

	Vector3 lightPos;
	float intensityRadius;
	Color color;
};

struct modelFromMaya {
	int index;
	std::string name;

	Mesh mesh;
	Model model;
	Matrix modelMatrix;

	Color color;
	std::string materialName;
};

struct materialMaya {
	int type;		//color = 0, texture = 1 

	int matNameLen;
	char materialName[MAX_CHAR_LEN];

	int textureNameLen;
	char fileTextureName[MAX_CHAR_LEN];

	Color color;
};

struct materialFromMaya {
	int type; 
	std::string materialName; 
	std::string texturePath;

	Color matColor; 
	Shader matShader; 
};

struct transformFromMaya {
	char transfName[MAX_CHAR_LEN];
	int transfNameLen;

	char childName[MAX_CHAR_LEN];
	int childNameLen;
};


void addNode(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);
void updateNode(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);
void deleteNode(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);
void updateNodeName(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);
void updateNodeMatrix(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);
void updateNodeMaterial(MsgHeader &msgHeader, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArrr, std::vector<Shader> &shaderArr);

void newMaterial(MsgHeader &msgHeader,std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);
void updateMaterial(MsgHeader &msgHeader,std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialMaya>& materialArray, char* buffer, int* index, std::vector<Texture2D> &textureArr, std::vector<Shader> &shaderArr);


typedef void(*FnPtr)(MsgHeader&, std::vector<modelFromMaya>&, std::vector<lightFromMaya>&, std::vector<cameraFromMaya>&, std::vector<materialMaya>&, char*, int*, std::vector<Texture2D>&, std::vector<Shader>&);

int findMesh(std::vector<modelFromMaya>& modelArr, std::string MeshName);
int findMaterial(std::vector<materialMaya>& matArr, std::string MatName);


void createModel(); 


#endif
