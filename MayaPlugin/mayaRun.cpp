#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>

#include "comLib.h"
#pragma comment(lib, "Project1.lib")

using namespace std;
ComLib comLib("shaderMemory", 50, PRODUCER);


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
	TRANSFORM,
	MESH,
	LIGHT,
	CAMERA
};

// what kind of command is sent 
enum CMDTYPE {
	DEFAULT				= 1000,

	NEW_NODE			= 1001,
	UPDATE_NODE			= 1002,

	UPDATE_MATRIX		= 1003,
	UPDATE_NAME			= 1004,

	NEW_MATERIAL		= 1005,
	UPDATE_MATERIAL		= 1006,

	DELETE_NODE			= 1007,
	TRANSFORM_UPDATE	= 1008
};

// header for message that are sent
struct MsgHeader {
	CMDTYPE	  cmdType;
	NODE_TYPE nodeType;
	char objName[64];
	int msgSize;
	int nameLen;
};

// ===========================================================

// structs with info about the specific node sent
struct Mesh {
	int vtxCount;
	int trisCount;
	int normalCount;
	int UVcount;
};

struct Camera {
	Vec3 up;
	Vec3 forward;
	Vec3 pos;
	int type;
	float fov;
};

struct Light {
	Vec3 lightPos; 
	float intensity;
	Color color; 
};

struct Material {
	int type;					//color = 0, texture = 1 
	char materialName[64]; 
	char fileTextureName[64];
	int matNameLen; 
	int textureNameLen; 
	Color color;
};

struct Transf {
	char childName[64]; 
	int childNameLen; 
};

// ===========================================================
// ==================== Variables ============================
// ===========================================================

MStatus status = MS::kSuccess;
MCallbackIdArray callbackIdArray;
bool initBool = false;

MTimer gTimer;
float globalTime  = 0.0f;
float timerPeriod = 3.0f; 

// keep track of created meshes/ lights to maintain them
queue<MObject> meshQueue;
queue<MObject> lightQueue;
queue<MObject> transformQueue; 
queue<MObject> materialQueue; 

string oldContent = "";
string oldName	  = "";


// Output messages in output window
//MStreamUtils::stdOutStream() << "...: " << ... << "_" << endl;


// ==================================================================================
// ============================== MATERIAL ==========================================
// ==================================================================================
 /*
// callback function for when a texture is connected to the material node on a mesh
void nodeTextureAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {
	
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << " ===================================" << endl;
	MStreamUtils::stdOutStream() << "NODE TEX ATTR CHANGED" << endl;
	
	Material materialInfo = {}; 
	MObject textureObj(plug.node());

	std::string materialName; 
	std::string fileNameString; 

	 //find texture filepath
	MPlugArray connections;
	plug.connectedTo(connections, true, true);

	if (connections.length() > 0) {

		std::string materialNamePlug = connections[0].name().asChar();
		if (materialNamePlug.length() > 0) {

			//if plug exists split to get material name
			std::string splitElement = ".";
			materialName = materialNamePlug.substr(0, materialNamePlug.find(splitElement));

			//get filepath
			MFnDependencyNode textureNode(textureObj);
			MPlug fileTextureName = textureNode.findPlug("ftn");

			MString fileName;
			fileTextureName.getValue(fileName);
			fileNameString = fileName.asChar();

			// make sure that there is a texture by checking texturepath
			if (fileNameString.length() > 0) {

				// fill struct with information
				materialInfo.type = 1; 
				materialInfo.color = {255, 255, 255, 255};
				
				materialInfo.textureNameLen = fileNameString.length();
				memcpy(materialInfo.fileTextureName, fileNameString.c_str(), materialInfo.textureNameLen);
				
				materialInfo.matNameLen = materialName.length();
				memcpy(materialInfo.materialName, materialName.c_str(), materialInfo.matNameLen);

				// create message ptr
				size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));
				const char* msg = new char[totalMsgSize];
				
				// Fill header ========
				MsgHeader msgHeader;
				msgHeader.msgSize = totalMsgSize;
				msgHeader.nodeType = NODE_TYPE::MESH;
				msgHeader.nameLen = 2;
				msgHeader.cmdType = CMDTYPE::NEW_MATERIAL;
				memcpy(msgHeader.objName, "NA", msgHeader.nameLen);
				

				// copy over msg ======
				memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
				memcpy((char*)msg + sizeof(MsgHeader), &materialInfo, sizeof(Material));

				
				//send it
				if (comLib.send(msg, totalMsgSize)) {
					MStreamUtils::stdOutStream() << "nodeTextureAttributeChanged: Message sent" << "\n";
				}

				delete[] msg; 

			}
		}
	}

}

// callback function for when a material attribute (ex color) changes
void nodeMaterialAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

		MStreamUtils::stdOutStream() << endl;
		MStreamUtils::stdOutStream() << " ===================================" << endl;

		MStreamUtils::stdOutStream() << "NODE MAT ATTR CHANGED" << endl;
		MStreamUtils::stdOutStream() << "msg " << msg << endl;
		MStreamUtils::stdOutStream() << "plug " << plug.name() << endl;
		MStreamUtils::stdOutStream() << "other plug " << otherPlug.name() << endl;

		
		MColor color;
		MCallbackId tempID;
		Material materialInfo = {};

		MObject lamObj(plug.node());
		MFnDependencyNode lambertDepNode(lamObj);


		//check if callback lamert has texture or color
		MPlugArray connections;
		bool hasTexture = false;
		MPlug colorPlug = lambertDepNode.findPlug("color");
		colorPlug.connectedTo(connections, true, false);

		// if node has a texture
		for (int x = 0; x < connections.length(); x++) {
			if (connections[x].node().apiType() == MFn::kFileTexture) {

				//if a texture node found, create a callback for the texture
				MObject textureObj(connections[x].node());
				//tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
				//callbackIdArray.append(tempID);

				hasTexture = true;
			}
		}

		// find the material and color of lambert
		if ((plug.node().hasFn(MFn::kLambert)) && (hasTexture == false)) {		
			if (plug.name() == colorPlug.name()) {

				//get material color
				lambertDepNode.setObject(lamObj);
				MPlug attr;

				attr = lambertDepNode.findPlug("colorR");
				attr.getValue(color.r);
				attr = lambertDepNode.findPlug("colorG");
				attr.getValue(color.g);
				attr = lambertDepNode.findPlug("colorB");
				attr.getValue(color.b);

				// convert from 0-1 to 0-255
				int red = color.r * 256;
				int blue = color.b * 256;
				int green = color.g * 256;
				int alpha = color.a * 256;

				Color tempColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };
				materialInfo.color = tempColor;

				MStreamUtils::stdOutStream() << "COLOR: " << red << " : " << green << " : " << blue << " : " << alpha << endl;

				// fill material struct
				materialInfo.type = 0;
				materialInfo.color = tempColor;

				materialInfo.textureNameLen = 2;
				memcpy(materialInfo.fileTextureName, "NA", materialInfo.textureNameLen);

				materialInfo.matNameLen = lambertDepNode.name().length();
				memcpy(materialInfo.materialName, lambertDepNode.name().asChar(), materialInfo.matNameLen);

				// create message ptr
				size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));
				const char* msgComLib = new char[totalMsgSize];

				// Fill header ========
				MsgHeader msgHeader;
				msgHeader.msgSize = totalMsgSize;
				msgHeader.nodeType = NODE_TYPE::MESH;
				msgHeader.nameLen = 2;
				msgHeader.cmdType = CMDTYPE::NEW_MATERIAL;
				memcpy(msgHeader.objName, "NA", msgHeader.nameLen);

				// copy over msg ======
				memcpy((char*)msgComLib, &msgHeader, sizeof(MsgHeader));
				memcpy((char*)msgComLib + sizeof(MsgHeader), &materialInfo, sizeof(Material));


				//send it
				if (comLib.send(msgComLib, totalMsgSize)) {
					MStreamUtils::stdOutStream() << "nodeMaterialAttributeChanged (color): Message sent" << "\n";

				}

				delete[] msgComLib;
			}
		}

}

// callback used for when a change is made for materials and textures
void meshConnectionChanged(MPlug &plug, MPlug &otherPlug, bool made, void *clientData) {

	// a connection was made
	//if (made == true) {
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << " ===================================" << endl;
	MStreamUtils::stdOutStream() << "MESH CONNECTION CHANGED (TEXTURE)" << endl;
	MStreamUtils::stdOutStream() << "plug name: " << plug.name() << endl;
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << "otherPlug name: " << otherPlug.name() << endl;
	MStreamUtils::stdOutStream() << endl;

	// get mesh through path
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	Material materialInfo = {};
	MCallbackId tempID;


	//////////////////////////////////
	//								//
	//	 MATERIALS AND TEXTURES 	//
	//								//
	//////////////////////////////////

	//find the meshShape the connection was connected to.
	// OBS when connecting material to obj the material is connected to both a shaderball and well
	std::string meshName = mesh.name().asChar();
	std::string testString = "shaderBallGeomShape";

	bool shaderBall = false;
	if (meshName.find(testString) == std::string::npos) {
		shaderBall = false;
	}
	else {
		shaderBall = true;
	}

	// if it's not shaderBall, the connection was madeto the mesh shape
	bool hasTexture = false;
	if (!shaderBall) {

		//get shader groups (only one shading group is supported) 
		MObjectArray shaderGroups;
		MIntArray shaderGroupsIndecies;
		mesh.getConnectedShaders(0, shaderGroups, shaderGroupsIndecies);

		MStreamUtils::stdOutStream() << "shaderGroups.length() " << shaderGroups.length() << endl;

		if (shaderGroups.length() > 0) {

			//MStreamUtils::stdOutStream() << "Found shader grp" << endl;

			//after shader group is found, find surface shader (connected to attributes)
			MFnDependencyNode shaderNode(shaderGroups[0]);
			MPlug surfaceShader = shaderNode.findPlug("surfaceShader");

			// Get the connections to find material
			MPlugArray shaderNodeconnections;
			surfaceShader.connectedTo(shaderNodeconnections, true, false);

			for (int j = 0; j < shaderNodeconnections.length(); j++) {

				//go though connection and find lambert material
				if (shaderNodeconnections[j].node().apiType() == MFn::kLambert) {

					//create callback for the material found
					MObject lambertObj(shaderNodeconnections[j].node());
					//tempID = MNodeMessage::addAttributeChangedCallback(lambertObj, nodeMaterialAttributeChanged);
					//callbackIdArray.append(tempID);

					// get color plug to check if its has a connection or not. 
					// If connection there is a texture, else just get the color
					MFnDependencyNode lambertDepNode(lambertObj);
					MPlugArray connetionsColor;

					MPlug colorPlug = lambertDepNode.findPlug("color");
					colorPlug.connectedTo(connetionsColor, true, false);

					//if connectionsColor > 0 there is a texutre
					for (int x = 0; x < connetionsColor.length(); x++) {

						// found fileTexture node
						if (connetionsColor[x].node().apiType() == MFn::kFileTexture) {

							MStreamUtils::stdOutStream() << "Texture was found" << endl;

							
							MObject textureObj(connetionsColor[x].node());
							//tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeMaterialAttributeChanged);
							//callbackIdArray.append(tempID);
					

							//get the filepath, search for plug name 
							MFnDependencyNode textureNode(textureObj);
							MPlug fileTextureName = textureNode.findPlug("ftn");

							MString fileName;
							fileTextureName.getValue(fileName);
							std::string fileNameString = fileName.asChar();

							if (fileNameString.length() > 0) {

								// fill struct with information
								materialInfo.type = 1;
								materialInfo.color = { 255, 255, 255, 255 };

								materialInfo.textureNameLen = fileNameString.length();
								memcpy(materialInfo.fileTextureName, fileNameString.c_str(), materialInfo.textureNameLen);

								materialInfo.matNameLen = lambertDepNode.name().length();
								memcpy(materialInfo.materialName, lambertDepNode.name().asChar(), materialInfo.matNameLen);

								// create message ptr
								size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));
								const char* msg = new char[totalMsgSize];

								// Fill header ========
								MsgHeader msgHeader;
								msgHeader.msgSize = totalMsgSize;
								msgHeader.nodeType = NODE_TYPE::MESH;
								msgHeader.nameLen = mesh.name().length();
								msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
								memcpy(msgHeader.objName, mesh.name().asChar(), msgHeader.nameLen);


								// copy over msg ======
								memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
								memcpy((char*)msg + sizeof(MsgHeader), &materialInfo, sizeof(Material));

								if (oldContent != msg) {
									//send it
									if (comLib.send(msg, totalMsgSize)) {
										MStreamUtils::stdOutStream() << "meshConnectionChanged: Message sent" << "\n";
									}
								}

								oldContent = msg;
								delete[]msg;



							}

							hasTexture = true;
						}
					}

					if (hasTexture == false) {

						//get all the rgb values for the color plug if no texture was found
						MColor color;
						MPlug attr;

						attr = lambertDepNode.findPlug("colorR");
						attr.getValue(color.r);
						attr = lambertDepNode.findPlug("colorG");
						attr.getValue(color.g);
						attr = lambertDepNode.findPlug("colorB");
						attr.getValue(color.b);

						// convert from 0-1 to 0-255
						Color tempColor = {};
						int red = color.r * 256;
						int blue = color.b * 256;
						int green = color.g * 256;
						int alpha = color.a * 256;

						tempColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };

						// fill material struct
						materialInfo.type = 0;
						materialInfo.color = tempColor;

						materialInfo.textureNameLen = 2;
						memcpy(materialInfo.fileTextureName, "NA", materialInfo.textureNameLen);

						materialInfo.matNameLen = lambertDepNode.name().length();
						memcpy(materialInfo.materialName, lambertDepNode.name().asChar(), materialInfo.matNameLen);

						// create message ptr
						size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));
						const char* msg = new char[totalMsgSize];

						// Fill header ========
						MsgHeader msgHeader;
						msgHeader.msgSize = totalMsgSize;
						msgHeader.nodeType = NODE_TYPE::MESH;
						msgHeader.nameLen = mesh.name().length();
						msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
						memcpy(msgHeader.objName, mesh.name().asChar(), msgHeader.nameLen);

						// copy over msg ======
						memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
						memcpy((char*)msg + sizeof(MsgHeader), &materialInfo, sizeof(Material));

						if (oldContent != msg) {

							//send it
							if (comLib.send(msg, totalMsgSize)) {
								MStreamUtils::stdOutStream() << "meshConnectionChanged: Message sent" << "\n";
							}
						}

						oldContent = msg;
						delete[] msg;


					}

				}

			}

		}

	}


	//MStreamUtils::stdOutStream() << " ===================================" << endl;


}

//	Callback used for when a new material is created and connected to a mesh
void nodeNameChangedMaterial(MObject &node, const MString &str, void*clientData) {

	MStreamUtils::stdOutStream() << "\n";
	MStreamUtils::stdOutStream() << " ===================================" << endl;
	MStreamUtils::stdOutStream() << "NODE NAME CHANGED MATERIAL" << "\n";

	// Get the new name of the node, and use it from now on.
	MString oldName = str;
	MString newName = MFnDagNode(node).name();

	std::string objName	   = oldName.asChar();
	std::string newObjName = newName.asChar();

	int newNameLen    = newObjName.length(); 
	char* newNameChar = new char[newNameLen];
	memcpy(newNameChar, newObjName.c_str(), newNameLen);


	// create message ptr
	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(int) + (sizeof(char) * newNameLen));
	const char* msg = new char[totalMsgSize];

	// Fill header ========
	MsgHeader msgHeader;
	msgHeader.msgSize  = totalMsgSize;
	msgHeader.nodeType = NODE_TYPE::MESH;
	msgHeader.nameLen  = objName.length();
	msgHeader.cmdType  = CMDTYPE::UPDATE_NAME;
	memcpy(msgHeader.objName, objName.c_str(), msgHeader.nameLen);
	
	// copy over msg ======
	memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
	memcpy((char*)msg + sizeof(MsgHeader), &newNameLen, sizeof(int));
	memcpy((char*)msg + sizeof(MsgHeader) + sizeof(int), newNameChar, (sizeof(char) * newNameLen));

	
	if (oldContent != msg) {
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "nodeNameChangedMaterial: Message sent" << "\n";
		}
	}

	
	oldContent = msg;
	delete[]msg;
	delete[]newNameChar; 
	
	//MStreamUtils::stdOutStream() << " ===================================" << endl;
	//MStreamUtils::stdOutStream() << "\n";

}
 */

void materialAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	//MStreamUtils::stdOutStream() << endl;

	//MStreamUtils::stdOutStream() << "==================================" << endl;
	//MStreamUtils::stdOutStream() << "MATERILA ATTR CHANGED" << endl;
	//MStreamUtils::stdOutStream() << endl;

	if ((msg & MNodeMessage::kAttributeSet) && (plug.node().hasFn(MFn::kLambert))){ // && (msg != 2052)) {
		
		//MStreamUtils::stdOutStream() << "Connection made or attr set" << endl;
		
		//MStreamUtils::stdOutStream() << "plug " << plug.name() << endl;
		//MStreamUtils::stdOutStream() << "otherPlug " << otherPlug.name() << endl;
		// plug: lambert1.message || otherP: swatchShadingGroup.surfaceShader - clr?
		//MStreamUtils::stdOutStream() << "msg " << msg << endl;

	
		MStatus result;

		// lambert material 
		MColor color;
		MPlug colorPlug;
		MFnLambertShader lambertShader;

		// texture 
		bool hasTexture = false;
		std::string fileNameString;
		MPlugArray textureConnections;
		

		lambertShader.setObject(plug.node()); 

		// get lambert color through Color plug
		colorPlug = lambertShader.findPlug("color", &result);

		if (result) {

			MPlug colorAttr;
			colorAttr = lambertShader.findPlug("colorR");
			colorAttr.getValue(color.r);
			colorAttr = lambertShader.findPlug("colorG");
			colorAttr.getValue(color.g);
			colorAttr = lambertShader.findPlug("colorB");
			colorAttr.getValue(color.b);
			//MStreamUtils::stdOutStream() << endl;
			//MStreamUtils::stdOutStream() << "COLOR: " << color << endl;

			// to check for textures, check if color plug is dest for another plug
			colorPlug.connectedTo(textureConnections, true, false);
			if (textureConnections.length() > 0) {

				//get filepath
				MFnDependencyNode textureNode(textureConnections[0].node());
				MPlug fileTextureName = textureNode.findPlug("ftn");

				MString fileName;
				fileTextureName.getValue(fileName);
				fileNameString = fileName.asChar();

				//MStreamUtils::stdOutStream() << "fileName: " << fileNameString << endl;
				hasTexture = true;
			}
		}

	}



}
// ==================================================================================
// =============================== TRANSFORM ========================================
// ==================================================================================

