#include "tree.h"
#include "tmath.h"
#include "memory.h"
#include "lighting.h"
#include "fog.h"

uint block_node_c;
block_node_t* node_root;

ray3i_t ray3CreateI(vec3 pos,vec3 dir){
	ray3i_t ray;

	ray.square_side = 0;
	ray.pos = (uvec3){pos.x * (1 << FIXED_PRECISION),pos.y * (1 << FIXED_PRECISION),pos.z * (1 << FIXED_PRECISION)};
	ray.dir = (ivec3){dir.x * (1 << FIXED_PRECISION),dir.y * (1 << FIXED_PRECISION),dir.z * (1 << FIXED_PRECISION)};
	vec3 delta = vec3absR(vec3divFR(dir,1.0f));
	ray.delta.x = (delta.x * (1 << FIXED_PRECISION));
	ray.delta.y = (delta.y * (1 << FIXED_PRECISION));
	ray.delta.z = (delta.z * (1 << FIXED_PRECISION));

	ray.step.x = ray.dir.x < 0 ? -1 : 1;
	ray.step.y = ray.dir.y < 0 ? -1 : 1;
	ray.step.z = ray.dir.z < 0 ? -1 : 1;

	ivec3 fract_pos;
	fract_pos.x = ray.pos.x & (1 << FIXED_PRECISION) - 1;
	fract_pos.y = ray.pos.y & (1 << FIXED_PRECISION) - 1;
	fract_pos.z = ray.pos.z & (1 << FIXED_PRECISION) - 1;

	ray.side.x = (ray.dir.x < 0 ? fract_pos.x : (1 << FIXED_PRECISION) - fract_pos.x);
	ray.side.y = (ray.dir.y < 0 ? fract_pos.y : (1 << FIXED_PRECISION) - fract_pos.y);
	ray.side.z = (ray.dir.z < 0 ? fract_pos.z : (1 << FIXED_PRECISION) - fract_pos.z);

	ray.side.x = (long long)ray.side.x * ray.delta.x >> FIXED_PRECISION;
	ray.side.y = (long long)ray.side.y * ray.delta.y >> FIXED_PRECISION;
	ray.side.z = (long long)ray.side.z * ray.delta.z >> FIXED_PRECISION;

	ray.square_pos.x = ray.pos.x >> FIXED_PRECISION;
	ray.square_pos.y = ray.pos.y >> FIXED_PRECISION;
	ray.square_pos.z = ray.pos.z >> FIXED_PRECISION;


	return ray;
}

uint getNodeFromPos(vec3 pos,uint depth){
	vec3div(&pos,MAP_SIZE);
	block_node_t* node = node_root;
	uint node_ptr = 0;
	for(int i = 0;i < depth;i++){
		uint child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!child_ptr)
			return 0;
		node_ptr = child_ptr;
		vec3mul(&pos,2.0f);
	}
	pos.x = tFractUnsigned(pos.x * 0.5f) * 2.0f;
	pos.y = tFractUnsigned(pos.y * 0.5f) * 2.0f;
	pos.z = tFractUnsigned(pos.z * 0.5f) * 2.0f;
	return node_ptr;
}

traverse_init_t initTraverse(vec3 pos){
	vec3div(&pos,MAP_SIZE);
	block_node_t* node = node_root;
	uint node_ptr = 0;
	for(;;){
		uint child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!child_ptr)
			break;
		node_ptr = child_ptr;
		vec3mul(&pos,2.0f);
	}
	pos.x = tFractUnsigned(pos.x * 0.5f) * 2.0f;
	pos.y = tFractUnsigned(pos.y * 0.5f) * 2.0f;
	pos.z = tFractUnsigned(pos.z * 0.5f) * 2.0f;
	return (traverse_init_t){pos,node_ptr};
}

block_t* insideBlock(vec3 pos){
	block_node_t* node = node_root;
	uint node_ptr = 0;
	vec3div(&pos,MAP_SIZE);
	for(;;){
		node_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!node_ptr)
			return 0;
		if(node[node_ptr].block)
			return node[node_ptr].block;
		vec3mul(&pos,2.0f);
	}
}

uint getNode(int x,int y,int z,int depth){
	block_node_t* node = node_root;
	uint node_ptr = 0;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		if(!node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x])
			return node->block ? node_ptr : 0;
		node_ptr = node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x];
	}
	return node_ptr;
}

