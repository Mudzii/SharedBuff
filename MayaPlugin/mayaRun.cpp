#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>

#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>
#pragma comment(lib, "Project1.lib")

ComLib comLib("shaderMemory", 50, PRODUCER);

using namespace std;


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
	UPDATE_MATERIAL		= 1005,
	UPDATE_MATERIALNAME = 1006,
	DELETE_NODE			= 1007
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

// ===========================================================
// ==================== Variables ============================
// ===========================================================

MObject m_node;
MStatus status = MS::kSuccess;
MCallbackIdArray callbackIdArray;
bool initBool = false;

MTimer gTimer;
float globalTime = 0.0f;
float timerPeriod = 5.0f; 

// keep track of created meshes/ lights to maintain them
queue<MObject> meshQueue;
queue<MObject> lightQueue;

string oldContent = "";
string oldName	  = "";


// Output messages in output window
//MStreamUtils::stdOutStream() << "...: " << ... << "_" << endl;

// ==================================================================================
// ==================================================================================
// ================================= FUNCTIONS ======================================
// ==================================================================================
// ==================================================================================


// ==================================================================================
// ============================== MATERIAL ==========================================
// ==================================================================================

// callback function for when a texture is connected to the material node on a mesh
void nodeTextureAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {
	
	MObject textureObj(plug.node());
	Material materialInfo = {}; 

	 //find texture filepath
	MPlugArray connections;
	plug.connectedTo(connections, true, true);
	if (connections.length() > 0) {

		//split connection to get material name
		std::string materialNamePlug = connections[0].name().asChar();
		if (materialNamePlug.length() > 0) {

			//if plug exists split to get material name
			std::string splitElement = ".";
			std::string materialName = materialNamePlug.substr(0, materialNamePlug.find(splitElement));

			//get filepath
			MFnDependencyNode textureNode(textureObj);
			MPlug fileTextureName = textureNode.findPlug("ftn");

			MString fileName;
			fileTextureName.getValue(fileName);
			std::string fileNameString = fileName.asChar();

			if (fileNameString.length() > 0) {
				//if filepath exists creat the final string to send to the raylib
				//std::string materialString = "";
				//materialString.append(materialName + " ");
				//materialString.append("texture ");
				//materialString.append(fileNameString);

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
				msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
				memcpy(msgHeader.objName, "NA", msgHeader.nameLen);
				

				// copy over msg ======
				memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
				memcpy((char*)msg + sizeof(MsgHeader), &materialInfo, sizeof(Material));

				if (oldContent != msg) {
					//send it
					if (comLib.send(msg, totalMsgSize)) {
						MStreamUtils::stdOutStream() << "nodeTextureAttributeChanged: Message sent" << "\n";
						}
				}

				oldContent = msg;

				delete[] msg; 
				//sendMsg(CMDTYPE::UPDATE_MATERIAL, NODE_TYPE::MESH, materialString.length(), 0, "noObjName", materialString);
				

			}
		}
	}
}

// callback function for when a material attribute (ex color) changes
void nodeMaterialAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

	MCallbackId tempID; 
	Material materialInfo = {}; 

	MObject lamObj(plug.node());
	MFnDependencyNode lambertDepNode(lamObj);

	//check if callback lamert has texture or color
	bool hasTexture = false;
	MPlugArray connetionsColor;
	MPlug colorPlug = lambertDepNode.findPlug("color");
	colorPlug.connectedTo(connetionsColor, true, false);

	// if node has a texture
	for (int x = 0; x < connetionsColor.length(); x++) {
		if (connetionsColor[x].node().apiType() == MFn::kFileTexture)
		{
			//if a texture node found, create a callback for the texture
			MObject textureObj(connetionsColor[x].node());
			tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
			callbackIdArray.append(tempID);

			hasTexture = true;
		}
	}

	//else node has color
	if (hasTexture == false) {
		if (plug.name() == colorPlug.name()) {

			//get material color
			MFnDependencyNode lambertItem(lamObj);
			MColor color;
			MPlug attr;

			attr = lambertItem.findPlug("colorR");
			attr.getValue(color.r);
			attr = lambertItem.findPlug("colorG");
			attr.getValue(color.g);
			attr = lambertItem.findPlug("colorB");
			attr.getValue(color.b);


			// convert from 0-1 to 0-255
			Color tempColor = {}; 
			int red = color.r * 255;
			int blue = color.b * 255;
			int green = color.g * 255;
			int alpha = color.a * 255;
			tempColor = { (unsigned char)red, (unsigned char)blue, (unsigned char)green, (unsigned char)alpha };

			MStreamUtils::stdOutStream() << "COLOR: " << red << " : " << green << " : " << blue << " : " << alpha << endl;

			// fill material struct
			materialInfo.type = 0;
			materialInfo.color = tempColor;
			
			materialInfo.textureNameLen = 2;
			memcpy(materialInfo.fileTextureName, "NA", materialInfo.textureNameLen);

			materialInfo.matNameLen = lambertItem.name().length();
			memcpy(materialInfo.materialName, lambertItem.name().asChar(), materialInfo.matNameLen);

			// create message ptr
			size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));
			const char* msg = new char[totalMsgSize];

			// Fill header ========
			MsgHeader msgHeader;
			msgHeader.msgSize = totalMsgSize;
			msgHeader.nodeType = NODE_TYPE::MESH;
			msgHeader.nameLen = 2;
			msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
			memcpy(msgHeader.objName, "NA", msgHeader.nameLen);

			// copy over msg ======
			memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
			memcpy((char*)msg + sizeof(MsgHeader), &materialInfo, sizeof(Material));


			if (oldContent != msg) {
				//send it
				if (comLib.send(msg, totalMsgSize)) {
					MStreamUtils::stdOutStream() << "nodeMaterialAttributeChanged: Message sent" << "\n";
				}
			}

			oldContent = msg;

			// OLD CODE
			/* 
			//create final string to send
			std::string colors = "";
			colors.append(lambertDepNode.name().asChar());
			colors.append(" color ");
			colors.append(to_string(color.r) + " ");
			colors.append(to_string(color.g) + " ");
			colors.append(to_string(color.b));
			*/
			/*
			//pass to send
			bool msgToSend = false;
			if (colors.length() > 0)
				msgToSend = true;

			if (msgToSend) {
				//sendMsg(CMDTYPE::UPDATE_MATERIAL, NODE_TYPE::MESH, colors.length(), 0, "noObjName", colors);
			}
			*/
		}
	}
}

