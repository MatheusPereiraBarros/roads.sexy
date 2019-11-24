using System;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;
using Random = System.Random;


public class RoadGen : MonoBehaviour
{
    public MeshCollider roadMeshCollider;
    
    private void Start()
    {

        GenHouse();
        //GenRoad();
        //GenTerrain();
    }

    private void GenHouse()
    {
        ObjToMesh o = new ObjToMesh();
        List<Tuple<Vector3, Vector3>> roadMesh = o.getPavementAnchors("D:\\roads.sexy\\unity_rendering\\Assets\\testAnchors.obj");
        Debug.Log(roadMesh[0].Item1.ToString());
        
    }

    private void GenRoad()
    {
        ObjToMesh o = new ObjToMesh();
        Mesh roadMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\objects\\streets.obj");
        Mesh curbMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\objects\\border.obj");
        Mesh stripeMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\objects\\markings.obj");
        Mesh pavementMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\objects\\sidewalk.obj");

        // Road Asphalt Surface 
        GameObject go = new GameObject();
        Texture asphaltTexture = Resources.Load("asphalt") as Texture;
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().mesh = roadMesh;
        //go.transform.eulerAngles = new Vector3(-90, 0, 0);
        go.GetComponent<MeshRenderer>().material.mainTexture = asphaltTexture;
        go.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 0.1, (float) 0.1);
        go.AddComponent<MeshCollider>();
        
        roadMeshCollider = go.GetComponent<MeshCollider>();
        roadMeshCollider.convex = false;
        Debug.Log(roadMeshCollider);
        
        /*
        // Road Asphalt Cracks
        Texture asphaltCrackTexture = Resources.Load("crack") as Texture;
        GameObject goCrack = new GameObject();
        goCrack.AddComponent<MeshFilter>();
        goCrack.AddComponent<MeshRenderer>();
        goCrack.GetComponent<MeshFilter>().mesh = roadMesh;
        goCrack.GetComponent<MeshRenderer>().material.mainTexture = asphaltCrackTexture;
        //goCrack.GetComponent<MeshRenderer>().material.SetFloat("_Mode", 1);
        //goCrack.GetComponent<MeshRenderer>().material.DisableKeyword("_ALPHATEST_ON");
        //goCrack.GetComponent<MeshRenderer>().material.EnableKeyword("_ALPHABLEND_ON");
        goCrack.transform.eulerAngles = new Vector3(-90, 0, 0);
        goCrack.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 0.1, (float) 0.1);
        */
        
        // Road Curbs
        Texture curbTexture = Resources.Load("pavement_texture") as Texture;
        GameObject goCurb = new GameObject();
        goCurb.AddComponent<MeshFilter>();
        goCurb.AddComponent<MeshRenderer>();
        goCurb.GetComponent<MeshFilter>().mesh = curbMesh;
        goCurb.GetComponent<MeshRenderer>().material.mainTexture = curbTexture;
        //  goCurb.transform.eulerAngles = new Vector3(-90, 0, 0);
        goCurb.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 1, (float) 1);
        
        // Road Stripes
        Texture stripsTexture = Resources.Load("line_texture") as Texture;
        GameObject goStrips = new GameObject();
        goStrips.AddComponent<MeshFilter>();
        goStrips.AddComponent<MeshRenderer>();
        goStrips.GetComponent<MeshFilter>().mesh = stripeMesh;
        goStrips.GetComponent<MeshRenderer>().material.mainTexture = stripsTexture;
        // goStrips.transform.eulerAngles = new Vector3(-90, 0, 0);
        goStrips.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 0.5, (float) 0.5);
        
        // Road Sidewalk
        Texture sideWalkTexture = Resources.Load("pavement") as Texture;
        GameObject goSideWalk = new GameObject();
        goSideWalk.AddComponent<MeshFilter>();
        goSideWalk.AddComponent<MeshRenderer>();
        goSideWalk.GetComponent<MeshFilter>().mesh = pavementMesh;
        goSideWalk.GetComponent<MeshRenderer>().material.mainTexture = sideWalkTexture;
        // goSideWalk.transform.eulerAngles = new Vector3(-90, 0, 0);
        goSideWalk.GetComponent<MeshRenderer>().material.mainTextureScale = new Vector2((float) 0.75, (float) 0.75);
        
    }


    private void GenTerrain()
    {

        ObjToMesh o = new ObjToMesh();
        Mesh importedMesh = o.genMesh("D:\\roads.sexy\\unity_rendering\\Assets\\objects\\terrain.obj");

        GameObject go = new GameObject();
        go.AddComponent<MeshFilter>();
        go.AddComponent<MeshRenderer>();
        go.GetComponent<MeshFilter>().mesh = importedMesh;
        // go.transform.eulerAngles = new Vector3(-90, 0, 0);
        go.GetComponent<MeshRenderer>().material.mainTexture = Resources.Load("grass") as Texture;
        go.GetComponent<MeshRenderer>().material.shader = Shader.Find("Nature/SpeedTree");
                

        Random r = new Random();
        for (int i = 0; i < importedMesh.vertexCount / 10; i++)
        {
            int v_index = r.Next(0, importedMesh.vertexCount);
            Vector3 v = importedMesh.vertices[v_index];


            if (!IsInCollider(roadMeshCollider, v))
            {
                GameObject grassObjectPrefab = Resources.Load<GameObject>("LowGrass") as GameObject;
                GameObject grassObject = GameObject.Instantiate(grassObjectPrefab, v, Quaternion.identity);
                grassObject.transform.localScale = new Vector3(5, 5, 5);

                grassObject.GetComponent<MeshRenderer>().material.mainTexture = Resources.Load("grass") as Texture;
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