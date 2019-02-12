#include "maya_includes.h"

#include <maya/MTimer.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>

//#include <C:\Users\BTH\Desktop\shared\Project1\ComLib.h>
#include <C:\Users\Mudzi\source\repos\SharedBuff\shared\Project1\ComLib.h>

#pragma comment(lib, "Project1.lib")

//using namespace std;

MCallbackIdArray callbackIdArray;
MCallbackId		 callbackId;

MObject m_node;
MStatus status = MS::kSuccess;
bool initBool = false;

MTimer gTimer;
float globalTime = 0;

enum NODE_TYPE { TRANSFORM, MESH }; 

enum CMDTYPE {
	DEFAULT		= 1000,
	NEW_NODE	= 1001,
	UPDATE_NODE = 1002

};

struct MsgHeader {
	CMDTYPE type;
	int nrOf;
	int nameLen;
	int msgSize; 
	char objName[64];
};

/*
 * how Maya calls this method when a node is added.
 * new POLY mesh: kPolyXXX, kTransform, kMesh
 * new MATERIAL : kBlinn, kShadingEngine, kMaterialInfo
 * new LIGHT    : kTransform, [kPointLight, kDirLight, kAmbientLight]
 * new JOINT    : kJoint
 */

// keep track of created objects to maintain them
std::queue<MObject> newMeshes;
std::queue<MObject> newLights; 

ComLib comLib("sharedBuff2", 100, PRODUCER);

// =========================================================
bool sendMsg(std::string &msgString, CMDTYPE msgType, int nrOfElements, std::string objName) {

	//MGlobal::displayInfo(MString("nrOfElements: ") + nrOfElements + "\n");
	bool sent = false;

	// Fill header ================= 
	MsgHeader msgHeader;
	msgHeader.type = msgType;
	msgHeader.nrOf = nrOfElements;
	msgHeader.msgSize = msgString.size() + 1;
	msgHeader.nameLen = objName.length();
	memcpy(msgHeader.objName, objName.c_str(), objName.length());


	// Copy MSG ================== 
	size_t totalMsgSize = (sizeof(MsgHeader) + msgHeader.msgSize); 
	char* msg = new char[totalMsgSize];

	memcpy((char*)msg, &msgHeader, sizeof(MsgHeader));
	memcpy((char*)msg + sizeof(MsgHeader), msgString.c_str(), msgHeader.msgSize);

	sent = comLib.send(msg, totalMsgSize);
	// =========================== 
	delete[]msg;
	return sent;

}

// ============== LIGHTS ==============
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
}

// ============ TEXTURES ==============
void nodeTextureAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x)
{
	MObject texObj(plug.node());

	MFnDependencyNode textureNode(texObj);
	MPlug fileTextureName = textureNode.findPlug("ftn");

	if (plug.name() == fileTextureName.name())
	{
		MString fileName;
		fileTextureName.getValue(fileName);
		MGlobal::displayInfo("FILENAME: " + fileName);

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

			MString clr("Color: ");
			clr += color.r;
			clr += " ; ";
			clr += color.g;
			clr += " ; ";
			clr += color.b;


			MGlobal::displayInfo("TEX COLOR: " + clr);
		}
	}
}

