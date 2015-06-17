#include <maya/OpenMayaMac.h>
#include <maya/MStatus.h>
#include <maya/MObject.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MItDag.h>
#include <maya/MPxCommand.h>

static void WalkNodes()
{
	MItDag it;

	for(; !it.isDone(); it.next())
	{
		MObject obj;
		
		obj = it.item();

		cout << "object: " << obj.apiTypeStr() << endl;
	}
}

class WalkNodesCmd : public MPxCommand
{
public:

	static void *StaticCreate()
	{
		return new WalkNodesCmd();
	}
	
	virtual MStatus doIt( const MArgList& args)
	{
		MGlobal::displayInfo(MString("this is the walknodes plugin"));
		
		WalkNodes();
	
		return MS::kSuccess;
	}
};

MStatus initializePlugin(MObject obj)
{
	MFnPlugin pluginFn(obj, "brycek", "1.0f");
	MStatus stat;
	
	stat = pluginFn.registerCommand("walknodes", WalkNodesCmd::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	
	return stat;
}

MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin pluginFn(obj);
	MStatus stat;
	
	stat = pluginFn.deregisterCommand("walknodes");
	if(!stat)
		stat.perror("deregisterCommand failed");

	return stat;
}
