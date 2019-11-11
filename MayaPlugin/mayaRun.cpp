//#include "maya_includes.h"
#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <string>

#include "functions.h"
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
float timerPeriod = 0.05f; 

// Output messages in output window
//MStreamUtils::stdOutStream() << "...: " << ... << "_" << endl;

Vec3 camPosScene; 
MStringArray lightInScene; 
MStringArray meshesInScene; 
MStringArray materialsInScene; 
MStringArray transformsInScene; 


std::vector<MeshInfo> meshInfoToSend; 
std::vector<LightInfo> lightInfoToSend;
std::vector<CameraInfo> cameraInfoToSend;
std::vector<MaterialInfo> materialInfoToSend;
std::vector<TransformInfo> transformInfoToSend;
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

// callback for if a texture image has been connected to a specific material
void texturePlugAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	 
	if (msg & msg & MNodeMessage::kAttributeSet) {
		//MStreamUtils::stdOutStream() << "==================================" << endl;
		//MStreamUtils::stdOutStream() << "TEX PLUG CHANGED?" << endl;
		//MStreamUtils::stdOutStream() << endl;
	
		MFnDependencyNode fn(plug.node());
	

		MStatus result;
		MPlugArray connections;

		// get the outColor plug which is connected to lambert 
		MPlug colorPlug = fn.findPlug("outColor", &result);
		std::string fileNameString; 

		if (result) {

			colorPlug.connectedTo(connections, false, true);
			if (connections.length() > 0) {

				MPlugArray textureConnections;

				// get lambert node and color plug 
				MFnDependencyNode lambertNode(connections[0].node());
				MFnLambertShader lambertShader(connections[0].node());
				MPlug lamColorPlug = lambertShader.findPlug("color", &result);
				
				if (result) {
					
					lamColorPlug.connectedTo(textureConnections, true, false);
					if (textureConnections.length() > 0) {

						//get filepath
						MFnDependencyNode textureNode(textureConnections[0].node());
						MPlug fileTextureName = textureNode.findPlug("ftn");

						MString fileName;
						fileTextureName.getValue(fileName);
						fileNameString = fileName.asChar();

						MStreamUtils::stdOutStream() << "fileNameString " << fileNameString << endl;

						// check if material exists in scene
						bool matExists = false; 
						for (int i = 0; i < materialsInScene.length(); i++) {
							if (materialsInScene[i] == lambertNode.name().asChar()) {
								matExists = true; 
								break; 
							}
						}

						if (matExists) {
							MStreamUtils::stdOutStream() << "Material exists. Editing texture " << endl;
							
							MStreamUtils::stdOutStream() << "lambertNode " << lambertNode.name() << endl;


							MaterialInfo mMatInfo = {}; 
							size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));
							
							mMatInfo.msgHeader.msgSize = totalMsgSize; 
							mMatInfo.msgHeader.nodeType = NODE_TYPE::MATERIAL; 
							mMatInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL; 

							mMatInfo.msgHeader.nameLen = lambertNode.name().length();
							memcpy(mMatInfo.msgHeader.objName, lambertNode.name().asChar(), mMatInfo.msgHeader.nameLen);

							mMatInfo.materialData.type = 1; 
							mMatInfo.materialData.color = {255,255,255,255};
							mMatInfo.materialData.matNameLen = lambertNode.name().length();
							memcpy(mMatInfo.msgHeader.objName, lambertNode.name().asChar(), mMatInfo.msgHeader.nameLen);

							mMatInfo.materialData.textureNameLen = fileNameString.length(); 
							memcpy(mMatInfo.materialData.fileTextureName, fileNameString.c_str(), mMatInfo.materialData.textureNameLen);

							mMatInfo.materialData.matNameLen = lambertNode.name().length();
							memcpy(mMatInfo.materialData.materialName, lambertNode.name().asChar(), mMatInfo.materialData.matNameLen);


							// check if a msg with the material already exists. If it does, replace it
							bool msgExists = false; 
							for (int i = 0; i < materialInfoToSend.size(); i++) {

								if (materialInfoToSend[i].materialName == lambertNode.name().asChar()) {
									msgExists = true; 

									materialInfoToSend[i] = mMatInfo; 
									
								}

							}

							// if it doesn't exist, add it
							if (!msgExists) {
								materialInfoToSend.push_back(mMatInfo);
							}
						}
						
					}
				}
			}
		}
	}
}

// callback for materials and if the attribues have changed
void materialAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	//MStreamUtils::stdOutStream() << endl;(msg & MNodeMessage::kAttributeSet) &&
	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "MATERIAL ATTR CHANGED" << endl;
	MStreamUtils::stdOutStream() << endl;

	if ((plug.node().hasFn(MFn::kLambert))){ // && (msg != 2052)) {
		
		MStatus result;
		MaterialInfo mMatInfo = {}; 

		// lambert material 
		Color mColor = {255,255,255,255};
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

		// check if material already exists in scene, otherwise add
		int index = -1;
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == lambertShader.name()) {

				index = i; 
				MStreamUtils::stdOutStream() << "Material already exists " << endl;
				mMatInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
				break;
			}
		}

		if (index == -1) {

			MStreamUtils::stdOutStream() << "Material didn't exist. Adding " << endl;

			materialsInScene.append(lambertShader.name());
			mMatInfo.msgHeader.cmdType = CMDTYPE::NEW_MATERIAL;
		}

		if (result) {

			MPlug colorAttr;
			colorAttr = lambertShader.findPlug("colorR");
			colorAttr.getValue(color.r);
			colorAttr = lambertShader.findPlug("colorG");
			colorAttr.getValue(color.g);
			colorAttr = lambertShader.findPlug("colorB");
			colorAttr.getValue(color.b);

			// convert from 0-1 to 0-255
			int red = color.r * 255;
			int blue = color.b * 255;
			int green = color.g * 255;
			int alpha = color.a * 255;
			mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };


			// to check for textures, check if color plug is dest for another plug
			colorPlug.connectedTo(textureConnections, true, false);
			if (textureConnections.length() > 0) {

				//get filepath
				MFnDependencyNode textureNode(textureConnections[0].node());
				MPlug fileTextureName = textureNode.findPlug("ftn");

				 
				MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(fileTextureName.node(), texturePlugAttributeChanged, NULL, &status);
				if (status == MS::kSuccess)
					callbackIdArray.append(tempID);
				
				
				MString fileName;
				fileTextureName.getValue(fileName);
				fileNameString = fileName.asChar();

				MStreamUtils::stdOutStream() << "fileNameString " << fileNameString << endl;

				hasTexture = true;
			}

			if((msg & MNodeMessage::kAttributeSet)){
			
				MStreamUtils::stdOutStream() << "kAttributeSet for material " << endl;

				size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));

				mMatInfo.msgHeader.msgSize  = totalMsgSize;
				mMatInfo.msgHeader.nodeType = NODE_TYPE::MATERIAL; 
				//mMatInfo.msgHeader.cmdType  = CMDTYPE::UPDATE_MATERIAL; 

				mMatInfo.msgHeader.nameLen = lambertShader.name().length(); 
				memcpy(mMatInfo.msgHeader.objName, lambertShader.name().asChar(), mMatInfo.msgHeader.nameLen);

				mMatInfo.materialData.color = mColor;
				mMatInfo.materialData.matNameLen = lambertShader.name().length();
				memcpy(mMatInfo.materialData.materialName, lambertShader.name().asChar(), mMatInfo.materialData.matNameLen);

				mMatInfo.materialName = lambertShader.name(); 

				if (hasTexture) {
					mMatInfo.materialData.type = 1;
					mMatInfo.texturePath = fileNameString.c_str(); 
					mMatInfo.materialData.textureNameLen = fileNameString.length();
					memcpy(mMatInfo.materialData.fileTextureName, fileNameString.c_str(), mMatInfo.materialData.textureNameLen);

				}

				else {
					mMatInfo.materialData.type = 0;
					mMatInfo.texturePath = "NA";
					mMatInfo.materialData.textureNameLen = 2;
					memcpy(mMatInfo.materialData.fileTextureName, "NA", mMatInfo.materialData.textureNameLen);

				}

				bool msgExists = false;
				for (int i = 0; i < materialInfoToSend.size(); i++) {
					if (materialInfoToSend[i].materialName == lambertShader.name()) {
						msgExists = true;

						materialInfoToSend[i] = mMatInfo; 
					}
				}

				if (!msgExists) {
					materialInfoToSend.push_back(mMatInfo);
				}
			}
		}

	}



}
// ==================================================================================
// =============================== TRANSFORM ========================================
// ==================================================================================
 
