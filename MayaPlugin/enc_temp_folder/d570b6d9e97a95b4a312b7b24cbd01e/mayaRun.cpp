#include "maya_includes.h"

#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>

//#include <C:\Users\BTH\Desktop\SharedBuff-master\shared\Project1\ComLib.h>
#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>

using namespace std;

#pragma comment(lib, "Project1.lib")

ComLib ourComLib("buffer2", 50, PRODUCER);

using namespace std;
MCallbackIdArray callbackIdArray;
MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

enum NODE_TYPE {
	TRANSFORM,
	MESH,
	LIGHT,
	CAMERA
};

enum CMDTYPE {
	DEFAULT = 1000,
	NEW_NODE = 1001,
	UPDATE_NODE = 1002,
	UPDATE_MATRIX = 1003,
	UPDATE_NAME = 1004

};

struct MsgHeader {
	CMDTYPE	  cmdType;
	NODE_TYPE nodeType;
	char objName[64];
	int msgSize;
	int nameLen;
};

struct Mesh {
	int vtxCount;
	int trisCount; 
	int normalCount;
	int UVCount; 
};


MTimer gTimer;
float globalTime = 0;

// keep track of created meshes to maintain them
queue<MObject> newMeshes;
queue<MObject> newLights;


bool sendMsg(CMDTYPE msgType, NODE_TYPE nodeT, int nrOfElements, int trisCount, int normalCount, int UVCount, std::string objName, std::string &msgString) {

	//MGlobal::displayInfo(MString("nrOfElements: ") + nrOfElements + "\n");
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

		Mesh mesh;
		mesh.trisCount   = trisCount;
		mesh.vtxCount	 = nrOfElements;
		mesh.normalCount = normalCount; 
		mesh.UVCount	 = UVCount;

		//MGlobal::displayInfo(MString("TRIS:  ") + mesh.trisCount + "\n");

		// Copy MSG ================== 
		size_t totalMsgSize = (sizeof(MsgHeader) + msgHeader.msgSize + sizeof(Mesh));
		char* msg = new char[totalMsgSize];

		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), &mesh, sizeof(Mesh));

		memcpy((char*)msg + sizeof(MsgHeader) + sizeof(Mesh), msgString.c_str(), msgHeader.msgSize);

		//MStreamUtils::stdOutStream() << "msgString: " << msgString << "_" << endl;

		sent = ourComLib.send(msg, totalMsgSize);

		// =========================== 
		delete[]msg;
	}

	else {

		// Copy MSG ================== 
		size_t totalMsgSize = (sizeof(MsgHeader) + msgHeader.msgSize);
		char* msg = new char[totalMsgSize];

		memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
		memcpy((char*)msg + sizeof(MsgHeader), msgString.c_str(), msgHeader.msgSize);

		sent = ourComLib.send(msg, totalMsgSize);
		// =========================== 
		delete[]msg;
	}

	return sent;
}

//sending 1/2
void nodeLightAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnLight sceneLight(path);

	MPlug colorPlug = sceneLight.findPlug("color");
	if (plug.name() == colorPlug.name())
	{
		MStreamUtils::stdOutStream() << "Light Color: " << sceneLight.color() << endl;
	}

	MPlug intensityPlug = sceneLight.findPlug("intensity");
	if (intensityPlug.name() == plug.name())
	{
		MStreamUtils::stdOutStream() << "Light intensity: " << sceneLight.intensity() << endl;
	}

	//add command + name to string
	std::string objName = sceneLight.name().asChar();

	//MVector to string
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
		sendMsg(CMDTYPE::UPDATE_NODE, NODE_TYPE::LIGHT, 0, 0, 0, 0, objName, msgString);
	}
}

//sending NOT WORKING
void activeCamera(const MString &panelName, void* cliendData) {

	MStatus status;
	std::string objName = "camera";

	int height;
	int width;

	M3dView activeView;
	MStatus result = M3dView::getM3dViewFromModelPanel(panelName, activeView);
	if (result != MS::kSuccess) {
		MStreamUtils::stdOutStream() << "Did not get active 3DView" << endl;
		MStreamUtils::stdOutStream() << "Error: " << result << endl;
		return;
	}

	height = activeView.portHeight();
	width = activeView.portWidth();

	MDagPath camDag;
	activeView.getCamera(camDag);
	MString camName = camDag.fullPathName();

	//MMatrix worldMatrix = MMatrix(camDag.inclusiveMatrix());

	MFnCamera cameraView(camDag);
	MVector viewDir = cameraView.viewDirection(MSpace::kWorld, &status);
	MVector upDir = cameraView.upDirection(MSpace::kWorld, &status);
	MVector rightDir = cameraView.rightDirection(MSpace::kWorld, &status);
	MPoint camPos = cameraView.eyePoint(MSpace::kWorld, &status);

	double horizontalFOV;
	double verticalFOV;
	cameraView.getPortFieldOfView(width, height, horizontalFOV, verticalFOV);

	//MMatrix modelViewMat;
	//activeView.modelViewMatrix(modelViewMat);

	//MMatrix projectionViewMat;
	//activeView.projectionMatrix(projectionViewMat);

	/*
	MStreamUtils::stdOutStream() << "==================================================" << endl;
	if (status = MS::kSuccess) {
		//MStreamUtils::stdOutStream() << "X: " << x << " : "  << "Y: " << y << endl;
		//MStreamUtils::stdOutStream() << "worldPoint: " << worldPoint << endl;
		//MStreamUtils::stdOutStream() << "worldVector: " << worldVector << endl;
		MStreamUtils::stdOutStream() << camName << endl;
		MStreamUtils::stdOutStream() << "Target: "  << viewDir << endl;
		MStreamUtils::stdOutStream() << "upDir: "    << upDir << endl;
		MStreamUtils::stdOutStream() << "rightDir: " << rightDir << endl;
		MStreamUtils::stdOutStream() << "CamPos: "	 << camPos << endl;
		MStreamUtils::stdOutStream() << "verticalFOV: " << verticalFOV << endl;
	}
	MStreamUtils::stdOutStream() << " " << endl;
	MStreamUtils::stdOutStream() << cameraView.name() << endl;
	MStreamUtils::stdOutStream() << " " << endl;
	MStreamUtils::stdOutStream() << camName << endl;
	MStreamUtils::stdOutStream() << " " << endl;
	MStreamUtils::stdOutStream() << "Model view mat: " << modelViewMat << endl;
	MStreamUtils::stdOutStream() << " " << endl;
	MStreamUtils::stdOutStream() << "Projection mat: " <<projectionViewMat << endl;
	MStreamUtils::stdOutStream() << "==================================================" << endl;
	*/

	//MVector to string
	std::string msgString;
	msgString.append(camName.asChar());
	msgString.append(" ");
	msgString.append(std::to_string(upDir.x) + " " + std::to_string(upDir.y) + " " + std::to_string(upDir.z) + " ");
	msgString.append(std::to_string(viewDir.x) + " " + std::to_string(viewDir.y) + " " + std::to_string(viewDir.z) + " ");
	msgString.append(std::to_string(camPos.x) + " " + std::to_string(camPos.y) + " " + std::to_string(camPos.z) + " ");
	msgString.append(std::to_string(verticalFOV));

	//MStreamUtils::stdOutStream() << "MSG:" + msgString  << endl;


	/*
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			msgString.append(to_string(modelViewMat(i, j)) + " ");
		}
	}
	 */

	 //pass to send
	bool msgToSend = false;
	if (msgString.length() > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(CMDTYPE::UPDATE_NODE, NODE_TYPE::CAMERA, 0, 0, 0, 0, objName, msgString);
	}
}

void nodeTextureAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MObject texObj(plug.node());

	MFnDependencyNode textureNode(texObj);
	MPlug fileTextureName = textureNode.findPlug("ftn");

	if (plug.name() == fileTextureName.name())
	{
		MString fileName;
		fileTextureName.getValue(fileName);
		MStreamUtils::stdOutStream() << fileName << endl;

	}
}

void nodeMaterialAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{

	MObject lamObj(plug.node());
	MFnDependencyNode lambertDepNode(lamObj);

	bool hasTexture = false;

	MPlug colorPlug = lambertDepNode.findPlug("color");
	//get lambert.color

	MPlugArray connetionsColor;
	colorPlug.connectedTo(connetionsColor, true, false);

	for (int x = 0; x < connetionsColor.length(); x++)
	{
		if (connetionsColor[x].node().apiType() == MFn::kFileTexture)
		{
			MObject textureObj(connetionsColor[x].node());
			MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(textureObj, nodeTextureAttributeChanged);
			callbackIdArray.append(tempID);

			hasTexture = true;
		}
	}

	if (hasTexture == false)
	{
		if (plug.name() == colorPlug.name())
		{
			MFnLambertShader lambertItem(lamObj);

			MColor color;
			MPlug attr;

			attr = lambertItem.findPlug("colorR");
			attr.getValue(color.r);
			attr = lambertItem.findPlug("colorG");
			attr.getValue(color.g);
			attr = lambertItem.findPlug("colorB");
			attr.getValue(color.b);

			MStreamUtils::stdOutStream() << "color: " << color.r << ", " << color.g << ", " << color.b << endl;
		}
	}
}

