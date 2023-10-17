#pragma once

#include <stdbool.h> 

#include "vec3.h"
#include "source.h"
#include "dda.h"

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
node_hit_t traverseTree(ray3_t ray,uint node_ptr);
node_hit_t traverseTreeI(ray3i_t ray,uint node_ptr);
void setVoxel(uint x,uint y,uint z,uint depth,uint material);
void removeVoxel(uint node_ptr);
block_t* insideBlock(vec3 pos);
uint getNode(int x,int y,int z,int depth);
uint getNodeFromPos(vec3 pos,uint depth);
beam_result_t traverseTreeBeamI(traverse_init_t init[4],uint x,uint y,uint beam_size);
ray3i_t ray3CreateI(vec3 pos,vec3 dir);
