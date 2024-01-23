#include "cellular_automata.h"
#include "tree.h"
#include "entity.h"
#include "tmath.h"

void liquidUnderSub(uint32_t node_ptr,node_t* liquid_node){
	block_t* liquid = dynamicArrayGet(block_array,liquid_node->index);
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_AIR){
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		int depth_difference = node->depth - liquid_node->depth;
		float cube_volume_rel = 1 << depth_difference * 2;
		float ammount_2 = ammount * cube_volume_rel;
		if(ammount_2 > 1.0f){
			float overflow = ammount_2 - 1.0f;
			ammount_2 = 1.0f;
			liquid->ammount = overflow / cube_volume_rel;
		}
		setVoxel(node->pos.x,node->pos.y,node->pos.z,node->depth,node->type,ammount_2);
		return;
	}
	if(node->type != BLOCK_PARENT){
		block_t* block = dynamicArrayGet(block_array,node->index);
		if(!(material_array[node->type].flags & MAT_LIQUID))
			return;
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		int depth_difference = node->depth - liquid_node->depth;
		float cube_volume_rel = 1 << depth_difference * 2;
		float ammount_2 = ammount * cube_volume_rel;
		block->ammount += ammount_2;
		float total_ammount = block->ammount + block->ammount;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			block->ammount -= overflow;
			liquid->ammount = overflow / cube_volume_rel;
		}
		return;
	}
	for(int i = 4;i < 8;i++)
		liquidUnderSub(node_ptr,node->child_s[i]);
}

void liquidUnder(int x,int y,int z,uint32_t depth){
	if(z - 1 < 0)
		return;
	node_t* liquid_node = dynamicArrayGet(node_root,getNode(x,y,z,depth));
	node_t* node = dynamicArrayGet(node_root,getNode(x,y,z - 1,depth));
	block_t* block = dynamicArrayGet(block_array,node->index);
	block_t* liquid = dynamicArrayGet(block_array,liquid_node->index);
	if(node->type == BLOCK_AIR){
		return;
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		setVoxel(x,y,z - 1,depth,liquid_node->type,ammount);
		return;
	}
	if(node->type != BLOCK_PARENT){
		if(!(material_array[node->type].flags & MAT_LIQUID))
			return;
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		block->ammount += ammount;
		float total_ammount = block->ammount + block->ammount;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			liquid->ammount = overflow;
			block->ammount -= overflow;
		}
		return;
	}
	for(int i = 4;i < 8;i++)
		liquidUnderSub(getNode(x,y,z,depth),node->child_s[i],liquid);
}

#define VISCOSITY 0.03f
#include <stdio.h>


void liquidSideSub(uint32_t node_ptr,node_t* base,uint32_t* neighbour){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	block_t* base_block = dynamicArrayGet(block_array,base->index);
	int depth_difference = tAbs(node->depth - base->depth);
	float base_height = base_block->ammount;
	float node_height = 0.0f;
	if(base->depth > node->depth){
		ivec3 base_pos = base->pos;
		base_pos.x >>= -depth_difference;
		base_pos.y >>= -depth_difference;
		base_pos.z >>= -depth_difference;
		if((base_pos.x == node->pos.x) + (base_pos.y == node->pos.y) + (base_pos.z == node->pos.z) != 1)
			return;
		int d = 1 << depth_difference - 1;
		base_height /= d;
		base_height += (base->pos.z & d - 1) / d;
		if(node->type == base->type){
			block_t* node_block = dynamicArrayGet(block_array,node->index);
			node_height += node_block->ammount;
			if(node_height < base_height)
				return;
		}
	}
	else if(base->depth < node->depth){
		if(node->type == base->type){
			block_t* node_block = dynamicArrayGet(block_array,node->index);
			node_height += node_block->ammount;
		}
		int d = 1 << depth_difference - 1;
		node_height /= d;
		node_height += (node->pos.z & d - 1) / d;
		if(node_height < base_height)
			return;
	}
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			liquidSideSub(node->child_s[neighbour[i]],base,neighbour);
		return;
	}
	float cube_volume = 1 << depth_difference * 2;
	float square_volume = 1 << depth_difference;
	if(node->type == BLOCK_AIR){
		float ammount = (base_height - node_height) * 0.5f * VISCOSITY;
		base_block->ammount -= ammount;
		setVoxel(node->pos.x,node->pos.y,node->pos.z,node->depth,base->type,ammount);
		return;
	}
	if(base->type == node->type){
		block_t* node_block = dynamicArrayGet(block_array,node->index);
		float ammount = (base_height - node_height) * 0.5f * VISCOSITY;
		base_block->ammount -= ammount;
		node_block->ammount += ammount;
		return;
	}
}

void liquidSide(uint32_t node_ptr,uint32_t side,uint32_t* neighbour){
	node_t* base = dynamicArrayGet(node_root,node_ptr);
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	uint32_t adjent = getNode(base->pos.x + dx,base->pos.y + dy,base->pos.z + dz,base->depth);
	liquidSideSub(adjent,base,neighbour);
}

