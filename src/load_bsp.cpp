#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_bsp.h"
#include "Game.h"
#include "endian.h"

#define AXIAL_EPS 0.999f

static void load_palette(char* address,bsp_model_t* tree){
  FILE* f = fopen(address,"rb");
  if (!f) { perror("fopen"); return ; }
  fread(tree->palette,1,768,f);
  fclose(f);
}

static int loadBsp(char* address,bsp_model_t** tree){
  *tree = (bsp_model_t*)malloc(sizeof(bsp_model_t));
  uint32_t f_offset = 0;
  FILE *f = fopen(address, "rb");
  if (!f) { perror("fopen"); return 0; }
  size_t read_size;
  int32_t magic,version;
  
  read_size = fread(&version,4,1,f);
  f_offset += 4;

 
  int num_header_lumps = 15;
  dheader_t *header = (dheader_t*)malloc(sizeof(dheader_t));
  //NOTE: All the offsets are counted from the start of the BSP files
  read_size = fread(&(header->entities),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->planes),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->miptex),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->vertices),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->visilist),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->nodes),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->texinfo),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->faces),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->lightmaps),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->clipnodes),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->leaves),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->lface),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->edges),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->ledges),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  read_size = fread(&(header->models),sizeof(dentry_t),1,f);
  f_offset += sizeof(dentry_t);
  //NOTE: get vertices
  FILE *f_vertices = fopen(address, "rb");
  if (!f_vertices) { perror("fopen"); return 0; }
  fseek(f_vertices,header->vertices.offset,SEEK_SET);
  (*tree)->vertices = (vertex_t*)malloc(header->vertices.size);
  (*tree)->numVertices = (header->vertices.size)/sizeof(vertex_t);// this is the number of floats
  //(*tree)->num_vertices/=3;// this is the number of points
  fread((*tree)->vertices,sizeof(vertex_t),header->vertices.size/sizeof(vertex_t),f_vertices);
  fclose(f_vertices);
  //NOTE: get planes
  FILE *f_planes = fopen(address, "rb");
  if (!f_planes) { perror("fopen"); return 0; }
  fseek(f_planes,header->planes.offset,SEEK_SET);
  (*tree)->planes = (plane_t*)malloc(header->planes.size);
  (*tree)->numPlanes = header->planes.size/sizeof(plane_t);
  fread((*tree)->planes,sizeof(plane_t),header->planes.size/sizeof(plane_t),f_planes);
  fclose(f_planes);

  // Then in your plane loading:
  for (int i = 0; i < (*tree)->numPlanes; i++) {
    (*tree)->planes[i].normal.x = LittleFloat((*tree)->planes[i].normal.x);
    (*tree)->planes[i].normal.y = LittleFloat((*tree)->planes[i].normal.y);
    (*tree)->planes[i].normal.z = LittleFloat((*tree)->planes[i].normal.z);
  
    (*tree)->planes[i].dist = LittleFloat((*tree)->planes[i].dist);  // ← THIS IS CRITICAL!
    plane_t *p = &(*tree)->planes[i];

    // recompute plane type 
    if (fabsf(p->normal.x) > AXIAL_EPS) {
        p->type = 0; // X
        p->normal.x = (p->normal.x > 0) ? 1.0f : -1.0f;
        p->normal.y = 0.0f;
        p->normal.z = 0.0f;
    }
    else if (fabsf(p->normal.y) > AXIAL_EPS) {
        p->type = 1; // Y
        p->normal.x = 0.0f;
        p->normal.y = (p->normal.y > 0) ? 1.0f : -1.0f;
        p->normal.z = 0.0f;
    }
    else if (fabsf(p->normal.z) > AXIAL_EPS) {
        p->type = 2; // Z
        p->normal.x = 0.0f;
        p->normal.y = 0.0f;
        p->normal.z = (p->normal.z > 0) ? 1.0f : -1.0f;
    }
    else {
        p->type = 3; // non-axial
    }
  }

  //NOTE: get faces
  FILE *f_faces = fopen(address, "rb");
  if (!f_faces) { perror("fopen"); return 0; }
  fseek(f_faces,header->faces.offset,SEEK_SET);
  (*tree)->faces = (face_t*)malloc(header->faces.size);
  (*tree)->numFaces = header->faces.size/sizeof(face_t);
  fread((*tree)->faces,sizeof(face_t),header->faces.size/sizeof(face_t),f_faces);
  fclose(f_faces);
  //NOTE: EDGES
  FILE *f_edges = fopen(address, "rb");
  if (!f_edges) { perror("fopen"); return 0; }
  fseek(f_edges,header->edges.offset,SEEK_SET);
  (*tree)->edges = (edge_t*)malloc(header->edges.size);
  (*tree)->numEdges = header->edges.size/sizeof(edge_t);
  fread((*tree)->edges,sizeof(edge_t),header->edges.size/sizeof(edge_t),f_edges);
  fclose(f_edges);
  //NOTE: NODES
  FILE *f_nodes = fopen(address, "rb");
  if (!f_nodes) { perror("fopen"); return 0; }
  fseek(f_nodes,header->nodes.offset,SEEK_SET);
  (*tree)->nodes = (node_t*)malloc(header->nodes.size);
  (*tree)->numNodes = header->nodes.size/sizeof(node_t);
  fread((*tree)->nodes,sizeof(node_t),header->nodes.size/sizeof(node_t),f_nodes);
  fclose(f_nodes);

  //NOTE: CLIPNODES
  FILE *f_clipnodes = fopen(address, "rb");
  if (!f_clipnodes) { perror("fopen"); return 0; }
  fseek(f_clipnodes,header->clipnodes.offset,SEEK_SET);
  (*tree)->clipnodes = (clipnode_t*)malloc(header->clipnodes.size);
  (*tree)->numClipNodes = header->clipnodes.size/sizeof(clipnode_t);
  fread((*tree)->clipnodes,sizeof(clipnode_t),header->clipnodes.size/sizeof(clipnode_t),f_clipnodes);
  fclose(f_clipnodes);
  
  //NOTE: Leafs
  FILE *f_leafs = fopen(address, "rb");
  if (!f_leafs) { perror("fopen"); return 0; }
  fseek(f_leafs,header->leaves.offset,SEEK_SET);
  (*tree)->leaves = (dleaf_t*)malloc(header->leaves.size);
  (*tree)->numLeaves = header->leaves.size/sizeof(dleaf_t);
  fread((*tree)->leaves,sizeof(dleaf_t),header->leaves.size/sizeof(dleaf_t),f_leafs);
  fclose(f_leafs);

  //NOTE: texinfo
  FILE *f_texinfo = fopen(address, "rb");
  if (!f_texinfo) { perror("fopen"); return 0; }
  fseek(f_texinfo,header->texinfo.offset,SEEK_SET);
  (*tree)->texinfo = (texinfo_t*)malloc(header->texinfo.size);
  (*tree)->numTexinfo = header->texinfo.size/sizeof(texinfo_t);
  fread((*tree)->texinfo,sizeof(texinfo_t),header->texinfo.size/sizeof(texinfo_t),f_texinfo);
  fclose(f_texinfo);
  
  //NOTE : MIP tex
  FILE *f_miptex = fopen(address, "rb");
  if (!f_miptex) { perror("fopen"); return 0; }
  fseek(f_miptex,header->miptex.offset,SEEK_SET);
  
  mipheader_t* mipheader = (mipheader_t*)malloc(sizeof(mipheader_t));
  fread(&(mipheader->numtex),sizeof(int32_t),1,f_miptex);
  mipheader->offsets = (int32_t*)malloc(sizeof(int32_t)*mipheader->numtex);
  fread(mipheader->offsets,sizeof(int32_t),mipheader->numtex,f_miptex);

  // Allocate array for all textures
  (*tree)->miptexes = (miptex_t*)malloc(sizeof(miptex_t) * mipheader->numtex);
  (*tree)->numMipTexes = mipheader->numtex;
  (*tree)->numPixels = (*tree)->numMipTexes;
  (*tree)->pixels = (uint8_t**)malloc(sizeof(uint8_t*)*(*tree)->numPixels);
  for (int i = 0; i < mipheader->numtex; i++) {
    // Seek to texture position (relative to miptex lump start)
    if (mipheader->offsets[i] == -1) {
      (*tree)->pixels[i] = NULL;
	memset(&(*tree)->miptexes[i], 0, sizeof(miptex_t));
      continue;
    }
    fseek(f_miptex, header->miptex.offset + mipheader->offsets[i], SEEK_SET);
   

    
    // Read the miptex header
    fread(&((*tree)->miptexes[i]), sizeof(miptex_t), 1, f_miptex);
    
    int w = (*tree)->miptexes[i].width;
    int h = (*tree)->miptexes[i].height;
    int dataSize = w * h; // mip level 0 only
    (*tree)->pixels[i] = (uint8_t*)malloc(dataSize);
    //(*tree)->pixels[i] = NULL;
    // If you want to read pixel data:
    if ((*tree)->miptexes[i].offset1 != 0) {
      // Seek relative to THIS texture's start
      fread((*tree)->pixels[i], 1, dataSize, f_miptex);
    }
  }
  //NOTE: SURFE-EDGES
  FILE* f_surfedge = fopen(address,"rb");
  if( !f_surfedge) {perror("fopen"); return 0;}
  fseek(f_surfedge,header->ledges.offset,SEEK_SET);
  (*tree)->surfedges = (int32_t*)malloc(header->ledges.size);
  (*tree)->numSurfedges = header->ledges.size / sizeof(int32_t);
  fread((*tree)->surfedges,sizeof(int32_t),(*tree)->numSurfedges,f_surfedge);
  fclose(f_surfedge);

  //NOTE: Lsurfaces
  FILE* f_lsurfaces = fopen(address,"rb");
  if( !f_lsurfaces) {perror("fopen"); return 0;}
  fseek(f_lsurfaces,header->lface.offset,SEEK_SET);
  (*tree)->lsurfaces = (uint16_t*)malloc(header->lface.size);
  (*tree)->numLsurfaces = header->lface.size / sizeof(uint16_t);
  fread((*tree)->lsurfaces,sizeof(uint16_t),(*tree)->numLsurfaces,f_lsurfaces);
  fclose(f_lsurfaces);

  //NOTE: models
  FILE* f_models = fopen(address,"rb");
  if( !f_models) {perror("fopen"); return 0;}
  fseek(f_models,header->models.offset,SEEK_SET);
  (*tree)->models = (model_t*)malloc(header->models.size);
  (*tree)->numModels = header->models.size / sizeof(model_t);
  fread((*tree)->models,sizeof(model_t),(*tree)->numModels,f_models);
  fclose(f_models);

  //NOTE: Lsurfaces
  FILE* f_visitlist = fopen(address,"rb");
  if( !f_visitlist) {perror("fopen"); return 0;}
  fseek(f_visitlist,header->visilist.offset,SEEK_SET);
  (*tree)->visitlist = (uint8_t*)malloc(header->visilist.size);
  (*tree)->numVisitlist = header->visilist.size / sizeof(uint8_t);
  fread((*tree)->visitlist,sizeof(uint8_t),(*tree)->numVisitlist,f_visitlist);
  fclose(f_visitlist);
  
  char* palette_address = "..\\..\\data\\palette.lmp";
  load_palette(palette_address,(*tree));
  printf("palette load: %d %d %d\n", (*tree)->palette[1][0], (*tree)->palette[1][1], (*tree)->palette[1][2]);
  //NOTE: just for checking,create the texture images here.
  // Debug-only: dumps decoded textures as .ppm files if a "debug_textures" folder exists
  // next to the executable. Purely optional - if the folder isn't there, this is skipped.
  for(int i = 0; i < mipheader->numtex; i++){
    char image_address[256];
    snprintf(image_address,sizeof(image_address),"debug_textures\\image__%d.ppm",i);
    if(!((*tree)->miptexes[i].offset1)){
      continue;
    }
    FILE* image_file = fopen(image_address,"wb");//TODO: open it as what ?
    if(!image_file){
      continue; // debug_textures folder doesn't exist - nothing to do, not an error
    }
    int width = (*tree)->miptexes[i].width;
    int height = (*tree)->miptexes[i].height;
    // ← ADD PPM HEADER HERE!
    fprintf(image_file, "P6\n%d %d\n255\n", width, height);
    int size = (*tree)->miptexes[i].width * (*tree)->miptexes[i].height;
    for(int k=0;k<size;k++){
      uint8_t index = (*tree)->pixels[i][k];
      fwrite((*tree)->palette[index],1,3,image_file);
    }
    fclose(image_file);
  }
  
  return 1;
}


