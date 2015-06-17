#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFileIO.h>
#include <maya/MPxCommand.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MfnMesh.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItSelectionList.h>
#include <maya/MItDag.h>
#include <maya/MPointArray.h>
#include <maya/MCommandResult.h>
#include <maya/MFnSet.h>
#include <maya/MFnAttribute.h> 
#include <maya/MFnTransform.h>
#include <maya/MEulerRotation.h>
#include <maya/MArgList.h>
#include <maya/MFnMatrixData.h>
#include <maya/MMatrix.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MDagPathArray.h>
#include <maya/MTime.h>
#include <maya/MItGeometry.h>
#include <maya/MQuaternion.h>

static FILE* fp = 0;

// The last file used by the commands
static MString s_lastfile;

static MString GetFilename()
{
	MString filename;
	MCommandResult result;

	MGlobal::executeCommand("fileDialog -m 1", result);
	result.getResult(filename);

	return filename;
}

static MString GetMaterialFilenameAttribute(const MObject& node)
{
	int i;
	MStatus status;
	MString filename;
	MFnDependencyNode fnNode(node);

	// fixme: we need this for some reason?
	filename = "materials/default.txt";

	for(i = 0; i < fnNode.attributeCount(); i++)
	{
		MFnAttribute fnAttr(fnNode.attribute(i));

		if(!strcmp(fnAttr.name().asChar(), "filename"))
		{
			MPlug plug = fnNode.findPlug(fnAttr.name(), &status);
			if(status)
			{
				plug.getValue(filename);
			}
		}
	}

	return filename;
}

static MObject findShader(const MObject& setNode) 
//Summary:	finds the shading node for the given shading group set node
//Args   :	setNode - the shading group set node
//Returns:  the shader node for setNode if found;
//			MObject::kNullObj otherwise
{

	MFnDependencyNode fnNode(setNode);
	MPlug shaderPlug = fnNode.findPlug("surfaceShader");
			
	if (!shaderPlug.isNull()) {			
		MPlugArray connectedPlugs;

		//get all the plugs that are connected as the destination of this 
		//surfaceShader plug so we can find the surface shaderNode
		//
		MStatus status;
		shaderPlug.connectedTo(connectedPlugs, true, false, &status);
		if (MStatus::kFailure == status) {
			MGlobal::displayError("MPlug::connectedTo");
			return MObject::kNullObj;
		}

		if (1 != connectedPlugs.length()) {
			MGlobal::displayError("Error getting shader for: ");
		} else {
			return connectedPlugs[0].node();
		}
	}
	
	return MObject::kNullObj;
}

#if 0
// Code to extract the texture filename
                MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", &status);
                if (status == MS::kFailure)
                        continue;

                MItDependencyGraph dgIt(colorPlug, MFn::kFileTexture,
                                                   MItDependencyGraph::kUpstream, 
                                                   MItDependencyGraph::kBreadthFirst,
                                                   MItDependencyGraph::kNodeLevel, 
                                                   &status);

                if (status == MS::kFailure)
                        continue;
                
                dgIt.disablePruningOnFilter();

        // If no texture file node was found, just continue.
        //
                if (dgIt.isDone())
                        continue;
                  
        // Print out the texture node name and texture file that it references.
        //
                MObject textureNode = dgIt.thisNode();
        MPlug filenamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
        MString textureName;
        filenamePlug.getValue(textureName);
                cerr << "Set: " << fnSet.name() << endl;
        cerr << "Texture Node Name: " << MFnDependencyNode(textureNode).name() << endl;
                cerr << "Texture File Name: " << textureName.asChar() << endl;
        
#endif


class HelloWorld2Cmd : public MPxCommand
{
public:

	// Run the bsp using the last saved file
	void RunBSP()
	{
		MString filename;
		MCommandResult result;

		MGlobal::executeCommand("runbsp -l", result);
		result.getResult(filename);
	}

	virtual MStatus doIt( const MArgList& args)
	{
		int i;
		MString filename;
		MStatus status;
		bool uselastfile;
		bool runbsp;

		MGlobal::displayInfo("Hello World\n");
		
		uselastfile = false;
		runbsp = false;

		// Parse the arguments.
		for ( i = 0; i < args.length(); i++ )
		{
			if (MString("-l") == args.asString( i, &status ) && MS::kSuccess == status )
				uselastfile = true;
			else if (MString("-bsp") == args.asString( i, &status ) && MS::kSuccess == status )
				runbsp = true;
			else
				filename = args.asString(i);
		}

		// Check for invalid filenames
		if(uselastfile && s_lastfile.length())
			filename = s_lastfile;
		else if((uselastfile && !s_lastfile.length()) || !filename.length())
			filename = GetFilename();		
		
		Main(filename);
		s_lastfile = filename;

		if(runbsp)
			RunBSP();

		return MS::kSuccess;
	}

	static void* StaticCreate()
	{
		return new HelloWorld2Cmd;
	}

	MString GetDagName(MDagPath dagpath)
	{
		MFnDagNode fnNode;

		fnNode.setObject(dagpath);
		
		return fnNode.name();
	}

	void BeginMesh()
	{
		fprintf(fp, "model\n{\n");
	}

	void EndMesh()
	{
		fprintf(fp, "}\n");
	}

