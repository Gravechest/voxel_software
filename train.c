#include "train.h"
#include "tree.h"

int getRailOrientation(uint32_t node_index){
	node_t* node = dynamicArrayGet(node_root,node_index);
	node_t* neighbour;
	int orientation = 0;
	neighbour = dynamicArrayGet(node_root,getNode(node->pos.x,node->pos.y + 1,node->pos.z,node->depth));
	if(neighbour->type == BLOCK_RAIL){
		air_t* air = dynamicArrayGet(air_array,neighbour->index);
		air->orientation |= 0x02;
		orientation |= 0x01;
	}
	neighbour = dynamicArrayGet(node_root,getNode(node->pos.x,node->pos.y - 1,node->pos.z,node->depth));
	if(neighbour->type == BLOCK_RAIL){
		air_t* air = dynamicArrayGet(air_array,neighbour->index);
		air->orientation |= 0x01;
		orientation |= 0x02;
	}
	neighbour = dynamicArrayGet(node_root,getNode(node->pos.x + 1,node->pos.y,node->pos.z,node->depth));
	if(neighbour->type == BLOCK_RAIL){
		air_t* air = dynamicArrayGet(air_array,neighbour->index);
		air->orientation |= 0x08;
		orientation |= 0x04;
	}
	neighbour = dynamicArrayGet(node_root,getNode(node->pos.x - 1,node->pos.y,node->pos.z,node->depth));
	if(neighbour->type == BLOCK_RAIL){
		air_t* air = dynamicArrayGet(air_array,neighbour->index);
		air->orientation |= 0x04;
		orientation |= 0x08;
	}
	return orientation;
}