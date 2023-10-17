#pragma once

#include "source.h"

#define FOG_DENSITY 0.03f

vec3 calculateFogColor(vec3 pos);
void calculateNodeLuminance(block_node_t* node,uint quality);
void calculateNodeLuminanceTree(uint node_ptr,uint quality);