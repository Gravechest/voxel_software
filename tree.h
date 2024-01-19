#pragma once

#include <stdbool.h> 

#include "dynamic_array.h"
#include "vec3.h"
#include "source.h"
#include "dda.h"
#include "fog.h"

#define FIXED_PRECISION 16

#define TREE_RAY_LIQUID (1 << 0)

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

	uint32_t square_side;
}ray3i_t;

typedef struct{
	vec3_t pos;
	uint32_t node;
}traverse_init_t;

extern dynamic_array_t node_root;
extern dynamic_array_t block_array;
extern dynamic_array_t air_array;

float getBlockSize(int depth);
traverse_init_t initTraverse(vec3_t pos);
node_hit_t treeRayFlags(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos,int flags);
node_hit_t treeRay(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos);
node_hit_t treeRayI(ray3i_t ray,uint32_t node_ptr);
uint32_t traverseTreeItt(ray3i_t ray,uint32_t node_ptr);
void setVoxel(uint32_t x,uint32_t y,uint32_t z,int depth,uint32_t material,float ammount);
void setVoxelSolid(uint32_t x,uint32_t y,uint32_t z,uint32_t depth,uint32_t material);
void removeVoxel(uint32_t node_ptr);
block_t* insideBlock(vec3_t pos);
uint32_t getNode(int x,int y,int z,int depth);
uint32_t getNodeFromPos(vec3_t pos,uint32_t depth);
ray3i_t ray3CreateI(vec3_t pos,vec3_t dir);
float rayNodeGetDistance(vec3_t ray_pos,ivec3 pos,int depth,vec3_t angle,int side);
float rayGetDistance(vec3_t ray_pos,vec3_t ray_dir);
node_t treeTraverse(vec3_t pos);
fog_t treeRayFog(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos,float max_distance);
node_hit_t treeRayOnce(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos);
ivec3 getGridPosFromPos(vec3_t pos,uint32_t depth);
