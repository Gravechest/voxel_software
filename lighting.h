#pragma once

#define LM_SIZE 16

#define LUMINANCE_SUN VEC3_ZERO
#define LUMINANCE_SKY (vec3_t){1.0f,1.0f,1.0f}

#define GETLIGHTMAPSIZE(DEPTH) (1 << (LM_MAXDEPTH - (int)(DEPTH) < 0 ? 0 : LM_MAXDEPTH - (int)(DEPTH)))

#include "vec3.h"
#include "tree.h"

void lighting();

void setLightMap(vec3_t* color,vec3_t pos,int side,uint32_t quality);
void setLightSideBigSingle(vec3_t* color,vec3_t pos,uint32_t side,uint32_t depth,uint32_t x,uint32_t y,uint32_t quality);
void setLightSideBig(vec3_t* color,vec3_t pos,int side,int depth,uint32_t quality);
void setLightSide(vec3_t* color,vec3_t pos,int side,int depth);
void setLightSideBig(vec3_t* color,vec3_t pos,int side,int depth,uint32_t quality);

void lightNewStackPut(uint32_t node_index,block_t* block,uint32_t side);

vec3_t sideGetLuminance(vec3_t ray_pos,vec3_t angle,node_hit_t result,uint32_t depth);
vec3_t rayGetLuminance(vec3_t ray_pos,vec3_t init_pos,vec3_t angle,uint32_t init_node,uint32_t depth);
vec3_t rayGetLuminanceDir(vec3_t init_pos,vec2_t direction,uint32_t init_node,vec3_t ray_pos);