	void EmitPolygon(MPointArray &vertices, MIntArray &indicies, MString texturename)
	{
		unsigned int i;
		MPoint v;

		fprintf(fp, "polygon \"%s\"\n{\n", texturename.asChar());

		for(i = 0; i< indicies.length(); i++)
		{
			v = vertices[indicies[i]];
			fprintf(fp, "%f %f %f\n", v.x, v.y, v.z);
		}

		fprintf(fp, "}\n");
	}

	void EmitMesh(MDagPath dagpath)
	{
		int numpolygons;
		MPointArray vertices;
		MObjectArray shadergroups;
		MIntArray shaderindicies;
		MIntArray indicies;
		int i;

		MFnMesh fnmesh(dagpath);
		numpolygons = fnmesh.numPolygons();
		fnmesh.getPoints(vertices, MSpace::kWorld);
		fnmesh.getConnectedShaders(0, shadergroups, shaderindicies);

		for(i = 0; i < numpolygons; i++)
		{
			MString materialname;
			
			// Get the material name
			materialname = "materials/default.txt";
			if(shaderindicies[i] != -1)
			{
					MObject set = shadergroups[shaderindicies[i]];
					MFnDependencyNode shaderNode = MFnDependencyNode(findShader(set));

					// fixme: Should this really check if it's a lambert shader type?
					//if(strstr(shaderNode.name().asChar(), "lambert"))
					materialname = GetMaterialFilenameAttribute(findShader(set));
			}

			fnmesh.getPolygonVertices(i, indicies);
			EmitPolygon(vertices, indicies, materialname);
		}
	}

	bool CheckWorldGeometry()
	{
		int count;

		count = 0;
		MItDag it(MItDag::kDepthFirst,MFn::kMesh);

		for(; !it.isDone(); it.next())
		{
			MFnMesh fnMesh(it.item());

			if(strstr(fnMesh.name().asChar(), "world_"))
				count++;
		}

		return (count != 0);
	}

	void IterateWorldMeshesInSelection()
	{
		if(!CheckWorldGeometry())
		{
			MGlobal::displayError("No valid world geometry defined");
			return;
		}

		fprintf(fp, "model\n{\n");

		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		for(; !it.isDone(); it.next())
		{
			MFnMesh fnMesh(it.item());

			if(strstr(fnMesh.name().asChar(), "world_"))
			{
				// Get the dag path
				MDagPath dagpath;
				it.getPath(dagpath);

				EmitMesh(dagpath);
			}
		}

		fprintf(fp, "}\n");
	}

	void IterateDetailMeshesInSelection()
	{
		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		for(; !it.isDone(); it.next())
		{
			MFnMesh fnMesh(it.item());

			if(strstr(fnMesh.name().asChar(), "model_")
				|| strstr(fnMesh.name().asChar(), "detail_"))
			{
				// Get the dag path
				MDagPath dagpath;
				it.getPath(dagpath);

				fprintf(fp, "model\n{\n");
				EmitMesh(dagpath);
				fprintf(fp, "}\n");
			}
		}
	}

	void EmitWorldMeshes()
	{
		IterateWorldMeshesInSelection();
	}

	void EmitDetailMeshes()
	{
		IterateDetailMeshesInSelection();	
	}

	//----------------------------------------
	// Brush dumping

	void DumpBrush(MDagPath dagpath)
	{
		MFnMesh meshFn(dagpath);

		// Extract the vertices
		MPointArray vertices;
		meshFn.getPoints(vertices, MSpace::kWorld);

		MItMeshPolygon it(dagpath);

		//cout << "***** brush *****" << endl;

		for(; !it.isDone(); it.next())
		{
				MPointArray trivertices;
				MIntArray triindicies;
				it.getTriangle(0, trivertices, triindicies, MSpace::kObject);
		
				MVector e0, e1;
				e0 = vertices[triindicies[1]] - vertices[triindicies[0]];
				e1 = vertices[triindicies[2]] - vertices[triindicies[0]];

				MVector n;
				float dist;
				n = e0 ^ e1;
				n = n.normal();
				dist = -(n * vertices[triindicies[0]]);

				//cout << "plane: " << n.x << " " << n.y << " " << n.z << " " << dist << endl;
				fprintf(fp, "%f %f %f %f\n", n.x, n.y, n.z, dist);
		}
	}

	void ExportBrushes()
	{
		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		for(; !it.isDone(); it.next())
		{
			MFnMesh fnMesh(it.item());

			if(strstr(fnMesh.name().asChar(), "brush_")
				|| strstr(fnMesh.name().asChar(), "watervol_")
				|| strstr(fnMesh.name().asChar(), "fogvol_"))
			{
				// Get the dag path
				MDagPath dagpath;
				it.getPath(dagpath);

				// Dump the brush
				if(strstr(fnMesh.name().asChar(), "fogvol_"))
					fprintf(fp, "fogvol\n");
				else if(strstr(fnMesh.name().asChar(), "watervol_"))
					fprintf(fp, "watervol\n");
				else
					fprintf(fp, "brush\n");

				fprintf(fp, "{\n");
				DumpBrush(dagpath);
				fprintf(fp, "}\n");
			}
		}	
	}

	//----------------------------------------
	// Static meshes