// callback used for when a change is made for materials and textures
void meshConnectionChanged(MPlug &plug, MPlug &otherPlug, bool made, void *clientData) {

	// a connection was made
	//if (made == true) {
	


		MStreamUtils::stdOutStream() << endl;
		MStreamUtils::stdOutStream() << " ===================================" << endl;

		MStreamUtils::stdOutStream() << "MESH CONNECTION CHANGED" << endl;
		MStreamUtils::stdOutStream() << "plug name: " << plug.name() << endl;
		MStreamUtils::stdOutStream() << "partaial name: " << plug.partialName() << endl;

		MStreamUtils::stdOutStream() << "otherPlug name: " << otherPlug.name() << endl;
		MStreamUtils::stdOutStream() << "made: " << made << endl;


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
				
				MStreamUtils::stdOutStream() << "Found shader grp" << endl;

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
						tempID = MNodeMessage::addAttributeChangedCallback(lambertObj, nodeMaterialAttributeChanged);
						callbackIdArray.append(tempID);

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


								//create callback for node
								MObject textureObj(connetionsColor[x].node());
								tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
								callbackIdArray.append(tempID);

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
									msgHeader.cmdType = CMDTYPE::UPDATE_MATERIALNAME;
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

							MStreamUtils::stdOutStream() << "No texture, getting color" << endl;

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
							int red	  = color.r * 255;
							int blue  = color.b * 255;
							int green = color.g * 255;
							int alpha = color.a * 255;

							tempColor = { (unsigned char)red, (unsigned char)blue, (unsigned char)green, (unsigned char)alpha };
							
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
							msgHeader.cmdType = CMDTYPE::UPDATE_MATERIALNAME;
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

	


	//}

			// OLD CODE
			/*

									//create the string with the stuff
									std::string materialString = "";
									materialString.append(lambertDepNode.name().asChar());
									materialString.append(" texture ");
									materialString.append(fileNameString);

									bool msgToSend = false;
									if (materialString.length() > 0)
										msgToSend = true;

									if (msgToSend) {
										//sendMsg(CMDTYPE::UPDATE_MATERIALNAME, NODE_TYPE::MESH, materialString.length(), 0, mesh.name().asChar(), materialString);
									}
									*/

			// OLD CODE
			/*
																//create final string to be sent to the rayli with information
																std::string colors = "";
																colors.append(lambertDepNode.name().asChar());
																colors.append(" color ");
																colors.append(to_string(color.r) + " ");
																colors.append(to_string(color.g) + " ");
																colors.append(to_string(color.b));

																//pass to send
																bool msgToSend = false;
																if (colors.length() > 0)
																	msgToSend = true;

																if (msgToSend) {
																	//sendMsg(CMDTYPE::UPDATE_MATERIALNAME, NODE_TYPE::MESH, colors.length(), 0, mesh.name().asChar(), colors);
																}
																*/
}

//sending NEEDS UPDATE
void nodeNameChangedMaterial(MObject &node, const MString &str, void*clientData) {

	// Get the new name of the node, and use it from now on.
	MString oldName = str;
	MString newName = MFnDagNode(node).name();

	std::string objName = oldName.asChar();
	std::string newObjName = newName.asChar();

	int newNameLen = newObjName.length(); 
	char* newNameChar = new char[newNameLen];
	memcpy(newNameChar, newObjName.c_str(), newNameLen);


	// create message ptr
	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(int) + (sizeof(char) * newNameLen));
	const char* msg = new char[totalMsgSize];

	// Fill header ========
	MsgHeader msgHeader;
	msgHeader.msgSize = totalMsgSize;
	msgHeader.nodeType = NODE_TYPE::MESH;
	msgHeader.nameLen = objName.length();
	msgHeader.cmdType = CMDTYPE::UPDATE_NAME;
	memcpy(msgHeader.objName, objName.c_str(), msgHeader.nameLen);
	
	// copy over msg ======
	memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
	memcpy((char*)msg + sizeof(MsgHeader), &newNameLen, sizeof(int));
	memcpy((char*)msg + sizeof(MsgHeader) + sizeof(int), newNameChar, (sizeof(char) * newNameLen));

	 

	if (oldContent != msg) {
		//send it
		if (comLib.send(msg, totalMsgSize)) {
			MStreamUtils::stdOutStream() << "nodeNameChangedMaterial: Message sent" << "\n";
		}
	}

	
	oldContent = msg;
	delete[]msg;
	delete[]newNameChar; 
	

	/* 
	//pass to send
	bool msgToSend = false;
	if (msgString.length() > 0)
		msgToSend = true;

	if (msgToSend) {
		//sendMsg(CMDTYPE::UPDATE_NAME, NODE_TYPE::MESH, 0, 0, objName, msgString);
	}
	*/
}

// ==================================================================================
// ================================= MESH ===========================================
// ==================================================================================