/* 
// callback for attribute change of transform (matrix)
void nodeTransformAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	if (msg & MNodeMessage::kAttributeSet) {

		//MStreamUtils::stdOutStream() << "nodeTransformAttributeChanged " << endl;

		//MStreamUtils::stdOutStream() << "msg " << msg << endl;
		//MStreamUtils::stdOutStream() << "plug  " << plug.name() << endl;
		//MStreamUtils::stdOutStream() << "plug part " << plug.partialName() << endl;
		//MStreamUtils::stdOutStream() << "other plug " << otherPlug.name() << endl;
		
		MDagPath path;
		MFnDagNode(plug.node()).getPath(path);
		MFnTransform transf(path);

		Transf transformInfo = {}; 
		MFnTransform transform(plug.node());
		std::string transfName = transform.name().asChar(); 

		if (transform.child(0).hasFn(MFn::kMesh)) {
			MFnMesh mesh(transform.child(0));

			transformInfo.childNameLen = mesh.name().length(); 
			memcpy(transformInfo.childName, mesh.name().asChar(), transformInfo.childNameLen);

		}

		else {
			transformInfo.childNameLen = 2;
			memcpy(transformInfo.childName, "NA", transformInfo.childNameLen);
		}

		MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

		// fill matrix struct to send 
		// matrix is transposed before sending to raylib
		Matrix matrixInfo = {};

		//row 1
		matrixInfo.a11 = worldMatrix(0, 0);
		matrixInfo.a12 = worldMatrix(1, 0);
		matrixInfo.a13 = worldMatrix(2, 0);
		matrixInfo.a14 = worldMatrix(3, 0);

		//row 2
		matrixInfo.a21 = worldMatrix(0, 1);
		matrixInfo.a22 = worldMatrix(1, 1);
		matrixInfo.a23 = worldMatrix(2, 1);
		matrixInfo.a24 = worldMatrix(3, 1);

		//row 3
		matrixInfo.a31 = worldMatrix(0, 2);
		matrixInfo.a32 = worldMatrix(1, 2);
		matrixInfo.a33 = worldMatrix(2, 2);
		matrixInfo.a34 = worldMatrix(3, 2);

		//row 4
		matrixInfo.a41 = worldMatrix(0, 3);
		matrixInfo.a42 = worldMatrix(1, 3);
		matrixInfo.a43 = worldMatrix(2, 3);
		matrixInfo.a44 = worldMatrix(3, 3);

		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Transf) + sizeof(Matrix));
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nodeType = NODE_TYPE::TRANSFORM;
		msgHeader.nameLen = transfName.length();
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, transfName.c_str(), transfName.length());

		// copy over msg ======
		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &transformInfo, sizeof(Transf));
		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Transf), &matrixInfo, sizeof(Matrix));

		if (oldContent != msg) {
			//send it
			if (comLib.send(msg, totalMsgSize)) {
				MStreamUtils::stdOutStream() << "nodeTransformAttributeChanged: Message sent" << "\n";
			}
		}

		oldContent = msg; 
		delete[] msg; 
	

	}
}

// callback for if names was changed for mesh or transform 
void NodeNameChanged(MObject& node, const MString& str, void* clientData) {

	std::string oldName;
	std::string newName;

	bool sendMsg = false; 
	NODE_TYPE nodeType; 

	if (node.hasFn(MFn::kMesh)) {
		MFnMesh mesh(node);

		oldName = str.asChar();
		newName = mesh.fullPathName().asChar();

		//MStreamUtils::stdOutStream() << "Name changed from " << oldName << " to " << newName << endl;
		nodeType = NODE_TYPE::MESH; 
		sendMsg = true; 
	}

	else if (node.hasFn(MFn::kTransform)) {

		MFnTransform transform(node);

		oldName = str.asChar(); 
		newName = transform.fullPathName().asChar(); 

		//MStreamUtils::stdOutStream() << "Name changed from " << oldName << " to " << newName << endl;
		nodeType = NODE_TYPE::TRANSFORM;
		sendMsg = true; 

	}

	if (sendMsg == true) {

		int newNameLen = newName.length();
		char* newNameChar = new char[newNameLen];
		memcpy(newNameChar, newName.c_str(), newNameLen);


		// create message ptr
		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(int) + (sizeof(char) * newNameLen));
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nodeType = nodeType;
		msgHeader.nameLen = oldName.length();
		msgHeader.cmdType = CMDTYPE::UPDATE_NAME;
		memcpy(msgHeader.objName, oldName.c_str(), oldName.length());


		// copy over msg ======
		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &newNameLen, sizeof(int));
		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(int), newNameChar, sizeof(char) * newNameLen);

		
			//send it
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "NodeNameChanged: Message sent" << "\n";
		}
		

		
		delete[] msg;
	}

}
*/

// ==================================================================================
// ================================= MESH ===========================================
// ==================================================================================

void GetMeshInfo(MFnMesh &mesh) {

	MStreamUtils::stdOutStream() << endl;

	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "GET MESH INFO" << endl;
	MStreamUtils::stdOutStream() << endl;
	MStatus result;

	// mesh information (vtx, UV, norm) ===========
	MItMeshPolygon polyIterator(mesh.object(), &result);
	int totalTrisCnt = 0; 
	int trisCurrentFace = 0; 

	MPointArray points;
	MIntArray vertexList;

	MFloatArray uvArray;
	MPointArray vtxArray;
	MFloatVectorArray normalArray; 

	// Checks if the mesh has a valid triangulation. If it doesn't, then the mesh was bad geometry
	if (polyIterator.hasValidTriangulation()) {
		while (!polyIterator.isDone()) {

			MStreamUtils::stdOutStream() << endl;

			// get the tris count for the face that it's currently iterating through
			polyIterator.numTriangles(trisCurrentFace);
			
			// get vtx and vtx positions of all the triangles in the current face
			polyIterator.getTriangles(points, vertexList);

			// get points, normals and UVs 
			for (int i = 0; i < points.length(); i++) {
				
				vtxArray.append(points[i]);

				float2 tempUV; 
				polyIterator.getUVAtPoint(points[i], tempUV);
				uvArray.append(tempUV[0]);
				uvArray.append(1.0 - tempUV[1]);

				MVector tempNormal; 
				polyIterator.getNormal(tempNormal); 
				normalArray.append(tempNormal);

			}


			MStreamUtils::stdOutStream() << "trisCurrentFace: " << trisCurrentFace << endl;			
			totalTrisCnt += trisCurrentFace; 
			polyIterator.next();
		}
	}
	
	MStreamUtils::stdOutStream() << "vtxArray len: " << vtxArray.length() << endl;
	MStreamUtils::stdOutStream() << "normalArray len: " << normalArray.length() << endl;
	MStreamUtils::stdOutStream() << "uvArray len: " << uvArray.length() << endl;
	MStreamUtils::stdOutStream() << "totalTrisCnt: " << totalTrisCnt << endl;
	
	// transform matrix ===========================
	MFnTransform parentTransform(mesh.parent(0), &result); 

	if (result) {

		MStreamUtils::stdOutStream() << "parentTransform: " << parentTransform.name() << endl;
		MMatrix transformationMatrix = parentTransform.transformationMatrix(); 
	
		MStreamUtils::stdOutStream() << "transformationMatrix: " << transformationMatrix << endl;
	}

	// material ===================================
	
	// lambert material 
	MColor color;
	MPlug colorPlug;
	MFnLambertShader lambertShader;

	MIntArray indices;
	MPlugArray shaderConnections;
	MObjectArray connectedShaders;

	// texture 
	bool hasTexture = false; 
	std::string fileNameString; 
	MPlugArray textureConnections;

	// get the connected shaders to the mesh
	mesh.getConnectedShaders(0, connectedShaders, indices);

	if(connectedShaders.length() > 0) {
		
		// if there are any connected shaders, get first one
		MFnDependencyNode initialShadingGroup(connectedShaders[0]);
		MStreamUtils::stdOutStream() << "Shader: " << initialShadingGroup.name() << endl;

		MPlug surfaceShader = initialShadingGroup.findPlug("surfaceShader", &result); 

		if (result) {
			MStreamUtils::stdOutStream() << "Found plug!" << endl;

			// to find material, find the destination connections for surficeShader plug
			// (AKA plugs that have surface shader as destination)
			surfaceShader.connectedTo(shaderConnections, true, false); 

			MStreamUtils::stdOutStream() << "shaderConnections: " << shaderConnections.length() << endl;

			if (shaderConnections.length() > 0) {

				//Only support 1 material per mesh + lambert. Check if first connection is lambert
				if (shaderConnections[0].node().apiType() == MFn::kLambert) {

					lambertShader.setObject(shaderConnections[0].node()); 
					std::string materialNamePlug = lambertShader.name().asChar(); 
					MStreamUtils::stdOutStream() << "materialNamePlug: " << materialNamePlug << endl;
					
					// get lambert color through Color plug
					colorPlug = lambertShader.findPlug("color", &result);

					if (result) {

						MPlug colorAttr; 
						colorAttr = lambertShader.findPlug("colorR");
						colorAttr.getValue(color.r);
						colorAttr = lambertShader.findPlug("colorG");
						colorAttr.getValue(color.g);
						colorAttr = lambertShader.findPlug("colorB");
						colorAttr.getValue(color.b);

						MStreamUtils::stdOutStream() << "COLOR: " << color << endl;

						// to check for textures, check if color plug is dest for another plug
						colorPlug.connectedTo(textureConnections, true, false);
						if (textureConnections.length() > 0) {

							//get filepath
							MFnDependencyNode textureNode(textureConnections[0].node());
							MPlug fileTextureName = textureNode.findPlug("ftn");

							MString fileName;
							fileTextureName.getValue(fileName);
							fileNameString = fileName.asChar();

							MStreamUtils::stdOutStream() << "fileName: " << fileNameString << endl;
							hasTexture = true; 
						}

					}
					
				}	
			}


		


		}
	}


}

void GeometryUpdate(MFnMesh &mesh) {

	MStreamUtils::stdOutStream() << endl;

	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "GEOMETRY INFO" << endl;
	MStreamUtils::stdOutStream() << endl;

	MStatus result;

	// mesh information (vtx, UV, norm) ===========
	MItMeshPolygon polyIterator(mesh.object(), &result);
	int totalTrisCnt = 0;
	int trisCurrentFace = 0;

	MPointArray points;
	MIntArray vertexList;

	MFloatArray uvArray;
	MPointArray vtxArray;
	MFloatVectorArray normalArray;

	// Checks if the mesh has a valid triangulation. If it doesn't, then the mesh was bad geometry
	if (polyIterator.hasValidTriangulation()) {
		while (!polyIterator.isDone()) {

			MStreamUtils::stdOutStream() << endl;

			// get the tris count for the face that it's currently iterating through
			polyIterator.numTriangles(trisCurrentFace);

			// get vtx and vtx positions of all the triangles in the current face
			polyIterator.getTriangles(points, vertexList);

			// get points, normals and UVs 
			for (int i = 0; i < points.length(); i++) {

				vtxArray.append(points[i]);

				float2 tempUV;
				polyIterator.getUVAtPoint(points[i], tempUV);
				uvArray.append(tempUV[0]);
				uvArray.append(1.0 - tempUV[1]);

				MVector tempNormal;
				polyIterator.getNormal(tempNormal);
				normalArray.append(tempNormal);

			}


			MStreamUtils::stdOutStream() << "trisCurrentFace: " << trisCurrentFace << endl;
			totalTrisCnt += trisCurrentFace;
			polyIterator.next();
		}
	}

	MStreamUtils::stdOutStream() << "vtxArray len: " << vtxArray.length() << endl;
	MStreamUtils::stdOutStream() << "normalArray len: " << normalArray.length() << endl;
	MStreamUtils::stdOutStream() << "uvArray len: " << uvArray.length() << endl;
	MStreamUtils::stdOutStream() << "totalTrisCnt: " << totalTrisCnt << endl;

}

