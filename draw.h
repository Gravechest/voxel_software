#pragma once

#include "source.h"
#include "tree.h"

void drawPixelGroup(int x,int y,traverse_init_t init,vec3 camera_pos);
void tracePixel(traverse_init_t init,vec3 dir,int x,int y,vec3 camera_pos);