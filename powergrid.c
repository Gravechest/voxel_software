#include "powergrid.h"
#include "memory.h"
#include "tree.h"

DYNAMIC_ARRAY(power_grid_array,power_grid_t);

#define BURN_EFFICIENCY_COAL 0.001f
#include <stdio.h>

static ivec3 getNeighbourPos(ivec3 pos,int side){
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	return (ivec3){pos.x + dx,pos.y + dy,pos.z + dz};
}

void powerGridRemoveElement(uint32_t power_grid,uint32_t element){
	power_grid_t* grid = dynamicArrayGet(power_grid_array,power_grid);
	for(int i = 0;i < grid->component.size;i++){
		uint32_t* index = dynamicArrayGet(grid->component,i);
		if(*index == element){
			*index = 0;
			dynamicArrayRemove(&grid->component,i);
			break;
		}
	}
	if(grid->component.size == grid->component.free_ptr){
		dynamicArrayReset(&grid->component);
		dynamicArrayRemove(&power_grid_array,power_grid);
	}
}

void powerGridFloodFill(uint32_t base_ptr,uint16_t power_grid_id){
	node_t* node = dynamicArrayGet(node_root,base_ptr);
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(node->type == BLOCK_AIR)
		return;
	if(node->type == BLOCK_PARENT)
		return;
	if(!(material_array[node->type].flags & MAT_POWER))
		return;
	if(block->power_grid[0] == power_grid_id)
		return;
	if(node->type == BLOCK_WIRE){
		block->power_grid[0] = power_grid_id;
		for(int i = 0;i < 6;i++){
			ivec3 pos = getNeighbourPos(node->pos,i);
			block_t* block = dynamicArrayGet(block_array,node->index);
			powerGridFloodFill(getNode(pos.x,pos.y,pos.z,node->depth),power_grid_id);
		}
		return;
	}
	uint16_t* grid_index = &block->power_grid[0];
	if(node->type == BLOCK_SWITCH){
		for(int i = 0;i < 6;i++){
			ivec3 pos = getNeighbourPos(node->pos,i);
			node_t* neighbour_node = dynamicArrayGet(node_root,getNode(pos.x,pos.y,pos.z,node->depth));
			if(neighbour_node->type != BLOCK_WIRE)
				continue;
			block_t* neighbour_block = dynamicArrayGet(block_array,neighbour_node->index);
			if(neighbour_block->power_grid[0] == GRID_DISCONNECTED)
				continue;
			grid_index = &block->power_grid[1];
			break;
		}
	}
	if(block->power_grid[0] != GRID_DISCONNECTED && block->power_grid[0] != GRID_TOGGLE)
		powerGridRemoveElement(block->power_grid[0],base_ptr);
	block->power_grid[0] = power_grid_id;
	if(power_grid_id == GRID_DISCONNECTED || power_grid_id == GRID_TOGGLE)
		return;
	power_grid_t* grid = dynamicArrayGet(power_grid_array,power_grid_id);
	dynamicArrayAdd(&grid->component,&base_ptr);
}

int powerGridCountFloodFill(uint32_t base_ptr){
	node_t* node = dynamicArrayGet(node_root,base_ptr);
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(node->type == BLOCK_AIR)
		return 0;
	if(node->type == BLOCK_PARENT)
		return 0;
	if(!(material_array[node->type].flags & MAT_POWER))
		return 0;
	if(block->power_grid[0] == GRID_TOGGLE)
		return 0;
	if(node->type == BLOCK_WIRE){
		int count = 0;
		block->power_grid[0] = GRID_TOGGLE;
		for(int i = 0;i < 6;i++){
			ivec3 pos = getNeighbourPos(node->pos,i);
			count += powerGridCountFloodFill(getNode(pos.x,pos.y,pos.z,node->depth));
		}
		return count;
	}
	if(node->type == BLOCK_SWITCH){
		for(int i = 0;i < 6;i++){
			ivec3 pos = getNeighbourPos(node->pos,i);
			node_t* neighbour_node = dynamicArrayGet(node_root,getNode(pos.x,pos.y,pos.z,node->depth));
			if(neighbour_node->type != BLOCK_WIRE)
				continue;
			block_t* neighbour_block = dynamicArrayGet(block_array,neighbour_node->index);
			if(neighbour_block->power_grid[0] == GRID_DISCONNECTED)
				continue;
			if(neighbour_block->power_grid[0] == GRID_TOGGLE)
				continue;
			if(block->power_grid[1] != GRID_DISCONNECTED)
				powerGridRemoveElement(block->power_grid[0],base_ptr);
			block->power_grid[1] = GRID_TOGGLE;
			return 1;
		}
	}
	if(block->power_grid[0] != GRID_DISCONNECTED)
		powerGridRemoveElement(block->power_grid[0],base_ptr);
	block->power_grid[0] = GRID_TOGGLE;
	return 1;
}

void powerGridRefresh(uint32_t base_ptr){
	node_t* node = dynamicArrayGet(node_root,base_ptr);
	block_t* block = dynamicArrayGet(block_array,node->index);
	int grid_index;
	int count = powerGridCountFloodFill(base_ptr);
	if(count > 1){
		power_grid_t grid;
		for(int i = 0;i < sizeof(power_grid_t);i++)
			((char*)&grid.component)[i] = 0;
		grid.component.element_size = sizeof(uint32_t);
		grid.throughput = 1024.0f / ((1 << node->depth) * (1 << node->depth));
		grid_index = dynamicArrayTop(power_grid_array);
		dynamicArrayAdd(&power_grid_array,&grid);
	}
	else
		grid_index = GRID_DISCONNECTED;
	powerGridFloodFill(base_ptr,grid_index);
}

