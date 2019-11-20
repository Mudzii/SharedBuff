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


// Output messages in output window
//MStreamUtils::stdOutStream() << "...: " << ... << "_" << endl;

// ===========================================================
// ==================== Variables ============================
// ===========================================================

MStatus status = MS::kSuccess;
MCallbackIdArray callbackIdArray;
bool initBool = false;

MTimer gTimer;
float globalTime = 0.0f;
float timerPeriod = 0.3f;

// arrays with info in scene
Vec3 camPosScene;
MStringArray lightInScene;
MStringArray meshesInScene;
MStringArray materialsInScene;
MStringArray transformsInScene;

// vectors with messages to send
std::vector<MeshInfo> meshInfoToSend;
std::vector<LightInfo> lightInfoToSend;
std::vector<MatrixInfo> matrixInfoToSend; 
std::vector<MaterialInfo> materialInfoToSend;
std::vector<NodeDeletedInfo> nodeDeleteInfoToSend;
std::vector<NodeRenamedInfo> nodeRenamedInfoToSend;


//std::vector<NodeInfo> newNodeInfo; 
//std::vector<NodeInfo> updateNodeInfo;


//std::vector<CameraInfo> cameraInfoToSend;
//std::vector<TransformInfo> transformInfoToSend;

// ==================================================================================
// ============================== MATERIAL ==========================================
// ==================================================================================