// Callback function for when a vtx connection is made to the dependency graph (such as triangulated, topology is changed, a new mesh is added)
void vtxPlugConnected(MPlug & srcPlug, MPlug & destPlug, bool made, void* clientData) {
	
	//if statement checking if it's otput mesh that has been connected 
	if (srcPlug.partialName() == "out" && destPlug.partialName() == "i") {

		//if connection made
		if (made == true) {

			MStreamUtils::stdOutStream() << "\n";
			MStreamUtils::stdOutStream() << " ======================================= " << "\n";
			MStreamUtils::stdOutStream() << "vtxPlugConnected: Connection made " << "\n";

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
				MStreamUtils::stdOutStream() << "TRANGULATING" << endl;
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
				mesh.getNormals(normalArray, MSpace::kWorld);

				//get IDs per face
				MIntArray normalIds;
				MIntArray tempNormalIds;
				for (int faceCnt = 0; faceCnt < nrOfTris; faceCnt++) {
					mesh.getFaceNormalIds(faceCnt, tempNormalIds);
					normalIds.append(tempNormalIds[0]);
				}



				// UVs ================

				//get all UVs
				MFloatArray uArray;
				MFloatArray vArray;
				mesh.getUVs(uArray, vArray);

				//get UVs IDs 
				MIntArray uvCounts;
				MIntArray uvIds;
				mesh.getAssignedUVs(uvCounts, uvIds, NULL);


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
					sortedUVs.append(vArray[uvIds[i]]);
				}

				MStreamUtils::stdOutStream() << "uvIds.length() " << uvIds.length() << "\n";
				MStreamUtils::stdOutStream() << "sortedUVs.length() " << sortedUVs.length() << "\n";
				MStreamUtils::stdOutStream() << "meshInfo.trisCount " << meshInfo.trisCount << "\n";
				MStreamUtils::stdOutStream() << "\n";


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
					meshUVs[j] = 1.0f - sortedUVs[j];
					uvPos += 2;
				}



				// =========================
				// Material ================
				// =========================

				Material materialInfo = {};
				MCallbackId tempID;

				// iterate though the lamberts in scene with ItDependencyNodes to find lambert1
				// Assume that when a obj is created in the scene it is bound to lambert 1
				MItDependencyNodes itLamberts(MFn::kLambert);

				while (!itLamberts.isDone()) {
					switch (itLamberts.item().apiType()) {

					case MFn::kLambert: {
						// found lambert, create callback for lambert
						MObject lambertObj(itLamberts.item());

						// assign callback for the material
						tempID = MNodeMessage::addAttributeChangedCallback(lambertObj, nodeMaterialAttributeChanged);
						callbackIdArray.append(tempID);

						//find color plug by creating shader obj och seraching for plugname
						MFnLambertShader fnLambertShader(itLamberts.item());
						MPlug colorPlug = fnLambertShader.findPlug("color");

						//check if color plug is connected, if so assume its connected to a fileTexture
						MPlugArray connetionsColor;
						colorPlug.connectedTo(connetionsColor, true, false);

						//if connectionsColor.length() > 0, material has texture connected
						for (int x = 0; x < connetionsColor.length(); x++) {

							// check for the node of type fileTexture
							if (connetionsColor[x].node().apiType() == MFn::kFileTexture) {

								//if texture was found, create callback for texture plug
								MObject textureObj(connetionsColor[x].node());
								tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
								callbackIdArray.append(tempID);

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


								MStreamUtils::stdOutStream() << "Tex name: " << fileName.asChar() << "\n";


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
							int red = color.r * 255;
							int blue = color.b * 255;
							int green = color.g * 255;
							int alpha = color.a * 255;
							tempColor = { (unsigned char)red, (unsigned char)blue, (unsigned char)green, (unsigned char)alpha };


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

					MStreamUtils::stdOutStream() << "meshInfo.UVcount: " << meshInfo.UVcount << "\n";

					MStreamUtils::stdOutStream() << "sortedUVs.length(): " << sortedUVs.length() << "\n";

					MStreamUtils::stdOutStream() << "meshUV: " << "\n";
					for (int i = 0; i < (sortedUVs.length()); i++)
						MStreamUtils::stdOutStream() << meshUVs[i] << " : ";
					MStreamUtils::stdOutStream() << "================================" << "\n";

					//send it
					if (comLib.send(msg, totalMsgSize)) {
						MStreamUtils::stdOutStream() << "vtxPlugConnected: Message sent" << "\n";
					}


					oldContent = msg;
					oldName = objName;

					delete[]msg;


				}

				// delete allocated arrays
				delete[] meshVtx;
				delete[] meshNormals;
				delete[] meshUVs;


			}

			




		}
	}
	


	// OLD CODE
				/*
				if (result == MS::kSuccess) {

					//get object name to send
					std::string objName = mesh.name().asChar();


					//////////////////////////
					//						//
					//			VTX			//
					//						//
					//////////////////////////

					//get all vertecies
					MPlug vtxArray = mesh.findPlug("controlPoints");
					size_t nrElements = vtxArray.numElements();

					//get vtx in array
					MVectorArray vtxArrayMessy;
					for (int i = 0; i < nrElements; i++)
					{
						//get plug for i array item (gives: shape.vrts[x])
						MPlug currentVtx = vtxArray.elementByLogicalIndex(i);

						float x = 0;
						float y = 0;
						float z = 0;
						//if its has attributes == vertex
						if (currentVtx.isCompound())
						{
							//we have control points [0] but in Maya the values are one hierarchy down so we acces them by getting the child
							MPlug plugX = currentVtx.child(0);
							MPlug plugY = currentVtx.child(1);
							MPlug plugZ = currentVtx.child(2);

							//get value and story them in our xyz
							plugX.getValue(x);
							plugY.getValue(y);
							plugZ.getValue(z);

							//add vtx to array
							MVector tempPoint = { x, y, z };
							vtxArrayMessy.append(tempPoint);
						}
					}

					//sort vtx correctly by index with the triangles in mind
					MIntArray triCount;
					MIntArray triVertsIndex;
					mesh.getTriangles(triCount, triVertsIndex);
					MVectorArray vtxByTriangleSorted;
					for (int i = 0; i < triVertsIndex.length(); i++) {
						vtxByTriangleSorted.append(vtxArrayMessy[triVertsIndex[i]]);
					}


					//old variables DELETE
					int totalTrisCount = triCount.length() * 2;


					int vtxCount = vtxByTriangleSorted.length();

					//Create string to be sent with all the vtx information
					std::string vtxArrayString;
					vtxArrayString.append(to_string(vtxCount) + " ");
					for (int u = 0; u < vtxByTriangleSorted.length(); u++) {
						for (int v = 0; v < 3; v++) {
							vtxArrayString.append(std::to_string(vtxByTriangleSorted[u][v]) + " ");
						}
					}


					//  TEEEEST --------------------------

					float* vtxArrayToSend = new float[vtxCount * 3];
					int arrLen = 0;
					int pos = 0;

					for (int u = 0; u < vtxByTriangleSorted.length(); u++) {

						for (int v = 0; v < 3; v++) {
							vtxArrayToSend[pos + v] = vtxByTriangleSorted[u][v];
							arrLen++;
						}

						pos += 3;

					}




					// -----------------------------------

					//////////////////////////
					//						//
					//		NORMALS			//
					//						//
					//////////////////////////

					//get normals in worldspace
					MFloatVectorArray normals;
					mesh.getNormals(normals, MSpace::kWorld);

					//get IDs per face
					MIntArray normalIds;
					MIntArray tempNormalIds;
					int nrOfFaces = triCount.length();
					for (int faceCnt = 0; faceCnt < nrOfFaces; faceCnt++) {
						mesh.getFaceNormalIds(faceCnt, tempNormalIds);
						normalIds.append(tempNormalIds[0]);
					}

					//get nomral per vtx and sort the normals
					MVectorArray orderedNormals;
					for (int i = 0; i < nrOfFaces; i++) {
						for (int j = 0; j < 3; j++) {
							orderedNormals.append(normals[normalIds[i]]);
						}
					}


					//create string with normals to be sent
					std::string NormArrayString;
					size_t nrOfNormals = orderedNormals.length();
					NormArrayString.append(to_string(nrOfNormals) + " ");
					for (int u = 0; u < orderedNormals.length(); u++) {
						for (int v = 0; v < 3; v++) {
							NormArrayString.append(to_string(orderedNormals[u][v]) + " ");
						}
					}



					//////////////////////////
					//						//
					//			UVS 		//
					//						//
					//////////////////////////

					//get active name for UVset, only supports one UV set (the first one)
					MString UVName;
					mesh.getCurrentUVSetName(UVName, -1);

					//get all UVs
					MFloatArray uArr;
					MFloatArray vArr;
					MString* UVSetNamePointer = new MString[1];
					UVSetNamePointer[0] = UVName;
					mesh.getUVs(uArr, vArr, UVSetNamePointer);

					//get UVs IDs for UVsetname
					MIntArray uvCounts;
					MIntArray uvIds;
					mesh.getAssignedUVs(uvCounts, uvIds, &UVSetNamePointer[0]);

					//sort the UVs per vtx
					MFloatArray orderedUVs;
					size_t totalUVCount = uvIds.length();
					for (int i = 0; i < totalUVCount; i++) {
						orderedUVs.append(uArr[uvIds[i]]);
						orderedUVs.append(vArr[uvIds[i]]);
					}


					//create the string message to be sent
					std::string UVArrayString;
					UVArrayString.append(to_string(totalUVCount) + " ");
					for (int i = 0; i < orderedUVs.length(); i++) {
						UVArrayString.append(to_string(orderedUVs[i]) + " ");
					}



					//////////////////////////
					//						//
					//		MATERIAL		//
					//						//
					//////////////////////////
					std::string materialString = "";

					//iterate though the lamberts in scene to find lambert 1. (Assume that when a obj is created in the scene it is bound to lambert 1).
					MItDependencyNodes itLamberts(MFn::kLambert);
					while (!itLamberts.isDone()) {
						switch (itLamberts.item().apiType())
						{
						case MFn::kLambert:
						{
							//found lambert, create callback for lambert
							MObject lambertObj(itLamberts.item());
							MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(lambertObj, nodeMaterialAttributeChanged);
							callbackIdArray.append(tempID);

							//find color plug by creating shader obj och seraching for plugname
							MFnLambertShader fnLambertShader(itLamberts.item());
							MPlug colorPlug = fnLambertShader.findPlug("color");

							//check if color plug is connected, if so assume its connected to a fileTexture
							MPlugArray connetionsColor;
							colorPlug.connectedTo(connetionsColor, true, false);
							//if connectionsColor.length() > 0 it has texture else color
							for (int x = 0; x < connetionsColor.length(); x++)
							{
								if (connetionsColor[x].node().apiType() == MFn::kFileTexture)
								{
									//if found texture create callback for texture plug
									MObject textureObj(connetionsColor[x].node());
									MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
									callbackIdArray.append(tempID);

									//get texture path to send, search for filetextuename plug
									MFnDependencyNode textureNode(textureObj);
									MPlug fileTextureName = textureNode.findPlug("ftn");
									MString fileName;
									fileTextureName.getValue(fileName);

									//convert to string
									std::string fileNameString = fileName.asChar();
									if (fileNameString.length() > 0)
									{
										//create the string to send with all the informations
										materialString.append(fnLambertShader.name().asChar());
										materialString.append(" texture ");
										materialString.append(fileNameString);
									}

									break;
								}
							}
							if (connetionsColor.length() == 0)
							{
								//find plug for color RGB
								MColor color;
								MPlug attr;
								attr = fnLambertShader.findPlug("colorR");
								attr.getValue(color.r);
								attr = fnLambertShader.findPlug("colorG");
								attr.getValue(color.g);
								attr = fnLambertShader.findPlug("colorB");
								attr.getValue(color.b);

								//create string to be sent
								materialString.append(fnLambertShader.name().asChar());
								materialString.append(" color ");
								materialString.append(to_string(color.r) + " ");
								materialString.append(to_string(color.g) + " ");
								materialString.append(to_string(color.b));
							}
						}
						break;

						default:
							break;
						}
						itLamberts.next();
					}


					//create final string to be sent to the raylib
					std::string msgString;
					msgString.append(vtxArrayString + " ");
					msgString.append(NormArrayString + " ");
					msgString.append(UVArrayString + " ");
					msgString.append(materialString);


					//pass to send
					bool msgToSend = false;
					if (vtxCount > 0)
						msgToSend = true;

					if (oldName != objName) {

						if (msgToSend) {
							//sendMsg(CMDTYPE::NEW_NODE, NODE_TYPE::MESH, nrElements, totalTrisCount, objName, msgString);



							size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * (vtxCount * 3)));
							const char* msg = new char[totalMsgSize];


							// Fill header =================
							MsgHeader msgHeader;
							msgHeader.nodeType = NODE_TYPE::MESH;
							msgHeader.cmdType = CMDTYPE::NEW_NODE;
							msgHeader.nameLen = objName.length();
							msgHeader.msgSize = totalMsgSize;
							memcpy(msgHeader.objName, objName.c_str(), objName.length());

							//define mesh struct variables
							Mesh mesh;
							mesh.trisCount = totalTrisCount;
							mesh.vtxCount = nrElements;

							// Copy MSG ==================


							//size_t lenghtVtx = vtxArrayString.length();
							//MStreamUtils::stdOutStream() << "lenghtVtx: " << lenghtVtx << "\n";
							//MStreamUtils::stdOutStream() << "totalMsgSize: " << totalMsgSize << "\n";

							memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
							memcpy((char*)msg + sizeof(MsgHeader), &mesh, sizeof(Mesh));
							memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh), vtxArrayToSend, sizeof(float) * (mesh.vtxCount * 3));
							//memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh), msgString.c_str(), msgHeader.msgSize);

							//send it
							if (comLib.send(msg, totalMsgSize)) {
								MStreamUtils::stdOutStream() << "vtxPlugConnected: Message sent" << "\n";
							}


							oldContent = msg;
							oldName = objName;

							delete[]msg;
						}

					}
					delete[] UVSetNamePointer;
					delete[] vtxArrayToSend;
				}

				*/
}

// Callback function for when an attribute is changed for a node
void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

	MStreamUtils::stdOutStream() << "\n";
	MStreamUtils::stdOutStream() << "Attribute changed " << "\n";

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
	mesh.getNormals(normalArray, MSpace::kWorld);

	//get IDs per face
	MIntArray normalIds;
	MIntArray tempNormalIds;
	for (int faceCnt = 0; faceCnt < nrOfTris; faceCnt++) {
		mesh.getFaceNormalIds(faceCnt, tempNormalIds);
		normalIds.append(tempNormalIds[0]);
	}


	// UVs ================

	//get all UVs
	MFloatArray uArray;
	MFloatArray vArray;
	mesh.getUVs(uArray, vArray);

	//get UVs IDs 
	MIntArray uvCounts;
	MIntArray uvIds;
	mesh.getAssignedUVs(uvCounts, uvIds, NULL);


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
		sortedUVs.append(vArray[uvIds[i]]);
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
		meshUVs[j] = 1.0f - sortedUVs[j];
		uvPos += 2;
	}

	//MStreamUtils::stdOutStream() << "\n";

	//MStreamUtils::stdOutStream() << "sortedVtxArray.length(): " << sortedVtxArray.length() << "\n";
	//MStreamUtils::stdOutStream() << "VTX POS: " << vtxPos << "\n";
	//MStreamUtils::stdOutStream() << "uvPos: " << uvPos << "\n";

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

		delete[]msg;


	}

	// delete allocated arrays
	delete[] meshVtx;
	delete[] meshNormals;
	delete[] meshUVs;


	// OLD CODE
	/*
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	//////////////////////////
	//						//
	//			VTX			//
	//						//
	//////////////////////////

				//get all vertecies
	MPlug vtxArray = mesh.findPlug("controlPoints");
	size_t nrElements = vtxArray.numElements();

	//get vtx in array
	MVectorArray vtxArrayMessy;
	for (int i = 0; i < nrElements; i++)
	{
		//get plug for i array item (gives: shape.vrts[x])
		MPlug currentVtx = vtxArray.elementByLogicalIndex(i);

		float x = 0;
		float y = 0;
		float z = 0;
		//if its has attributes == vertex
		if (currentVtx.isCompound())
		{
			//we have control points [0] but in Maya the values are one hierarchy down so we acces them by getting the child
			MPlug plugX = currentVtx.child(0);
			MPlug plugY = currentVtx.child(1);
			MPlug plugZ = currentVtx.child(2);

			//get value and story them in our xyz
			plugX.getValue(x);
			plugY.getValue(y);
			plugZ.getValue(z);

			//add vtx to array
			MVector tempPoint = { x, y, z };
			vtxArrayMessy.append(tempPoint);
		}
	}

	//sort vtx correctly by index with the triangles in mind
	MIntArray triCount;
	MIntArray triVertsIndex;
	mesh.getTriangles(triCount, triVertsIndex);
	MVectorArray vtxByTriangleSorted;
	for (int i = 0; i < triVertsIndex.length(); i++) {
		vtxByTriangleSorted.append(vtxArrayMessy[triVertsIndex[i]]);
	}

	//old variables DELETE
	int totalTrisCount = triCount.length() * 2;

	//get object name to send
	std::string objName = mesh.name().asChar();

	//Create string to be sent with all the vtx information
	std::string vtxArrayString;
	int vtxCount = vtxByTriangleSorted.length();
	vtxArrayString.append(to_string(vtxCount) + " ");
	for (int u = 0; u < vtxByTriangleSorted.length(); u++) {
		for (int v = 0; v < 3; v++) {
			vtxArrayString.append(std::to_string(vtxByTriangleSorted[u][v]) + " ");
		}
	}


	//////////////////////////
	//						//
	//		NORMALS			//
	//						//
	//////////////////////////

	//get normals in worldspace
	MFloatVectorArray normals;
	mesh.getNormals(normals, MSpace::kWorld);

	//get IDs per face
	MIntArray normalIds;
	MIntArray tempNormalIds;
	int nrOfFaces = triCount.length();
	for (int faceCnt = 0; faceCnt < nrOfFaces; faceCnt++) {
		mesh.getFaceNormalIds(faceCnt, tempNormalIds);
		normalIds.append(tempNormalIds[0]);
	}

	//get nomral per vtx and sort the normals
	MVectorArray orderedNormals;
	for (int i = 0; i < nrOfFaces; i++) {
		for (int j = 0; j < 3; j++) {
			orderedNormals.append(normals[normalIds[i]]);
		}
	}

	//create string with normals to be sent
	std::string NormArrayString;
	size_t nrOfNormals = orderedNormals.length();
	NormArrayString.append(to_string(nrOfNormals) + " ");
	for (int u = 0; u < orderedNormals.length(); u++) {
		for (int v = 0; v < 3; v++) {
			NormArrayString.append(to_string(orderedNormals[u][v]) + " ");
		}
	}


	//////////////////////////
	//						//
	//			UVS 		//
	//						//
	//////////////////////////

	//get active name for UVset, only supports one UV set (the first one)
	MString UVName;
	mesh.getCurrentUVSetName(UVName, -1);

	//get all UVs
	MFloatArray uArr;
	MFloatArray vArr;
	MString* UVSetNamePointer = new MString[1];
	UVSetNamePointer[0] = UVName;
	mesh.getUVs(uArr, vArr, UVSetNamePointer);

	//get UVs IDs for UVsetname
	MIntArray uvCounts;
	MIntArray uvIds;
	mesh.getAssignedUVs(uvCounts, uvIds, &UVSetNamePointer[0]);

	//sort the UVs per vtx
	MFloatArray orderedUVs;
	size_t totalUVCount = uvIds.length();
	for (int i = 0; i < totalUVCount; i++) {
		orderedUVs.append(uArr[uvIds[i]]);
		orderedUVs.append(vArr[uvIds[i]]);
	}

	//create the string message to be sent
	std::string UVArrayString;
	UVArrayString.append(to_string(totalUVCount) + " ");
	for (int i = 0; i < orderedUVs.length(); i++) {
		UVArrayString.append(to_string(orderedUVs[i]) + " ");
	}

	std::string masterTransformString;
	masterTransformString.append(vtxArrayString + " ");
	masterTransformString.append(NormArrayString + " ");
	masterTransformString.append(UVArrayString);

	//MStreamUtils::stdOutStream() << "masterTransformString: " << masterTransformString << "_" << endl;

	if (oldContent != masterTransformString)
	{
		//pass to send
		bool msgToSend = false;
		if (vtxCount > 0)
			msgToSend = true;

		if (msgToSend) {
			//sendMsg(CMDTYPE::UPDATE_NODE, NODE_TYPE::MESH, nrElements, totalTrisCount, objName, masterTransformString);
		}
	}
	*/

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

	/* 
	MStreamUtils::stdOutStream() <<  "\n";
	MStreamUtils::stdOutStream() << "worldMatrix: " << worldMatrix << "\n";
	MStreamUtils::stdOutStream() << "matrixInfo: " << matrixInfo.a11 << " : ";
	MStreamUtils::stdOutStream() << matrixInfo.a12 << " : ";
	MStreamUtils::stdOutStream() << matrixInfo.a13 << " : ";
	MStreamUtils::stdOutStream() << matrixInfo.a14 << "\n";
	*/



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
		MStreamUtils::stdOutStream() << "nodeWorldMatrixChanged: Message sent" << "\n";
	}

	oldContent = msg;
	oldName = objName;

	delete[] msg; 

	
	/* 
	if (oldContent != msgString)
	{
		//pass to send
		bool msgToSend = false;
		if (msgString.length() > 0)
			msgToSend = true;

		if (msgToSend) {
			//sendMsg(CMDTYPE::UPDATE_MATRIX, NODE_TYPE::MESH, 0, 0, objName, msgString);
		}
	}
	*/
	/* 
	//creat string from all the vector rows
	std::string row4;
	row4.append(to_string(worldMatrix(0, 3)) + " ");
	row4.append(to_string(worldMatrix(1, 3)) + " ");
	row4.append(to_string(worldMatrix(2, 3)) + " ");
	row4.append(to_string(worldMatrix(3, 3)) + " ");

	std::string row2;
	row2.append(to_string(worldMatrix(0, 1)) + " ");
	row2.append(to_string(worldMatrix(1, 1)) + " ");
	row2.append(to_string(worldMatrix(2, 1)) + " ");
	row2.append(to_string(worldMatrix(3, 1)) + " ");

	std::string row3;
	row3.append(to_string(worldMatrix(0, 2)) + " ");
	row3.append(to_string(worldMatrix(1, 2)) + " ");
	row3.append(to_string(worldMatrix(2, 2)) + " ");
	row3.append(to_string(worldMatrix(3, 2)) + " ");

	std::string row1;
	row1.append(to_string(worldMatrix(0, 0)) + " ");
	row1.append(to_string(worldMatrix(1, 0)) + " ");
	row1.append(to_string(worldMatrix(2, 0)) + " ");
	row1.append(to_string(worldMatrix(3, 0)) + " ");

	//create string to send to raylib
	std::string objName = mesh.name().asChar();
	std::string msgString;
	msgString.append(row1);
	msgString.append(row2);
	msgString.append(row3);
	msgString.append(row4);

	if (oldContent != msgString)
	{
		//pass to send
		bool msgToSend = false;
		if (msgString.length() > 0)
			msgToSend = true;

		if (msgToSend) {
			//sendMsg(CMDTYPE::UPDATE_MATRIX, NODE_TYPE::MESH, 0, 0, objName, msgString);
		}
	}
	*/
}

