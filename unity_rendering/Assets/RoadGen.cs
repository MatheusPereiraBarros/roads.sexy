using System;
using System.IO;
using UnityEngine;
using UnityEngine.UI;
using Random = System.Random;


public class RoadGen : MonoBehaviour
{
    private void Start()
    {
        //GenTerrain();
        GenRoad();
    }

    private void GenRoad()
    {
        ObjToMesh o = new ObjToMesh();
        Mesh importedMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\road.obj");

        Mesh m = importedMesh;

        Mesh mesh = new Mesh();
        mesh.Clear ();
        mesh.vertices = m.vertices;
        mesh.triangles = m.triangles;
        mesh.RecalculateNormals();


        GameObject go = new GameObject();
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().sharedMesh = m;
        go.GetComponent<MeshRenderer>().material.mainTexture = Resources.Load("asphalt") as Texture;
    }


    private void GenTerrain()
    {

        ObjToMesh o = new ObjToMesh();
        Mesh importedMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\terrain.obj");

        Mesh m = importedMesh;

        Mesh mesh = new Mesh();
        mesh.Clear ();
        mesh.vertices = m.vertices;
        mesh.triangles = m.triangles;
        mesh.RecalculateNormals();


        GameObject go = new GameObject();
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().sharedMesh = m;
        go.GetComponent<MeshRenderer>().material.mainTexture = Resources.Load("grass") as Texture;

        float xMax = o.getMaxX(m.vertices);
        float zMax = o.getMaxZ(m.vertices);
        float zMin = o.getMinZ(m.vertices);
        float xMin = o.getMinX(m.vertices);

        Random r = new Random();
        int x = r.Next((int) xMin, (int) xMax);
        int z = r.Next((int) zMin, (int) zMax);

        for (int i = 0; i < m.vertexCount/10; i++)
        {
            int v_index = r.Next(0, m.vertexCount);
            Vector3 v = m.vertices[v_index];
            Debug.Log(v);
            GameObject grassObjectPrefab = Resources.Load<GameObject>("LowGrass") as GameObject;
            Debug.Log(grassObjectPrefab);
            GameObject grassObject = GameObject.Instantiate(grassObjectPrefab, v, Quaternion.identity);
            grassObject.transform.localScale = new Vector3(10, 10, 10);
            
            grassObject.GetComponent<MeshRenderer>().material.mainTexture = Resources.Load("grass") as Texture;
        }



    }

    private void Update()
    {
       
    }
}