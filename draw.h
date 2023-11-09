#pragma once

#include "source.h"
#include "tree.h"

#define MIPMAP_AGGRESION 4.0f
#define LM_DYNAMIC_UPDATE 1
#define LUMINANCE_SINGLE_THRESHOLD 0.08f

typedef struct{
	pixel_t color;
	block_t* block;
	uint side;
}pixel_cache_t;

extern pixel_cache_t pixel_cache[4];

void drawSurfacePixel(block_node_t node,vec3 ray_direction,vec3 camera_pos,int axis,int x,int y);
void lightNewStackPut(block_node_t* node,block_t* block,uint side);
void tracePixel(traverse_init_t init,vec3 dir,int x,int y,vec3 camera_pos);
void drawSky(int x,int y,int pixelblock_size,vec3 ray_direction);
void drawSky16(int x,int y,vec3 ray_direction);
