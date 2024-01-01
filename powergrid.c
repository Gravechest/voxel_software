#include "powergrid.h"
#include "memory.h"
#include "tree.h"

DYNAMIC_ARRAY(power_grid_array,power_grid_t);

void removePowerGridElement(uint32_t power_grid,uint32_t element){
	power_grid_t* grid = dynamicArrayGet(power_grid_array,power_grid);
	dynamicArrayRemove(&grid->component,element);
}

void floodFillPowerGrid(uint32_t base_ptr);

void changePowerGrid(uint32_t power_grid_id,uint32_t base_depth,uint32_t node_ptr,uint32_t* neighbour){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			changePowerGrid(power_grid_id,base_depth,node->child_s[neighbour[i]],neighbour);
		return;
	}
	if(node->type == BLOCK_AIR)
		return;
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(material_array[node->type].flags & MAT_POWER){
		if(block->power_grid[0] != power_grid_id){
			if(node->type == BLOCK_WIRE){
				if(node->depth != base_depth)
					return;
				block->power_grid[0] = power_grid_id;
				floodFillPowerGrid(node_ptr);
				return;
			}
			removePowerGridElement(block->power_grid[0],node_ptr);
			power_grid_t* grid = dynamicArrayGet(power_grid_array,power_grid_id);
			dynamicArrayAdd(&grid->component,&node_ptr);
			block->power_grid[0] = power_grid_id;
		}
	}
}

void floodFillPowerGrid(uint32_t base_ptr){
	node_t* node = dynamicArrayGet(node_root,base_ptr);
	for(int i = 0;i < 6;i++){
		int dx = i >> 1 == VEC3_X ? i & 1 ? -1 : 1 : 0;
		int dy = i >> 1 == VEC3_Y ? i & 1 ? -1 : 1 : 0;
		int dz = i >> 1 == VEC3_Z ? i & 1 ? -1 : 1 : 0;
		int x = node->pos.x + dx;
		int y = node->pos.y + dy;
		int z = node->pos.z + dz;
		block_t* block = dynamicArrayGet(block_array,node->index);
		changePowerGrid(block->power_grid[0],node->depth,getNode(x,y,z,node->depth),border_block_table[i]);
	}
}

bool applyPowerGridSub(uint32_t base_ptr,uint32_t node_ptr,uint32_t* neighbour){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			applyPowerGridSub(base_ptr,node->child_s[neighbour[i]],neighbour);
		return false;
	}
	if(node->type == BLOCK_AIR)
		return false;
	node_t* base = dynamicArrayGet(node_root,base_ptr);
	block_t* base_block = dynamicArrayGet(block_array,base->index);
	block_t* node_block = dynamicArrayGet(block_array,node->index);
	if(base->type == BLOCK_WIRE){
		if(node->type == BLOCK_WIRE && node->depth != base->depth)
			return false;
		if(material_array[node->type].flags & MAT_POWER){
			base_block->power_grid[0] = node_block->power_grid[0];
			if(base_block->power_grid[0] == GRID_DISCONNECTED)
				return true;
			float throughput = 1024.0f / ((1 << base->depth) * (1 << base->depth));
			power_grid_t* grid = dynamicArrayGet(power_grid_array,base_block->power_grid[0]);
			grid->throughput = throughput;
			return true;
		}
		return false;
	}
	if(node->type == BLOCK_WIRE){
		if(node_block->power_grid[0] == GRID_DISCONNECTED){
			power_grid_t grid;
			for(int i = 0;i < sizeof(power_grid_t);i++)
				((char*)&grid)[i] = 0;
			dynamicArrayAdd(&grid.component,&base_ptr);
			grid.throughput = 1024.0f / ((1 << base->depth) * (1 << base->depth));
			base_block->power_grid[0] = dynamicArrayTop(power_grid_array);
			node_block->power_grid[0] = dynamicArrayTop(power_grid_array);
			dynamicArrayAdd(&power_grid_array,&grid);
			floodFillPowerGrid(node_ptr);
			return true;
		}
		base_block->power_grid[0] = node_block->power_grid[0];
		power_grid_t* grid = dynamicArrayGet(power_grid_array,base_block->power_grid[0]);
		dynamicArrayAdd(&grid->component,&base_ptr);
		return true;
	}
	return false;
}

