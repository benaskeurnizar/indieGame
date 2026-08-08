#ifndef GAME_H
#define GAME_H

#include "cglm.h"
#include "mat4.h"
#include "vec4.h"
#include "vec3.h"
#include "my_bsp.h"
//TODO: Make this platform indipendent (check episode 011 of HandMade Hero)
typedef struct{
  BITMAPINFO BitMapInfo;
  void* Memory;
  int width;
  int height;
}BitMap;



typedef struct{
  mat4 view;
  vec3 pos;
  vec3 lookdir;
  vec3 up;
  vec3 WorldUp;
  vec3 right;
  float yaw;
  float pitch;
}Camera;

typedef struct {
  float* vertices;
  int numVertices;
  unsigned int VAO;
  unsigned int VBO;
  unsigned int TextureID;
  mat4 model;
  vec3 pos;
  vec3 scale;
}Object;

typedef struct {
  float* vertices;
  int numVertices;
  unsigned int VAO;
  unsigned int VBO;
  unsigned int TextureID;
  mat4 model;
  vec3 center;
  vec3 rightAxis;
  vec3 upAxis;
}CardObject;

typedef struct { 
  vec3 pos;
  vec3 lookdir;
  vec3 lightUp;
  vec3 lightRight;
  mat4 lightView;
  mat4 lightProj;
}LightSource;

typedef struct{
  unsigned int VertexShader;
  unsigned int FragmentShader;
  unsigned int ProgramShader;
}MyShaderProgram;

typedef struct{
  unsigned int frame_buffer;
  int width,height;
}MyFrameBuffer;

typedef struct{
  MyFrameBuffer fb;
  unsigned int depth_map;
}ShadowPass;

typedef struct{
  float* waterSurfaceVertices;
  int numSurfaceVertices;
  mat4 model;
  MyFrameBuffer reflection_framebuffer;
  MyFrameBuffer refraction_framebuffer;
  unsigned int reflection_texture;
  unsigned int refraction_texture;
  unsigned int dudvTexture,normalMapTexture;
  float width,height;
  vec3 world_pos;
  unsigned int VAO;
  unsigned int VBO;
  int textures_given;
  float waterHeight;
}WaterObject;

typedef struct{
  char* fps_text;
  int fps_text_size;

  char* faces_text;
  int faces_text_size;

  char* leaves_text;
  int leaves_text_size;
  unsigned int txtVBO,txtVAO;
  
  ShadowPass shadow3D;
  ShadowPass shadow2D;

  mat4 proj;

  MyShaderProgram ObjectsShader;
  MyShaderProgram CardObjectsShader;
  MyShaderProgram ShadowShader1;
  MyShaderProgram ShadowShader2;
  MyShaderProgram WaterShader;
  MyShaderProgram MapShader;
  MyShaderProgram TextShader;
  
  int screenWidth,screenHeight;
}Renderer;

typedef struct{
  bsp_model_t* tree;
  unsigned int mapVBO,mapVAO;
  //face_rt* faces_ray_tracing;
  //face_render* faces;
  
  
  Camera camera;
  Camera debugCamera;

  LightSource* lights;
  int num_lights;
  
  MeshedFace* meshed_faces;

  int* faces_indexes;//TODO: indexes of faces to render;
  int num_faces;//TODO: this is for rendering
}GameWorld;

typedef struct {
  int mouseX, mouseY;
  float DPitch,DYaw;
  int mouseLeftHeld;
  int mouseRightHeld;
  int keyW, keyA, keyS, keyD;
  int keyUp,keyDown,keyRight,keyLeft;
  int SpacePressed;
  int firstMousePress;
} InputState;

typedef struct{
  unsigned int TextureID;  // ID handle of the glyph texture
  float   Size[2];       // Size of glyph
  float   Bearing[2];    // Offset from baseline to left/top of glyph
  unsigned int Advance;    // Offset to advance to next glyph
}Character;


typedef struct
{
  clipnode_t	*clipnodes;
  plane_t	*planes;
  int		 firstclipnode;
  int		 lastclipnode;
  vec3      	clip_mins;
  vec3       	clip_maxs;
} hull_t;

typedef struct {
  vec3 pos;
  vec3 velocity;

  float yaw;
  float pitch;

  float radius;   // for collision
  float height;
  int on_ground;
  vec3 mins;
  vec3 maxs;
  hull_t* player_hull;
  int jumping;
} player_t;

typedef struct{
  player_t player;
  Renderer renderer;
  GameWorld world;
  InputState input;
  int first;//TODO: This is to be removed,just for testing!
  int refresh_rate_hz; // frame per secons
  Character* characters;
  int num_characters;// this is for rendering the indicator text
  int current_leaf; // this is for rendering the indicator text
  //float deltaTime;
  mnode_t* my_nodes;
  mleaf_t* my_leafs;
  int debug_mode;
}Engine;


void GameInit();
void GameUpdateAndRender(BitMap* Buffer);




#endif
