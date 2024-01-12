#include "tree.h"
#include "tmath.h"
#include "memory.h"
#include "lighting.h"
#include "fog.h"
#include "powergrid.h"
#include "dynamic_array.h"

DYNAMIC_ARRAY(node_root,node_t);
DYNAMIC_ARRAY(block_array,block_t);
DYNAMIC_ARRAY(air_array,air_t);

float getBlockSize(int depth){
	return (float)((1 << 20) >> depth) * MAP_SIZE_INV;
}

float rayNodeGetDistance(vec3_t ray_pos,ivec3 pos,int depth,vec3_t angle,int side){
	vec3_t block_pos;
	float block_size = (float)MAP_SIZE / (1 << depth) * 2.0f;
	block_pos.x = pos.x * block_size;
	block_pos.y = pos.y * block_size;
	block_pos.z = pos.z * block_size;
	plane_ray_t plane = getPlane(block_pos,angle,side,block_size);
	vec3_t plane_pos = vec3subvec3R(plane.pos,ray_pos);
	return rayIntersectPlane(plane_pos,angle,plane.normal);
}

float rayGetDistance(vec3_t ray_pos,vec3_t ray_dir){
	traverse_init_t init = initTraverse(ray_pos);
	node_hit_t result = treeRay(ray3Create(init.pos,ray_dir),init.node,ray_pos);
	if(!result.node)
		return 999999.0f;
	node_t* node_hit = dynamicArrayGet(node_root,result.node);
	return rayNodeGetDistance(ray_pos,node_hit->pos,node_hit->depth,ray_dir,result.side);
}

ray3i_t ray3CreateI(vec3_t pos,vec3_t dir){
	ray3i_t ray;

	ray.square_side = 0;
	ray.pos = (uvec3){pos.x * (1 << FIXED_PRECISION),pos.y * (1 << FIXED_PRECISION),pos.z * (1 << FIXED_PRECISION)};
	ray.dir = (ivec3){dir.x * (1 << FIXED_PRECISION),dir.y * (1 << FIXED_PRECISION),dir.z * (1 << FIXED_PRECISION)};
	vec3_t delta = vec3absR(vec3divFR(dir,1.0f));
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

uint32_t getNodeFromPos(vec3_t pos,uint32_t depth){
	vec3div(&pos,MAP_SIZE);
	node_t* node = dynamicArrayGet(node_root,0);
	uint32_t node_ptr = 0;
	for(int i = 0;i < depth;i++){
		uint32_t child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
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

node_t treeTraverse(vec3_t pos){
	vec3div(&pos,MAP_SIZE);
	node_t* node = dynamicArrayGet(node_root,0);
	uint32_t node_ptr = 0;
	for(;;){
		uint32_t child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!child_ptr)
			return *(node_t*)dynamicArrayGet(node_root,node_ptr); 
		node_ptr = child_ptr;
		vec3mul(&pos,2.0f);
	}
}

traverse_init_t initTraverse(vec3_t pos){
	vec3div(&pos,MAP_SIZE);
	node_t* node = node_root.data;
	uint32_t node_ptr = 0;
	for(;;){
		uint32_t child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(node[child_ptr].type != BLOCK_PARENT)
			break;
		node_ptr = child_ptr;
		vec3mul(&pos,2.0f);
	}
	pos.x = tFractUnsigned(pos.x * 0.5f) * 2.0f;
	pos.y = tFractUnsigned(pos.y * 0.5f) * 2.0f;
	pos.z = tFractUnsigned(pos.z * 0.5f) * 2.0f;
	return (traverse_init_t){pos,node_ptr};
}

block_t* insideBlock(vec3_t pos){
	node_t* node = dynamicArrayGet(node_root,0);
	uint32_t node_ptr = 0;
	vec3div(&pos,MAP_SIZE);
	for(;;){
		node_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(node[node_ptr].type == BLOCK_AIR)
			return 0;
		if(node[node_ptr].type != BLOCK_PARENT)
			return node[node_ptr].type;
		vec3mul(&pos,2.0f);
	}
}

uint32_t getNode(int x,int y,int z,int depth){
	node_t* node = dynamicArrayGet(node_root,0);
	uint32_t node_ptr = 0;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		node_ptr = node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x];
		if(node[node_ptr].type != BLOCK_PARENT)
			return node_ptr;
	}
	return node_ptr;
}

