#include "tree.h"
#include "tmath.h"
#include "memory.h"
#include "lighting.h"

block_node_t block_node_root;

traverse_init_t initTraverse(vec3 pos){
	block_node_t* node = &block_node_root;
	for(;;){
		block_node_t* child = node->child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!child)
			break;
		node = child;
		vec3mul(&pos,2.0f);
	}
	pos.x = tFractUnsigned(pos.x * 0.5f) * 2.0f;
	pos.y = tFractUnsigned(pos.y * 0.5f) * 2.0f;
	pos.z = tFractUnsigned(pos.z * 0.5f) * 2.0f;
	return (traverse_init_t){pos,node};
}

block_t* insideBlock(vec3 pos){
	block_node_t* node = &block_node_root;
	vec3div(&pos,MAP_SIZE);
	for(;;){
		if(!node->child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1])
			return 0;
		node = node->child[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(node->block)
			return node->block;
		vec3mul(&pos,2.0f);
	}
}

block_node_t* getNode(int x,int y,int z,int depth){
	block_node_t* node = &block_node_root;	
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		if(!node->child[sub_coord.z][sub_coord.y][sub_coord.x])
			return node->block ? node : 0;

		node = node->child[sub_coord.z][sub_coord.y][sub_coord.x];
	}
	return node;
}

void removeVoxel(block_node_t** node){
	block_node_t** parent = &(*node)->parent;
	block_node_t* node_buffer = *node;
	if((*node)->block){
		MFREE((*node)->block);
		(*node)->block = 0;
	}
	for(int i = 0;i < 8;i++){
		if((*node)->parent->child_s[i] == *node){
			(*node)->parent->child_s[i] = 0;
			break;
		}
	}
	MFREE(*node);
	*node = 0;
	for(int i = 0;i < 8;i++)
		if((*parent)->child_s[i] && (*parent)->child_s[i] != node_buffer)
			return;
	removeVoxel(parent);
}

static void removeSubVoxel(block_node_t* node){
	for(int i = 0;i < 8;i++){
		if(!node->child_s[i])
			continue;
		removeSubVoxel(node->child_s[i]);
		MFREE(node->child_s[i]);
		node->child_s[i] = 0;
	}
}

void setVoxel(int x,int y,int z,int depth){
	block_node_t* node = &block_node_root;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		if(!node->child[sub_coord.z][sub_coord.y][sub_coord.x]){
			node->child[sub_coord.z][sub_coord.y][sub_coord.x] = MALLOC_ZERO(sizeof(block_node_t));
			node->child[sub_coord.z][sub_coord.y][sub_coord.x]->parent = node;
			node->child[sub_coord.z][sub_coord.y][sub_coord.x]->pos = sub_coord;
		}
		node = node->child[sub_coord.z][sub_coord.y][sub_coord.x];
	}
	for(;;){
		for(int i = 0;i < 8;i++){
			if(node->parent->child_s[i] && node->parent->child_s[i]->block || node->parent->child_s[i] == node)
				continue;
			removeSubVoxel(node);
			block_t* block = MALLOC_ZERO(sizeof(block_t));
			block->depth = depth;
			block->pos = (ivec3){x,y,z};
			if(depth < LM_MAXDEPTH){
				int size = (1 << LM_MAXDEPTH - depth);
				for(int j = 0;j < 6;j++){
					block->side[j].luminance_p = MALLOC_ZERO(sizeof(vec3) * (size + 2) * (size + 2));
					block->side[j].luminance_update_priority_p = MALLOC_ZERO(sizeof(vec3) * size * size);
				}
			}
			node->block = block;
			return;
		}
		node = node->parent;
		x >>= 1;
		y >>= 1;
		z >>= 1;
		depth++;
	}
}
#include <stdio.h>
void setVoxelLM(int x,int y,int z,int depth,int material){
	block_node_t* node = &block_node_root;
	for(int i = depth - 1;i >= 0;i--){
		ivec3 sub_coord = {x >> i & 1,y >> i & 1,z >> i & 1};
		if(!node->child[sub_coord.z][sub_coord.y][sub_coord.x]){
			node->child[sub_coord.z][sub_coord.y][sub_coord.x] = MALLOC_ZERO(sizeof(block_node_t));
			node->child[sub_coord.z][sub_coord.y][sub_coord.x]->parent = node;
			node->child[sub_coord.z][sub_coord.y][sub_coord.x]->pos = sub_coord;
		}
		node = node->child[sub_coord.z][sub_coord.y][sub_coord.x];
	}
	for(;;){
		for(int i = 0;i < 8;i++){
			if(node->parent->child_s[i] && node->parent->child_s[i]->block || node->parent->child_s[i] == node)
				continue;
			removeSubVoxel(node);
			block_t* block = MALLOC_ZERO(sizeof(block_t));
			block->depth = depth;
			block->material = material;
			block->pos = (ivec3){x,y,z};
			if(material_array[material].flags & MAT_LUMINANT){
				for(int j = 0;j < 6;j++)
					block->side[j].luminance = material_array[material].luminance;
			}
			else{
				if(depth < LM_MAXDEPTH){
					int size = (1 << LM_MAXDEPTH - depth);
					for(int j = 0;j < 6;j++){
						block->side[j].luminance_p = MALLOC_ZERO(sizeof(vec3) * (size + 2) * (size + 2));
						block->side[j].luminance_update_priority_p = MALLOC_ZERO(sizeof(vec3) * size * size);
						vec3 pos = {block->pos.x,block->pos.y,block->pos.z};
						setLightSideBig(block->side[j].luminance_p,pos,j,block->depth);
					}
				}
				else
					for(int j = 0;j < 6;j++)
						setLightSide(&block->side[j].luminance,(vec3){x,y,z},j,depth);
			}
			node->block = block;
			for(int nx = -1;nx < 2;nx += 2){
				for(int ny = -1;ny < 2;ny += 2){
					for(int nz = -1;nz < 2;nz += 2){
						if(x + nx < 0 || y + ny < 0 || z + nz < 0)
							continue;
						block_node_t* neighbour = getNode(x + nx,y + ny,z + nz,depth);
						if(!neighbour)
							continue;
						block = neighbour->block;
						if(!block)
							continue;
						vec3 pos = {block->pos.x,block->pos.y,block->pos.z};
						if(block->depth < LM_MAXDEPTH){
							for(int j = 0;j < 6;j++)
								setLightSideBig(block->side[j].luminance_p,pos,j,block->depth);

							continue;
						}
						for(int j = 0;j < 6;j++)
							setLightSide(&block->side[j].luminance,pos,j,block->depth);
					}
				}
			}
			return;
		}
		node = node->parent;
		x >>= 1;
		y >>= 1;
		z >>= 1;
		depth--;
	}
}

