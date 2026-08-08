#include "my_bsp.h"
#include "Game.h"
#include <stdint.h>
#include "cglm.h"
#include "mat4.h"
#include "vec4.h"
#include "vec3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void engine_pos_to_quake(vec3 p,vec3 result){
  result[0] = p[0];
  result[1] = -p[2];
  result[2] = p[1];
}
void quake_pos_to_engine(vec3 q, vec3 result){
    result[0] = q[0];  // X remains X
    result[1] = q[2];  // Engine Y was Quake Z
    result[2] = -q[1]; // Engine Z was -Quake Y
}

mleaf_t* bsp_traversal(bsp_model_t* tree, mnode_t* node, vec3 cam_pos) {
    if (!node) return NULL;
    
    int depth = 0;
    
    while (1) {
        if (node->contents < 0) {
            mleaf_t* leaf = (mleaf_t*)node;
            return leaf;
        }
        
        plane_t* plane = &tree->planes[node->plane_id];
        float d = DotProduct(cam_pos, plane->normal) - plane->dist;
        
        node = (d >= 0) ? node->children[0] : node->children[1];
        
        if (++depth > 100) {
            printf("ERROR: Infinite loop!\n");
            return NULL;
        }
    }
}

int* getFrameVisileaves(Engine* engine,int* num_visible,int* updated){
  bsp_model_t* tree = engine->world.tree;
  vec3 bsp_pos;
  engine_pos_to_quake(engine->world.camera.pos,bsp_pos);
  mleaf_t* my_leaf = bsp_traversal(tree,engine->my_nodes,bsp_pos);
  //mleaf_t* my_leaf = bsp_traversal_debug(tree,engine->my_nodes,engine->world.camera.pos);
  if(!my_leaf){
    *num_visible = 0;
    return NULL;
  }
  //int leaf_index = find_camera_leaf(&(engine->world.camera),tree);
  int leaf_index = my_leaf->index_to_leaf;
  if(leaf_index == engine->current_leaf){
    // TODO: Return the old ones!
    return NULL;
  }
  *updated = 1;
  engine->current_leaf = leaf_index;
  dleaf_t* current_leaf = &(tree->leaves[leaf_index]);
  // Check if leaf has visibility data
  if (current_leaf->vislist == -1) {
    // No PVS data - assume all leaves visible (shouldn't happen in good maps)
    *num_visible = tree->numLeaves;
    int* all_leaves = (int*)malloc(sizeof(int) * tree->numLeaves);
    for (int i = 0; i < tree->numLeaves; i++) {
      all_leaves[i] = i;
    }
    return all_leaves;
  }
  uint8_t* leafVis = tree->visitlist + current_leaf->vislist;
  int bitIndex = 1;
  int initialVS_size = tree->numLeaves;
  int *visible_leafs = (int*)malloc(sizeof(int)*initialVS_size);
  int vs_index = 0;
  int num_visible_leafs = 0;
  while(bitIndex < tree->numLeaves){
    uint8_t byte = *leafVis++;
    if(byte){
      uint8_t b = byte;
      for(int i=0;i<8 && bitIndex < tree->numLeaves;i++){
	if(b & (1 << i)){
	  visible_leafs[vs_index++] = bitIndex;
	}
	bitIndex++;
      }
    }else{
      uint8_t skip = *leafVis++;
      bitIndex += skip* 8;
    }
  }
  num_visible_leafs = vs_index;
  visible_leafs = (int*)realloc(visible_leafs,sizeof(int)*vs_index);
  *num_visible = num_visible_leafs;
  return visible_leafs;
}

//TODO : assume the above is correct and recheck the one down!
leaf_render* getFrameFaces(Engine* engine,int* visileafs,int num_visileafs){
  leaf_render* leafs = (leaf_render*)malloc(sizeof(leaf_render)*num_visileafs);
  for(int i=0;i<num_visileafs;i++){
    dleaf_t* leaf = &(engine->world.tree->leaves[visileafs[i]]);
    int start = leaf->lface_id;
    int count = leaf->lface_num;
    leafs[i].leaf_index = visileafs[i];
    leafs[i].face_indexes = (int*)malloc(sizeof(int)*count);
    leafs[i].num_faces = count;
    for(int j = 0;j<count;j++){
      leafs[i].face_indexes[j] = engine->world.tree->lsurfaces[start+j];
    }
  }
  return leafs;
}


