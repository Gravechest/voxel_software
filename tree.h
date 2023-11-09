#pragma once

#include <stdbool.h> 

#include "vec3.h"
#include "source.h"
#include "dda.h"
#include "fog.h"

#define FIXED_PRECISION 16

typedef struct{
	int node;
	int side;
}node_hit_t;

typedef struct{
	uvec3 pos;
	ivec3 dir;
	ivec3 delta;
	ivec3 side;

	ivec3 step;
	uvec3 square_pos;

	uint square_side;
}ray3i_t;

typedef struct{
	int side;
	union{
		int node;
		struct{
			int node_a[4];
			int side_a[4];
		};
	};
	plane_t plane[4];
}beam_result_t;

typedef struct{
	vec3 pos;
	uint node;
}traverse_init_t;

extern block_node_t* node_root;
extern uint block_node_c;

traverse_init_t initTraverse(vec3 pos);
node_hit_t treeRay(ray3_t ray,uint node_ptr,vec3 ray_pos);
node_hit_t treeRayI(ray3i_t ray,uint node_ptr);
uint traverseTreeItt(ray3i_t ray,uint node_ptr);
void setVoxel(uint x,uint y,uint z,uint depth,uint material,float ammount);
void setVoxelSolid(uint x,uint y,uint z,uint depth,uint material);
void removeVoxel(uint node_ptr);
block_t* insideBlock(vec3 pos);
uint getNode(int x,int y,int z,int depth);
uint getNodeFromPos(vec3 pos,uint depth);
ray3i_t ray3CreateI(vec3 pos,vec3 dir);
float rayNodeGetDistance(vec3 ray_pos,ivec3 pos,int depth,vec3 angle,int side);
float rayGetDistance(vec3 ray_pos,vec3 ray_dir);
block_node_t treeTraverse(vec3 pos);
fog_t treeRayFog(ray3_t ray,uint node_ptr,vec3 ray_pos,float max_distance);
node_hit_t treeRayOnce(ray3_t ray,uint node_ptr,vec3 ray_pos);