// ============= MESHES ==============
void meshConnectionChanged(MPlug &plug, MPlug &otherPlug, bool made, void *clientData) {
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnTransform transform(path);
	MFnMesh mesh(path);

	MStreamUtils::stdOutStream() << "Connection changed! " << plug.name() << endl;

	//MGlobal::displayInfo("MESH NAME: " + mesh.name());


	//////////////////////////////////
	//	 MATERIALS AND TEXTURES 	//
	//////////////////////////////////

	MObjectArray shaderGroups;
	MIntArray shaderGroupsIndecies;
	mesh.getConnectedShaders(0, shaderGroups, shaderGroupsIndecies);

	if (shaderGroups.length() > 0)
	{
		MFnDependencyNode shaderNode(shaderGroups[0]);
		MPlug surfaceShader = shaderNode.findPlug("surfaceShader");
		MPlugArray shaderNodeconnections;
		surfaceShader.connectedTo(shaderNodeconnections, true, false);

		for (int j = 0; j < shaderNodeconnections.length(); j++)
		{
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

void nodeUVAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

	//MGlobal::displayInfo(MString("PLUG CHANGED: " + plug.name() + " : " + plug.partialName()) + "\n");
	//MGlobal::displayInfo(MString("OTHER PLUG: " + otherPlug.name() + " : " + otherPlug.partialName()) + "\n");

	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnMesh mesh(path);

	//MGlobal::displayInfo("NODE: " + path.partialPathName() + "\n");

	/////////////
	// ALL UVS //
	/////////////

	MStringArray texCoordNames;
	mesh.getUVSetNames(texCoordNames);
	//MGlobal::displayInfo("UV set names: " + texCoordNames[0]);

	MString* texCoordSet = &texCoordNames[0];
	MFloatArray texCoordArrU;
	MFloatArray texCoordArrV;
	mesh.getUVs(texCoordArrU, texCoordArrV, texCoordSet);

	int nrOfUVs = mesh.numUVs();

	MString nrOfUVsString("");
	nrOfUVsString += nrOfUVs;

	//MGlobal::displayInfo("Nr of UVs: " + nrOfUVsString);

	for (int i = 0; i < nrOfUVs; i++) {

		float texCoordU = texCoordArrU[i];
		float texCoordV = texCoordArrV[i];

		MString texCoordString("");
		texCoordString += texCoordU;
		texCoordString += ", ";
		texCoordString += texCoordV;

		//MGlobal::displayInfo("UV: " + texCoordString);
	}

	//MGlobal::displayInfo("=========================================");


	// ===================

	/*
	MPlug uvArray = mesh.findPlug("uvSet");
	//MStreamUtils::stdOutStream() << uvArray.name() << endl;
	//MStreamUtils::stdOutStream() << "mesh.NumUVS: " << mesh.numUVs() << endl;

	//MGlobal::displayInfo("UV Array: " + uvArray.name());
	//MGlobal::displayInfo("Num UV's: " + mesh.numUVs());


	MPlug uvSet0 = uvArray.elementByLogicalIndex(0);
	//MStreamUtils::stdOutStream() << uvSet0.name() << endl;
	//MGlobal::displayInfo("UV set 0: " + uvSet0.name());

	MPlug uvSetPoints = uvSet0.child(1);
	//MStreamUtils::stdOutStream() << uvSetPoints.name() << endl;
	//MGlobal::displayInfo("UV set points: " + uvSetPoints.name());

	for (int i = 0; i < mesh.numUVs(); i++) {
		MPlug uvSetPoints0 = uvSetPoints.elementByLogicalIndex(i);
		//MStreamUtils::stdOutStream() << uvSetPoints0.name() << endl;
		//MGlobal::displayInfo("UV set points 0: " + uvSetPoints0.name());

		MPlug plugU = uvSetPoints0.child(0);
		MPlug plugV = uvSetPoints0.child(1);

		float posU;
		plugU.getValue(posU);
		float posV;
		plugV.getValue(posV);

		MString values("Values: ");
		values += posU;
		values += ", ";
		values += posV;

		//MGlobal::displayInfo(values);

		//MStreamUtils::stdOutStream() << posU << endl;
		//MStreamUtils::stdOutStream() << posV << endl;
	}
*/

}

void nodeNormalAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

	/////////////
	// NORMALS //
	/////////////

	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnMesh mesh(path);

	MFloatVectorArray normals;
	mesh.getNormals(normals, MSpace::kWorld);

	int nrOfNormals = normals.length();

	for (int i = 0; i < nrOfNormals; i++) {

		MFloatVector currentNormal = normals[i];
		MString nrml("");
		nrml += currentNormal[0];
		nrml += ", ";
		nrml += currentNormal[1];
		nrml += ", ";
		nrml += currentNormal[2];

		//MGlobal::displayInfo("NORMAL: " + nrml);
	}


}

void nodeAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug &plug, MPlug &otherPlug, void* x) {

	// VERTS  =========================
	MDagPath path;
	MFnDagNode(plug.node()).getPath(path);
	MFnMesh mesh(path);
	
	MPlug vtxArray = mesh.findPlug("controlPoints");
	size_t nrElements = vtxArray.numElements();

	//get vtx in array
	MVectorArray vtxArrayMessy;
	for (int i = 0; i < nrElements; i++) {
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
	mesh.getTriangles(triCount, triVertsIndex);
	MVectorArray trueVtxForm;

	for (int i = 0; i < triVertsIndex.length(); i++) {
		trueVtxForm.append(vtxArrayMessy[triVertsIndex[i]]);
	}

	std::string objName = mesh.name().asChar();
	
	//MVector to string
	std::string vtxArrayString;
	size_t vtxArrElements = 0;

	for (int u = 0; u < trueVtxForm.length(); u++) {
		for (int v = 0; v < 3; v++) {
			vtxArrayString.append(std::to_string(trueVtxForm[u][v]) + " ");
			vtxArrElements++;
		}
	}

	//pass to send
	bool msgToSend = false;
	if (vtxArrElements > 0)
		msgToSend = true;

	if (msgToSend) {
		sendMsg(vtxArrayString, CMDTYPE::UPDATE_NODE, nrElements, objName);
	}

	// MATERIAL ================================================
	MObjectArray shaderGroups;
	MIntArray shaderGroupsIndecies;
	mesh.getConnectedShaders(0, shaderGroups, shaderGroupsIndecies);

	if (shaderGroups.length() > 1) {
		MFnDependencyNode shaderNode(shaderGroups[0]);
		MPlug surfaceShader = shaderNode.findPlug("surfaceShader");

		//MStreamUtils::stdOutStream() << surfaceShader.name() << endl;
		MGlobal::displayInfo("SHADER: " + surfaceShader.name());
	}

}

void vtxPlugConnected(MPlug & srcPlug, MPlug & destPlug, bool made, void* clientData) {


	MStreamUtils::stdOutStream() << "srcPlug.partialName()" << srcPlug.partialName() << endl;
	MStreamUtils::stdOutStream() << "destPlug.partialName()" << destPlug.partialName() << endl;
	MStreamUtils::stdOutStream() << "destPlug.name()" << destPlug.name() << endl;
	MStreamUtils::stdOutStream() << "srcPlug.name()" << srcPlug.name() << endl;

	if (srcPlug.partialName() == "out" && destPlug.partialName() == "i")
	{
		if (made == true)
		{
			MDagPath path;
			MFnDagNode(destPlug.node()).getPath(path);
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
			mesh.getTriangles(triCount, triVertsIndex);
			MVectorArray trueVtxForm;
			for (int i = 0; i < triVertsIndex.length(); i++) {
				trueVtxForm.append(vtxArrayMessy[triVertsIndex[i]]);
			}

			//add command + name to string
			std::string objName = mesh.name().asChar();

			//std::string objName = "addModel ";
			//objName += mesh.name().asChar();
			//objName += " | ";
			//size_t lengthName = objName.length();

			//MVector to string
			std::string vtxArrayString;
			//vtxArrayString.append(objName);
			size_t vtxArrElements = 0;

			for (int u = 0; u < trueVtxForm.length(); u++)
			{
				for (int v = 0; v < 3; v++)
				{
					vtxArrayString.append(std::to_string(trueVtxForm[u][v]) + " ");
					vtxArrElements++;
				}
			}

			//pass to send
			bool msgToSend = false;
			if (vtxArrElements > 0)
				msgToSend = true;

			if (msgToSend) {
				sendMsg(vtxArrayString, CMDTYPE::NEW_NODE, nrElements, objName);
			}

		}
	}
}

// ============= CAMERA ==============
void activeCamera(const MString &panelName, void* cliendData) {

	//MGlobal::displayInfo("active Cam is: " + panelName);

	M3dView activeView;
	MStatus result = M3dView::getM3dViewFromModelPanel(panelName, activeView);
	if (result != MS::kSuccess) {
		MGlobal::displayInfo("Did not get active 3DView");
		return;

	}

	MMatrix modelViewMat;
	activeView.modelViewMatrix(modelViewMat);

	MString cam("Active Cam: ");
	cam += result;
	MString mat("Cam matrix: ");


	for (int i = 0; i < 4; i++) {

		mat += "[ ";
		for (int j = 0; j < 4; j++)
		{
			mat += modelViewMat[i][j];
			mat += +"; ";

		}

		mat += "] ";
	}

	//MGlobal::displayInfo(cam);
	//MGlobal::displayInfo(mat);


}

// ============= METRIX ==============
void nodeWorldMatrixModified(MObject &transformNode, MDagMessage::MatrixModifiedFlags &modified, void *clientData) {

	MDagPath path;
	MFnDagNode(transformNode).getPath(path);

	//MGlobal::displayInfo("Worldmatrix changed for:" + MFnTransform(transformNode).name() + "\n");


	// ====== worldspace MMatrix of a node =========

	//MString worldMat("World Matrix: ");
	//MString localMat("Local Matrix: ");

	MMatrix worldMatr = MMatrix(path.inclusiveMatrix());
	MMatrix localMatr = MMatrix(path.exclusiveMatrix());

	/* 
	for (int i = 0; i < 4; i++) {

		worldMat += "[ ";
		localMat += "[ ";
		for (int j = 0; j < 4; j++)
		{
			worldMat += worldMatr[i][j];
			worldMat += +"; ";

			localMat += localMatr[i][j];
			localMat += +"; ";
		}

		worldMat += "] ";
		localMat += "] ";
	}
	*/

	//MGlobal::displayInfo("\n================= MATRIX ===================");
	//MGlobal::displayInfo(worldMat);
	//Global::displayInfo(localMat);
	//MGlobal::displayInfo("============================================\n");

}

// ============= OTHER ==============

void nodeChangeName(MObject &node, const MString &str, void*clientData) {

	// Get the new name of the node, and use it from now on.
	MString oldName = str;
	MString newName = MFnDagNode(node).name();
	MGlobal::displayInfo("Object name changed for " + oldName + ". The new name is: " + newName);
}

void nodeConnectionChange(MPlug &srcPlug, MPlug &destPlug, bool made, void *clientData) {

	MGlobal::displayInfo("=======================================");
	MGlobal::displayInfo("Node connection changed!");
	MGlobal::displayInfo("Source plug of the connection: " + srcPlug.name());
	//MGlobal::displayInfo("Destination plug of the connection: " + destPlug.name());

	if (made == true)
		MGlobal::displayInfo("Connection was made");

	else if (made == false)
		MGlobal::displayInfo("Connection was broken");


	if (srcPlug.partialName() == "uvTweak") {
		MGlobal::displayInfo("UV Tweaked");

	}


	MGlobal::displayInfo("=======================================");
}



void nodeAdded(MObject &node, void* clientData) {

	//... implement this and other callbacks
	MCallbackId newCallbackID;
	MStatus Result = MS::kSuccess;

	if (node.hasFn(MFn::kLight)) {

		MGlobal::displayInfo(MString("Light has been added: ") + MFnDagNode(node).name() + "\n");
		newLights.push(node);

	}


	if (node.hasFn(MFn::kMesh)) {

		MGlobal::displayInfo(MString("New Mesh has been added: ") + MFnDagNode(node).name() + "\n");
		newMeshes.push(node);

		MCallbackId tempID = MDGMessage::addConnectionCallback(vtxPlugConnected, NULL, &status);
	
	
		if (Result == MS::kSuccess) {
			callbackIdArray.append(tempID);
		}
	}

}

void nodeRemoved(MObject& node, void* clientData) {
	MFnDependencyNode nodeFn(node);
	MGlobal::displayInfo(MString("About to delete callback for node: ") + nodeFn.name() + "\n");


}

// ============= TIMER ==============
void timerCallback(float elapsedTime, float lastTime, void* clientData) {

	elapsedTime = gTimer.elapsedTime();

	globalTime += (elapsedTime - lastTime);
	lastTime    = elapsedTime;

	MCallbackId newCallbackID;
	MStatus Result = MS::kSuccess;

	for (int i = 0; i < newMeshes.size(); i++) {

		MObject currenNode = newMeshes.back();

		if (currenNode.hasFn(MFn::kMesh)) {

			MFnMesh currentMesh = currenNode;
			MGlobal::displayInfo("MESH: " + currentMesh.name());

			MDagPath path;
			MFnDagNode(currenNode).getPath(path);
			MFnDagNode currentDagNode = currenNode;

			unsigned int nrOfPrnts = currentDagNode.parentCount();
			MObject parentTransf = currentDagNode.parent(0);
			//Global::displayInfo("PRNT: " + MFnDagNode(parentTransf).name());

			newCallbackID = MDGMessage::addConnectionCallback(meshConnectionChanged);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(newCallbackID);
			}

			newCallbackID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixModified, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(newCallbackID);
			}

			newCallbackID = MNodeMessage::addNameChangedCallback(currenNode, nodeChangeName, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(newCallbackID);
			}

			newCallbackID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(newCallbackID);
			}

			newCallbackID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeUVAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(newCallbackID);
			}

			newCallbackID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeNormalAttributeChanged, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(newCallbackID);
			}

			newMeshes.pop();
		}

	}

	for (int j = 0; j < newLights.size(); j++) {

		MObject currenNode = newLights.back();

		if (currenNode.hasFn(MFn::kLight)) {

			MFnLight sceneLight(currenNode);

			MColor lightColor;
			lightColor = sceneLight.color();

			//MStreamUtils::stdOutStream() << "Light Color: " << lightColor << endl;
			//MStreamUtils::stdOutStream() << "Light intensity: " << sceneLight.intensity() << endl;

			MCallbackId tempID = MNodeMessage::addAttributeChangedCallback(currenNode, nodeLightAttributeChanged);
			callbackIdArray.append(tempID);

			MDagPath path;
			MFnDagNode(currenNode).getPath(path);

			MMatrix localMatrix = MMatrix(path.exclusiveMatrix());
			MMatrix worldMatrix = MMatrix(path.inclusiveMatrix());

			//MStreamUtils::stdOutStream() << "Transform World2 Matrix: " << worldMatrix << endl;
			//MStreamUtils::stdOutStream() << "Transform Local Matrix: " << localMatrix << endl;

			tempID = MDagMessage::addWorldMatrixModifiedCallback(path, nodeWorldMatrixModified);
			callbackIdArray.append(tempID);

			tempID = MNodeMessage::addNameChangedCallback(currenNode, nodeChangeName, NULL, &Result);
			if (Result == MS::kSuccess) {
				callbackIdArray.append(tempID);
			}
		}

		newLights.pop();
	}


}