void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
  node->parent = parent;
  if (node->contents < 0)
    return;
  Mod_SetParent (node->children[0], node);
  Mod_SetParent (node->children[1], node);
}

mnode_t* create_custom_nodes(node_t* in_nodes,mleaf_t* leafs,int num_in_nodes){
  mnode_t* out_nodes = (mnode_t*)malloc(sizeof(mnode_t)*num_in_nodes);
  memset(out_nodes,0,sizeof(mnode_t)*num_in_nodes);
  int count = num_in_nodes;
  node_t* in = in_nodes;
  mnode_t* out = out_nodes;
  int i, j, p;
  for ( i=0 ; i<count ; i++)
    {
      out[i].contents = 0;
      out[i].minmaxs[0] = in[i].box.minX;
      out[i].minmaxs[1] = in[i].box.minY;
      out[i].minmaxs[2] = in[i].box.minZ;
      out[i].minmaxs[3] = in[i].box.maxX;
      out[i].minmaxs[4] = in[i].box.maxY;
      out[i].minmaxs[5] = in[i].box.maxZ;
      
      p = in[i].plane_id;
      out[i].plane_id = p;

      out[i].firstsurface = in[i].face_id;
      out[i].numsurfaces =  in[i].face_num;
		
      
      p = LittleShort(in[i].front); 
      if(p >= 0){
	out[i].children[0] = out+p;
      }else{
	//out[i].children[0] = (mnode_t*)(leafs + (-1-p));
	out[i].children[0] = (mnode_t*)(leafs + (~p));
      }
      p = LittleShort(in[i].back);
      if(p >= 0){
	out[i].children[1] = out + p;
      }else{
	//out[i].children[1] = (mnode_t*)(leafs + (-1-p));
	out[i].children[1] = (mnode_t*)(leafs + (~p));
      }
      // Right after you create the children pointers for node 2:
      if (i == 2) {
    
	if (out_nodes[2].children[0]->contents < 0) {
	  mleaf_t* l = (mleaf_t*)out_nodes[2].children[0];
	}
	if (out_nodes[2].children[1]->contents < 0) {
	  mleaf_t* l = (mleaf_t*)out_nodes[2].children[1];
	}
      }
    }
	
  Mod_SetParent (out_nodes, NULL);	// sets nodes and leafs
  return out_nodes;
}