// callback for transforms in scene. Only sends info on the ones that have a shape as geometry
void transformWorldMatrixChanged(MObject &transformNode, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	MStatus result;
	MDagPath path;
	MFnDagNode(transformNode).getPath(path);
	MFnTransform transfNode(path, &result);

	Matrix mTransformMatrix = {};
	TransformInfo mTransfInfo = {};
	bool msgToSend = false; 

	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << " =============================== " << endl;
	MStreamUtils::stdOutStream() << "TRANSFORM MAT CHANGED " << endl;

	if (result && !transformNode.hasFn(MFn::kCamera)) {
		
		MMatrix transfMatAbs; 

		// check if transform node is directry connected to a mesh
		MFnMesh meshNode(transfNode.child(0), &result);
		if (result) {

			MStreamUtils::stdOutStream() << "Has a child node mesh" << endl;
			transfMatAbs = transfNode.transformationMatrix();

			mTransformMatrix.a11 = transfMatAbs(0, 0);
			mTransformMatrix.a12 = transfMatAbs(1, 0);
			mTransformMatrix.a13 = transfMatAbs(2, 0);
			mTransformMatrix.a14 = transfMatAbs(3, 0);

			//row 2
			mTransformMatrix.a21 = transfMatAbs(0, 1);
			mTransformMatrix.a22 = transfMatAbs(1, 1);
			mTransformMatrix.a23 = transfMatAbs(2, 1);
			mTransformMatrix.a24 = transfMatAbs(3, 1);

			//row 3
			mTransformMatrix.a31 = transfMatAbs(0, 2);
			mTransformMatrix.a32 = transfMatAbs(1, 2);
			mTransformMatrix.a33 = transfMatAbs(2, 2);
			mTransformMatrix.a34 = transfMatAbs(3, 2);

			//row 4
			mTransformMatrix.a41 = transfMatAbs(0, 3);
			mTransformMatrix.a42 = transfMatAbs(1, 3);
			mTransformMatrix.a43 = transfMatAbs(2, 3);
			mTransformMatrix.a44 = transfMatAbs(3, 3);

			//MStreamUtils::stdOutStream() << "TRANSF CHILD " << meshNode.name() << endl;
			size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Transform) + sizeof(Matrix));

			mTransfInfo.msgHeader.msgSize = totalMsgSize;
			mTransfInfo.msgHeader.nodeType = NODE_TYPE::MESH;
			mTransfInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATRIX;
			mTransfInfo.msgHeader.nameLen = transfNode.name().length();
			memcpy(mTransfInfo.msgHeader.objName, transfNode.name().asChar(), mTransfInfo.msgHeader.nameLen);

			mTransfInfo.transfData.transfNameLen = transfNode.name().length();
			memcpy(mTransfInfo.transfData.transfName, transfNode.name().asChar(), mTransfInfo.transfData.transfNameLen);

			mTransfInfo.transfData.childNameLen = meshNode.name().length();
			memcpy(mTransfInfo.transfData.childName, meshNode.name().asChar(), mTransfInfo.transfData.childNameLen);

			mTransfInfo.transfName = transfNode.fullPathName();
			mTransfInfo.childShapeName = meshNode.name();

			mTransfInfo.transformMatrix = mTransformMatrix;
			msgToSend = true; 
		}


		// if not, check hierarchy
		else {
			MStreamUtils::stdOutStream() << "Has no child node mesh" << endl;

			MObjectArray hierarchy;
			//int totalChildCnt = 0; 

			bool hasShapeNode = false;
			int childCnt = transfNode.childCount();

			MObject firstParentNode(transfNode.object());
			hierarchy.append(firstParentNode);

			// get hierarchy to mesh
			for (int j = 0; j < hierarchy.length(); j++) {

				MFnDagNode prnt(hierarchy[j]);
				int prntChildCnt = prnt.childCount();

				for (int i = 0; i < prntChildCnt; i++) {

					MObject childObject(prnt.child(i));
					if (childObject.apiType() == MFn::kTransform) {

						MStreamUtils::stdOutStream() << "Child is transform " << endl;
						MFnDagNode child(childObject);
						int tempChildCnt = child.childCount();
						std::string childName = child.fullPathName().asChar();

						if (tempChildCnt != 0) {

							hierarchy.append(childObject);

						}

					}

					else if (childObject.apiType() == MFn::kMesh) {
						MStreamUtils::stdOutStream() << "Child is mesh " << endl;
						meshNode.setObject(childObject);
						break;
					}
				}
			}

			// get tot matrix by multiplying
			for (int i = hierarchy.length() - 1; i-- > 0; ) {
				MFnDagNode child(hierarchy[i]);
				transfMatAbs *= child.transformationMatrix();

			}


			//row 1
			mTransformMatrix.a11 = transfMatAbs(0, 0);
			mTransformMatrix.a12 = transfMatAbs(1, 0);
			mTransformMatrix.a13 = transfMatAbs(2, 0);
			mTransformMatrix.a14 = transfMatAbs(3, 0);

			//row 2
			mTransformMatrix.a21 = transfMatAbs(0, 1);
			mTransformMatrix.a22 = transfMatAbs(1, 1);
			mTransformMatrix.a23 = transfMatAbs(2, 1);
			mTransformMatrix.a24 = transfMatAbs(3, 1);

			//row 3
			mTransformMatrix.a31 = transfMatAbs(0, 2);
			mTransformMatrix.a32 = transfMatAbs(1, 2);
			mTransformMatrix.a33 = transfMatAbs(2, 2);
			mTransformMatrix.a34 = transfMatAbs(3, 2);

			//row 4
			mTransformMatrix.a41 = transfMatAbs(0, 3);
			mTransformMatrix.a42 = transfMatAbs(1, 3);
			mTransformMatrix.a43 = transfMatAbs(2, 3);
			mTransformMatrix.a44 = transfMatAbs(3, 3);


			//MStreamUtils::stdOutStream() << "TRANSF CHILD " << meshNode.name() << endl;
			size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Transform) + sizeof(Matrix));

			mTransfInfo.msgHeader.msgSize = totalMsgSize;
			mTransfInfo.msgHeader.nodeType = NODE_TYPE::MESH;
			mTransfInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATRIX;
			mTransfInfo.msgHeader.nameLen = transfNode.name().length();
			memcpy(mTransfInfo.msgHeader.objName, transfNode.name().asChar(), mTransfInfo.msgHeader.nameLen);

			mTransfInfo.transfData.transfNameLen = transfNode.name().length();
			memcpy(mTransfInfo.transfData.transfName, transfNode.name().asChar(), mTransfInfo.transfData.transfNameLen);

			mTransfInfo.transfData.childNameLen = meshNode.name().length();
			memcpy(mTransfInfo.transfData.childName, meshNode.name().asChar(), mTransfInfo.transfData.childNameLen);

			mTransfInfo.transfName = transfNode.fullPathName();
			mTransfInfo.childShapeName = meshNode.name();

			mTransfInfo.transformMatrix = mTransformMatrix;
			msgToSend = true;
		}

		if (msgToSend) {

		// check if msg already exists. If so, update
		bool msgExists = false;

		for (int i = 0; i < meshInfoToSend.size(); i++) {
			if (transformInfoToSend[i].childShapeName == meshNode.name()) {

				msgExists = true;
				//MStreamUtils::stdOutStream() << "Transf Msg already exists! Updating " << endl;
				transformInfoToSend[i] = mTransfInfo;
			}
		}


		if (!msgExists) {

			//MStreamUtils::stdOutStream() << "Transf Msg doesn't exists! Adding .... " << endl;
			transformInfoToSend.push_back(mTransfInfo);
		}	

	}
	}
	
}

