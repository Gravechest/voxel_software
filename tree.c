#include "tree.h"
#include "tmath.h"
#include "memory.h"
#include "lighting.h"
#include "fog.h"
#include "powergrid.h"

uint block_node_c;
node_t* node_root;

float rayNodeGetDistance(vec3 ray_pos,ivec3 pos,int depth,vec3 angle,int side){
	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << depth) * 2.0f;
	block_pos.x = pos.x * block_size;
	block_pos.y = pos.y * block_size;
	block_pos.z = pos.z * block_size;
	plane_t plane = getPlane(block_pos,angle,side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,ray_pos);
	return rayIntersectPlane(plane_pos,angle,plane.normal);
}

float rayGetDistance(vec3 ray_pos,vec3 ray_dir){
	traverse_init_t init = initTraverse(ray_pos);
	node_hit_t result = treeRay(ray3Create(init.pos,ray_dir),init.node,ray_pos);
	if(!result.node)
		return 999999.0f;
	node_t node_hit = node_root[result.node];
	return rayNodeGetDistance(ray_pos,node_hit.pos,node_hit.depth,ray_dir,result.side);
}

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
	node_t* node = node_root;
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

node_t treeTraverse(vec3 pos){
	vec3div(&pos,MAP_SIZE);
	node_t* node = node_root;
	uint node_ptr = 0;
	for(;;){
		uint child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!child_ptr)
			return node_root[node_ptr]; 
		node_ptr = child_ptr;
		vec3mul(&pos,2.0f);
	}
}

traverse_init_t initTraverse(vec3 pos){
	vec3div(&pos,MAP_SIZE);
	node_t* node = node_root;
	uint node_ptr = 0;
	for(;;){
		uint child_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(node[child_ptr].air || node[child_ptr].block)
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
	node_t* node = node_root;
	uint node_ptr = 0;
	vec3div(&pos,MAP_SIZE);
	for(;;){
		node_ptr = node[node_ptr].child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(node[node_ptr].air)
			return 0;
		if(node[node_ptr].block)
			return node[node_ptr].block;
		vec3mul(&pos,2.0f);
	}
}

uint getNode(int x,int y,int z,int depth){
	node_t* node = node_root;
	uint node_ptr = 0;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		uint child_ptr = node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x];
		node_ptr = node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x];
		if(node[node_ptr].air || node[node_ptr].block)
			return node_ptr;
	}
	return node_ptr;
}

void removeVoxel(uint node_ptr){
	node_t* node = &node_root[node_ptr];
	if(node->block){
		if(material_array[node->block->material].flags & MAT_POWER)
			removePowerGrid(node_ptr);
		if(material_array[node->block->material].flags & MAT_LIQUID){

		}
		else{
			for(int i = 0;i < 6;i++){
				if(!node->block->side[i].luminance_p)
					continue;
				MFREE(node->block->side[i].luminance_p);
			}
		}
		MFREE(node->block);
		node->block = 0;
		node->air = MALLOC_ZERO(sizeof(air_t));
		node->air->entity = -1;
	}
	else if(node->air){
		MFREE(node->air);
		node->air = 0;
	}
	else{
		for(int i = 0;i < 8;i++){
			MFREE(node_root[node->child_s[i]].air);
			node_root[node->child_s[i]].air = 0;
			node->child_s[i] = 0;
			node->air = MALLOC_ZERO(sizeof(air_t));
		}
	}
	if(!node_root[node->parent].depth)
		return;
	bool exit = false;
	for(int i = 0;i < 8;i++){
		if(node_root[node_root[node->parent].child_s[i]].air)
			continue;
		exit = true;
	}
	if(exit)
		return;
	removeVoxel(node->parent);
}
	
static uint stack_ptr;
static uint stack[240000];

void removeSubVoxel(uint node_ptr){
	node_t* node = &node_root[node_ptr];
	if(node->block){
		if(material_array[node->block->material].flags & MAT_POWER)
			removePowerGrid(node_ptr);
		else{
			for(int i = 0;i < 6;i++){
				if(!node->block->side[i].luminance_p)
					continue;
				MFREE(node->block->side[i].luminance_p);
			}
		}
		MFREE(node->block);
		memset(node,0,sizeof(node_t));
		stack[stack_ptr++] = node_ptr;
		return;
	}
	if(node->air){
		MFREE(node->air);
		memset(node,0,sizeof(node_t));
		stack[stack_ptr++] = node_ptr;
		return;
	}
	for(int i = 0;i < 8;i++){
		if(!node->child_s[i])
			continue;
		removeSubVoxel(node->child_s[i]);
	}
	memset(node,0,sizeof(node_t));
	stack[stack_ptr++] = node_ptr;
}