mleaf_t* create_custom_leafs(dleaf_t* in_leafs,int num_in_leafes){
  mleaf_t* out_leafs = (mleaf_t*)malloc(sizeof(mleaf_t)*num_in_leafes);
  memset(out_leafs,0,sizeof(mleaf_t)*num_in_leafes);
  int count = num_in_leafes;
  dleaf_t* in = in_leafs;
  mleaf_t* out = out_leafs;
  int i, p;

  for ( i=0 ; i<count ; i++)
    {
      out[i].minmaxs[0] = in[i].bound.minX;
      out[i].minmaxs[1] = in[i].bound.minY;
      out[i].minmaxs[2] = in[i].bound.minZ;
      out[i].minmaxs[3] = in[i].bound.maxX;
      out[i].minmaxs[4] = in[i].bound.maxY;
      out[i].minmaxs[5] = in[i].bound.maxZ;
      
      p = in[i].type;
      out[i].contents = p;
      //out[i].contents = -1;
      
      out[i].index_to_leaf = i;
    }
  return out_leafs;
}

float DotProduct(vec3 a,bsp_vec3 b){
  float d = (a[0] * b.x + 
	     a[1] * b.y + 
	     a[2] * b.z);
  return d;
}

void p3d(vec3 origin,vec3 u,vec3 v,float x,float y,vec3 result){
  vec3 xU,yV;
  glm_vec_scale(u,x,xU);
  glm_vec_scale(v,y,yV);
  glm_vec_copy(origin,result);
  glm_vec_add(result,xU,result);
  glm_vec_add(result,yV,result);
}

