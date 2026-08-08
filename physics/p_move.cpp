#include "cglm.h"
#include "mat4.h"
#include "vec4.h"
#include "vec3.h"
#include "move_def.h"
#include "../src/Game.h"
#include "../src/my_bsp.h"


#define	MAX_CLIP_PLANES	5
#define	STOP_EPSILON	0.1
#define	STEPSIZE	18
#define	DIST_EPSILON	(0.03125)
#define	CONTENTS_EMPTY		-1
#define	CONTENTS_SOLID		-2
#define	CONTENTS_WATER		-3
#define	CONTENTS_SLIME		-4
#define	CONTENTS_LAVA		-5
#define	CONTENTS_SKY		-6
#define	CONTENTS_ORIGIN		-7		// removed at csg time
#define	CONTENTS_CLIP		-8		// changed to contents_solid

#define	CONTENTS_CURRENT_0		-9
#define	CONTENTS_CURRENT_90		-10
#define	CONTENTS_CURRENT_180	-11
#define	CONTENTS_CURRENT_270	-12
#define	CONTENTS_CURRENT_UP		-13
#define	CONTENTS_CURRENT_DOWN	-14

float entgravity = 1.0f;
float gravity = 800.0f;
float accelerate_val = 9.8f;
float Gfriction = 1.8;

float VectorNormalize (vec3 v)
{
  float length, ilength;
  length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
  length = sqrt (length);
  // FIXME
  if (length)
    {
      ilength = 1/length;
      v[0] *= ilength;
      v[1] *= ilength;
      v[2] *= ilength;
    }
  return length;
}

void accelerate(player_t* player,vec3 wichdir,float wichspeed,float time_elapsed){
  float current_speed = glm_vec_dot(player->velocity,wichdir);
  float add_speed = wichspeed - current_speed;
  if(add_speed <= 0.0){
    return;
  }
  float accelspeed = accelerate_val*wichspeed*time_elapsed;
  if(accelspeed > add_speed){
    accelspeed = add_speed;
  }
  for(int i=0;i<3;i++){
    player->velocity[i] += accelspeed * wichdir[i];
  }
}


void air_accelerate(player_t* player,vec3 wichdir,float wichspeed,float time_elapsed){
  float wichspd = wichspeed;
  if (wichspd > 30)
    wichspd = 30;
  float currentspeed = glm_vec_dot(player->velocity,wichdir);
  float addspeed = wichspd- currentspeed;
  if (addspeed <= 0)
    return;
  float accelspeed = accelerate_val * wichspeed * time_elapsed;
  if (accelspeed > addspeed)
    accelspeed = addspeed;
  for (int i=0 ; i<3 ; i++)
    player->velocity[i] += accelspeed*wichdir[i];
}

void My_GroundMove(player_t* player,float time_elapsed){
  vec3	start, dest;
  pmtrace_t	trace;
  vec3	original, originalvel, down, up, downvel;
  float	downdist, updist;

  player->velocity[2] = 0;
  if (!player->velocity[0] && !player->velocity[1] && !player->velocity[2])
    return;

  // first try just moving to the destination	
  dest[0] = player->pos[0] + player->velocity[0]*time_elapsed;
  dest[1] = player->pos[1] + player->velocity[1]*time_elapsed;	
  dest[2] = player->pos[2];

  // first try moving directly to the next spot
  glm_vec_copy(dest, start);
  trace = PM_PlayerMove (player,player->pos, dest,time_elapsed);

  if (trace.fraction == 1)
    {
      glm_vec_copy(trace.endpos, player->pos);
      return;
    }

  // try sliding forward both on ground and up 16 pixels
  // take the move that goes farthest
  glm_vec_copy(player->pos, original);
  glm_vec_copy(player->velocity, originalvel);

  My_FlyMove(player,time_elapsed);
  
  glm_vec_copy(player->pos, down);
  glm_vec_copy(player->velocity, downvel);

  glm_vec_copy(original, player->pos);
  glm_vec_copy(originalvel,player->velocity);

  // move up a stair height
  glm_vec_copy(player->pos, dest);
  dest[2] += STEPSIZE;
  trace = PM_PlayerMove (player,player->pos, dest,time_elapsed);
  if (!trace.startsolid && !trace.allsolid)
    {
      glm_vec_copy(trace.endpos, player->pos);
    }
  // slide move
  My_FlyMove(player,time_elapsed);

  // press down the stepheight
  glm_vec_copy(player->pos, dest);
  dest[2] -= STEPSIZE;
  trace = PM_PlayerMove (player,player->pos, dest,time_elapsed);
  if ( trace.plane.normal.z < 0.7){
    goto usedown;
  }
  if (!trace.startsolid && !trace.allsolid)
    {
      glm_vec_copy(trace.endpos, player->pos);
    }
  glm_vec_copy(player->pos, up);

  // decide which one went farther
  downdist = (down[0] - original[0])*(down[0] - original[0])
    + (down[1] - original[1])*(down[1] - original[1]);
  
  updist = (up[0] - original[0])*(up[0] - original[0])
    + (up[1] - original[1])*(up[1] - original[1]);

  if(downdist > updist){
  usedown:
    glm_vec_copy(down, player->pos);
    glm_vec_copy(downvel, player->velocity);
  }else{
    player->velocity[2] = downvel[2];
  }
}

