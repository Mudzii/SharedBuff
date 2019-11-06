//#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>


#include "Header.h"
#include "comLib.h"
#pragma comment(lib, "Project1.lib")
ComLib comLib("shaderMemory", 50, PRODUCER);

// ===========================================================
// ==================== Variables ============================
// ===========================================================

MStatus status = MS::kSuccess;
MCallbackIdArray callbackIdArray;
bool initBool = false;

MTimer gTimer;
float globalTime  = 0.0f;
float timerPeriod = 0.1f; 

// keep track of created meshes/ lights to maintain them
//std::queue<MObject> meshQueue;
//std::queue<MObject> lightQueue;
//std::queue<MObject> transformQueue; 
//std::queue<MObject> materialQueue; 

//std::string oldContent = "";
//std::string oldName	  = "";


// Output messages in output window
//MStreamUtils::stdOutStream() << "...: " << ... << "_" << endl;

MStringArray meshesInScene; 

std::vector<MeshInfo> meshInfoToSend; 
std::vector<LightInfo> lightInfoToSend;
std::vector<CameraInfo> cameraInfoToSend;
std::vector<MaterialInfo> MaterialInfoToSend;
std::vector<TransformInfo> TransformInfoToSend;
std::vector<NodeDeletedInfo> nodeDeleteInfoToSend;
std::vector<NodeRenamedInfo> nodeRenamedInfoToSend;


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
 