//sending 1/3
void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	//////////////////////////
	//						//
	//			VTX			//
	//						//
	//////////////////////////
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

			MVector tempPoint = { x, y, z };
			vtxArrayMessy.append(tempPoint);
		}
	}

	//sort vtx correctly
	MIntArray triCount;
	MIntArray triVertsIndex;
	mesh.getTriangles(triCount, triVertsIndex);

	MVectorArray trueVtxForm;
	for (int i = 0; i < triVertsIndex.length(); i++) {
		trueVtxForm.append(vtxArrayMessy[triVertsIndex[i]]);
	}

	std::string objName = mesh.name().asChar();
	int totalTrisCount = triCount.length() * 2;

	//MVector to string
	std::string vtxArrayString;
	size_t vtxArrElements = 0;

	int vtxCount = trueVtxForm.length();
	vtxArrayString.append(to_string(vtxCount) + " ");

	for (int u = 0; u < trueVtxForm.length(); u++) {
		for (int v = 0; v < 3; v++) {
			vtxArrayString.append(to_string(trueVtxForm[u][v]) + " ");
			vtxArrElements++;
		}
	}

	/* 

	/////////////
	// NORMALS //
	/////////////

	MStreamUtils::stdOutStream() << "================================= " << endl;

	MFloatVectorArray normals;
	MFloatVectorArray initialNormals;
	mesh.getNormals(normals, MSpace::kWorld);

	MIntArray normCounts;
	MIntArray triNormIndex;
	//mesh.getNormalIds(normCounts, triNormIndex);

	MIntArray vertexCount;
	MIntArray vertexList;
	mesh.getVertices(vertexCount, vertexList);

	int currentIndex = 0;
	MFloatVectorArray orderedNormals;
	for (int faceCnt = 0; faceCnt < vertexCount.length(); ++faceCnt) {
		for (int faceVtxCnt = 0; faceVtxCnt < vertexCount[faceCnt]; ++faceVtxCnt) {

			MVector tempNorm = normals[triNormIndex[currentIndex]];
			orderedNormals.append(tempNorm);
			++currentIndex;
		}
	}


	MStreamUtils::stdOutStream() << "====================== " << endl;
	MStreamUtils::stdOutStream() << "orderedNormals.len: " << orderedNormals.length() << endl;
	MStreamUtils::stdOutStream() << "orderedNormals: "	   << orderedNormals << endl;


	std::string nmlArrayString;

	int nrmlCount = orderedNormals.length();
	vtxArrayString.append(to_string(nrmlCount) + " ");

	for (int u = 0; u < orderedNormals.length(); u++) {
		for (int v = 0; v < 3; v++) {
			nmlArrayString.append(std::to_string(orderedNormals[u][v]) + " ");
		}
	}

	*/

	///////////////
			//// NORMALS //
			///////////////

			//MStreamUtils::stdOutStream() << "================================= " << endl;

	MFloatVectorArray normals;
	//MFloatVectorArray initialNormals;
	mesh.getNormals(normals, MSpace::kWorld);


	//MIntArray vertexCount;
	//MIntArray vertexList;
	//mesh.getVertices(vertexCount, vertexList);

	MStreamUtils::stdOutStream() << "================" << endl;
	MStreamUtils::stdOutStream() << triCount.length() << endl;

	//MIntArray normalCounts;
	MIntArray normalIds;
	//mesh.getNormalIds(normalCounts, normalIds);

	MStreamUtils::stdOutStream() << "==========================" << endl;
	MStreamUtils::stdOutStream() << "TRI count: " << triCount.length() << endl;

	int currentVtx = 0;
	MIntArray tempNormalIds;
	int nrOfFaces = triCount.length();

	for (int faceCnt = 0; faceCnt < nrOfFaces; faceCnt++) {

		mesh.getFaceNormalIds(faceCnt, tempNormalIds);

		//MStreamUtils::stdOutStream() << "triVertsIndex[faceCnt]: " << faceCnt << endl;
		//MStreamUtils::stdOutStream() << "tempNormalIds: " << tempNormalIds << endl;
		//MStreamUtils::stdOutStream() << "tempNormalIds[0]: " << tempNormalIds[0] << endl;
		//MStreamUtils::stdOutStream() << "tempNormalIds[1]: " << tempNormalIds[1] << endl;
		//MStreamUtils::stdOutStream() << "tempNormalIds[2]: " << tempNormalIds[2] << endl;

		//MStreamUtils::stdOutStream() << " " << endl;
		normalIds.append(tempNormalIds[currentVtx]);

	}

	//MStreamUtils::stdOutStream() << "===============" << endl;
	//MStreamUtils::stdOutStream() << "normalIds.len: " << normalIds.length() << endl;
	//MStreamUtils::stdOutStream() << "normalIds: "     << normalIds			<< endl;
	//MStreamUtils::stdOutStream() << "normals.len: "   << normals.length()   << endl;
	//MStreamUtils::stdOutStream() << "normalIds.len: " << normalIds.length() << endl;
	//MStreamUtils::stdOutStream() << "nromals: " << normals << endl;
	//MStreamUtils::stdOutStream() << " " << endl;


	int vtxFace = 0;
	MVectorArray orderedNormals;

	for (int i = 0; i < nrOfFaces; i++) {
		//MStreamUtils::stdOutStream() << "normalIds at faceCnt: " << normalIds[vtxFace] << endl;			
		//MStreamUtils::stdOutStream() << "normals: " << normals[normalIds[vtxFace]] << endl;

		for (int j = 0; j < 3; j++) {
			//MStreamUtils::stdOutStream() << "normals: " << normals[normalIds[vtxFace]] << endl;
			orderedNormals.append(normals[normalIds[vtxFace]]);
		}


		//MStreamUtils::stdOutStream() << " " << endl;
		vtxFace++;

		//for (int vtxCnt = 0; vtxCnt < vtxCount; vtxCnt++) {
		//}
	}


	MStreamUtils::stdOutStream() << "orderedNormals.len: " << orderedNormals.length() << endl;
	MStreamUtils::stdOutStream() << "orderedNormals: " << endl;
	MStreamUtils::stdOutStream() << orderedNormals << endl;



	/*
	MVectorArray orderedNorm;
	for (int i = 0; i < normalIds.length(); i++) {
		int vtxInd = normalIds[i];
		//MStreamUtils::stdOutStream() << "triVertsIndex: " << triVertsIndex[vtxInd] << endl;
		//MStreamUtils::stdOutStream() << "normals[vtxInd]: " << normals[vtxInd] << endl;

		orderedNorm.append(normals[vtxInd]);
	}

	MStreamUtils::stdOutStream() << "orderedNorm: " << orderedNorm << endl;


	//MVector to string
	std::string nrmlArrayString;
	size_t nrmlArrElements = 0;

	int nrmlCount = orderedNorm.length();
	nrmlArrayString.append(to_string(nrmlCount) + " ");

	for (int u = 0; u < orderedNorm.length(); u++) {
		for (int v = 0; v < 3; v++) {
			nrmlArrayString.append(std::to_string(orderedNorm[u][v]) + " ");
		}
	}
	*/


	/*
	//for (int j = 0; j < normalIds.length(); j++) {

	//	//MStreamUtils::stdOutStream() << "normals: " << normals[j] << endl;
	//	int normIndex = normalIds[j];
	//	MStreamUtils::stdOutStream() << "Normal index: " << normIndex << endl;

	//	for (int i = 0; i < 3; i++) {
	//		MStreamUtils::stdOutStream() << "Normal at index: " << normals[normalIds[normIndex]] << endl;
	//	}

	//	MStreamUtils::stdOutStream() << " " << endl;


	//	//orderedNormals.append(normals[normalIds[j]]);

	}
	 */


	 /*

	 MStreamUtils::stdOutStream() << "======================== " << endl;

	 MStreamUtils::stdOutStream() << "nrElements: "  << nrElements << endl;
	 MStreamUtils::stdOutStream() << "nrOfNormals: " << nrOfNormals << endl;

	 if (nrElements == nrOfNormals) {
		 MStreamUtils::stdOutStream() << "IN NORMAL!" << endl;
		 status = MS::kFailure;

		 MIntArray normCounts;
		 MIntArray triNormIndex;
		 MVectorArray normalsArray;

		 mesh.getNormalIds(normCounts, triNormIndex)

		 for (int i = 0; i < triNormIndex.length(); i++) {
			 normalsArray.append(normals[triNormIndex[i]]);
		 }
		 */


		 /*MStreamUtils::stdOutStream() << "normalsArray: " << normalsArray << endl;

				 int nrNormals = triNormIndex.length();
				 MVector to string
				 std::string NormArrayString;
				 NormArrayString.append(to_string(nrOfNormals) + " ");
				 size_t normArrElements = 0;
			 for (int u = 0; u < normalsArray.length(); u++)
			 {
				 for (int v = 0; v < 3; v++)
				 {
					 NormArrayString.append(to_string(normalsArray[u][v]) + " ");
					 normArrElements++;
				 }
			 }
			 MStreamUtils::stdOutStream() << "NormArrayString: " << NormArrayString << "_" << endl;

		 */

	std::string NormArrayString;
	size_t nrOfNormals = orderedNormals.length();
	NormArrayString.append(to_string(nrOfNormals) + " ");

	for (int u = 0; u < orderedNormals.length(); u++) {
		for (int v = 0; v < 3; v++) {
			NormArrayString.append(to_string(orderedNormals[u][v]) + " ");
		}
	}

	//MVectorArray normalsArray;
	//MFloatVectorArray normals;
	//mesh.getNormals(normals, MSpace::kWorld);

	/*
	MIntArray normCount;
	MIntArray triNormIndex;
	mesh.getNormalIds(normCount, triNormIndex);

	int nrOfNormals = normals.length();

	for (int i = 0; i < triNormIndex.length(); i++) {
		normalsArray.append(normals[triNormIndex[i]]);
	}
	int nrNormals = triNormIndex.length();

	std::string NormArrayString;
	NormArrayString.append(to_string(nrNormals) + " ");
	size_t normArrElements = 0;

	for (int u = 0; u < normalsArray.length(); u++)
	{
		for (int v = 0; v < 3; v++)
		{
			NormArrayString.append(to_string(normalsArray[u][v]) + " ");
			normArrElements++;
		}
	}
	*/

	//std::string masterTransformString;
	//masterTransformString.append(vtxArrayString + " ");
	//masterTransformString.append(NormArrayString);


	//////////////////////////
	//						//
	//			UVS			//
	//						//
	//////////////////////////

	MStringArray texCoordNames;
	mesh.getUVSetNames(texCoordNames);
	//MStreamUtils::stdOutStream() << "UV set names: " + texCoordNames[0] << endl;

	MString* texCoordSet = &texCoordNames[0];
	MFloatArray texCoordArrU;
	MFloatArray texCoordArrV;
	mesh.getUVs(texCoordArrU, texCoordArrV, texCoordSet);

	int nrOfUVs = mesh.numUVs();

	MString nrOfUVsString("");
	nrOfUVsString += nrOfUVs;

	//MStreamUtils::stdOutStream() << "Nr of UVs: " + nrOfUVsString << endl;

	for (int i = 0; i < nrOfUVs; i++) {

		float texCoordU = texCoordArrU[i];
		float texCoordV = texCoordArrV[i];

		MString texCoordString("");
		texCoordString += texCoordU;
		texCoordString += ", ";
		texCoordString += texCoordV;

		//MStreamUtils::stdOutStream() << "UV: " + texCoordString << endl;
	}

	std::string msgString = ""; 
	msgString.append(vtxArrayString + " "); 
	msgString.append(NormArrayString);

	//pass to send
	bool msgToSend = false;
	if (vtxArrElements > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(CMDTYPE::UPDATE_NODE, NODE_TYPE::MESH, nrElements, totalTrisCount, nrOfNormals, nrOfUVs, objName, msgString);
	}
	


}