int My_FlyMove(player_t* player,float time_elapsed){
  
  int     bumpcount, numbumps;
  vec3    dir;
  float	  d;
  int	  numplanes;
  vec3    planes[MAX_CLIP_PLANES];
  vec3	  primal_velocity, original_velocity;
  int	  i, j;
  pmtrace_t   trace;
  vec3	      end;
  float	      time_left;
  int	      blocked;

  vec3 vec3_origin;
  vec3_origin[0] = 0.0;vec3_origin[1] = 0.0;vec3_origin[2] = 0.0;
  
  numbumps = 4;
	
  blocked = 0;
  glm_vec_copy(player->velocity, original_velocity);
  glm_vec_copy(player->velocity, primal_velocity);
  numplanes = 0;
	
  time_left = time_elapsed;

  for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++){
    
    for (i=0 ; i<3 ; i++){
	end[i] = player->pos[i] + time_left * player->velocity[i];
    }
    trace = PM_PlayerMove(player,player->pos, end,time_elapsed);

    if (trace.startsolid || trace.allsolid)
      {	// entity is trapped in another solid
	glm_vec_copy(vec3_origin,player->velocity);
	return 3;
      }

    if (trace.fraction > 0)
      {	// actually covered some distance
	glm_vec_copy(trace.endpos, player->pos);
	numplanes = 0;
      }
    if (trace.fraction == 1)
      break;		// moved the entire distance


    //TODO: don't know what this is but :
    // save entity for contact

    if (trace.plane.normal.z > 0.7)
      {
	blocked |= 1;		// floor
      }
    if (!trace.plane.normal.z)
      {
	blocked |= 2;		// step
      }

    time_left -= time_left * trace.fraction;
		
    // cliped to another plane
    if (numplanes >= MAX_CLIP_PLANES)
      {	// this shouldn't really happen
	glm_vec_copy(vec3_origin,player->velocity);
	break;
      }

    vec3 trace_plane_normal;
    trace_plane_normal[0] = trace.plane.normal.x;
    trace_plane_normal[1] = trace.plane.normal.y;
    trace_plane_normal[2] = trace.plane.normal.z;
    glm_vec_copy(trace_plane_normal, planes[numplanes]);
    numplanes++; //TODO : here

    //
    // modify original_velocity so it parallels all of the clip planes
    //
    for(i=0;i<numplanes;i++){
      PM_ClipVelocity (original_velocity, planes[i], player->velocity, 1);
      for(j=0;j<numplanes;j++){
	if(j != i){
	  if (glm_vec_dot(player->velocity, planes[j]) < 0)
	    break;
	}
      }
      if(j == numplanes){
	break;
      }
    }
    if (i != numplanes)
      {	// go along this plane
      }
    else
      {	// go along the crease
	if(numplanes !=2){
	  glm_vec_copy(vec3_origin,player->velocity);
	  break;
	}
	glm_vec_cross(planes[0],planes[1],dir);
	d = glm_vec_dot(dir,player->velocity);
	glm_vec_scale(dir,d,player->velocity);
      }

    //
    // if original velocity is against the original velocity, stop dead
    // to avoid tiny occilations in sloping corners
    //
    if(glm_vec_dot(player->velocity,primal_velocity) <= 0){
      glm_vec_copy(vec3_origin,player->velocity);
      break;
    }
  }
  //TODO: there is some sort of water jump shit check here
  
  return blocked;
}