// callback for texture plug - meaning if a texture image has been connected to a specific material
void texturePlugAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	// if the attribute was set, meaning the plug was connected to someting
	if (msg & msg & MNodeMessage::kAttributeSet) {

		/* 
		MStreamUtils::stdOutStream() << "==================================" << endl;
		MStreamUtils::stdOutStream() << "TEX PLUG CHANGED" << endl;
		MStreamUtils::stdOutStream() << endl;
		*/
		

		MStatus result;
		MPlugArray connections;
		MFnDependencyNode fn(plug.node());

		// get the outColor plug which is connected to lambert 
		std::string fileNameString;
		MPlugArray textureConnections;
		MPlug colorPlug = fn.findPlug("outColor", &result);

		// if colorplug was found, proceed
		if (result) {

			// check if anything is connected to the color plug (as source). 
			colorPlug.connectedTo(connections, false, true);
			if (connections.length() > 0) {

				// get lambert node and color plug connected to texture plug
				MFnDependencyNode lambertNode(connections[0].node());
				MFnLambertShader lambertShader(connections[0].node());
				MPlug lamColorPlug = lambertShader.findPlug("color", &result);

				if (result) {

					// get the connections of the lambert color plug
					lamColorPlug.connectedTo(textureConnections, true, false);
					if (textureConnections.length() > 0) {

						//get filepath
						MFnDependencyNode textureNode(textureConnections[0].node());
						MPlug fileTextureName = textureNode.findPlug("ftn");

						MString fileName;
						fileTextureName.getValue(fileName);
						fileNameString = fileName.asChar();

						// check if material exists in scene
						bool matExists = false;
						for (int i = 0; i < materialsInScene.length(); i++) {
							if (materialsInScene[i] == lambertNode.name().asChar()) {
								matExists = true;
								break;
							}
						}

						if (matExists) {
							//MStreamUtils::stdOutStream() << "Material exists. Editing texture " << endl;

							MaterialInfo mMatInfo = {};
							//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));

							mMatInfo.msgHeader.nodeType = NODE_TYPE::MATERIAL;
							mMatInfo.msgHeader.msgSize  = totalMsgSizeMaterial;
							mMatInfo.msgHeader.cmdType  = CMDTYPE::UPDATE_MATERIAL;

							mMatInfo.msgHeader.nameLen = lambertNode.name().length();
							memcpy(mMatInfo.msgHeader.objName, lambertNode.name().asChar(), mMatInfo.msgHeader.nameLen);

							mMatInfo.materialData.type = 1;
							mMatInfo.materialData.color = { 255,255,255,255 };
							mMatInfo.materialData.matNameLen = lambertNode.name().length();
							memcpy(mMatInfo.msgHeader.objName, lambertNode.name().asChar(), mMatInfo.msgHeader.nameLen);

							mMatInfo.materialData.textureNameLen = fileNameString.length();
							memcpy(mMatInfo.materialData.fileTextureName, fileNameString.c_str(), mMatInfo.materialData.textureNameLen);

							mMatInfo.materialData.matNameLen = lambertNode.name().length();
							memcpy(mMatInfo.materialData.materialName, lambertNode.name().asChar(), mMatInfo.materialData.matNameLen);

							mMatInfo.matName = lambertNode.name(); 

							// check if a msg with the material already exists. If it does, replace it
							bool msgExists = false;
							for (int i = 0; i < materialInfoToSend.size(); i++) {

								if (materialInfoToSend[i].matName == lambertNode.name()) {
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

	/* 
	MStreamUtils::stdOutStream() << endl;(msg & MNodeMessage::kAttributeSet) &&
	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "MATERIAL ATTR CHANGED" << endl;
	MStreamUtils::stdOutStream() << endl;
	*/
	
	if ((plug.node().hasFn(MFn::kLambert))) { // && (msg != 2052)) {

		MStatus result;
		MaterialInfo mMatInfo = {};

		// lambert material variables
		MPlug transp;
		MColor color;
		MPlug colorPlug;
		float transparency = 1;
		Color mColor = { 255,255,255,255 };
		MFnLambertShader lambertShader;

		// texture variables
		bool hasTexture = false;
		std::string fileNameString;
		MPlugArray textureConnections;

		// get lambert color through Color plug
		lambertShader.setObject(plug.node());
		colorPlug = lambertShader.findPlug("color", &result);

		if (result) {


			/* 
			// get material transparency
			transp = lambertShader.findPlug("transparency", &result);
			if (result) {
				MPlug transpAttr;
				MColor transpClr;
				transpAttr = lambertShader.findPlug("transparencyR");
				transpAttr.getValue(transpClr.r);
				transparency = transpClr.r;
			}
			*/

			// get material color through color plugs
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
			int alpha = transparency * 255;
			mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)alpha };


			// to check for textures, check if color plug is dest for another plug
			colorPlug.connectedTo(textureConnections, true, false);
			if (textureConnections.length() > 0) {

				//get filepath
				MFnDependencyNode textureNode(textureConnections[0].node());
				MPlug fileTextureName = textureNode.findPlug("ftn");

				// connect a callback to texture plug
				MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(fileTextureName.node(), texturePlugAttributeChanged, NULL, &status);
				if (status == MS::kSuccess)
					callbackIdArray.append(tempID);

				// get the filename of texture
				MString fileName;
				fileTextureName.getValue(fileName);
				fileNameString = fileName.asChar();
				hasTexture = true;
			}

			// if an attribute was set (ex color changed) or 
			if ((msg & MNodeMessage::kAttributeSet) || (msg & MNodeMessage::kOtherPlugSet)) {


				// check if material already exists in scene, otherwise add
				int index = -1;
				for (int i = 0; i < materialsInScene.length(); i++) {
					if (materialsInScene[i] == lambertShader.name()) {

						index = i;
						//MStreamUtils::stdOutStream() << "Material already exists " << endl;
						mMatInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
						break;
					}
				}

				if (index == -1) {

					//MStreamUtils::stdOutStream() << "Material didn't exist. Adding " << endl;
					materialsInScene.append(lambertShader.name());
					mMatInfo.msgHeader.cmdType = CMDTYPE::NEW_MATERIAL;
				}

				else if (index >= 0) {
					mMatInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATERIAL;
				}

				// fill the MaterialInfo struct to add to messages
				//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Material));

				mMatInfo.msgHeader.msgSize = totalMsgSizeMaterial;
				mMatInfo.msgHeader.nodeType = NODE_TYPE::MATERIAL;

				mMatInfo.msgHeader.nameLen = lambertShader.name().length();
				memcpy(mMatInfo.msgHeader.objName, lambertShader.name().asChar(), mMatInfo.msgHeader.nameLen);

				mMatInfo.materialData.color = mColor;
				mMatInfo.materialData.matNameLen = lambertShader.name().length();
				memcpy(mMatInfo.materialData.materialName, lambertShader.name().asChar(), mMatInfo.materialData.matNameLen);

				mMatInfo.matName = lambertShader.name();

				if (hasTexture) {
					mMatInfo.materialData.type = 1;
					mMatInfo.materialData.textureNameLen = fileNameString.length();
					memcpy(mMatInfo.materialData.fileTextureName, fileNameString.c_str(), mMatInfo.materialData.textureNameLen);
				}

				else {
					mMatInfo.materialData.type = 0;
					mMatInfo.materialData.textureNameLen = 2;
					memcpy(mMatInfo.materialData.fileTextureName, "NA", mMatInfo.materialData.textureNameLen);

				}

				// check if msg exists. If so update. Otherwise add to msg vector
				bool msgExists = false;
				for (int i = 0; i < materialInfoToSend.size(); i++) {
					if (materialInfoToSend[i].matName == lambertShader.name() && materialInfoToSend[i].msgHeader.cmdType != CMDTYPE::NEW_MATERIAL) {
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
	bool msgToSend = false;

	// get transform node through path
	MDagPath path;
	MFnDagNode(transformNode).getPath(path);
	MFnTransform transfNode(path, &result);

	MMatrix transfMatAbs;
	Matrix mTransformMatrix = {};
	TransformInfo mTransfInfo = {};

	
}

// ==================================================================================
// ================================= MESH ===========================================
// ==================================================================================

// function to go through meshes in scene. Returns mesh index.
int findMesh(MString MeshName) {

	int objIndex = -1;
	for (int i = 0; i < meshesInScene.length(); i++) {
		if (meshesInScene[i] == MeshName) {

			objIndex = i;
			break;
		}
	}

	return objIndex;
}

// function that gets mesh information (vtx, normal, UV), material and matrix information for meshes
void GetMeshInfo(MFnMesh &mesh) {

	/* 
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "GET MESH INFO" << endl;
	*/

	MStatus result;

	MeshInfo mMeshInfo = {};
	Color mColor	   = { 255, 255, 255, 255 };

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

	// material ===================================

	// lambert material 
	MColor color = { 255, 255, 255, 255 };
	float transparency = 1;
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
		MPlug surfaceShader = initialShadingGroup.findPlug("surfaceShader", &result);

		if (result) {

			// to find material, find the destination connections for surficeShader plug
			// (AKA plugs that have surface shader as destination)
			surfaceShader.connectedTo(shaderConnections, true, false);
			if (shaderConnections.length() > 0) {

				//Only support 1 material per mesh (and only lambert). Check if first connection is lambert
				if (shaderConnections[0].node().apiType() == MFn::kLambert) {

					lambertShader.setObject(shaderConnections[0].node());
					materialNamePlug = lambertShader.name().asChar();

					// get lambert color through Color plug
					colorPlug = lambertShader.findPlug("color", &result);
					if (result) {

						/* 
						// get material transperency
						MPlug transp = lambertShader.findPlug("transparency", &result);
						if (result) {
							MPlug transpAttr;
							MColor transpClr;
							transpAttr = lambertShader.findPlug("transparencyR");
							transpAttr.getValue(transpClr.r);
							transparency = transpClr.r;
						}
						*/

						// get material color
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
						int alpha = transparency * 255;
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
							hasTexture = true;
						}

					}

				}
			}

		}
	}

	// check if mesh already exists in scene, otherwise add
	int index = findMesh(mesh.name());

	if (index >= 0)
		mMeshInfo.msgHeader.cmdType = CMDTYPE::UPDATE_NODE;

	else {
		meshesInScene.append(mesh.name());
		mMeshInfo.msgHeader.cmdType = CMDTYPE::NEW_NODE;
		index = meshesInScene.length() - 1;
	}

	// if new mesh, get material connected to the new mesh
	if (mMeshInfo.msgHeader.cmdType == CMDTYPE::NEW_NODE) {

		// check if material already exists in scene, otherwise add
		int matIndex = -1;
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == lambertShader.name()) {

				matIndex = i;
				break;
			}
		}

		if (matIndex == -1)
			materialsInScene.append(lambertShader.name());

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
	mMeshInfo.meshData.meshID    = index;
	mMeshInfo.meshData.trisCount = totalTrisCnt;
	mMeshInfo.meshData.UVcount   = uvArray.length();
	mMeshInfo.meshData.vtxCount  = vtxArray.length();
	mMeshInfo.meshData.normalCount = normalArray.length();

	mMeshInfo.meshName	   = mesh.name();
	
	// get transform matrix for new mesh
	if (mMeshInfo.msgHeader.cmdType == CMDTYPE::NEW_NODE) {

		MFnTransform parentTransform(mesh.parent(0), &result);
		MMatrix transformationMatrix;

		if (result) 
			transformationMatrix = parentTransform.transformationMatrix();
		
		else 
			transformationMatrix = mesh.transformationMatrix();
		
		float matFloat[4][4]; 	
		transformationMatrix.get(matFloat); 

		mMeshInfo.transformMatrix = {  matFloat[0][0], matFloat[1][0], matFloat[2][0], matFloat[3][0],
						matFloat[0][1], matFloat[1][1], matFloat[2][1], matFloat[3][1],
						matFloat[0][2], matFloat[1][2], matFloat[2][2], matFloat[3][2],
						matFloat[0][3], matFloat[1][3], matFloat[2][3], matFloat[3][3] };

		
	}

	// Fill header ========
	size_t totalMsgSize;

	if (mMeshInfo.msgHeader.cmdType == CMDTYPE::NEW_NODE)
		totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * mMeshInfo.meshData.vtxCount * 3) + (sizeof(float) * mMeshInfo.meshData.normalCount * 3) + (sizeof(float) * mMeshInfo.meshData.UVcount) + sizeof(Material) + sizeof(Matrix));

	else if (mMeshInfo.msgHeader.cmdType == CMDTYPE::UPDATE_NODE)
		totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * mMeshInfo.meshData.vtxCount * 3) + (sizeof(float) * mMeshInfo.meshData.normalCount * 3) + (sizeof(float) * mMeshInfo.meshData.UVcount));

	mMeshInfo.msgHeader.msgSize  = totalMsgSize;
	mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH;
	mMeshInfo.msgHeader.nameLen  = mesh.name().length();
	memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);

	// check if msg already exists. If so, update 
	// make sure that it only updates the message if it isn't a new node
	bool msgExists = false;
	for (int i = 0; i < meshInfoToSend.size(); i++) {
		if (meshInfoToSend[i].meshName == mesh.name() && meshInfoToSend[i].msgHeader.cmdType != CMDTYPE::NEW_NODE) {
			
			msgExists = true;

			// if vtx (and thus normal and UV) count are not the same, allocate new 
			if (meshInfoToSend[i].meshData.vtxCount != mMeshInfo.meshData.vtxCount) {

				delete[] meshInfoToSend[i].meshVtx;
				delete[] meshInfoToSend[i].meshUVs;
				delete[] meshInfoToSend[i].meshNrms;

				meshInfoToSend[i].meshUVs  = new float[mMeshInfo.meshData.UVcount];
				meshInfoToSend[i].meshVtx  = new float[mMeshInfo.meshData.vtxCount * 3];
				meshInfoToSend[i].meshNrms = new float[mMeshInfo.meshData.normalCount * 3];
			}

			meshInfoToSend[i].meshData = mMeshInfo.meshData;
			memcpy((char*)meshInfoToSend[i].meshUVs,  meshUVs, (sizeof(float) * (mMeshInfo.meshData.UVcount)));
			memcpy((char*)meshInfoToSend[i].meshVtx,  meshVtx, (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
			memcpy((char*)meshInfoToSend[i].meshNrms, meshNormals, (sizeof(float) * mMeshInfo.meshData.normalCount * 3));

			break;
		}
	}
	
	// if it doesn't exist, add it to the queue 
	if (!msgExists) {

		mMeshInfo.meshUVs  = new float[mMeshInfo.meshData.UVcount];
		mMeshInfo.meshVtx  = new float[mMeshInfo.meshData.vtxCount * 3];
		mMeshInfo.meshNrms = new float[mMeshInfo.meshData.normalCount * 3];

		memcpy((char*)mMeshInfo.meshUVs,  meshUVs, (sizeof(float) * (mMeshInfo.meshData.UVcount)));
		memcpy((char*)mMeshInfo.meshVtx,  meshVtx, (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
		memcpy((char*)mMeshInfo.meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));

		meshInfoToSend.push_back(mMeshInfo);
	}

	// delete allocated arrays 
	delete[] meshVtx;
	delete[] meshUVs;
	delete[] meshNormals;

}

// function that gets mesh information (vtx, normal, UV) for existing meshes if multicut, extrude etc is used
void GeometryUpdate(MFnMesh &mesh) {

	/* 
	MStreamUtils::stdOutStream() << endl;
	MStreamUtils::stdOutStream() << "==================================" << endl;
	MStreamUtils::stdOutStream() << "GEOMETRY UPDATE" << endl;
	MStreamUtils::stdOutStream() << endl;
	*/

	MStatus result;
	MeshInfo mMeshInfo = {};
	Color mColor	   = { 255, 255, 255, 255 };

	// mesh information (vtx, UV, norm) ===========
	MItMeshPolygon polyIterator(mesh.object(), &result);
	int trisCurrentFace = 0;
	int totalTrisCnt    = 0;

	MPointArray points;
	MIntArray vertexList;

	MFloatArray uvArray;
	MPointArray vtxArray;
	MFloatVectorArray normalArray;

	// Checks if the mesh has a valid triangulation. If it doesn't, then the mesh was bad geometry
	if (polyIterator.hasValidTriangulation()) {
		while (!polyIterator.isDone()) {

			// get the tris count for the face that it's currently iterating through
			polyIterator.numTriangles(trisCurrentFace);

			// get vtx and vtx positions of all the triangles in the current face
			polyIterator.getTriangles(points, vertexList);
			//MStreamUtils::stdOutStream() << trisCurrentFace << endl;

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
	int index = findMesh(mesh.name());
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
		mMeshInfo.meshData.vtxCount    = vtxArray.length();
		mMeshInfo.meshData.normalCount = normalArray.length();

		mMeshInfo.meshName	   = mesh.name();
		//mMeshInfo.meshPathName = mesh.fullPathName();


		// Fill header ========
		size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * mMeshInfo.meshData.vtxCount * 3) + (sizeof(float) * mMeshInfo.meshData.normalCount * 3) + (sizeof(float) * mMeshInfo.meshData.UVcount));

		mMeshInfo.msgHeader.msgSize  = totalMsgSize;
		mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH;
		mMeshInfo.msgHeader.cmdType  = CMDTYPE::UPDATE_NODE;
		mMeshInfo.msgHeader.nameLen  = mesh.name().length();
		memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);

		// check if msg already exists. If so, update
		// make sure that it's not a new node's message that is updated
		bool msgExists = false;
		for (int i = 0; i < meshInfoToSend.size(); i++) {
			if (meshInfoToSend[i].meshName == mesh.name() && meshInfoToSend[i].msgHeader.cmdType != CMDTYPE::NEW_NODE) {
				msgExists = true;

				// if vtx, normal or UV count is different from the stored info, update
				if (meshInfoToSend[i].meshData.vtxCount != mMeshInfo.meshData.vtxCount) {

					delete[] meshInfoToSend[i].meshVtx;
					delete[] meshInfoToSend[i].meshUVs; 
					delete[] meshInfoToSend[i].meshNrms;

					meshInfoToSend[i].meshUVs  = new float[mMeshInfo.meshData.UVcount];
					meshInfoToSend[i].meshVtx  = new float[mMeshInfo.meshData.vtxCount * 3];
					meshInfoToSend[i].meshNrms = new float[mMeshInfo.meshData.normalCount * 3];

				}

				meshInfoToSend[i].meshData = mMeshInfo.meshData;
				meshInfoToSend[i].msgHeader = mMeshInfo.msgHeader;
				//meshInfoToSend[i].msgHeader.nodeType = NODE_TYPE::MESH;
				//meshInfoToSend[i].msgHeader.cmdType = CMDTYPE::UPDATE_NODE;

				memcpy((char*)meshInfoToSend[i].meshUVs,  meshUVs, (sizeof(float) * (mMeshInfo.meshData.UVcount)));
				memcpy((char*)meshInfoToSend[i].meshVtx,  meshVtx, (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
				memcpy((char*)meshInfoToSend[i].meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));

				break;
			}
		}

		// if it doesn't exist, add it to the queue 
		if (!msgExists) {
			mMeshInfo.meshUVs  = new float[mMeshInfo.meshData.UVcount];
			mMeshInfo.meshVtx  = new float[mMeshInfo.meshData.vtxCount * 3];
			mMeshInfo.meshNrms = new float[mMeshInfo.meshData.normalCount * 3];

			memcpy((char*)mMeshInfo.meshUVs,  meshUVs, (sizeof(float) * (mMeshInfo.meshData.UVcount)));
			memcpy((char*)mMeshInfo.meshVtx,  meshVtx, (sizeof(float) * (mMeshInfo.meshData.vtxCount * 3)));
			memcpy((char*)mMeshInfo.meshNrms, meshNormals, (sizeof(float) * (mMeshInfo.meshData.normalCount * 3)));

			meshInfoToSend.push_back(mMeshInfo);
		}

		delete[] meshUVs;
		delete[] meshVtx;
		delete[] meshNormals;
	}
}

// function that gets material info for a mesh if a new material is connected
void MaterialChanged(MFnMesh &mesh) {

	// check if mesh exists in scene
	int index = findMesh(mesh.name());

	if (index != -1) {

		/* 
		MStreamUtils::stdOutStream() << endl;
		MStreamUtils::stdOutStream() << "==================================" << endl;
		MStreamUtils::stdOutStream() << "MATERIAL ON MESH CHANGED" << endl;
		MStreamUtils::stdOutStream() << endl;
		*/
		

		MStatus result;
		float transparency = 1;
		MeshInfo mMeshInfo = {};
		Color mColor	   = { 255,255,255,255 };

		// lambert material 
		MColor color;
		MPlug colorPlug;
		std::string materialNamePlug;
		MFnLambertShader lambertShader;

		MIntArray indices;
		MPlugArray shaderConnections;
		MObjectArray connectedShaders;

		// texture variables
		bool hasTexture = false;
		std::string fileNameString;
		MPlugArray textureConnections;

		// get the shaders connected to the mesh
		mesh.getConnectedShaders(0, connectedShaders, indices);
		if (connectedShaders.length() > 0) {

			// if there are any connected shaders, get first one. Check if it's connected to surfaceShader
			MFnDependencyNode initialShadingGroup(connectedShaders[0]);
			MPlug surfaceShader = initialShadingGroup.findPlug("surfaceShader", &result);

			if (result) {
				// to find material, find the destination connections for surficeShader plug
				// (AKA plugs that have surface shader as destination)
				surfaceShader.connectedTo(shaderConnections, true, false);

				if (shaderConnections.length() > 0) {

					//Only support 1 material per mesh (and only lambert). Check if first connection is lambert
					if (shaderConnections[0].node().apiType() == MFn::kLambert) {

						// set the object and get the name of the material 
						lambertShader.setObject(shaderConnections[0].node());
						materialNamePlug	= lambertShader.name().asChar();

						// check for the color plug on the material
						colorPlug = lambertShader.findPlug("color", &result);
						if (result) {

							/* 
							// check transparency
							MPlug transp = lambertShader.findPlug("transparency", &result);
							if (result) {
								MPlug transpAttr;
								MColor transpClr;
								transpAttr = lambertShader.findPlug("transparencyR");
								transpAttr.getValue(transpClr.r);
								transparency = transpClr.r;
							}
							*/

							// get the color 
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
							int alpha = transparency * 255;
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
				break;
			}
		}

		if (matIndex == -1) 
			materialsInScene.append(lambertShader.name());
		
		//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Mesh) + sizeof(Material));

		mMeshInfo.msgHeader.msgSize  = totalMsgSizeMatAndMesh;
		mMeshInfo.msgHeader.nodeType = NODE_TYPE::MESH;
		mMeshInfo.msgHeader.cmdType  = CMDTYPE::UPDATE_NODE_MATERIAL;

		mMeshInfo.msgHeader.nameLen = mesh.name().length();
		memcpy(mMeshInfo.msgHeader.objName, mesh.name().asChar(), mMeshInfo.msgHeader.nameLen);

		mMeshInfo.meshData.UVcount	   = 0;
		mMeshInfo.meshData.vtxCount    = 0;
		mMeshInfo.meshData.trisCount   = 0;
		mMeshInfo.meshData.normalCount = 0;
		mMeshInfo.meshData.meshID	   = index;
		mMeshInfo.meshData.matNameLen  = materialNamePlug.length();
		memcpy(mMeshInfo.meshData.materialName, materialNamePlug.c_str(), mMeshInfo.meshData.matNameLen);

		mMeshInfo.meshName = mesh.name();
		//mMeshInfo.meshPathName = mesh.fullPathName();

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

	MStatus result;
	std::string plugName	  = plug.name().asChar();
	std::string otherPlugName = otherPlug.name().asChar();
	std::string plugPartName  = plug.partialName().asChar();

	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);

	// check if the plug has in its name "doubleSided", which is an attribute that is set when the mesh is finally available
	if (plugName.find("doubleSided") != std::string::npos && (msg & MNodeMessage::AttributeMessage::kAttributeSet)) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);

		// if result gives no error, get mesh information through the node
		if (result) {
			//MStreamUtils::stdOutStream() << "Double sided: In result statement" << endl;
			GetMeshInfo(meshNode);
		}



	}

	// if vtx has changed on a mesh changed
	else if (plugPartName.find("pt[") != std::string::npos) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);
		if (result) {
			//MStreamUtils::stdOutStream() << "Vertex In result statement" << endl;
			GetMeshInfo(meshNode);
		}
	}

	// if normal has changed on a mesh
	else if (otherPlugName.find("polyNormal") != std::string::npos || (otherPlugName.find("polySoftEdge") != std::string::npos && otherPlugName.find("manipMatrix") != std::string::npos)) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);
		if (result) {
			//MStreamUtils::stdOutStream() << "normal In result statement" << endl;
			GetMeshInfo(meshNode);
		}
	}

	// if pivot changed, UV probably changed on a mesh
	else if (plugPartName.find("pv") != std::string::npos) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);
		if (result) {
			//MStreamUtils::stdOutStream() << "UV In result statement" << endl;
			GetMeshInfo(meshNode);

		}
	}

	// if material connection changed on a mesh (i.e another material was assigned to the mesh)
	else if (plugPartName.find("iog") != std::string::npos && otherPlug.node().apiType() == MFn::Type::kShadingEngine) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);
		if (result) {
			//MStreamUtils::stdOutStream() << "material changed. In result statement" << endl;
			MaterialChanged(meshNode);
		}
	}

	// used for when moving a face after extrude is used
	else if ((plugName.find("outMesh") != std::string::npos) && (plugName.find("polySplit") == std::string::npos) && (otherPlugName.find("polyExtrude") == std::string::npos)) {

		// get mesh through dag path
		MFnMesh meshNode(path, &result);
		if (result) {

			// check if mesh exists in scene
			int index = findMesh(meshNode.name());
			if (index >= 0) {
				//MStreamUtils::stdOutStream() << "outmesh In result statement" << endl;
				GeometryUpdate(meshNode);
			}
		}
	}

}