// ==================================================================================
// ================================= MESH ===========================================
// ==================================================================================
int findMesh(MStringArray meshesInScene, std::string MeshName) {

	int objIndex = -1;
	for (int i = 0; i < meshesInScene.length(); i++) {
		if (meshesInScene[i] == MeshName.c_str()) {

			objIndex = i;
			break;
		}
	}

	return objIndex;
}

// function that gets mesh information (vtx, normal, UV), material and matrix information for new meshes
void GetMeshInfo(MFnMesh &mesh) {

	MStreamUtils::stdOutStream() << endl;

	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "GET MESH INFO" << endl;
	MStreamUtils::stdOutStream() << endl;

	MStatus result;

	Color mColor = { 255, 255, 255, 255 };
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

			//MStreamUtils::stdOutStream() << endl;

			// get the tris count for the face that it's currently iterating through
			polyIterator.numTriangles(trisCurrentFace);

			// get vtx and vtx positions of all the triangles in the current face
			polyIterator.getTriangles(points, vertexList);

			// get points, normals and UVs 
			for (int i = 0; i < points.length(); i++) {

				int index = polyIterator.index();
				vtxArray.append(points[i]);

				float2 tempUV;
				polyIterator.getUVAtPoint(points[i], tempUV);
				uvArray.append(tempUV[0]);
				uvArray.append(1.0 - tempUV[1]);

				MVector tempNormal;
				//polyIterator.getNormal(index, tempNormal, MSpace::kWorld);
				polyIterator.getNormal(tempNormal);
				normalArray.append(tempNormal);

			}


			//MStreamUtils::stdOutStream() << "trisCurrentFace: " << trisCurrentFace << endl;			
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
	MColor color = { 255, 255, 255, 255 };
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

	if (connectedShaders.length() > 0) {

		// if there are any connected shaders, get first one
		MFnDependencyNode initialShadingGroup(connectedShaders[0]);
		//MStreamUtils::stdOutStream() << "Shader: " << initialShadingGroup.name() << endl;

		MPlug surfaceShader = initialShadingGroup.findPlug("surfaceShader", &result);
		if (result) {

			// to find material, find the destination connections for surficeShader plug
			// (AKA plugs that have surface shader as destination)
			surfaceShader.connectedTo(shaderConnections, true, false);

			if (shaderConnections.length() > 0) {

				//Only support 1 material per mesh + lambert. Check if first connection is lambert
				if (shaderConnections[0].node().apiType() == MFn::kLambert) {

					lambertShader.setObject(shaderConnections[0].node());
					materialNamePlug = lambertShader.name().asChar();
					
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
						int red = color.r * 255;
						int blue = color.b * 255;
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
	int index = findMesh(meshesInScene, mesh.name().asChar());

	if(index == -1) {
		MStreamUtils::stdOutStream() << "Mesh didn't exist. Adding " << endl;
		meshesInScene.append(mesh.name());
		mMeshInfo.msgHeader.cmdType = NEW_NODE;
		index = meshesInScene.length() - 1;

	}

	else if (index >= 0) {
		MStreamUtils::stdOutStream() << "Mesh already exists " << endl;
		mMeshInfo.msgHeader.cmdType = CMDTYPE::UPDATE_NODE;

	}




		// check if mesh already exists in scene, otherwise add
		int matIndex = -1;
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == lambertShader.name()) {

				matIndex = i;
				//MStreamUtils::stdOutStream() << "Material already exists " << endl;
				break;
			}
		}

		if (matIndex == -1) {

			//MStreamUtils::stdOutStream() << "Material didn't exist. Adding " << endl;
			materialsInScene.append(lambertShader.name());
		}

		// float arrays that store mesh info  ================
		float* meshUVs = new float[uvArray.length()];
		float* meshVtx = new float[vtxArray.length() * 3];
		float* meshNormals = new float[normalArray.length() * 3];

		// fill arrays with info
		int vtxPos = 0;
		for (int i = 0; i < vtxArray.length(); i++) {

			// vtx
			meshVtx[vtxPos] = vtxArray[i][0];
			meshVtx[vtxPos + 1] = vtxArray[i][1];
			meshVtx[vtxPos + 2] = vtxArray[i][2];

			// normals
			meshNormals[vtxPos] = normalArray[i][0];
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

		mMeshInfo.meshName = mesh.name();
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
		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * mMeshInfo.meshData.vtxCount * 3) + (sizeof(float) * mMeshInfo.meshData.normalCount * 3) + (sizeof(float) * mMeshInfo.meshData.UVcount) + sizeof(Material) + sizeof(Matrix));

		mMeshInfo.msgHeader.msgSize = totalMsgSize;
		mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH;
		mMeshInfo.msgHeader.nameLen = mesh.name().length();
		memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);

		// check if msg already exists. If so, update
		bool msgExists = false;
		for (int i = 0; i < meshInfoToSend.size(); i++) {
			if (meshInfoToSend[i].meshName == mesh.name()) {
				msgExists = true;

				//MStreamUtils::stdOutStream() << "Msg already exists! Updating " << endl;


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

				memcpy((char*)meshInfoToSend[i].meshVtx, meshVtx, (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
				memcpy((char*)meshInfoToSend[i].meshNrms, meshNormals, (sizeof(float) * mMeshInfo.meshData.normalCount * 3));
				memcpy((char*)meshInfoToSend[i].meshUVs, meshUVs, (sizeof(float) * (mMeshInfo.meshData.UVcount)));

				meshInfoToSend[i].materialData = mMeshInfo.materialData;
				meshInfoToSend[i].transformMatrix = mMeshInfo.transformMatrix;


				break;
			}
		}

		// if it doesn't exist, add it to the queue 
		if (!msgExists) {

			//MStreamUtils::stdOutStream() << "Msg didn't exist! Adding..." << endl;

			mMeshInfo.meshVtx = new float[mMeshInfo.meshData.vtxCount * 3];
			mMeshInfo.meshNrms = new float[mMeshInfo.meshData.normalCount * 3];
			mMeshInfo.meshUVs = new float[mMeshInfo.meshData.UVcount];

			memcpy((char*)mMeshInfo.meshVtx, meshVtx, (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
			memcpy((char*)mMeshInfo.meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));
			memcpy((char*)mMeshInfo.meshUVs, meshUVs, (sizeof(float) * (mMeshInfo.meshData.UVcount)));

			meshInfoToSend.push_back(mMeshInfo);
		}

		delete[] meshVtx;
		delete[] meshUVs;
		delete[] meshNormals;

	
}

// function that gets mesh information (vtx, normal, UV) for existing meshes
void GeometryUpdate(MFnMesh &mesh) {

	
	MStreamUtils::stdOutStream() << endl;

	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "GEOMETRY UPDATE" << endl;
	MStreamUtils::stdOutStream() << endl;
	
	MStatus result;

	Color mColor = { 255, 255, 255, 255 };
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

			//MStreamUtils::stdOutStream() << endl;

			// get the tris count for the face that it's currently iterating through
			polyIterator.numTriangles(trisCurrentFace);

			// get vtx and vtx positions of all the triangles in the current face
			polyIterator.getTriangles(points, vertexList);

			// get points, normals and UVs 
			for (int i = 0; i < points.length(); i++) {

				int index = polyIterator.index();
				vtxArray.append(points[i]);

				float2 tempUV;
				polyIterator.getUVAtPoint(points[i], tempUV);
				uvArray.append(tempUV[0]);
				uvArray.append(1.0 - tempUV[1]);

				MVector tempNormal;
				//polyIterator.getNormal(index, tempNormal, MSpace::kWorld);
				polyIterator.getNormal(tempNormal);
				normalArray.append(tempNormal);

			}


			totalTrisCnt += trisCurrentFace;
			polyIterator.next();
		}
	}


	// check if mesh already exists in scene
	int index = findMesh(meshesInScene, mesh.name().asChar());
	
	/* 
	int index = -1;
	for (int i = 0; i < meshesInScene.length(); i++) {
		if (meshesInScene[i] == mesh.name()) {
			MStreamUtils::stdOutStream() << "mesh exists" << endl;
			index = i;
			
			break;
		}
	}
	*/

	/* 
	// if it doesn't exist, it's a new mesh and should be handled by that function
	if (index == -1) {
		meshesInScene.append(mesh.name());
		index = meshesInScene.length() - 1; 
		//GetMeshInfo(mesh);
	}
	*/

	if (index >= 0) {
		

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
		mMeshInfo.meshData.meshID	   = index;
		mMeshInfo.meshData.trisCount   = totalTrisCnt;
		mMeshInfo.meshData.UVcount	   = uvArray.length();
		mMeshInfo.meshData.vtxCount	   = vtxArray.length();
		mMeshInfo.meshData.normalCount = normalArray.length();
		
		mMeshInfo.meshName     = mesh.name();
		mMeshInfo.meshPathName = mesh.fullPathName();


		// Fill header ========
		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * mMeshInfo.meshData.vtxCount * 3) + (sizeof(float) * mMeshInfo.meshData.normalCount * 3) + (sizeof(float) * mMeshInfo.meshData.UVcount));
		
		mMeshInfo.msgHeader.msgSize = totalMsgSize;
		mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH;
		mMeshInfo.msgHeader.cmdType = CMDTYPE::UPDATE_NODE; 
		mMeshInfo.msgHeader.nameLen = mesh.name().length();
		memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);

		// check if msg already exists. If so, update
		bool msgExists = false;
		for (int i = 0; i < meshInfoToSend.size(); i++) {
			if (meshInfoToSend[i].meshName == mesh.name()) {
				msgExists = true;

				//MStreamUtils::stdOutStream() << "Msg exists" << endl;

				if (meshInfoToSend[i].meshData.vtxCount != mMeshInfo.meshData.vtxCount) {

					delete[] meshInfoToSend[i].meshVtx; 
					delete[] meshInfoToSend[i].meshNrms;

					meshInfoToSend[i].meshVtx  = new float[mMeshInfo.meshData.vtxCount * 3];
					meshInfoToSend[i].meshNrms = new float[mMeshInfo.meshData.normalCount * 3];

				}

				if (meshInfoToSend[i].meshData.UVcount != mMeshInfo.meshData.UVcount) {
					delete[] meshInfoToSend[i].meshUVs;
					meshInfoToSend[i].meshUVs = new float[mMeshInfo.meshData.UVcount];
				}

				meshInfoToSend[i].msgHeader = mMeshInfo.msgHeader;
				meshInfoToSend[i].msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
				meshInfoToSend[i].meshData = mMeshInfo.meshData; 

				memcpy((char*)meshInfoToSend[i].meshVtx,  meshVtx,	   (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));	
				memcpy((char*)meshInfoToSend[i].meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));
				memcpy((char*)meshInfoToSend[i].meshUVs,  meshUVs,	   (sizeof(float) * (mMeshInfo.meshData.UVcount)));

				break; 
			}
		}


		// if it doesn't exist, add it to the queue 
		if (!msgExists) {
			//MStreamUtils::stdOutStream() << "Msg doesnt' exists" << endl;

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
}

