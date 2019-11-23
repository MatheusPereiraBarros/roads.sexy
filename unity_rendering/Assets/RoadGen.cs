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


    private void GenTerrain()
    {

        ObjToMesh o = new ObjToMesh();
        Mesh[] importedMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\terrain.obj");

        // Mesh imported_mesh = FastObjImporter.Instance.ImportFile("C:\\Users\\elias\\Roads\\Assets\\square.obj");

        Mesh m = importedMesh[0];

        Mesh mesh = new Mesh();
        mesh.Clear ();
        mesh.vertices = m.vertices;
        mesh.triangles = m.triangles;
        mesh.RecalculateNormals();
            
            
        GameObject go = new GameObject();
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().sharedMesh = m;
            

        
        

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

    /*
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
    */

    private void Update()
    {
       
    }
}