// callback for whenever topology is changed (poly split, extrude, subdivision change)
void meshTopologyChanged(MObject& node, void* clientData) {

	if (node.hasFn(MFn::kMesh)) {

		MStatus stat;
		MFnMesh meshNode(node, &stat);

		if (stat) {
			//MStreamUtils::stdOutStream() << " =============================== " << endl;
			//MStreamUtils::stdOutStream() << "TOPOLOGY CHANGED on " << meshNode.name() << endl;
			GeometryUpdate(meshNode);
		}
	}

}

// callback for when world matrix changes for mesh
void meshWorldMatrixChanged(MObject &transformNode, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	MStatus result;
	
	MatrixInfo mMatrixInfo = {};
	MItDag dagIterator(MItDag::kDepthFirst, MFn::kTransform, &status);
	dagIterator.reset(transformNode);

	//MStreamUtils::stdOutStream() << " =================== " << endl;
	for (; !dagIterator.isDone(); dagIterator.next()) {

		// get path for selected node
		MDagPath objectPath; 
		status = dagIterator.getPath(objectPath); 

		// skip over the transform with shape 
		if ((objectPath.hasFn(MFn::kMesh)) && (objectPath.hasFn(MFn::kTransform))) {
			continue;
		}

		// get mesh shape node
		else if (objectPath.hasFn(MFn::kMesh)) {

			MFnMesh meshNode(objectPath);
			MMatrix meshMatrix = objectPath.inclusiveMatrix(); 
			float matFloat[4][4]; 
			meshMatrix.get(matFloat); 

			//MStreamUtils::stdOutStream() << " ------------------------- " << endl;
			//MStreamUtils::stdOutStream() << "meshNode: " << meshNode.name() << endl;


			mMatrixInfo.nodeName = meshNode.name(); 

			mMatrixInfo.matrixData = {  matFloat[0][0], matFloat[1][0], matFloat[2][0], matFloat[3][0],
										matFloat[0][1], matFloat[1][1], matFloat[2][1], matFloat[3][1],
										matFloat[0][2], matFloat[1][2], matFloat[2][2], matFloat[3][2],
										matFloat[0][3], matFloat[1][3], matFloat[2][3], matFloat[3][3] };
	
			//MsgHeader msgHeader = {}; 
			mMatrixInfo.msgHeader.nodeType = NODE_TYPE::MESH;
			mMatrixInfo.msgHeader.msgSize = totalMsgSizeMatrix;
			mMatrixInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATRIX;
			mMatrixInfo.msgHeader.nameLen = meshNode.name().length();
			memcpy(mMatrixInfo.msgHeader.objName, meshNode.name().asChar(), mMatrixInfo.msgHeader.nameLen);

			bool msgExists = false;
			
			for (int i = 0; i < matrixInfoToSend.size(); i++) {
				if (matrixInfoToSend[i].nodeName == meshNode.name()) {
					//MStreamUtils::stdOutStream() << "msg exists. Changing...."  << endl;

					msgExists = true;
					matrixInfoToSend[i] = mMatrixInfo;
				}
			}
			

			if (!msgExists) {
				//MStreamUtils::stdOutStream() << "msg doesn't exists. Adding..." << endl;

				matrixInfoToSend.push_back(mMatrixInfo);
			}
		}

	}
	
	
	/* 
	if (result) {
		absMatrix = path.inclusiveMatrix(); 
		MatrixInfo mMatrixInfo = {};

		//row 1
		mMatrixInfo.matrixData.a11 = absMatrix(0, 0);
		mMatrixInfo.matrixData.a12 = absMatrix(1, 0);
		mMatrixInfo.matrixData.a13 = absMatrix(2, 0);
		mMatrixInfo.matrixData.a14 = absMatrix(3, 0);

		//row 2
		mMatrixInfo.matrixData.a21 = absMatrix(0, 1);
		mMatrixInfo.matrixData.a22 = absMatrix(1, 1);
		mMatrixInfo.matrixData.a23 = absMatrix(2, 1);
		mMatrixInfo.matrixData.a24 = absMatrix(3, 1);

		//row 3
		mMatrixInfo.matrixData.a31 = absMatrix(0, 2);
		mMatrixInfo.matrixData.a32 = absMatrix(1, 2);
		mMatrixInfo.matrixData.a33 = absMatrix(2, 2);
		mMatrixInfo.matrixData.a34 = absMatrix(3, 2);

		//row 4
		mMatrixInfo.matrixData.a41 = absMatrix(0, 3);
		mMatrixInfo.matrixData.a42 = absMatrix(1, 3);
		mMatrixInfo.matrixData.a43 = absMatrix(2, 3);
		mMatrixInfo.matrixData.a44 = absMatrix(3, 3);

		//MsgHeader msgHeader = {}; 
		mMatrixInfo.msgHeader.nodeType = NODE_TYPE::MESH;
		mMatrixInfo.msgHeader.msgSize = totalMsgSizeMatrix;
		mMatrixInfo.msgHeader.cmdType = CMDTYPE::UPDATE_MATRIX;
		mMatrixInfo.msgHeader.nameLen = meshNode.name().length();
		memcpy(mMatrixInfo.msgHeader.objName, meshNode.name().asChar(), mMatrixInfo.msgHeader.nameLen);

		
		// copy over msg ======
		const char* msg = new char[totalMsgSizeMatrix];

		memcpy((char*)msg, &mMatrixInfo.msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &mMatrixInfo.matrixData, sizeof(Matrix));

		//send it
		if (comLib.send(msg, totalMsgSizeMatrix)) {
			//MStreamUtils::stdOutStream() << "activeCamera: Message sent" << "\n";
		}

		delete[]msg;
		

	}
	*/
	
	/* 
	else {
		
		// add first node to hierarchy
		MObjectArray hierarchy;
		MObject firstParentNode(transfNode.object());
		hierarchy.append(firstParentNode);

		// get hierarchy to mesh
		for (int j = 0; j < hierarchy.length(); j++) {

				// get the next node in hierarchy and check child count
				MFnDagNode prnt(hierarchy[j]);
				int prntChildCnt = prnt.childCount();

				// if it has children, loop through
				for (int i = 0; i < prntChildCnt; i++) {
					// get the child object
					MObject childObject(prnt.child(i));

					// if the child is of type kTransform and has children or is a mesh, add to chirarchy 
					if (childObject.apiType() == MFn::kTransform || childObject.apiType() == MFn::kMesh) {

						MFnDagNode child(childObject);
						int tempChildCnt = child.childCount();
						std::string childName = child.fullPathName().asChar();

						if (tempChildCnt != 0 ) { 
							hierarchy.append(childObject);
						}

						if (childName.find("Shape") != std::string::npos)
							break; 
					}
	
				}
		}
		
		
	
		

		childHierarchy.setObject(hierarchy[hierarchy.length() - 1]);
		MDagPath path2;
		MFnDagNode(hierarchy[hierarchy.length() - 1]).getPath(path2);
		meshNode.setObject(path2);

		//MStreamUtils::stdOutStream() << childHierarchy.name() << endl;
		//MStreamUtils::stdOutStream() << path2.fullPathName() << endl;
		MStreamUtils::stdOutStream() << "Mesh " << meshNode.name() << endl;

		absMatrix = path2.inclusiveMatrix(); 
		
		//MStreamUtils::stdOutStream() << " ============================== "<< endl;
	

		

		MatrixInfo mMatrixInfo = {}; 


		//row 1
		mMatrixInfo.matrixData.a11 = absMatrix(0, 0);
		mMatrixInfo.matrixData.a12 = absMatrix(1, 0);
		mMatrixInfo.matrixData.a13 = absMatrix(2, 0);
		mMatrixInfo.matrixData.a14 = absMatrix(3, 0);

		//row 2
		mMatrixInfo.matrixData.a21 = absMatrix(0, 1);
		mMatrixInfo.matrixData.a22 = absMatrix(1, 1);
		mMatrixInfo.matrixData.a23 = absMatrix(2, 1);
		mMatrixInfo.matrixData.a24 = absMatrix(3, 1);

		//row 3
		mMatrixInfo.matrixData.a31 = absMatrix(0, 2);
		mMatrixInfo.matrixData.a32 = absMatrix(1, 2);
		mMatrixInfo.matrixData.a33 = absMatrix(2, 2);
		mMatrixInfo.matrixData.a34 = absMatrix(3, 2);

		//row 4
		mMatrixInfo.matrixData.a41 = absMatrix(0, 3);
		mMatrixInfo.matrixData.a42 = absMatrix(1, 3);
		mMatrixInfo.matrixData.a43 = absMatrix(2, 3);
		mMatrixInfo.matrixData.a44 = absMatrix(3, 3);

		// fill TransformInfo struct to add to msg vector
		//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Matrix));

		//MsgHeader msgHeader = {}; 
		mMatrixInfo.msgHeader.msgSize   = totalMsgSizeMatrix;
		mMatrixInfo.msgHeader.nodeType  = NODE_TYPE::MESH;
		mMatrixInfo.msgHeader.cmdType   = CMDTYPE::UPDATE_MATRIX;
		mMatrixInfo.msgHeader.nameLen   = meshNode.name().length();
		memcpy(mMatrixInfo.msgHeader.objName, meshNode.name().asChar(), mMatrixInfo.msgHeader.nameLen);

		// copy over msg ======
		const char* msg = new char[totalMsgSizeMatrix];

		memcpy((char*)msg, &mMatrixInfo.msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &mMatrixInfo.matrixData, sizeof(Matrix));

		//send it
		if (comLib.send(msg, totalMsgSizeMatrix)) {
			//MStreamUtils::stdOutStream() << "activeCamera: Message sent" << "\n";
		}

		delete[]msg;
		


		//matrixInfoToSend.push_back(mMatrixInfo);
			

		
		
	}
		*/

}