// function that gets material info for a mesh if a new material is connected
void MaterialChanged(MFnMesh &mesh) {

	// check if mesh exists in scene
	int index = findMesh(meshesInScene, mesh.name().asChar());
	/* 
	for (int i = 0; i < meshesInScene.length(); i++) {
		if (meshesInScene[i] == mesh.name()) {
			index = i;
			break;
		}
	}
	*/

	if (index != -1) {

		MStreamUtils::stdOutStream() << endl;

		MStreamUtils::stdOutStream() << "==================================" << endl;
		MStreamUtils::stdOutStream() << "MATERIAL CHANGED" << endl;
		MStreamUtils::stdOutStream() << endl;

		MStatus result;
		MeshInfo mMeshInfo = {}; 
		Color mColor = { 255,255,255,255 }; 

		// lambert material 
		MColor color;
		MPlug colorPlug;
		std::string materialNamePlug; 
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

		// check if material already exists in scene, otherwise add
		int matIndex = -1;
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == lambertShader.name()) {

				matIndex = i;
				//MStreamUtils::stdOutStream() << "Material already exists " << endl;
				break;
			}
		}

		if (matIndex == -1) {

			//MStreamUtils::stdOutStream() << "Material didn't exist. Adding " << endl;
			materialsInScene.append(lambertShader.name());
		}

		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + sizeof(Material));

		mMeshInfo.msgHeader.msgSize = totalMsgSize; 
		mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH; 
		mMeshInfo.msgHeader.cmdType  = CMDTYPE::UPDATE_NODE_MATERIAL; 

		mMeshInfo.msgHeader.nameLen = mesh.name().length(); 
		memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);

		mMeshInfo.meshData.meshID	   = index;
		mMeshInfo.meshData.trisCount   = 0;
		mMeshInfo.meshData.UVcount	   = 0;
		mMeshInfo.meshData.vtxCount	   = 0;
		mMeshInfo.meshData.normalCount = 0;
		mMeshInfo.meshData.matNameLen = materialNamePlug.length();
		memcpy(mMeshInfo.meshData.materialName, materialNamePlug.c_str(), mMeshInfo.meshData.matNameLen);

		mMeshInfo.meshName	   = mesh.name();
		mMeshInfo.meshPathName = mesh.fullPathName();

		mMeshInfo.materialData.color	  = mColor;
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


		// check if msg already exists. If so, update
		bool msgExists = false;
		for (int i = 0; i < meshInfoToSend.size(); i++) {
			if (meshInfoToSend[i].meshName == mesh.name()) {
				msgExists = true;

				meshInfoToSend[i].materialData = mMeshInfo.materialData;
				meshInfoToSend[i].meshData.matNameLen = mMeshInfo.meshData.matNameLen; 
				memcpy(meshInfoToSend[i].meshData.materialName, mMeshInfo.meshData.materialName, mMeshInfo.materialData.matNameLen);

				break; 
			}
		}
		
		// if it doesn't exist, add it to the queue 
		if (!msgExists) {
			meshInfoToSend.push_back(mMeshInfo);
		}

	}


}