void MaterialChanged(MFnMesh &mesh) {

	MStreamUtils::stdOutStream() << endl;

	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "MATERIAL CHANGED" << endl;
	MStreamUtils::stdOutStream() << endl;

	MStatus result;

	// lambert material 
	MColor color;
	MPlug colorPlug;
	MFnLambertShader lambertShader;

	MIntArray indices;
	MPlugArray shaderConnections;
	MObjectArray connectedShaders;

	// texture 
	bool hasTexture = false;
	std::string fileNameString;
	MPlugArray textureConnections;

	// get the connected shaders to the mesh
	mesh.getConnectedShaders(0, connectedShaders, indices);

	if (connectedShaders.length() > 0) {

		// if there are any connected shaders, get first one
		MFnDependencyNode initialShadingGroup(connectedShaders[0]);
		MStreamUtils::stdOutStream() << "Shader: " << initialShadingGroup.name() << endl;

		MPlug surfaceShader = initialShadingGroup.findPlug("surfaceShader", &result);

		if (result) {
			MStreamUtils::stdOutStream() << "Found plug!" << endl;

			// to find material, find the destination connections for surficeShader plug
			// (AKA plugs that have surface shader as destination)
			surfaceShader.connectedTo(shaderConnections, true, false);

			MStreamUtils::stdOutStream() << "shaderConnections: " << shaderConnections.length() << endl;

			if (shaderConnections.length() > 0) {

				//Only support 1 material per mesh + lambert. Check if first connection is lambert
				if (shaderConnections[0].node().apiType() == MFn::kLambert) {

					lambertShader.setObject(shaderConnections[0].node());
					std::string materialNamePlug = lambertShader.name().asChar();
					MStreamUtils::stdOutStream() << "materialNamePlug: " << materialNamePlug << endl;

					// get lambert color through Color plug
					colorPlug = lambertShader.findPlug("color", &result);

					if (result) {

						MPlug colorAttr;
						colorAttr = lambertShader.findPlug("colorR");
						colorAttr.getValue(color.r);
						colorAttr = lambertShader.findPlug("colorG");
						colorAttr.getValue(color.g);
						colorAttr = lambertShader.findPlug("colorB");
						colorAttr.getValue(color.b);

						MStreamUtils::stdOutStream() << "COLOR: " << color << endl;

						// to check for textures, check if color plug is dest for another plug
						colorPlug.connectedTo(textureConnections, true, false);
						if (textureConnections.length() > 0) {

							//get filepath
							MFnDependencyNode textureNode(textureConnections[0].node());
							MPlug fileTextureName = textureNode.findPlug("ftn");

							MString fileName;
							fileTextureName.getValue(fileName);
							fileNameString = fileName.asChar();

							MStreamUtils::stdOutStream() << "fileName: " << fileNameString << endl;
							hasTexture = true;
						}

					}

				}
			}



		}
	}



}

void PolySplitMesh(MPlug &plug, MPlug &otherPlug) {

	MStreamUtils::stdOutStream() << " =============================== " << endl;
	MStreamUtils::stdOutStream() << "IN POLYSPLIT" << endl;

	MStatus result;
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnMesh meshNode(path, &result);


	MStreamUtils::stdOutStream() << "meshNode: " << meshNode.name() << endl;

}

// ============================== CALLBACKS ========================================
void nodePlugConnected(MPlug & plug, MPlug & otherPlug, bool made, void* clientData) {
	
	if (plug.partialName() == "out" && otherPlug.partialName() == "i") {

		//if connection made
		if (made == true) {

			
			MStreamUtils::stdOutStream() << "\n";
			MStreamUtils::stdOutStream() << "A connection was made" << endl;

			MStreamUtils::stdOutStream() << "plug.name: " << plug.name() << endl;
			MStreamUtils::stdOutStream() << "plug.partname: " << plug.partialName() << endl;

			MStreamUtils::stdOutStream() << "otherPlug.name: " << otherPlug.name() << endl;

			// check if mesh is triangulated or not =====================
			MStatus res;
			MDagPath path;
			MFnDagNode(otherPlug.node()).getPath(path);
			MFnMesh mesh(path, &res);

			std::string plugName = plug.name().asChar(); 
			std::string otherPlugName = plug.name().asChar(); 

			// check if mesh is triangulated or not =====================
			MPlugArray plugArray;
			bool meshTriangulated = false; 
			otherPlug.connectedTo(plugArray, true, true);
			/* 
			if (plugName.find("polyTweak") != std::string::npos){
				MStreamUtils::stdOutStream() << "PolyTweak" << endl;

			}

			if (plugName.find("UV") == std::string::npos && res) {

				if (plugArray.length() > 0) {
					std::string name = plugArray[0].name().asChar();

					if (name.find("polyTriangulate") == std::string::npos) {
						status = MGlobal::executeCommand("polyTriangulate " + mesh.name(), true, true);
						MStreamUtils::stdOutStream() << "TRANGULATING" << endl;
						meshTriangulated = true; 
					}
				}
			}
		
			if (meshTriangulated) {

				MStreamUtils::stdOutStream() << "mesh was Triangulated" << endl;

				// VTX ================
				// get verticies 
				MPointArray vtxArray;
				mesh.getPoints(vtxArray, MSpace::kObject);

				// get triangles
				MIntArray trisCount;
				MIntArray trisVtxIndex;
				mesh.getTriangles(trisCount, trisVtxIndex);

				size_t nrOfTris = trisCount.length();
				MStreamUtils::stdOutStream() << "nrOfTris: " << nrOfTris << endl;

				
			}
			*/

		}
	}
}

// callback for when an attribute is changed on a node
void meshAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	/*
	MStreamUtils::stdOutStream() << " =============================== " << endl;

	MStreamUtils::stdOutStream() << "MSG TYPES: " << endl;
	MStreamUtils::stdOutStream() << "kConnectionMade " << MNodeMessage::AttributeMessage::kConnectionMade << endl;
	MStreamUtils::stdOutStream() << "kConnectionBroken " << MNodeMessage::AttributeMessage::kConnectionBroken << endl;
	MStreamUtils::stdOutStream() << "kAttributeEval " << MNodeMessage::AttributeMessage::kAttributeEval<< endl;
	MStreamUtils::stdOutStream() << "kAttributeSet " << MNodeMessage::AttributeMessage::kAttributeSet << endl;
	MStreamUtils::stdOutStream() << "kAttributeLocked  " << MNodeMessage::AttributeMessage::kAttributeLocked << endl;
	MStreamUtils::stdOutStream() << "kAttributeUnlocked  " << MNodeMessage::AttributeMessage::kAttributeUnlocked << endl;
	MStreamUtils::stdOutStream() << "kAttributeAdded  " << MNodeMessage::AttributeMessage::kAttributeAdded << endl;
	MStreamUtils::stdOutStream() << "kAttributeRemoved  " << MNodeMessage::AttributeMessage::kAttributeRemoved << endl;
	MStreamUtils::stdOutStream() << "kAttributeRenamed  " << MNodeMessage::AttributeMessage::kAttributeRenamed << endl;
	MStreamUtils::stdOutStream() << "kAttributeKeyable  " << MNodeMessage::AttributeMessage::kAttributeKeyable << endl;
	MStreamUtils::stdOutStream() << "kAttributeUnkeyable  " << MNodeMessage::AttributeMessage::kAttributeUnkeyable << endl;
	MStreamUtils::stdOutStream() << "kIncomingDirection  " << MNodeMessage::AttributeMessage::kIncomingDirection << endl;
	MStreamUtils::stdOutStream() << "kAttributeArrayAdded  " << MNodeMessage::AttributeMessage::kAttributeArrayAdded << endl;
	MStreamUtils::stdOutStream() << "kAttributeArrayRemoved  " << MNodeMessage::AttributeMessage::kAttributeArrayRemoved << endl;
	MStreamUtils::stdOutStream() << "kOtherPlugSet  " << MNodeMessage::AttributeMessage::kOtherPlugSet << endl;
	MStreamUtils::stdOutStream() << "kLast  " << MNodeMessage::AttributeMessage::kLast << endl;
	MStreamUtils::stdOutStream() << " =============================== " << endl;
	*/

	/* 
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << " ------------ " << endl;

	MStreamUtils::stdOutStream() << "OTHER: " << endl;

	MStreamUtils::stdOutStream() << "PlugName: " << plug.name().asChar() << endl;
	MStreamUtils::stdOutStream() << "oteher plug: " << otherPlug.name().asChar() << endl;
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << " ------------ " << endl;

	*/

	MStreamUtils::stdOutStream() << endl;

	std::string plugName	  = plug.name().asChar();
	std::string plugPartName  = plug.partialName().asChar();
	std::string otherPlugName = otherPlug.name().asChar();

	MStatus result;
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);

	// check if the plug has in its name "doubleSided", which is an attribute that is set when the mesh is finally available
	if (plugName.find("doubleSided") != std::string::npos && (msg & MNodeMessage::AttributeMessage::kAttributeSet)) {
		
		// get mesh through dag path
		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "Mesh name: " << meshNode.name().asChar() << endl;
		MStreamUtils::stdOutStream() << "result: " << result << endl;
		MStreamUtils::stdOutStream() << "\n";

		// if result gives no error, get mesh information through the node
		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GetMeshInfo(meshNode); 
		}
		
	

	}

	// finished extruding mesh
	else if (otherPlugName.find("polyExtrude") != std::string::npos && otherPlugName.find("manipMatrix") != std::string::npos) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "Mesh name: " << meshNode.name().asChar() << endl;
		// if result gives no error, get mesh information through the node
		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GeometryUpdate(meshNode);
		}

	}

	// if vtx is changed
	else if (plugPartName.find("pt[") != std::string::npos) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "Vertex changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;

		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GeometryUpdate(meshNode);
		}
	}
	
	// if normal has changed
	else if (otherPlugName.find("polyNormal") != std::string::npos || (otherPlugName.find("polySoftEdge") != std::string::npos && otherPlugName.find("manipMatrix") != std::string::npos)) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "normal changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;

		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GeometryUpdate(meshNode);
		}
	}

	// if pivot changed, UV probably changed
	else if (plugPartName.find("pv") != std::string::npos) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "UV changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;
		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GeometryUpdate(meshNode);
		}
	}
	
	// if material connection changed (i.e another material was assigned to the mesh)
	else if (plugPartName.find("iog") != std::string::npos && otherPlug.node().apiType() == MFn::Type::kShadingEngine) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "material changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;
		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			MaterialChanged(meshNode);
		}
	}
	
	
	//FIX !!!! 
	// if multicut was performed 
	/* 
	else if (otherPlugName.find("polySplit") != std::string::npos && (msg & MNodeMessage::AttributeMessage::kConnectionMade)) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "PlugName: " << plug.name().asChar() << endl;
		MStreamUtils::stdOutStream() << "oteher plug: " << otherPlug.name().asChar() << endl;
		MStreamUtils::stdOutStream() << endl;

		MStreamUtils::stdOutStream() << "polySplit" << endl;
		MStreamUtils::stdOutStream() << "MSG: " << msg << endl;

		//MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;
		MStreamUtils::stdOutStream() << endl;

		// if result gives no error, get mesh information through the node
		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			PolySplitMesh(plug, otherPlug);
			//GetMeshInfo(meshNode);
		}
	}
	*/

	/* 
	if (plugName.find("outMesh") != std::string::npos) {


		MFnMesh meshNode(path, &result);
		

		MStreamUtils::stdOutStream() << endl;
		MStreamUtils::stdOutStream() << " ------------ " << endl;

		MStreamUtils::stdOutStream() << "OUT MESH: " << endl;

		if (result) {

			MStreamUtils::stdOutStream() << "PlugName: " << plug.name().asChar() << endl;
			MStreamUtils::stdOutStream() << "other plug: " << otherPlug.name().asChar() << endl;
			MStreamUtils::stdOutStream() << "meshNode: " << meshNode.name() << endl;
			
			if (msg & MNodeMessage::kIncomingDirection) {

				MStreamUtils::stdOutStream() << "MSG: " << msg << endl;
				MStreamUtils::stdOutStream() << " <-- " << otherPlug.info().asChar() << endl;

			}
			
		}

		MStreamUtils::stdOutStream() << endl;
		MStreamUtils::stdOutStream() << " ------------ " << endl;
	}
	*/
  
	
	
	
 
	 

}