void removePowerGridSub(uint32_t node_ptr,uint32_t* neighbour,bool* grid_new){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			removePowerGridSub(node->child_s[neighbour[i]],neighbour,grid_new);
		return;
	}
	if(node->type == BLOCK_AIR)
		return;
	if(!(material_array[node->type].flags & MAT_POWER))
		return;
	if(*grid_new){
		block_t* block = dynamicArrayGet(block_array,node->index);
		if(node->type == BLOCK_WIRE)
			floodFillPowerGrid(node_ptr);
	}
	*grid_new = true;
}

void removePowerGrid(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	uint32_t buffer = node->index;
	node->type = BLOCK_AIR;
	bool grid_new = false;
	for(int i = 0;i < 6;i++){
		int dx = i >> 1 == VEC3_X ? i & 1 ? -1 : 1 : 0;
		int dy = i >> 1 == VEC3_Y ? i & 1 ? -1 : 1 : 0;
		int dz = i >> 1 == VEC3_Z ? i & 1 ? -1 : 1 : 0;
		int x = node->pos.x + dx;
		int y = node->pos.y + dy;
		int z = node->pos.z + dz;
		removePowerGridSub(getNode(x,y,z,node->depth),border_block_table[i],&grid_new);
	}
	node->type = buffer;
	if(node->type == BLOCK_WIRE)
		return;
	block_t* block = dynamicArrayGet(block_array,node->index);
	removePowerGridElement(block->power_grid[0],node_ptr);
}

void applyPowerGrid(uint32_t base_ptr){
	node_t* node = dynamicArrayGet(node_root,base_ptr);
	block_t* block = dynamicArrayGet(block_array,node->index);
	block->power_grid[0] = -1;
	bool applied_buffer = false;
	for(int i = 0;i < 6;i++){
		int dx = i >> 1 == VEC3_X ? i & 1 ? -1 : 1 : 0;
		int dy = i >> 1 == VEC3_Y ? i & 1 ? -1 : 1 : 0;
		int dz = i >> 1 == VEC3_Z ? i & 1 ? -1 : 1 : 0;
		int x = node->pos.x + dx;
		int y = node->pos.y + dy;
		int z = node->pos.z + dz;
		bool applied = applyPowerGridSub(base_ptr,getNode(x,y,z,node->depth),border_block_table[i]);
		if(!applied)
			continue;
		if(node->type == BLOCK_WIRE){
			if(applied_buffer)
				floodFillPowerGrid(base_ptr);
		}
		else
			break;
		applied_buffer = applied;
	}
}

void powerGridTick(){
	for(int i = 0;i < power_grid_array.size;i++){
		float supply = 0.0f;
		power_grid_t* grid = dynamicArrayGet(power_grid_array,i);
		for(int j = 0;j < grid->component.size;j++){
			node_t* node = dynamicArrayGet(node_root,((int*)grid->component.data)[j]);
			if(node->type == BLOCK_BATTERY)
				supply += 1.0f;
			block_t* block = dynamicArrayGet(block_array,node->index);
			block->power = 0.0f;
		}
		while(supply > 0.0f){
			int demand = 0;
			for(int j = 0;j < grid->component.size;j++){
				node_t* node = dynamicArrayGet(node_root,((int*)grid->component.data)[j]);
				if(node->type == BLOCK_LAMP){
					demand++;
				}
			}
			if(!demand)
				break;
			float supply_single = supply / demand;
			for(int j = 0;j < grid->component.size;j++){
				node_t* node = dynamicArrayGet(node_root,((int*)grid->component.data)[j]);
				if(node->type == BLOCK_LAMP){
					block_t* block = dynamicArrayGet(block_array,node->index);
					block->power = supply_single;
					supply -= supply_single;
				}
			}
			supply -= 0.001f;
		}
	}
}