//sending
void nodeNameChangedMaterial(MObject &node, const MString &str, void*clientData) {

	// Get the new name of the node, and use it from now on.
	MString oldName = str;
	MString newName = MFnDagNode(node).name();
	MStreamUtils::stdOutStream() << "old name:" + oldName + ". New name: " + newName << endl;

	std::string objName = oldName.asChar();
	std::string msgString = newName.asChar();

	//pass to send
	bool msgToSend = false;
	if (msgString.length() > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(CMDTYPE::UPDATE_NAME, NODE_TYPE::MESH, 0, 0, 0, 0, objName, msgString);
	}
}

//we do not support multiply lights yet and therefore we do not have to send light name
void nodeNameChangedLight(MObject &node, const MString &str, void*clientData) {

	// Get the new name of the node, and use it from now on.
	MString oldName = str;
	MString newName = MFnDagNode(node).name();
	MStreamUtils::stdOutStream() << "old name:" + oldName + ". New name: " + newName << endl;

	std::string objName = oldName.asChar();
	std::string msgString = newName.asChar();

	//pass to send
	bool msgToSend = false;
	if (msgString.length() > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(CMDTYPE::UPDATE_NAME, NODE_TYPE::LIGHT, 0, 0, 0, 0, objName, msgString);
	}
}

//sending
void nodeWorldMatrixChanged(MObject &node, MDagMessage::MatrixModifiedFlags &modified, void *clientData)
{
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	//MStreamUtils::stdOutStream() << "MATRIX STUFF NAME: " << mesh.name().asChar() << endl;

	MMatrix localMatrix = MMatrix(path.exclusiveMatrix());
	MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

	std::string translateVector;
	translateVector.append(to_string(worldMatrix(3, 0)) + " ");
	translateVector.append(to_string(worldMatrix(3, 1)) + " ");
	translateVector.append(to_string(worldMatrix(3, 2)) + " ");

	std::string objName = mesh.name().asChar();
	std::string msgString = translateVector;

	//pass to send
	bool msgToSend = false;
	if (msgString.length() > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(CMDTYPE::UPDATE_MATRIX, NODE_TYPE::MESH, 0, 0, 0, 0, objName, msgString);
	}
}

//sending
void nodeWorldMatrixChangedLight(MObject &node, MDagMessage::MatrixModifiedFlags &modified, void *clientData)
{
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	//MStreamUtils::stdOutStream() << "MATRIX STUFF NAME: " << mesh.name().asChar() << endl;

	MMatrix localMatrix = MMatrix(path.exclusiveMatrix());
	MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

	std::string translateVector;
	translateVector.append(to_string(worldMatrix(3, 0)) + " ");
	translateVector.append(to_string(worldMatrix(3, 1)) + " ");
	translateVector.append(to_string(worldMatrix(3, 2)) + " ");

	std::string objName = mesh.name().asChar();
	std::string msgString = translateVector;

	//pass to send
	bool msgToSend = false;
	if (msgString.length() > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(CMDTYPE::UPDATE_MATRIX, NODE_TYPE::LIGHT, 0, 0, 0, 0, objName, msgString);
	}
}

void meshConnectionChanged(MPlug &plug, MPlug &otherPlug, bool made, void *clientData)
{
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	//////////////////////////////////
	//	 MATERIALS AND TEXTURES 	//
	//////////////////////////////////

	std::string meshName = mesh.name().asChar();
	std::string testString = "shaderBallGeomShape";
	bool shaderBall = false;
	if (meshName.find(testString) == std::string::npos)
	{
		shaderBall = false;
	}
	else {
		shaderBall = true;
	}

	if (!shaderBall) {

		MObjectArray shaderGroups;
		MIntArray shaderGroupsIndecies;
		mesh.getConnectedShaders(0, shaderGroups, shaderGroupsIndecies);

		if (shaderGroups.length() > 0)
		{
			MFnDependencyNode shaderNode(shaderGroups[0]);
			MPlug surfaceShader = shaderNode.findPlug("surfaceShader");
			MPlugArray shaderNodeconnections;
			surfaceShader.connectedTo(shaderNodeconnections, true, false);

			for (int j = 0; j < shaderNodeconnections.length(); j++) {
				if (shaderNodeconnections[j].node().apiType() == MFn::kLambert)
				{
					MObject lambertObj(shaderNodeconnections[j].node());
					MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(lambertObj, nodeMaterialAttributeChanged);
					callbackIdArray.append(tempID);

					MFnDependencyNode lambertDepNode(lambertObj);
					MPlug colorPlug = lambertDepNode.findPlug("color");

					MFnLambertShader lambertItem(lambertObj);

					MColor color;
					MPlug attr;

					attr = lambertItem.findPlug("colorR");
					attr.getValue(color.r);
					attr = lambertItem.findPlug("colorG");
					attr.getValue(color.g);
					attr = lambertItem.findPlug("colorB");
					attr.getValue(color.b);

					MStreamUtils::stdOutStream() << "color: " << color.r << ", " << color.g << ", " << color.b << endl;
				}
			}
		}
	}
}

