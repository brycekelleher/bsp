import sys
import maya.OpenMaya as om
import maya.OpenMayaMPx as OpenMayaMPx

def FilenameDialog():
	filename = om.MString()
	result = om.MCommandResult()
 
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

def EmitTriangles(node, transform):
	mesh = om.MFnMesh(node)

	# get the mesh vertices in object space
	vertices = om.MPointArray()
	mesh.getPoints(vertices, om.MSpace.kObject)
	
	counts = om.MIntArray()
	indicies = om.MIntArray()
	mesh.getTriangles(counts, indicies)

	print '--- triangle data ----'
	#iterate through the triangles and dump the indicies
	print 'numtriangles %i' % (indicies.length() / 3)
	for i in range(indicies.length() / 3):
		print 'triangle %i %i %i %i' % (i, indicies[i * 3 + 0], indicies[i * 3 + 1], indicies[i * 3 + 2])
	
	#iterate through the vertices
	print 'numvertices %i' % (vertices.length())
	for i in range(vertices.length()):
		v = vertices[i]
		print 'vertex %i %f %f %f' % (i, v[0], v[1], v[2])

def PrintPath(path):
	sys.stdout.write('path: ' + path.fullPathName())

def MapMeshNodes(func):
	it = om.MItDag()
	while(not it.isDone()):
		obj = it.currentItem()

		# filter nodes
		if (not IsNodeType(obj, om.MFn.kMesh)):
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

			EmitTriangles(obj, path.inclusiveMatrix())
		it.next()

def Main():
	om.MGlobal.displayInfo('iterating nodes')
	IterateNodes()

Main()

