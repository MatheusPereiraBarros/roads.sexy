using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

public sealed class ObjToMesh
{
    
    public ObjToMesh()
    {
        
    }

    public Mesh genMesh(String fileName)
    {
        String text = File.ReadAllText(fileName);
        String[] lines = text.Split('\n');
        List<Vector3> verts = new List<Vector3>();
        List<int> tris = new List<int>();
        
        foreach (String line in lines)
        {
            Debug.Log(line);
            String[] l = line.Split(' ');

            if (l[0] == "v")
            {
                verts.Add(new Vector3(float.Parse(l[1]), float.Parse(l[2]), float.Parse(l[3])));
            }else if (l[0] == "f")
            {
                
                tris.Add(Int32.Parse(l[1]));
                tris.Add(Int32.Parse(l[2]));
                tris.Add(Int32.Parse(l[3]));
            }
        }
        
        Mesh myMesh = new Mesh();
        myMesh.vertices = verts.ToArray();
        myMesh.triangles = tris.ToArray();

        return myMesh;
    }
}
