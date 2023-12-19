#include "powergrid.h"
#include "memory.h"
#include "tree.h"

uint power_grid_ptr;
power_grid_t power_grid_array[12];

grid_component_t* getPowerGridComponent(uint power_grid,uint node_ptr){
	power_grid_t grid = power_grid_array[power_grid];
	for(int i = 0;i < grid.ptr;i++){
		if(grid.component[i].block == node_ptr)
			return &grid.component[i];
	}
}

void removePowerGridElement(uint power_grid,uint element){
	power_grid_t* grid = &power_grid_array[power_grid];
	int i = 0;
	while(grid->component[i].block != element)
		i++;
	for(;i < grid->ptr;i++)
		grid->component[i] = grid->component[i + 1];
	grid->ptr--;
	if(!grid->ptr){
		MFREE(grid->component);
		grid->component = 0;
		for(int i = power_grid;i < power_grid_ptr;i++){
			for(int j = 0;j < power_grid_array[i + 1].ptr;j++)
				node_root[power_grid_array[i + 1].component[j].block].block->power_grid[0]--;
			power_grid_array[i] = power_grid_array[i + 1];
		}
		power_grid_ptr--;
	}
}

bool applyPowerGridSub(uint base_ptr,uint node_ptr,uint* neighbour){
	node_t node = node_root[node_ptr];
	if(node.air)
		return false;
	if(node.block){
		node_t base = node_root[base_ptr];
		block_t* block = base.block;
		if(block->material == BLOCK_WIRE){
			if(node.block->material == BLOCK_WIRE && node.depth != base.depth)
				return false;
			if(material_array[node.block->material].flags & MAT_POWER){
				block->power_grid[0] = node.block->power_grid[0];
				if(block->power_grid[0] == 0xffff)
					return true;
				float throughput = 1024.0f / ((1 << base.depth) * (1 << base.depth));
				power_grid_array[block->power_grid[0]].throughput = throughput;
				return true;
			}
			return false;
		}
		if(node.block->material == BLOCK_WIRE){
			block->power_grid[0] = node.block->power_grid[0];
			power_grid_t* grid = &power_grid_array[block->power_grid[0]];
			if(!grid->component)
				grid->component = MALLOC(sizeof(grid_component_t) * 12);
			grid->component[grid->ptr++].block = base_ptr;
			return true;
		}
		return false;
	}
	for(int i = 0;i < 4;i++)
		applyPowerGridSub(base_ptr,node.child_s[neighbour[i]],neighbour);
	return false;
}

void floodFillPowerGrid(uint base_ptr);

void changePowerGrid(uint power_grid_id,uint base_depth,uint node_ptr,uint* neighbour){
	node_t node = node_root[node_ptr];
	if(node.air)
		return;
	if(node.block){
		block_t* block = node.block;
		if(material_array[block->material].flags & MAT_POWER){
			if(block->power_grid[0] != power_grid_id){
				if(block->material == BLOCK_WIRE){
					if(node.depth != base_depth)
						return;
					block->power_grid[0] = power_grid_id;
					floodFillPowerGrid(node_ptr);
					return;
				}
				removePowerGridElement(block->power_grid[0],node_ptr);
				power_grid_t* grid = &power_grid_array[power_grid_id];
				if(!grid->component)
					grid->component = MALLOC(sizeof(grid_component_t) * 12);
				grid->component[grid->ptr++].block = node_ptr;
				block->power_grid[0] = power_grid_id;
			}
		}
		return;
	}
	for(int i = 0;i < 4;i++)
		changePowerGrid(power_grid_id,base_depth,node.child_s[neighbour[i]],neighbour);
}

void floodFillPowerGrid(uint base_ptr){
	node_t node = node_root[base_ptr];
	for(int i = 0;i < 6;i++){
		int dx = i >> 1 == VEC3_X ? i & 1 ? -1 : 1 : 0;
		int dy = i >> 1 == VEC3_Y ? i & 1 ? -1 : 1 : 0;
		int dz = i >> 1 == VEC3_Z ? i & 1 ? -1 : 1 : 0;
		int x = node.pos.x + dx;
		int y = node.pos.y + dy;
		int z = node.pos.z + dz;
		changePowerGrid(node.block->power_grid[0],node.depth,getNode(x,y,z,node.depth),border_block_table[i]);
	}
}

