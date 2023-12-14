#include <math.h>
#include <intrin.h>

#include "draw.h" 
#include "tmath.h"
#include "lighting.h"
#include "vec2.h"
#include "memory.h"

void lightNewStackPut(node_t* node,block_t* block,uint side){
	if(light_new_stack_ptr >= LIGHT_NEW_STACK_SIZE - 1)
		return;

	light_new_stack_t* stack = &light_new_stack[light_new_stack_ptr++];
	stack->node = node;
	stack->side = side;

	uint size = GETLIGHTMAPSIZE(node->depth);
	block->side[side].luminance_p = MALLOC_ZERO(sizeof(vec3) * (size + 1) * (size + 1));
}
