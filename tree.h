#pragma once

#include <stdbool.h> 

#include "vec3.h"
#include "source.h"
#include "dda.h"


typedef struct{
	block_node_t* node;
	int side;
}blockray_t;

typedef struct{
	block_node_t** node;
	int side;
}nodepointer_hit_t;

typedef struct{
	vec3 pos;
	block_node_t* node;
}traverse_init_t;

extern block_node_t block_node_root;

traverse_init_t initTraverse(vec3 pos);
blockray_t traverseTree(ray3_t ray,block_node_t* node);
void setVoxel(int x,int y,int z,int depth);
void setVoxelLM(int x,int y,int z,int depth,int texture);
void removeVoxel(block_node_t** node);
block_t* insideBlock(vec3 pos);
nodepointer_hit_t traverseTreeNode(ray3_t ray,block_node_t* node);
block_node_t* getNode(int x,int y,int z,int depth);