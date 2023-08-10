#include "dda.h"

RAY3 ray3Create(VEC3 pos,VEC3 dir){
	RAY3 ray;
	ray.pos = pos;
	ray.dir = dir;
	ray.delta = VEC3absR(VEC3divFR(ray.dir,1.0f));

	ray.step.x = ray.dir.x < 0.0f ? -1 : 1;
	ray.side.x = ray.dir.x < 0.0f ? (ray.pos.x-(int)ray.pos.x) * ray.delta.x : ((int)ray.pos.x + 1.0f - ray.pos.x) * ray.delta.x;

	ray.step.y = ray.dir.y < 0.0f ? -1 : 1;
	ray.side.y = ray.dir.y < 0.0f ? (ray.pos.y-(int)ray.pos.y) * ray.delta.y : ((int)ray.pos.y + 1.0f - ray.pos.y) * ray.delta.y;

	ray.step.z = ray.dir.z < 0.0f ? -1 : 1;
	ray.side.z = ray.dir.z < 0.0f ? (ray.pos.z-(int)ray.pos.z) * ray.delta.z : ((int)ray.pos.z + 1.0f - ray.pos.z) * ray.delta.z;

	ray.square_pos.x = ray.pos.x;
	ray.square_pos.y = ray.pos.y;
	ray.square_pos.z = ray.pos.z;
	return ray;
}

RAY2 ray2Create(VEC2 pos,VEC2 dir){
	RAY2 ray;
	ray.pos = pos;
	ray.dir = dir;
	ray.delta = VEC2absR(VEC2divFR(ray.dir,1.0f));

	ray.step.x = ray.dir.x < 0.0f ? -1 : 1;
	ray.side.x = ray.dir.x < 0.0f ? (ray.pos.x-(int)ray.pos.x) * ray.delta.x : ((int)ray.pos.x + 1.0f - ray.pos.x) * ray.delta.x;

	ray.step.y = ray.dir.y < 0.0f ? -1 : 1;
	ray.side.y = ray.dir.y < 0.0f ? (ray.pos.y-(int)ray.pos.y) * ray.delta.y : ((int)ray.pos.y + 1.0f - ray.pos.y) * ray.delta.y;

	ray.square_pos.x = ray.pos.x;
	ray.square_pos.y = ray.pos.y;
	return ray;
}

void ray2Itterate(RAY2 *ray){
	if(ray->side.x < ray->side.y){
		ray->square_pos.x += ray->step.x;
		ray->side.x += ray->delta.x;
		return;
	}
	ray->square_pos.y += ray->step.y;
	ray->side.y += ray->delta.y;
}

void ray3Itterate(RAY3* ray){
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

VEC2 ray3UV(RAY3 ray){
	VEC2 wall;
	switch(ray.square_side){
	case VEC3_X:
		wall.x = ray.pos.y + (ray.side.x - ray.delta.x) * ray.dir.y;
		wall.y = ray.pos.z + (ray.side.x - ray.delta.x) * ray.dir.z;
		return (VEC2){wall.x - (int)wall.x,wall.y - (int)wall.y};
	case VEC3_Y:
		wall.x = ray.pos.x + (ray.side.y - ray.delta.y) * ray.dir.x;
		wall.y = ray.pos.z + (ray.side.y - ray.delta.y) * ray.dir.z;
		return (VEC2){wall.x - (int)wall.x,wall.y - (int)wall.y};
	case VEC3_Z:
		wall.x = ray.pos.x + (ray.side.z - ray.delta.z) * ray.dir.x;
		wall.y = ray.pos.y + (ray.side.z - ray.delta.z) * ray.dir.y;
		return (VEC2){wall.x - (int)wall.x,wall.y - (int)wall.y};
	}
}