void setVoxel(uint x,uint y,uint z,uint depth,uint material,float ammount){
	node_t* node = node_root;
	uint node_ptr = 0;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		uint* child = &node[node_ptr].child[sub_coord.z][sub_coord.y][sub_coord.x];
		if(*child){
			node_ptr = *child;
			continue;
		}
		for(int j = 0;j < 8;j++){
			uint* node_new = &node[node_ptr].child_s[j];
			*node_new = stack_ptr ? stack[--stack_ptr] : ++block_node_c;
			node[*node_new].parent = node_ptr;
			node[*node_new].pos = (ivec3){
				(x >> i + 1 << 1) + (j >> 0 & 1),
				(y >> i + 1 << 1) + (j >> 1 & 1),
				(z >> i + 1 << 1) + (j >> 2 & 1)
			};
			node[*node_new].depth = depth - i;
			if(j == sub_coord.z * 4 + sub_coord.y * 2 + sub_coord.x)
				continue;
			if(node[node_ptr].block && material_array[node[node_ptr].block->material].flags & MAT_LIQUID){
				float ammount = node[node_ptr].block->ammount + node[node_ptr].block->ammount_buffer;
				if(node[*node_new].pos.z)
					ammount -= 0.5f;
				ammount *= 2.0f;
				if(ammount > 0.0f){
					node[*node_new].block = MALLOC_ZERO(sizeof(block_t));
					node[*node_new].block->ammount_buffer = tMinf(ammount,1.0f);
					node[*node_new].block->material = node[node_ptr].block->material;
					continue;
				}
			}
			node[*node_new].air = MALLOC_ZERO(sizeof(air_t));
			node[*node_new].air->entity = -1;
		}
		if(node[node_ptr].air){
			MFREE(node[node_ptr].air);
			node[node_ptr].air = 0;
		}
		if(node[node_ptr].block){
			MFREE(node[node_ptr].block);
			node[node_ptr].block = 0;
		}
		node_ptr = *child;
	}
	for(;;){
		uint parent_ptr = node[node_ptr].parent;
		float ammount_add = 0.0f;
		for(int i = 0;i < 8;i++){
			uint child_ptr = node[parent_ptr].child_s[i];
			if(!(material_array[material].flags & MAT_LIQUID)){
				if(child_ptr == node_ptr)
					continue;
				if(node[child_ptr].block && node[child_ptr].block->material == material)
					continue;
			}
			for(int j = 0;j < 8;j++){
				if(!node[node_ptr].child_s[j])
					continue;
				removeSubVoxel(node[node_ptr].child_s[j]);
				node[node_ptr].child_s[j] = 0;
			}
			if(node[node_ptr].block){
				MFREE(node[node_ptr].block);
				node[node_ptr].block = 0;
			}
			if(node[node_ptr].air){
				MFREE(node[node_ptr].air);
				node[node_ptr].air = 0;
			}
			block_t* block = MALLOC_ZERO(sizeof(block_t));
			block->material = material;
			if(material_array[material].flags & MAT_LIQUID){
				block->ammount_buffer = ammount;
				block->ammount = 0.0f;
				block->disturbance = 0.005f;
			}
			node[node_ptr].block = block;
			if(material_array[material].flags & MAT_POWER)
				applyPowerGrid(node_ptr);
			return;
		}
		node_ptr = node[node_ptr].parent;
	}
}