//sending
void vtxPlugConnected(MPlug & srcPlug, MPlug & destPlug, bool made, void* clientData) {

	MStatus status = MS::kSuccess;

	if (srcPlug.partialName() == "out" && destPlug.partialName() == "i") {

		if (made == true) {

			MDagPath path;
			MFnDagNode(destPlug.node()).getPath(path);
			MFnTransform transform(path);
			MFnMesh mesh(path);

			MPlugArray plugArray;
			destPlug.connectedTo(plugArray, true, true);

			std::string testString = "polyTriangulate";
			std::string name = plugArray[0].name().asChar();
			MStreamUtils::stdOutStream() << "============================================" << endl;

			MStreamUtils::stdOutStream() << "Plug name: " << plugArray[0].name().asChar() << endl;

			// Make sure mesh is triangulated =======

			std::size_t found = name.find(testString);

			if (found != std::string::npos) {
				MStreamUtils::stdOutStream() << "Already triangulated..." << endl;
			}

			else {
				MStreamUtils::stdOutStream() << "Triangulating..." << endl;
				status = MGlobal::executeCommand("polyTriangulate " + mesh.name(), true, true);
			}
		
			/*else {
				status = MS::kFailure;
				MStreamUtils::stdOutStream() << "NONE" << endl;

			}*/

			// if triangulated, proceed =======
			//if (status == MS::kSuccess) {

				/////////////
				// 	 VTX   //
				/////////////

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

				if (currentVtx.isCompound()) {
					//we have control points [0] but in Maya the values are one hierarchy down so we acces them by getting the child
					MPlug plugX = currentVtx.child(0);
					MPlug plugY = currentVtx.child(1);
					MPlug plugZ = currentVtx.child(2);

					//get value and story them in our xyz
					plugX.getValue(x);
					plugY.getValue(y);
					plugZ.getValue(z);

					MVector tempPoint = { x, y, z };
					vtxArrayMessy.append(tempPoint);
				}
			}

			//sort vtx correctly
			MIntArray triCount;
			MIntArray triVertsIndex;
			MVectorArray trueVtxForm;

			mesh.getTriangles(triCount, triVertsIndex);

			for (int i = 0; i < triVertsIndex.length(); i++) {
				trueVtxForm.append(vtxArrayMessy[triVertsIndex[i]]);
			}

			int totalTrisCount = triCount.length() * 2;
			std::string objName = mesh.name().asChar();

			//MVector to string
			std::string vtxArrayString;
			size_t vtxArrElements = 0;

			int vtxCount = trueVtxForm.length();
			vtxArrayString.append(to_string(vtxCount) + " ");

			for (int u = 0; u < trueVtxForm.length(); u++) {
				for (int v = 0; v < 3; v++) {
					vtxArrayString.append(std::to_string(trueVtxForm[u][v]) + " ");
					vtxArrElements++;
				}
			}


			///////////////
			//// NORMALS //
			///////////////

			//MStreamUtils::stdOutStream() << "================================= " << endl;

			MFloatVectorArray normals;
			//MFloatVectorArray initialNormals;
			mesh.getNormals(normals, MSpace::kWorld);


			//MIntArray vertexCount;
			//MIntArray vertexList;
			//mesh.getVertices(vertexCount, vertexList);

			MStreamUtils::stdOutStream() << "================" << endl;
			MStreamUtils::stdOutStream() << triCount.length() << endl;


			MIntArray normalIds;
			//mesh.getNormalIds(normalCounts, normalIds);

			int currentVtx = 0; 
			MIntArray tempNormalIds;
			int nrOfFaces = triCount.length(); 

			for (int faceCnt = 0; faceCnt < nrOfFaces; faceCnt++) {
				mesh.getFaceNormalIds(faceCnt, tempNormalIds);
				normalIds.append(tempNormalIds[currentVtx]);
			}
			
	
			int vtxFace = 0;
			MVectorArray orderedNormals;

			for (int i = 0; i < nrOfFaces; i++) {
				for (int j = 0; j < 3; j++) {
					orderedNormals.append(normals[normalIds[vtxFace]]);
				}

				vtxFace++;
			}


			/* 
			MVectorArray orderedNorm; 
			for (int i = 0; i < normalIds.length(); i++) {
				int vtxInd = normalIds[i];
				//MStreamUtils::stdOutStream() << "triVertsIndex: " << triVertsIndex[vtxInd] << endl;
				//MStreamUtils::stdOutStream() << "normals[vtxInd]: " << normals[vtxInd] << endl;

				orderedNorm.append(normals[vtxInd]);
			}
			
			MStreamUtils::stdOutStream() << "orderedNorm: " << orderedNorm << endl;


			//MVector to string
			std::string nrmlArrayString;
			size_t nrmlArrElements = 0;

			int nrmlCount = orderedNorm.length();
			nrmlArrayString.append(to_string(nrmlCount) + " ");

			for (int u = 0; u < orderedNorm.length(); u++) {
				for (int v = 0; v < 3; v++) {
					nrmlArrayString.append(std::to_string(orderedNorm[u][v]) + " ");
				}
			}
			*/


			/*
			//for (int j = 0; j < normalIds.length(); j++) {

			//	//MStreamUtils::stdOutStream() << "normals: " << normals[j] << endl;
			//	int normIndex = normalIds[j]; 
			//	MStreamUtils::stdOutStream() << "Normal index: " << normIndex << endl;

			//	for (int i = 0; i < 3; i++) {
			//		MStreamUtils::stdOutStream() << "Normal at index: " << normals[normalIds[normIndex]] << endl;
			//	}

			//	MStreamUtils::stdOutStream() << " " << endl;


			//	//orderedNormals.append(normals[normalIds[j]]);

			}
			 */


			/*

			MStreamUtils::stdOutStream() << "======================== " << endl;

			MStreamUtils::stdOutStream() << "nrElements: "  << nrElements << endl;
			MStreamUtils::stdOutStream() << "nrOfNormals: " << nrOfNormals << endl;

			if (nrElements == nrOfNormals) {
				MStreamUtils::stdOutStream() << "IN NORMAL!" << endl;
				status = MS::kFailure;

				MIntArray normCounts;
				MIntArray triNormIndex;
				MVectorArray normalsArray;

				mesh.getNormalIds(normCounts, triNormIndex)

				for (int i = 0; i < triNormIndex.length(); i++) {
					normalsArray.append(normals[triNormIndex[i]]);
				}
				*/

		
			/*MStreamUtils::stdOutStream() << "normalsArray: " << normalsArray << endl;

					int nrNormals = triNormIndex.length();
					MVector to string
					std::string NormArrayString;
					NormArrayString.append(to_string(nrOfNormals) + " ");
					size_t normArrElements = 0;
				for (int u = 0; u < normalsArray.length(); u++)
				{
					for (int v = 0; v < 3; v++)
					{
						NormArrayString.append(to_string(normalsArray[u][v]) + " ");
						normArrElements++;
					}
				}
				MStreamUtils::stdOutStream() << "NormArrayString: " << NormArrayString << "_" << endl;
				
			*/

			std::string NormArrayString;
			size_t nrOfNormals = orderedNormals.length();
			NormArrayString.append(to_string(nrOfNormals) + " ");

			for (int u = 0; u < orderedNormals.length(); u++) {
				for (int v = 0; v < 3; v++)	{
					NormArrayString.append(to_string(orderedNormals[u][v]) + " ");
				}
			}



			///////////////
			////   UVS	 //
			///////////////
			MString UVName; 
			mesh.getCurrentUVSetName(UVName, -1);
			MFloatArray uArr;
			MFloatArray vArr;
			 
			mesh.getUVs(uArr, vArr, &UVName);
			
			MStreamUtils::stdOutStream() << "================" << endl;
			MStreamUtils::stdOutStream() << "UVName: " << UVName << endl;
			MStreamUtils::stdOutStream() << "U arr len: " << uArr.length() << endl;
			MStreamUtils::stdOutStream() << "V arr len: " << vArr.length() << endl;

			MStreamUtils::stdOutStream() << "uArr: " << uArr << endl;
			MStreamUtils::stdOutStream() << "vArr: " << vArr << endl;
			MStreamUtils::stdOutStream() << " " << endl;

			MFloatArray sortedUVs; 
			int nrOfUVs = 0; 
			for (int vtxNr = 0; vtxNr < trueVtxForm.length(); vtxNr++) {
			
				int vtx = triVertsIndex[vtxNr];
				float tempU, tempV; 
				mesh.getUV(vtx, tempU, tempV);

				MStreamUtils::stdOutStream() << vtxNr << " UV: " << tempU << " : " << tempV << endl;
				sortedUVs.append(tempU);
				sortedUVs.append(tempV);
				nrOfUVs++; 
			}

			MStreamUtils::stdOutStream() << " " << endl;
			MStreamUtils::stdOutStream() << "sortedUVs: " << sortedUVs << endl;
			MStreamUtils::stdOutStream() << "sortedUVs.lenght: " << sortedUVs.length() << endl;
			MStreamUtils::stdOutStream() << "nrOfUVs: " << nrOfUVs << endl;

			int nrOfUVs2 = mesh.numUVs();
			MStreamUtils::stdOutStream() << "nrOfUVs2: " << nrOfUVs2 << endl;

			// SEND MESSAGE ==================================================
			std::string masterTransformString;
			masterTransformString.append(vtxArrayString + " ");
			masterTransformString.append(NormArrayString);

			//pass to send
			bool msgToSend = false;
			if (vtxArrElements > 0)
				msgToSend = true;

			if (msgToSend) {
				sendMsg(CMDTYPE::NEW_NODE, NODE_TYPE::MESH, nrElements, totalTrisCount, nrOfNormals, nrOfUVs, objName, masterTransformString);
			}
		}
	}

}