// ==================================================================================
// ================================ CAMERA ==========================================
// ==================================================================================

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
	MPoint camPos = cameraView.eyePoint(MSpace::kWorld, &status);
	MVector upDir = cameraView.upDirection(MSpace::kWorld, &status);
	MVector rightDir = cameraView.rightDirection(MSpace::kWorld, &status);
	MPoint COI = cameraView.centerOfInterestPoint(MSpace::kWorld, &status);

	//get fov or zoom depending on ortho/persp
	bool isOrtographic = cameraView.isOrtho();
	if (isOrtographic) {
		double ortoWidth = cameraView.orthoWidth(&status);
		FOV = ortoWidth;
	}
	else {
		double horizontalFOV;
		double verticalFOV;
		cameraView.getPortFieldOfView(width, height, horizontalFOV, verticalFOV);
		FOV = verticalFOV;
	}

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

	}



	//OLD CODE
	/*
//create string with all the information needed
std::string msgString;
msgString.append(camName.asChar());
msgString.append(" ");
msgString.append(std::to_string(upDir.x) + " " + std::to_string(upDir.y) + " " + std::to_string(upDir.z) + " ");
msgString.append(std::to_string(COI.x) + " " + std::to_string(COI.y) + " " + std::to_string(COI.z) + " ");
msgString.append(std::to_string(camPos.x) + " " + std::to_string(camPos.y) + " " + std::to_string(camPos.z) + " ");
msgString.append(std::to_string(FOV));
*/


}