int pointCrossesEdge(float x,float y,vec3 origin,vec3 v0,vec3 v1,vec3 u,vec3 v){
  // we shoot a ray to the right
  // we assume v0.y > v1.y
  int result = 0;
  vec3 temp;
  /*if(v0[1] < v1[1]){
    glm_vec_copy(v0,temp);
    glm_vec_copy(v1,v0);
    glm_vec_copy(temp,v1);
    }*/
  vec3 v02d,v12d;
  float x0,y0,x1,y1;
  glm_vec_sub(v0,origin,v02d);
  x0 = glm_vec_dot(v02d,u);
  y0 = glm_vec_dot(v02d,v);
  glm_vec_sub(v1,origin,v12d);
  x1 = glm_vec_dot(v12d,u);
  y1 = glm_vec_dot(v12d,v);
   float x_temp,y_temp;
  if(y0 < y1){
    x_temp = x0;y_temp = y0;
    x0 = x1;y0 = y1;
    x1 = x_temp;y1 = y_temp;
  }
  if((y1 > y) != (y0 > y)){
    float x_intersect = x0+(y-y0)*(x1-x0)/(y1-y0);//x0+(py−y0)∗(x1−x0)/(y1−y0)
    if(x<x_intersect){
      result = 1;
    }
  }
  return result;
}