void transformWorldMatrixChanged(MObject &transformNode, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	MStatus result;
	MDagPath path;
	MFnDagNode(transformNode).getPath(path);
	MFnTransform transfNode(path, &result);

	if (result) {

		//MStreamUtils::stdOutStream() << endl;
		//MStreamUtils::stdOutStream() << " =============================== " << endl;
		//MStreamUtils::stdOutStream() << "TRANSFORM MAT CHANGED " << endl;
		

		// check if transform is directly connected to mesh
		bool hasShapeNode = false; 
		MFnMesh meshNode(path, &result);
		if (result) {
			hasShapeNode = true; 
			MStreamUtils::stdOutStream() << "mesh of transform: " << meshNode.name() << endl;
		}
		

		// if not directly connected to mesh, check if in hierarchy
		MObjectArray hierarchy; 
		int totalChildCnt = 0; 

		MObject firstParentNode(transfNode.object());
		hierarchy.append(firstParentNode);
		int childCnt = transfNode.childCount(); 

		if (!hasShapeNode) {

			for (int j = 0; j < hierarchy.length(); j++) {

				MFnDagNode prnt(hierarchy[j]);
				int prntChildCnt = prnt.childCount();

				for (int i = 0; i < prntChildCnt; i++) {

					MObject childObject(prnt.child(i));

					if (childObject.apiType() == MFn::kTransform || childObject.apiType() == MFn::kMesh) {

						MFnDagNode child(childObject);
						int tempChildCnt = child.childCount(); 
						std::string childName = child.fullPathName().asChar();

						if (tempChildCnt != 0 || childName.find("Shape") != std::string::npos) {

							hierarchy.append(childObject);
							totalChildCnt += 1; 
							//MStreamUtils::stdOutStream()  << endl;
							//MStreamUtils::stdOutStream() << "child " << child.fullPathName() << endl;
							//MStreamUtils::stdOutStream() << "child cnt " << tempChildCnt << endl;
							//MStreamUtils::stdOutStream()  << endl;
						}

						if (childName.find("Shape") != std::string::npos) {
							hasShapeNode = true; 

						}


					}



				}
			}

			MStreamUtils::stdOutStream()  << endl;

		}
		
		if (hasShapeNode) {

			
			MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());
			MMatrix localMatrix = MMatrix(path.exclusiveMatrix());
		}



		//MStreamUtils::stdOutStream() << totalChildCnt  << endl;


	}

}


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
	
	Color mColor; 
	Matrix matrixInfo = {};
	MeshInfo mMeshInfo = {};

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
	
	// transform matrix ===========================
	MFnTransform parentTransform(mesh.parent(0), &result); 
	MMatrix transformationMatrix; 
	if (result) {
		transformationMatrix = parentTransform.transformationMatrix(); 
	}

	else {
		transformationMatrix = mesh.transformationMatrix(); 
	}

	// material ===================================
	
	// lambert material 
	MColor color;
	MPlug colorPlug;
	MFnLambertShader lambertShader;
	std::string materialNamePlug; 

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
		//MStreamUtils::stdOutStream() << "Shader: " << initialShadingGroup.name() << endl;

		MPlug surfaceShader = initialShadingGroup.findPlug("surfaceShader", &result); 

		if (result) {
			//MStreamUtils::stdOutStream() << "Found plug!" << endl;

			// to find material, find the destination connections for surficeShader plug
			// (AKA plugs that have surface shader as destination)
			surfaceShader.connectedTo(shaderConnections, true, false); 

			//MStreamUtils::stdOutStream() << "shaderConnections: " << shaderConnections.length() << endl;

			if (shaderConnections.length() > 0) {

				//Only support 1 material per mesh + lambert. Check if first connection is lambert
				if (shaderConnections[0].node().apiType() == MFn::kLambert) {

					lambertShader.setObject(shaderConnections[0].node()); 
					materialNamePlug = lambertShader.name().asChar(); 
					//MStreamUtils::stdOutStream() << "materialNamePlug: " << materialNamePlug << endl;
					
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

						// convert from 0-1 to 0-255
						int red   = color.r * 255;
						int blue  = color.b * 255;
						int green = color.g * 255;
						int alpha = color.a * 255;
						mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };

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


		


		}
	}


	// check if mesh already exists in scene, otherwise add
	int index = -1;
	for (int i = 0; i < meshesInScene.length(); i++) {
		if (meshesInScene[i] == mesh.name()) {
			index = i;
			mMeshInfo.msgHeader.cmdType = UPDATE_NODE;
			break;
		}
	}

	if (index == -1) {
		
		meshesInScene.append(mesh.name());
		mMeshInfo.msgHeader.cmdType = NEW_NODE;
		index = 0; 
	}

	// float arrays that store mesh info  ================
	float* meshUVs	   = new float[uvArray.length()];
	float* meshVtx	   = new float[vtxArray.length() * 3];
	float* meshNormals = new float[normalArray.length() * 3];

	// fill arrays with info
	int vtxPos = 0;
	for (int i = 0; i < vtxArray.length(); i++) {

		// vtx
		meshVtx[vtxPos]		= vtxArray[i][0];
		meshVtx[vtxPos + 1] = vtxArray[i][1];
		meshVtx[vtxPos + 2] = vtxArray[i][2];

		// normals
		meshNormals[vtxPos]		= normalArray[i][0];
		meshNormals[vtxPos + 1] = normalArray[i][1];
		meshNormals[vtxPos + 2] = normalArray[i][2];

		vtxPos += 3;
	}

	for (int j = 0; j < uvArray.length(); j++) {
		meshUVs[j] = uvArray[j];
	}


	// fill msg to queue 
	mMeshInfo.meshData.meshID = index;
	mMeshInfo.meshData.trisCount = totalTrisCnt;
	mMeshInfo.meshData.UVcount = uvArray.length();
	mMeshInfo.meshData.vtxCount = vtxArray.length();
	mMeshInfo.meshData.normalCount = normalArray.length();

	mMeshInfo.meshName     = mesh.name();
	mMeshInfo.meshPathName = mesh.fullPathName();

	mMeshInfo.materialData.color = mColor;
	mMeshInfo.materialData.matNameLen = materialNamePlug.length();
	memcpy(mMeshInfo.materialData.materialName, materialNamePlug.c_str(), mMeshInfo.materialData.matNameLen);

	if (hasTexture) {
		mMeshInfo.materialData.type = 1;
		mMeshInfo.materialData.textureNameLen = fileNameString.length();
		memcpy(mMeshInfo.materialData.fileTextureName, fileNameString.c_str(), mMeshInfo.materialData.textureNameLen);

	}

	else {
		mMeshInfo.materialData.type = 0;
		mMeshInfo.materialData.textureNameLen = 2;
		memcpy(mMeshInfo.materialData.fileTextureName, "NA", mMeshInfo.materialData.textureNameLen);

	}
	//row 1
	matrixInfo.a11 = transformationMatrix(0, 0);
	matrixInfo.a12 = transformationMatrix(1, 0);
	matrixInfo.a13 = transformationMatrix(2, 0);
	matrixInfo.a14 = transformationMatrix(3, 0);

	//row 2
	matrixInfo.a21 = transformationMatrix(0, 1);
	matrixInfo.a22 = transformationMatrix(1, 1);
	matrixInfo.a23 = transformationMatrix(2, 1);
	matrixInfo.a24 = transformationMatrix(3, 1);

	//row 3
	matrixInfo.a31 = transformationMatrix(0, 2);
	matrixInfo.a32 = transformationMatrix(1, 2);
	matrixInfo.a33 = transformationMatrix(2, 2);
	matrixInfo.a34 = transformationMatrix(3, 2);
	//row 4
	matrixInfo.a41 = transformationMatrix(0, 3);
	matrixInfo.a42 = transformationMatrix(1, 3);
	matrixInfo.a43 = transformationMatrix(2, 3);
	matrixInfo.a44 = transformationMatrix(3, 3);

	mMeshInfo.transformMatrix = matrixInfo; 

	// Fill header ========
	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (2 * (sizeof(float) * mMeshInfo.meshData.vtxCount * 3)) + (sizeof(float) * mMeshInfo.meshData.UVcount) + sizeof(Material) + sizeof(Matrix));

	mMeshInfo.msgHeader.msgSize = totalMsgSize;
	mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH;
	mMeshInfo.msgHeader.nameLen = mesh.name().length();
	memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);
	
	// check if msg already exists. If so, update
	bool msgExists = false;
	for (int i = 0; i < meshInfoToSend.size(); i++) {
		if (meshInfoToSend[i].meshName == mesh.name()) {
			msgExists = true;

			if (meshInfoToSend[i].meshData.vtxCount != mMeshInfo.meshData.vtxCount) {

				delete[] meshInfoToSend[i].meshVtx; 
				delete[] meshInfoToSend[i].meshNrms;

				meshInfoToSend[i].meshVtx = new float[mMeshInfo.meshData.vtxCount * 3];
				meshInfoToSend[i].meshNrms = new float[mMeshInfo.meshData.normalCount * 3];

			}

			if (meshInfoToSend[i].meshData.UVcount != mMeshInfo.meshData.UVcount) {
				delete[] meshInfoToSend[i].meshUVs;
				meshInfoToSend[i].meshUVs = new float[mMeshInfo.meshData.UVcount];
			}

			//meshInfoToSend[i].meshData.UVcount = mMeshInfo.meshData.UVcount; 
			//meshInfoToSend[i].meshData.vtxCount = mMeshInfo.meshData.vtxCount; 
			//meshInfoToSend[i].meshData.normalCount = mMeshInfo.meshData.normalCount; 

			meshInfoToSend[i].meshData = mMeshInfo.meshData; 

			memcpy((char*)meshInfoToSend[i].meshVtx,  meshVtx,	   (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));	
			memcpy((char*)meshInfoToSend[i].meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));
			memcpy((char*)meshInfoToSend[i].meshUVs,  meshUVs,	   (sizeof(float) * (mMeshInfo.meshData.UVcount)));

			meshInfoToSend[i].materialData	  = mMeshInfo.materialData; 
			meshInfoToSend[i].transformMatrix = mMeshInfo.transformMatrix; 


			break; 
		}
	}

	// if it doesn't exist, add it to the queue 
	if (!msgExists) {
		
		mMeshInfo.meshVtx  = new float[mMeshInfo.meshData.vtxCount * 3];
		mMeshInfo.meshNrms = new float[mMeshInfo.meshData.normalCount * 3];
		mMeshInfo.meshUVs  = new float[mMeshInfo.meshData.UVcount];

		memcpy((char*)mMeshInfo.meshVtx,  meshVtx,	   (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
		memcpy((char*)mMeshInfo.meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));
		memcpy((char*)mMeshInfo.meshUVs,  meshUVs,	   (sizeof(float) * (mMeshInfo.meshData.UVcount)));

		meshInfoToSend.push_back(mMeshInfo);
	}

	delete[] meshVtx; 
	delete[] meshUVs; 
	delete[] meshNormals; 

}