// ==================================================================================
// ================================= LIGHT ==========================================
// ==================================================================================

//not sending 
void nodeLightAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {
	
	// only do this if the msg is a change of attributes (pos, color etc)
	if (msg & MNodeMessage::kAttributeSet) {
		
		MStreamUtils::stdOutStream() << "nodeLightAttributeChanged" << "\n";

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
		int red = lightColor.r * 255;
		int blue = lightColor.b * 255;
		int green = lightColor.g * 255;
		int alpha = lightColor.a * 255;
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


		/* 
		MStreamUtils::stdOutStream() << endl;
		MStreamUtils::stdOutStream() << "nodeLightAttributeChanged " << endl;
		//only supports one light

		// get path of the light
		MDagPath path;
		MFnDagNode(plug.node()).getPath(path);
		MFnLight sceneLight(path);

		//get light color and intensity, OBS raylib doesnt have light ish so intensityn is not used. 
		MPlug colorPlug = sceneLight.findPlug("color");
		MPlug intensityPlug = sceneLight.findPlug("intensity");



		//create string to send
		std::string msgString = "";
		msgString.append(to_string(sceneLight.color().r) + " ");
		msgString.append(to_string(sceneLight.color().g) + " ");
		msgString.append(to_string(sceneLight.color().b) + " ");
		msgString.append(to_string(sceneLight.color().a) + " ");

		//pass to send
		bool msgToSend = false;
		if (msgString.length() > 0)
			msgToSend = true;

		if (msgToSend) {
			//sendMsg(CMDTYPE::UPDATE_NODE, NODE_TYPE::LIGHT, 0, 0, "light", msgString);
		}
		*/
	}
}