int pointInPolygone(Engine* engine,int* indices,int num_indices,float x,float y,vec3 origin,vec3 u,vec3 v){
  int intersections = 0;
  for(int i=0;i<num_indices;i++){
    vec3 v0,v1;
    v0[0] = engine->world.tree->vertices[indices[i]].X;
    v0[1] = engine->world.tree->vertices[indices[i]].Y;
    v0[2] = engine->world.tree->vertices[indices[i]].Z;

    if(i==num_indices-1){
      v1[0] = engine->world.tree->vertices[indices[0]].X;
      v1[1] = engine->world.tree->vertices[indices[0]].Y;
      v1[2] = engine->world.tree->vertices[indices[0]].Z;
    }else{
      v1[0] = engine->world.tree->vertices[indices[i+1]].X;
      v1[1] = engine->world.tree->vertices[indices[i+1]].Y;
      v1[2] = engine->world.tree->vertices[indices[i+1]].Z;
    }
    //TODO: v0 is origin????
    if(pointCrossesEdge(x,y,origin,v0,v1,u,v)){
      intersections += 1;
    }
  }
  if(intersections==0){
    return 0;
  }
  if(intersections%2 == 0){
    return 0;
  }
  return 1;
}

typedef struct{
  float x1,x2,x3;
  float y1,y2,y3;
  float x_center,y_center;
  float raduis;
}triangle2D;

typedef struct{
  float maxX,maxY;
  float minX,minY;
}Bounds;

typedef struct{
  float x1,y1;
  float x2,y2;
}Edge;

void triangulate(MeshedFace* meshed_face,vec3 origin,float origin_u,float origin_v,vec3 u,vec3 v,float* xs,float* ys,Bounds* bound,int width,int height);


void calculate_uv2(bsp_model_t* tree,vec3 pos,texinfo_t* texinfo,float* u,float* v){
  int miptex_id = texinfo->texture_id;
  miptex_t* miptex = &tree->miptexes[miptex_id];
  vec3 mvectorS;
  mvectorS[0] = (texinfo->vectorS).x;mvectorS[1] = texinfo->vectorS.y;mvectorS[2] = texinfo->vectorS.z;
  vec3 mvectorT;
  mvectorT[0] = (texinfo->vectorT).x;mvectorT[1] = texinfo->vectorT.y;mvectorT[2] = texinfo->vectorT.z;
  float u_world = (glm_vec_dot(pos,mvectorS)) + texinfo->distS;
  float v_world = (glm_vec_dot(pos,mvectorT)) + texinfo->distT;

  *u = u_world/miptex->width;
  *v = v_world/miptex->height;
}