// ============================== CALLBACKS ========================================

// callback for when an attribute is changed on a node (vtx, normal, UV etc)
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

		MStreamUtils::stdOutStream() << "Double sided Mesh name: " << meshNode.name().asChar() << endl;
		MStreamUtils::stdOutStream() << "result: " << result << endl;
		MStreamUtils::stdOutStream() << "\n";

		// if result gives no error, get mesh information through the node
		if (result) {
			MStreamUtils::stdOutStream() << "Double sided: In result statement" << endl;
			GetMeshInfo(meshNode); 
		}
		
	

	}

	/* 
	// finished extruding mesh
	else if (otherPlugName.find("polyExtrude") != std::string::npos && otherPlugName.find("manipMatrix") != std::string::npos) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "Mesh name: " << meshNode.name().asChar() << endl;
		// if result gives no error, get mesh information through the node
		if (result) {
			MStreamUtils::stdOutStream() << "polyExtrude In result statement" << endl;
			GeometryUpdate(meshNode);
		}

	}
	*/

	// if vtx is changed
	else if (plugPartName.find("pt[") != std::string::npos) {

		MFnMesh meshNode(path, &result);

		//MStreamUtils::stdOutStream() << "Vertex changed" << endl;
		//MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;

		if (result) {
			//MStreamUtils::stdOutStream() << "Vertex In result statement" << endl;
			GeometryUpdate(meshNode);
		}
	}
	
	// if normal has changed
	else if (otherPlugName.find("polyNormal") != std::string::npos || (otherPlugName.find("polySoftEdge") != std::string::npos && otherPlugName.find("manipMatrix") != std::string::npos)) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << "normal changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;

		if (result) {
			MStreamUtils::stdOutStream() << "normal In result statement" << endl;
			//GeometryUpdate(meshNode);
		}
	}

	// if pivot changed, UV probably changed
	else if (plugPartName.find("pv") != std::string::npos) {

		MFnMesh meshNode(path, &result);

		MStreamUtils::stdOutStream() << " UV changed" << endl;
		MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;
		if (result) {
			MStreamUtils::stdOutStream() << "UV In result statement" << endl;
			//GeometryUpdate(meshNode);
		}
	}
	
	// if material connection changed (i.e another material was assigned to the mesh)
	else if (plugPartName.find("iog") != std::string::npos && otherPlug.node().apiType() == MFn::Type::kShadingEngine) {

		MFnMesh meshNode(path, &result);

		//MStreamUtils::stdOutStream() << "material changed" << endl;
		//MStreamUtils::stdOutStream() << "mesh: " << meshNode.name().asChar() << endl;
		if (result) {
			//MStreamUtils::stdOutStream() << "material In result statement" << endl;
			MaterialChanged(meshNode);
		}
	}
	
	
	else if ((plugName.find("outMesh") != std::string::npos) && (plugName.find("polySplit") == std::string::npos) && (otherPlugName.find("polyExtrude") == std::string::npos)){
	
		MFnMesh meshNode(path, &result);
		if (result) {

			int index = findMesh(meshesInScene, meshNode.name().asChar());
			if (index >= 0) {

				MStreamUtils::stdOutStream() << "outmesh In result statement" << endl;
				//GeometryUpdate(meshNode);
			}
		}
	}
	

		
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

// for whenever topology is changed (poly split, extrude, subdivision change)
void meshTopologyChanged(MObject& node, void* clientData) {

	if (node.hasFn(MFn::kMesh)) {
		MStatus stat;
		MFnMesh meshNode(node, &stat);

		if (stat) {
			MStreamUtils::stdOutStream() << " =============================== " << endl;
			MStreamUtils::stdOutStream() << "TOPOLOGY CHANGED on " << meshNode.name() << endl;

			//MStreamUtils::stdOutStream() << "plug " << plug.name() << endl;
			//MStreamUtils::stdOutStream() << "other plug " << otherPlug.name() << endl;
			//GeometryUpdate(meshNode); 


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
		//MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());		
		//MMatrix localMatrix = MMatrix(path.exclusiveMatrix());	

	}

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
	MFnCamera cameraNode(camDag); 

	// get height/width
	height = activeView.portHeight();
	width  = activeView.portWidth();

	//get relevant vectors for raylib
	MFnCamera cameraView(camDag);
	MPoint camPos	 = cameraView.eyePoint(MSpace::kWorld, &status);
	MVector upDir	 = cameraView.upDirection(MSpace::kWorld, &status);
	MVector rightDir = cameraView.rightDirection(MSpace::kWorld, &status);
	MPoint COI = cameraView.centerOfInterestPoint(MSpace::kWorld, &status);

	//float vertFOV = cameraNode.verticalFieldOfView();


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
	cameraInfo.pos = { (float)camPos.x, (float)camPos.y, (float)camPos.z };
	if ((camPosScene.x != cameraInfo.pos.x) && (camPosScene.y != cameraInfo.pos.y) && (camPosScene.z != cameraInfo.pos.z)) {

		cameraInfo.up = { (float)upDir.x, (float)upDir.y, (float)upDir.z };
		cameraInfo.forward = { (float)COI.x, (float)COI.y, (float)COI.z };
		cameraInfo.type = isOrtographic;
		cameraInfo.fov = FOV;

		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Camera));

		// Fill header ========
		MsgHeader msgHeader;
		msgHeader.msgSize = totalMsgSize;
		msgHeader.nameLen = objName.length();
		msgHeader.nodeType = NODE_TYPE::CAMERA;
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, objName.c_str(), objName.length());


		CameraInfo mCamInfo = {};
		mCamInfo.cameraName = cameraNode.name();
		mCamInfo.msgHeader = msgHeader;
		mCamInfo.camData = cameraInfo;

		camPosScene = cameraInfo.pos; 

		/* 
		// check if msg already exists. If so, update
		bool msgExists = false;
		for (int i = 0; i < cameraInfoToSend.size(); i++) {
			if (cameraInfoToSend[i].cameraName == cameraNode.name()) {
				msgExists = true;

				cameraInfoToSend[i] = mCamInfo;
			}
		}

		if (!msgExists) {
			cameraInfoToSend.push_back(mCamInfo);
		}

		*/
		//cameraInfoToSend.push_back(mCamInfo); 

		
		//pass to send
		bool msgToSend = false;
		if (objName.length() > 0) {
			msgToSend = true;
		}

		if (msgToSend) {

			const char* msg = new char[totalMsgSize];

			// copy over msg ======
			memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
			memcpy((char*)msg + sizeof(MsgHeader), &cameraInfo, sizeof(Camera));

			//send it
			if (comLib.send(msg, totalMsgSize)) {
				//MStreamUtils::stdOutStream() << "activeCamera: Message sent" << "\n";
			}

			delete[]msg;

		}
		
		
	}
}