void nodeAdded(MObject &node, void * clientData)
{
	//we register callbacks for both the new mesh kTransform and kMesh. the KTransform is in charge of the rotationg, scaling, transltion on a mesh or vrts. The kMesh is in charge of ex. subdividing or adding more vrts on the mesh.
	//we want callbacks for both beacuse we need both "transformation" information.
	MDagPath path;
	MFnDagNode(node).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);
	//we check if has kMesh attribute == is a mesh
	if (node.hasFn(MFn::kMesh))
	{
		MCallbackId tempID;
		MStatus Result = MS::kSuccess;
		newMeshes.push(node);

		tempID = MDGMessage::addConnectionCallback(vtxPlugConnected, NULL, &status);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}

		tempID = MDGMessage::addConnectionCallback(meshConnectionChanged, NULL, &Result);
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
	}

	if (node.hasFn(MFn::kLight))
	{
		newLights.push(node);
	}

	//... implement this and other callbacks
	//this function is called whenever a node is added. should pass to appropriate funtions(?)
}

void timerCallback(float elapsedTime, float lastTime, void* clientData)
{
	//MStreamUtils::stdOutStream() << "Elapsed time: " << elapsedTime << endl << "Last Time: " << lastTime << endl;
	globalTime += (elapsedTime - lastTime);
	lastTime = elapsedTime;

	//MGlobal::displayInfo(MString("TIME: ") + globalTime);

	MCallbackId tempID;
	MStatus Result = MS::kSuccess;

	for (int i = 0; i < newMeshes.size(); i++) {

		MObject currenNode = newMeshes.back();

		if (currenNode.hasFn(MFn::kMesh)) {

			MFnMesh currentMesh = currenNode;
			//MGlobal::displayInfo("MESH " + currentMesh.name());

			MDagPath path;
			MFnDagNode(currenNode).getPath(path);
			MFnDagNode currentDagNode = currenNode;

			unsigned int nrOfPrnts = currentDagNode.parentCount();
			MObject parentTransf = currentDagNode.parent(0);
			//Global::displayInfo("PRNT: " + MFnDagNode(parentTransf).name());

			/*tempID = MDGMessage::addConnectionCallback(meshConnectionChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}*/

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
		newMeshes.pop();
	}

	for (int i = 0; i < newLights.size(); i++) {

		MObject currenNode = newLights.back();
		if (currenNode.hasFn(MFn::kLight))
		{
			MFnLight sceneLight(currenNode);
			MColor lightColor;
			lightColor = sceneLight.color();
			MStreamUtils::stdOutStream() << "Light Color: " << lightColor << endl;
			MStreamUtils::stdOutStream() << "Light intensity: " << sceneLight.intensity() << endl;

			tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeLightAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			tempID = MNodeMessage::addNameChangedCallback(currenNode, nodeNameChangedLight, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

			MDagPath path;
			MFnDagNode(currenNode).getPath(path);


			MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

			MStreamUtils::stdOutStream() << "Transform World2 Matrix: " << worldMatrix << endl;

			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixChangedLight, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}

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
				sendMsg(CMDTYPE::NEW_NODE, NODE_TYPE::LIGHT, 0, 0, 0, 0, objName, msgString);
			}
		}
		newLights.pop();
	}

}

