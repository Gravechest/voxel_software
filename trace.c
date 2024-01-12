#include <intrin.h>

#include "source.h"
#include "tree.h"
#include "trace.h"
#include "lighting.h"
#include "tmath.h"

static bool isInBlockSide(vec3_t origin,vec3_t ray_direction,float distance,plane_ray_t plane,float block_size){
	vec3_t hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
	vec2_t uv = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	if(uv.x < 0.0f || uv.y < 0.0f || uv.x > block_size || uv.y > block_size)
		return false;
	return true;
}

typedef struct{
	int node;
	plane_ray_t plane;
	uint32_t side;
}node_plane_t;