// ==================================================================================
// ================================ CAMERA ==========================================
// ==================================================================================

// callback function for the active pannel in the viewport
void activeCamera(const MString &panelName, void* cliendData) {

	MStatus status = MS::kFailure;
	
	//get active camera
	M3dView activeView;
	status = M3dView::getM3dViewFromModelPanel(panelName, activeView);
	if (status != MS::kSuccess) {
		MStreamUtils::stdOutStream() << "Did not get active 3DView" << endl;
		MStreamUtils::stdOutStream() << "Error: " << status << endl;
		return;
	}

	//get name
	MDagPath camDag;
	activeView.getCamera(camDag);
	MFnCamera cameraNode(camDag);

	// get height/width
	//int height = activeView.portHeight();
	//int width  = activeView.portWidth();

	//get relevant vectors for raylib
	MFnCamera cameraView(camDag);
	std::string objName = cameraView.name().asChar();

	bool isOrtographic = cameraView.isOrtho();
	MPoint camPos	   = cameraView.eyePoint(MSpace::kWorld, &status);
	MVector upDir	   = cameraView.upDirection(MSpace::kWorld, &status);
	MVector rightDir   = cameraView.rightDirection(MSpace::kWorld, &status);
	MPoint COI		   = cameraView.centerOfInterestPoint(MSpace::kWorld, &status);

	
	double FOV = 0; // = cameraNode.verticalFieldOfView();
	//get fov or zoom depending on ortho/persp
	if (isOrtographic) {
		double ortoWidth = cameraView.orthoWidth(&status);
		FOV = ortoWidth;
	}

	else {
		//double verticalFOV;
		//double horizontalFOV;
		//cameraView.getPortFieldOfView(width, height, horizontalFOV, verticalFOV);

		// get FOV for perspective cam and change FOV from radians to degrees
		FOV = cameraNode.verticalFieldOfView();
		FOV = FOV * (180.0 / 3.141592653589793238463);
	}


	bool updateCam = false;
	if (!isOrtographic && (camPosScene.x == camPos.x) && (camPosScene.y == camPos.y) && (camPosScene.z == camPos.z))
		updateCam = false;

	else
		updateCam = true;

	if (updateCam) {
		//Camera cameraInfo = {};
		Camera camData; 
		MsgHeader msgHeader;
		messageType msgType = {CMDTYPE::UPDATE_NODE, 1}; 

		// fill camInfo struct
		//CameraInfo mCamInfo = {};
		camData.fov  = FOV;
		camData.type = isOrtographic;

		camData.forward = { (float)COI.x, (float)COI.y, (float)COI.z };
		camData.up		= { (float)upDir.x, (float)upDir.y, (float)upDir.z };
		camData.pos		= { (float)camPos.x, (float)camPos.y, (float)camPos.z };


		msgHeader.nameLen = objName.length();
		msgHeader.msgSize = totalMsgSizeCamera;
		msgHeader.nodeType = NODE_TYPE::CAMERA;
		msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
		memcpy(msgHeader.objName, objName.c_str(), objName.length());

		/* 
		MStreamUtils::stdOutStream() << "\n"; 
		MStreamUtils::stdOutStream() << " =============================== \n";
		MStreamUtils::stdOutStream() << "isOrtographic: " << isOrtographic << "\n";
		MStreamUtils::stdOutStream() << "upDir: " << camData.up.x << " " << camData.up.y << " " << camData.up.z << "\n";
		MStreamUtils::stdOutStream() << "pos: " << camData.pos.x << " " << camData.pos.y << " " << camData.pos.z << "\n";
		
		MStreamUtils::stdOutStream() << "Forward: " << camData.forward.x << " " << camData.forward.y << " " << camData.forward.z << "\n";
		MStreamUtils::stdOutStream() << "COI: " << COI.x << " " << COI.y << " " << COI.z << "\n";
		*/

		size_t msgSize = sizeof(messageType) + totalMsgSizeCamera; 
		const char* msg = new char[msgSize];
		// copy over msg ======
		memcpy((char*)msg, &msgType, sizeof(messageType));
		memcpy((char*)msg + sizeof(messageType), &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(messageType) + sizeof(MsgHeader), &camData, sizeof(Camera));

		//send it
		if (comLib.send(msg, msgSize)) {
			//MStreamUtils::stdOutStream() << "activeCamera: Message sent" << "\n";
		}

		delete[]msg;
		camPosScene = camData.pos;
	}
	

}

