#pragma once

#define LM_SIZE 16

#include "vec3.h"

void setLightSideBig(vec3* color,vec3 pos,int side,int depth);
void setLightSide(vec3* color,vec3 pos,int side,int depth);
vec3 lightmapUpX(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size);
vec3 lightmapDownX(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size);
vec3 lightmapUpY(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size);
vec3 lightmapDownY(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size);