	// Mesh dumping
	// This is shit, getTriangles returns object relative vertex indicies instead of face relative
	void GetTriangleLocalVertexIndicies(MObject obj, int polyindex, int triindex, MIntArray& indicies)
	{
		MFnMesh meshFn(obj);

		MIntArray polyindicies;
		meshFn.getPolygonVertices(polyindex, polyindicies);

		int triindicies[3];
		meshFn.getPolygonTriangleVertices(polyindex, triindex, triindicies);
	
		for(int i = 0; i < 3; i++)
		{
			// Match the triangle index to one of the polygons indicies
			for(int j = 0; j < polyindicies.length(); j++)
			{
				if(triindicies[i] == polyindicies[j])
				{
					indicies.append(j);
					break;
				}
			}
		}
	}

	void VertexListBounds(MPointArray vertices, float* min, float* max)
	{
		int i, j;

		min[0] = min[1] = min[2] = 1e20f;
		max[0] = max[1] = max[2] = -1e20f;

		for(i = 0; i < vertices.length(); i++)
		{
			for(j = 0; j < 3; j++)
			{
				if(vertices[i][j] < min[j])
					min[j] = vertices[i][j];
				if(vertices[i][j] > max[j])
					max[j] = vertices[i][j];
			}
		}
	}

	void DumpMesh(MObject obj)
	{
		MFnMesh fnMesh(obj);

		// Extract the vertices
		MPointArray vertices;
		fnMesh.getPoints(vertices, MSpace::kObject);

		// Extract the uv's
		MFloatArray uarray, varray;
		fnMesh.getUVs(uarray, varray);

		// Extract the normals
		MFloatVectorArray normals;
		fnMesh.getNormals(normals, MSpace::kObject);

		MItMeshPolygon polyit(obj);

		while(!polyit.isDone())
		{
			int i, numtriangles;

			polyit.numTriangles(numtriangles);
			for(i = 0; i < numtriangles; i++)
			{
				MPointArray trivertices;
				MIntArray triindicies;
				polyit.getTriangle(i, trivertices, triindicies, MSpace::kObject);

				// Get the local vertex indicies
				MIntArray local;
				GetTriangleLocalVertexIndicies(obj, polyit.index(), i, local);

				int j;

				// normals
				int normalindex[3];
				for(int j = 0; j < 3; j++)
					normalindex[j] = polyit.normalIndex(local[j]);

				// UVs
				int uvindex[3];
				for(int j = 0; j < 3; j++)
					 polyit.getUVIndex(local[j], uvindex[j]);

				fprintf(fp, "triangle\n{\n");
				for(j = 0; j < 3; j++)
				{
					fprintf(fp, "%f %f %f %f %f %f %f %f\n",
						vertices[triindicies[j]].x, vertices[triindicies[j]].y, vertices[triindicies[j]].z,
						uarray[uvindex[j]], varray[uvindex[j]],
						normals[normalindex[j]].x, normals[normalindex[j]].y, normals[normalindex[j]].z
					);
				}
				fprintf(fp, "}\n");

			}

			polyit.next();
		}
	}

	void DumpMeshInstance(MDagPath dagpath, MString material)
	{
		MFnTransform fnTransform(dagpath.transform());
		MVector translation = fnTransform.getTranslation(MSpace::kTransform);
		MEulerRotation er;
		fnTransform.getRotation(er);
		MVector rot = er.asVector();

		fprintf(fp, "instance \"%s\"\n{\n", material.asChar());
		fprintf(fp, "%f %f %f\n", translation.x, translation.y, translation.z);
		fprintf(fp, "%f %f %f\n", rot.x, rot.y, rot.z);
		fprintf(fp, "}\n");
	}

	// Mesh indexing stuff
	MObject m_meshlist[16];
	int m_nummeshes;
	MDagPath m_dagpath[16][16];
	int m_numinstances[16];

	int FindMesh(MString name)
	{
		int i;

		for(i = 0; i < m_nummeshes; i++)
		{
			MFnMesh fnMesh(m_meshlist[i]);
			if(!strcmp(fnMesh.name().asChar(), name.asChar()))
				return i;
		}

		return -1;
	}
	
	void AddMesh(MObject mesh, MDagPath dagpath)
	{
		int i;
		MFnMesh fnMesh(mesh);

		i = FindMesh(MString(fnMesh.name().asChar()));

		if(i == -1)
		{
			i = m_nummeshes;
			m_meshlist[m_nummeshes] = mesh;
			m_nummeshes++;
		}

		m_dagpath[i][m_numinstances[i]] = dagpath;
		m_numinstances[i]++;
	}

	void DumpStaticMeshes()
	{
		int i, j;
		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		m_nummeshes = 0;
		for(i = 0; i < 16; i++)
			m_numinstances[i] = 0;

		// Search for instanced meshes and add them to the list
		for(; !it.isDone(); it.next())
		{
			MFnMesh fnMesh(it.item());

			if(strstr(fnMesh.name().asChar(), "static_"))
			{
				// Get the dag path
				MDagPath dagpath;
				it.getPath(dagpath);

				AddMesh(it.item(), dagpath);
			}
		}	

		// Dump the meshes to a file
		fprintf(fp, "// ***** Mesh list ******\n");

		for(i = 0; i < m_nummeshes; i++)
		{
			MFnMesh fnMesh(m_meshlist[i]);

			// Dump the mesh data
			fprintf(fp, "// Mesh: %s Instances: %i\n", fnMesh.name().asChar(), m_numinstances[i]);
			fprintf(fp, "mesh\n{\n");
			DumpMesh(m_meshlist[i]);
			
			// Dump the instance data
			for(j = 0; j < m_numinstances[i]; j++)
			{
				// Get the instance material
				MObjectArray shaders;
				MIntArray shaderindicies;
				fnMesh.getConnectedShaders(m_dagpath[i][j].instanceNumber(), shaders, shaderindicies);
				MString material = GetMaterialFilenameAttribute(findShader(shaders[0]));

				fprintf(fp, "// Instance %s %s\n", m_dagpath[i][j].fullPathName().asChar(), material.asChar());
				DumpMeshInstance(m_dagpath[i][j], material);
			}

			fprintf(fp, "}\n");
		}
	}