// ==================================================================================
// ================================= LIGHT ==========================================
// ==================================================================================


// get light information for when a light is created 
void GetLightInfo(MFnPointLight &light) {

	MColor color;
	MStatus status;
	MVector position;
	float intensity		 = 0;
	LightInfo mLightInfo = {};

	color	  = light.color();
	intensity = light.intensity();

	MFnTransform parent(light.child(0), &status);
	if (status) 
		position = parent.getTranslation(MSpace::kObject, &status);
	
	// convert from 0-1 to 0-255
	int red   = color.r * 255;
	int blue  = color.b * 255;
	int green = color.g * 255;
	Color mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)255 };

	int lightIndex = -1;
	for (int i = 0; i < lightInScene.length(); i++) {
		if (lightInScene[i] == light.name()) {
			lightIndex = i;
			break;
		}
	}

	if (lightIndex == -1) {
		lightInScene.append(light.name());
		lightIndex = lightInScene.length() - 1;
	}

	if (lightIndex >= 0) {

		//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Light));

		mLightInfo.msgHeader.nodeType = NODE_TYPE::LIGHT;
		mLightInfo.msgHeader.msgSize  = totalMsgSizeLight;
		mLightInfo.msgHeader.cmdType  = CMDTYPE::NEW_NODE;
		mLightInfo.msgHeader.nameLen  = light.name().length();
		memcpy(mLightInfo.msgHeader.objName, light.name().asChar(), mLightInfo.msgHeader.nameLen);

		mLightInfo.lightData.lightNameLen = light.name().length();
		memcpy(mLightInfo.lightData.lightName, light.name().asChar(), mLightInfo.lightData.lightNameLen);

		mLightInfo.lightData.color	   = mColor;
		mLightInfo.lightData.intensity = intensity;
		mLightInfo.lightData.lightPos  = { (float)position.x, (float)position.y, (float)position.z };

		//mLightInfo.lightName	 = light.name();
		//mLightInfo.lightPathName = light.fullPathName();
		
		lightInfoToSend.push_back(mLightInfo);
	}
}

