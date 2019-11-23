using System;
using System.IO;
using UnityEngine;
using UnityEngine.UI;


public class RoadGen : MonoBehaviour
{

    private void Start()
    {
        GenRoad();
    }

    private void GenRoad()
    {
        //Camera c  = Camera.main;
        //c.enabled = true;
        //Mesh mesh = FastObjImporter.Instance.ImportFile("C:\\Users\\elias\\Roads\\Assets\\square.obj");
        //GameObject r = new GameObject("MyObject");
        
        ObjToMesh o = new ObjToMesh();
        Mesh importedMesh = o.genMesh("C:\\Users\\elias\\Roads\\Assets\\square.obj");

        // Mesh imported_mesh = FastObjImporter.Instance.ImportFile("C:\\Users\\elias\\Roads\\Assets\\square.obj");
        Mesh mesh = new Mesh();
        
        mesh.Clear ();
        mesh.vertices = importedMesh.vertices;
        mesh.triangles = importedMesh.triangles;
        mesh.RecalculateNormals();

        GameObject go = new GameObject();
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().sharedMesh = mesh;
        go.GetComponent<MeshRenderer>().material.SetTexture("_MainTexture", LoadPNG("C:\\Users\\elias\\Roads\\Assets\\asphalt.png"));


        // Instantiate(go, Vector3.zero, Quaternion.identity);



        /*
        Color[] colors = new Color[vertices.Length];
        for (int i = 0; i < vertices.Length; i++)
            colors[i] = Color.Lerp(Color.red, Color.green, vertices[i].y); // assign the array of colors to the Mesh.
        mesh.colors = colors;

        r.AddComponent<MeshRenderer>();
        r.AddComponent<MeshFilter>().mesh = mesh;

        Material cubeMaterial = new Material(Shader.Find("Standard"));
        cubeMaterial.SetColor("_Color", new Color(0f, 0.7f, 0f)); //green main color
        r.GetComponent<Renderer>().material = cubeMaterial;
        */

        //Instantiate(mesh, Vector3.zero, Quaternion.identity);
    }

    public static Texture2D LoadPNG(string filePath)
    {
        Texture2D tex = null;
        byte[] fileData;
        if (File.Exists(filePath))
        {
            fileData = File.ReadAllBytes(filePath);
            tex = new Texture2D(2, 2);
            tex.LoadImage(fileData); //..this will auto-resize the texture dimensions.
        }

        return tex;
    }

    private void Update()
    {
       
    }
}