	//----------------------------------------
	// Main
	
	void Main(MString filename)
	{
		fp = fopen(filename.asChar(), "w");
		if(!fp)
		{
			cout << "Unable to open file" << endl;
			return;
		}

		EmitWorldMeshes();
		EmitDetailMeshes();
		ExportBrushes();
		DumpStaticMeshes();

		fclose(fp);
	}
};



class InstanceTest : public MPxCommand
{
public:
	virtual MStatus doIt( const MArgList& )
	{
		MGlobal::displayInfo("Instance test\n");

		cout << "***** Iterating DG *****" << endl;

		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		for(; !it.isDone(); it.next())
		{
			MFnMesh fnMesh(it.item());

			if(it.isInstanced())
			{
				cout << "Node " << fnMesh.name().asChar() << " is instanced " << it.instanceCount(false) << " times" << endl;
				cout << ">path:" << it.fullPathName().asChar() << endl;
			}
			else
			{
				cout << "Node " << fnMesh.name().asChar() << " is not instanced" << endl;
			}
		}

		return MS::kSuccess;
	}

	static void* StaticCreate()
	{
		return new InstanceTest;
	}
};

class StaticMesh : public MPxCommand
{
public:
	virtual MStatus doIt( const MArgList& )
	{
		MGlobal::displayInfo("Static Mesh\n");
		cout << "***** Static Mesh *****" << endl;

		Main();

		return MS::kSuccess;
	}

	// This is shit, getTriangles returns object relative vertex indicies instead of face relative
	void GetTriangleLocalVertexIndicies(MDagPath obj, int polyindex, int triindex, MIntArray& indicies)
	{
		MFnMesh meshFn(obj);

		MIntArray polyindicies;
		meshFn.getPolygonVertices(polyindex, polyindicies);

		int triindicies[3];
		meshFn.getPolygonTriangleVertices(polyindex, triindex, triindicies);
	
		for(int i = 0; i < 3; i++)
		{
			// Match the triangle index to one of the polygons indicies
			for(int j = 0; j < polyindicies.length(); j++)
			{
				if(triindicies[i] == polyindicies[j])
				{
					indicies.append(j);
					break;
				}
			}
		}
	}

#if 0
	void GetUVIndicies(MDagPath obj, int polyindex, int triinedx, MIntArray indicies)
	{
		// Get the local vertex indicies
		MIntArray local;
		GetTriangleLocalVertexIndicies(dagpath, polyit.index(), i, local);

		int j;

		// normals
		int normalindex[3];
		for(int j = 0; j < 3; j++)
					polyit.normalindex(local[j], normalindex[j]);

		// UVs
		int uvindex[3];
		for(int j = 0; j < 3; j++)
			polyit.getUVIndex(local[j], uvindex[j]);
	}
#endif

	void DumpMesh(MDagPath dagpath)
	{
		MFnMesh meshFn(dagpath);

		// Extract the vertices
		MPointArray vertices;
		meshFn.getPoints(vertices, MSpace::kObject);

		// Extract the uv's
		MFloatArray uarray, varray;
		meshFn.getUVs(uarray, varray);

		// Extract the normals
		MFloatVectorArray normals;
		meshFn.getNormals(normals, MSpace::kObject);

		MItMeshPolygon polyit(dagpath);

		while(!polyit.isDone())
		{
			int i, numtriangles;

			polyit.numTriangles(numtriangles);
			for(i = 0; i < numtriangles; i++)
			{
				MPointArray trivertices;
				MIntArray triindicies;
				polyit.getTriangle(i, trivertices, triindicies, MSpace::kObject);

				MPoint v;
				cout << "Triangle:" << endl;
				cout << "face 0: " << vertices[triindicies[0]].x << " " << vertices[triindicies[0]].y << " " << vertices[triindicies[0]].z << endl;
				cout << "face 1: " << vertices[triindicies[1]].x << " " << vertices[triindicies[1]].y << " " << vertices[triindicies[1]].z << endl;
				cout << "face 2: " << vertices[triindicies[2]].x << " " << vertices[triindicies[2]].y << " " << vertices[triindicies[2]].z << endl;
			
				// Get the local vertex indicies
				MIntArray local;
				GetTriangleLocalVertexIndicies(dagpath, polyit.index(), i, local);

				int j;

				// normals
				int normalindex[3];
				for(int j = 0; j < 3; j++)
					normalindex[j] = polyit.normalIndex(local[j]);

				// UVs
				int uvindex[3];
				for(int j = 0; j < 3; j++)
					 polyit.getUVIndex(local[j], uvindex[j]);
			}

			polyit.next();
		}
	}

/*
	void Main()
	{
		MSelectionList list;
		
		MGlobal::getActiveSelectionList(list);
		MItSelectionList it(list, MFn::kMesh);

		while(!it.isDone())
		{
			MDagPath dagpath;

			it.getDagPath(dagpath);
			DumpMesh(dagpath);

			it.next();
		}
	}
*/

