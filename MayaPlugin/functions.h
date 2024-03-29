#pragma once
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "maya_includes.h"

#define MAX_CHAR_LEN 64

// ===========================================================
// ================ Structs and ENUMs ========================
// ===========================================================

struct Vec3 {
	float x;
	float y;
	float z;
};

struct Color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

struct Matrix {
	float a11, a12, a13, a14;
	float a21, a22, a23, a24;
	float a31, a32, a33, a34;
	float a41, a42, a43, a44;
};

// ===========================================================

// NODE TYPE for what kind of node that is sent
enum NODE_TYPE {
	MESH,
	CAMERA,
	MATERIAL,
	MESH_MATERIAL
};

// what kind of command is sent 
enum CMDTYPE {
	DEFAULT = 1000,

	NEW_NODE	= 1001,
	UPDATE_NODE = 1002,
	
	UPDATE_CAMERA = 1003,
	UPDATE_MATRIX = 1004,
	UPDATE_NAME   = 1005,

	DELETE_NODE	= 1006
};

// header for message that are sent
struct MsgHeader {
	NODE_TYPE nodeType;
	char objName[MAX_CHAR_LEN];
	int msgSize;
	int nameLen;
};

struct messageType {
	CMDTYPE	cmdType;
	int msgNr; 
};

// ===========================================================

struct Mesh {
	int meshID; 

	int vtxCount;
	int trisCount;
	int normalCount;
	int UVcount;

	int matNameLen; 
	char materialName[MAX_CHAR_LEN]; 
};

struct Camera {
	int type;

	Vec3 up;
	Vec3 forward;
	Vec3 pos;
	float fov;
};

struct Light {
	int lightNameLen; 
	char lightName[MAX_CHAR_LEN];

	Vec3 lightPos;
	float intensity;
	Color color;
};

struct Material {
	int type;		//color = 0, texture = 1 

	int matNameLen;
	char materialName[MAX_CHAR_LEN];

	int textureNameLen;
	char fileTextureName[MAX_CHAR_LEN];

	Color color;
};

struct Transform {
	char transfName[MAX_CHAR_LEN];
	int transfNameLen; 

	char childName[MAX_CHAR_LEN];
	int childNameLen;
};

struct NodeRenamed {
	int nodeNameLen;
	char nodeName[MAX_CHAR_LEN];

	int nodeOldNameLen;
	char nodeOldName[MAX_CHAR_LEN];
};



// ===========================================================
// ========== STRUCTS USED TO QUEUE MESSAGES =================
// ===========================================================

struct MeshInfo {
	//MsgHeader msgHeader; 

	MString meshName; 
	//MString meshPathName;

	float* meshVtx; 
	float* meshUVs;
	float* meshNrms; 

	Mesh meshData; 
	Material materialData; 
	Matrix transformMatrix; 
};

struct CameraInfo {
	MsgHeader msgHeader;
	Camera camData; 
};

struct LightInfo {
	MsgHeader msgHeader;
	Light lightData; 
};

struct MaterialInfo {
	MString matName; 
	Material materialData; 
};

struct TransformInfo {
	MsgHeader msgHeader;
	MString transfName;
	MString childShapeName; 

	Matrix transformMatrix; 
	Transform transfData; 
};

struct NodeDeletedInfo {
	int nodeIndex; 
	MsgHeader msgHeader;
	MString nodeName; 

};

struct NodeRenamedInfo {
	MsgHeader msgHeader;
	MString nodeOldName; 
	MString nodeNewName; 

	NodeRenamed renamedInfo; 
};

struct MatrixInfo {
	MString nodeName; 
	MsgHeader msgHeader;
	Matrix matrixData; 
};


struct NodeInfo {
	MsgHeader msgHeader;

	MeshInfo mesh;
	MaterialInfo material;
};

// ===========================================================
// ===================== MSG SIZES ===========================
// ===========================================================
size_t totalMsgSizeLight      = (sizeof(MsgHeader) + sizeof(Light));
size_t totalMsgSizeCamera     = (sizeof(MsgHeader) + sizeof(Camera));
size_t totalMsgSizeMatrix     = (sizeof(MsgHeader) + sizeof(Matrix));
size_t totalMsgSizeMaterial   = (sizeof(MsgHeader) + sizeof(Material));
size_t totalMsgSizeRenamed    = (sizeof(MsgHeader) + sizeof(NodeRenamed));
size_t totalMsgSizeMatAndMesh = (sizeof(MsgHeader) + sizeof(Mesh) + sizeof(Material));

// =====================================


#endif