void calculate_uv(bsp_model_t* tree,vec3 pos,texinfo_t* texinfo,float* u,float* v){
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

void addTriangle(bsp_model_t* tree,face_render* face,vec3 normal,int vertices_index,int index){
  int fpv = 8;
  vertex_t* vertex = &tree->vertices[index];
  vec3 pos ;
  pos[0] = vertex->X;pos[1] = vertex->Y;pos[2] = vertex->Z;
  float u,v;
  int face_index = face->face_index;
  int texinfo_id = tree->faces[face_index].texinfo_id;
  texinfo_t* texinfo = &tree->texinfo[texinfo_id];
  calculate_uv(tree,pos,texinfo,&u,&v);
  
  face->vertices[vertices_index++] = pos[0];face->vertices[vertices_index++] = pos[2];face->vertices[vertices_index++] = -pos[1];
  face->vertices[vertices_index++] = normal[0];face->vertices[vertices_index++] = normal[1];face->vertices[vertices_index++] = normal[2];
  face->vertices[vertices_index++] = u;face->vertices[vertices_index++] = v;
  
}

void createFaceVertices(Engine* engine,face_render* face){
  bsp_model_t* tree = engine->world.tree;
  int ledge_id = tree->faces[face->face_index].ledge_id;
  int num_ledges = tree->faces[face->face_index].ledge_num;
  int plane_index = tree->faces[face->face_index].plane_id;
  plane_t* plane = &tree->planes[plane_index];
  vec3 normal ;
  normal[0] = plane->normal.x;
  normal[1] = plane->normal.y;
  normal[2] = plane->normal.z;
  int texinfo_index = tree->faces[face->face_index].texinfo_id;
  texinfo_t* texinfo = &tree->texinfo[texinfo_index];

  int num_vertex_indices = num_ledges;
  int* vertex_indices = (int*)malloc(sizeof(int)*num_vertex_indices);
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
  int reverse = tree->faces[face->face_index].side;
  if(reverse){
    normal[0] = -normal[0];
    normal[1] = -normal[1];
    normal[2] = -normal[2];
  }
  int fpv = 8;
  face->vertices = (float*)malloc(sizeof(float)*(num_ledges-2)*fpv*3);
  face->num_vertices = (num_ledges-2)*3;
  int vertices_index = 0;
  for(int i=1;i<num_vertex_indices-1;i++){
    addTriangle(tree,face,normal,vertices_index,vertex_indices[0]);
    vertices_index+=8;
    if(!reverse){
      addTriangle(tree,face,normal,vertices_index,vertex_indices[i]);
      vertices_index+=8;
      addTriangle(tree,face,normal,vertices_index,vertex_indices[i+1]);
      vertices_index+=8;
    }else{
      addTriangle(tree,face,normal,vertices_index,vertex_indices[i+1]);
      vertices_index+=8;
      addTriangle(tree,face,normal,vertices_index,vertex_indices[i]);
      vertices_index+=8;
    }
  }
  free(vertex_indices);
}

// afloor3_t -> marble_texture2
// city2_2 city2_2 city2_3 city2_7 -> marble_texture1
// city5_3 ->concrete_texture1

uint8_t *LoadPng(char* address,int* width,int *height,int* channels){
  //int width,height,channels;
  uint8_t* data = stbi_load(address,width,height,channels,0);
  return data;
}

char* mapTextures(char* texture_name){
  FILE* f = fopen("..\\..\\materials\\materials.txt","rb");
  if(f == NULL){
    return NULL;
  }
  char* result = (char*)malloc(256);
  char line[256];
  char textures[256];
  while(fgets(line,sizeof(line),f)){
    char* token = strtok(line," ");
    if(!token){
      continue;
    }
    strcpy(textures,token);
    token = strtok(NULL, " \n");  // strip newline too
    if (!token) continue;
    strcpy(result,token);
    
    char* tex = strtok(textures,",");
    while(tex !=NULL){
      if(strcmp(tex,texture_name)==0){
	return result;
      }
      tex = strtok(NULL, ",");
    }
  }
  free(result);
  return NULL;
}

void loadTexturesToGpu(Engine* engine){
  engine->world.tree->my_texes = (my_miptex*)malloc(sizeof(my_miptex)*engine->world.tree->numMipTexes);
  my_miptex* mytexes = engine->world.tree->my_texes;
  int created = 0, skipped = 0;
  for(int i=0;i<engine->world.tree->numMipTexes;i++){
    (mytexes)[i].miptex_index = i;
    miptex_t* tex = &(engine->world.tree->miptexes[i]);
    if(!tex->offset1){
      skipped++;
      continue;
    }
    created++;
    uint8_t* pixels = engine->world.tree->pixels[i];
    int size = tex->width * tex->height;
    uint8_t* rgb = NULL;
    int width=0,height=0,channels=0;
    char* material_path = mapTextures(tex->name);

    printf("MATCHECK tex[%d] name=%s material_path=%s\n", i, tex->name, material_path ? material_path : "(null)");
    fflush(stdout);

    if(material_path){
      char* token = strtok(material_path, " \n\r");
      printf("MATCHECK token=%s\n", token ? token : "(null)");
      fflush(stdout);
      rgb = LoadPng(token,&width,&height,&channels);
      printf("MATCHECK LoadPng result rgb=%p\n", (void*)rgb);
      fflush(stdout);
    }else{
      rgb = (uint8_t*)malloc(size*3);
      width = tex->width;
      height = tex->height;
      for (int j = 0; j < size; j++) {
        uint8_t idx = pixels[j];
        rgb[j*3 + 0] = engine->world.tree->palette[idx][0];
        rgb[j*3 + 1] = engine->world.tree->palette[idx][1];
        rgb[j*3 + 2] = engine->world.tree->palette[idx][2];
      }
    }

    unsigned int tex_id;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width,height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    float aniso;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, aniso);
    (mytexes)[i].tex_id = tex_id;
    if(rgb) free(rgb);
  }
  printf("numMipTexes=%d created=%d skipped=%d\n", engine->world.tree->numMipTexes, created, skipped);
  fflush(stdout);
}
void bindVertices(Engine* engine,float* vertices,int num_vertices,int fpv){
  glGenVertexArrays(1, &(engine->world.mapVAO) );
  glGenBuffers(1, &(engine->world.mapVBO));

  glBindVertexArray(engine->world.mapVAO);

  glBindBuffer(GL_ARRAY_BUFFER, engine->world.mapVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*num_vertices*8, vertices, GL_STATIC_DRAW);

  // Set attribute pointers (for position only here)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
  glEnableVertexAttribArray(2);
  
  glBindVertexArray(0);
}