	void Main()
	{
		m_nummeshes = 0;
		for(int i = 0; i < 16; i++)
			m_numinstances[i] = 0;

		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		for(; !it.isDone(); it.next())
		{
			// Get the dag path
			MDagPath dagpath;
			it.getPath(dagpath);

			MFnMesh fnMesh(it.item());
			cout << "***** mesh info *******" << endl;
			cout << "meshname: " << fnMesh.name().asChar() << endl;
			cout << "dagpathname: " << dagpath.fullPathName().asChar() << endl;

			DumpMesh(dagpath);
			MFnTransform fnTransform(dagpath.transform());
			MVector translation = fnTransform.getTranslation(MSpace::kTransform);
			MEulerRotation er;
			fnTransform.getRotation(er);
			MVector rot = er.asVector();

			cout << "pos: " << translation.x << " " << translation.y << " " << translation.z << endl;
			cout << "rot: " << rot.x << " " << rot.y << " " << rot.z << endl;
			
			AddPath(it.item(), dagpath);
		}

		ListMeshes();
	}

	public:

	MObject m_meshlist[16];
	int m_nummeshes;
	MDagPath m_dagpath[16][16];
	int m_numinstances[16];

	// Mesh indexing stuff
	int FindMesh(MString name)
	{
		int i;

		for(i = 0; i < m_nummeshes; i++)
		{
			MFnMesh fnMesh(m_meshlist[i]);
			if(!strcmp(fnMesh.name().asChar(), name.asChar()))
				return i;
		}

		return -1;
	}
	
	void AddPath(MObject mesh, MDagPath dagpath)
	{
		int i;
		MFnMesh fnMesh(mesh);

		i = FindMesh(MString(fnMesh.name().asChar()));

		if(i == -1)
		{
			i = m_nummeshes;
			m_meshlist[m_nummeshes] = mesh;
			m_nummeshes++;
		}

		m_dagpath[i][m_numinstances[i]] = dagpath;
		m_numinstances[i]++;
	}

	void ListMeshes()
	{
		int i, j;

		cout << "***** Mesh list ******" << endl;

		for(i = 0; i < m_nummeshes; i++)
		{
			MFnMesh fnMesh(m_meshlist[i]);

			cout << "Mesh: " << fnMesh.name().asChar() << " " << "Instances: " << m_numinstances[i] << endl;
			
			for(j = 0; j < m_numinstances[i]; j++)
				cout << m_dagpath[i][j].fullPathName().asChar() << endl;
		}
	}

	static void* StaticCreate()
	{
		return new StaticMesh;
	}
};

class Brush : public MPxCommand
{
public:
	virtual MStatus doIt( const MArgList& )
	{
		MGlobal::displayInfo("Brush\n");
		cout << "***** Brush *****" << endl;

		Main();

		return MS::kSuccess;
	}

	void DumpBrush(MDagPath dagpath)
	{
		MFnMesh meshFn(dagpath);

		// Extract the vertices
		MPointArray vertices;
		meshFn.getPoints(vertices, MSpace::kWorld);

		MItMeshPolygon it(dagpath);

		cout << "***** brush *****" << endl;

		for(; !it.isDone(); it.next())
		{
				MPointArray trivertices;
				MIntArray triindicies;
				it.getTriangle(0, trivertices, triindicies, MSpace::kObject);
		
				MVector e0, e1;
				e0 = vertices[triindicies[1]] - vertices[triindicies[0]];
				e1 = vertices[triindicies[2]] - vertices[triindicies[0]];

				MVector n;
				float dist;
				n = e0 ^ e1;
				n = n.normal();
				dist = -(n * vertices[triindicies[0]]);

				cout << "plane: " << n.x << " " << n.y << " " << n.z << " " << dist << endl;
		}
	}

	void Main()
	{
		MSelectionList list;
		
		MGlobal::getActiveSelectionList(list);
		MItSelectionList it(list, MFn::kMesh);

		while(!it.isDone())
		{
			MDagPath dagpath;

			it.getDagPath(dagpath);
			DumpBrush(dagpath);

			it.next();
		}
	}

	static void* StaticCreate()
	{
		return new Brush;
	}
};

class RunBSP : public MPxCommand
{
public:

	virtual MStatus doIt( const MArgList& args)
	{
		int i;
		MStatus status;
		MString filename;
		bool uselastfile;

		MGlobal::displayInfo("Static Mesh\n");
		cout << "***** RunBSP *****" << endl;

		uselastfile = false;

		// Parse the arguments.
		for ( i = 0; i < args.length(); i++ )
		{
			if (MString("-l") == args.asString( i, &status ) && MS::kSuccess == status )
				uselastfile = true;
			else
				filename = args.asString(i);
		}

		// Check for invalid filenames
		if(uselastfile && s_lastfile.length())
			filename = s_lastfile;
		else if((uselastfile && !s_lastfile.length()) || !filename.length())
			filename = GetFilename();
		
		Run(filename);
		s_lastfile = filename;

		return MS::kSuccess;
	}
	MString GetFilename()
	{
		MString filename;
		MCommandResult result;

		MGlobal::executeCommand("fileDialog", result);
		result.getResult(filename);

		return filename;
	}
	void Run(MString filename)
	{
		MString command;
		MCommandResult result;
		
		command = "system(\"start c:/bsptools/bsp.exe -debug -nolight -o bsptest.bin " + filename + "\")";
		MGlobal::executeCommand(command, result);
	}
	static void* StaticCreate()
	{
		return new RunBSP;
	}
};