/*
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
*/

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
/* 
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
			

		}
	}
}

*/
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
			GetMeshInfo(meshNode);
		}

	}

	// if vtx is changed
	else if (plugPartName.find("pt[") != std::string::npos) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "Vertex changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;

		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GetMeshInfo(meshNode);
		}
	}
	
	// if normal has changed
	else if (otherPlugName.find("polyNormal") != std::string::npos || (otherPlugName.find("polySoftEdge") != std::string::npos && otherPlugName.find("manipMatrix") != std::string::npos)) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "normal changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;

		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GetMeshInfo(meshNode);
		}
	}

	// if pivot changed, UV probably changed
	else if (plugPartName.find("pv") != std::string::npos) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "UV changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;
		if (result) {
			MStreamUtils::stdOutStream() << "In result statement" << endl;
			GetMeshInfo(meshNode);
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

void meshTopologyChanged(MObject& node, void* clientData) {

	if (node.hasFn(MFn::kMesh)) {
		MStatus stat;
		MFnMesh mesh(node, &stat);
		if (stat == MS::kSuccess) {
			MStreamUtils::stdOutStream() << " =============================== " << endl;
			MStreamUtils::stdOutStream() << "TOPOLOGY CHANGED on " << mesh.name() << endl;


		}
	}

}

void meshWorldMatrixChanged(MObject &transformNode, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	MStatus result;
	MDagPath path; 
	MFnDagNode(transformNode).getPath(path);

	MFnTransform transfNode(path);
	MFnMesh meshNode(path, &result); 

	if (result) {
		//MStreamUtils::stdOutStream() << endl;

		//MStreamUtils::stdOutStream() << " =============================== " << endl;
		//MStreamUtils::stdOutStream() << "meshWorldMatrixChanged for " << meshNode.name() << endl;
		//MStreamUtils::stdOutStream() << "transfNoder " << transfNode.name() << endl;

		// inclusive matrix = world matrix. exclusive matrix = local matrix
		MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());		
		MMatrix localMatrix = MMatrix(path.exclusiveMatrix());	

	}

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

	//MStreamUtils::stdOutStream() << "=================================\n";
	//MStreamUtils::stdOutStream() << "cameraWorldMatrixChanged\n";

	M3dView activeView = M3dView::active3dView();

	MDagPath path;
	activeView.getCamera(path);
	MFnCamera cameraNode(path);

	std::string cameraName = cameraNode.name().asChar(); 
	//MStreamUtils::stdOutStream() << "camera: " << cameraName << endl;


	// get height/width
	int height = activeView.portHeight();
	int width  = activeView.portWidth();
	double FOV = 0; 
	//get relevant vectors for raylib

	MPoint camPos	 = cameraNode.eyePoint(MSpace::kWorld, &status);
	MVector upDir	 = cameraNode.upDirection(MSpace::kWorld, &status);
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


void nodeNameChanged(MObject& node, const MString& str, void* clientData) {

	//MStreamUtils::stdOutStream() << "=========================" << endl;
	//MStreamUtils::stdOutStream() << "NAME CHANGED " << endl;

	std::string oldName;
	std::string newName;

	if (node.hasFn(MFn::kMesh)) {

		MFnMesh meshNode(node);

		oldName = str.asChar();
		newName = meshNode.fullPathName().asChar();

		
	}

	else if (node.hasFn(MFn::kTransform)) {

		MFnTransform fransfNode(node);

		oldName = str.asChar();
		newName = fransfNode.fullPathName().asChar();
	}

	else if (node.hasFn(MFn::kCamera)) {
	
		MFnCamera camNode(node);

		oldName = str.asChar();
		newName = camNode.fullPathName().asChar();
	}

	else if (node.hasFn(MFn::kLight)) {

		MFnLight lightNode(node);

		oldName = str.asChar();
		newName = lightNode.fullPathName().asChar();
	}

	else if (node.hasFn(MFn::kLambert)) {

		MFnDependencyNode materialNode(node);

		oldName = str.asChar();
		newName = materialNode.name().asChar();
	}


	MStreamUtils::stdOutStream() << "From " << oldName << " to " << newName << endl;
	MStreamUtils::stdOutStream() <<  endl;

}


// callback that goes through nodes that have been created and attatches callbacks
void nodeAdded(MObject &node, void * clientData) {

	MCallbackId tempID; 
	MStatus Result = MS::kSuccess;

	// if the node has kMesh attributes it is a mesh
	if (node.hasFn(MFn::kMesh)) {
		
		// check if mesh is not intermediate
		// The intermediate object is the object as it was before adding the deformers
		MDagPath path;
		MFnDagNode(node).getPath(path);
		MFnMesh meshNode(node);

		if (!meshNode.isIntermediateObject()) {

			MStreamUtils::stdOutStream() << "nodeAdded: MESH was added " << endl;
			//meshQueue.push(node);

			tempID = MNodeMessage::addAttributeChangedCallback(node, meshAttributeChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);

			tempID = MPolyMessage::addPolyTopologyChangedCallback(node, meshTopologyChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);

			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, meshWorldMatrixChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);

			tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);
			
		}

	}

	// if the node has kTransform attributes it is a transform
	if (node.hasFn(MFn::kTransform)) {

		MStreamUtils::stdOutStream() << "nodeAdded: TRANSFORM was added " << endl;

		MDagPath path;
		MFnDagNode(node).getPath(path);
		MFnTransform transfNode(node);
		//transformQueue.push(node);

		
		tempID = MDagMessage::addWorldMatrixModifiedCallback(path, transformWorldMatrixChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		//callbackIds.append(MNodeMessage::addAttributeChangedCallback(transform.object(), TransformChangedCB));
		//callbackIds.append(MNodeMessage::addNameChangedCallback(transform.object(), NameChangedCB));
		
		
	}

	// if the node has kLight attributes it is a light
	if (node.hasFn(MFn::kLight)) {

		MStreamUtils::stdOutStream() << "nodeAdded: LIGHT was added " << endl;
		//lightQueue.push(node);

		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);
	}

	// if the node has kLambert attributes it is a lambert
	if (node.hasFn(MFn::kLambert)) {

		MStreamUtils::stdOutStream() << "nodeAdded: LAMBERT was added " << endl;
	
		MObject lambertShader(node);
		tempID = MNodeMessage::addAttributeChangedCallback(lambertShader, materialAttributeChanged, NULL, &status);
		if (Result == MS::kSuccess)
			callbackIdArray.append(tempID);

		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);
		

		//materialQueue.push(node);
	}

}