void recalculateRay(ray3_t* ray){
	vec3 fract_pos;
	fract_pos.x = tFractUnsigned(ray->pos.x);
	fract_pos.y = tFractUnsigned(ray->pos.y);
	fract_pos.z = tFractUnsigned(ray->pos.z);

	ray->side.x = (ray->dir.x < 0.0f ? fract_pos.x : 1.0f - fract_pos.x) * ray->delta.x;
	ray->side.y = (ray->dir.y < 0.0f ? fract_pos.y : 1.0f - fract_pos.y) * ray->delta.y;
	ray->side.z = (ray->dir.z < 0.0f ? fract_pos.z : 1.0f - fract_pos.z) * ray->delta.z;

	ray->square_pos.x = ray->pos.x;
	ray->square_pos.y = ray->pos.y;
	ray->square_pos.z = ray->pos.z;
}

nodepointer_hit_t traverseTreeNode(ray3_t ray,block_node_t* node){
	ray3Itterate(&ray);
	for(;;){
		int combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		bool bound = combine > 1 || combine < 0;
		if(bound){
			if(!node->parent)
				return (nodepointer_hit_t){0};
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3){node->pos.x,node->pos.y,node->pos.z});
			
			recalculateRay(&ray);
			node = node->parent;
			ray3Itterate(&ray);
			continue;
		}
		ivec3 pos = ray.square_pos;
		block_node_t* child = node->child[pos.z][pos.y][pos.x];
		if(!child){
			ray3Itterate(&ray);
			continue;
		}
		if(child->block)
			return (nodepointer_hit_t){&node->child[pos.z][pos.y][pos.x],ray.square_side};
		node = child;

		int x_table[] = {VEC3_Y,VEC3_X,VEC3_X};
		int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};

		int x = x_table[ray.square_side];
		int y = y_table[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];

		ray.pos.a[x] = tFract(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFract(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * 1.999998f;

		recalculateRay(&ray);
	}
}

blockray_t traverseTree(ray3_t ray,block_node_t* node){
	ray3Itterate(&ray);
	for(;;){
		int combine = ray.square_pos.x | ray.square_pos.y | ray.square_pos.z;
		bool bound = combine > 1 || combine < 0;
		if(bound){
			if(!node->parent)
				return (blockray_t){0};
			vec3mul(&ray.pos,0.5f);
			vec3addvec3(&ray.pos,(vec3){node->pos.x,node->pos.y,node->pos.z});
			
			recalculateRay(&ray);
			node = node->parent;
			ray3Itterate(&ray);
			continue;
		}
		ivec3 pos = ray.square_pos;
		block_node_t* child = node->child[pos.z][pos.y][pos.x];
		if(!child){
			ray3Itterate(&ray);
			continue;
		}
		if(child->block)
			return (blockray_t){child,ray.square_side};
		node = child;

		int x_table[3] = {VEC3_Y,VEC3_X,VEC3_X};
		int y_table[3] = {VEC3_Z,VEC3_Z,VEC3_Y};

		int x = x_table[ray.square_side];
		int y = y_table[ray.square_side];

		float side_delta = ray.side.a[ray.square_side] - ray.delta.a[ray.square_side];

		ray.pos.a[x] = tFract(ray.pos.a[x] + side_delta * ray.dir.a[x]) * 2.0f;
		ray.pos.a[y] = tFract(ray.pos.a[y] + side_delta * ray.dir.a[y]) * 2.0f;
		ray.pos.a[ray.square_side] = (ray.dir.a[ray.square_side] < 0.0f) * 1.999998f;

		recalculateRay(&ray);
	}
}