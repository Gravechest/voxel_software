#include "dda.h"
#include "tmath.h"
#include "source.h"

ray3_t ray3Create(vec3_t pos,vec3_t dir){
	ray3_t ray;

	ray.square_side = 0;
	ray.pos = pos;
	ray.dir = dir;
	ray.delta = vec3absR(vec3divFR(ray.dir,1.0f));

	ray.step.x = ray.dir.x < 0.0f ? -1 : 1;
	ray.step.y = ray.dir.y < 0.0f ? -1 : 1;
	ray.step.z = ray.dir.z < 0.0f ? -1 : 1;

	vec3_t fract_pos;
	fract_pos.x = tFractUnsigned(ray.pos.x);
	fract_pos.y = tFractUnsigned(ray.pos.y);
	fract_pos.z = tFractUnsigned(ray.pos.z);

	ray.side.x = (ray.dir.x < 0.0f ? fract_pos.x : 1.0f - fract_pos.x) * ray.delta.x;
	ray.side.y = (ray.dir.y < 0.0f ? fract_pos.y : 1.0f - fract_pos.y) * ray.delta.y;
	ray.side.z = (ray.dir.z < 0.0f ? fract_pos.z : 1.0f - fract_pos.z) * ray.delta.z;

	ray.square_pos.x = ray.pos.x;
	ray.square_pos.y = ray.pos.y;
	ray.square_pos.z = ray.pos.z;
	return ray;
}

ray2_t ray2Create(vec2_t pos,vec2_t dir){
	ray2_t ray;
	ray.pos = pos;
	ray.dir = dir;
	ray.delta = vec2absR(vec2divFR(ray.dir,1.0f));

	ray.step.x = ray.dir.x < 0.0f ? -1 : 1;
	ray.side.x = ray.dir.x < 0.0f ? (ray.pos.x-(int)ray.pos.x) * ray.delta.x : ((int)ray.pos.x + 1.0f - ray.pos.x) * ray.delta.x;

	ray.step.y = ray.dir.y < 0.0f ? -1 : 1;
	ray.side.y = ray.dir.y < 0.0f ? (ray.pos.y-(int)ray.pos.y) * ray.delta.y : ((int)ray.pos.y + 1.0f - ray.pos.y) * ray.delta.y;

	ray.square_pos.x = ray.pos.x;
	ray.square_pos.y = ray.pos.y;
	return ray;
}

void ray2Itterate(ray2_t *ray){
	if(ray->side.x < ray->side.y){
		ray->square_pos.x += ray->step.x;
		ray->side.x += ray->delta.x;
		return;
	}
	ray->square_pos.y += ray->step.y;
	ray->side.y += ray->delta.y;
}

void ray3Itterate(ray3_t* ray){
	if(ray->side.x < ray->side.y){
		if(ray->side.x < ray->side.z){
			ray->square_pos.x += ray->step.x;
			ray->side.x += ray->delta.x;
			ray->square_side = VEC3_X;
			return;
		}
		ray->square_pos.z += ray->step.z;
		ray->side.z += ray->delta.z;
		ray->square_side = VEC3_Z;
		return;
	}
	if(ray->side.y < ray->side.z){
		ray->square_pos.y += ray->step.y;
		ray->side.y += ray->delta.y;
		ray->square_side = VEC3_Y;
		return;
	}
	ray->square_pos.z += ray->step.z;
	ray->side.z += ray->delta.z;
	ray->square_side = VEC3_Z;
}

vec2_t ray3UV(ray3_t ray){
	vec2_t wall;
	switch(ray.square_side){
	case VEC3_X:
		wall.x = ray.pos.y + (ray.side.x - ray.delta.x) * ray.dir.y;
		wall.y = ray.pos.z + (ray.side.x - ray.delta.x) * ray.dir.z;
		return (vec2_t){wall.x - (int)wall.x,wall.y - (int)wall.y};
	case VEC3_Y:
		wall.x = ray.pos.x + (ray.side.y - ray.delta.y) * ray.dir.x;
		wall.y = ray.pos.z + (ray.side.y - ray.delta.y) * ray.dir.z;
		return (vec2_t){wall.x - (int)wall.x,wall.y - (int)wall.y};
	case VEC3_Z:
		wall.x = ray.pos.x + (ray.side.z - ray.delta.z) * ray.dir.x;
		wall.y = ray.pos.y + (ray.side.z - ray.delta.z) * ray.dir.y;
		return (vec2_t){wall.x - (int)wall.x,wall.y - (int)wall.y};
	}
}