#pragma once

#include "source.h"

#define FOG_DENSITY 0.03f

typedef struct{
	vec3_t luminance;
	float density;
}fog_t;

fog_t calculateFogColor(vec3_t pos);
void calculateNodeLuminance(node_t* node,uint32_t quality);
void calculateNodeLuminanceTree(uint32_t node_ptr,uint32_t quality);