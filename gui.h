#pragma once

#include "source.h"

void guiInventoryContent(inventoryslot_t* slot,vec2_t pos,vec2_t size);
void guiRectangle(vec2_t pos,vec2_t size,vec3_t color);
void guiFrame(vec2_t pos,vec2_t size,vec3_t color,float thickness);
void drawString(vec2_t pos,float size,char* str);