void Air_Move(player_t* player,Camera* camera,float fmove,float smove,float time_elapsed){
  vec3 forward,right;
  engine_pos_to_quake(camera->lookdir,forward);
  engine_pos_to_quake(camera->right,right);
  forward[2] = 0.0;
  right[2] = 0.0;
  glm_vec_normalize(forward);glm_vec_normalize(right);
  vec3 wishvel,wichdir;
  glm_vec_zero(wichdir);glm_vec_zero(wishvel);
  for(int i=0;i<2;i++){
    wishvel[i] = forward[i]*fmove + right[i]*smove;
  }
  wishvel[2] = 0;
  
  glm_vec_copy(wishvel,wichdir);
  float wishspeed = VectorNormalize(wichdir);
  
  //TODO:
  //
  // clamp to server defined max speed
  //
  if (wishspeed > 3000.0)
    {
      float val = 200.0/wishspeed;
      wishvel[0]*=val;wishvel[1]*=val;wishvel[2]*=val;
      //VectorScale (wishvel, movevars.maxspeed/wishspeed, wishvel);
      wishspeed = 3000;
    }

  if(player->on_ground != -1){
    player->velocity[2] = 0.0;
    accelerate(player,wichdir,wishspeed,time_elapsed);
    player->velocity[2] -= gravity*entgravity*time_elapsed;
    //TODO: ground move here
    My_GroundMove(player,time_elapsed);
    
  }else{
    air_accelerate(player,wichdir,wishspeed,time_elapsed);
    player->velocity[2] -= gravity*entgravity*time_elapsed;

    //TODO: FLY move here
    My_FlyMove(player,time_elapsed);
  }
}


int PM_ClipVelocity (vec3 in, vec3 normal, vec3 out, float overbounce){//TODO: this is for changing our velocity based on the plane we collided with!
  float	backoff;
  float	change;
  int  	i, blocked;
	
  blocked = 0;
  if (normal[2] > 0)
    blocked |= 1;		// floor
  if (!normal[2])
    blocked |= 2;		// step
	
  backoff = glm_vec_dot(in, normal) * overbounce;

  for (i=0 ; i<3 ; i++)
    {
      change = normal[i]*backoff;
      out[i] = in[i] - change;
      if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
	out[i] = 0;
    }
	
  return blocked;
}

void My_CategorizePosition(player_t* player){
  vec3		point;
  int	       	cont;
  pmtrace_t    	tr;
  float frame_time = 1.0/30.0;
  // if the player hull point one unit down is solid, the player
  // is on ground

  // see if standing on something solid	
  point[0] = player->pos[0];
  point[1] = player->pos[1];
  point[2] = player->pos[2] - 1;
  if (player->velocity[2] > 180)
    {
      player->on_ground = -1;
    }
  else
    {
      tr = PM_PlayerMove(player,player->pos, point,frame_time);
      if ( tr.plane.normal.z < 0.7)
	player->on_ground = -1;	// too steep
      else
	player->on_ground = tr.ent;
      if (player->on_ground != -1)
	{
	  //pmove.waterjumptime = 0;
	  if (!tr.startsolid && !tr.allsolid)
	    glm_vec_copy(tr.endpos, player->pos);
	}

      // standing on an entity other than the world
      if (tr.ent > 0)
	{
	  //NOTE: not sure what these are!
	  //pmove.touchindex[pmove.numtouch] = tr.ent;
	  //pmove.numtouch++;
	}
    }
  //NOTE: there's more but it's for water soo... i guess not now!
}

