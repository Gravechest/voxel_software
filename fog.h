#pragma once

#include "source.h"

#define FOG_DENSITY 0.03f

typedef struct{
	vec3 luminance;
	float density;
}fog_t;

fog_t calculateFogColor(vec3 pos);
void calculateNodeLuminance(node_t* node,uint quality);
void calculateNodeLuminanceTree(uint node_ptr,uint quality);