EXPORT MStatus initializePlugin(MObject obj)
{
	//ourComLib.send("1 0 1 1 0 0 0 0 1", 17);


	MStatus res = MS::kSuccess;
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
	MCallbackId tempID = MDGMessage::addNodeAddedCallback(nodeAdded);
	callbackIdArray.append(tempID);
	//MDGMessage::addNodeRemovedCallback(nodeRemoved, "dependNode", NULL, &status);
	tempID = MTimerMessage::addTimerCallback(5, timerCallback, NULL, &status);
	callbackIdArray.append(tempID);

	// Camera callbacks for all active view panels ===============================
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

	// ===========================================================================

	// a handy timer, courtesy of Maya
	gTimer.clear();
	gTimer.beginTimer();

	return res;
}

EXPORT MStatus uninitializePlugin(MObject obj) {
	MFnPlugin plugin(obj);
	cout << "Plugin unloaded =========================" << endl;
	MMessage::removeCallbacks(callbackIdArray);
	return MS::kSuccess;
}


//// ADD BOTH TOGETHER

/*int newSize = checkLoopFaceIndex + 1 + nrElements * 3;
std::string* tempString = new std::string[newSize];
int tempSTringArrayValue = 0;
for (int u = 0; u < checkLoopFaceIndex + 1; u++)
{
	tempString[tempSTringArrayValue] = arrayVtxIndecies[u];
	tempSTringArrayValue++;
}
for (int u = 0; u < nrElements * 3; u++)
{
	tempString[tempSTringArrayValue] = test1[u];
	tempSTringArrayValue++;
}*/

/*for (int u = 0; u < newSize; u++)
{
	MStreamUtils::stdOutStream() << "tempString:  " << tempString[u] << endl;
}*/
/*for (int u = 0; u < newSize; u++)*/
//MStreamUtils::stdOutStream() << "msg length:  " << newSize << endl;
//MStreamUtils::stdOutStream() << "msg length with space:  " << newSize * 2 << endl;
/*MIntArray* vtxCountAsArray;
vtxCountAsArray[0] = nrElements;
status2 = mesh.getVertices(&pointsOnMesh, &vtxIndecies);
if (status2)
{
	MStreamUtils::stdOutStream() << "verts indecies:  " << vtxCountAsArray[0] << endl;
}*/

/*MObjectArray components;
MObjectArray sets;
unsigned int instanceNr = path.instanceNumber();
mesh.getConnectedSetsAndMembers(instanceNr, sets, components, 1);
for (int u = 0; i < sets.length(); u++)
{
	MFnSet setFn(sets[u]);
	MItMeshPolygon faceIt(path, components[u]);
	MStreamUtils::stdOutStream() << "sets:  " << setFn.name() << endl;
	MStreamUtils::stdOutStream() << "faceIt count:  " << faceIt.count() << endl;
	MStreamUtils::stdOutStream() << "faceIt index:  " << faceIt.index() << endl;
}*/

//while (!itPoly.isDone())
//{
//	int vtxCountForCurrentFace = itPoly.polygonVertexCount();

//	MStreamUtils::stdOutStream() << "current vtx count:  " << vtxCountForCurrentFace << endl;
//	/*for (int u = 0; u < vtxCountForCurrentFace; u++)
//	{
//		MStreamUtils::stdOutStream() << "vertexIndex array:  " << itPoly.vertexIndex(u) << endl;
//		MStreamUtils::stdOutStream() << "normalIndex array:  " << itPoly.normalIndex(u) << endl;
//	}*/
//	
//}



//MDagPath path;
//MFnDagNode(destPlug.node()).getPath(path);
//MFnMesh mesh(path);
//
////////////////////////// points not cp
//
//MPlug vtxArray = mesh.findPlug("controlPoints");
//
////gives: 8 (8 on cube)
//size_t nrElements = vtxArray.numElements();
//std::string test1;	//*2 spaces, *3 xyz
//MVectorArray test1temp;
//
//for (int i = 0; i < nrElements; i++)
//{
//	//get plug for i array item (gives: shape.vrts[x])
//	MPlug currentVtx = vtxArray.elementByLogicalIndex(i);
//
//	float x = 0;
//	float y = 0;
//	float z = 0;
//
//	if (currentVtx.isCompound())
//	{
//		//we have control points [0] but in Maya the values are one hierarchy down so we acces them by getting the child
//		MPlug plugX = currentVtx.child(0);
//		MPlug plugY = currentVtx.child(1);
//		MPlug plugZ = currentVtx.child(2);
//
//		//get value and story them in our xyz
//		plugX.getValue(x);
//		plugY.getValue(y);
//		plugZ.getValue(z);
//
//		MVector tempPoint = { x, y, z };
//		test1temp.append(tempPoint);
//	}
//}
//
//MStreamUtils::stdOutStream() << "------------------------------------" << std::endl;
//
//MIntArray triCount;
//MIntArray triVertsIndex;
//mesh.getTriangles(triCount, triVertsIndex);
//MVectorArray trueVtxForm;
//for (int i = 0; i < triVertsIndex.length(); i++)
//{
//	trueVtxForm.append(test1temp[triVertsIndex[i]]);
//	MStreamUtils::stdOutStream() << trueVtxForm[i] << std::endl;
//}
//
//
//std::string objName = "addModel ";
//objName += mesh.name().asChar();
//objName += " | ";
//size_t lengthName = objName.length();
//
//std::string* vtxArrayString = new std::string[1 + trueVtxForm.length() * 3];
//size_t vtxArrayElementCheck = 1;
//vtxArrayString[0] = objName;
//
//for (int u = 0; u < trueVtxForm.length(); u++)
//{
//	for (int v = 0; v < 3; v++)
//	{
//		vtxArrayString[vtxArrayElementCheck] = to_string(trueVtxForm[u][v]) + " ";
//		vtxArrayElementCheck++;
//	}
//}
//
//size_t finalSize = 0;
//size_t tempSizeElement = 0;
//for (int x = 0; x < vtxArrayElementCheck; x++)	//*2 number of spaces
//{
//	tempSizeElement = strlen(vtxArrayString[x].c_str());
//	finalSize = finalSize + tempSizeElement;
//}
//
//
//char* charArrayTest = new char[finalSize];	//FINAL SIZE IS GENERAL DONT CHANGE
//size_t z = 0;
//size_t xMoveForward = 0;
//
//MStreamUtils::stdOutStream() << "------------------------------------" << std::endl;
//
////elementMoveForwardCheck = 0;
//while (z != vtxArrayElementCheck)	//antalet vartiser
//{
//	for (int y = 0; y < vtxArrayString[z].length(); y++)
//	{
//		charArrayTest[xMoveForward] = vtxArrayString[z][y];
//		xMoveForward++;
//	}
//	z++;
//}
//
//for (int i = 0; i < strlen(charArrayTest); i++)
//{
//	MStreamUtils::stdOutStream() << charArrayTest[i] << std::endl;
//}
//
//ourComLib.send(charArrayTest, strlen(charArrayTest));
//delete[] charArrayTest;
//delete[] vtxArrayString;


	//////////////////////// points not cp