void removeVoxel(uint node_ptr){
	block_node_t* node = &node_root[node_ptr];
	if(node->block){
		for(int i = 0;i < 6;i++){
			if(!node->block->side[i].luminance_p)
				continue;
			MFREE(node->block->side[i].luminance_p);
		}
		memset(node->block,0,sizeof(block_t));
		MFREE(node->block);
	}
	uint parent_ptr = node->parent;
	block_node_t* parent = &node_root[parent_ptr];
	if(parent_ptr == -1)
		return;
	memset(node,0,sizeof(block_node_t));
	bool exit = false;
	for(int i = 0;i < 8;i++){
		if(parent->child_s[i] == node_ptr){
			parent->child_s[i] = 0;
			parent->shape ^= 1 << i;
			continue;
		}
		if(parent->child_s[i])
			exit = true;
	}
	if(exit)
		return;
	removeVoxel(parent_ptr);
}

static uint stack_ptr;
static uint stack[240000];

void removeSubVoxel(uint node_ptr){
	block_node_t* node = &node_root[node_ptr];
	if(node->block){
		for(int i = 0;i < 6;i++){
			if(!node->block->side[i].luminance_p)
				continue;
			MFREE(node->block->side[i].luminance_p);
		}
		MFREE(node->block);
		memset(node,0,sizeof(block_node_t));
		stack[stack_ptr++] = node_ptr;
		return;
	}
	for(int i = 0;i < 8;i++){
		if(!node->child_s[i])
			continue;
		removeSubVoxel(node->child_s[i]);
		node->child_s[i] = 0;
		node->shape ^= 1 << i;
	}
}

void setVoxel(uint x,uint y,uint z,uint depth,uint material){
	block_node_t* node = node_root;
	uint node_ptr = 0;
	for(int j = 0;j < 6;j++){
		uint dx = (uint[]){1,-1,0,0,0,0}[j];
		uint dy = (uint[]){0,0,1,-1,0,0}[j];
		uint dz = (uint[]){0,0,0,0,1,-1}[j];
		uint neighbour = getNode(x + dx,y + dy,z + dz,depth);
		if(neighbour && node_root[neighbour].block){
			block_t* block = node_root[neighbour].block;
			for(int k = 0;k < 6;k++){
				MFREE(block->side[k].luminance_p);
				block->side[k].luminance_p = 0;
			}
		}
	}
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		uint* child = &node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x];
		if(!*child){
			*child = stack_ptr ? stack[--stack_ptr] : ++block_node_c;
			node[node_ptr].shape ^= 1 << sub_coord.z * 4 + sub_coord.y * 2 + sub_coord.x;
			node[*child].parent = node_ptr;
			node[*child].pos = (ivec3){x >> i,y >> i,z >> i};
			node[*child].depth = depth - i;
		}
		node_ptr = *child;
	}
	for(;;){
		uint parent_ptr = node[node_ptr].parent;
		for(int i = 0;i < 8;i++){
			uint child_ptr = node[parent_ptr].child_s[i];
			if(child_ptr == node_ptr)
				continue;
			if(child_ptr && node[child_ptr].block && node[child_ptr].block->material == material)
				continue;
			removeSubVoxel(node_ptr);
			block_t* block = MALLOC_ZERO(sizeof(block_t));
			block->material = material;
			node[node_ptr].block = block;
			return;
		}
		node_ptr = node[node_ptr].parent;
		x >>= 1;
		y >>= 1;
		z >>= 1;
		depth--;
	}
}

void recalculateRay(ray3_t* ray){
	ray->side.x = tFractUnsigned(ray->pos.x);
	ray->side.y = tFractUnsigned(ray->pos.y);
	ray->side.z = tFractUnsigned(ray->pos.z);

	ray->side.x = (ray->dir.x < 0.0f ? ray->side.x : 1.0f - ray->side.x) * ray->delta.x;
	ray->side.y = (ray->dir.y < 0.0f ? ray->side.y : 1.0f - ray->side.y) * ray->delta.y;
	ray->side.z = (ray->dir.z < 0.0f ? ray->side.z : 1.0f - ray->side.z) * ray->delta.z;

	ray->square_pos.x = ray->pos.x;
	ray->square_pos.y = ray->pos.y;
	ray->square_pos.z = ray->pos.z;
}

