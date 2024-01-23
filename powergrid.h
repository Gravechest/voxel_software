#pragma once

#include "source.h"
#include "dynamic_array.h"

#define GRID_TOGGLE 0xfffe
#define GRID_DISCONNECTED 0xffff

typedef struct{
	int demand;
	int suppliers;
	float supply;
	float used_supply;
	float throughput;
	dynamic_array_t component;
}power_grid_t;

void powerGridRemove(uint32_t node_ptr);
void powerGridApply(uint32_t base_ptr);
void powerGridTick();

extern dynamic_array_t power_grid_array;