// ======== LOAD/UNLOAD PY ==========
EXPORT MStatus initializePlugin(MObject obj) {

	MStatus res = MS::kSuccess;
	MFnPlugin myPlugin(obj, "level editor", "1.0", "Any", &res);
	if (MFAIL(res)) {
		CHECK_MSTATUS(res);
		return res;
	}

	// redirect cout to cerr, so that when we do cout goes to cerr
	// in the maya output window (not the scripting output!)
	cout.rdbuf(cerr.rdbuf());
	//cout << "Plugin loaded ===========================" << endl;  
	//MStreamUtils::stdOutStream() << "Plugin loaded ===========================\n";
	MGlobal::displayInfo(MString("Plugin loaded ===========================\n"));


	// register callbacks here for
	MCallbackId callbackID;


	callbackID = MDGMessage::addNodeAddedCallback(nodeAdded, "dependNode", NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);
	}

	callbackID = MDGMessage::addNodeRemovedCallback(nodeRemoved, "dependNode", NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);
	}

	callbackID = MTimerMessage::addTimerCallback(2.0, timerCallback, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);
	}


	// Camera callbacks for all active view panels
	callbackID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel1", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);
	}

	callbackID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel2", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);

	}

	callbackID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel3", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);
	}

	callbackID = MUiMessage::add3dViewPreRenderMsgCallback("modelPanel4", activeCamera, NULL, &status);
	if (status == MS::kSuccess) {
		callbackIdArray.append(callbackID);
	}


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