void recalculateRayI(ray3i_t* ray){
	ray->side.x = ray->pos.x & (1 << FIXED_PRECISION) - 1;
	ray->side.y = ray->pos.y & (1 << FIXED_PRECISION) - 1;
	ray->side.z = ray->pos.z & (1 << FIXED_PRECISION) - 1;

	ray->side.x = ray->dir.x < 0 ? ray->side.x : (1 << FIXED_PRECISION) - ray->side.x;
	ray->side.y = ray->dir.y < 0 ? ray->side.y : (1 << FIXED_PRECISION) - ray->side.y;
	ray->side.z = ray->dir.z < 0 ? ray->side.z : (1 << FIXED_PRECISION) - ray->side.z;

	ray->side.x = (long long)ray->side.x * ray->delta.x >> FIXED_PRECISION;
	ray->side.y = (long long)ray->side.y * ray->delta.y >> FIXED_PRECISION;
	ray->side.z = (long long)ray->side.z * ray->delta.z >> FIXED_PRECISION;

	ray->square_pos.x = ray->pos.x >> FIXED_PRECISION;
	ray->square_pos.y = ray->pos.y >> FIXED_PRECISION;
	ray->square_pos.z = ray->pos.z >> FIXED_PRECISION;
}

#define TREE_ALMOST_TWO 1.99999f

void ray3iItterate(ray3i_t* ray){
	if(ray->side.z < ray->side.y){
		if(ray->side.z < ray->side.x){
			ray->square_pos.z += ray->step.z;
			ray->side.z += ray->delta.z;
			ray->square_side = VEC3_Z;
			return;
		}
		ray->square_pos.x += ray->step.x;
		ray->side.x += ray->delta.x;
		ray->square_side = VEC3_X;
		return;
	}
	uint p = ray->side.y < ray->side.x;
	ray->square_pos.a[p] += ray->step.a[p];
	ray->side.a[p] += ray->delta.a[p];
	ray->square_side = p;
}

