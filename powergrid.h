#pragma once

#include "source.h"

typedef struct{
	uint block;
}grid_component_t;

typedef struct{
	float throughput;
	uint ptr;
	grid_component_t* component;
}power_grid_t;

void removePowerGrid(uint node_ptr);
void applyPowerGrid(uint base_ptr);
grid_component_t* getPowerGridComponent(uint power_grid,uint node_ptr);
void powerGridTick();

extern power_grid_t power_grid_array[12];
extern uint power_grid_ptr;