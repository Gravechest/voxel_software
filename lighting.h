#pragma once

#define LM_SIZE 16

#define LUMINANCE_SUN VEC3_ZERO
#define LUMINANCE_SKY (vec3){1.0f,1.0f,1.0f}

#define GETLIGHTMAPSIZE(DEPTH) (1 << (LM_MAXDEPTH - (int)(DEPTH) < 0 ? 0 : LM_MAXDEPTH - (int)(DEPTH)))

#include "vec3.h"

void lighting();

void updateLightMapArea(unsigned int node_ptr,vec3 pos,float radius,int depth);
void updateEdgeLightMapArea(uint node_ptr,vec3 pos,float radius,int depth);

void setLightMap(vec3* color,vec3 pos,int side,int depth,uint quality);
void setLightSideBigSingle(vec3* color,vec3 pos,uint side,uint depth,uint x,uint y,uint quality);
void setLightSideBig(vec3* color,vec3 pos,int side,int depth,uint quality);
void setLightSide(vec3* color,vec3 pos,int side,int depth);
void setLightSideBig(vec3* color,vec3 pos,int side,int depth,uint quality);
void setEdgeLightSide(block_node_t node,uint side);

vec3 lightmapUpX(block_node_t node,int side,int lightmap_x,int lightmap_y,int size);
vec3 lightmapDownX(block_node_t node,int side,int lightmap_x,int lightmap_y,int size);
vec3 lightmapUpY(block_node_t node,int side,int lightmap_x,int lightmap_y,int size);
vec3 lightmapDownY(block_node_t node,int side,int lightmap_x,int lightmap_y,int size);

vec3 rayGetLuminance(vec3 ray_pos,vec3 init_pos,vec3 angle,uint init_node,uint depth);
vec3 rayGetLuminanceDir(vec3 init_pos,vec2 direction,uint init_node,vec3 ray_pos);

void calculateDynamicLight(vec3 pos,uint node_ptr,float radius,vec3 color);