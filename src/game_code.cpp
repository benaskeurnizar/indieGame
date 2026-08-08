#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "load_bsp.cpp"
#include "render_bsp.cpp"
#include "../physics/p_move.cpp"
#include "Game.h"
#include "../openGlPipeline/shaders.c"
#include "../renderer/render.c"
#include "cglm.h"
#include "mat4.h"
#include "vec4.h"
#include "vec3.h"
#include "lighting.cpp"
#include "triangulate.cpp"
typedef struct{
  uint32_t* memory;
  int bpp;
  int width;
  int height;
  uint32_t RedMask;
  uint32_t GreenMask;
  uint32_t BlueMask;
}LoadedBitMap;

float MyRadians(float angle) {
  vec3 vc;
  float result = (angle * M_PI) / 180.0;
  return result;
}

void addLightSource(int width,int height,GameWorld* world,vec3 pos,int index){
  glm_vec_copy(pos,world->lights[index].pos);
  glm_vec_copy(vec3{0.0,-1.0,-0.0},world->lights[index].lookdir);
  glm_vec_normalize(world->lights[index].lookdir);
  vec3 WorldUp;
  glm_vec_copy(vec3{0.0f, 1.0f, 0.0f},WorldUp);
  glm_vec_cross(WorldUp,world->lights[index].lookdir,world->lights[index].lightRight);
  glm_vec_normalize(world->lights[index].lightRight);
  glm_vec_cross(world->lights[index].lookdir,world->lights[index].lightRight,world->lights[index].lightUp);

  vec3 target;
  glm_vec_add(world->lights->pos, world->lights[index].lookdir, target);

  mat4 camMatrix;
  glm_lookat(world->lights[index].pos, target, world->lights[index].lightUp, camMatrix);
  glm_mat4_copy(camMatrix, world->lights[index].lightView);
  float near_plane = 1.0f, far_plane = 17.5f;
  glm_ortho(-10.0f, 10.0f,-10.0f, 10.0f,near_plane,far_plane,world->lights[index].lightProj);
}

