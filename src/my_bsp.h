#ifndef MY_BSP_H
#define MY_BSP_H

#include <stdint.h>

#include "cglm.h"
#include "mat4.h"
#include "vec4.h"
#include "vec3.h"

void engine_pos_to_quake(vec3 p,vec3 result);
void quake_pos_to_engine(vec3 q, vec3 result);

// At the top of your BSP header file
#pragma pack(push, 1)
// BSP-specific vector (guaranteed 12 bytes, no padding)
typedef struct {
    float x;
    float y;
    float z;
} bsp_vec3;

typedef struct 
{
  int32_t    offset;     // offset (in bytes) of the data from the beginning of the file
  int32_t    size;     // length (in bytes) of the data
}dentry_t;

typedef struct 
{
  uint32_t    version;    // version of the BSP format (38)
  dentry_t entities;           // List of Entities.
  dentry_t planes;             // Map Planes.
                               // numplanes = size/sizeof(plane_t)
  dentry_t miptex;             // Wall Textures.
  dentry_t vertices;           // Map Vertices.
                               // numvertices = size/sizeof(vertex_t)
  dentry_t visilist;           // Leaves Visibility lists.
  dentry_t nodes;              // BSP Nodes.
                               // numnodes = size/sizeof(node_t)
  dentry_t texinfo;            // Texture Info for faces.
                               // numtexinfo = size/sizeof(texinfo_t)
  dentry_t faces;              // Faces of each surface.
                               // numfaces = size/sizeof(face_t)
  dentry_t lightmaps;          // Wall Light Maps.
  dentry_t clipnodes;          // clip nodes, for Models.
                               // numclips = size/sizeof(clipnode_t)
  dentry_t leaves;             // BSP Leaves.
                               // numlaves = size/sizeof(leaf_t)
  dentry_t lface;              // List of Faces.
  dentry_t edges;              // Edges of faces.
                               // numedges = Size/sizeof(edge_t)
  dentry_t ledges;             // List of Edges.
  dentry_t models;             // List of Models.
}dheader_t;

typedef struct                 // Bounding Box, Float values
{ bsp_vec3 min;                // minimum values of X,Y,Z
  bsp_vec3 max;                // maximum values of X,Y,Z
} boundbox_t;

typedef struct                 // Bounding Box, Short values
{
  int16_t minX,minY,minZ;                 // minimum values of X,Y,Z
  int16_t maxX,maxY,maxZ;                 // maximum values of X,Y,Z
} bboxshort_t;

typedef struct
{
  boundbox_t bound;            // The bounding box of the Model
  bsp_vec3 origin;               // origin of model, usually (0,0,0)
  int32_t node_id0;               // index of first BSP node
  int32_t node_id1;               // index of the first Clip node
  int32_t node_id2;               // index of the second Clip node
  int32_t node_id3;               // usually zero
  int32_t numleafs;               // number of BSP leaves
  int32_t face_id;                // index of Faces
  int32_t face_num;               // number of Faces
} model_t;

typedef struct
{ float X;                    // X,Y,Z coordinates of the vertex
  float Y;                    // usually some integer value
  float Z;                    // but coded in floating point
} vertex_t;

typedef struct
{ bsp_vec3 normal;               // Vector orthogonal to plane (Nx,Ny,Nz)
                               // with Nx2+Ny2+Nz2 = 1
  float dist;               // Offset to plane, along the normal vector.
                               // Distance from (0,0,0) to the plane
  int32_t    type;                // Type of plane, depending on normal vector.
} plane_t;

typedef struct  
{
  uint16_t plane_id;            // The plane in which the face lies
                               //           must be in [0,numplanes[ 
  uint16_t side;                // 0 if in front of the plane, 1 if behind the plane
  int32_t ledge_id;               // first edge in the List of edges
                               //           must be in [0,numledges[
  uint16_t ledge_num;           // number of edges in the List of edges
  uint16_t texinfo_id;          // index of the Texture info the face is part of
                               //           must be in [0,numtexinfos[ 
  uint8_t typelight;            // type of lighting, for the face
  uint8_t baselight;            // from 0xFF (dark) to 0 (bright)
  uint8_t light[2];             // two additional light models  
  int32_t lightmap;               // Pointer inside the general light map, or -1
                               // this define the start of the face light map
} face_t;

typedef struct
{
  uint16_t vertex0;             // index of the start vertex
                               //  must be in [0,numvertices[
  uint16_t vertex1;             // index of the end vertex
                               //  must be in [0,numvertices[
} edge_t;


typedef struct
{
  int32_t plane_id;            // The plane that splits the node
                               //           must be in [0,numplanes[
  
  //TODO: I changed the front and back children from unsigned to signed,according to the Quake source code
  
  int16_t front;               // If bit15==0, index of Front child node
                               // If bit15==1, ~front = index of child leaf
  int16_t back;                // If bit15==0, id of Back child node
                               // If bit15==1, ~back =  id of child leaf
  bboxshort_t box;             // Bounding box of node and all childs
  uint16_t face_id;             // Index of first Polygons in the node
  uint16_t face_num;            // Number of faces in the node
} node_t;

