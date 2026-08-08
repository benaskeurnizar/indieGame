#include "cglm.h"
#include "mat4.h"
#include "vec4.h"
#include "vec3.h"
#include "Game.h"
#include "my_bsp.h"
#include <math.h>
#include "lighting.h"

#define ON_EPSILON 0.1f

tnode_t	   *tnode_p;
void MakeTnode (int nodenum,node_t* dnodes,mleaf_t* dleafs,plane_t* dplanes,tnode_t* tnodes)
{
  tnode_t			*t;
  plane_t		*plane;
  int				i;
  node_t 		*node;
	
  t = tnode_p++;

  node = dnodes + nodenum;
  plane = dplanes + node->plane_id;

  t->type = plane->type;
  //VectorCopy (plane->normal, t->normal);
  t->normal[0] = plane->normal.x;t->normal[1] = plane->normal.y;t->normal[2] = plane->normal.z;
  
  t->dist = plane->dist;
  
  if(node->front < 0){
    t->children[0] = dleafs[-node->front - 1].contents;
  }else{
     t->children[0] = tnode_p - tnodes;
     MakeTnode (node->front,dnodes,dleafs,dplanes,tnodes);
  }
  
  if(node->back < 0){
    t->children[1] = dleafs[-node->back - 1].contents;
  }else{
    t->children[1] = tnode_p - tnodes;
    MakeTnode (node->back,dnodes,dleafs,dplanes,tnodes);
  }
  
  /*for (i=0 ; i<2 ; i++)
    {
      if (node->children[i] < 0)
	t->children[i] = dleafs[-node->children[i] - 1].contents;
      else
	{
	  t->children[i] = tnode_p - tnodes;
	  MakeTnode (node->children[i]);
	}
	}*/
			
}

void MakeTnodes (Engine* engine,tnode_t** tnodes)
{
  tnode_p = *tnodes =(tnode_t*) malloc(engine->world.tree->numNodes * sizeof(tnode_t));
	
  MakeTnode (0,engine->world.tree->nodes,engine->my_leafs,engine->world.tree->planes,*tnodes);
}

int TestLine (vec3 start, vec3 stop,tnode_t* tnodes)
{
  int				node;
  float			front, back;
  tracestack_t	*tstack_p;
  int				side;
  float 			frontx,fronty, frontz, backx, backy, backz;
  tracestack_t	tracestack[64];
  tnode_t			*tnode;
	
  frontx = start[0];
  fronty = start[1];
  frontz = start[2];
  backx = stop[0];
  backy = stop[1];
  backz = stop[2];
	
  tstack_p = tracestack;
  node = 0;
	
  while (1)
    {
      while (node < 0 && node != -2)
	{
	  // pop up the stack for a back side
	  tstack_p--;
	  if (tstack_p < tracestack)
	    return 1;
	  node = tstack_p->node;
			
	  // set the hit point for this plane
			
	  frontx = backx;
	  fronty = backy;
	  frontz = backz;
			
	  // go down the back side

	  backx = tstack_p->backpt[0];
	  backy = tstack_p->backpt[1];
	  backz = tstack_p->backpt[2];
			
	  node = tnodes[tstack_p->node].children[!tstack_p->side];
	}

      if (node == -2)
	return 0;	// DONE!
		
      tnode = &tnodes[node];
		
      switch (tnode->type)
	{
	case 0:
	  front = frontx - tnode->dist;
	  back = backx - tnode->dist;
	  break;
	case 1:
	  front = fronty - tnode->dist;
	  back = backy - tnode->dist;
	  break;
	case 2:
	  front = frontz - tnode->dist;
	  back = backz - tnode->dist;
	  break;
	default:
	  front = (frontx*tnode->normal[0] + fronty*tnode->normal[1] + frontz*tnode->normal[2]) - tnode->dist;
	  back = (backx*tnode->normal[0] + backy*tnode->normal[1] + backz*tnode->normal[2]) - tnode->dist;
	  break;
	}

      if (front > -ON_EPSILON && back > -ON_EPSILON)
	//		if (front > 0 && back > 0)
	{
	  node = tnode->children[0];
	  continue;
	}
		
      if (front < ON_EPSILON && back < ON_EPSILON)
	//		if (front <= 0 && back <= 0)
	{
	  node = tnode->children[1];
	  continue;
	}

      side = front < 0;
		
      front = front / (front-back);
	
      tstack_p->node = node;
      tstack_p->side = side;
      tstack_p->backpt[0] = backx;
      tstack_p->backpt[1] = backy;
      tstack_p->backpt[2] = backz;
		
      tstack_p++;
		
      backx = frontx + front*(backx-frontx);
      backy = fronty + front*(backy-fronty);
      backz = frontz + front*(backz-frontz);
		
      node = tnode->children[side];		
    }	
}

void calculateMeshedFaceRT(Engine* engine,MeshedFace* face,tnode_t* tnodes){
  float intensity =700.0;
 
  hull_t* hull = (hull_t*)malloc(sizeof(hull_t));
  hull->clipnodes = (engine)->world.tree->clipnodes;
  hull->planes = (engine)->world.tree->planes;
  hull->firstclipnode = (engine)->world.tree->models[0].node_id0;
  hull->lastclipnode = (engine)->world.tree->numClipNodes - 1;
  glm_vec_zero(hull->clip_mins);
  glm_vec_zero(hull->clip_maxs);


  plane_t* plane = &engine->world.tree->planes[
					       engine->world.tree->faces[face->face_id].plane_id
					       ];
  vec3 face_normal_bsp;
  face_normal_bsp[0] = plane->normal.x;
  face_normal_bsp[1] = plane->normal.y;
  face_normal_bsp[2] = plane->normal.z;
  if(engine->world.tree->faces[face->face_id].side){
      face_normal_bsp[0] = -face_normal_bsp[0];
      face_normal_bsp[1] = -face_normal_bsp[1];
      face_normal_bsp[2] = -face_normal_bsp[2];
  }

  float k = 0.1;
  for(int i=0;i<face->num_vertices;i++){
    vec3 pos,pos_bsp;
    pos[0] = face->vertices[i*face->fpv];
    pos[1] = face->vertices[i*face->fpv +1];
    pos[2] = face->vertices[i*face->fpv +2];
    //TODO: is it in engine or quake space?
    glm_vec_copy(pos,pos_bsp);
    engine_pos_to_quake(pos,pos_bsp);
    
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
	float ndotl = glm_vec_dot(face_normal_bsp, L);
	if (ndotl < 0) ndotl = 0;

	float attenuation = 1.0f / (1.0f + k * 10 *d);

	float light_contrib = ndotl * attenuation;
	total_light += intensity *light_contrib;
      }
    }
    face->vertices[i*face->fpv +5] = total_light;
    if(face->vertices[i*face->fpv +5] < 0.05){
      face->vertices[i*face->fpv +5] = 0.05;
    }else if(face->vertices[i*face->fpv +5] > 1.0){
      face->vertices[i*face->fpv +5] = 1.0;
    }
    //face->vertices[i*face->fpv +5] = 1.0;
  }
}

/*
  CONTENTS_SOLID   = -2
  CONTENTS_EMPTY   = -1
  CONTENTS_WATER   = -3
  CONTENTS_SLIME   = -4
  CONTENTS_LAVA    = -5
*/