void My_Friction(player_t* player,float elapsed_time){
  float	*vel;
  float	speed, newspeed, control;
  float	friction;
  float	drop;
  vec3	start, stop;
  pmtrace_t    	trace;
	
  //if (pmove.waterjumptime)
  // return;

  vel = player->velocity;
	
  speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2]);
  if (speed < 1)
    {
      vel[0] = 0;
      vel[1] = 0;
      return;
    }

  friction = Gfriction;

  // if the leading edge is over a dropoff, increase friction
  if (player->on_ground != -1) {
    start[0] = stop[0] = player->pos[0] + vel[0]/speed*16;
    start[1] = stop[1] = player->pos[1] + vel[1]/speed*16;
    start[2] = player->pos[2] + player->mins[2];
    stop[2] = start[2] - 34;

    trace = PM_PlayerMove (player,start, stop,elapsed_time);

    if (trace.fraction == 1) {
      friction *= 2;
    }
  }

  drop = 0;
  float stopspeed = 0.01;
  if (0){ // apply water friction//TODO: this is supposed to be some water thing,but we have no water
    //drop += speed*waterfriction*waterlevel*frametime;
  }
  else if (player->on_ground != -1)  // apply ground friction
    {
      control = speed < stopspeed ? stopspeed : speed;
      drop += control*friction*elapsed_time;
    }


  // scale the velocity
  newspeed = speed - drop;
  if (newspeed < 0)
    newspeed = 0;
  newspeed /= speed;

  vel[0] = vel[0] * newspeed;
  vel[1] = vel[1] * newspeed;
  vel[2] = vel[2] * newspeed;
}



int PM_TestPlayerPosition (player_t* player,vec3 pos)
{
  int      i;
  //physent_t	*pe;
  vec3		mins, maxs, test;
  hull_t   	*hull;

  vec3 origin;
  glm_vec_zero(origin);
  for (i=0 ; i< 1 ; i++)
    {
      hull = player->player_hull;
      glm_vec_sub(pos, origin, test);

      if (PM_HullPointContents (hull, hull->firstclipnode, test) == CONTENTS_SOLID)
	return 0;
    }

  return 1;
}


void NudgePosition (player_t* player)
{
  vec3	base;
  int  	x, y, z;
  int  	i;
  static int		sign[3] = {0, -1, 1};

  glm_vec_copy(player->pos, base);

  for (i=0 ; i<3 ; i++)
    player->pos[i] = ((int)(player->pos[i]*8)) * 0.125;
  //if (PM_TestPlayerPosition (pmove.origin) )
 

  for (z=0 ; z<=2 ; z++)
    {
      for (x=0 ; x<=2 ; x++)
	{
	  for (y=0 ; y<=2 ; y++)
	    {
	      player->pos[0] = base[0] + (sign[x] * 1.0/8);
	      player->pos[1] = base[1] + (sign[y] * 1.0/8);
	      player->pos[2] = base[2] + (sign[z] * 1.0/8);
	      if (PM_TestPlayerPosition (player,player->pos))
		return;
	    }
	}
    }
  glm_vec_copy(base, player->pos);
  //	Con_DPrintf ("NudgePosition: stuck\n");
}

void NudgePositionVect(vec3 pos){
   int  	x, y, z;
   int  	i;
   static int		sign[3] = {0, -1, 1};
   
}

void playerJump(player_t* player){
  if(player->on_ground == -1){
    return;
  }
  if(player->jumping){
    return;
  }
  player->on_ground = -1;
  player->velocity[2] += 470;

  player->jumping = 1;
}

void PlayerMove(player_t* player,Camera* camera,float smove,float fmove,int jump,float elapsed_time){

  //TODO : This is to be added later
  //NudgePosition ();
  player_t* temp_player = (player_t*)malloc(sizeof(player_t));
  memset(temp_player,0,sizeof(player_t));
  engine_pos_to_quake(player->pos,temp_player->pos);
  engine_pos_to_quake(player->velocity,temp_player->velocity);
  //glm_vec_copy(player->velocity, temp_player->velocity);
  temp_player->player_hull = player->player_hull;
  temp_player->pitch = player->pitch;
  temp_player->yaw = player->yaw;
  temp_player->radius = player->radius;
  temp_player->height = player->height;
  temp_player->on_ground = player->on_ground;
  glm_vec_copy(player->mins,temp_player->mins);
  glm_vec_copy(player->maxs,temp_player->maxs);


  NudgePosition (temp_player);
  // set onground, watertype, and waterlevel
  My_CategorizePosition(temp_player);

  //TODO: if jump button is pressed,jump here
  if(jump){
    playerJump(temp_player);
  }else{
    temp_player->jumping = 0;
  }
  My_Friction(temp_player,elapsed_time); 

  //TODO: for now there is no water so we always move on the air
  //TODO: calculate fmove and smove from input struct.
  Air_Move(temp_player,camera,fmove,smove,elapsed_time);

  // set onground, watertype, and waterlevel for final spot
  My_CategorizePosition(temp_player);


  quake_pos_to_engine(temp_player->pos,player->pos);
  //glm_vec_copy(temp_player->velocity, player->velocity);
  quake_pos_to_engine(temp_player->velocity,player->velocity);
  player->player_hull = temp_player->player_hull;
  player->pitch = temp_player->pitch;
  player->yaw = temp_player->yaw;
  player->radius = temp_player->radius;
  player->height = temp_player->height;
  player->on_ground = temp_player->on_ground;
  glm_vec_copy(temp_player->mins,player->mins);
  glm_vec_copy(temp_player->maxs,player->maxs);
}


