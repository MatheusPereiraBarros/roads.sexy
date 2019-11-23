using System;
using System.IO;
using UnityEngine;
using UnityEngine.UI;
using Random = System.Random;


public class RoadGen : MonoBehaviour
{
    public MeshCollider roadMeshCollider;
    
    private void Start()
    {
        GenRoad();
        GenTerrain();
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
        
        Texture ttt = Resources.Load("asphalt") as Texture;
        Texture tt = Resources.Load("crack") as Texture;


        GameObject go = new GameObject();
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().mesh = m;
        go.GetComponent<MeshRenderer>().material.mainTexture = ttt;
        go.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 0.1, (float) 0.1);
        go.AddComponent<MeshCollider>();
        roadMeshCollider = go.GetComponent<MeshCollider>();
        roadMeshCollider.convex = false;
        Debug.Log(roadMeshCollider);

        GameObject goCrack = new GameObject();
        goCrack.AddComponent<MeshFilter>();
        goCrack.AddComponent<MeshRenderer>();
        goCrack.GetComponent<MeshFilter>().mesh = m;
        goCrack.GetComponent<MeshRenderer>().material.mainTexture = tt;
        //goCrack.GetComponent<MeshRenderer>().material.SetFloat("_Mode", 1);
        //goCrack.GetComponent<MeshRenderer>().material.DisableKeyword("_ALPHATEST_ON");
        //goCrack.GetComponent<MeshRenderer>().material.EnableKeyword("_ALPHABLEND_ON");
        goCrack.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 0.1, (float) 0.1);
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
        mesh.RecalculateBounds();


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

        for (int i = 0; i < m.vertexCount / 100; i++)
        {
            int v_index = r.Next(0, m.vertexCount);
            Vector3 v = m.vertices[v_index];

            if (!IsInCollider(roadMeshCollider, v))
            {
                GameObject grassObjectPrefab = Resources.Load<GameObject>("LowGrass") as GameObject;
                GameObject grassObject = GameObject.Instantiate(grassObjectPrefab, v, Quaternion.identity);
                grassObject.transform.localScale = new Vector3(5, 5, 5);

                grassObject.GetComponent<MeshRenderer>().material.mainTexture = Resources.Load("grass") as Texture;
            }
            
            if (!roadMeshCollider.bounds.Contains(v))
            {
                
            }

            
        }
    }
    
    public  bool IsInCollider(MeshCollider other, Vector3 point) {
        Vector3 from = (Vector3.up * 5000f);
        Vector3 dir = (point - from).normalized;
        float dist = Vector3.Distance(from, point);        
        //fwd      
        int hit_count = Cast_Till(from, point, other);
        //back
        dir = (from - point).normalized;
        hit_count += Cast_Till(point, point + (dir * dist), other);
 
        if (hit_count % 2 == 1) {
            return (true);
        }
        return (false);
    }

    int Cast_Till(Vector3 from, Vector3 to, MeshCollider other)
    {
        int counter = 0;
        Vector3 dir = (to - from).normalized;
        float dist = Vector3.Distance(from, to);
        bool Break = false;
        while (!Break)
        {
            Break = true;
            RaycastHit[] hit = Physics.RaycastAll(from, dir, dist);
            for (int tt = 0; tt < hit.Length; tt++)
            {
                if (hit[tt].collider == other)
                {
                    counter++;
                    from = hit[tt].point + dir.normalized * .001f;
                    dist = Vector3.Distance(from, to);
                    Break = false;
                    break;
                }
            }
        }

        return (counter);
    }

    private void Update()
    {
       
    }
}