/* 
// Callback function for when a vtx connection is made to the dependency graph (such as triangulated, topology is changed, a new mesh is added)
void vtxPlugConnected(MPlug & srcPlug, MPlug & destPlug, bool made, void* clientData) {
	
	//MStreamUtils::stdOutStream() << "\n";
	//MStreamUtils::stdOutStream() << " ===================================" << endl;
	//MStreamUtils::stdOutStream() << "VTX PLUG CONNECTED" << "\n";
	//MStreamUtils::stdOutStream() << "srcPlug.partialName() " << srcPlug.partialName() << "\n";
	//MStreamUtils::stdOutStream() << "destPlug.partialName() " << destPlug.partialName() << "\n";

	MCallbackId tempID;

	//if statement checking if it's otput mesh that has been connected 
	if (srcPlug.partialName() == "out" && destPlug.partialName() == "i") {

		//if connection made
		if (made == true) {

	
			// variables
			Mesh meshInfo = {};
			bool triangulated = false;
			MStatus vtxRresult = MS::kFailure;

			// Get mesh through dag path
			MDagPath path;
			MFnDagNode(destPlug.node()).getPath(path);
			MFnMesh mesh(path);


			// check if mesh is triangulated or not =====================
			MPlugArray plugArray;
			destPlug.connectedTo(plugArray, true, true);
			std::string name = plugArray[0].name().asChar();
			std::string polyTriangulateStr = "polyTriangulate";

			if (name.find(polyTriangulateStr) == std::string::npos)
			{
				status = MGlobal::executeCommand("polyTriangulate " + mesh.name(), true, true);
				//MStreamUtils::stdOutStream() << "TRANGULATING" << endl;
				triangulated = true;
			}

			else {
				triangulated = true;
				status = MS::kSuccess;
			}

			
			// if triangulated, continue ================================
			if (triangulated == true) {

				// get mesh name
				std::string objName = mesh.name().asChar();


				// VTX ================

				// get verticies 
				MPointArray vtxArray;
				mesh.getPoints(vtxArray, MSpace::kObject);

				// get triangles
				MIntArray trisCount;
				MIntArray trisVtxIndex;
				mesh.getTriangles(trisCount, trisVtxIndex);

				size_t nrOfTris = trisCount.length();


				// NORMALS ============
				MFloatVectorArray normalArray;
				mesh.getNormals(normalArray, MSpace::kPreTransform);

				//get IDs per face
				MIntArray normalIds;
				MIntArray tempNormalIds;
				for (int faceCnt = 0; faceCnt < nrOfTris; faceCnt++) {
					mesh.getFaceNormalIds(faceCnt, tempNormalIds);
					normalIds.append(tempNormalIds[0]);
				}

				// UVs ================

				MString UVName;
				mesh.getCurrentUVSetName(UVName, -1);

				//get all UVs
				MFloatArray uArray;
				MFloatArray vArray;
				MString* UVSetNamePointer = new MString[1];
				UVSetNamePointer[0] = UVName;
				mesh.getUVs(uArray, vArray, UVSetNamePointer);

				//get UVs IDs for UVsetname
				MIntArray uvCounts;
				MIntArray uvIds;
				mesh.getAssignedUVs(uvCounts, uvIds, &UVSetNamePointer[0]);


				// fill meshInfo struct
				meshInfo.trisCount = nrOfTris;
				meshInfo.vtxCount = vtxArray.length();
				meshInfo.normalCount = normalIds.length();
				meshInfo.UVcount = uvIds.length();


				//sort arrays  ================
				MPointArray sortedVtxArray;
				for (int i = 0; i < trisVtxIndex.length(); i++) {
					sortedVtxArray.append(vtxArray[trisVtxIndex[i]]);
				}

				MVectorArray sortedNormals;
				for (int i = 0; i < meshInfo.trisCount; i++) {
					for (int j = 0; j < 3; j++) {
						sortedNormals.append(normalArray[normalIds[i]]);
					}
				}

				MFloatArray sortedUVs;
				for (int i = 0; i < uvIds.length(); i++) {
					sortedUVs.append(uArray[uvIds[i]]);
					float v = 1.0f - vArray[uvIds[i]];
					sortedUVs.append(v);
				}


				// float arrays that store mesh info  ================
				float* meshVtx = new float[sortedVtxArray.length() * 3];
				float* meshNormals = new float[sortedNormals.length() * 3];
				float* meshUVs = new float[sortedUVs.length()];

				// fill arrays with info
				int vtxPos = 0;
				for (int i = 0; i < sortedVtxArray.length(); i++) {

					// vtx
					meshVtx[vtxPos] = sortedVtxArray[i][0];
					meshVtx[vtxPos + 1] = sortedVtxArray[i][1];
					meshVtx[vtxPos + 2] = sortedVtxArray[i][2];

					// normals
					meshNormals[vtxPos] = sortedNormals[i][0];
					meshNormals[vtxPos + 1] = sortedNormals[i][1];
					meshNormals[vtxPos + 2] = sortedNormals[i][2];

					vtxPos += 3;
				}

				int uvPos = 0;
				for (int j = 0; j < sortedUVs.length(); j++) {
					meshUVs[j] = sortedUVs[j];
					uvPos += 2;
				}



				// =========================
				// Material ================
				// =========================

				Material materialInfo = {};

				// iterate though the lamberts in scene with ItDependencyNodes to find lambert1
				// Assume that when a obj is created in the scene it is bound to lambert 1
				MItDependencyNodes itLamberts(MFn::kLambert);

				while (!itLamberts.isDone()) {
					switch (itLamberts.item().apiType()) {

					case MFn::kLambert: {
						// found lambert, create callback for lambert
						MObject lambertObj(itLamberts.item());

						// assign callback for the material
						//tempID = MNodeMessage::addAttributeChangedCallback(lambertObj, nodeMaterialAttributeChanged);
						//callbackIdArray.append(tempID);

						//find color plug by creating shader obj och seraching for plugname
						MFnLambertShader fnLambertShader(itLamberts.item());
						MPlug colorPlug = fnLambertShader.findPlug("color");

						//check if color plug is connected, if so assume its connected to a fileTexture
						MPlugArray connetionsColor;
						colorPlug.connectedTo(connetionsColor, true, false);

						//MStreamUtils::stdOutStream() << "Has lambert" << "\n";


						//if connectionsColor.length() > 0, material has texture connected
						for (int x = 0; x < connetionsColor.length(); x++) {

							// check for the node of type fileTexture
							if (connetionsColor[x].node().apiType() == MFn::kFileTexture) {

								//if texture was found, create callback for texture plug
								
								MObject textureObj(connetionsColor[x].node());
								
								//tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
								//callbackIdArray.append(tempID);
								
								
								//get texture path to send, search for filetextuename plug
								MFnDependencyNode textureNode(textureObj);
								MPlug fileTextureName = textureNode.findPlug("ftn");

								MString fileName;
								fileTextureName.getValue(fileName);

								// add info to string
								materialInfo.type = 1;
								materialInfo.color = { 255, 255, 255, 255 };

								materialInfo.textureNameLen = fileName.length();
								memcpy(materialInfo.fileTextureName, fileName.asChar(), materialInfo.textureNameLen);

								materialInfo.matNameLen = fnLambertShader.name().length();
								memcpy(materialInfo.materialName, fnLambertShader.name().asChar(), materialInfo.matNameLen);

								break;
							}

						}


						//if connectionsColor.length() = 0, material doesn't have texture connected 
						if (connetionsColor.length() == 0) {

							//find plug for color RGB
							MColor color;
							MPlug attr;

							attr = fnLambertShader.findPlug("colorR");
							attr.getValue(color.r);

							attr = fnLambertShader.findPlug("colorG");
							attr.getValue(color.g);

							attr = fnLambertShader.findPlug("colorB");
							attr.getValue(color.b);


							// convert from 0-1 to 0-255
							Color tempColor = {};
							int red = color.r * 256;
							int blue = color.b * 256;
							int green = color.g * 256;
							int alpha = color.a * 256;
							tempColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };


							// fill material struct
							materialInfo.type = 0;
							materialInfo.color = tempColor;

							materialInfo.textureNameLen = 2;
							memcpy(materialInfo.fileTextureName, "NA", materialInfo.textureNameLen);

							materialInfo.matNameLen = fnLambertShader.name().length();
							memcpy(materialInfo.materialName, fnLambertShader.name().asChar(), materialInfo.matNameLen);

						}

					}

					break;

					default:
						break;
					}

					itLamberts.next();
				}



				// Send message to RayLib ================
				bool msgToSend = false;
				if (meshInfo.vtxCount > 0)
					msgToSend = true;

				if (msgToSend && oldName != objName) {


					size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (2 * (sizeof(float) * sortedVtxArray.length() * 3)) + (sizeof(float) * sortedUVs.length()) + sizeof(Material));
					const char* msg = new char[totalMsgSize];

					// Fill header ========
					MsgHeader msgHeader;
					msgHeader.msgSize = totalMsgSize;
					msgHeader.nodeType = NODE_TYPE::MESH;
					msgHeader.nameLen = objName.length();
					msgHeader.cmdType = CMDTYPE::NEW_NODE;
					memcpy(msgHeader.objName, objName.c_str(), objName.length());

					// copy over msg ======
					memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
					memcpy((char*)msg + sizeof(MsgHeader), &meshInfo, sizeof(Mesh));
					memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh), meshVtx, (sizeof(float) * sortedVtxArray.length() * 3));
					memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * sortedVtxArray.length() * 3), meshNormals, (sizeof(float) * sortedNormals.length() * 3));
					memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * sortedVtxArray.length() * 3) + (sizeof(float) * sortedNormals.length() * 3), meshUVs, sizeof(float) * sortedUVs.length());
					memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * sortedVtxArray.length() * 3) + (sizeof(float) * sortedNormals.length() * 3) + (sizeof(float) * sortedUVs.length()), &materialInfo, sizeof(Material));


					//send it
					if (comLib.send(msg, totalMsgSize)) {
						MStreamUtils::stdOutStream() << "vtxPlugConnected: Message sent" << "\n";
					}


					oldContent = msg;
					oldName = objName;

					delete[] UVSetNamePointer;
					delete[]msg;


				}

				// delete allocated arrays
				delete[] meshVtx;
				delete[] meshNormals;
				delete[] meshUVs;


			}

			




		}
	}
	
}

// Callback function for when an attribute is changed for a node
void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

	//MStreamUtils::stdOutStream() << "\n";
	//MStreamUtils::stdOutStream() << " ===================================" << endl;
	//MStreamUtils::stdOutStream() << "NODE ATTRIBUTE CHANGED" << "\n";

	// variables
	Mesh meshInfo = {};
	MStatus vtxRresult = MS::kFailure;

	// Get mesh through dag path
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	// get mesh name
	std::string objName = mesh.name().asChar();

	// VTX ================

	// get verticies 
	MPointArray vtxArray;
	mesh.getPoints(vtxArray, MSpace::kObject);

	// get triangles
	MIntArray trisCount;
	MIntArray trisVtxIndex;
	mesh.getTriangles(trisCount, trisVtxIndex);
	size_t nrOfTris = trisCount.length();

	// NORMALS ============
	MFloatVectorArray normalArray;
	mesh.getNormals(normalArray, MSpace::kPreTransform);

	//get IDs per face
	MIntArray normalIds;
	MIntArray tempNormalIds;
	for (int faceCnt = 0; faceCnt < nrOfTris; faceCnt++) {
		mesh.getFaceNormalIds(faceCnt, tempNormalIds);
		normalIds.append(tempNormalIds[0]);
	}


	// UVs ================
	MString UVName;
	mesh.getCurrentUVSetName(UVName, -1);
	
	//get all UVs
	MFloatArray uArray;
	MFloatArray vArray;
	MString* UVSetNamePointer = new MString[1];
	UVSetNamePointer[0] = UVName;
	mesh.getUVs(uArray, vArray, UVSetNamePointer);

	//get UVs IDs for UVsetname
	MIntArray uvCounts;
	MIntArray uvIds;
	mesh.getAssignedUVs(uvCounts, uvIds, &UVSetNamePointer[0]);


	// fill meshInfo struct
	meshInfo.trisCount = nrOfTris;
	meshInfo.vtxCount = vtxArray.length();
	meshInfo.normalCount = normalIds.length();
	meshInfo.UVcount = uvIds.length();

	//sort arrays  ================
	MPointArray sortedVtxArray;
	for (int i = 0; i < trisVtxIndex.length(); i++) {
		sortedVtxArray.append(vtxArray[trisVtxIndex[i]]);
	}

	MVectorArray sortedNormals;
	for (int i = 0; i < meshInfo.trisCount; i++) {
		for (int j = 0; j < 3; j++) {
			sortedNormals.append(normalArray[normalIds[i]]);
		}
	}

	MFloatArray sortedUVs;
	for (int i = 0; i < uvIds.length(); i++) {
		sortedUVs.append(uArray[uvIds[i]]);
		float v = 1.0f - vArray[uvIds[i]];
		sortedUVs.append(v);
	}

	// float arrays that store mesh info  ================
	float* meshVtx = new float[sortedVtxArray.length() * 3];
	float* meshNormals = new float[sortedNormals.length() * 3];
	float* meshUVs = new float[sortedUVs.length()];

	// fill arrays with info
	int vtxPos = 0;
	for (int i = 0; i < sortedVtxArray.length(); i++) {
		// vtx
		meshVtx[vtxPos] = sortedVtxArray[i][0];
		meshVtx[vtxPos + 1] = sortedVtxArray[i][1];
		meshVtx[vtxPos + 2] = sortedVtxArray[i][2];

		// normals
		meshNormals[vtxPos] = sortedNormals[i][0];
		meshNormals[vtxPos + 1] = sortedNormals[i][1];
		meshNormals[vtxPos + 2] = sortedNormals[i][2];

		vtxPos += 3;
	}

	int uvPos = 0;
	for (int j = 0; j < sortedUVs.length(); j++) {
		meshUVs[j] = sortedUVs[j];
		uvPos += 2;
	}


	// Send message to RayLib ================
	bool msgToSend = false;
	if (meshInfo.vtxCount > 0)
		msgToSend = true;

	if (msgToSend && oldName != objName) {


		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (2 * (sizeof(float) * sortedVtxArray.length() * 3)) + sortedUVs.length() );
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nodeType = NODE_TYPE::MESH;
		msgHeader.nameLen = objName.length();
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, objName.c_str(), objName.length());

		// copy over msg ======

		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &meshInfo, sizeof(Mesh));
		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh), meshVtx, (sizeof(float) * sortedVtxArray.length() * 3));
		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * sortedVtxArray.length() * 3), meshNormals, (sizeof(float) * sortedNormals.length() * 3));
		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * sortedVtxArray.length() * 3) + (sizeof(float) * sortedNormals.length() * 3), meshUVs, sortedUVs.length());



		//send it
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "Attribute changed: Message sent" << "\n";
		}


		oldContent = msg;
		oldName = objName;

		delete[] UVSetNamePointer;
		delete[]msg;

	}

	// delete allocated arrays
	delete[] meshVtx;
	delete[] meshNormals;
	delete[] meshUVs;


}

//Callback function for when the matrix is changed for a mesh object (when rotated) 
void nodeWorldMatrixChanged(MObject &node, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	// Get mesh through dag path
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnMesh mesh(path);

	// get name of object 
	std::string objName = mesh.name().asChar();

	//get local and world matrix using the path
	MMatrix localMatrix = MMatrix(path.exclusiveMatrix());
	MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

	// fill matrix struct to send 
	// matrix is transposed before sending to raylib
	Matrix matrixInfo = {}; 

	//row 1
	matrixInfo.a11 = worldMatrix(0, 0); 
	matrixInfo.a12 = worldMatrix(1, 0);
	matrixInfo.a13 = worldMatrix(2, 0);
	matrixInfo.a14 = worldMatrix(3, 0);

	//row 2
	matrixInfo.a21 = worldMatrix(0, 1);
	matrixInfo.a22 = worldMatrix(1, 1);
	matrixInfo.a23 = worldMatrix(2, 1);
	matrixInfo.a24 = worldMatrix(3, 1);

	//row 3
	matrixInfo.a31 = worldMatrix(0, 2);
	matrixInfo.a32 = worldMatrix(1, 2);
	matrixInfo.a33 = worldMatrix(2, 2);
	matrixInfo.a34 = worldMatrix(3, 2);

	//row 4
	matrixInfo.a41 = worldMatrix(0, 3);
	matrixInfo.a42 = worldMatrix(1, 3);
	matrixInfo.a43 = worldMatrix(2, 3);
	matrixInfo.a44 = worldMatrix(3, 3);


	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Matrix));
	const char* msg = new char[totalMsgSize];

	// Fill header ========
	MsgHeader msgHeader;
	msgHeader.msgSize = totalMsgSize;
	msgHeader.nameLen = objName.length();
	msgHeader.nodeType = NODE_TYPE::MESH;
	msgHeader.cmdType = CMDTYPE::UPDATE_MATRIX;
	memcpy(msgHeader.objName, objName.c_str(), objName.length());

	// copy over msg ======
	memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
	memcpy((char*)msg + sizeof(MsgHeader), &matrixInfo, sizeof(Matrix));

	//send it
	if (comLib.send(msg, totalMsgSize)) {
		//MStreamUtils::stdOutStream() << "nodeWorldMatrixChanged: Message sent" << "\n";
	}

	oldContent = msg;
	oldName = objName;

	delete[] msg; 

}
*/

