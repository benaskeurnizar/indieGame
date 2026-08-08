#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_bsp.h"
#include "Game.h"
#include "endian.h"


typedef struct{
  vec3 pt1;float u1,v1,i1;
  vec3 pt2;float u2,v2,i2;
  vec3 pt3;float u3,v3,i3;
}Tri2;

void calculate_uv3(bsp_model_t* tree,vec3 pos,texinfo_t* texinfo,float* u,float* v){
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

void addTrianle(Tri2** triangles,int* index,int* array_size,Tri2 tri){
  if(*index >= *array_size){
    *array_size = (*array_size)+1000;
    Tri2* temp_array = (Tri2*)realloc(*triangles,sizeof(Tri2)*(*array_size));
    if(!temp_array){
      free(*triangles);
      return;
    }
    *triangles = temp_array;
  }
  (*triangles)[*index] = tri;
  (*index) = (*index)+1;
}

float THRESHOLD = 0.0001 ;  // smaller = more detail
int MAX_DEPTH = 12;  // safety limit


float getPointLight(Engine* engine,tnode_t* tnodes,vec3 pos,vec3 normal,hull_t* hull){
  float intensity =3000.0;
 
  float k = 0.1;
  
  vec3 pos_bsp;
   
  //TODO: is it in engine or quake space?
  glm_vec_copy(pos,pos_bsp);
  //engine_pos_to_quake(pos,pos_bsp);
    
  /*for(int h=0;h<3;h++){
    pos_bsp[h] += face_normal_bsp[h]*;
    }*/

  float total_light = 0.0f;
  for(int j=0;j<(engine)->world.num_lights;j++){
    if(TestLine ((engine)->world.lights[j].pos,pos_bsp, tnodes)){
	
      //TODO: set the brightness based on the distance and angle
      vec3 L;
      glm_vec_sub((engine)->world.lights[j].pos,pos_bsp,L);
      float d = sqrtf(L[0]*L[0] + L[1]*L[1] + L[2]*L[2]);
      glm_vec_normalize(L);
      float ndotl = glm_vec_dot(normal, L);
      if (ndotl < 0) ndotl = 0;

      float attenuation = 1.0f / (1.0f + k * d *d);

      float light_contrib = ndotl * attenuation;
      total_light += intensity *light_contrib;
    }
  }
  if(total_light < 0.05){
    total_light = 0.05;
  }else if(total_light > 1.0){
    total_light = 1.0;
  }
  
  return total_light;
}

void processTriangle(Engine* engine,Tri2** triangles,texinfo_t* texinfo,tnode_t* tnodes,Tri2 t,vec3 normal,int* index,int* array_size,int depth,hull_t* hull){
  float L0 = getPointLight(engine,tnodes,t.pt1,normal,hull);
  float L1 = getPointLight(engine,tnodes,t.pt2,normal,hull);
  float L2 = getPointLight(engine,tnodes,t.pt3,normal,hull);

  vec3 center;
  glm_vec_zero(center);
  center[0] += t.pt1[0]+t.pt2[0]+t.pt3[0];
  center[1] += t.pt1[1]+t.pt2[1]+t.pt3[1];
  center[2] += t.pt1[2]+t.pt2[2]+t.pt3[2];
  center[0]/=3;
  center[1]/=3;
  center[2]/=3;

  float Lc = getPointLight(engine,tnodes,center,normal,hull);

  float avg = (L0 + L1 + L2) / 3;

  float error = fabs(Lc - avg);
  if(( error < THRESHOLD) || (depth >= MAX_DEPTH)){
    float u1,v1,u2,v2,u3,v3;
    calculate_uv3(engine->world.tree,t.pt1,texinfo,&u1,&v1);
    calculate_uv3(engine->world.tree,t.pt2,texinfo,&u2,&v2);
    calculate_uv3(engine->world.tree,t.pt3,texinfo,&u3,&v3);
    t.u1 = u1;t.v1 = v1;
    t.u2 = u2;t.v2 = v2;
    t.u3 = u3;t.v3 = v3;
    t.i1 = L0;t.i2 = L1;
    t.i3 = L2;
    addTrianle(triangles,index,array_size,t);
    return;
  }
  //subdivide
  vec3 m01,m12,m20;
  glm_vec_add(t.pt1,t.pt2,m01);
  glm_vec_scale(m01,0.5,m01);
  glm_vec_add(t.pt2,t.pt3,m12);
  glm_vec_scale(m12,0.5,m12);
  glm_vec_add(t.pt3,t.pt1,m20);
  glm_vec_scale(m20,0.5,m20);

  Tri2 t1,t2,t3,t4;
  glm_vec_copy(t.pt1, t1.pt1); t1.u1 = 0.0; t1.v1 = 0.0; t1.i1 = 0.0;
  glm_vec_copy(m01,   t1.pt2); t1.u2 = 0.0; t1.v2 = 0.0; t1.i2 = 0.0;
  glm_vec_copy(m20,   t1.pt3); t1.u3 = 0.0; t1.v3 = 0.0; t1.i3 = 0.0;

  glm_vec_copy(m01,   t2.pt1); t2.u1 = 0.0; t2.v1 = 0.0; t2.i1 = 0.0;
  glm_vec_copy(t.pt2, t2.pt2); t2.u2 = 0.0; t2.v2 = 0.0; t2.i2 = 0.0;
  glm_vec_copy(m12,   t2.pt3); t2.u3 = 0.0; t2.v3 = 0.0; t2.i3 = 0.0;

  glm_vec_copy(m20,   t3.pt1); t3.u1 = 0.0; t3.v1 = 0.0; t3.i1 = 0.0;
  glm_vec_copy(m12,   t3.pt2); t3.u2 = 0.0; t3.v2 = 0.0; t3.i2 = 0.0;
  glm_vec_copy(t.pt3, t3.pt3); t3.u3 = 0.0; t3.v3 = 0.0; t3.i3 = 0.0;

  glm_vec_copy(m01,   t4.pt1); t4.u1 = 0.0; t4.v1 = 0.0; t4.i1 = 0.0;
  glm_vec_copy(m12,   t4.pt2); t4.u2 = 0.0; t4.v2 = 0.0; t4.i2 = 0.0;
  glm_vec_copy(m20,   t4.pt3); t4.u3 = 0.0; t4.v3 = 0.0; t4.i3 = 0.0;
 

  processTriangle(engine,triangles,texinfo,tnodes,t1,normal,index,array_size,depth+1,hull);
  processTriangle(engine,triangles,texinfo,tnodes,t2,normal,index,array_size,depth+1,hull);
  processTriangle(engine,triangles,texinfo,tnodes,t3,normal,index,array_size,depth+1,hull);
  processTriangle(engine,triangles,texinfo,tnodes,t4,normal,index,array_size,depth+1,hull);
}


void processFace(Engine* engine,MeshedFace* faceM,face_t* face,tnode_t* tnodes){
  // start with initial triangles
  int ledge_id = face->ledge_id;
  int num_ledges = face->ledge_num;
  int plane_index = face->plane_id;
  plane_t* plane = &(engine->world.tree->planes[plane_index]);
  vec3 normal ;
  normal[0] = plane->normal.x;
  normal[1] = plane->normal.y;
  normal[2] = plane->normal.z;

  if(face->side){
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }

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
    //normal[0] = -normal[0];
    //normal[1] = -normal[1];
    //normal[2] = -normal[2];
  }

  // calculate center point
  vec3 center;
  glm_vec_zero(center);
  for(int i=0;i<num_vertex_indices;i++){
    center[0] += engine->world.tree->vertices[vertex_indices[i]].X;
    center[1] += engine->world.tree->vertices[vertex_indices[i]].Y;
    center[2] += engine->world.tree->vertices[vertex_indices[i]].Z;
  }
  center[0]/=num_vertex_indices;
  center[1]/=num_vertex_indices;
  center[2]/=num_vertex_indices;

  // now we create the outline triangle
  int initial_num_trianglesO = 15;
  Tri2* trianglesO = (Tri2*)malloc(sizeof(Tri2)*initial_num_trianglesO);
  // now we create the initial triangles
  int initial_num_triangles = 1000;
  Tri2* triangles = (Tri2*)malloc(sizeof(Tri2)*initial_num_triangles);
  int triangles_indexO = 0;
  for(int i=0;i< num_vertex_indices;i++){
    vec3 pt1,pt2;
    pt1[0] = engine->world.tree->vertices[vertex_indices[i]].X;
    pt1[1] = engine->world.tree->vertices[vertex_indices[i]].Y;
    pt1[2] = engine->world.tree->vertices[vertex_indices[i]].Z;
    if(i==num_vertex_indices-1){
      pt2[0] = engine->world.tree->vertices[vertex_indices[0]].X;
      pt2[1] = engine->world.tree->vertices[vertex_indices[0]].Y;
      pt2[2] = engine->world.tree->vertices[vertex_indices[0]].Z;
    }else{
      pt2[0] = engine->world.tree->vertices[vertex_indices[i+1]].X;
      pt2[1] = engine->world.tree->vertices[vertex_indices[i+1]].Y;
      pt2[2] = engine->world.tree->vertices[vertex_indices[i+1]].Z;
    }
    Tri2 t;
    glm_vec_copy(pt1,   t.pt1); t.u1 = 0.0; t.v1 = 0.0; t.i1 = 0.0;
    glm_vec_copy(pt2,   t.pt2); t.u2 = 0.0; t.v2 = 0.0; t.i2 = 0.0;
    glm_vec_copy(center,t.pt3); t.u3 = 0.0; t.v3 = 0.0; t.i3 = 0.0;
    addTrianle(&trianglesO,&triangles_indexO,&initial_num_trianglesO,t);
  }

  hull_t* hull = (hull_t*)malloc(sizeof(hull_t));
  hull->clipnodes = (engine)->world.tree->clipnodes;
  hull->planes = (engine)->world.tree->planes;
  hull->firstclipnode = (engine)->world.tree->models[0].node_id0;
  hull->lastclipnode = (engine)->world.tree->numClipNodes - 1;
  glm_vec_zero(hull->clip_mins);
  glm_vec_zero(hull->clip_maxs);
  int triangles_index = 0;
  for(int i=0;i<triangles_indexO;i++){
    processTriangle(engine,&triangles,texinfo,tnodes,trianglesO[i],normal,&triangles_index,&initial_num_triangles,0,hull);
  }
  //for each base triangle of face:
  // processTriangle(v0, v1, v2, 0)
  faceM->fpv = 6;
  faceM->num_vertices = 3*triangles_index;
  faceM->vertices = (float*)malloc(sizeof(float)*3 * triangles_index * faceM->fpv);
  int vertices_index = 0;
  for(int i=0;i<triangles_index;i++){
    Tri2 tri = triangles[i];
    faceM->vertices[vertices_index++] = tri.pt1[0];faceM->vertices[vertices_index++] = tri.pt1[2];faceM->vertices[vertices_index++] = -tri.pt1[1];faceM->vertices[vertices_index++] = tri.u1;faceM->vertices[vertices_index++] = tri.v1;faceM->vertices[vertices_index++] = tri.i1;

    faceM->vertices[vertices_index++] = tri.pt2[0];faceM->vertices[vertices_index++] = tri.pt2[2];faceM->vertices[vertices_index++] = -tri.pt2[1];faceM->vertices[vertices_index++] = tri.u2;faceM->vertices[vertices_index++] = tri.v2;faceM->vertices[vertices_index++] = tri.i2;


    faceM->vertices[vertices_index++] = tri.pt3[0];faceM->vertices[vertices_index++] = tri.pt3[2];faceM->vertices[vertices_index++] = -tri.pt3[1];faceM->vertices[vertices_index++] = tri.u3;faceM->vertices[vertices_index++] = tri.v3;faceM->vertices[vertices_index++] = tri.i3;
  }
  free(hull);
  free(triangles);
  free(vertex_indices);
  free(trianglesO);
}
