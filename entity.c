#include "entity.h"
#include "physics.h"
#include "tree.h"
#include "tmath.h"

entity_t entity_array[ENTITY_AMMOUNT];

entity_t* getNewEntity(){
	uint32_t entity_id = 0;
	for(;;entity_id++){
		if(!entity_array[entity_id].alive || entity_id == ENTITY_AMMOUNT - 1){
			entity_array[entity_id].alive = true;
			return &entity_array[entity_id];
		}
	}
}

void entityBehavior(entity_t* entity){
	if(!entity->alive)
		return;
	if(entity->flags & ENTITY_FLAG_FUSE){
		if(!entity->fuse--){
			entity->alive = false;
			return;
		}
	}
	vec3addvec3(&entity->pos,entity->dir);
	bool bound_x = entity->pos.x < 0.0f || entity->pos.x > MAP_SIZE * 2.0f;
	bool bound_y = entity->pos.y < 0.0f || entity->pos.y > MAP_SIZE * 2.0f;
	bool bound_z = entity->pos.z < 0.0f || entity->pos.z > MAP_SIZE * 2.0f;
	if(bound_x || bound_y || bound_z){
		entity->alive = false;
		return;
	}
	if(insideBlock(entity->pos)){
		vec3subvec3(&entity->pos,entity->dir);
		entity->dir = VEC3_ZERO;
	}
	if(entity->flags & ENTITY_FLAG_GRAVITY){
		entity->dir.z -= MV_GRAVITY;
	}
}
#include <stdio.h>
void entityTick(uint32_t tick_ammount){
	for(int i = 0;i < ENTITY_AMMOUNT;i++){
		if(!entity_array[i].alive)
			continue;
		node_t node = treeTraverse(entity_array[i].pos);
		if(node.type != BLOCK_AIR)
			continue;
		air_t* air = dynamicArrayGet(air_array,node.index);
		air->entity = -1;
	}
	for(int j = 0;j < tick_ammount;j++){
		for(int i = 0;i < ENTITY_AMMOUNT;i++){
			entityBehavior(&entity_array[i]);
		}
	}
	for(int i = 0;i < ENTITY_AMMOUNT;i++){
		if(!entity_array[i].alive)
			continue;
		node_t node = treeTraverse(entity_array[i].pos);
		if(node.type != BLOCK_AIR)
			continue;
		air_t* air = dynamicArrayGet(air_array,node.index);
		if(air->entity != -1)
			entity_array[i].next_entity = air->entity;
		else
			entity_array[i].next_entity = -1;
		air->entity = i;
	}
}

void trySpawnEnemy(){
	if(TRND1 < 0.9f)
		return;
	vec3_t pos = vec3addvec3R(camera.pos,VEC3_SINGLE((TRND1 - 0.5f) * 30.0f));
	traverse_init_t init = initTraverse(pos);
	node_t* node = dynamicArrayGet(node_root,init.node);
	if(node == BLOCK_AIR)
		return;
	vec3_t dir = {0.01f,0.01f,-1.0f};
	float distance = rayGetDistance(pos,dir);
	if(distance > 1.0f)
		return;
	dir = vec3normalizeR(vec3subvec3R(pos,camera.pos));
	distance = rayGetDistance(pos,dir);
	if(distance > vec3distance(pos,camera.pos))
		return;
	entity_t* entity = getNewEntity();
	entity->type = 3;
	entity->pos = pos;
	entity->color = (vec3_t){1.0f,1.0f,1.0f};
	entity->texture_pos = (vec2_t){0.0f,(1.0f / 2048) * (1024 + 80)};
	entity->texture_size = (vec2_t){(1.0f / (2048 * 3)) * 64,(1.0f / 2048) * 64};
	entity->size = 0.3f;
	entity->health = 100;
}