// callback to get light attributes 
void lightAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	if (msg & MNodeMessage::kAttributeSet) {

		std::string plugName	  = plug.name().asChar();
		std::string plugPartName  = plug.partialName().asChar();
		std::string otherPlugName = otherPlug.name().asChar();

		MColor color;
		MStatus result;
		MVector position;
		float intensity		 = 0;
		LightInfo mLightInfo = {};

		MDagPath path;
		MFnDagNode(plug.node()).getPath(path);

		bool msgToSend = false;
		MFnPointLight lightNode;
		MFnTransform lightTransf;

		// if plugname is color, light shape has directly been changed
		if (plugName.find("color") != std::string::npos) {

			lightNode.setObject(path);
			lightTransf.setObject(lightNode.parent(0));
			msgToSend = true;
		}

		//if plugname translate, rotate or scale, light transform has been changed
		else if (plugName.find("translate") != std::string::npos || plugName.find("rotate") != std::string::npos || plugName.find("scale") != std::string::npos) {

			lightTransf.setObject(path);
			lightNode.setObject(lightTransf.child(0));
			msgToSend = true;
		}

		// check if light is in scene. If not, add it
		int lightIndex = -1;
		for (int i = 0; i < lightInScene.length(); i++) {
			if (lightInScene[i] == lightNode.name()) {
				lightIndex = i;
				mLightInfo.msgHeader.cmdType = CMDTYPE::UPDATE_NODE;
				break;
			}
		}

		if (lightIndex == -1) {
			lightInScene.append(lightNode.name());
			lightIndex = lightInScene.length() - 1;
			mLightInfo.msgHeader.cmdType = CMDTYPE::NEW_NODE;
		}

	
		if (lightIndex >= 0 && msgToSend == true) {

			// get color, intensity and pos
			color	  = lightNode.color();
			intensity = lightNode.intensity();
			position  = lightTransf.getTranslation(MSpace::kObject, &status);

			// convert from 0-1 to 0-255
			int red   = color.r * 255;
			int blue  = color.b * 255;
			int green = color.g * 255;
			Color mColor = { (unsigned char)red, (unsigned char)green, (unsigned char)blue, (unsigned char)255 };

			//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(Light));

			mLightInfo.msgHeader.nodeType = NODE_TYPE::LIGHT;
			mLightInfo.msgHeader.msgSize  = totalMsgSizeLight;

			mLightInfo.msgHeader.nameLen = lightNode.name().length();
			memcpy(mLightInfo.msgHeader.objName, lightNode.name().asChar(), mLightInfo.msgHeader.nameLen);

			mLightInfo.lightData.lightNameLen = lightNode.name().length();
			memcpy(mLightInfo.lightData.lightName, lightNode.name().asChar(), mLightInfo.lightData.lightNameLen);

			mLightInfo.lightData.color	   = mColor;
			mLightInfo.lightData.intensity = intensity;
			mLightInfo.lightData.lightPos  = { (float)position.x, (float)position.y, (float)position.z };

			//mLightInfo.lightName	 = lightNode.name();
			//mLightInfo.lightPathName = lightNode.fullPathName();

			lightInfoToSend.push_back(mLightInfo);
		}
	}
}

// ==================================================================================
// ================================= OTHER ==========================================
// ==================================================================================

// callback for when a node's name has been changed
void nodeNameChanged(MObject& node, const MString& str, void* clientData) {

	/* 
	MStreamUtils::stdOutStream() << "=========================" << endl;
	MStreamUtils::stdOutStream() << "NAME CHANGED " << endl;
	*/

	std::string oldName;
	std::string newName;

	bool nodeToRename = false;
	NodeRenamedInfo mNodeRenamed = {};

	if (node.hasFn(MFn::kMesh)) {

		MFnMesh meshNode(node);
		oldName = str.asChar();
		newName = meshNode.name().asChar();

		int meshIndex = findMesh(str);
		if (meshIndex >= 0) {
			meshesInScene[meshIndex] = newName.c_str();
			mNodeRenamed.msgHeader.nodeType = NODE_TYPE::MESH;
			nodeToRename = true;
		}

	}

	else if (node.hasFn(MFn::kTransform)) {

		MFnTransform fransfNode(node);
		oldName = str.asChar();
		newName = fransfNode.name().asChar();

		for (int i = 0; i < transformsInScene.length(); i++) {
			if (transformsInScene[i] == oldName.c_str()) {
				
				transformsInScene[i] = newName.c_str();
				mNodeRenamed.msgHeader.nodeType = NODE_TYPE::TRANSFORM;
				nodeToRename = true;
				break; 
			}
		}

	
	}

	else if (node.hasFn(MFn::kCamera)) {

		MFnCamera camNode(node);
		oldName = str.asChar();
		newName = camNode.name().asChar();

		mNodeRenamed.msgHeader.nodeType = NODE_TYPE::CAMERA;
		nodeToRename = true;

	}

	else if (node.hasFn(MFn::kPointLight)) {

		MFnPointLight lightNode(node);
		oldName = str.asChar();
		newName = lightNode.name().asChar();

		for (int i = 0; i < lightInScene.length(); i++) {
			if (lightInScene[i] == oldName.c_str()) {

				lightInScene[i] = newName.c_str();
				mNodeRenamed.msgHeader.nodeType = NODE_TYPE::LIGHT;
				nodeToRename = true;
				break; 
			}
		}

	}

	else if (node.hasFn(MFn::kLambert)) {

		MFnDependencyNode materialNode(node);
		oldName = str.asChar();
		newName = materialNode.name().asChar();

		
		for (int i = 0; i < materialsInScene.length(); i++) {
			if (materialsInScene[i] == oldName.c_str()) {
				
				materialsInScene[i] = newName.c_str();
				mNodeRenamed.msgHeader.nodeType = NODE_TYPE::MATERIAL;
				nodeToRename = true;
				break; 
			}
		}

		
	}

	if (nodeToRename) {

		//size_t totalMsgSize = (sizeof(MsgHeader) + sizeof(NodeRenamed));

		mNodeRenamed.nodeOldName = oldName.c_str();
		mNodeRenamed.nodeNewName = newName.c_str();

		mNodeRenamed.msgHeader.nameLen = oldName.length();
		mNodeRenamed.msgHeader.msgSize = totalMsgSizeRenamed;
		mNodeRenamed.msgHeader.cmdType = CMDTYPE::UPDATE_NAME;
		memcpy(mNodeRenamed.msgHeader.objName, oldName.c_str(), mNodeRenamed.msgHeader.nameLen);

		mNodeRenamed.renamedInfo.nodeNameLen = newName.length();
		memcpy(mNodeRenamed.renamedInfo.nodeName, newName.c_str(), mNodeRenamed.renamedInfo.nodeNameLen);

		mNodeRenamed.renamedInfo.nodeOldNameLen = oldName.length();
		memcpy(mNodeRenamed.renamedInfo.nodeOldName, oldName.c_str(), mNodeRenamed.renamedInfo.nodeOldNameLen);

		nodeRenamedInfoToSend.push_back(mNodeRenamed);
	}

}

/* 
void nodeDuplicated(void* clientData) {


	
}
*/