// ==================================================================================
// ================================ CAMERA ==========================================
// ==================================================================================
void cameraWorldMatrixChanged(MObject &transformNode, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	MStreamUtils::stdOutStream() << "=================================\n";
	MStreamUtils::stdOutStream() << "cameraWorldMatrixChanged\n";

	M3dView activeView = M3dView::active3dView();

	MDagPath path;
	activeView.getCamera(path);
	MFnCamera cameraNode(path);

	std::string cameraName = cameraNode.name().asChar(); 
	MStreamUtils::stdOutStream() << "camera: " << cameraName << endl;


	// get height/width
	int height = activeView.portHeight();
	int width = activeView.portWidth();
	double FOV = 0; 
	//get relevant vectors for raylib

	MPoint camPos = cameraNode.eyePoint(MSpace::kWorld, &status);
	MVector upDir = cameraNode.upDirection(MSpace::kWorld, &status);
	MVector rightDir = cameraNode.rightDirection(MSpace::kWorld, &status);
	MPoint COI = cameraNode.centerOfInterestPoint(MSpace::kWorld, &status);

	float vertFOV = cameraNode.verticalFieldOfView();


	bool isOrtographic = cameraNode.isOrtho();
	if (isOrtographic) {
		double ortoWidth = cameraNode.orthoWidth(&status);
		FOV = ortoWidth;
	}
	else {
		double verticalFOV;
		double horizontalFOV;
		cameraNode.getPortFieldOfView(width, height, horizontalFOV, verticalFOV);
		FOV = verticalFOV;
	}

}