void removeSubVoxel(uint32_t node_index){
	node_t* node = dynamicArrayGet(node_root,node_index);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 8;i++)
			removeSubVoxel(node->child_s[i]);
		return;
	}
	dynamicArrayRemove(&node_root,node_index);
	if(node->type == BLOCK_AIR){
		dynamicArrayRemove(&air_array,node->index);
		return;
	}
	if(material_array[node->type].flags & MAT_POWER)
		removePowerGrid(node_index);
	dynamicArrayRemove(&block_array,node->index);
}

void removeVoxel(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	node_t* parent = dynamicArrayGet(node_root,node->parent);
	if(node->type == BLOCK_PARENT){
		node->index = dynamicArrayTop(air_array);
		node->type = BLOCK_AIR;
		air_t air;
		air.entity = -1;
		air.o2 = 0.3f;
		dynamicArrayAdd(&air_array,&air);
		bool exit = false;
		for(int i = 0;i < 8;i++){
			node_t* child = dynamicArrayGet(node_root,parent->child_s[i]);
			if(child->type == BLOCK_AIR)
				continue;
			exit = true;
		}
		if(!exit){
			removeVoxel(node->parent);
			return;
		}
		for(int i = 0;i < 8;i++)
			removeSubVoxel(node->child_s[i]);
		return;
	}
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(material_array[node->type].flags & MAT_POWER)
		removePowerGrid(node_ptr);
	for(int i = 0;i < 6;i++){
		if(!block->luminance[i])
			continue;
		tFree(block->luminance[i]);
	}
	node->index = dynamicArrayTop(air_array);
	node->type = BLOCK_AIR;
	air_t air;
	air.entity = -1;
	air.o2 = 0.3f;
	dynamicArrayAdd(&air_array,&air);
	if(!parent->depth)
		return;
	bool exit = false;
	for(int i = 0;i < 8;i++){
		node_t* child = dynamicArrayGet(node_root,parent->child_s[i]);
		if(child->type == BLOCK_AIR)
			continue;
		exit = true;
	}
	if(exit)
		return;
	removeVoxel(node->parent);
}