pmtrace_t PM_PlayerMove(player_t* player,vec3 start,vec3 end,float time_elapsed){
  pmtrace_t   	trace, total;
  vec3		offset;
  vec3		start_l, end_l;
  hull_t      	*hull;
  int		i;
  //physent_t	*pe;
  vec3		mins, maxs;

  memset (&total, 0, sizeof(pmtrace_t));
  total.fraction = 1;
  total.ent = -1;
  glm_vec_copy(end, total.endpos);
  
  vec3 world_mins,world_maxes,origin;
  world_mins[0] = 0.0;world_mins[1] = 0.0;world_mins[2] = 0.0;
  world_maxes[0] = 0.0;world_maxes[1] = 0.0;world_maxes[2] = 0.0;
  origin[0] = 0.0;origin[1] = 0.0;origin[2] = 0.0;
  
  int numphysent = 1;
  for(i=0;i<numphysent;i++){
    // get the clipping hull TODO: since for now we only have the world,
    hull = player->player_hull;

    glm_vec_copy(origin,offset);
    glm_vec_sub(start, offset, start_l);
    glm_vec_sub(end, offset, end_l);

    // fill in a default trace
    memset (&trace, 0, sizeof(pmtrace_t));
    trace.fraction = 1;
    trace.allsolid = 1;
    //		trace.startsolid = true;
    glm_vec_copy(end, trace.endpos);

    // trace a line through the apropriate clipping hull
    PM_RecursiveHullCheck (hull, hull->firstclipnode, 0, 1, start_l, end_l, &trace);

    if (trace.allsolid)
      trace.startsolid = 1;
    if (trace.startsolid)
      trace.fraction = 0;

    // did we clip the move?
    if (trace.fraction < total.fraction)
      {
	// fix trace up by the offset
	glm_vec_add(trace.endpos, offset, trace.endpos);
	total = trace;
	total.ent = i;
      }
  }
  return total;
}

int PM_HullPointContents (hull_t *hull, int num, vec3 p)
{
  float		d;
  clipnode_t	*node;
  plane_t	*plane;

  while (num >= 0)
    {
      if (num < hull->firstclipnode || num > hull->lastclipnode)
	printf("PM_HullPointContents: bad node number");
	
      node = hull->clipnodes + num;
      plane = hull->planes + node->planenum;
      vec3 plane_normal;
      plane_normal[0] = plane->normal.x;plane_normal[1] = plane->normal.y;plane_normal[2] = plane->normal.z;
      
      if (plane->type < 3)
	d = p[plane->type] - plane->dist;
      else
	d = glm_vec_dot(plane_normal, p) - plane->dist;
      if (d < 0)
	//num = node->back;
	num = node->children[1];
      else
	//num = node->front;
	num = node->children[0];
    }
	
  return num;
}