class NodeDump : public MPxCommand
{	
public:

	virtual MStatus doIt( const MArgList& args)
	{
		IterateNodes();
		return MS::kSuccess;
	}
	void IterateNodes()
	{
		MItDependencyNodes it(MFn::kInvalid);

		while(!it.isDone())
		{
			MObject obj;
			obj = it.item();

			//printf("type: %s\n", obj.apiTypeStr());
			MFnDependencyNode fnNode(obj);
			printf("name=%s typename=%s\n", fnNode.name().asChar(), fnNode.typeName().asChar());

			it.next();
		}
	}
	static void* StaticCreate()
	{
		return new NodeDump;
	}
};

class DagDump : public MPxCommand
{
public:

	virtual MStatus doIt( const MArgList& args)
	{
		IterateNodes();
		return MS::kSuccess;
	}
	void IterateNodes()
	{
		MItDag it(MItDag::kDepthFirst, MFn::kInvalid);

		while(!it.isDone())
		{
			MDagPath dagpath;
			it.getPath(dagpath);
			
			//printf("dagpath: %s\n", dagpath.fullPathName().asChar());
			cout << "dagpath:" << dagpath.fullPathName().asChar() << endl;
			
			it.next();		
		}
	}
	static void* StaticCreate()
	{
		return new DagDump;
	}
};

class PlaneTest : public MPxCommand
{
public:

	virtual MStatus doIt( const MArgList& args)
	{
		IterateNodes();
		return MS::kSuccess;
	}
	void IterateNodes()
	{
/*
		MItDependencyNodes it(MFn::kInvalid);

		MFnDependencyNode fnNode(it.item());

		// See if this node is a poly mesh
		if(fnNode.type() == MFn::kPolyMesh)
		{
// Result:message caching isHistoricallyInteresting nodeState binMembership output axis axisX axisY axisZ paramWarn uvSetName width height subdivisionsWidth subdivisionsHeight texture createUVs//


			fnNode.findPlug("subdivisionsWidth").getValue();
			fnNode.findPlug("subdivisionsHeight").getValue();

			MPlug plug;
		}
*/

		MItDag it(MItDag::kDepthFirst, MFn::kMesh);

		if(it.item().apiType() == MFn::kMesh)
		{}

		while(!it.isDone())
		{
			MDagPath dagpath;
			it.getPath(dagpath);
			
			MFnDependencyNode fnNode(it.currentItem());

			MPlugArray plugarray;
			fnNode.findPlug("inMesh").connectedTo(plugarray, true, false);
			int count = plugarray.length();
			MFn::Type type = plugarray[0].node().apiType();

			MFnDependencyNode fnPlaneNode(plugarray[0].node());

			int swidth, sheight;
			fnPlaneNode.findPlug("subdivisionsWidth").getValue(swidth);
			fnPlaneNode.findPlug("subdivisionsHeight").getValue(sheight);

			// Dump the mesh data
			MFnMesh fnMesh(dagpath);
			MPointArray vertices;
			fnMesh.getPoints(vertices, MSpace::kObject);
	
			it.next();
		}
	}

	static void* StaticCreate()
	{
		return new PlaneTest;
	}
};

typedef struct exportjoint_s
{
	//MDagPath dagpath;
	MFnDagNode *dagnode;
	MString name;
	int index;
	struct exportjoint_s *parent;
	MVector pos;
	MQuaternion rot;
	MMatrix bindmat;	// takes an object space vertex and puts it into joint space

} exportjoint_t;

class MeshExport : public MPxCommand
{
public:

	exportjoint_t joints[256];
	int numjoints;

	FILE* meshfp;

	virtual MStatus doIt( const MArgList& args)
	{
		meshfp = fopen("c:\\temp\\mesh.txt", "w");

		ExtractJoints();
		WriteJoints();
		ExtractMeshes();
		AnimTest2();
		//VertexWeightTest();
		//AnimTest();

		fclose(meshfp);

		return MS::kSuccess;
	}

	exportjoint_t *AllocJoint()
	{
		exportjoint_t *joint;

		joint = joints + numjoints;
		joint->index = numjoints;
		joint->parent = NULL;
		numjoints++;

		return joint;
	}
	exportjoint_t *FindJoint(MString name)
	{
		int i;

		for(i = 0; i < numjoints; i++)
			if(name == joints[i].name)
				return joints + i;

		return NULL;
	}