void InitLightSource(Engine* engine,char* bsp_address){
  //NOTE: set an initial number of lights
  int initial_num_lights = 10;
  engine->world.num_lights = initial_num_lights;
  engine->world.lights = (LightSource*)malloc(sizeof(LightSource)*initial_num_lights);
  FILE* f = fopen(bsp_address,"rb");
  if (f == NULL) {
    printf("Error opening file \n");
    return;
  }
dentry_t* entities_lump = (dentry_t*)malloc(sizeof(dentry_t));
  //NOTE: All the offsets are counted from the start of the BSP files
  fseek(f, sizeof(int), SEEK_SET); 
  int read_size = fread(entities_lump,sizeof(dentry_t),1,f);
  if(read_size != 1 || entities_lump->size <= 0){
    printf("Error reading entities lump\n");
    free(entities_lump);
    fclose(f);
    return;
  }
  fseek(f,entities_lump->offset,SEEK_SET);
  char* lump_data = (char*)malloc(entities_lump->size + 1);
  fread(lump_data,1,entities_lump->size,f);
  lump_data[entities_lump->size] = '\0';  
  int num_lights = 0;
  char* p = lump_data;
  vec3 light_pos;
  while(*p ){
    while(*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    if(*p == '{'){
      p++;
      vec3 origin;
      char classname[64];
      float angle = 90.0;
      
      while(*p && *p != '}'){
	if(*p== '"' ){
	  p++;
	  char key[64] = {0};
	  int k=0;
	  while(*p && *p != '"') {key[k++] = *(p++);}
	  p++; // skip closing quote

	  //skipp all white spaces
	  while(*p && *p != '"') p++;

	  if(*p== '"' ) p++;
	  char value[64] = {0};
	  int v=0;
	  while(*p && *p != '"') {value[v++] = *(p++);}
	  if (*p == '"') p++;

	  // store key/value
	  if(strcmp(key,"classname") == 0){strcpy(classname,value);}
	  if(strcmp(key,"origin") == 0 ){
	    sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
	  }
	}
	p++;
      }
      if(strcmp(classname,"light") == 0){
	 light_pos[0] = origin[0];
	 light_pos[1] = origin[1];
	 light_pos[2] = origin[2];
	 if(num_lights >=  engine->world.num_lights){
	   initial_num_lights+= 10;
	   engine->world.num_lights = initial_num_lights;
	   engine->world.lights = (LightSource*)realloc(engine->world.lights,sizeof(LightSource)*initial_num_lights);
	 }
	 addLightSource(engine->renderer.screenWidth,engine->renderer.screenHeight,&(engine->world),light_pos,num_lights);
	 num_lights++;
	 //quake_pos_to_engine(map_pos, engine->player.pos);
      }
    }else{
      p++;
    }
  }
  if(num_lights <  engine->world.num_lights){
    engine->world.num_lights = num_lights;
    engine->world.lights = (LightSource*)realloc(engine->world.lights,sizeof(LightSource)*num_lights);
  }
}

void my_vec3_add(vec3 v1,vec3 v2,vec3 result){
  //vec3 result;
  result[0] = v1[0] + v2[0];
  result[1] = v1[1] + v2[1];
  result[2] = v1[2] + v2[2];
  //return result;
}
void my_vec3_sub(vec3 v1,vec3 v2,vec3 result){
  //vec3 result;
  result[0] = v1[0] - v2[0];
  result[1] = v1[1] - v2[1];
  result[2] = v1[2] - v2[2];
  //return result;
}

void updateCamera(float Dyaw,float Dpitch,Engine* engine) {//TODO TODO TODO : HERE

  engine->world.camera.pos[0] = engine->player.pos[0];
  engine->world.camera.pos[1] = engine->player.pos[1];
  engine->world.camera.pos[2] = engine->player.pos[2];

  //engine->world.camera.pos[2] -= engine->player.mins[2];

  engine->world.camera.yaw = engine->player.yaw;
  engine->world.camera.pitch = engine->player.pitch;
  vec3 front;
  float yawRad = glm_rad(engine->world.camera.yaw);
  float pitchRad = glm_rad(engine->world.camera.pitch);

  front[0] = cosf(yawRad) * cosf(pitchRad);
  front[1] = sinf(pitchRad);
  front[2] = sinf(yawRad) * cosf(pitchRad);
  glm_vec_normalize(front);
  glm_vec_copy(front,  engine->world.camera.lookdir);

  glm_vec_cross(engine->world.camera.lookdir, engine->world.camera.WorldUp,  engine->world.camera.right);
  glm_vec_cross( engine->world.camera.right,  engine->world.camera.lookdir,  engine->world.camera.up);
  //vec3 pos_scaled;
  //glm_vec_scale( engine->world.camera.lookdir,DForward,pos_scaled);
  //glm_vec_add( engine->world.camera.pos,pos_scaled, engine->world.camera.pos);
  vec3 target;
  glm_vec_add( engine->world.camera.pos,  engine->world.camera.lookdir, target);

  glm_lookat( engine->world.camera.pos, target,  engine->world.camera.up,  engine->world.camera.view);

}
void updateDebugCamera(float Dyaw,float Dpitch,Engine* engine){
  float forward_sensitivity = 5.3;
  float right_sensitivity = 5.3;
  float DForward = 0.0;
  float DRight = 0.0;
  int cameraUpdated = 0;
  if(engine->input.keyW){
    DForward = forward_sensitivity;
  }else if(engine->input.keyS){
    DForward = -forward_sensitivity;
  }else if(engine->input.keyS && engine->input.keyW){
    DForward = 0.0f;
  }

  if(engine->input.keyD){
    DRight = right_sensitivity;
  }else if(engine->input.keyA){
    DRight = -right_sensitivity;
  }else if(engine->input.keyA && engine->input.keyD){
    DRight = 0.0;
  }

  
  if(Dpitch == 0.0f && Dyaw == 0.0f && DForward == 0.0 &&DRight == 0.0){
    return;
  }
  engine->world.debugCamera.yaw   += Dyaw;
  engine->world.debugCamera.pitch += Dpitch;

  // Constrain pitch
  if ( engine->world.debugCamera.pitch > 89.0f)
     engine->world.debugCamera.pitch = 89.0f;
  if ( engine->world.debugCamera.pitch < -89.0f)
     engine->world.debugCamera.pitch = -89.0f;
  vec3 front;
  float yawRad = glm_rad(engine->world.debugCamera.yaw);
  float pitchRad = glm_rad(engine->world.debugCamera.pitch);

  front[0] = cosf(yawRad) * cosf(pitchRad);
  front[1] = sinf(pitchRad);
  front[2] = sinf(yawRad) * cosf(pitchRad);
  glm_vec_normalize(front);
  glm_vec_copy(front,  engine->world.debugCamera.lookdir);

  glm_vec_cross(engine->world.debugCamera.lookdir, engine->world.debugCamera.WorldUp,  engine->world.debugCamera.right);
  glm_vec_cross( engine->world.debugCamera.right,  engine->world.debugCamera.lookdir,  engine->world.debugCamera.up);
  vec3 pos_scaled;
  glm_vec_scale( engine->world.debugCamera.lookdir,DForward,pos_scaled);
  glm_vec_add( engine->world.debugCamera.pos,pos_scaled, engine->world.debugCamera.pos);
  vec3 target;
  glm_vec_add( engine->world.debugCamera.pos,  engine->world.debugCamera.lookdir, target);

  glm_lookat( engine->world.debugCamera.pos, target,  engine->world.debugCamera.up,  engine->world.debugCamera.view);
}

void OpenGlShaders(unsigned int* V,unsigned int* F,unsigned int* P,const char* VCodeAdr,const char* FCodeAdr){
  int size = 0;
  LinkVShader(V,VCodeAdr);
  LinkFrShader(F,FCodeAdr);
  LinkPShader(V,F,P);
}

void addMyProgramShader(MyShaderProgram* shader,const char* VCodeAdr,const char* FCodeAdr){
  LinkVShader(&(shader->VertexShader),VCodeAdr);
  LinkFrShader(&(shader->FragmentShader),FCodeAdr);
  LinkPShader(&(shader->VertexShader),&(shader->FragmentShader),&(shader->ProgramShader));
}

void initMainCamera(GameWorld* world,player_t player){
  world->camera.yaw = player.yaw;
  world->camera.pitch = player.pitch;

  world->camera.pos[0] = player.pos[0];
  world->camera.pos[1] = player.pos[1];
  world->camera.pos[2] = player.pos[2];

  world->camera.pos[1] += 22;

 
  glm_vec_copy(vec3{0.0f, 1.0f, 0.0f}, world->camera.up);
  glm_vec_copy(vec3{0.0f, 1.0f, 0.0f}, world->camera.WorldUp);
  glm_vec_copy(vec3{0.5,-0.5,1.0}, world->camera.lookdir);
  glm_vec_normalize(world->camera.lookdir);
  glm_vec_cross(world->camera.lookdir, world->camera.WorldUp, world->camera.right);
  
  vec3 target;
  glm_vec_add(world->camera.pos, world->camera.lookdir, target);

  mat4 camMatrix;
  glm_lookat(world->camera.pos, target, world->camera.up, camMatrix);
  glm_mat4_copy(camMatrix, world->camera.view);
}

void initDebugCamera(GameWorld* world){
  world->debugCamera.yaw = world->camera.yaw;
  world->debugCamera.pitch = world->camera.pitch;

  glm_vec_copy(world->camera.pos,world->debugCamera.pos);
  glm_vec_copy(world->camera.lookdir,world->debugCamera.lookdir);
  glm_vec_copy(world->camera.up,world->debugCamera.up);
  glm_vec_copy(world->camera.WorldUp,world->debugCamera.WorldUp);
  glm_vec_copy(world->camera.right,world->debugCamera.right);

  glm_mat4_copy(world->camera.view,world->debugCamera.view);
}

void init_player(Engine* engine,char* bsp_address){
  engine->player.pos[0] = 0.0;engine->player.pos[1] = 0.0;engine->player.pos[2] = 0.0;
  engine->player.yaw = 90.0;
  engine->player.pitch = 0.0;
  vec3 map_pos;
  //TODO: this is for finding info_player_start and setting the player pos.
  FILE* f = fopen(bsp_address,"rb");
  if (f == NULL) {
    printf("Error opening file \n");
    return;
  }
  dentry_t* entities_lump = (dentry_t*)malloc(sizeof(dentry_t));
  //NOTE: All the offsets are counted from the start of the BSP files
  fseek(f, sizeof(int), SEEK_SET); 
  int read_size = fread(entities_lump,sizeof(dentry_t),1,f);
  fseek(f,entities_lump->offset,SEEK_SET);
  char* lump_data = (char*)malloc(entities_lump->size);
  fread(lump_data,1,entities_lump->size,f);
  int found_player = 0;
  char* p = lump_data;
  while(*p && !found_player){
    while(*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    if(*p == '{'){
      p++;
      vec3 origin;
      char classname[64];
      float angle = 90.0;
      
      while(*p && *p != '}'){
	if(*p== '"' ){
	  p++;
	  char key[64] = {0};
	  int k=0;
	  while(*p && *p != '"') {key[k++] = *(p++);}
	  p++; // skip closing quote

	  //skipp all white spaces
	  while(*p && *p != '"') p++;

	  if(*p== '"' ) p++;
	  char value[64] = {0};
	  int v=0;
	  while(*p && *p != '"') {value[v++] = *(p++);}
	  if (*p == '"') p++;

	  // store key/value
	  if(strcmp(key,"classname") == 0){strcpy(classname,value);}
	  if(strcmp(key,"origin") == 0 ){
	    sscanf(value, "%f %f %f", &origin[0], &origin[1], &origin[2]);
	  }
	}
	p++;
      }
      if(strcmp(classname,"info_player_start") == 0){
	 map_pos[0] = origin[0];
	 map_pos[1] = origin[1];
	 map_pos[2] = origin[2];
	 //quake_pos_to_engine(map_pos, engine->player.pos);
	 engine->player.pos[0] = map_pos[0];  // X remains X
	 engine->player.pos[1] = map_pos[2] ;  // Engine Y was Quake Z
	 engine->player.pos[2] = -(map_pos[1]); // Engine Z was -Quake Y
	 found_player = 1;
      }
    }else{
      p++;
    }
  }
  //here
  engine->player.velocity[0] = 0.0;engine->player.velocity[1] = 0.0;engine->player.velocity[2] = 0.0;
  engine->player.on_ground = 0;
  engine->player.jumping = 0;
  engine->player.radius = 10.0;
  engine->player.height = 20.0;
  engine->player.mins[0] =-16.0;engine->player.mins[1] = -16.0;engine->player.mins[2] = -24.0;
  engine->player.maxs[0] = 16.0;engine->player.maxs[1] = 16.0;engine->player.maxs[2] = 32.0;

  //TODO: create the player's hull
  engine->player.player_hull = (hull_t*)malloc(sizeof(hull_t));
  engine->player.player_hull->clipnodes = engine->world.tree->clipnodes;//TODO: CREATE THE CLIPNODES
  engine->player.player_hull->planes = engine->world.tree->planes;
  engine->player.player_hull->firstclipnode = engine->world.tree->models[0].node_id1;
  engine->player.player_hull->lastclipnode = engine->world.tree->numClipNodes - 1;
  glm_vec_copy(engine->player.mins,engine->player.player_hull->clip_mins);
  glm_vec_copy(engine->player.maxs,engine->player.player_hull->clip_maxs);
}

int ray_hits_face(Engine* engine,hull_t* hull,vec3 face_center,vec3 light_pos,vec3 face_normal_bsp){
  pmtrace_t trace;
  memset(&trace, 0, sizeof(pmtrace_t));
  trace.fraction = 1.0f;
  trace.allsolid = 1;
  
  PM_RecursiveHullCheck(hull, hull->firstclipnode, 0.0f, 1.0f, light_pos,face_center, &trace);
  
  if (trace.fraction == 1.0f) {
    // not blocked, light reaches face
    //free(hull);
    return 1;
  }else if(trace.fraction < 1.0f){
    printf("test here \n");
  }
  
  //free(hull);
  return 0;
}

/*void calculate_faces_rt(Engine* engine){
  tnode_t* tnodes;
  MakeTnodes (engine,&tnodes);
  float intensity =300.0;
  //TODO: calculate faces brightness here
  //engine->world.faces_ray_tracing = (face_rt*)malloc(sizeof(face_rt)*(engine)->world.tree->numFaces);  
  hull_t* hull = (hull_t*)malloc(sizeof(hull_t));
  hull->clipnodes = (engine)->world.tree->clipnodes;
  hull->planes = (engine)->world.tree->planes;
  hull->firstclipnode = (engine)->world.tree->models[0].node_id0;
  hull->lastclipnode = (engine)->world.tree->numClipNodes - 1;
  glm_vec_zero(hull->clip_mins);
  glm_vec_zero(hull->clip_maxs);
  face_render temp_fr = {};
  for(int i=0;i<(engine)->world.tree->numFaces;i++){
    engine->world.faces_ray_tracing[i].face_index = i;
    engine->world.faces_ray_tracing[i].brightness = 0.05;
    temp_fr.face_index = i;
    createFaceVertices(engine,&temp_fr);
    bindFaceVertices(&temp_fr);

    vec3 face_center;
    glm_vec_zero(face_center);
    //NOTE : get face center
    for(int k=0;k<temp_fr.num_vertices;k++){
      //NOTE : there are 8 floats per vertice,hence the *8
      face_center[0]+=temp_fr.vertices[k*8];
      face_center[1]+=temp_fr.vertices[k*8 +1];
      face_center[2]+=temp_fr.vertices[k*8 +2];
    }
    face_center[0]/=temp_fr.num_vertices;
    face_center[1]/=temp_fr.num_vertices;
    face_center[2]/=temp_fr.num_vertices;
    vec3 face_center_bsp;
    engine_pos_to_quake(face_center,face_center_bsp);

    plane_t* plane = &engine->world.tree->planes[
						 engine->world.tree->faces[temp_fr.face_index].plane_id
						 ];

    vec3 face_normal_bsp;
    face_normal_bsp[0] = plane->normal.x;
    face_normal_bsp[1] = plane->normal.y;
    face_normal_bsp[2] = plane->normal.z;

    if(engine->world.tree->faces[temp_fr.face_index].side){
      face_normal_bsp[0] = -face_normal_bsp[0];
      face_normal_bsp[1] = -face_normal_bsp[1];
      face_normal_bsp[2] = -face_normal_bsp[2];
    }

    for(int h=0;h<3;h++){
      face_center_bsp[h] += face_normal_bsp[h]*0.5;
    }
 
    float total_light = 0.0f;
    for(int j=0;j<(engine)->world.num_lights;j++){
      if(TestLine ((engine)->world.lights[j].pos,face_center_bsp, tnodes)){
	
	float dist = glm_vec_distance(face_center_bsp,(engine)->world.lights[j].pos);
	//float attenuation = 1.0f / (dist * dist);
	float attenuation = 1.0f / (1 + 0.01 * dist);
	vec3 dir;

	glm_vec_sub((engine)->world.lights[j].pos, face_center_bsp, dir);
	glm_vec_normalize(dir);
	float angle = glm_vec_dot(face_normal_bsp, dir);
	//total_light += intensity * attenuation ;
	if (angle > 0)
	  total_light += intensity * attenuation * angle;
      }
    }
    engine->world.faces_ray_tracing[i].brightness = total_light;//TODO : clamp it to 1.0 and 0.0
    if(engine->world.faces_ray_tracing[i].brightness < 0.05){
      engine->world.faces_ray_tracing[i].brightness = 0.05;
    }else if(engine->world.faces_ray_tracing[i].brightness > 1.0){
      engine->world.faces_ray_tracing[i].brightness = 1.0;
    }
  }
  }*/

void GameInit(int width,int height,Engine** engine){
  printf("sizeof(plane_t) = %zu\n", sizeof(plane_t));
  printf("sizeof(bsp_vec3) = %zu\n", sizeof(bsp_vec3));
  //TODO: Create the engine global structure here and initialize it
  *engine = (Engine*)malloc(sizeof(Engine));
  (*engine)->debug_mode = 0;
  (*engine)->first = 1;
  (*engine)->refresh_rate_hz = 60;
  //TODO: Give it a GameWorld Struct and a Renderer Struct,initialize each of them
  //NOTE: The renderer here :
  (*engine)->renderer.fps_text_size = 64;
  (*engine)->renderer.fps_text = (char*)malloc((*engine)->renderer.fps_text_size);

  (*engine)->renderer.faces_text_size = 64;
  (*engine)->renderer.faces_text = (char*)malloc((*engine)->renderer.faces_text_size);
  
  (*engine)->renderer.leaves_text_size = 64;
  (*engine)->renderer.leaves_text = (char*)malloc((*engine)->renderer.leaves_text_size);
  
  (*engine)->renderer.screenWidth = width;
  (*engine)->renderer.screenHeight = height;

  (*engine)->current_leaf = 0;
  
  glm_perspective(MyRadians(60.0),(float) (width/height), 0.1f,10000.0f,(*engine)->renderer.proj);
  
  const char* QMapVertex = "..\\..\\shaders\\vertex.txt";  
  const char* QMapFragment = "..\\..\\shaders\\fragment.txt" ;
  
  addMyProgramShader(&((*engine)->renderer.MapShader),QMapVertex,QMapFragment);
  //NOTE: The renderer ends here

  //NOTE: create the GameWorld here
  //"E:\\2.5DIM\\mesh\\map_parsing\\maps\\1bsp7.bsp"
  //"E:\\my_maps\\tools\\ericw-tools-2.0.0-alpha10-win64\\myroom.bsp"
  char* address = "..\\..\\maps\\map5.bsp";
  (*engine)->world.debugCamera = {0};
  InitLightSource(*engine,address);
  
  //NOTE:the GameWorld creation ends here.

  //NOTE: THE INPUT STRUCT
  (*engine)->input = {0};
  (*engine)->input.firstMousePress = 1;
  //q1q3dm14
  //saint
  
  //"e:/2.5DIM/mesh/"
  //"E:\\my_maps\\tools\\ericw-tools-2.0.0-alpha10-win64\\myroom.bsp"
  
  if(loadBsp(address,&((*engine)->world.tree))){
    printf("success \n");
  }else{
    printf("failure \n");
    return;
  }
  //TODO: Create my nodes and my leafs here :
  (*engine)->my_leafs =  create_custom_leafs((*engine)->world.tree->leaves,(*engine)->world.tree->numLeaves);
  (*engine)->my_nodes =  create_custom_nodes((*engine)->world.tree->nodes,(*engine)->my_leafs,(*engine)->world.tree->numNodes);
  (*engine)->world.meshed_faces = (MeshedFace*)malloc(sizeof(MeshedFace)*(*engine)->world.tree->numFaces);
  tnode_t* tnodes;
  MakeTnodes (*engine,&tnodes);
  //NOTE: mesh the face
  for(int i=0;i<(*engine)->world.tree->numFaces;i++){
    (*engine)->world.meshed_faces[i].face_id = i;
    processFace(*engine,&((*engine)->world.meshed_faces[i]),&((*engine)->world.tree->faces[i]),tnodes);
    //mesh_face(*engine,&((*engine)->world.tree->faces[i]),&((*engine)->world.meshed_faces[i]),3.0);
    //calculateMeshedFaceRT(*engine,&((*engine)->world.meshed_faces[i]),tnodes);
    bindMeshedFaceVertices(&((*engine)->world.meshed_faces[i]));
    //bindMeshedFaceVertices(&((*engine)->world.meshed_faces[i]));
  }
  loadTexturesToGpu(*engine);
  //NOTE : this is a test to see if light one to light two hits
  pmtrace_t trace;
  memset(&trace, 0, sizeof(pmtrace_t));
  trace.fraction = 1.0f;
  trace.allsolid = 1;
  
  
  //verify_node_leaf_pointers((*engine)->my_nodes,(*engine)->my_leafs,(*engine)->world.tree->numNodes,(*engine)->world.tree->numLeaves);
  //find_duplicate_leaf_references((*engine)->my_nodes,(*engine)->my_leafs,(*engine)->world.tree->numNodes,(*engine)->world.tree->numLeaves);
  //calculate_faces_rt(*engine);
  //find_any_valid_leaf((*engine)->world.tree, (*engine)->my_nodes, valid_pos);
  init_player((*engine),address);
  initMainCamera(&((*engine)->world),(*engine)->player);
  //TODO: for the first frame :
  int num_visileaves = 0;
  int updated = 0;
  int* visileaves= getFrameVisileaves(*engine,&num_visileaves,&updated);
  leaf_render* frame_leaves =  getFrameFaces(*engine,visileaves,num_visileaves);
  (*engine)->world.num_faces = 0;
  for(int i = 0;i<num_visileaves;i++){
    (*engine)->world.num_faces+=frame_leaves[i].num_faces;
  }
  /*int num_faces = (*engine)->world.num_faces;
  (*engine)->world.faces = (face_render*)malloc(sizeof(face_render)*num_faces);
  int faces_index = 0;
  for(int i=0;i<num_visileaves;i++){
    for(int j=0;j<frame_leaves[i].num_faces;j++){
      (*engine)->world.faces[faces_index].face_index = frame_leaves[i].face_indexes[j];
      //TODO: add the face vertices
      createFaceVertices(*engine,&(*engine)->world.faces[faces_index]);
      bindFaceVertices(&(*engine)->world.faces[faces_index]);
      uint16_t plane_index = (*engine)->world.tree->faces[(*engine)->world.faces[faces_index].face_index].plane_id;
      plane_t plane = (*engine)->world.tree->planes[plane_index];
      
      (*engine)->world.faces[faces_index].brightness = (*engine)->world.faces_ray_tracing[frame_leaves[i].face_indexes[j]].brightness;
      faces_index++;
    }
    }*/

  int num_faces = (*engine)->world.num_faces;
  (*engine)->world.faces_indexes = (int*)malloc(sizeof(int)*num_faces);
  int faces_index = 0;
  for(int i=0;i<num_visileaves;i++){
    for(int j=0;j<frame_leaves[i].num_faces;j++){
      //(*engine)->world.meshed_faces[faces_index].face_id = frame_leaves[i].face_indexes[j];
      (*engine)->world.faces_indexes[faces_index] = frame_leaves[i].face_indexes[j];
      //NOTE: add and bindthe face vertices
      //bindMeshedFaceVertices(&((*engine)->world.meshed_faces[frame_leaves[i].face_indexes[j]]));
      //createFaceVertices(engine,&engine->world.faces[faces_index]);
      //bindFaceVertices(&engine->world.faces[faces_index]);
      //engine->world.faces[faces_index].brightness = engine->world.faces_ray_tracing[frame_leaves[i].face_indexes[j]].brightness;
      faces_index++;
    }
  }
  free(visileaves);
  for(int i=0;i<num_visileaves;i++){
    free(frame_leaves[i].face_indexes);
  }
  free(frame_leaves);
}


uint16_t getLeftShift(uint32_t value){
  uint16_t result = 0;
  while(!((value >> result ) & 1)){
    result ++;
  }
  return result;
}
uint16_t getShift(uint32_t mask) {
    uint16_t shift = 0;
    if (mask == 0) return 0;  // no channel
    while ((mask & 1) == 0) {
        mask >>= 1;
        shift++;
    }
    return shift;
}
void BindTexture(CardObject* object,LoadedBitMap* bitmap){
  uint16_t rShift = getLeftShift(bitmap->RedMask);
  uint16_t gShift = getLeftShift(bitmap->GreenMask);
  uint16_t bShift = getLeftShift(bitmap->BlueMask);
  uint32_t AlphaMask = ~(bitmap->RedMask | bitmap->GreenMask | bitmap->BlueMask);
  uint16_t alphaShift = getShift(AlphaMask);
  
  int width, height, nrChannels;
  //unsigned char* data = stbi_load(object->texture_adress, &width, &height, &nrChannels, 0);
  printf("here \n");
  glGenTextures(1, &(object->TextureID));
  glBindTexture(GL_TEXTURE_2D, object->TextureID);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (bitmap->memory) {
    uint8_t* rgba =(uint8_t*) malloc(bitmap->width*bitmap->height * bitmap->bpp);
    for(int i = 0;i < (bitmap->width*bitmap->height);i++){
      uint32_t pixel = bitmap->memory[i];
      uint8_t r = (pixel & bitmap->RedMask)   >> rShift;
      uint8_t g = (pixel & bitmap->GreenMask) >> gShift;
      uint8_t b = (pixel & bitmap->BlueMask)  >> bShift;
      uint8_t alpha = (pixel & AlphaMask)  >> alphaShift;
      rgba[i*4 + 0 ] = r;
      rgba[i*4 + 1 ] = g;
      rgba[i*4 + 2 ] = b;
      rgba[i*4 + 3 ] = alpha;
    }
    //glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_RGBA,
		 GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    free(rgba);
  } else {
    printf("no bitmap memory!\n");
  }
}

typedef struct{
  int8_t type;
  int8_t id;
  float  pos_x;
  float  pos_y;
  float  pos_z;
}ObjectHeader;


void updateFrameFaces(Engine* engine){
  int num_visileaves = 0;
  int updated = 0;
  int* visileaves= getFrameVisileaves(engine,&num_visileaves,&updated);
  if(!updated){

  }else{
    /*if(engine->world.faces){
      for(int i=0;i< engine->world.num_faces;i++){
	if(engine->world.faces[i].vertices){
	  free(engine->world.faces[i].vertices);
	  engine->world.faces[i].vertices = NULL;
	}
	glDeleteBuffers(1, &engine->world.faces[i].VBO);
	glDeleteVertexArrays(1, &engine->world.faces[i].VAO);
      }
      free(engine->world.faces);
      engine->world.faces = NULL;
      engine->world.num_faces = 0;
    }*/
    if(engine->world.faces_indexes){
      for(int i=0;i<engine->world.num_faces;i++){
	int index = engine->world.faces_indexes[i];
	//glDeleteBuffers(1, &engine->world.meshed_faces[index].VBO);
	//glDeleteVertexArrays(1, &engine->world.meshed_faces[index].VAO);
      }
      free(engine->world.faces_indexes);
      engine->world.num_faces = 0;
    }
    leaf_render* frame_leaves =  getFrameFaces(engine,visileaves,num_visileaves);
    engine->world.num_faces = 0;
    for(int i = 0;i<num_visileaves;i++){
      engine->world.num_faces+=frame_leaves[i].num_faces;
    }
    int num_faces = engine->world.num_faces;
    engine->world.faces_indexes = (int*)malloc(sizeof(int)*num_faces);
    int faces_index = 0;
    for(int i=0;i<num_visileaves;i++){
      for(int j=0;j<frame_leaves[i].num_faces;j++){
	//engine->world.meshed_faces[faces_index].face_id = frame_leaves[i].face_indexes[j];
	engine->world.faces_indexes[faces_index] = frame_leaves[i].face_indexes[j];
	//NOTE: add and bindthe face vertices
	//bindMeshedFaceVertices(&((engine)->world.meshed_faces[frame_leaves[i].face_indexes[j]]));
	//createFaceVertices(engine,&engine->world.faces[faces_index]);
	//bindFaceVertices(&engine->world.faces[faces_index]);
       	//engine->world.faces[faces_index].brightness = engine->world.faces_ray_tracing[frame_leaves[i].face_indexes[j]].brightness;
	faces_index++;
      }
    }
    free(visileaves);
    for(int i=0;i<num_visileaves;i++){
      free(frame_leaves[i].face_indexes);
    }
    free(frame_leaves);
  }
}

void update_player_pos(Engine* engine,int* moved){
  float forward_sensitivity = 45;
  float right_sensitivity = 20;
  float DForward = 0.0;
  float DRight = 0.0;
  int cameraUpdated = 0;
  int jump =0;
  if(engine->input.keyW){
    DForward = forward_sensitivity;
  }else if(engine->input.keyS){
    DForward = -forward_sensitivity;
  }else if(engine->input.keyS && engine->input.keyW){
    DForward = 0.0f;
  }

  if(engine->input.keyD){
    DRight = right_sensitivity;
  }else if(engine->input.keyA){
    DRight = -right_sensitivity;
  }else if(engine->input.keyA && engine->input.keyD){
    DRight = 0.0;
  }

  if(engine->input.SpacePressed){
    jump = 1;
  }
  
  float frametime = 1.0f / 30.0f;   // ≈ 0.0333333f
  float fmove = DForward ;float smove = DRight; 
  engine->player.yaw   += engine->input.DYaw;
  engine->player.pitch += engine->input.DPitch;

  // Constrain pitch
  if ( engine->player.pitch > 89.0f)
     engine->player.pitch = 89.0f;
  if ( engine->player.pitch < -89.0f)
     engine->player.pitch = -89.0f;

  vec3 old_pos;
  glm_vec_copy(engine->player.pos,old_pos);
  PlayerMove(&engine->player,&engine->world.camera,smove,fmove,jump,frametime);
  if((engine->player.pos[0] != old_pos[0]) || (engine->player.pos[1] != old_pos[1]) || (engine->player.pos[2] != old_pos[2])){
    *moved = 1;
  }
}
void GameUpdate(Engine* engine,double elapsed_time){
  int player_moved = 0;
  //NOTE: update player position
  update_player_pos(engine,&player_moved);
  if( player_moved && !engine->debug_mode ){
    //NOTE: update frame visible faces
    updateFrameFaces(engine);
  }
  
  if(engine->debug_mode){
    updateDebugCamera(engine->input.DYaw,engine->input.DPitch,engine);
  }else{
    updateCamera(engine->input.DYaw,engine->input.DPitch,engine);
  }
  engine->input.DYaw = 0.0;
  engine->input.DPitch = 0.0;
  engine->input.SpacePressed = 0;
}


// renderer should have my own renderer,openGl or vulkan,and this file doesn't have to know any of that,i just pass it the faces and objects to render
void GameRender(Engine* engine){
  renderFrameFaces(engine);
}

/*
what happens each frame ?
update the player/camera based on input state ,with collision detecion
get the new leaf the player is at
get visible leaves from that leave
get all faces to render
get objects to render (to add)
call game render with all that's before
empty the input state
*/
