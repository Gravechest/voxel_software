#include <intrin.h>

#include "source.h"
#include "tree.h"
#include "trace.h"
#include "draw.h"
#include "lighting.h"
#include "tmath.h"

static bool isInBlockSide(vec3 origin,vec3 ray_direction,float distance,plane_t plane,float block_size){
	vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
	vec2 uv = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	if(uv.x < 0.0f || uv.y < 0.0f || uv.x > block_size || uv.y > block_size)
		return false;
	return true;
}

typedef struct{
	int node;
	plane_t plane;
	uint side;
}node_plane_t;