// callback that goes through nodes that have been deleted
void nodeDeleted(MObject &node, void *clientData) {

	//MStreamUtils::stdOutStream() << "\n";
	//MStreamUtils::stdOutStream() << "NODE DELETED";

	if (node.hasFn(MFn::kMesh)) {

		MFnMesh mesh(node);
		std::string meshName = mesh.name().asChar(); 
		MStreamUtils::stdOutStream() << "mesh deleted " << meshName << endl;

		/* 
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
		*/


	}

	if (node.hasFn(MFn::kTransform)) {

		MFnTransform transfNode(node);
		std::string transfName = transfNode.name().asChar();
		MStreamUtils::stdOutStream() << "transform deleted " << transfName << endl;


	}

	else if (node.hasFn(MFn::kLight)) {
		
		MFnLight lightNode(node);
		std::string lightName = lightNode.name().asChar();
		MStreamUtils::stdOutStream() << "light deleted " << lightName << endl;

	}

	else if (node.hasFn(MFn::kLambert)) {

		MFnDependencyNode materialNode(node);
		std::string matName = materialNode.name().asChar();
		MStreamUtils::stdOutStream() << "lambert deleted " << matName << endl;

	}

	
	callbackIdArray.remove(MNodeMessage::currentCallbackId());
}

// callback called every x seconds 
void timerCallback(float elapsedTime, float lastTime, void* clientData) {
	
	globalTime += elapsedTime;

	//MCallbackId tempID;
	//MStatus Result = MS::kSuccess;

	for (int i = 0; i < meshInfoToSend.size(); i++) {
		
		//meshInfoToSend
		//MStreamUtils::stdOutStream() << "\n";
		//MStreamUtils::stdOutStream() << "meshInfoToSend.size() " << meshInfoToSend.size() << "\n";
		//MStreamUtils::stdOutStream() << "meshInfoToSend mesh "   << meshInfoToSend[i].meshName << "\n";

		const char* msgChar = new char[meshInfoToSend[i].msgHeader.msgSize];

		// copy over msg ======
		memcpy((char*)msgChar, &meshInfoToSend[i].msgHeader, sizeof(MsgHeader));
		memcpy((char*)msgChar + sizeof(MsgHeader), &meshInfoToSend[i].meshData, sizeof(Mesh));
		memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh), meshInfoToSend[i].meshVtx, (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3));
		memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3), &meshInfoToSend[i].meshNrms, (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3));
		memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3), &meshInfoToSend[i].meshUVs, sizeof(float) * meshInfoToSend[i].meshData.UVcount);
		memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.UVcount), &meshInfoToSend[i].materialData, sizeof(Material));
		memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.UVcount) + sizeof(Material), &meshInfoToSend[i].transformMatrix, sizeof(Matrix));

		//send it
		if (comLib.send(msgChar, meshInfoToSend[i].msgHeader.msgSize)) {
				MStreamUtils::stdOutStream() << "meshInfoToSend: Message sent" << "\n";
		}
			

		// delete allocated arrays + message
		delete[] msgChar; 
		delete[] meshInfoToSend[i].meshVtx;
		delete[] meshInfoToSend[i].meshUVs;
		delete[] meshInfoToSend[i].meshNrms;

		meshInfoToSend.erase(meshInfoToSend.begin() + i);
	}
	

}



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

		tempID = MNodeMessage::addNameChangedCallback(cameraIterator.thisNode(), nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		cameraIterator.next();
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

