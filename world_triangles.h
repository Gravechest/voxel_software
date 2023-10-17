#pragma once

#include "source.h"

#define RD_MASK_SIZE 8
#define RD_MASK_X (WND_RESOLUTION_X / RD_MASK_SIZE)
#define RD_MASK_Y (WND_RESOLUTION_Y / RD_MASK_SIZE)
#define RD_LOD 0.1f

extern char mask[RD_MASK_X][RD_MASK_Y];

void sceneGatherTriangles(uint node_ptr);