void gasSpread(uint32_t x,uint32_t y,uint32_t z,uint32_t depth,uint32_t side,uint32_t* neighbour){
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	node_t* node = dynamicArrayGet(node_root,getNode(x + dx,y + dy,z + dz,depth));
	node_t* base = dynamicArrayGet(node_root,getNode(x,y,z,depth));
	if(node->type != BLOCK_AIR)
		return;
	air_t* node_air = dynamicArrayGet(air_array,node->index);
	air_t* base_air = dynamicArrayGet(air_array,base->index);
	float ammount_1 = node_air->o2;
	float ammount_2 = base_air->o2;
	float avg = (ammount_1 + ammount_2) * 0.5f;
	node_air->o2 = avg;
	base_air->o2 = avg;
}

#define POWDER_DEPTH 11

bool powderSideSub(uint32_t base_ptr,uint32_t node_ptr,uint32_t* neighbour,int dx,int dy,int dz){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			if(powderSideSub(base_ptr,node->child_s[neighbour[i]],neighbour,dx,dy,dz))
				return true;
		return false;
	}
	if(node->type != BLOCK_AIR)
		return false;
	node_t* base = dynamicArrayGet(node_root,base_ptr);
	int x = node->pos.x;
	int y = node->pos.y;
	int z = node->pos.z;
	int depth = node->depth;
	for(;depth < base->depth && depth < POWDER_DEPTH;depth++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += dx ? dx == -1 ? 1 : 0 : !!(base->pos.x & 1 << POWDER_DEPTH - depth - 1);
		y += dy ? dy == -1 ? 1 : 0 : !!(base->pos.y & 1 << POWDER_DEPTH - depth - 1);
		z += dz ? dz == -1 ? 1 : 0 : !!(base->pos.z & 1 << POWDER_DEPTH - depth - 1);
	}
	if(base->depth == POWDER_DEPTH){
		node_t* below = dynamicArrayGet(node_root,getNode(x,y,z - 1,POWDER_DEPTH));
		if(below->type < BLOCK_PARENT)
			return false;
		setVoxelSolid(x,y,z,POWDER_DEPTH,1);
		//removeVoxel(base_ptr);
		return true;
	}
	for(int i = node->depth;i < POWDER_DEPTH;i++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += dx ? dx == -1 ? 1 : 0 : TRND1 < 0.5f;
		y += dy ? dy == -1 ? 1 : 0 : TRND1 < 0.5f;
		z += dz ? dz == -1 ? 1 : 0 : TRND1 < 0.5f;
	}
	node_t* below = dynamicArrayGet(node_root,getNode(x,y,z - 1,POWDER_DEPTH));
	if(below->type < BLOCK_PARENT)
		return false;
	setVoxelSolid(x,y,z,POWDER_DEPTH,1);
	//removeVoxel(base_ptr);
	int depth_mask = (1 << POWDER_DEPTH - base->depth) - 1;
	addSubVoxel(base->pos.x << 1,base->pos.y << 1,base->pos.z << 1,x - dx & depth_mask,y - dy & depth_mask,z - dz & depth_mask,POWDER_DEPTH - base->depth,POWDER_DEPTH,1);
	return true;
}

bool powderSide(uint32_t node_ptr,int side){
	node_t* base = dynamicArrayGet(node_root,node_ptr);
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	uint32_t adjent = getNode(base->pos.x + dx,base->pos.y + dy,base->pos.z + dz,base->depth);
	return powderSideSub(node_ptr,adjent,border_block_table[side],dx,dy,dz);
}

bool powderUnderSub(uint32_t base_ptr,uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			if(powderUnderSub(base_ptr,node->child_s[border_block_table[4][i]]))
				return true;
		return false;
	}
	if(node->type != BLOCK_AIR)
		return false;
	node_t* base = dynamicArrayGet(node_root,base_ptr);
	int x = node->pos.x;
	int y = node->pos.y;
	int z = node->pos.z;
	int depth = node->depth;
	for(;depth < base->depth && depth < POWDER_DEPTH;depth++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += !!(base->pos.x & 1 << POWDER_DEPTH - depth - 1);
		y += !!(base->pos.y & 1 << POWDER_DEPTH - depth - 1);
		z++;
	}
	if(base->depth >= POWDER_DEPTH){
		setVoxelSolid(x,y,z,base->depth,1);
		//removeVoxel(base_ptr);
		return true;
	}
	for(;depth < POWDER_DEPTH;depth++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += TRND1 < 0.5f;
		y += TRND1 < 0.5f;
		z++;
	}
	setVoxelSolid(x,y,z,POWDER_DEPTH,1);
	//removeVoxel(base_ptr);
	int depth_mask = (1 << POWDER_DEPTH - base->depth) - 1;
	addSubVoxel(base->pos.x << 1,base->pos.y << 1,base->pos.z << 1,x & depth_mask,y & depth_mask,z + 1 & depth_mask,POWDER_DEPTH - base->depth,POWDER_DEPTH,1);
	return true;
}

bool powderUnder(uint32_t node_ptr){
	node_t* base = dynamicArrayGet(node_root,node_ptr);
	return powderUnderSub(node_ptr,getNode(base->pos.x,base->pos.y,base->pos.z - 1,base->depth));
}