// ==================================================================================
// ================================= LIGHT ==========================================
// ==================================================================================

void GetLightInfo(MFnLight &light) {

	MColor color;
	MStatus status;
	MVector position;
	float intensity = 0;
	LightInfo mLightInfo = {}; 
	
	MFnTransform parent(light.child(0), &status);

	color = light.color(); 
	intensity = light.intensity();

	if (status) {
		position = parent.getTranslation(MSpace::kObject, &status);
	}

	// convert from 0-1 to 0-255
	int red   = color.r * 255;
	int blue  = color.b * 255;
	int green = color.g * 255;
	int alpha = color.a * 255;
	Color mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };

	int lightIndex = -1; 
	for (int i = 0; i < lightInScene.length(); i++) {
		if (lightInScene[i] == light.name()) {
			lightIndex = i; 
			break; 
		}
	}

	if (lightIndex == -1)
		lightInScene.append(light.name());


	size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Light));
	
	mLightInfo.msgHeader.msgSize = totalMsgSize; 
	mLightInfo.msgHeader.cmdType = CMDTYPE::NEW_NODE; 
	mLightInfo.msgHeader.nodeType = NODE_TYPE::LIGHT; 
	mLightInfo.msgHeader.nameLen = light.name().length(); 
	memcpy(mLightInfo.msgHeader.objName, light.name().asChar(), mLightInfo.msgHeader.nameLen);

	mLightInfo.lightData.lightNameLen = light.name().length();
	memcpy(mLightInfo.lightData.lightName, light.name().asChar(), mLightInfo.lightData.lightNameLen);

	mLightInfo.lightData.color = mColor; 
	mLightInfo.lightData.intensity = intensity; 
	mLightInfo.lightData.lightPos = { (float)position.x, (float)position.y, (float)position.z};

	mLightInfo.lightName = light.name(); 
	mLightInfo.lightPathName = light.fullPathName(); 

	lightInfoToSend.push_back(mLightInfo);
}

void lightAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	if (msg & MNodeMessage::kAttributeSet) {

		std::string plugName = plug.name().asChar();
		std::string plugPartName = plug.partialName().asChar();
		std::string otherPlugName = otherPlug.name().asChar();

		MColor color;
		MStatus result;
		MVector position;
		float intensity = 0;
		LightInfo mLightInfo = {};

		MDagPath path;
		MFnDagNode(plug.node()).getPath(path);

		MFnLight lightNode; 
		MFnTransform lightTransf;

		if (plugName.find("color") != std::string::npos) {

			lightNode.setObject(path); 
			lightTransf.setObject(lightNode.parent(0));


			
		}

		else if (plugName.find("translate") != std::string::npos || plugName.find("rotate") != std::string::npos || plugName.find("scale") != std::string::npos) {

			lightTransf.setObject(path); 
			lightNode.setObject(lightTransf.child(0));


		}

		int lightIndex = -1;
		for (int i = 0; i < lightInScene.length(); i++) {
			if (lightInScene[i] == lightNode.name()) {
				lightIndex = i;
				break;
			}
		}

		if (lightIndex == -1)
			lightInScene.append(lightNode.name());


		color	  = lightNode.color(); 
		intensity = lightNode.intensity(); 
		position  = lightTransf.getTranslation(MSpace::kObject, &status);


		// convert from 0-1 to 0-255
		int red = color.r * 255;
		int blue = color.b * 255;
		int green = color.g * 255;
		int alpha = color.a * 255;
		Color mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };


		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Light));

		mLightInfo.msgHeader.msgSize = totalMsgSize;
		mLightInfo.msgHeader.cmdType = CMDTYPE::NEW_NODE;
		mLightInfo.msgHeader.nodeType = NODE_TYPE::LIGHT;
		mLightInfo.msgHeader.nameLen = lightNode.name().length();
		memcpy(mLightInfo.msgHeader.objName, lightNode.name().asChar(), mLightInfo.msgHeader.nameLen);

		mLightInfo.lightData.lightNameLen = lightNode.name().length();
		memcpy(mLightInfo.lightData.lightName, lightNode.name().asChar(), mLightInfo.lightData.lightNameLen);

		mLightInfo.lightData.color = mColor;
		mLightInfo.lightData.intensity = intensity;
		mLightInfo.lightData.lightPos = { (float)position.x, (float)position.y, (float)position.z };

		mLightInfo.lightName = lightNode.name();
		mLightInfo.lightPathName = lightNode.fullPathName();

		lightInfoToSend.push_back(mLightInfo);

	}
}



// ==================================================================================
// ================================= OTHER ==========================================
// ==================================================================================