void bindFaceVertices(face_render* face){
  glGenVertexArrays(1, &(face->VAO) );
  glGenBuffers(1, &(face->VBO));

  glBindVertexArray(face->VAO);

  glBindBuffer(GL_ARRAY_BUFFER, face->VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*face->num_vertices*8, face->vertices, GL_STATIC_DRAW);

  // Set attribute pointers (for position only here)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
  glEnableVertexAttribArray(2);
  
  glBindVertexArray(0);
}

void bindMeshedFaceVertices(MeshedFace* face){
  glGenVertexArrays(1, &(face->VAO) );
  glGenBuffers(1, &(face->VBO));

  glBindVertexArray(face->VAO);

  glBindBuffer(GL_ARRAY_BUFFER, face->VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*face->num_vertices*face->fpv, face->vertices, GL_STATIC_DRAW);

  // Set attribute pointers (for position only here)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, face->fpv * sizeof(float), (void*)0);//position
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, face->fpv * sizeof(float), (void*)(3*sizeof(float)));//texture
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, face->fpv * sizeof(float), (void*)(5*sizeof(float)));//brightness
  glEnableVertexAttribArray(2);
  
  glBindVertexArray(0);
}

/*

leaf_render
  ↓ face_indexes[]
Faces
  ↓ ledge_id, ledge_num
Surfedges (signed edge indices)
  ↓ positive/negative
Edges (vertex pairs)
  ↓ vertex0, vertex1
Vertices (3D positions)

*/
