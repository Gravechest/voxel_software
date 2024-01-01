#pragma once

#include "source.h"
#include "dynamic_array.h"

#define GRID_DISCONNECTED 0xffff

typedef struct{
	float throughput;
	dynamic_array_t component;
}power_grid_t;

void removePowerGrid(uint32_t node_ptr);
void applyPowerGrid(uint32_t base_ptr);
void powerGridTick();

extern dynamic_array_t power_grid_array;