int PM_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3 p1, vec3 p2, pmtrace_t *trace){
  clipnode_t	*node;
  plane_t	*plane;
  float		t1, t2;
  float		frac;
  int	       	i;
  vec3       	mid;
  int			side;
  float		midf;
  
  
  vec3 vec3_origin;
  vec3_origin[0] = 0.0;vec3_origin[1] = 0.0;vec3_origin[2] = 0.0;
  if (num < 0)
    {
      if (num != CONTENTS_SOLID)
	{
	  trace->allsolid = 0;
	  if (num == CONTENTS_EMPTY)
	    trace->inopen = 1;
	  else
	    trace->inwater = 1;
	}
      else
	trace->startsolid = 1;
      return 1;		// empty
    }

  if (num < hull->firstclipnode || num > hull->lastclipnode)
    printf("PM_RecursiveHullCheck: bad node number");

  //
  // find the point distances
  //
  node = hull->clipnodes + num;
  plane = hull->planes + node->planenum;
  vec3 plane_normal;
  plane_normal[0] = plane->normal.x;plane_normal[1] = plane->normal.y;plane_normal[2] = plane->normal.z;
  float offset =
    (plane_normal[0] >= 0 ? hull->clip_maxs[0] : hull->clip_mins[0]) * plane_normal[0] +
    (plane_normal[1] >= 0 ? hull->clip_maxs[1] : hull->clip_mins[1]) * plane_normal[1] +
    (plane_normal[2] >= 0 ? hull->clip_maxs[2] : hull->clip_mins[2]) * plane_normal[2];
  float dot = fabsf(plane_normal[1]);
  float dist = plane->dist;
  //if(dot > 0.9848f){
  //dist -= offset;
  //}
  

  if (plane->type < 3)
    {
      float offset2;
      if (plane_normal[plane->type] < 0)
        offset2 = hull->clip_maxs[plane->type];
      else
        offset2 = hull->clip_mins[plane->type];

      float dist2 = plane->dist - offset2;
      t1 = p1[plane->type] - dist;
      t2 = p2[plane->type] - dist;
    }
  else
    {
      t1 = glm_vec_dot(plane_normal, p1) - (dist);
      t2 = glm_vec_dot(plane_normal, p2) - (dist);
      }
  
  
  if (t1 >=0 && t2 >= 0)
    return PM_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
  if (t1 < 0 && t2 < 0)
    return PM_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
  
  
   // put the crosspoint DIST_EPSILON pixels on the near side
   if (t1 < 0)
     frac = (t1 + DIST_EPSILON)/(t1-t2);
   else
     frac = (t1 - DIST_EPSILON)/(t1-t2);
   if (frac < 0)
     frac = 0;
   if (frac > 1)
     frac = 1;
		
   midf = p1f + (p2f - p1f)*frac;
   for (i=0 ; i<3 ; i++)
     mid[i] = p1[i] + frac*(p2[i] - p1[i]);

   side = (t1 < 0);

   // move up to the node
   //if (!PM_RecursiveHullCheck (hull, side ? node->back : node->front, p1f, midf, p1, mid, trace))
   //return 0;
   if(!PM_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace))
     return 0;

   if (PM_HullPointContents (hull, node->children[side^1], mid)
       != CONTENTS_SOLID)
     // go past the node
     return PM_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
   //TODO: ifdef PARANOID here ,wtf is that btw??

   /*int side_xored = side^1;
   if (PM_HullPointContents (hull, side ? node->front : node->back, mid) != CONTENTS_SOLID)
     {
       return PM_RecursiveHullCheck (hull, side ? node->front : node->back, midf, p2f, mid, p2, trace);
     }
   */

   if (trace->allsolid)
     return 0;		// never got out of the solid area

   //==================
   // the other side of the node is solid, this is the impact point
   //==================

   if (!side)
     {
       //VectorCopy (plane->normal, trace->plane.normal);
       trace->plane.normal.x = plane->normal.x;trace->plane.normal.y = plane->normal.y;trace->plane.normal.z = plane->normal.z;
       trace->plane.dist = plane->dist;
     }
   else
     {
       trace->plane.normal.x = vec3_origin[0] - plane->normal.x;
       trace->plane.normal.y = vec3_origin[1] - plane->normal.y;
       trace->plane.normal.z = vec3_origin[2] - plane->normal.z;
       trace->plane.dist = -plane->dist;
     }

   while (PM_HullPointContents (hull, hull->firstclipnode, mid)
	  == CONTENTS_SOLID)
     { // shouldn't really happen, but does occasionally
       frac -= 0.1;
       if (frac < 0)
	 {
	   trace->fraction = midf;
	   glm_vec_copy(mid, trace->endpos);
	   printf("backup past 0\n");
	   return 0;
	 }
       midf = p1f + (p2f - p1f)*frac;
       for (i=0 ; i<3 ; i++)
	 mid[i] = p1[i] + frac*(p2[i] - p1[i]);
     }
   trace->fraction = midf;
   glm_vec_copy(mid, trace->endpos);

   return 0;
  
}