void mesh_face(Engine* engine,face_t* face,MeshedFace* meshed_face,float resolution){
  // travel between 2 vertices,and check their middle point,if all of them is hit by light : move,other wise keep creating middle points between lit and non lit vertices
  // i should end up with a more detailed face
  int ledge_id = face->ledge_id;
  int num_ledges = face->ledge_num;
  int plane_index = face->plane_id;
  plane_t* plane = &(engine->world.tree->planes[plane_index]);
  vec3 normal ;
  normal[0] = plane->normal.x;
  normal[1] = plane->normal.y;
  normal[2] = plane->normal.z;
  int texinfo_index = face->texinfo_id;
  texinfo_t* texinfo = &(engine->world.tree->texinfo[texinfo_index]);

  int num_vertex_indices = num_ledges;
  int* vertex_indices = (int*)malloc(sizeof(int)*num_vertex_indices);
  bsp_model_t* tree = (engine->world.tree);
  for(int i=0;i<num_ledges;i++){
    int32_t edge_index = tree->surfedges[ledge_id+i];
    edge_t* edge;
    int inversed = 0;
    if (edge_index >= 0) {
      edge = &tree->edges[edge_index];
      vertex_indices[i] = edge->vertex0;
    } else {
      edge = &tree->edges[-edge_index];
      vertex_indices[i] = edge->vertex1;
    }
  }
  int reverse = face->side;
  if(reverse){
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }

  //NOTE: now we have the list of indices for vertices
  //NOTE: everything is in quake space
  //vec3 normal;
  normal[0] = engine->world.tree->planes[face->plane_id].normal.x;
  normal[1] = engine->world.tree->planes[face->plane_id].normal.y;
  normal[2] = engine->world.tree->planes[face->plane_id].normal.z;
  vec3 U,V;
  vec3 v0,v1;
  v0[0] = engine->world.tree->vertices[vertex_indices[0]].X;
  v0[1] = engine->world.tree->vertices[vertex_indices[0]].Y;
  v0[2] = engine->world.tree->vertices[vertex_indices[0]].Z;

  v1[0] = engine->world.tree->vertices[vertex_indices[1]].X;
  v1[1] = engine->world.tree->vertices[vertex_indices[1]].Y;
  v1[2] = engine->world.tree->vertices[vertex_indices[1]].Z;
  glm_vec_sub(v1,v0,U);
  glm_vec_normalize(U);
  glm_vec_cross(normal,U,V);
  glm_vec_normalize(V);
  //NOTE: get the bounding box
  float minX = 0,minY = 0,maxX = 0,maxY = 0;
  vec3 p_rel1;
  glm_vec_sub(v1,v0,p_rel1);
  minX = glm_vec_dot(p_rel1,U);
  minY = glm_vec_dot(p_rel1,V);
  maxX = glm_vec_dot(p_rel1,U);
  maxY = glm_vec_dot(p_rel1,V);
  for(int i=0;i<num_vertex_indices;i++){
    vec3 p;
    p[0] = engine->world.tree->vertices[vertex_indices[i]].X;
    p[1] = engine->world.tree->vertices[vertex_indices[i]].Y;
    p[2] = engine->world.tree->vertices[vertex_indices[i]].Z;
    vec3 p_rel;
    glm_vec_sub(p,v0,p_rel);
    float x = glm_vec_dot(p_rel,U);
    float y = glm_vec_dot(p_rel,V);
    if(x>maxX){
      maxX = x;
    }
    if(x<minX){
      minX = x;
    }
    if(y>maxY){
      maxY = y;
    }
    if(y<minY){
      minY = y;
    }
  }

  //NOTE: create the points grid
  int grid_width = (maxX-minX)/resolution +1;
  int grid_height = (maxY-minY)/resolution +1;
  float* Xs = (float*)malloc(sizeof(float)*grid_width*grid_height);
  float* Ys = (float*)malloc(sizeof(float)*grid_width*grid_height);
  int xi = 0,yi = 0;
  for(float x = minX; x < maxX+1.0; x += resolution){
    yi = 0;  // reset yi for each new column of x
    for(float y = minY; y < maxY+1.0; y += resolution){
      if(xi < grid_width && yi < grid_height){
	if(pointInPolygone(engine, vertex_indices, num_vertex_indices, x, y, v0, U, V)){
	  Xs[yi * grid_width + xi] = x;
	  Ys[yi * grid_width + xi] = y;
	} else {
	  Xs[yi * grid_width + xi] = -999.0;
	  Ys[yi * grid_width + xi] = -999.0;
	}
      }
      yi++;  // inner loop increments yi (y-axis)
    }
    xi++;  // outer loop increments xi (x-axis)
  }
 
  
  //NOTE: create the triangle mesh out of the points grid
  Bounds bound = Bounds{maxX,maxY,minX,minY};
  float origin_u,origin_v;
  int texinfo_id = face->texinfo_id;
  texinfo = &(engine->world.tree->texinfo[texinfo_id]);
  calculate_uv2(engine->world.tree,v0,texinfo,&origin_u,&origin_v);
  triangulate(meshed_face,v0,origin_u,origin_v,U,V,Xs,Ys,&bound,grid_width,grid_height);

  free(Xs);
  free(Ys);
  free(vertex_indices);
}

void addTriangle(triangle2D** plane_triangles,int* num_triangles,int* array_size,triangle2D tri){
  if(*num_triangles >= *array_size){
    *array_size = (*array_size)*2;
    triangle2D* new_ptr = (triangle2D*)realloc(*plane_triangles, sizeof(triangle2D)*(*array_size));
    if(!new_ptr){
      printf("failed to realloc\n");
      return;
    }
    *plane_triangles = new_ptr;
  }
  (*plane_triangles)[(*num_triangles)] = tri;
  *num_triangles = (*num_triangles)+1;
}

float calc_distance(float x1,float y1,float x2,float y2){
  float dx = x2-x1;
  float dy = y2-y1;
  return sqrtf(dx*dx + dy*dy);
}