typedef struct
{
  int32_t planenum;             // The plane which splits the node

  int16_t children[2];

  //int16_t front;                 // If positive, id of Front child node
                               // If -2, the Front part is inside the model
                               // If -1, the Front part is outside the model
  //int16_t back;                  // If positive, id of Back child node
                               // If -2, the Back part is inside the model
                               // If -1, the Back part is outside the model
} clipnode_t;

typedef struct
{
  int32_t type;                   // Special type of leaf
  int32_t vislist;                // Beginning of visibility lists
                               //     must be -1 or in [0,numvislist[
  bboxshort_t bound;           // Bounding box of the leaf
  uint16_t lface_id;            // First item of the list of faces
                               //     must be in [0,numlfaces[
  uint16_t lface_num;           // Number of faces in the leaf  
  uint8_t sndwater;             // level of the four ambient sounds:
  uint8_t sndsky;               //   0    is no sound
  uint8_t sndslime;             //   0xFF is maximum volume
  uint8_t sndlava;              //
} dleaf_t;

typedef struct                 // Mip texture list header
{
  int32_t numtex;                 // Number of textures in Mip Texture list
  int32_t* offsets;         // Offset to each of the individual texture
} mipheader_t;

typedef struct                 // Mip Texture
{
  char   name[16];             // Name of the texture.
  uint32_t width;                // width of picture, must be a multiple of 8
  uint32_t height;               // height of picture, must be a multiple of 8
  uint32_t offset1;              // offset to u_char Pix[width   * height]
  uint32_t offset2;              // offset to u_char Pix[width/2 * height/2]
  uint32_t offset4;              // offset to u_char Pix[width/4 * height/4]
  uint32_t offset8;              // offset to u_char Pix[width/8 * height/8]
} miptex_t;

typedef struct
{
  int miptex_index;
  unsigned int tex_id;
}my_miptex;
  
typedef struct
{
  bsp_vec3   vectorS;            // S vector, horizontal in texture space)
  float distS;              // horizontal offset in texture space
  bsp_vec3   vectorT;            // T vector, vertical in texture space
  float distT;              // vertical offset in texture space
  uint32_t   texture_id;         // Index of Mip Texture
                               //           must be in [0,numtex[
  uint32_t   animated;           // 0 for ordinary textures, 1 for water 
} texinfo_t;

typedef struct{
  int leaf_index;
  int* face_indexes;
  int num_faces;
}leaf_render;


typedef struct{
  int face_id;
  float* vertices;//x,y,z,nx,ny,nz,u,v,b
  int num_vertices;
  int fpv;
  unsigned int VAO,VBO;
}MeshedFace;

typedef struct{
  int face_index;
  float brightness;
  float* vertices;
  int num_vertices;
  unsigned int VAO,VBO;
}face_render;

typedef struct{
  int face_index;
  float brightness;
}face_rt;

typedef struct{
  float x,y,z;
  float nx,ny,nz;
  float u,v;
}render_vertex ;

typedef struct {
  node_t* nodes;        int numNodes;
  clipnode_t* clipnodes; int numClipNodes;
  dleaf_t* leaves;      int numLeaves;
  plane_t* planes;      int numPlanes;
  vertex_t* vertices;   int numVertices;
  edge_t* edges;        int numEdges;
  face_t* faces;        int numFaces;
  MeshedFace* meshed_faces;
  texinfo_t* texinfo;   int numTexinfo;
  miptex_t* miptexes;   int numMipTexes;
  int32_t* surfedges;   int numSurfedges;    // Links faces to edges
  uint16_t* lsurfaces;  int numLsurfaces;  // ← ADD THIS!
  model_t* models;      int numModels;
  uint8_t** pixels;     int numPixels;
  uint8_t* visitlist;    int numVisitlist;
  my_miptex* my_texes;
  uint8_t palette[256][3];
} bsp_model_t;

typedef struct mnode_s
{
  // common with leaf
  int			contents;		// 0, to differentiate from leafs
  int			visframe;		// node needs to be traversed if current
	
  int16_t		minmaxs[6];		// for bounding box culling

  struct mnode_s	*parent;

  //node specific
  struct mnode_s	*children[2];	
  int plane_id;
  uint16_t		firstsurface;
  uint16_t		numsurfaces;
} mnode_t;

//TODO: we only need to know wich leaf to render right ? after that we can use the same approach as before for rendering!
typedef struct mleaf_s
{
  // common with node
  int			contents;		// wil be a negative contents number
  int			visframe;		// node needs to be traversed if current

  int16_t		minmaxs[6];		// for bounding box culling

  struct mnode_s	*parent;

  // leaf specific
  int index_to_leaf; // this is used to get the index to the dleaf_t that's used to render
} mleaf_t;

mleaf_t* bsp_traversal(bsp_model_t* tree, mnode_t* node, vec3 cam_pos);

#pragma pack(pop)  // Restore normal packing

#endif