node_hit_t traverseTreeI(ray3i_t ray,uint node_ptr){
	block_node_t* node = node_root;
	ray3iItterate(&ray);
	for(;;){
		uint combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(node[node_ptr].parent == 0xffffffff)
				return (node_hit_t){.node = 0,.side = ray.square_side};
			
			ray.pos.x >>= 1;
			ray.pos.y >>= 1;
			ray.pos.z >>= 1;

			ray.pos.x += (node[node_ptr].pos.x & 1) << FIXED_PRECISION;
			ray.pos.y += (node[node_ptr].pos.y & 1) << FIXED_PRECISION;
			ray.pos.z += (node[node_ptr].pos.z & 1) << FIXED_PRECISION;

			recalculateRayI(&ray);

			node_ptr = node[node_ptr].parent;
			ray3iItterate(&ray);
			continue;
		}
	yeet:
		uint child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(!child){
			ray3iItterate(&ray);
			continue;
		}
		if(node[child].block)
			return (node_hit_t){child,ray.square_side};

		uvec3 pre = ray.pos;
		uint x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		long long side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];

		ray.pos.a[x] = (ray.pos.a[x] + (side_delta * ray.dir.a[x] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
		ray.pos.a[y] = (ray.pos.a[y] + (side_delta * ray.dir.a[y] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
		ray.pos.a[ray.square_side] = ray.dir.a[ray.square_side] >> 31 & (2 << FIXED_PRECISION) - 1;
		if(ray.dir.z > 0){
			if(ray.pos.z > (1 << FIXED_PRECISION) && !(node[child].shape & 0xf0)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.z < (1 << FIXED_PRECISION) && !(node[child].shape & 0x0f)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		if(ray.dir.y > 0){
			if(ray.pos.y > (1 << FIXED_PRECISION) && !(node[child].shape & 0xcc)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.y < (1 << FIXED_PRECISION) && !(node[child].shape & 0x33)){
			ray.pos = pre;
			ray3iItterate(&ray);
			continue;
			}
		}
		if(ray.dir.x > 0){
			if(ray.pos.x > (1 << FIXED_PRECISION) && !(node[child].shape & 0xaa)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.x < (1 << FIXED_PRECISION) && !(node[child].shape & 0x55)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		node_ptr = child;
		recalculateRayI(&ray);
		goto yeet;
	}
}

uint traverseTreeItt(ray3i_t ray,uint node_ptr){
	block_node_t* node = node_root;
	ray3iItterate(&ray);
	uint i = 0;
	for(;;){
		uint combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(node[node_ptr].parent == 0xffffffff)
				return i;
			
			ray.pos.x >>= 1;
			ray.pos.y >>= 1;
			ray.pos.z >>= 1;

			ray.pos.x += (node[node_ptr].pos.x & 1) << FIXED_PRECISION;
			ray.pos.y += (node[node_ptr].pos.y & 1) << FIXED_PRECISION;
			ray.pos.z += (node[node_ptr].pos.z & 1) << FIXED_PRECISION;

			recalculateRayI(&ray);

			node_ptr = node[node_ptr].parent;
			i++;
			ray3iItterate(&ray);
			continue;
		}
	yeet:
		uint child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(!child){
			i++;
			ray3iItterate(&ray);
			continue;
		}
		if(node[child].block)
			return i;

		uvec3 pre = ray.pos;
		uint x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		long long side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];

		ray.pos.a[x] = (ray.pos.a[x] + (side_delta * ray.dir.a[x] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
		ray.pos.a[y] = (ray.pos.a[y] + (side_delta * ray.dir.a[y] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
		ray.pos.a[ray.square_side] = ray.dir.a[ray.square_side] >> 31 & (2 << FIXED_PRECISION) - 1;
			
		if(ray.dir.z > 0){
			if(ray.pos.z > (1 << FIXED_PRECISION) && !(node[child].shape & 0xf0)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.z < (1 << FIXED_PRECISION) && !(node[child].shape & 0x0f)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		if(ray.dir.y > 0){
			if(ray.pos.y > (1 << FIXED_PRECISION) && !(node[child].shape & 0xcc)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.y < (1 << FIXED_PRECISION) && !(node[child].shape & 0x33)){
			ray.pos = pre;
			ray3iItterate(&ray);
			continue;
			}
		}
		if(ray.dir.x > 0){
			if(ray.pos.x > (1 << FIXED_PRECISION) && !(node[child].shape & 0xaa)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.x < (1 << FIXED_PRECISION) && !(node[child].shape & 0x55)){
				ray.pos = pre;
				ray3iItterate(&ray);
				continue;
			}
		}
			
		node_ptr = child;
		recalculateRayI(&ray);
		goto yeet;
	}
}

node_hit_t traverseTree(ray3_t ray,uint node_ptr){
	bool small_x = ray.dir.x < 0.0001f && ray.dir.x > -0.0001f;
	bool small_y = ray.dir.y < 0.0001f && ray.dir.y > -0.0001f;
	bool small_z = ray.dir.z < 0.0001f && ray.dir.z > -0.0001f;
	if(small_x || small_y || small_z)
		return (node_hit_t){.node = 0,.side = ray.square_side};		
	block_node_t* node = node_root;
	ray3Itterate(&ray);
	for(;;){
		uint combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(node[node_ptr].parent == 0xffffffff)
				return (node_hit_t){.node = 0,.side = ray.square_side};
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3){node[node_ptr].pos.x & 1,node[node_ptr].pos.y & 1,node[node_ptr].pos.z & 1});
			
			recalculateRay(&ray);
			node_ptr = node[node_ptr].parent;
			ray3Itterate(&ray);
			continue;
		}
	yeet2:
		uint child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(!child){
			ray3Itterate(&ray);
			continue;
		}
		if(node[child].block)
			return (node_hit_t){child,ray.square_side};

		uint x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
		vec3 pre = ray.pos;
		ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
		
		if(ray.dir.z > 0.0f){
			if(ray.pos.z > 1.0f && !(node[child].shape & 0xf0)){
				ray.pos = pre;
				ray3Itterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.z < 1.0f && !(node[child].shape & 0x0f)){
				ray.pos = pre;
				ray3Itterate(&ray);
				continue;
			}
		}
		if(ray.dir.y > 0.0f){
			if(ray.pos.y > 1.0f && !(node[child].shape & 0xcc)){
				ray.pos = pre;
				ray3Itterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.y < 1.0f && !(node[child].shape & 0x33)){
				ray.pos = pre;
				ray3Itterate(&ray);
				continue;
			}
		}
		if(ray.dir.x > 0.0f){
			if(ray.pos.x > 1.0f && !(node[child].shape & 0xaa)){
				ray.pos = pre;
				ray3Itterate(&ray);
				continue;
			}
		}
		else{
			if(ray.pos.x < 1.0f && !(node[child].shape & 0x55)){
				ray.pos = pre;
				ray3Itterate(&ray);
				continue;
			}
		}
		
		node_ptr = child;
		recalculateRay(&ray);
		goto yeet2;
	}
}

beam_result_t traverseTreeBeamI(traverse_init_t init[4],uint x,uint y,uint beam_size){
	uint node_a[4];
	block_node_t* node = node_root;
	uint hit_side[4];
	
	/*
	if(1){
		uint r_x = x + 16 * 0.5f;
		uint r_y = y + 16 * 0.5f;
		ray3i_t ray = ray3CreateI(init[0].pos,getRayAngle(r_x,r_y));
		uint itt = traverseTreeItt(ray,init[0].node);
		for(int i = x;i < x + 16;i++){
			for(int j = y;j < y + 16;j++){
				vram.draw[i * (int)WND_RESOLUTION.y + j] = (pixel_t){0,tMin(itt << 3,255),0};
			}
		}
		return (beam_result_t){.node = node_a[0],.side = -3};
	}
	*/
	
	for(int i = 0;i < 4;i++){
		uint r_x = x + (i / 2) * beam_size;
		uint r_y = y + (i & 1) * beam_size;
		node_hit_t hit;
		vec3 angle = getRayAngle(r_x,r_y);
		if(tAbsf(angle.x) < 0.02f || tAbsf(angle.y) < 0.02f || tAbsf(angle.z) < 0.02f){
			ray3_t ray = ray3Create(init[i].pos,angle);
			hit = traverseTree(ray,init[i].node);
		}
		else{
			ray3i_t ray = ray3CreateI(init[i].pos,angle);
			hit = traverseTreeI(ray,init[i].node);
		}
		node_a[i] = hit.node;
		hit_side[i] = hit.side;
	}

	bool different_1 = node_a[0] != node_a[1] || hit_side[0] != hit_side[1];
	bool different_2 = node_a[0] != node_a[2] || hit_side[0] != hit_side[2];
	bool different_3 = node_a[0] != node_a[3] || hit_side[0] != hit_side[3];

	if(!node_a[0] && !node_a[1] && !node_a[2] && !node_a[3])
		return (beam_result_t){.node = node_a[0],.side = -2};

	if(!different_1 && !different_2 && !different_3)
		return (beam_result_t){.node = node_a[0],.side = hit_side[0]};

	uint result_node = 0;
	uint shortest = 0;
	float shortest_distance = 999999.0f;
	plane_t plane_shortest;
	beam_result_t result = {
		.node = result_node,
		.side = -1
	};

	for(int i = 0;i < 4;i++){
		uint r_x = x + (i / 2) * beam_size;
		uint r_y = y + (i & 1) * beam_size;
		if(!node_a[i]){
			switch(hit_side[i]){
			case VEC3_X:
				result.plane[i].normal = (vec3){1.0f,0.0f,0.0f};
				result.plane[i].x = VEC3_Y;
				result.plane[i].y = VEC3_Z;
				result.plane[i].z = VEC3_X;
				result.plane[i].pos = (vec3){getRayAngle(r_x,r_y).x > 0.0f ? MAP_SIZE * 2.0f : 0.0f,0.0f,0.0f};
				break;
			case VEC3_Y:
				result.plane[i].normal = (vec3){0.0f,1.0f,0.0f};
				result.plane[i].x = VEC3_X;
				result.plane[i].y = VEC3_Z;
				result.plane[i].z = VEC3_Y;
				result.plane[i].pos = (vec3){0.0f,getRayAngle(r_x,r_y).y > 0.0f ? MAP_SIZE * 2.0f : 0.0f,0.0f};
				break;
			case VEC3_Z:
				result.plane[i].normal = (vec3){0.0f,0.0f,1.0f};
				result.plane[i].x = VEC3_X;
				result.plane[i].y = VEC3_Y;
				result.plane[i].z = VEC3_Z;
				result.plane[i].pos = (vec3){0.0f,0.0f,getRayAngle(r_x,r_y).z > 0.0f ? MAP_SIZE * 2.0f : 0.0f};
				break;
			}
			result.side_a[i] = hit_side[i];
			result.node_a[i] = -1;
			continue;
		}
		int depth = node[node_a[i]].depth;
		ivec3 pos = node[node_a[i]].pos;
		float block_size = (float)((1 << 20) >> depth) * MAP_SIZE_INV;
		vec3 block_pos = vec3mulR((vec3){pos.x,pos.y,pos.z},block_size);
		vec3 dir = getRayAngle(r_x,r_y);
		result.plane[i] = getPlane(block_pos,vec3divR(dir,1 << FIXED_PRECISION),hit_side[i],block_size);
		result.side_a[i] = hit_side[i];
		result.node_a[i] = node_a[i];
	}
	return result;
}