﻿using System;
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

    public Mesh genMesh(String fileName)
    {
        List<Mesh> sections = new List<Mesh>();
        String text = File.ReadAllText(fileName);
        Debug.Log(text);
        String[] lines = text.Split('\n');
        List<Vector3> verts = new List<Vector3>();
        List<int> tris = new List<int>();

        foreach (String line in lines)
        {
            if (line.Equals("")) continue;
            String[] l = line.Split(' ');
            char type = line.ToCharArray()[0];
            switch (type)
            {
                case 'o':
                    
                    break;
                case 'v':
                    verts.Add(new Vector3(float.Parse(l[1]), float.Parse(l[3]), -float.Parse(l[2])));
                    break;
                case 'f':
                    if (l.Length == 4)
                    {
                        tris.Add(Int32.Parse(l[1]) - 1);
                        tris.Add(Int32.Parse(l[2]) - 1);
                        tris.Add(Int32.Parse(l[3]) - 1);
                    }

                    break;
            }
        }
        
        Mesh myMesh = new Mesh();
        myMesh.vertices = verts.ToArray();
        myMesh.triangles = tris.ToArray();

        Vector2[] uvs = new Vector2[verts.Count];

                        
                        
        for (int i = 0; i < uvs.Length; i++)
        {
            uvs[i] =  Quaternion.AngleAxis(20, Vector3.right) * (new Vector2(verts[i].x, verts[i].z));
        }

        myMesh.uv = uvs;
        

        verts.Clear();
        tris.Clear();

        return myMesh;
    }
    
    public List<Tuple<Vector3, Vector3>> getPavementAnchors(String fileName)
    {
        List<Tuple<Vector3, Vector3>> anchors = new List<Tuple<Vector3, Vector3>>();
        
        String text = File.ReadAllText(fileName);
        Debug.Log(text);
        String[] lines = text.Split('\n');
        int pair_index = 0;
        Vector3 v = new Vector3();
        
        foreach (String line in lines)
        {
            if (line.Equals("")) continue;
            String[] l = line.Split(' ');
            char type = line.ToCharArray()[0];
            Debug.Log(type);
            switch (type)
            {
                case 'o':

                    break;
                case 'v':
                    if (pair_index == 0)
                    {
                        v = new Vector3(float.Parse(l[1]), float.Parse(l[3]), -float.Parse(l[2]));
                        pair_index = 1;
                    }
                    else
                    {
                        Debug.Log("Added");
                        anchors.Add(new Tuple<Vector3, Vector3>(v,
                            new Vector3(float.Parse(l[1]), float.Parse(l[3]), -float.Parse(l[2]))));
                        pair_index = 0;
                    }

                    break;
                case 'f':

                    break;
            }
        }

        return anchors;
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