//	MStreamUtils::stdOutStream() << "------------------------------------" << endl;
//	//MIntArray* vtxIndecies;
//	MPointArray pointsOnMesh;
//	MStatus status2 = mesh.getPoints(pointsOnMesh, MSpace::kObject);
//	if (status2)
//	{
//		for (int i = 0; i < nrElements; i++)
//		{
//			MStreamUtils::stdOutStream() << "Point array:  " << pointsOnMesh[i] << endl;
//		}
//	}
//
//	int checkLoopFaceIndex = 0;
//	MItMeshPolygon faceIt(path);
//	for (; !faceIt.isDone(); faceIt.next())
//	{
//		int currentFaceCount = faceIt.polygonVertexCount();
//		checkLoopFaceIndex += currentFaceCount;
//	}
//
//	MVectorArray vtxTest;
//	MIntArray vrtsTest2;
//	MItMeshPolygon faceIt2(path);
////	std::string* arrayVtxIndecies = new std::string[checkLoopFaceIndex + 1];
//	int nrFaces = 0;
//	for (; !faceIt2.isDone(); faceIt2.next())
//	{
//		int currentFaceCount = faceIt2.polygonVertexCount();
//		MStreamUtils::stdOutStream() << "currect face iterating: " << nrFaces << endl;
//		nrFaces++;
//
//		MStreamUtils::stdOutStream() << "currect face nr vtx: " << currentFaceCount << endl;
//
//		MPointArray tempVtxTest;
//		faceIt2.getPoints(tempVtxTest, MSpace::kObject);
//		MStreamUtils::stdOutStream() << "temp points: " << tempVtxTest << endl;
//
//		for (int u = 0; u < currentFaceCount; u++)
//		{
//			MVector tempPoint = { tempVtxTest[u][0], tempVtxTest[u][1], tempVtxTest[u][2] };
//			vtxTest.append(tempPoint);
//		}
//
//		/*MStreamUtils::stdOutStream() << "current face count:  " << currentFaceCount << endl;
//		for (int u = 0; u < currentFaceCount; u++)
//		{
//
//			MStreamUtils::stdOutStream() << "face NR array:  " << faceIt2.vertexIndex(u) << endl;
//			arrayVtxIndecies[faceElementIndexCount] = to_string(faceIt2.vertexIndex(u)) + " ";
//			faceElementIndexCount++;
//		}*/
//	}
//
//	MIntArray triCount;
//	MIntArray triVertsIndex;
//	mesh.getTriangles(triCount, triVertsIndex);
//	MVectorArray trueVtxForm;
//	for (int i = 0; i < triVertsIndex.length(); i++)
//	{
//		trueVtxForm.append(vtxTest[triVertsIndex[i]]);
//		MStreamUtils::stdOutStream() << trueVtxForm[i] << std::endl;
//	}




//get array size
//int finalSize = strlen(vtxArrayString.c_str());
//MStreamUtils::stdOutStream() << "finalSize: " << finalSize << endl;
//
//
//char* charArrayTest = new char[finalSize * 2];	//FINAL SIZE IS GENERAL DONT CHANGE
//int z = 0;
//int xMoveForward = 0;
//while (z != vtxArrayElementCheck)	//antalet vartiser
//{
//	for (int y = 0; y < vtxArrayString[z].length(); y++)
//	{
//		charArrayTest[xMoveForward] = vtxArrayString[z][y];
//		xMoveForward++;
//	}
//	z++;
//}
//
//const void* test5 = test1;
//for (int x = 0; x < nrElements * 3; x++)
//{
//	MStreamUtils::stdOutStream() << "test1[x].c_str(): " << strdup(test1[x].c_str()) << endl;
//	char temp = strdup(test1[x].c_str());
//	test3[x] = temp;
//
//	MStreamUtils::stdOutStream() << "vtx with char array (char): " << test3[x] << endl;
//	MStreamUtils::stdOutStream() << "vtx with Mstring array: " << test1[x] << endl;
//	
//}
//for (int x = 0; x < finalSize; x++)
//{
//	MStreamUtils::stdOutStream() << "vtx with charArray array: " << charArrayTest[x] << endl;
//}
//const char* test3 = test1.asChar();
//
//MStreamUtils::stdOutStream() << "strlen(charArrayTest): " << strlen(charArrayTest) << endl;
//
///*for (int u = 0; u < strlen(charArrayTest); u++)
//{
//	MStreamUtils::stdOutStream() << "strlen(charArrayTest): " << charArrayTest[u] << endl;
//}*/
//
//ourComLib.send(charArrayTest, strlen(charArrayTest));
//delete[] charArrayTest;
//delete[] vtxArrayString;
//måste skicka en char pointer