//not sending 
void vtxPlugConnectedLight(MPlug & srcPlug, MPlug & destPlug, bool made, void* clientData) {
	
	MStreamUtils::stdOutStream() << "in vtxPlugConnectedLight " << endl;
	MStreamUtils::stdOutStream() << "srcPlug " << srcPlug.name()  << endl;

}

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
	int red   = lightColor.r * 255;
	int blue  = lightColor.b * 255;
	int green = lightColor.g * 255;
	int alpha = lightColor.a * 255;
	tempColor = { (unsigned char)red, (unsigned char)blue, (unsigned char)green, (unsigned char)alpha};

	// fill light struct 
	Light lightInfo = {}; 
	lightInfo.color = tempColor; 
	lightInfo.intensity = intensity; 
	lightInfo.lightPos = { (float)lightPos.x, (float)lightPos.y, (float)lightPos.z };

	MStreamUtils::stdOutStream() << "Light Color: " << lightColor << endl;
	MStreamUtils::stdOutStream() << "Light intensity: " << intensity << endl;
	MStreamUtils::stdOutStream() << "name: " << lightNode.name() << endl;

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


// ==================================================================================
// ================================= OTHER ==========================================
// ==================================================================================

// callback that goes through nodes that have been created and attatches callbacks
void nodeAdded(MObject &node, void * clientData) {

	//we register callbacks for both the new mesh kTransform and kMesh. the KTransform is in charge of the rotationg, scaling, transltion on a mesh or vrts. The kMesh is in charge of ex. subdividing or adding more vrts on the mesh.
	//we want callbacks for both beacuse we need both "transformation" information.

	MCallbackId tempID; 
	MStatus Result = MS::kSuccess;

	// if the node has kMesh attributes it is a mesh
	if (node.hasFn(MFn::kMesh)) {

		MStreamUtils::stdOutStream() << "nodeAdded: MESH was added " << endl;
		meshQueue.push(node);
		
		tempID = MDGMessage::addConnectionCallback(vtxPlugConnected, NULL, &status);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
		
		tempID = MDGMessage::addConnectionCallback(meshConnectionChanged, NULL, &Result);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
	}

	// if the node has kLight attributes it is a light
	if (node.hasFn(MFn::kLight)) {

		MStreamUtils::stdOutStream() << "nodeAdded: LIGHT was added " << endl;
		lightQueue.push(node);
		
		tempID = MDGMessage::addConnectionCallback(vtxPlugConnectedLight, NULL, &status);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
		
	}

	//... implement this and other callbacks
	//this function is called whenever a node is added. should pass to appropriate funtions(?)
}

