#if 0	

void IterateNodes()
	{
		MItDependencyNodes it(MFn::kInvalid);

		while(!it.isDone())
		{
			MObject obj;
			obj = it.item();

			cout << obj.apiTypeStr() << endl;

			it.next();
		}
	}


	void IterateSelection()
	{
		unsigned int i;
		MSelectionList list;
		MDagPath dagpath;
		MFnDagNode fnnode;

		MGlobal::getActiveSelectionList(list);

		for(i = 0; i < list.length(); i++)
		{
			list.getDagPath(i, dagpath);
			fnnode.setObject(dagpath);
			cout << fnnode.name().asChar() << " of type " << fnnode.typeName().asChar() << " is selected" << endl;

			cout << "has " << fnnode.childCount() << " children" << endl;

			//Iterate the children
			for(int j = 0; j < fnnode.childCount(); j++)
			{
				MObject childobj;
				MFnDagNode fnchild;

				childobj = fnnode.child(j);
				fnchild.setObject(childobj);

				cout << "child " << j << " is a " << fnchild.typeName().asChar() << endl;

				//MFn::Type type = fnchild.type()
				//if(fnchild.type() == MFn::kMesh)
				//DumpMesh(dagpath);
				//DumpMesh2(dagpath);
				IterateWorldMeshesInSelection();
			}
		}
	}

	void DumpMesh(MDagPath dagpath)
	{
		MStatus status;
		MPointArray points;

		for(MItMeshPolygon it(dagpath); !it.isDone(); it.next())
		{
			it.getPoints(points, MSpace::kWorld);
			
			for(int i = 0; i < points.length(); i++)
			{
				cout << "i: " << i << " "
					<< points[i].x << " "
					<< points[i].y << " "
					<< points[i].z << " "
					<< points[i].w << endl;
			}
		}
	}

	void DumpMesh2(MDagPath dagpath)
	{
		MFnMesh fnmesh(dagpath);
		int numpolygons;
		MPointArray vertices;
		int i, j;

		numpolygons = fnmesh.numPolygons();
		fnmesh.getPoints(vertices, MSpace::kWorld);
	
		cout << "Mesh has " << numpolygons << "polygons";

		cout << "Vertex list:" << endl;
		for(i = 0; i < vertices.length(); i++)
		{
				cout << i << " "
					<< vertices[i].x << " "
					<< vertices[i].y << " "
					<< vertices[i].z << " "
					<< vertices[i].w << endl;
		}

		MIntArray indexlist;

		for(i = 0; i < numpolygons; i++)
		{
			fnmesh.getPolygonVertices(i, indexlist);
			cout << "polygon " << i << ": ";
			for(j = 0; j < indexlist.length(); j++)
			{
				cout << indexlist[j] << ", ";
			}

			cout << endl;
		}
	}

	void DoFilename()
	{
		MString filename;
		MCommandResult result;

		MGlobal::executeCommand("fileDialog -m 1", result);
		result.getResult(filename);

		cout << "filename is: " << filename.asChar() << endl;
	}

#endif