	// Find the top level joint, recursively walk the heirachy
	MVector GetJointTranslation(MFnDagNode &dagnode)
	{
		MTransformationMatrix matrix(dagnode.transformationMatrix());
		return matrix.getTranslation(MSpace::kTransform);
	}
	MQuaternion GetJointRotation(MFnDagNode &dagnode)
	{
		MTransformationMatrix matrix(dagnode.transformationMatrix());
		return matrix.rotation();
	}
	MMatrix GetJointBindMatrix(MFnDagNode &dagnode)
	{
		MDagPath dagpath;
		dagnode.getPath(dagpath);
		
		return dagpath.inclusiveMatrix();
	}
	exportjoint_t *GetJointParent(MObject jointnode)
	{
		MFnDagNode dagnode(jointnode);
		MFnDagNode parentdagnode(dagnode.parent(0));

		return FindJoint(parentdagnode.name());
	}
	void WalkTree(MFnDagNode &dagnode, exportjoint_t *parentjoint)
	{
		exportjoint_t *joint;
		MStatus status;
		int i;
		
		joint = AllocJoint();
		joint->dagnode = new MFnDagNode(dagnode);
		joint->name = joint->dagnode->name();
		joint->parent = parentjoint;
		joint->pos = GetJointTranslation(dagnode);
		joint->rot = GetJointRotation(dagnode);
		joint->bindmat = GetJointBindMatrix(dagnode);
		
		// Process all the children
		for(i = 0; i < joint->dagnode->childCount(); i++)
		{
			MFnDagNode childnode(joint->dagnode->child(i, &status));
			WalkTree(childnode, joint);
		}
	}
	void ExtractJoints()
	{
		numjoints = 0;

		MItDag it(MItDag::kDepthFirst, MFn::kJoint);
		for(; !it.isDone(); it.next())
		{
			MDagPath dagpath;
			it.getPath(dagpath);
			
			MFnDagNode dagnode(dagpath);
			MObject parent = dagnode.parent(0);
			if(parent.apiType() == MFn::kWorld)
			{
				WalkTree(dagnode, NULL);
				return;
			}
		}
	}

	// joint name parent pos rot
	void WriteJoints()
	{
		int i;
		exportjoint_t *joint;
		FILE *fp = meshfp;

		fprintf(fp, "joints\n");
		fprintf(fp, "{\n");

		for(i = 0; i < numjoints; i++)
		{
			joint = joints + i;
			fprintf(fp, "\"%s\" %i %f %f %f %f %f %f %f\n",
				joint->name.asChar(),
				(joint->parent ? joint->parent->index : -1),
				joint->pos.x, joint->pos.y, joint->pos.z,
				joint->rot.x, joint->rot.y, joint->rot.z, joint->rot.w);
		}

		fprintf(fp, "}\n");
		fprintf(fp, "\n");
		fflush(fp);
	}

	void GetTriangleLocalVertexIndicies(MDagPath obj, int polyindex, int triindex, MIntArray& indicies)
	{
		MFnMesh meshFn(obj);

		MIntArray polyindicies;
		meshFn.getPolygonVertices(polyindex, polyindicies);

		int triindicies[3];
		meshFn.getPolygonTriangleVertices(polyindex, triindex, triindicies);
	
		for(int i = 0; i < 3; i++)
		{
			// Match the triangle index to one of the polygons indicies
			for(int j = 0; j < polyindicies.length(); j++)
			{
				if(triindicies[i] == polyindicies[j])
				{
					indicies.append(j);
					break;
				}
			}
		}
	}
	
	void DumpMesh(MDagPath dagpath)
	{
		FILE *fp = meshfp;

		MFnDagNode node(dagpath);

		// Hack to stop dump "Orig" meshes
		if(strstr(node.name().asChar(), "Orig"))
			return;

		fprintf(fp, "mesh \"%s\" \"default\"\n", node.name().asChar());
		fprintf(fp, "{\n");

		//MItDag it(MItDag::kDepthFirst, MFn::kMesh);
		//for(; !it.isDone(); it.next())
		{
			MFnMesh mesh(dagpath);

			// Extract the vertices
			MPointArray vertices;
			mesh.getPoints(vertices, MSpace::kWorld);

			// Extract the uv's
			MFloatArray uarray, varray;
			mesh.getUVs(uarray, varray);

			// Extract the normals
			MFloatVectorArray normals;
			mesh.getNormals(normals, MSpace::kWorld);

			MItMeshPolygon polyit(dagpath);

			while(!polyit.isDone())
			{
				int i, numtriangles;

				polyit.numTriangles(numtriangles);
				for(i = 0; i < numtriangles; i++)
				{
					MPointArray trivertices;
					MIntArray triindicies;
					polyit.getTriangle(i, trivertices, triindicies, MSpace::kWorld);

					// Get the local vertex indicies
					MIntArray local;
					GetTriangleLocalVertexIndicies(dagpath, polyit.index(), i, local);

					int j;

					// normals
					int normalindex[3];
					for(int j = 0; j < 3; j++)
						normalindex[j] = polyit.normalIndex(local[j]);

					// UVs
					int uvindex[3];
					for(int j = 0; j < 3; j++)
						 polyit.getUVIndex(local[j], uvindex[j]);

					fprintf(fp, "triangle\n{\n");
					for(j = 0; j < 3; j++)
					{
						fprintf(fp, "%f %f %f %f %f %f %f %f\n",
							vertices[triindicies[j]].x, vertices[triindicies[j]].y, vertices[triindicies[j]].z,
							normals[normalindex[j]].x, normals[normalindex[j]].y, normals[normalindex[j]].z,
							uarray[uvindex[j]], varray[uvindex[j]]
						);
					}
					fprintf(fp, "}\n");
				}

				polyit.next();
			}
		}

		fprintf(fp, "}\n");
	}	

	void ExtractMeshes()
	{
		MItDag it(MItDag::kDepthFirst, MFn::kMesh);
		for(; !it.isDone(); it.next())
		{	
			MDagPath dagpath;
			it.getPath(dagpath);

			DumpMesh(dagpath);
		}

		/*
		MItDag it2(MItDag::kDepthFirst, MFn::kTransform);
		for(; !it2.isDone(); it2.next())
		{	
			MDagPath dagpath;
			it2.getPath(dagpath);

			MFnDagNode node(dagpath);
			cout << "Transform " << node.name().asChar() << endl;
		}
		*/
	}