/* 
// callback function for the active pannel in the viewport
void activeCamera(const MString &panelName, void* cliendData) {

	// Variables 
	Camera cameraInfo = {};
	std::string objName = "";
	MStatus status = MS::kFailure;

	int height = 0;
	int width = 0;
	double FOV = 0;

	//get active camera
	M3dView activeView;
	MStatus result = M3dView::getM3dViewFromModelPanel(panelName, activeView);
	if (result != MS::kSuccess) {
		MStreamUtils::stdOutStream() << "Did not get active 3DView" << endl;
		MStreamUtils::stdOutStream() << "Error: " << result << endl;
		return;
	}

	//get name
	MDagPath camDag;
	activeView.getCamera(camDag);
	objName = camDag.fullPathName().asChar();

	// get height/width
	height = activeView.portHeight();
	width = activeView.portWidth();

	//get relevant vectors for raylib
	MFnCamera cameraView(camDag);
	MPoint camPos	 = cameraView.eyePoint(MSpace::kWorld, &status);
	MVector upDir	 = cameraView.upDirection(MSpace::kWorld, &status);
	MVector rightDir = cameraView.rightDirection(MSpace::kWorld, &status);
	MPoint COI = cameraView.centerOfInterestPoint(MSpace::kWorld, &status);

	//get fov or zoom depending on ortho/persp
	bool isOrtographic = cameraView.isOrtho();
	if (isOrtographic) {
		double ortoWidth = cameraView.orthoWidth(&status);
		FOV = ortoWidth;
	}
	else {
		double verticalFOV;
		double horizontalFOV;
		cameraView.getPortFieldOfView(width, height, horizontalFOV, verticalFOV);
		FOV = verticalFOV;
	}

	// fill camInfo struct
	cameraInfo.pos	   = { (float)camPos.x, (float)camPos.y, (float)camPos.z };
	cameraInfo.up	   = { (float)upDir.x, (float)upDir.y, (float)upDir.z };
	cameraInfo.forward = { (float)COI.x, (float)COI.y, (float)COI.z };
	cameraInfo.type    = isOrtographic;
	cameraInfo.fov	   = FOV;

	//pass to send
	bool msgToSend = false;
	if (objName.length() > 0) {
		msgToSend = true;
	}

	if (msgToSend) {

		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Camera));
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nameLen = objName.length();
		msgHeader.nodeType = NODE_TYPE::CAMERA;
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, objName.c_str(), objName.length());


		// copy over msg ======
		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &cameraInfo, sizeof(Camera));


		//send it
		if (comLib.send(msg, totalMsgSize)) {
			//MStreamUtils::stdOutStream() << "activeCamera: Message sent" << "\n";
		}


		oldContent = msg;
		oldName = objName;

		delete[]msg;

	}

}

*/
// ==================================================================================
// ================================= LIGHT ==========================================
// ==================================================================================

/* 
//not sending 
void nodeLightAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {
	
	// only do this if the msg is a change of attributes (pos, color etc)
	if (msg & MNodeMessage::kAttributeSet) {
		
		//MStreamUtils::stdOutStream() << "nodeLightAttributeChanged" << "\n";

		// get light through dagpath
		MDagPath path;
		MFnDagNode(plug.node()).getPath(path);
		MFnLight lightNode(path);

		// variables
		MStatus status;
		float intensity;
		MVector lightPos;
		MColor lightColor;

		// fill variables 
		lightColor = lightNode.color();
		intensity = lightNode.intensity();

		MObject parent = lightNode.parent(0);
		if (parent.hasFn(MFn::kTransform)) {
			MFnTransform lightPoint(parent);
			lightPos = lightPoint.getTranslation(MSpace::kObject, &status);
		}

		// convert from 0-1 to 0-255
		Color tempColor = {};
		int red = lightColor.r * 256;
		int blue = lightColor.b * 256;
		int green = lightColor.g * 256;
		int alpha = lightColor.a * 256;
		tempColor = { (unsigned char)red, (unsigned char)blue, (unsigned char)green, (unsigned char)alpha };

		// fill light struct 
		Light lightInfo = {};
		lightInfo.color = tempColor;
		lightInfo.intensity = intensity;
		lightInfo.lightPos = { (float)lightPos.x, (float)lightPos.y, (float)lightPos.z };

		// create message ptr
		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Light));
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nodeType = NODE_TYPE::LIGHT;
		msgHeader.nameLen = lightNode.name().length();
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, lightNode.name().asChar(), lightNode.name().length());

		// copy over msg ======
		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &lightInfo, sizeof(Light));

		
		//send it
		//if (comLib.send(msg, totalMsgSize)) {
			//MStreamUtils::stdOutStream() << "nodeLightAttributeChanged: Message sent" << "\n";
		//}
		

		
		delete[] msg;
	}
}

*/