void setVoxel(uint32_t x,uint32_t y,uint32_t z,int depth,uint32_t material,float ammount){
	node_t* node = node_root.data;
	uint32_t* node_index = 0;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		uint32_t child_index = node->child[sub_coord.z][sub_coord.y][sub_coord.x];
		if(child_index){
			node_index = child_index;
			node = dynamicArrayGet(node_root,child_index);
			continue;
		}
		for(int j = 0;j < 8;j++){
			node_t node_new;
			node_new.parent = node_index;
			node_new.type = BLOCK_AIR;
			node_new.pos = (ivec3){
				(x >> i + 1 << 1) + (j >> 0 & 1),
				(y >> i + 1 << 1) + (j >> 1 & 1),
				(z >> i + 1 << 1) + (j >> 2 & 1)
			};
			node_new.depth = node->depth + 1;
			for(int k = 0;k < 8;k++)
				node_new.child_s[k] = 0;
			air_t air;
			air.entity = -1;
			air.o2 = 0.3f;
			air.node = node_root.size;
			air.luminance = VEC3_ZERO;
			node_new.index = dynamicArrayTop(air_array);
			dynamicArrayAdd(&air_array,&air);
			node->child_s[j] = dynamicArrayTop(node_root);
			dynamicArrayAdd(&node_root,&node_new);
			node = dynamicArrayGet(node_root,node_index);
			if(j != sub_coord.z * 4 + sub_coord.y * 2 + sub_coord.x)
				continue;
			child_index = node->child_s[j];
			if(node->type < BLOCK_PARENT && material_array[node->type].flags & MAT_LIQUID){
				block_t* block = dynamicArrayGet(block_array,node->index);
				float ammount = block->ammount + block->ammount_buffer;
				if(node[child_index].pos.z)
					ammount -= 0.5f;
				ammount *= 2.0f;
				if(ammount > 0.0f){
					block_t block_new = {0};
					block_new.ammount_buffer = tMinf(ammount,1.0f);
					node[child_index].type = node->type;
					node[child_index].index = block_array.size;
					dynamicArrayAdd(&block_array,&block_new);
					continue;
				}
			}
		}
		node->type = BLOCK_PARENT;	
		node_index = child_index;
		node = dynamicArrayGet(node_root,child_index);
	}
	for(;;){
		uint32_t parent_ptr = node->parent;
		node_t* parent = dynamicArrayGet(node_root,parent_ptr);
		float ammount_add = 0.0f;
		for(int i = 0;i < 8;i++){
			uint32_t child_ptr = parent->child_s[i];
			if(!(material_array[material].flags & MAT_LIQUID)){
				node_t* child = dynamicArrayGet(node_root,child_ptr);
				if(child_ptr == node_index || child->type == material)
					continue;
			}
			if(node->type == BLOCK_PARENT){
				for(int j = 0;j < 8;j++){
					removeSubVoxel(node->child_s[j]);
					node->child_s[j] = 0;
				}
			}
			if(node->type == BLOCK_AIR)
				dynamicArrayRemove(&air_array,node->index);
			block_t block;
			if(material_array[material].flags & MAT_LIQUID){
				block.ammount_buffer = ammount;
				block.ammount = 0.0f;
				block.disturbance = 0.005f;
			}
			block.node = node_index;
			block.power = 0.0f;
			for(int j = 0;j < 6;j++)
				block.luminance[j] = 0;
			node->index = dynamicArrayTop(block_array);
			node->type = material;
			dynamicArrayAdd(&block_array,&block);
			if(material_array[material].flags & MAT_POWER)
				applyPowerGrid(node_index);
			return;
		}
		if(!node->depth)
			return;
		node = parent;
	}
}

void setVoxelSolid(uint32_t x,uint32_t y,uint32_t z,uint32_t depth,uint32_t material){
	node_t* node = dynamicArrayGet(node_root,0);
	uint32_t node_ptr = 0;
	/*
	for(int j = 0;j < 6;j++){
		uint32_t dx = (uint32_t[]){1,-1,0,0,0,0}[j];
		uint32_t dy = (uint32_t[]){0,0,1,-1,0,0}[j];
		uint32_t dz = (uint32_t[]){0,0,0,0,1,-1}[j];
		uint32_t neighbour = getNode(x + dx,y + dy,z + dz,depth);
		if(neighbour && node_root[neighbour].block){
			block_t* block = node_root[neighbour].block;
			if(material_array[block->material].flags & MAT_LIQUID)
				continue;
			for(int k = 0;k < 6;k++){
				tFree(block->side[k].luminance_p);
				block->side[k].luminance_p = 0;
			}
		}
	}
	*/
	setVoxel(x,y,z,depth,material,0.0f);
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
	uint32_t p = ray->side.y < ray->side.x;
	ray->square_pos.a[p] += ray->step.a[p];
	ray->side.a[p] += ray->delta.a[p];
	ray->square_side = p;
}