void nodeNameChanged(MObject& node, const MString& str, void* clientData) {

	//MStreamUtils::stdOutStream() << "=========================" << endl;
	//MStreamUtils::stdOutStream() << "NAME CHANGED " << endl;
	std::string oldName;
	std::string newName;
	
	bool nodeToRename = false; 
	NodeRenamedInfo mNodeRenamed = {}; 


	if (node.hasFn(MFn::kMesh)) {

		MFnMesh meshNode(node);

		oldName = str.asChar();
		newName = meshNode.fullPathName().asChar();

		int meshIndex = -1; 
		for (int i = 0; i < meshesInScene.length(); i++) {
			if (meshesInScene[i] == oldName.c_str()) {
				meshIndex = i;
			}
		}

		if (meshIndex >= 0) {
			meshesInScene[meshIndex] = newName.c_str();
			mNodeRenamed.msgHeader.nodeType = NODE_TYPE::MESH; 
			nodeToRename = true; 
		}
		
	}

	else if (node.hasFn(MFn::kTransform)) {

		MFnTransform fransfNode(node);

		oldName = str.asChar();
		newName = fransfNode.fullPathName().asChar();

		int transfIndex = -1;
		for (int i = 0; i < transformsInScene.length(); i++) {
			if (transformsInScene[i] == oldName.c_str()) {
				transfIndex = i;
			}
		}

		if (transfIndex >= 0) {
			transformsInScene[transfIndex] = newName.c_str();
			mNodeRenamed.msgHeader.nodeType = NODE_TYPE::TRANSFORM;
			nodeToRename = true;
		}
	}

	else if (node.hasFn(MFn::kCamera)) {
	
		MFnCamera camNode(node);

		oldName = str.asChar();
		newName = camNode.fullPathName().asChar();

		mNodeRenamed.msgHeader.nodeType = NODE_TYPE::CAMERA;
		nodeToRename = true;

	}

	else if (node.hasFn(MFn::kLight)) {

		MFnLight lightNode(node);

		oldName = str.asChar();
		newName = lightNode.fullPathName().asChar();

		int lightIndex = -1; 
		for (int i = 0; i < lightInScene.length(); i++) {
			if (lightInScene[i] == oldName.c_str()) {
				lightIndex = i; 
			}
		}

		if (lightIndex >= 0) {
			lightInScene[lightIndex] = newName.c_str();
			mNodeRenamed.msgHeader.nodeType = NODE_TYPE::LIGHT;
			nodeToRename = true;
		}

	}

	else if (node.hasFn(MFn::kLambert)) {

		MFnDependencyNode materialNode(node);

		oldName = str.asChar();
		newName = materialNode.name().asChar();

		int materialIndex = -1; 
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == oldName.c_str()) {
				materialIndex = i; 
			}
		}

		if (materialIndex >= 0) {
			materialsInScene[materialIndex] = newName.c_str();
			mNodeRenamed.msgHeader.nodeType = NODE_TYPE::MATERIAL;
			nodeToRename = true;

		}
	}

	if (nodeToRename) {

		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(NodeRenamed));

		mNodeRenamed.nodeOldName = oldName.c_str();
		mNodeRenamed.nodeNewName = newName.c_str(); 

		mNodeRenamed.msgHeader.msgSize = totalMsgSize; 
		mNodeRenamed.msgHeader.nameLen = oldName.length(); 
		mNodeRenamed.msgHeader.cmdType = CMDTYPE::UPDATE_NAME; 
		memcpy(mNodeRenamed.msgHeader.objName, oldName.c_str(), mNodeRenamed.msgHeader.nameLen);

		mNodeRenamed.renamedInfo.nodeNameLen = newName.length(); 
		memcpy(mNodeRenamed.renamedInfo.nodeName, newName.c_str(), mNodeRenamed.renamedInfo.nodeNameLen);
	
		mNodeRenamed.renamedInfo.nodeOldNameLen = oldName.length();
		memcpy(mNodeRenamed.renamedInfo.nodeOldName, oldName.c_str(), mNodeRenamed.renamedInfo.nodeOldNameLen);
		
		nodeRenamedInfoToSend.push_back(mNodeRenamed); 
	}

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

			//MStreamUtils::stdOutStream() << "nodeAdded: MESH was added " << endl;
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

		//MStreamUtils::stdOutStream() << "nodeAdded: LIGHT was added " << endl;
		//lightQueue.push(node);
		MDagPath path;
		MFnDagNode(node).getPath(path);
		MFnLight lightNode(path);

		MObject parentNode(lightNode.parent(0));

		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		tempID = MNodeMessage::addAttributeChangedCallback(node, lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		tempID = MNodeMessage::addAttributeChangedCallback(parentNode, lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

	}

	// if the node has kLambert attributes it is a lambert
	if (node.hasFn(MFn::kLambert)) {

		//MStreamUtils::stdOutStream() << "nodeAdded: LAMBERT was added " << endl;
	
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
	NodeDeletedInfo deleteInfo = {}; 

	MFnDagNode dgNode(node); 

	if (node.hasFn(MFn::kCamera)) {

		MFnCamera camNode(node);
		std::string CamName = camNode.name().asChar();
		//MStreamUtils::stdOutStream() << "camera deleted " << CamName << endl;


	}

	else if (node.hasFn(MFn::kMesh)) {

		MFnMesh mesh(node);
		std::string meshName = mesh.name().asChar(); 

		int index = -1;
		for (int i = 0; i < meshesInScene.length(); i++) {
			if (meshesInScene[i] == mesh.name()) {
				index = i;
				deleteInfo.msgHeader.cmdType = CMDTYPE::DELETE_NODE;
				break;
			}
		}

		if (index >= 0) {

			//MStreamUtils::stdOutStream() << "mesh to delete " << meshName << endl;
			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::MESH; 
			deleteInfo.msgHeader.nameLen = meshName.length();
			memcpy(deleteInfo.msgHeader.objName, meshName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index; 
			deleteInfo.nodeName = mesh.name(); 

			nodeDeleteInfoToSend.push_back(deleteInfo);

			meshesInScene.remove(index);
		
		}


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

	else if (node.hasFn(MFn::kTransform)) {

		MFnTransform transfNode(node);
		std::string transfName = transfNode.name().asChar();
		//MStreamUtils::stdOutStream() << "transform deleted " << transfName << endl;


		int index = -1;
		for (int i = 0; i < transformsInScene.length(); i++) {
			if (transformsInScene[i] == transfNode.name()) {
				index = i;
				deleteInfo.msgHeader.cmdType = CMDTYPE::DELETE_NODE;
				break;
			}
		}

		if (index >= 0) {

			//MStreamUtils::stdOutStream() << "mesh to delete " << meshName << endl;
			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::TRANSFORM;
			deleteInfo.msgHeader.nameLen = transfName.length();
			memcpy(deleteInfo.msgHeader.objName, transfName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName = transfNode.name();

			transformsInScene.remove(index);
			nodeDeleteInfoToSend.push_back(deleteInfo);
		}
	}

	else if (node.hasFn(MFn::kLight)) {
		
		MFnLight lightNode(node);
		std::string lightName = lightNode.name().asChar();
		//MStreamUtils::stdOutStream() << "light deleted " << lightName << endl;

		int index = -1;
		for (int i = 0; i < lightInScene.length(); i++) {
			if (lightInScene[i] == lightNode.name()) {
				index = i;
				deleteInfo.msgHeader.cmdType = CMDTYPE::DELETE_NODE;
				break;
			}
		}
			

		if (index >= 0) {

			//MStreamUtils::stdOutStream() << "mesh to delete " << meshName << endl;
			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::LIGHT;
			deleteInfo.msgHeader.nameLen = lightName.length();
			memcpy(deleteInfo.msgHeader.objName, lightName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName = lightNode.name();

			lightInScene.remove(index);
			nodeDeleteInfoToSend.push_back(deleteInfo);
		}
		
	}

	else if (node.hasFn(MFn::kLambert)) {

		MFnDependencyNode materialNode(node);
		std::string matName = materialNode.name().asChar();

		int index = -1;
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == materialNode.name()) {
				index = i;
				
				break;
			}
		}

		if (index >= 0) {

			//MStreamUtils::stdOutStream() << "lambert deleted " << matName << endl;
			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::MATERIAL;
			deleteInfo.msgHeader.nameLen = matName.length();
			memcpy(deleteInfo.msgHeader.objName, matName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName  = matName.c_str();

			materialsInScene.remove(index);
			nodeDeleteInfoToSend.push_back(deleteInfo);
		}

	}

	
	callbackIdArray.remove(MNodeMessage::currentCallbackId());
}

// callback called every x seconds 
void timerCallback(float elapsedTime, float lastTime, void* clientData) {
	
	globalTime += elapsedTime;

	 /* 
	MStreamUtils::stdOutStream() << "\n";
	MStreamUtils::stdOutStream() << " ========== MESHES IN SCENE ========== \n";
	for (int i = 0; i < meshesInScene.length(); i++) {
		MStreamUtils::stdOutStream() << meshesInScene[i] << "\n";

	}
	MStreamUtils::stdOutStream() << "\n";
	 */
	
	

	for (int i = 0; i < meshInfoToSend.size(); i++) {
		
	
		if (meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::NEW_NODE || meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::UPDATE_NODE) {
			const char* msgChar = new char[meshInfoToSend[i].msgHeader.msgSize];

			// copy over msg ======
			memcpy((char*)msgChar, &meshInfoToSend[i].msgHeader, sizeof(MsgHeader));
			memcpy((char*)msgChar + sizeof(MsgHeader), &meshInfoToSend[i].meshData, sizeof(Mesh));

			memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh), meshInfoToSend[i].meshVtx, (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3));
			memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3), meshInfoToSend[i].meshNrms, (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3));
			memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3), meshInfoToSend[i].meshUVs, sizeof(float) * meshInfoToSend[i].meshData.UVcount);

			//if (meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::NEW_NODE) {
				memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.UVcount), &meshInfoToSend[i].materialData, sizeof(Material));
				memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.UVcount) + sizeof(Material), &meshInfoToSend[i].transformMatrix, sizeof(Matrix));
			//}
			
			
			//send it
			if (comLib.send(msgChar, meshInfoToSend[i].msgHeader.msgSize)) {
				MStreamUtils::stdOutStream() << "meshInfoToSend: Message new/update node sent" << "\n";
			}


			// delete allocated arrays + message
			delete[] msgChar; 
			delete[] meshInfoToSend[i].meshVtx;
			delete[] meshInfoToSend[i].meshUVs;
			delete[] meshInfoToSend[i].meshNrms;
		
		}

		else if (meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::UPDATE_NODE_MATERIAL) {

			const char* msgChar = new char[meshInfoToSend[i].msgHeader.msgSize];

			// copy over msg ======
			memcpy((char*)msgChar, &meshInfoToSend[i].msgHeader, sizeof(MsgHeader));
			memcpy((char*)msgChar + sizeof(MsgHeader), &meshInfoToSend[i].meshData, sizeof(Mesh));
			memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Mesh), &meshInfoToSend[i].materialData, sizeof(Material));

			//send it
			if (comLib.send(msgChar, meshInfoToSend[i].msgHeader.msgSize)) {
				//MStreamUtils::stdOutStream() << "meshInfoToSend: Update material on node sent" << "\n";
			}

			delete[] msgChar; 
		}

		
		meshInfoToSend.erase(meshInfoToSend.begin() + i);

	}
	
	for (int i = 0; i < nodeDeleteInfoToSend.size(); i++) {
		
		
		int index = nodeDeleteInfoToSend[i].nodeIndex; 
		const char* msgChar = new char[nodeDeleteInfoToSend[i].msgHeader.msgSize];

		// copy over msg ======
		memcpy((char*)msgChar, &nodeDeleteInfoToSend[i].msgHeader, sizeof(MsgHeader));
	
	

		//send it
		if (comLib.send(msgChar, nodeDeleteInfoToSend[i].msgHeader.msgSize)) {
			//MStreamUtils::stdOutStream() << "nodeDeleted: Message sent" << "\n";
		}

		delete[] msgChar; 
		nodeDeleteInfoToSend.erase(nodeDeleteInfoToSend.begin() + i);

			
	}

	for (int i = 0; i < transformInfoToSend.size(); i++) {

		//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Transform) + sizeof(Matrix));
		const char* msgChar = new char[transformInfoToSend[i].msgHeader.msgSize];
		memcpy((char*)msgChar, &transformInfoToSend[i].msgHeader, sizeof(MsgHeader));

		memcpy((char*)msgChar + sizeof(MsgHeader), &transformInfoToSend[i].transfData, sizeof(Transform));
		memcpy((char*)msgChar + sizeof(MsgHeader) + sizeof(Transform), &transformInfoToSend[i].transformMatrix, sizeof(Matrix));

		//send it
		if (comLib.send(msgChar, transformInfoToSend[i].msgHeader.msgSize)) {
			//MStreamUtils::stdOutStream() << "nodeTransform: Message sent" << "\n";
		}

		delete[] msgChar; 
		transformInfoToSend.erase(transformInfoToSend.begin() + i);
	}

	for (int i = 0; i < materialInfoToSend.size(); i++) {

		const char* msgChar = new char[materialInfoToSend[i].msgHeader.msgSize];

		memcpy((char*)msgChar, &materialInfoToSend[i].msgHeader, sizeof(MsgHeader));
		memcpy((char*)msgChar + sizeof(MsgHeader), &materialInfoToSend[i].materialData, sizeof(Material));


		// send it
		if (comLib.send(msgChar, materialInfoToSend[i].msgHeader.msgSize)) {
			//MStreamUtils::stdOutStream() << "matInfo: Message sent" << "\n";
		}

		delete[] msgChar;
		materialInfoToSend.erase(materialInfoToSend.begin() + i);
	}

	for (int i = 0; i < cameraInfoToSend.size(); i++) {

		//MStreamUtils::stdOutStream() << "cameraInfoToSend " << cameraInfoToSend[i].cameraName << "\n";

		const char* msgChar = new char[cameraInfoToSend[i].msgHeader.msgSize];

		// copy over msg ======
		memcpy((char*)msgChar, &cameraInfoToSend[i].msgHeader, sizeof(MsgHeader));
		memcpy((char*)msgChar + sizeof(MsgHeader), &cameraInfoToSend[i].camData, sizeof(Camera));

		//send it
		if (comLib.send(msgChar, cameraInfoToSend[i].msgHeader.msgSize)) {
			MStreamUtils::stdOutStream() << "camInfo: Message sent" << "\n";
		}

		delete[] msgChar;
		cameraInfoToSend.erase(cameraInfoToSend.begin() + i);


	}

	
}


