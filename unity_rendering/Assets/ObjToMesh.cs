using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;
using Random = System.Random;

public sealed class ObjToMesh
{
    public ObjToMesh()
    {
    }

    public Mesh[] genMesh(String fileName)
    {
        List<Mesh> sections = new List<Mesh>();
        String text = File.ReadAllText(fileName);
        Debug.Log(text);
        String[] lines = text.Split('\n');
        List<Vector3> verts = new List<Vector3>();
        List<int> tris = new List<int>();

        foreach (String line in lines)
        {
            String[] l = line.Split(' ');
            char type = line.ToCharArray()[0];
            switch (type)
            {
                case 'o':
                    Debug.Log("Object Finished");
                    // if there were previously scanned .obj files generate mesh
                    if ((verts.Count > 0) && (tris.Count > 0))
                    {
                        Mesh myMesh = new Mesh();
                        myMesh.vertices = verts.ToArray();
                        myMesh.triangles = tris.ToArray();

                        Vector2[] uvs = new Vector2[verts.Count];

                        
                        
                        for (int i = 0; i < uvs.Length; i++)
                        {
                            uvs[i] = new Vector2(verts[i].x, verts[i].z);
                        }

                        myMesh.uv = uvs;
                        
                        sections.Add(myMesh);

                        verts.Clear();
                        tris.Clear();
                    }

                    break;
                case 'v':
                    verts.Add(new Vector3(float.Parse(l[1]), float.Parse(l[3]), float.Parse(l[2])));
                    break;
                case 'f':
                    if (l.Length == 4)
                    {
                        tris.Add(Int32.Parse(l[1])-1);
                        tris.Add(Int32.Parse(l[2])-1);
                        tris.Add(Int32.Parse(l[3])-1);
                    }
                    else if (l.Length == 5)
                    {
                        tris.Add(Int32.Parse(l[1])-1);
                        tris.Add(Int32.Parse(l[2])-1);
                        tris.Add(Int32.Parse(l[3])-1);
                        tris.Add(Int32.Parse(l[4])-1);
                    }

                    break;
            }

            if (l[0] == "o")
            {
            }
            else if (l[0] == "v")
            {
            }
            else if (l[0] == "f")
            {
            }
        }

        return sections.ToArray();
    }

    public float getMaxX(Vector3[] arr)
    {
        float max = Int32.MinValue;
        foreach (var v in arr)
        {
            if (v.x > max)
            {
                max = v.x;
            }
        }

        return max;
    }
    
    public float getMaxY(Vector3[] arr)
    {
        float max = Int32.MinValue;
        foreach (var v in arr)
        {
            if (v.y > max)
            {
                max = v.y;
            }
        }

        return max;
    }
    
    public float getMaxZ(Vector3[] arr)
    {
        float max = Int32.MinValue;
        foreach (var v in arr)
        {
            if (v.z > max)
            {
                max = v.z;
            }
        }

        return max;
    }
    
    public float getMinX(Vector3[] arr)
    {
        float min = Int32.MaxValue;
        foreach (var v in arr)
        {
            if (v.z < min)
            {
                min = v.z;
            }
        }

        return min;
    }
    
    public float getMinY(Vector3[] arr)
    {
        float min = Int32.MaxValue;
        foreach (var v in arr)
        {
            if (v.y > min)
            {
                min = v.y;
            }
        }

        return min;
    }
    
    public float getMinZ(Vector3[] arr)
    {
        float min = Int32.MaxValue;
        foreach (var v in arr)
        {
            if (v.z > min)
            {
                min = v.z;
            }
        }

        return min;
    }
}