	void VertexWeightTest()
	{
		MStatus status;

		fprintf(stdout, "Influence list:\n");
		MItDependencyNodes it(MFn::kSkinClusterFilter);
		for(; !it.isDone(); it.next())
		{
			MDagPathArray infs;
			int numinfs;
			MFnSkinCluster skin(it.item());

			// Iterate the "influence (joint) list"		
			numinfs = skin.influenceObjects(infs, &status);
			for(int i = 0; i < numinfs; i++)
			{
				fprintf(stdout, "influence %i: %s\n", i, infs[i].fullPathName().asChar());
			}

			// Iterate the mesh
			MObjectArray dagnodelist;
			skin.getOutputGeometry(dagnodelist);
			
			MFnDagNode dagnode(dagnodelist[0], &status);
			
			MDagPath dagpath;
			dagnode.getPath(dagpath);

			// For each vertex
			fprintf(stdout, "weights:\n");
			MItGeometry geomit(dagpath);
			for(; !geomit.isDone(); geomit.next())
			{
				// Get the weights for this vertex
				MDoubleArray weights;
				unsigned int count;
				skin.getWeights(dagpath, geomit.currentItem(), weights, count);
				fprintf(stdout, "vertex %i: ", geomit.index());

				for(int i = 0; i < count; i++)
					fprintf(stdout, "%f ", weights[i]); 

				fprintf(stdout, "\n");
			}
		}



		fflush(stdout);
	}

	// for each frame
		// for each joint
	void AnimSetFrame(int frame)
	{
		MTime time;
			
		// set time unit to 24 frames per second
		time.setUnit(MTime::kFilm);
		time.setValue(frame);
		MGlobal::viewFrame(time);
	}
	void AnimTest2()
	{
		MStatus status;
		int i;

		FILE* fp = meshfp;

		fprintf(fp, "anim\n");
		fprintf(fp, "{\n");

		for(i = 1; i <= 24; i++)
		{
			fprintf(fp, "frame\n");	 
			fprintf(fp, "{\n");
			
			// Set the frame
			AnimSetFrame(i);

			// Process each joint
			MItDag it(MItDag::kDepthFirst, MFn::kJoint);
			for(; !it.isDone(); it.next())
			{
				MVector pos;
				MQuaternion rot;
				
				MDagPath dagpath;
				it.getPath(dagpath);
				//it.currentItem instead???

				MFnDagNode dagnode(dagpath);

				pos = GetJointTranslation(dagnode);
				rot = GetJointRotation(dagnode);

				// fixme: these will be written out in the correct order...
				fprintf(fp, "%f %f %f %f %f %f %f\n",
					pos.x, pos.y, pos.z,
					rot.x, rot.y, rot.z, rot.w);
			}

			fprintf(fp, "}\n");
		}

		fprintf(fp, "}\n");
		fprintf(fp, "\n");
		fflush(stdout);
	}

	void AnimTest()
	{
		MStatus status;

		MItDag it(MItDag::kDepthFirst, MFn::kJoint);
		for(; !it.isDone(); it.next())
		{
			MDagPath dagpath;
			it.getPath(dagpath);

			MFnDagNode dagnode(dagpath);
			//MDagPath dagpath(it.getPath());
			//MFnDagNode dagnode(it.item(), &status);
	
			if(dagnode.name() != MString("fan"))
				continue;

			// Note maya frames start at 1 so we need [1 24] not [0 23] 
			for(int i = 0; i < 24; i++)
			{
				AnimSetFrame(i);

				MFnTransform fnTransform(dagpath.transform());
				MVector translation = fnTransform.getTranslation(MSpace::kTransform);
				MEulerRotation er;
				fnTransform.getRotation(er);

				float radtodeg = (180.0f / 3.14152f);
				fprintf(stdout, "frame %i\n", i);
				fprintf(stdout, "trs: %f %f %f\n", translation.x, translation.y, translation.z);
				fprintf(stdout, "rot: %f %f %f\n", er.x * radtodeg, er.y * radtodeg, er.z * radtodeg);
				fflush(stdout);
			}
		}
	}
	
	static void* StaticCreate()
	{
		return new MeshExport;
	}
};

__declspec(dllexport) MStatus initializePlugin(MObject obj)
{
	MFnPlugin pluginFn(obj, "brycek", "1.0f");
	MStatus stat;

	stat = pluginFn.registerCommand("exportbsp", HelloWorld2Cmd::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("itest", InstanceTest::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("smesh", StaticMesh::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("brush", Brush::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("runbsp", RunBSP::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("nodedump", NodeDump::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("dagdump", DagDump::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("planetest", PlaneTest::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");
	stat = pluginFn.registerCommand("meshexport", MeshExport::StaticCreate);
	if(!stat)
		stat.perror("registerCommand failed");

	return stat;
}

__declspec(dllexport) MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin pluginFn(obj);
	MStatus stat;

	stat = pluginFn.deregisterCommand("exportbsp");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("itest");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("smesh");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("brush");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("runbsp");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("nodedump");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("dagdump");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("planetest");
	if(!stat)
		stat.perror("deregisterCommand failed");
	stat = pluginFn.deregisterCommand("meshexport");
	if(!stat)
		stat.perror("deregisterCommand failed");
	return stat;
}