void powerGridRemove(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(block->power_grid[0] == GRID_DISCONNECTED)
		return;
	uint32_t buffer = node->index;
	node->type = BLOCK_AIR;
	for(int i = 0;i < 6;i++){
		ivec3 pos = getNeighbourPos(node->pos,i);
		uint32_t neighbour = getNode(pos.x,pos.y,pos.z,node->depth);
		node_t* neighbour_node = dynamicArrayGet(node_root,neighbour);
		if(neighbour_node->type == BLOCK_AIR || neighbour_node->type == BLOCK_PARENT)
			continue;
		if(material_array[neighbour_node->type].flags & MAT_POWER){
			block_t* neighbour_block = dynamicArrayGet(block_array,neighbour_node->index);
			powerGridRefresh(neighbour);
		}
		node = dynamicArrayGet(node_root,node_ptr);
	}
	node->type = buffer;
	if(node->type == BLOCK_WIRE)
		return;
 	powerGridRemoveElement(block->power_grid[0],node_ptr);
}

void powerGridApply(uint32_t base_ptr){
	node_t* node = dynamicArrayGet(node_root,base_ptr);
	block_t* block = dynamicArrayGet(block_array,node->index);
	block->power_grid[0] = GRID_DISCONNECTED;
	block->power_grid[1] = GRID_DISCONNECTED;
	if(node->type != BLOCK_WIRE){
		for(int i = 0;i < 6;i++){
			ivec3 pos = getNeighbourPos(node->pos,i);
			uint32_t neighbour = getNode(pos.x,pos.y,pos.z,node->depth);
			node_t* neighbour_node = dynamicArrayGet(node_root,neighbour);
			if(neighbour_node->type == BLOCK_WIRE){
				powerGridRefresh(neighbour);
				return;
			}
		}
		return;
	}
	powerGridRefresh(base_ptr);
}

void powerGridTick(){
	for(int i = 0;i < power_grid_array.size;i++){
		power_grid_t* grid = dynamicArrayGet(power_grid_array,i);
		grid->demand = 0;
		grid->suppliers = 0;
		grid->used_supply = 0.0f;
		for(int j = 0;j < grid->component.size;j++){
			uint32_t* index = dynamicArrayGet(grid->component,j);
			if(!*index)
				continue;
			node_t* node = dynamicArrayGet(node_root,*index);
			block_t* block = dynamicArrayGet(block_array,node->index);
			if(node->type == BLOCK_GENERATOR){
				if(block->burn_progress > 0.0f){
					grid->supply += 1.0f;
					grid->suppliers++;
				}
				else{
					if(block->inventory[0].type == BLOCK_COAL){
						itemChange(&block->inventory[0],-1);
						block->burn_progress = 1.0f;
						grid->supply += 1.0f;
						grid->suppliers++;
					}
				}
			}
			if(node->type == BLOCK_LAMP){
				block->power = 0.0f;
				grid->demand++;
			}
		}
	}
	for(int j = 0;j < 8;j++){
		for(int i = 0;i < power_grid_array.size;i++){
			power_grid_t* grid = dynamicArrayGet(power_grid_array,i);
			float power = grid->supply / grid->demand;
			for(int j = 0;j < grid->component.size;j++){
				uint32_t* index = dynamicArrayGet(grid->component,j);
				if(!*index)
					continue;
				node_t* node = dynamicArrayGet(node_root,*index);
				block_t* block = dynamicArrayGet(block_array,node->index);
				if(node->type == BLOCK_SWITCH && block->on){
					power_grid_t* other_grid;
					if(block->power_grid[0] == i && block->power_grid[1] != GRID_DISCONNECTED)
						other_grid = dynamicArrayGet(power_grid_array,block->power_grid[1]);
					else if(block->power_grid[1] == i && block->power_grid[0] != GRID_DISCONNECTED)
						other_grid = dynamicArrayGet(power_grid_array,block->power_grid[0]);
					else
						continue;
					float av_suply;
					float av_suply_2;
					float acc = other_grid->supply + grid->supply;
					if(!grid->supply || !other_grid->demand){
						other_grid->supply = acc * 0.5f;
						grid->supply = acc * 0.5f;
					}
					else{
						int acc_demand = grid->demand + other_grid->demand;
						other_grid->supply = acc / (other_grid->demand / acc_demand);
						grid->supply = acc / (grid->demand / acc_demand);
					}
				}
			}
		}
		for(int i = 0;i < power_grid_array.size;i++){
			power_grid_t* grid = dynamicArrayGet(power_grid_array,i);
			float power = grid->supply / grid->demand;
			for(int j = 0;j < grid->component.size;j++){
				uint32_t* index = dynamicArrayGet(grid->component,j);
				if(!*index)
					continue;
				node_t* node = dynamicArrayGet(node_root,*index);
				block_t* block = dynamicArrayGet(block_array,node->index);
				if(node->type == BLOCK_LAMP){
					block->power += power;
					grid->supply -= power;
					grid->used_supply += power;
				}
			}
		}
	}
	for(int i = 0;i < power_grid_array.size;i++){
		power_grid_t* grid = dynamicArrayGet(power_grid_array,i);
		for(int j = 0;j < grid->component.size;j++){
			uint32_t* index = dynamicArrayGet(grid->component,j);
			if(!*index)
				continue;
			node_t* node = dynamicArrayGet(node_root,*index);
			block_t* block = dynamicArrayGet(block_array,node->index);
			if(node->type == BLOCK_GENERATOR){
				if(block->burn_progress){
					block->burn_progress -= 1.0f * (grid->used_supply / grid->suppliers) * BURN_EFFICIENCY_COAL;
				}
			}
		}
	}
}