void triangleCircumCircle(triangle2D* t){
  float ax = t->x2 - t->x1;
  float ay = t->y2 - t->y1;
  float bx = t->x3 - t->x1;
  float by = t->y3 - t->y1;

  float D = 2 * (ax * by - ay * bx);
  if(fabs(D) < 0.00001){
    return;
  }
  float ux = (by * (ax*ax + ay*ay) - ay * (bx*bx + by*by)) / D;
  float uy = (ax * (bx*bx + by*by) - bx * (ax*ax + ay*ay)) / D;

  t->x_center = t->x1 + ux;
  t->y_center = t->y1 + uy;
  t->raduis = calc_distance(t->x1,t->y1,t->x_center,t->y_center);
}

int pointInTriangle(triangle2D tri,float x,float y){
  float distance = calc_distance(x,y,tri.x_center,tri.y_center);
  if(distance <= tri.raduis){
    return 1;
  }
  return 0;
}

void getTexCoords(float origin_u,float origin_v,float origin_x,float origin_y,int width,int height,float x,float y,float*u,float*v );

void triangulate(MeshedFace* meshed_face,vec3 origin,float origin_u,float origin_v,vec3 u,vec3 v,float* xs,float* ys,Bounds* bound,int width,int height){
  int num_triangles = (width ) * (height) * 2*10;
  int triangle_index = 0;
  
  triangle2D* plane_triangles = (triangle2D*)malloc(sizeof(triangle2D)*num_triangles);
  //NOTE: Create super triangle
  float dx = bound->maxX - bound->minX;
  float dy = bound->maxY - bound->minY;
  float delta = (dx > dy ? dx : dy) * 10.0f;  // large enough margin

  float cx = (bound->minX + bound->maxX) / 2.0f;
  float cy = (bound->minY + bound->maxY) / 2.0f;

  float super_x[3] = { cx,           cx - 2.0f*delta, cx + 2.0f*delta };
  float super_y[3] = { cy + 2.0f*delta, cy - delta,      cy - delta      };
  //float super_x[3] = {(float)(bound->minX-1000.0),(float)(bound->maxX+1000.0),(float)(bound->maxX+1000.0)};
  //float super_y[3] = {(float)(bound->minY-1000.0),(float)(bound->minY-1000.0),(float)(bound->maxY+1000.0)};
  triangle2D super_triangle = triangle2D{super_x[0],super_x[1],super_x[2],super_y[0],super_y[1],super_y[2],0.0,0.0,0.0};
  plane_triangles[0] = super_triangle;
  triangle_index++;
  int num_points = width*height;
  for(int i=0;i<num_points+2;i++){
    float x,y;
    if(i>=num_points){
      int index = i-num_points;
      x = super_x[index];
      y = super_y[index];
    }else{
      x = xs[i];
      y = ys[i];
    }
    if(x==-999.0 && y==-999.0){
      continue;
    }
    int num_edges = 50;
    Edge* edge_buffer = (Edge*)malloc(sizeof(Edge)*num_edges);
    int edge_index = 0;
    int triangle_write = 0;
    for(int j=0;j<triangle_index;j++){
      triangleCircumCircle(&plane_triangles[j]);
      triangle2D tri = plane_triangles[j];
      int delete_triangle = 0;
      if(pointInTriangle(tri,x,y)){
	//TODO: delete triangle from triangles buffer
	delete_triangle = 1;
	//TODO: mind the edge_buffer size
	if(edge_index +3 >= num_edges){
	  num_edges+= 50;
	  Edge* new_ptr = (Edge*)realloc(edge_buffer, sizeof(Edge)*num_edges);
	  if(!new_ptr){
	    printf("couldn't realloc edge buffer\n");
	    free(edge_buffer); // free original
	    return;
	  }
	  edge_buffer = new_ptr;
	}
	  
	edge_buffer[edge_index++] = Edge{tri.x1,tri.y1,tri.x2,tri.y2};
	edge_buffer[edge_index++] = Edge{tri.x2,tri.y2,tri.x3,tri.y3};
	edge_buffer[edge_index++] = Edge{tri.x3,tri.y3,tri.x1,tri.y1};
      }
      if(!delete_triangle){
	plane_triangles[triangle_write++] = tri;
      }
    }
    triangle_index = triangle_write;
    //TODO: delete all doubly specified edges from the edge buffer
    int edge_write = 0;
    for(int k=0;k<edge_index;k++){
      Edge edge1 = edge_buffer[k];
      int duplicate = 0;
      for(int l=0;l<edge_index;l++){	
	if(k!=l){
	  Edge edge2 = edge_buffer[l];
	  if(((edge1.x1==edge2.x1 && edge1.y1==edge2.y1) && (edge1.x2==edge2.x2 && edge1.y2==edge2.y2)) || ((edge1.x1==edge2.x2 && edge1.y1==edge2.y2) && (edge1.x2==edge2.x1 && edge1.y2==edge2.y1))){
	    duplicate = 1;
	    break;
	  }
	}
      }
      if(!duplicate){
	edge_buffer[edge_write++] = edge1;
      }
    }
    edge_index = edge_write;

    //TODO: add all the triangles between our point and edges to the triangle list
    for(int j=0;j<edge_index;j++){
      Edge e = edge_buffer[j];
      triangle2D triangle = triangle2D{e.x1,e.x2,x,e.y1,e.y2,y,0.0,0.0,0.0};
      addTriangle(&plane_triangles,&triangle_index,&num_triangles,triangle);
    }
    free(edge_buffer);
  }
  //TODO: remove any triangles from the triangle list that use the supertriangle vertices
  int write = 0;
  for(int i=0;i<triangle_index;i++){
    int delete_triangle = 0;
    triangle2D tri = plane_triangles[i];
    for(int j=0;j<3;j++){
      if(tri.x1== super_x[j] && tri.y1==super_y[j] ){
	delete_triangle = 1;
	break;
      }
      if(tri.x2== super_x[j] && tri.y2==super_y[j] ){
	delete_triangle = 1;
	break;
      }
      if(tri.x3== super_x[j] && tri.y3==super_y[j] ){
	delete_triangle = 1;
	break;
      }
    }
    if(!delete_triangle){
      plane_triangles[write++] = tri;
    }
  }
  triangle_index = write;
  //TODO: now we have the list of triangles in the plane,convert them to 3D and calculate texture coordinates for each of thei vertices
  
  int fpv = 3+2+1;
  meshed_face->vertices = (float*)malloc(sizeof(float)*triangle_index*3*fpv);
  int vertices_index = 0;
  vec3 pt1,pt2,pt3;
  float u1,v1,u2,v2,u3,v3;
  triangle2D tri;
  int num_vertices = 0;
  for(int i=0;i<triangle_index;i++){
    tri = plane_triangles[i];
    
    p3d(origin,u,v,tri.x1,tri.y1,pt1);
    p3d(origin,u,v,tri.x2,tri.y2,pt2);
    p3d(origin,u,v,tri.x3,tri.y3,pt3);
    getTexCoords(origin_u,origin_v,origin[0],origin[1],width,height,tri.x1,tri.y1,&u1,&v1);
    getTexCoords(origin_u,origin_v,origin[0],origin[1],width,height,tri.x2,tri.y2,&u2,&v2);
    getTexCoords(origin_u,origin_v,origin[0],origin[1],width,height,tri.x3,tri.y3,&u3,&v3);
    
    meshed_face->vertices[vertices_index++] = pt1[0];meshed_face->vertices[vertices_index++] = pt1[2];meshed_face->vertices[vertices_index++] = -pt1[1];meshed_face->vertices[vertices_index++] = u1;meshed_face->vertices[vertices_index++] = v1;meshed_face->vertices[vertices_index++] = 0.0;
    num_vertices++;

    meshed_face->vertices[vertices_index++] = pt2[0];meshed_face->vertices[vertices_index++] = pt2[2];meshed_face->vertices[vertices_index++] = -pt2[1];meshed_face->vertices[vertices_index++] = u2;meshed_face->vertices[vertices_index++] = v2;meshed_face->vertices[vertices_index++] = 0.0;
    num_vertices++;


    meshed_face->vertices[vertices_index++] = pt3[0];meshed_face->vertices[vertices_index++] = pt3[2];meshed_face->vertices[vertices_index++] = -pt3[1];meshed_face->vertices[vertices_index++] = u3;meshed_face->vertices[vertices_index++] = v3;meshed_face->vertices[vertices_index++] = 0.0;
    num_vertices++;


  }
  meshed_face->num_vertices = num_vertices;
  meshed_face->fpv = fpv;
  free(plane_triangles);
}

void getTexCoords(float origin_u,float origin_v,float origin_x,float origin_y,int width,int height,float x,float y,float*u,float*v ){
  *u = origin_u + (x - origin_x) / width;
  *v = origin_v + (y - origin_y) / height;
}

/*
typedef struct{
  float x1,x2,x3;
  float y1,y2,y3;
  float x_center,y_center;
  float raduis;
}triangle2D;
 */