//not sending 
void nodeWorldMatrixChangedLight(MObject &node, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {
	//MStreamUtils::stdOutStream() << "nodeWorldMatrixChangedLight " << endl;
	/* 
	// get light node path
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnLight sceneLight(path);

	MFnTransform transform(path);
	//MFnMesh mesh(path);

	MMatrix localMatrix = MMatrix(path.exclusiveMatrix());
	MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());


	std::string objName = sceneLight.name().asChar();

	// fill matrix struct to send 
	// matrix is transposed before sending to raylib
	Matrix matrixInfo = {};

	//row 1
	matrixInfo.a11 = worldMatrix(0, 0);
	matrixInfo.a12 = worldMatrix(1, 0);
	matrixInfo.a13 = worldMatrix(2, 0);
	matrixInfo.a14 = worldMatrix(3, 0);

	//row 2
	matrixInfo.a21 = worldMatrix(0, 1);
	matrixInfo.a22 = worldMatrix(1, 1);
	matrixInfo.a23 = worldMatrix(2, 1);
	matrixInfo.a24 = worldMatrix(3, 1);

	//row 3
	matrixInfo.a31 = worldMatrix(0, 2);
	matrixInfo.a32 = worldMatrix(1, 2);
	matrixInfo.a33 = worldMatrix(2, 2);
	matrixInfo.a34 = worldMatrix(3, 2);

	//row 4
	matrixInfo.a41 = worldMatrix(0, 3);
	matrixInfo.a42 = worldMatrix(1, 3);
	matrixInfo.a43 = worldMatrix(2, 3);
	matrixInfo.a44 = worldMatrix(3, 3);


	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Matrix));
	const char* msg = new char[totalMsgSize];

	// Fill header ========
	MsgHeader msgHeader;
	msgHeader.msgSize = totalMsgSize;
	msgHeader.nameLen = objName.length();
	msgHeader.nodeType = NODE_TYPE::LIGHT;
	msgHeader.cmdType = CMDTYPE::UPDATE_MATRIX;
	memcpy(msgHeader.objName, objName.c_str(), objName.length());

	// copy over msg ======
	memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
	memcpy((char*)msg + sizeof(MsgHeader), &matrixInfo, sizeof(Matrix));

	//send it
	
	if (oldContent != msg) {
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "nodeWorldMatrixChangedLight: Message sent" << "\n";
		}
	}


	oldContent = msg;
	oldName = objName;

	delete[] msg;

	*/


	/* 
	// fill camInfo struct
	cameraInfo.pos = { (float)camPos.x, (float)camPos.y, (float)camPos.z };
	cameraInfo.up = { (float)upDir.x, (float)upDir.y, (float)upDir.z };
	cameraInfo.forward = { (float)COI.x, (float)COI.y, (float)COI.z };
	cameraInfo.type = isOrtographic;
	cameraInfo.fov = FOV;

	//MStreamUtils::stdOutStream() << "cameraInfo.type: " << cameraInfo.type << "\n";

	//pass to send
	bool msgToSend = false;
	if (objName.length() > 0) {
		msgToSend = true;
	}

	if (msgToSend) {

		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Camera));
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nameLen = objName.length();
		msgHeader.nodeType = NODE_TYPE::CAMERA;
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, objName.c_str(), objName.length());


		// copy over msg ======
		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &cameraInfo, sizeof(Camera));



		//send it
		if (comLib.send(msg, totalMsgSize)) {
			//MStreamUtils::stdOutStream() << "activeCamera: Message sent" << "\n";
		}


		oldContent = msg;
		oldName = objName;

		delete[]msg;

	
	*/
	/* 
	if (oldContent != msgString)
	{
		//pass to send
		bool msgToSend = false;
		if (msgString.length() > 0)
			msgToSend = true;

		if (msgToSend) {
			//sendMsg(CMDTYPE::UPDATE_MATRIX, NODE_TYPE::LIGHT, 0, 0, objName, msgString);
		}
	}
	*/
}

/*
void GetLightInfo(MFnLight& lightNode) {

	float intensity; 
	MStatus status;
	MColor lightColor;
	MVector lightPos; 

	lightColor = lightNode.color();
	intensity  = lightNode.intensity(); 
	MObject parent = lightNode.parent(0); 

	if (parent.hasFn(MFn::kTransform)) {
		MFnTransform lightPoint(parent);
		lightPos = lightPoint.getTranslation(MSpace::kObject, &status);
	}

	// convert from 0-1 to 0-255
	Color tempColor = {};
	int red   = lightColor.r * 256;
	int blue  = lightColor.b * 256;
	int green = lightColor.g * 256;
	int alpha = lightColor.a * 256;
	tempColor = { (unsigned char)red, (unsigned char)blue, (unsigned char)green, (unsigned char)alpha};

	// fill light struct 
	Light lightInfo = {}; 
	lightInfo.color = tempColor; 
	lightInfo.intensity = intensity; 
	lightInfo.lightPos = { (float)lightPos.x, (float)lightPos.y, (float)lightPos.z };

	//MStreamUtils::stdOutStream() << "Light Color: " << lightColor << endl;
	//MStreamUtils::stdOutStream() << "Light intensity: " << intensity << endl;
	//MStreamUtils::stdOutStream() << "name: " << lightNode.name() << endl;

	// create message ptr
	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Light));
	const char* msg = new char[totalMsgSize];
	
	// Fill header ========
	MsgHeader msgHeader;
	msgHeader.msgSize = totalMsgSize;
	msgHeader.nodeType = NODE_TYPE::LIGHT;
	msgHeader.nameLen = lightNode.name().length();
	msgHeader.cmdType = CMDTYPE::NEW_NODE;
	memcpy(msgHeader.objName, lightNode.name().asChar(), lightNode.name().length());
	
	// copy over msg ======
	memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
	memcpy((char*)msg + sizeof(MsgHeader), &lightInfo, sizeof(Light));
	
				

	if (oldContent != msg) {
		//send it
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "GetLightInfo: Message sent" << "\n";
			}
		}

	oldContent = msg;
	delete[] msg; 
	
}

*/

// ==================================================================================
// ================================= OTHER ==========================================
// ==================================================================================

void InitializeScene() {

	MCallbackId tempID;

	// get the lamberts in scene at startup
	MItDependencyNodes lambertIterator(MFn::kLambert);
	while (!lambertIterator.isDone()) {

		MStreamUtils::stdOutStream() << "LAMBERT FOUND\n";
		MObject lambertShader(lambertIterator.thisNode());

		tempID = MNodeMessage::addAttributeChangedCallback(lambertShader, materialAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		lambertIterator.next();
	}


	// get all the active cameras in scene at startup 
	MStreamUtils::stdOutStream() << "\n";
	MItDependencyNodes cameraIterator(MFn::kCamera);
	while (!cameraIterator.isDone()) {

		MFnCamera cameraNode(cameraIterator.item());
		MStreamUtils::stdOutStream() << "CAMERA FOUND\n";
		MStreamUtils::stdOutStream() << "CAM " << cameraNode.name().asChar() << endl;

		MDagPath path;
		MFnDagNode(cameraIterator.thisNode()).getPath(path);
		MFnCamera cam(path);

		tempID = MDagMessage::addWorldMatrixModifiedCallback(path, cameraWorldMatrixChanged, NULL, &status);
		callbackIdArray.append(tempID);

		cameraIterator.next();
	}

}

// ============================== CALLBACKS ========================================

// callback that goes through nodes that have been created and attatches callbacks
void nodeAdded(MObject &node, void * clientData) {

	MCallbackId tempID; 
	MStatus Result = MS::kSuccess;

	// if the node has kMesh attributes it is a mesh
	if (node.hasFn(MFn::kMesh)) {
		
		// check if mesh is not intermediate
		// The intermediate object is the object as it was before adding the deformers
		MFnMesh meshNode(node);
		if (!meshNode.isIntermediateObject()) {

			MStreamUtils::stdOutStream() << "nodeAdded: MESH was added " << endl;
			meshQueue.push(node);

			tempID = MNodeMessage::addAttributeChangedCallback(node, meshAttributeChanged, NULL, &status);
			if (Result == MS::kSuccess) 
				callbackIdArray.append(tempID);
			
		}

		/* 
		tempID = MDGMessage::addConnectionCallback(nodePlugConnected, NULL, &status);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
		*/
	
	
		
		/* 
		tempID = MDGMessage::addConnectionCallback(meshConnectionChanged, NULL, &Result);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
		*/
		
	}

	// if the node has kTransform attributes it is a transform
	if (node.hasFn(MFn::kTransform)) {

		MStreamUtils::stdOutStream() << "nodeAdded: TRANSFORM was added " << endl;
		transformQueue.push(node);

		
	}

	// if the node has kLight attributes it is a light
	if (node.hasFn(MFn::kLight)) {

		MStreamUtils::stdOutStream() << "nodeAdded: LIGHT was added " << endl;
		lightQueue.push(node);
	}

	// if the node has kLambert attributes it is a lambert
	if (node.hasFn(MFn::kLambert)) {

		MStreamUtils::stdOutStream() << "nodeAdded: LAMBERT was added " << endl;
	
		MObject lambertShader(node);
		tempID = MNodeMessage::addAttributeChangedCallback(lambertShader, materialAttributeChanged, NULL, &status);
		if (Result == MS::kSuccess)
			callbackIdArray.append(tempID);
		

		materialQueue.push(node);
	}

}

// callback that goes through nodes that have been deleted
void nodeDeleted(MObject &node, void *clientData) {

	//MStreamUtils::stdOutStream() << "\n";
	//MStreamUtils::stdOutStream() << "NODE DELETED";

	if (node.hasFn(MFn::kMesh)) {

		MFnMesh mesh(node);
		std::string meshName = mesh.name().asChar(); 

		size_t totalMsgSize = (sizeof(MsgHeader));
		const char* msg = new char[totalMsgSize];

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nodeType = NODE_TYPE::MESH;
		msgHeader.nameLen = meshName.length();
		msgHeader.cmdType = CMDTYPE::DELETE_NODE;
		memcpy(msgHeader.objName, meshName.c_str(), meshName.length());

		// copy over msg ======
		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "nodeDeleted: Message sent" << "\n";
		}


	}

	if (node.hasFn(MFn::kLight)) {
		//MStreamUtils::stdOutStream() << "light to delete";

	}
}

// callback called every x seconds and goes through existing nodes
void timerCallback(float elapsedTime, float lastTime, void* clientData) {
	
	globalTime += elapsedTime;

	MCallbackId tempID;
	MStatus Result = MS::kSuccess;

	for (int i = 0; i < meshQueue.size(); i++) {

		MObject currenNode = meshQueue.back();
		if (currenNode.hasFn(MFn::kMesh)) {

			MFnMesh currentMesh = currenNode;

			MDagPath path;
			MFnDagNode(currenNode).getPath(path);
			MFnDagNode currentDagNode = currenNode;
			MFnTransform transform(path);

			//unsigned int nrOfPrnts = currentDagNode.parentCount();
			MObject parentTransf = currentDagNode.parent(0);

			/* 
			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/

			/* 
			tempID = MNodeMessage::addNameChangedCallback(currenNode, nodeNameChangedMaterial, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/

			/* 
			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/

			/* 
			tempID = MNodeMessage::addNameChangedCallback(currenNode, NodeNameChanged, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/
		}

		meshQueue.pop();
	}

	for (int i = 0; i < transformQueue.size(); i++) {

		MObject currentNode = transformQueue.back();
		if (currentNode.hasFn(MFn::kTransform)) {

			MFnTransform currentTransform = currentNode;

			/* 
			tempID = MNodeMessage::addAttributeChangedCallback(currentNode, nodeTransformAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/

			/* 
			tempID = MNodeMessage::addNameChangedCallback(currentNode, NodeNameChanged, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/

			/*
			MDagPath path;
			MFnDagNode(currentNode).getPath(path);
			MFnDagNode currentDagNode = currentNode;
			MFnTransform transform(path);

			//unsigned int nrOfPrnts = currentDagNode.parentCount();
			MObject parentTransf = currentDagNode.parent(0);

			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/
		}
	}


	for (int i = 0; i < lightQueue.size(); i++) {

		MObject currenNode = lightQueue.back();
		if (currenNode.hasFn(MFn::kLight)) {

			//MStreamUtils::stdOutStream() << "LIGHT NODE " << endl;
			MDagPath path;
			MFnLight sceneLight(currenNode);
			MFnDagNode(currenNode).getPath(path);
			/* 
			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			MObject parentNode = sceneLight.parent(0); 
			tempID = MNodeMessage::addAttributeChangedCallback(parentNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/
			

			//GetLightInfo(sceneLight);

			/*
			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/

		}

		lightQueue.pop();
	}

}



// Function to Load plugin Load
EXPORT MStatus initializePlugin(MObject obj) {

	MStatus res = MS::kSuccess;
	MCallbackId tempID; 

	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);
	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
		return res;
	}

	// redirect cout to cerr, so that when we do cout goes to cerr
	// in the maya output window (not the scripting output!)
	cout.rdbuf(cerr.rdbuf());

	MStreamUtils::stdOutStream() << "\n";
	MStreamUtils::stdOutStream() << "Plugin loaded ===========================\n";


	// register callbacks here for
	tempID = MDGMessage::addNodeAddedCallback(nodeAdded);
	callbackIdArray.append(tempID);

	tempID = MDGMessage::addNodeRemovedCallback(nodeDeleted);
	callbackIdArray.append(tempID);

	tempID = MTimerMessage::addTimerCallback(timerPeriod, timerCallback, NULL, &status);
	callbackIdArray.append(tempID);

	/* 
	// get the lamberts in scene at startup
	MItDependencyNodes lambertIterator(MFn::kLambert);
	while (!lambertIterator.isDone()) {

		MStreamUtils::stdOutStream() << "LAMBERT FOUND\n";
		MObject lambertShader(lambertIterator.thisNode());

		tempID = MNodeMessage::addAttributeChangedCallback(lambertShader, materialAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		lambertIterator.next();
	}

	
	// get all the active cameras in scene at startup 
	MStreamUtils::stdOutStream() << "\n";
	MItDependencyNodes cameraIterator(MFn::kCamera);
	while (!cameraIterator.isDone()) {
		
		MFnCamera cameraNode(cameraIterator.item());
		MStreamUtils::stdOutStream() << "CAMERA FOUND\n";
		MStreamUtils::stdOutStream() << "CAM " << cameraNode.name().asChar() << endl;
		
		MDagPath path;
		MFnDagNode(cameraIterator.thisNode()).getPath(path);
		MFnCamera cam(path);

		tempID = MDagMessage::addWorldMatrixModifiedCallback(path, cameraWorldMatrixChanged, NULL, &status);
		callbackIdArray.append(tempID);

		cameraIterator.next();
	}
	
	*/


	/* 
	// camera callbacks initialized for all the view panels 
	tempID = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel1", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);
	}

	tempID = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel2", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);

	}

	tempID = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel3", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);
	}

	tempID = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel4", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);
	}
	*/
	
	InitializeScene(); 


	// Timer
	gTimer.clear();
	gTimer.beginTimer();

	return res;
}

// function to unload plugin
EXPORT MStatus uninitializePlugin(MObject obj) {
	
	MFnPlugin plugin(obj);

	cout << "Plugin unloaded =========================" << endl;
	MMessage::removeCallbacks(callbackIdArray);

	return MS::kSuccess;
}