void setVoxelSolid(uint x,uint y,uint z,uint depth,uint material){
	node_t* node = node_root;
	uint node_ptr = 0;
	/*
	for(int j = 0;j < 6;j++){
		uint dx = (uint[]){1,-1,0,0,0,0}[j];
		uint dy = (uint[]){0,0,1,-1,0,0}[j];
		uint dz = (uint[]){0,0,0,0,1,-1}[j];
		uint neighbour = getNode(x + dx,y + dy,z + dz,depth);
		if(neighbour && node_root[neighbour].block){
			block_t* block = node_root[neighbour].block;
			if(material_array[block->material].flags & MAT_LIQUID)
				continue;
			for(int k = 0;k < 6;k++){
				MFREE(block->side[k].luminance_p);
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
	uint p = ray->side.y < ray->side.x;
	ray->square_pos.a[p] += ray->step.a[p];
	ray->side.a[p] += ray->delta.a[p];
	ray->square_side = p;
}

node_hit_t treeRayI(ray3i_t ray,uint node_ptr){
	node_t* node = node_root;
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
		node_ptr = child;
		recalculateRayI(&ray);
		goto yeet;
	}
}

uint traverseTreeItt(ray3i_t ray,uint node_ptr){
	node_t* node = node_root;
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
		if(node[child].air){
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
			
		node_ptr = child;
		recalculateRayI(&ray);
		goto yeet;
	}
}

#define O2_COLOR (vec3){0.3f,0.6f,1.0f}

fog_t treeRayFog(ray3_t ray,uint node_ptr,vec3 ray_pos,float max_distance){
	fog_t fog = {0};
	fog_t fog_buffer = {0};
	float distance_buf = 0.0f;
	node_t* node = node_root;
	for(;;){
		uint combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(!node_ptr)
				return fog;
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3){node[node_ptr].pos.x & 1,node[node_ptr].pos.y & 1,node[node_ptr].pos.z & 1});
			
			recalculateRay(&ray);
			node_ptr = node[node_ptr].parent;
			ray3Itterate(&ray);
			continue;
		}
		uint child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].air){
			float distance = rayNodeGetDistance(ray_pos,node[child].pos,node[child].depth,ray.dir,ray.square_side);
			fog_t fog_prebuf;
			fog_prebuf.luminance = vec3mulvec3R(node[child].air->luminance,O2_COLOR);
			fog_prebuf.density = node[child].air->o2;
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
		if(node[child].block){
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

		uint x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
		vec3 pre = ray.pos;
		ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
		
		node_ptr = child;
		recalculateRay(&ray);
	}
}

node_hit_t treeRayOnce(ray3_t ray,uint node_ptr,vec3 ray_pos){
	bool small_x = ray.dir.x < 0.0001f && ray.dir.x > -0.0001f;
	bool small_y = ray.dir.y < 0.0001f && ray.dir.y > -0.0001f;
	bool small_z = ray.dir.z < 0.0001f && ray.dir.z > -0.0001f;
	if(small_x || small_y || small_z)
		return (node_hit_t){.node = 0,.side = ray.square_side};		
	node_t* node = node_root;
	uint material_id = node->block->material;
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
		uint child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].air){
			return (node_hit_t){child,.side = ray.square_side};
		}
		if(node[child].block){
			return (node_hit_t){child,ray.square_side};
		}

		uint x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
		vec3 pre = ray.pos;
		ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
		
		node_ptr = child;
		recalculateRay(&ray);
	}
}

node_hit_t treeRay(ray3_t ray,uint node_ptr,vec3 ray_pos){
	bool small_x = ray.dir.x < 0.0001f && ray.dir.x > -0.0001f;
	bool small_y = ray.dir.y < 0.0001f && ray.dir.y > -0.0001f;
	bool small_z = ray.dir.z < 0.0001f && ray.dir.z > -0.0001f;
	if(small_x || small_y || small_z)
		return (node_hit_t){.node = 0,.side = ray.square_side};		
	node_t* node = node_root;
	ray3Itterate(&ray);
	for(;;){
		uint combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		if(combine > 1){
			if(!node[node_ptr].depth)
				return (node_hit_t){.node = 0,.side = ray.square_side};
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3){node[node_ptr].pos.x & 1,node[node_ptr].pos.y & 1,node[node_ptr].pos.z & 1});
			
			recalculateRay(&ray);
			node_ptr = node[node_ptr].parent;
			ray3Itterate(&ray);
			continue;
		}
		uint child = node[node_ptr].child[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
		if(node[child].air){
			ray3Itterate(&ray);
			continue;
		}
		if(node[child].block){
			if(material_array[node[child].block->material].flags & MAT_LIQUID){
				ray3Itterate(&ray);
				continue;
			}
			return (node_hit_t){child,ray.square_side};
		}

		uint x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[ray.square_side];
		uint y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];
		vec3 pre = ray.pos;
		ray.pos.a[x] = tFractUnsigned(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFractUnsigned(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * TREE_ALMOST_TWO;
		
		node_ptr = child;
		recalculateRay(&ray);
	}
}