// callback that goes through nodes that have been created and attatches callbacks
void nodeAdded(MObject &node, void * clientData) {

	MCallbackId tempID;
	MStatus Result = MS::kSuccess;

	// if the node has kMesh attributes it is a mesh
	if (node.hasFn(MFn::kMesh)) {

		// get mesh node and path
		MDagPath path;
		MFnDagNode(node).getPath(path);
		MFnMesh meshNode(node);

		// check if mesh is not intermediate
		// intermediate object is the object as it was before adding the deformers
		if (!meshNode.isIntermediateObject()) {
			
			// callback for whenever an attribute on the mesh is changed (ex vtx, normal, material)
			tempID = MNodeMessage::addAttributeChangedCallback(node, meshAttributeChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);

			// callback for whenever the topology is changed on a mesh (ex polysplit, extrude, bevel)
			tempID = MPolyMessage::addPolyTopologyChangedCallback(node, meshTopologyChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);

			// callback for when the world matrix is changed for a mesh
			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, meshWorldMatrixChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);

			// callback for when the name of a mesh is changed
			tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
			if (status == MS::kSuccess)
				callbackIdArray.append(tempID);


		}

	}

	// if the node has kTransform attributes it is a transform
	if (node.hasFn(MFn::kTransform)) {
		
		// get transform and path
		MDagPath path;
		MFnDagNode(node).getPath(path);
		MFnTransform transfNode(node);

		// add callback for when the world matrix is changed for a transform
		tempID = MDagMessage::addWorldMatrixModifiedCallback(path, transformWorldMatrixChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		// add callback for when the name of a transform is canged
		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

	}

	// if the node has kLambert attributes it is a lambert
	if (node.hasFn(MFn::kLambert)) {

		MObject lambertShader(node);

		// add callback for when an attribute has been changed on the material node
		tempID = MNodeMessage::addAttributeChangedCallback(lambertShader, materialAttributeChanged, NULL, &status);
		if (Result == MS::kSuccess)
			callbackIdArray.append(tempID);

		// add callback for when material name has changed
		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

	}

	// if the node has kPointLight attributes it is a pointLight
	if (node.hasFn(MFn::kPointLight)) {
		
		// get light node, path and parent transform of light
		MDagPath path;
		MFnDagNode(node).getPath(path);
		MFnPointLight lightNode(path);
		MObject parentNode(lightNode.parent(0));

		// add callback for when name is changed on light node
		tempID = MNodeMessage::addNameChangedCallback(node, nodeNameChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		// add callback for when attribute is changed on light node
		tempID = MNodeMessage::addAttributeChangedCallback(node, lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		// add callback for when attribute is changed on parent of light node
		tempID = MNodeMessage::addAttributeChangedCallback(parentNode, lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

	}
	
}

// callback that goes through nodes that have been deleted
void nodeDeleted(MObject &node, void *clientData) {

	NodeDeletedInfo deleteInfo = {};
	MFnDagNode dgNode(node);

	if (node.hasFn(MFn::kMesh)) {

		MFnMesh mesh(node);
		std::string meshName = mesh.name().asChar();

		int index = findMesh(mesh.name());
	
		if (index >= 0) {

			//fill msg info and send. Also remove mesh from scene
			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize  = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::MESH;
			deleteInfo.msgHeader.cmdType  = CMDTYPE::DELETE_NODE;
			deleteInfo.msgHeader.nameLen  = meshName.length();
			memcpy(deleteInfo.msgHeader.objName, meshName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName  = mesh.name();

			nodeDeleteInfoToSend.push_back(deleteInfo);
			meshesInScene.remove(index);

		}


	}

	else if (node.hasFn(MFn::kLambert)) {

		MStreamUtils::stdOutStream() << "MATERIAL DELETED " << endl;

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

			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize  = totalMsgSize;
			deleteInfo.msgHeader.nameLen  = matName.length();
			deleteInfo.msgHeader.nodeType = NODE_TYPE::MATERIAL;
			deleteInfo.msgHeader.cmdType = CMDTYPE::DELETE_NODE;
			memcpy(deleteInfo.msgHeader.objName, matName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName  = matName.c_str();

			nodeDeleteInfoToSend.push_back(deleteInfo);
			materialsInScene.remove(index);

		}

	}
	
	else if (node.hasFn(MFn::kPointLight)) {

		MFnPointLight lightNode(node);
		std::string lightName = lightNode.name().asChar();

		int index = -1;
		for (int i = 0; i < lightInScene.length(); i++) {
			if (lightInScene[i] == lightNode.name()) {
				index = i;
				break;
			}
		}

		if (index >= 0) {
			
			size_t totalMsgSize = (sizeof(MsgHeader));

			deleteInfo.msgHeader.msgSize  = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::LIGHT;
			deleteInfo.msgHeader.nameLen  = lightName.length();
			deleteInfo.msgHeader.cmdType  = CMDTYPE::DELETE_NODE;
			memcpy(deleteInfo.msgHeader.objName, lightName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName  = lightNode.name();

			lightInScene.remove(index);
			nodeDeleteInfoToSend.push_back(deleteInfo);
		}

	}



	/* 
	else if (node.hasFn(MFn::kCamera)) {

		MFnCamera camNode(node);
		std::string CamName = camNode.name().asChar();
	}

	else if (node.hasFn(MFn::kTransform)) {

		MFnTransform transfNode(node);
		std::string transfName = transfNode.name().asChar();
		
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

			deleteInfo.msgHeader.msgSize  = totalMsgSize;
			deleteInfo.msgHeader.nodeType = NODE_TYPE::TRANSFORM;
			deleteInfo.msgHeader.nameLen  = transfName.length();
			memcpy(deleteInfo.msgHeader.objName, transfName.c_str(), deleteInfo.msgHeader.nameLen);

			deleteInfo.nodeIndex = index;
			deleteInfo.nodeName  = transfNode.name();
			

			transformsInScene.remove(index);
			//nodeDeleteInfoToSend.push_back(deleteInfo);
		}
	}

	
	*/

	callbackIdArray.remove(MNodeMessage::currentCallbackId());
}

// callback called every x seconds 
void timerCallback(float elapsedTime, float lastTime, void* clientData) {

	/*
   MStreamUtils::stdOutStream() << "\n";
   MStreamUtils::stdOutStream() << " ========== MESHES IN SCENE ========== \n";
   for (int i = 0; i < meshesInScene.length(); i++) {
	   MStreamUtils::stdOutStream() << meshesInScene[i] << "\n";

   }
   MStreamUtils::stdOutStream() << "\n";
	*/
	globalTime += elapsedTime;

	// send all matrix related messages
	int matrixMsgLen = matrixInfoToSend.size(); 
	if (matrixMsgLen > 0) {
			
		messageType msgType = { CMDTYPE::UPDATE_MATRIX , matrixMsgLen };
		size_t totalMsgSize = sizeof(messageType) + totalMsgSizeMatrix * matrixMsgLen;

		const char* msgChar = new char[totalMsgSize];
		memcpy((char*)msgChar, &msgType, sizeof(messageType));

		int msgOffset = 0; 
		for (int i = matrixInfoToSend.size() - 1; i >= 0; i--) {

			memcpy((char*)msgChar + sizeof(messageType) + (totalMsgSizeMatrix * msgOffset), &matrixInfoToSend[i].msgHeader, sizeof(MsgHeader));
			memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + (totalMsgSizeMatrix * msgOffset), &matrixInfoToSend[i].matrixData, sizeof(Matrix));

			msgOffset += 1;
			matrixInfoToSend.erase(matrixInfoToSend.begin() + i);
		}

		// send it
		if (comLib.send(msgChar, totalMsgSize)) {
			//MStreamUtils::stdOutStream() << "matrixInfo: Message sent" << "\n";
		}

		delete[] msgChar;
	}

	// send all messages with material infromation
	int matInfoSize = materialInfoToSend.size(); 
	if (matInfoSize > 0) {

		for (int i = materialInfoToSend.size() - 1; i >= 0; i--) {

			MStreamUtils::stdOutStream() << "=================== \n";

			messageType msgType = { CMDTYPE::UPDATE_MATERIAL, 1};
			if (materialInfoToSend[i].msgHeader.cmdType == CMDTYPE::NEW_MATERIAL) {
				msgType.cmdType = CMDTYPE::NEW_MATERIAL; 
			}

			


			size_t totalMsgSize = sizeof(messageType) + totalMsgSizeMaterial;
			const char* msgChar = new char[totalMsgSize];

			memcpy((char*)msgChar, &msgType, sizeof(messageType));
			memcpy((char*)msgChar + sizeof(messageType), &materialInfoToSend[i].msgHeader, sizeof(MsgHeader));
			memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader), &materialInfoToSend[i].materialData, sizeof(Material));

			
			// send it
			if (comLib.send(msgChar, totalMsgSize)) {
				//MStreamUtils::stdOutStream() << "matInfo: Message sent" << "\n";
			}

			delete[] msgChar;
			materialInfoToSend.erase(materialInfoToSend.begin() + i);
		}

	}

	// send all messages with mesh information
	int meshMsgSize = meshInfoToSend.size(); 
	if (meshMsgSize > 0) {

		for (int i = meshInfoToSend.size() - 1; i >= 0; i--) {


			if (meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::UPDATE_NODE) {

				messageType msgType = { CMDTYPE::UPDATE_NODE, 1 };

				size_t totalMsgSize = sizeof(messageType) + meshInfoToSend[i].msgHeader.msgSize;
				const char* msgChar = new char[totalMsgSize];

				// copy over msg ======
				memcpy((char*)msgChar, &msgType, sizeof(messageType));

				memcpy((char*)msgChar + sizeof(messageType), &meshInfoToSend[i].msgHeader, sizeof(MsgHeader));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader), &meshInfoToSend[i].meshData, sizeof(Mesh));

				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh), meshInfoToSend[i].meshVtx, (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3), meshInfoToSend[i].meshNrms, (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3), meshInfoToSend[i].meshUVs, sizeof(float) * meshInfoToSend[i].meshData.UVcount);

				//send it
				if (comLib.send(msgChar, meshInfoToSend[i].msgHeader.msgSize)) {
					//MStreamUtils::stdOutStream() << "meshInfoToSend: Message new/update node sent" << "\n";
				}

				//MStreamUtils::stdOutStream() << "=======================================\n";

				// delete allocated arrays + message
				delete[] msgChar;
				delete[] meshInfoToSend[i].meshVtx;
				delete[] meshInfoToSend[i].meshUVs;
				delete[] meshInfoToSend[i].meshNrms;

			}

			if (meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::NEW_NODE) {

				messageType msgType = { CMDTYPE::NEW_NODE, 1};

				size_t totalMsgSize = sizeof(messageType) + meshInfoToSend[i].msgHeader.msgSize; 
				const char* msgChar = new char[totalMsgSize];

				// copy over msg ======
				memcpy((char*)msgChar, &msgType, sizeof(messageType));

				memcpy((char*)msgChar + sizeof(messageType), &meshInfoToSend[i].msgHeader, sizeof(MsgHeader));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader), &meshInfoToSend[i].meshData, sizeof(Mesh));

				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh), meshInfoToSend[i].meshVtx, (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3), meshInfoToSend[i].meshNrms, (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3), meshInfoToSend[i].meshUVs, sizeof(float) * meshInfoToSend[i].meshData.UVcount);

				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.UVcount), &meshInfoToSend[i].materialData, sizeof(Material));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh) + (sizeof(float) * meshInfoToSend[i].meshData.vtxCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.normalCount * 3) + (sizeof(float) * meshInfoToSend[i].meshData.UVcount) + sizeof(Material), &meshInfoToSend[i].transformMatrix, sizeof(Matrix));
				

				//send it
				if (comLib.send(msgChar, meshInfoToSend[i].msgHeader.msgSize)) {
					//MStreamUtils::stdOutStream() << "meshInfoToSend: Message new/update node sent" << "\n";
				}

				//MStreamUtils::stdOutStream() << "=======================================\n";

				// delete allocated arrays + message
				delete[] msgChar;
				delete[] meshInfoToSend[i].meshVtx;
				delete[] meshInfoToSend[i].meshUVs;
				delete[] meshInfoToSend[i].meshNrms;

			}

			else if (meshInfoToSend[i].msgHeader.cmdType == CMDTYPE::UPDATE_NODE_MATERIAL) {

				messageType msgType = { CMDTYPE::UPDATE_NODE_MATERIAL, 1 };

				size_t totMsgSize = sizeof(messageType) + meshInfoToSend[i].msgHeader.msgSize; 
				const char* msgChar = new char[totMsgSize];


				// copy over msg ======
				memcpy((char*)msgChar, &msgType, sizeof(messageType));
				memcpy((char*)msgChar + sizeof(messageType), &meshInfoToSend[i].msgHeader, sizeof(MsgHeader));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader), &meshInfoToSend[i].meshData, sizeof(Mesh));
				memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + sizeof(Mesh), &meshInfoToSend[i].materialData, sizeof(Material));

				
				//send it
				if (comLib.send(msgChar, totMsgSize)) {
					//MStreamUtils::stdOutStream() << "meshInfoToSend: Update material on node sent" << "\n";
				}

				delete[] msgChar;
			}


			meshInfoToSend.erase(meshInfoToSend.begin() + i);

		}

	}


	// send all messages with rename information
	int renameMsgSize = nodeRenamedInfoToSend.size();
	if (renameMsgSize > 0) {

		messageType msgType = { CMDTYPE::UPDATE_NAME, renameMsgSize };
		size_t totalMsgSize = sizeof(messageType) + totalMsgSizeRenamed * renameMsgSize;

		const char* msgChar = new char[totalMsgSize];
		memcpy((char*)msgChar, &msgType, sizeof(messageType));

		int msgOffset = 0; 
		for (int i = 0; i < nodeRenamedInfoToSend.size(); i++) {


			memcpy((char*)msgChar + sizeof(messageType) + (totalMsgSizeRenamed * msgOffset), &nodeRenamedInfoToSend[i].msgHeader, sizeof(MsgHeader));
			memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader) + (totalMsgSizeRenamed * msgOffset), &nodeRenamedInfoToSend[i].renamedInfo, sizeof(NodeRenamed));


			msgOffset++; 
			nodeRenamedInfoToSend.erase(nodeRenamedInfoToSend.begin() + i);
		}

		//send it
		if (comLib.send(msgChar, totalMsgSize)) {
			//MStreamUtils::stdOutStream() << "nodeRenamed: Message sent" << "\n";
		}

		delete[]msgChar;
	}
		
	// send all messages with delete information
	int deleteMsgSize = nodeDeleteInfoToSend.size(); 
	if (deleteMsgSize > 0) {

		messageType msgType = { CMDTYPE::DELETE_NODE, deleteMsgSize };
		size_t totalMsgSize = sizeof(messageType) + sizeof(MsgHeader) * deleteMsgSize;

		const char* msgChar = new char[totalMsgSize];
		memcpy((char*)msgChar, &msgType, sizeof(messageType));

		MStreamUtils::stdOutStream() << "deleteMsgSize " << deleteMsgSize << "\n";

		int msgOffset = 0;
		for (int i = nodeDeleteInfoToSend.size() - 1; i >= 0; i--) {

			MStreamUtils::stdOutStream() << "MEEESH " << nodeDeleteInfoToSend[i].nodeName << "\n";

			memcpy((char*)msgChar + sizeof(messageType) + (sizeof(MsgHeader) * msgOffset), &nodeDeleteInfoToSend[i].msgHeader, sizeof(MsgHeader));

			msgOffset++; 
			nodeDeleteInfoToSend.erase(nodeDeleteInfoToSend.begin() + i);
		}

		if (comLib.send(msgChar, totalMsgSize)) {
			//MStreamUtils::stdOutStream() << "nodeDeleted: Message sent for " << nodeDeleteInfoToSend[i].nodeName << "\n";
		}
		delete[] msgChar;
	}

	/* 
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
	*/

	
	// send all light information
	int lightMsgSize = lightInfoToSend.size();
	if (lightMsgSize > 0) {


		for (int i = 0; i < lightInfoToSend.size(); i++) {

			messageType msgType = { CMDTYPE::UPDATE_NODE, 1 };
			if (lightInfoToSend[i].msgHeader.cmdType == CMDTYPE::NEW_NODE)
				msgType.cmdType = CMDTYPE::NEW_NODE;

			size_t totalMsgSize = sizeof(messageType) + totalMsgSizeLight ;
			const char* msgChar = new char[totalMsgSize];
			memcpy((char*)msgChar, &lightMsgSize, sizeof(int));

			memcpy((char*)msgChar + sizeof(messageType), &lightInfoToSend[i].msgHeader, sizeof(MsgHeader));
			memcpy((char*)msgChar + sizeof(messageType) + sizeof(MsgHeader), &lightInfoToSend[i].lightData, sizeof(Light));

			//send it
			if (comLib.send(msgChar, totalMsgSize)) {
				//MStreamUtils::stdOutStream() << "light: Message sent" << "\n";
			}
		
			delete[] msgChar;
			lightInfoToSend.erase(lightInfoToSend.begin() + i);

		}

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

	
	MItDependencyNodes lightIterator(MFn::kPointLight);
	while (!lightIterator.isDone()) {

		MFnPointLight light(lightIterator.item());
		GetLightInfo(light);

		tempID = MNodeMessage::addAttributeChangedCallback(lightIterator.thisNode(), lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		tempID = MNodeMessage::addAttributeChangedCallback(light.parent(0), lightAttributeChanged, NULL, &status);
		if (status == MS::kSuccess)
			callbackIdArray.append(tempID);

		lightIterator.next();
	}
	

	// callbacks for all the 4 model panels used for camera 
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

	InitializeScene();

	// register callbacks here for
	tempID = MDGMessage::addNodeAddedCallback(nodeAdded);
	callbackIdArray.append(tempID);

	tempID = MDGMessage::addNodeRemovedCallback(nodeDeleted);
	callbackIdArray.append(tempID);

	//tempID = MModelMessage::addAfterDuplicateCallback(nodeDuplicated);
	//callbackIdArray.append(tempID);

	tempID = MTimerMessage::addTimerCallback(timerPeriod, timerCallback, NULL, &status);
	callbackIdArray.append(tempID);

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

