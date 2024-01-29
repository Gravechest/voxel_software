#pragma once

#include "source.h"

#define RD_MASK_SIZE 4
#define RD_MASK_X (window_size.x / RD_MASK_SIZE)
#define RD_MASK_Y (window_size.y / RD_MASK_SIZE)
#define RD_LOD 0.1f

extern char* occlusion_mask;
extern ivec2* occlusion_scanline;

void sceneGatherTriangles(uint32_t node_ptr);