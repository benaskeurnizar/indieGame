#ifndef LIGHTING_H
#define LIGHTING_H


typedef struct tnode_s
{
  int		type;
  vec3	normal;
  float	dist;
  int		children[2];
  int		pad;
} tnode_t;

typedef struct
{
  vec3	backpt;
  int		side;
  int		node;
} tracestack_t;

#endif