// ==================================================================================
// =============================== INIT & UNINIT ====================================
// ==================================================================================

// function to initialize callbacks for items in scene (camera, lambert)
void InitializeScene() {

	MCallbackId tempID;

	// get the lamberts in scene at startup
	MItDependencyNodes lambertIterator(MFn::kLambert);
	while (!lambertIterator.isDone()) {

		//MStreamUtils::stdOutStream() << "LAMBERT FOUND\n";
		MObject lambertShader(lambertIterator.thisNode());

		tempID = MNodeMessage::addAttributeChangedCallback(lambertShader, materialAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		MFnLambertShader lambNode(lambertIterator.thisNode()); 
		materialsInScene.append(lambNode.name());
		lambertIterator.next();
	}

	// get all the active cameras in scene at startup 
	MItDependencyNodes cameraIterator(MFn::kCamera);
	while (!cameraIterator.isDone()) {

		MFnCamera cameraNode(cameraIterator.item());
		//MStreamUtils::stdOutStream() << "CAMERA FOUND\n";
		//MStreamUtils::stdOutStream() << "CAM " << cameraNode.name().asChar() << endl;

		MDagPath path;
		MFnDagNode(cameraIterator.thisNode()).getPath(path);
		MFnCamera cam(path);

		/* 
		tempID = MDagMessage::addWorldMatrixModifiedCallback(path, cameraWorldMatrixChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);
		*/
		

		tempID = MNodeMessage::addNameChangedCallback(cameraIterator.thisNode(), nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		cameraIterator.next();
	}

	MItDependencyNodes lightIterator(MFn::kLight);
	while (!lightIterator.isDone()) {

		MFnLight light(lightIterator.item());
		GetLightInfo(light);
		
		tempID = MNodeMessage::addAttributeChangedCallback(lightIterator.thisNode(), lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);
		
		tempID = MNodeMessage::addAttributeChangedCallback(light.parent(0), lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);
		
		lightIterator.next(); 
	}


	
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

