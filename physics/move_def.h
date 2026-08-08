#ifndef MOVE_DEF_H
#define MOVE_DEF_H

#include "../src/Game.h"
#include "../src/my_bsp.h"

typedef struct
{
  int	allsolid;	// if true, plane is not valid
  int	startsolid;	// if true, the initial point was in a solid area
  int	inopen, inwater;
  float		fraction;		// time completed, 1.0 = didn't hit anything
  vec3		endpos;			// final position
  plane_t	plane;			// surface normal at impact
  int		ent;			// entity the surface is on
} pmtrace_t;


void accelerate(player_t* player,vec3 wichdir,float wichspeed,float time_elapsed);
void air_accelerate(player_t* player,vec3 wichdir,float wichspeed,float time_elapsed);
void My_GroundMove(player_t* player,float time_elapsed);
int My_FlyMove(player_t* player,float time_elapsed);
void Air_Move(player_t* player,Camera* camera,float fmove,float smove,float time_elapsed);
int PM_ClipVelocity (vec3 in, vec3 normal, vec3 out, float overbounce);
void My_CategorizePosition(player_t* player);
void My_Friction(player_t* player,float elapsed_time);
void PlayerMove(player_t* player,Camera* camera,float smove,float fmove,float elapsed_time);
pmtrace_t PM_PlayerMove(player_t* player,vec3 start,vec3 end,float time_elapsed);
int PM_HullPointContents (hull_t *hull, int num, vec3 p);
int PM_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3 p1, vec3 p2, pmtrace_t *trace);



#endif
