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
	MESH,
	CAMERA,
	MATERIAL,
	MESH_MATERIAL
};

enum CMDTYPE {
	DEFAULT = 1000,

	NEW_NODE	= 1001,
	UPDATE_NODE = 1002,

	UPDATE_CAMERA = 1003,
	UPDATE_MATRIX = 1004,
	UPDATE_NAME   = 1005,

	DELETE_NODE = 1006
};

struct MsgHeader {
	NODE_TYPE nodeType;
	char objName[64];
	int msgSize;
	int nameLen;
};

struct messageType {
	CMDTYPE	cmdType;
	int msgNr;
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

struct msgMaterial {
	int type;		//color = 0, texture = 1 

	int matNameLen;
	char materialName[MAX_CHAR_LEN];

	int textureNameLen;
	char fileTextureName[MAX_CHAR_LEN];

	Color color;
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

struct materialFromMaya {
	int type;
	std::string materialName;
	std::string texturePath;

	Color matColor;
	Shader matShader;
	Texture2D matTexture; 
};

struct transformFromMaya {
	char transfName[MAX_CHAR_LEN];
	int transfNameLen;

	char childName[MAX_CHAR_LEN];
	int childNameLen;
};

void updateCamera(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);

void addNode(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);
void updateNode(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);
void deleteNode(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);
void updateNodeName(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);
void updateNodeMatrix(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);

//void updateNodeMaterial(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);

//void newMaterial(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);
//void updateMaterial(int nrMsg, std::vector<modelFromMaya>& modelArray, std::vector<lightFromMaya>& lightsArray, std::vector<cameraFromMaya>& cameraArray, std::vector<materialFromMaya>& materialArray, char* buffer);


typedef void(*FnPtr)(int, std::vector<modelFromMaya>&, std::vector<lightFromMaya>&, std::vector<cameraFromMaya>&, std::vector<materialFromMaya>&, char*);

int findMesh(std::vector<modelFromMaya>& modelArr, std::string MeshName);
int findMaterial(std::vector<materialFromMaya>& matArr, std::string MatName);


#endif