int PM_RecursiveHullCheckRay (hull_t *hull, int num, float p1f, float p2f, vec3 p1, vec3 p2, pmtrace_t *trace){
  clipnode_t	*node;
  plane_t	*plane;
  float		t1, t2;
  float		frac;
  int	       	i;
  vec3       	mid;
  int			side;
  float		midf;
  
  
  vec3 vec3_origin;
  vec3_origin[0] = 0.0;vec3_origin[1] = 0.0;vec3_origin[2] = 0.0;
  if (num < 0)
    {
      if (num != CONTENTS_SOLID)
	{
	  trace->allsolid = 0;
	  if (num == CONTENTS_EMPTY)
	    trace->inopen = 1;
	  else
	    trace->inwater = 1;
	}
      else
	trace->startsolid = 1;
      return 1;		// empty
    }

  if (num < hull->firstclipnode || num > hull->lastclipnode)
    printf("PM_RecursiveHullCheck: bad node number");

  //
  // find the point distances
  //
  node = hull->clipnodes + num;
  plane = hull->planes + node->planenum;
  vec3 plane_normal;
  plane_normal[0] = plane->normal.x;plane_normal[1] = plane->normal.y;plane_normal[2] = plane->normal.z;
  
  float dot = fabsf(plane_normal[1]);
  float dist = plane->dist;
  //if(dot > 0.9848f){
  //dist -= offset;
  //}
  

  if (plane->type < 3)
    {
      t1 = p1[plane->type] - dist;
      t2 = p2[plane->type] - dist;
    }
  else
    {
      t1 = glm_vec_dot(plane_normal, p1) - (dist);
      t2 = glm_vec_dot(plane_normal, p2) - (dist);
      }
  
  
  if (t1 >=0 && t2 >= 0)
    return PM_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
  if (t1 < 0 && t2 < 0)
    return PM_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
  
  
   // put the crosspoint DIST_EPSILON pixels on the near side
   if (t1 < 0)
     frac = (t1 + DIST_EPSILON)/(t1-t2);
   else
     frac = (t1 - DIST_EPSILON)/(t1-t2);
   if (frac < 0)
     frac = 0;
   if (frac > 1)
     frac = 1;
		
   midf = p1f + (p2f - p1f)*frac;
   for (i=0 ; i<3 ; i++)
     mid[i] = p1[i] + frac*(p2[i] - p1[i]);

   side = (t1 < 0);

   // move up to the node
   //if (!PM_RecursiveHullCheck (hull, side ? node->back : node->front, p1f, midf, p1, mid, trace))
   //return 0;
   if(!PM_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace))
     return 0;

   if (PM_HullPointContents (hull, node->children[side^1], mid)
       != CONTENTS_SOLID)
     // go past the node
     return PM_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);
   //TODO: ifdef PARANOID here ,wtf is that btw??

   /*int side_xored = side^1;
   if (PM_HullPointContents (hull, side ? node->front : node->back, mid) != CONTENTS_SOLID)
     {
       return PM_RecursiveHullCheck (hull, side ? node->front : node->back, midf, p2f, mid, p2, trace);
     }
   */

   if (trace->allsolid)
     return 0;		// never got out of the solid area

   //==================
   // the other side of the node is solid, this is the impact point
   //==================

   if (!side)
     {
       //VectorCopy (plane->normal, trace->plane.normal);
       trace->plane.normal.x = plane->normal.x;trace->plane.normal.y = plane->normal.y;trace->plane.normal.z = plane->normal.z;
       trace->plane.dist = plane->dist;
     }
   else
     {
       trace->plane.normal.x = vec3_origin[0] - plane->normal.x;
       trace->plane.normal.y = vec3_origin[1] - plane->normal.y;
       trace->plane.normal.z = vec3_origin[2] - plane->normal.z;
       trace->plane.dist = -plane->dist;
     }

   while (PM_HullPointContents (hull, hull->firstclipnode, mid)
	  == CONTENTS_SOLID)
     { // shouldn't really happen, but does occasionally
       frac -= 0.1;
       if (frac < 0)
	 {
	   trace->fraction = midf;
	   glm_vec_copy(mid, trace->endpos);
	   printf("backup past 0\n");
	   return 0;
	 }
       midf = p1f + (p2f - p1f)*frac;
       for (i=0 ; i<3 ; i++)
	 mid[i] = p1[i] + frac*(p2[i] - p1[i]);
     }
   trace->fraction = midf;
   glm_vec_copy(mid, trace->endpos);

   return 0;
  
}