node_hit_t treeRayI(ray3i_t ray,uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,0);
	ray3iItterate(&ray);
	for(;;){
		uint32_t combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
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
		uint32_t child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(!child){
			ray3iItterate(&ray);
			continue;
		}
		if(node[child].type < BLOCK_PARENT)
			return (node_hit_t){child,ray.square_side};

		uvec3 pre = ray.pos;
		uint32_t x = (uint32_t[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint32_t y = (uint32_t[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		long long side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];

		ray.pos.a[x] = (ray.pos.a[x] + (side_delta * ray.dir.a[x] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
		ray.pos.a[y] = (ray.pos.a[y] + (side_delta * ray.dir.a[y] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
		ray.pos.a[ray.square_side] = ray.dir.a[ray.square_side] >> 31 & (2 << FIXED_PRECISION) - 1;
		node_ptr = child;
		recalculateRayI(&ray);
		goto yeet;
	}
}

uint32_t traverseTreeItt(ray3i_t ray,uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,0);
	ray3iItterate(&ray);
	uint32_t i = 0;
	for(;;){
		uint32_t combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(!node[node_ptr].depth)
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
		uint32_t child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].type == BLOCK_AIR){
			i++;
			ray3iItterate(&ray);
			continue;
		}
		if(node[child].type == BLOCK_PARENT){
			uvec3 pre = ray.pos;
			uint32_t x = (uint32_t[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
			uint32_t y = (uint32_t[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

			int64_t side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];

			ray.pos.a[x] = (ray.pos.a[x] + (side_delta * ray.dir.a[x] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
			ray.pos.a[y] = (ray.pos.a[y] + (side_delta * ray.dir.a[y] >> FIXED_PRECISION) & (1 << FIXED_PRECISION) - 1) << 1;
			ray.pos.a[ray.square_side] = ray.dir.a[ray.square_side] >> 31 & (2 << FIXED_PRECISION) - 1;
			
			node_ptr = child;
			recalculateRayI(&ray);
			goto yeet;
		}
		return i;
	}
}
#define O2_COLOR (vec3_t){0.9f,0.9f,0.9f}
//#define O2_COLOR (vec3_t){0.3f,0.6f,1.0f}

fog_t treeRayFog(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos,float max_distance){
	fog_t fog = {0.0f,0.0f,0.0f,0.0f};
	fog_t fog_buffer = {0.0f,0.0f,0.0f,0.0f};
	float distance_buf = 0.0f;
	node_t* node = dynamicArrayGet(node_root,0);
	for(;;){
		uint32_t combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(!node_ptr)
				return fog;
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3_t){node[node_ptr].pos.x & 1,node[node_ptr].pos.y & 1,node[node_ptr].pos.z & 1});
			
			recalculateRay(&ray);
			node_ptr = node[node_ptr].parent;
			ray3Itterate(&ray);
			continue;
		}
		uint32_t child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].type == BLOCK_AIR){
			air_t* air = dynamicArrayGet(air_array,node[child].index);
			float distance = rayNodeGetDistance(ray_pos,node[child].pos,node[child].depth,ray.dir,ray.square_side);
			fog_t fog_prebuf;
			fog_prebuf.luminance = vec3mulvec3R(air->luminance,O2_COLOR);
			fog_prebuf.density = air->o2;
			if(distance > max_distance){
				fog_buffer.density *= max_distance - distance_buf;
				float p_dst_1 = 1.0f / (fog.density + 1.0f);
				float p_dst_2 = 1.0f / (fog.density + fog_buffer.density + 1.0f);
				vec3addvec3(&fog.luminance,vec3mulR(fog_buffer.luminance,(p_dst_1 - p_dst_2)));
				fog.density += fog_buffer.density;
				return fog;
			}
			fog_buffer.density *= distance - distance_buf;
			if(distance < 0.0f){
				fog_buffer = fog_prebuf;
				ray3Itterate(&ray);
				continue;
			}
			float p_dst_1 = 1.0f / (fog.density + 1.0f);
			float p_dst_2 = 1.0f / (fog.density + fog_buffer.density + 1.0f);
			vec3addvec3(&fog.luminance,vec3mulR(fog_buffer.luminance,(p_dst_1 - p_dst_2)));
			fog.density += fog_buffer.density;
			distance_buf = distance;
			fog_buffer = fog_prebuf;
			ray3Itterate(&ray);
			continue;
		}
		if(node[child].type != BLOCK_PARENT){
			float distance = rayNodeGetDistance(ray_pos,node[child].pos,node[child].depth,ray.dir,ray.square_side);
			if(distance > max_distance){
				fog_buffer.density *= max_distance - distance_buf;
				float p_dst_1 = 1.0f / (fog.density + 1.0f);
				float p_dst_2 = 1.0f / (fog.density + fog_buffer.density + 1.0f);
				vec3addvec3(&fog.luminance,vec3mulR(fog_buffer.luminance,(p_dst_1 - p_dst_2)));
				fog.density += fog_buffer.density;
				return fog;
			}
			ray3Itterate(&ray);
			continue;
		}

		uint32_t x = (uint32_t[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint32_t y = (uint32_t[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
		vec3_t pre = ray.pos;
		ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
		
		node_ptr = child;
		recalculateRay(&ray);
	}
}

node_hit_t treeRayOnce(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos){
	bool small_x = ray.dir.x < 0.0001f && ray.dir.x > -0.0001f;
	bool small_y = ray.dir.y < 0.0001f && ray.dir.y > -0.0001f;
	bool small_z = ray.dir.z < 0.0001f && ray.dir.z > -0.0001f;
	if(small_x || small_y || small_z)
		return (node_hit_t){.node = 0,.side = ray.square_side};		
	node_t* node = node_root.data;
	uint32_t material_id = node->type;
	ray3Itterate(&ray);
	for(;;){
		uint32_t combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(!node[node_ptr].depth)
				return (node_hit_t){.node = 0,.side = ray.square_side};
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3_t){node[node_ptr].pos.x & 1,node[node_ptr].pos.y & 1,node[node_ptr].pos.z & 1});
			
			recalculateRay(&ray);
			node_ptr = node[node_ptr].parent;
			ray3Itterate(&ray);
			continue;
		}
		uint32_t child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].type == BLOCK_AIR){
			return (node_hit_t){child,.side = ray.square_side};
		}
		if(node[child].type != BLOCK_PARENT){
			return (node_hit_t){child,ray.square_side};
		}

		uint32_t x = (uint32_t[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint32_t y = (uint32_t[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
		vec3_t pre = ray.pos;
		ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
		
		node_ptr = child;
		recalculateRay(&ray);
	}
}

node_hit_t treeRay(ray3_t ray,uint32_t node_ptr,vec3_t ray_pos){
	bool small_x = ray.dir.x < 0.0001f && ray.dir.x > -0.0001f;
	bool small_y = ray.dir.y < 0.0001f && ray.dir.y > -0.0001f;
	bool small_z = ray.dir.z < 0.0001f && ray.dir.z > -0.0001f;
	if(small_x || small_y || small_z)
		return (node_hit_t){.node = 0,.side = ray.square_side};		
	node_t* node = node_root.data;
	ray3Itterate(&ray);
	for(;;){
		uint32_t combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(!node[node_ptr].depth)
				return (node_hit_t){.node = 0,.side = ray.square_side};
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3_t){node[node_ptr].pos.x & 1,node[node_ptr].pos.y & 1,node[node_ptr].pos.z & 1});
			recalculateRay(&ray);
			node_ptr = node[node_ptr].parent;
			ray3Itterate(&ray);
			continue;
		}
		uint32_t child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].type == BLOCK_AIR){
			ray3Itterate(&ray);
			continue;
		}
		if(node[child].type == BLOCK_PARENT){
			uint32_t x = (uint32_t[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
			uint32_t y = (uint32_t[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

			float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
			vec3_t pre = ray.pos;
			ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
			ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
			ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
			node_ptr = child;
			recalculateRay(&ray);
			continue;
		}
		if(material_array[node[child].type].flags & MAT_LIQUID){
			ray3Itterate(&ray);
			continue;
		}
		if(node[child].type == BLOCK_SPHERE){
			float radius = getBlockSize(node[child].depth);
			vec3_t pos = {node[child].pos.x,node[child].pos.y,node[child].pos.z};
			vec3mul(&pos,radius);
			vec3add(&pos,radius * 0.5f);
			if(rayIntersectSphere(ray_pos,pos,ray.dir,radius) == -1.0f){
				ray3Itterate(&ray);
				continue;
			}
		}
		return (node_hit_t){child,ray.square_side};
	}
}