void removePowerGridSub(uint node_ptr,uint* neighbour,bool* grid_new){
	node_t node = node_root[node_ptr];
	if(node.air)
		return;
	if(node.block){
		if(!(material_array[node.block->material].flags & MAT_POWER))
			return;
		if(*grid_new){
			node_root[node_ptr].block->power_grid[0] = power_grid_ptr++;
			if(node.block->material == BLOCK_WIRE)
				floodFillPowerGrid(node_ptr);
		}
		*grid_new = true;
		return;
	}
	for(int i = 0;i < 4;i++)
		removePowerGridSub(node.child_s[neighbour[i]],neighbour,grid_new);
}

void removePowerGrid(uint node_ptr){
	block_t* buffer = node_root[node_ptr].block;
	node_t node = node_root[node_ptr];
	node_root[node_ptr].block = 0;
	node_root[node_ptr].air = 1;
	bool grid_new = false;
	for(int i = 0;i < 6;i++){
		int dx = i >> 1 == VEC3_X ? i & 1 ? -1 : 1 : 0;
		int dy = i >> 1 == VEC3_Y ? i & 1 ? -1 : 1 : 0;
		int dz = i >> 1 == VEC3_Z ? i & 1 ? -1 : 1 : 0;
		int x = node.pos.x + dx;
		int y = node.pos.y + dy;
		int z = node.pos.z + dz;
		removePowerGridSub(getNode(x,y,z,node.depth),border_block_table[i],&grid_new);
	}
	node_root[node_ptr].block = buffer;
	node_root[node_ptr].air = 0;
	if(node.block->material == BLOCK_WIRE)
		return;
	removePowerGridElement(node.block->power_grid[0],node_ptr);
}

void applyPowerGrid(uint base_ptr){
	node_t node = node_root[base_ptr];
	block_t* block = node_root[base_ptr].block;
	block->power_grid[0] = -1;
	bool applied_buffer = false;
	for(int i = 0;i < 6;i++){
		int dx = i >> 1 == VEC3_X ? i & 1 ? -1 : 1 : 0;
		int dy = i >> 1 == VEC3_Y ? i & 1 ? -1 : 1 : 0;
		int dz = i >> 1 == VEC3_Z ? i & 1 ? -1 : 1 : 0;
		int x = node.pos.x + dx;
		int y = node.pos.y + dy;
		int z = node.pos.z + dz;
		bool applied = applyPowerGridSub(base_ptr,getNode(x,y,z,node.depth),border_block_table[i]);
		if(block->material == BLOCK_WIRE && applied && applied_buffer){
			floodFillPowerGrid(base_ptr);
		}
		applied_buffer |= applied;
	}
	if(applied_buffer || block->material == BLOCK_WIRE)
		return;
	block->power_grid[0] = power_grid_ptr++;
	power_grid_t* grid = &power_grid_array[block->power_grid[0]];
	grid->component = MALLOC(sizeof(grid_component_t) * 12);
	grid->component[grid->ptr++].block = base_ptr;
}
#include <stdio.h>
void powerGridTick(){
	for(int i = 0;i < power_grid_ptr;i++){
		float supply = 0.0f;
		power_grid_t grid = power_grid_array[i];
		for(int j = 0;j < grid.ptr;j++){
			node_t node = node_root[grid.component[j].block];
			if(node.block->material == BLOCK_BATTERY)
				supply += 1.0f;
			node.block->power = 0.0f;
		}
		while(supply > 0.0f){
			int demand = 0;
			for(int j = 0;j < grid.ptr;j++){
				node_t node = node_root[grid.component[j].block];
				if(node.block->material == BLOCK_LAMP)
					demand++;
			}
			if(!demand)
				break;
			float supply_single = supply / demand;
			for(int j = 0;j < grid.ptr;j++){
				node_t node = node_root[grid.component[j].block];
				if(node.block->material == BLOCK_LAMP){
					node.block->power = supply_single;
					supply -= supply_single;
				}
			}
			supply -= 0.001f;
		}
	}
}