// callback that goes through nodes that have been deleted
void nodeDeleted(MObject &node, void *clientData) {

	MStreamUtils::stdOutStream() << "\n";
	MStreamUtils::stdOutStream() << "NODE DELETED";

	if (node.hasFn(MFn::kMesh)) {

		MFnMesh mesh(node);

		MStreamUtils::stdOutStream() << "node name: " << mesh.name();
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
		MStreamUtils::stdOutStream() << "light to delete";

	}
}

// NEEDS UPDATE
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

			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			tempID = MNodeMessage::addNameChangedCallback(currenNode, nodeNameChangedMaterial, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
		}

		meshQueue.pop();
	}

	for (int i = 0; i < lightQueue.size(); i++) {

		MObject currenNode = lightQueue.back();
		if (currenNode.hasFn(MFn::kLight)) {

			//MStreamUtils::stdOutStream() << "LIGHT NODE " << endl;
			MDagPath path;
			MFnLight sceneLight(currenNode);
			MFnDagNode(currenNode).getPath(path);
			
			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			MObject parentNode = sceneLight.parent(0); 
			tempID = MNodeMessage::addAttributeChangedCallback(parentNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			/* 
			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixChangedLight, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
			*/
			

			//GetLightInfo(sceneLight);

			/*
			MColor lightColor;
			lightColor = sceneLight.color();
			MStreamUtils::stdOutStream() << "Light Color: " << lightColor << endl;
			MStreamUtils::stdOutStream() << "Light intensity: " << sceneLight.intensity() << endl;

			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}



			MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

			MStreamUtils::stdOutStream() << "Transform World2 Matrix: " << worldMatrix << endl;

			*/


			/* 

			std::string translateVector;
			translateVector.append(to_string(worldMatrix(3, 0)) + " ");
			translateVector.append(to_string(worldMatrix(3, 1)) + " ");
			translateVector.append(to_string(worldMatrix(3, 2)) + " ");

			//add command + name to string
			std::string objName = "light";

			//MVector to string
			std::string msgString;
			msgString.append(translateVector);
			msgString.append(to_string(lightColor.r) + " ");
			msgString.append(to_string(lightColor.g) + " ");
			msgString.append(to_string(lightColor.b) + " ");
			msgString.append(to_string(lightColor.a) + " ");

			//pass to send
			bool msgToSend = false;
			if (msgString.length() > 0)
				msgToSend = true;

			if (msgToSend) {
				//sendMsg(CMDTYPE::NEW_NODE, NODE_TYPE::LIGHT, 0, 0, objName, msgString);
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
	MStreamUtils::stdOutStream() << "Plugin loaded ===========================\n";


	// register callbacks here for
	tempID = MDGMessage::addNodeAddedCallback(nodeAdded);
	callbackIdArray.append(tempID);

	tempID = MDGMessage::addNodeRemovedCallback(nodeDeleted);
	callbackIdArray.append(tempID);

	tempID = MTimerMessage::addTimerCallback(timerPeriod, timerCallback, NULL, &status);
	callbackIdArray.append(tempID);

	/////////////////////////////////////////////////
	//											   //
	// Camera callbacks for all active view panels //
	//											   //
	/////////////////////////////////////////////////

	// camera callbacks initialized for all the view panels 
	tempID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel1", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);
	}

	tempID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel2", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);

	}

	tempID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel3", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);
	}

	tempID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel4", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(tempID);
	}


	// a handy timer, courtesy of Maya
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

// OLD SEND MSG CODE
/* 
bool sendMsg(CMDTYPE msgType, NODE_TYPE nodeT, int nrOfElements, int trisCount, std::string objName, std::string &msgString) {

	bool sent = false;

	// Fill header ================= 
	MsgHeader msgHeader;
	msgHeader.nodeType = nodeT;
	msgHeader.cmdType = msgType;
	msgHeader.nameLen = objName.length();
	msgHeader.msgSize = msgString.size() + 1;
	memcpy(msgHeader.objName, objName.c_str(), objName.length());

	//MStreamUtils::stdOutStream() << "msgString: " << msgString << "_" << endl;

	if (nodeT == NODE_TYPE::MESH) {

		//define mesh struct variables
		Mesh mesh;
		mesh.trisCount = trisCount;
		mesh.vtxCount = nrOfElements;

		// Copy MSG ================== 
		size_t totalMsgSize = (sizeof(MsgHeader) + msgHeader.msgSize + sizeof(Mesh));
		char* msg = new char[totalMsgSize];

		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &mesh, sizeof(Mesh));
		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh), msgString.c_str(), msgHeader.msgSize);

		//send it
		sent = comLib.send(msg, totalMsgSize);
		delete[]msg;

	}
	else {

		//if no mesh we dont need the mesh struct
		// Copy MSG ================== 
		size_t totalMsgSize = (sizeof(MsgHeader) + msgHeader.msgSize);
		char* msg = new char[totalMsgSize];

		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), msgString.c_str(), msgHeader.msgSize);

		sent = comLib.send(msg, totalMsgSize);
		// =========================== 
		delete[]msg;
	}

	//check for dublicat msgs
	oldContent = msgString;
	oldName = objName;
	return sent;

}
*/
