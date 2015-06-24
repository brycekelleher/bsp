import sys
import maya.OpenMaya as om
import maya.OpenMayaMPx as OpenMayaMPx

def FilenameDialog():
        om.MString filename
        om.MCommandResult result
 
        om.MGlobal.executeCommand('fileDialog -m 1', result)
        result.getResult(filename)

	return filename

def IsNodeType(node, nodetype):
	return (obj.apiType() == nodetype)

def GetAllPathsForNode(node):
	patharray = om.MDagPathArray()
	om.MDagPath.getAllPathsTo(node, patharray)    
	return patharray

def GetTransformMatrix(node):
	return om.MFnTransform(node).transformation().asMatrix()

def EmitPolygon(vertices, indicies, transform):
	print 'polygon'
	print '{'
	for i in range(indicies.length()):
		v = vertices[indicies[i]] * transform
		print 'vertex %i %f %f %f' % (i, v[0], v[1], v[2])
	print '}'

def EmitMesh(node, transform):
	mesh = om.MFnMesh(node)

	# get the mesh vertices in object space
	vertices = om.MPointArray()
	mesh.getPoints(vertices, om.MSpace.kObject);

	# iterate through the polygons
	for i in range(mesh.numPolygons()):
		indicies = om.MIntArray()
		mesh.getPolygonVertices(i, indicies)
		EmitPolygon(vertices, indicies, transform)

def PrintPath(path):
	sys.stdout.write('path: ' + path.fullPathName())

def MapMeshNodes(func):
	it = om.MItDag()
	while(not it.isDone()):
		obj = it.currentItem()

		# filter nodes
		if(!IsNodeType(om.MFn.kMesh):
			continue

		# extract the transform from the path
		path = om.MDagPath()
		it.getPath(path)
		print 'path: ' + path.fullPathName()

		# callback
		func(obj, transform)

		it.next()


def IterateNodes():
	it = om.MItDag()
	while(not it.isDone()):
		obj = it.currentItem()

		if(obj.apiType() == om.MFn.kMesh):
			print 'processsing mesh: ' + om.MFnMesh(obj).name()

			# extract the transform from the path
			path = om.MDagPath()
			it.getPath(path)

			EmitMesh(obj, path.inclusiveMatrix())

		it.next()

def Main():
	om.MGlobal.displayInfo('iterating nodes')
	IterateNodes()

#Main()

# Command
class scriptedCommand(OpenMayaMPx.MPxCommand):
	kPluginCmdName = "dumpgeo"

	def __init__(self):
		OpenMayaMPx.MPxCommand.__init__(self)

	# Invoked when the command is run.
	def doIt(self, argList):
		print "this is another edit"
		Main()

	# Creator
	@staticmethod
	def cmdCreator():
		return OpenMayaMPx.asMPxPtr( scriptedCommand() )

	@staticmethod
	def register(mobject):
		mplugin = OpenMayaMPx.MFnPlugin(mobject)
		try:
			mplugin.registerCommand( scriptedCommand.kPluginCmdName, scriptedCommand.cmdCreator )
		except:
			sys.stderr.write( "Failed to register command: %s\n" % scriptedCommand.kPluginCmdName )
			raise
	
	@staticmethod
	def unregister(mobject):
		mplugin = OpenMayaMPx.MFnPlugin(mobject)
		try:
			mplugin.deregisterCommand( scriptedCommand.kPluginCmdName )
		except:
			sys.stderr.write( "Failed to unregister command: %s\n" % scriptedCommand.kPluginCmdName )
			raise
    
# Initialize the script plug-in
def initializePlugin(mobject):
	scriptedCommand.register(mobject)

# Uninitialize the script plug-in
def uninitializePlugin(mobject):
	scriptedCommand.unregister(mobject)

