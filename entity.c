#include "entity.h"
#include "physics.h"
#include "tree.h"
#include "tmath.h"

entity_t entity_array[ENTITY_AMMOUNT];

entity_t* getNewEntity(){
	for(int entity_id = 0;entity_id < ENTITY_AMMOUNT;entity_id++){
		if(!entity_array[entity_id].alive){
			entity_array[entity_id].alive = true;
			return &entity_array[entity_id];
		}
	}
	return 0;
}

void spawnNumberParticle(vec3_t pos,uint32_t number){
	char str[8];
	if(!number)
		return;
	uint32_t length = 0;
	for(uint32_t i = number;i;i /= 10)
		length++;
	vec3_t v1 = getRayAngleCamera(0,window_size.y / 2);
	vec3_t v2 = getRayAngleCamera(window_size.x,window_size.y / 2);
	vec3_t direction = vec3cross(v1,v2);
	vec3subvec3(&pos,vec3mulR(direction,length * 0.05f));
	for(int i = 0;i < length;i++){
		char c = number % 10 + 0x30 - 0x21;
		int x = 9 - c / 10;
		int y = c % 10;

		float t_y = (1.0f / 2048) * (x * 8) + (1.0f / 2048.0f * 256.0f);
		float t_x = (1.0f / 2048) * y * 8;
		entity_t* entity = getNewEntity();
		entity->pos = pos;
		entity->dir = (vec3_t){0.0f,0.0f,0.001f};
		entity->size = 0.1f;
		entity->health = 300;
		entity->texture_pos = (vec2_t){t_x,t_y};
		entity->texture_size = (vec2_t){(1.0f / (2048)) * 8,(1.0f / (2048)) * 8};
		entity->color = (vec3_t){1.0f,0.2f,0.2f};
		entity->flags = ENTITY_FLAG_FUSE;
		number /= 10;
		vec3addvec3(&pos,vec3mulR(direction,0.1f));
	}
}
#include <stdio.h>
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
	bool z_collision = false;
	bool y_collision = false;
	bool x_collision = false;
	if(insideBlock(entity->pos)){
		z_collision = !insideBlock(vec3subvec3R(entity->pos,(vec3_t){0.0f,0.0f,entity->dir.z}));
		y_collision = !insideBlock(vec3subvec3R(entity->pos,(vec3_t){0.0f,entity->dir.y,0.0f}));
		x_collision = !insideBlock(vec3subvec3R(entity->pos,(vec3_t){entity->dir.x,0.0f,0.0f}));
		if(z_collision){
			entity->pos.z -= entity->dir.z;
			entity->dir.z = 0.0f;
		}
		if(y_collision){
			entity->pos.y -= entity->dir.y;
			entity->dir.y = 0.0f;
		}
		if(x_collision){
			entity->pos.x -= entity->dir.x;
			entity->dir.x = 0.0f;
		}
	}
	if(entity->flags & ENTITY_FLAG_GRAVITY || entity->flags & ENTITY_FLAG_ENEMY)
		entity->dir.z -= MV_GRAVITY;
	if(entity->flags & ENTITY_FLAG_ENEMY){
		if(z_collision && TRND1 < 0.005f){
			float distance = vec3distance(camera.pos,entity->pos);
			vec3_t rnd = {TRND1 - 0.5f,TRND1 - 0.5f,TRND1 - 0.5f};
			vec3_t offset = vec3addvec3R(camera.pos,vec3mulR(rnd,distance));
			entity->dir = vec3mulR(vec3subvec3R(offset,entity->pos),0.02f * TRND1);
			entity->dir.z += 0.02f * TRND1;
		}
	}
}

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
	return;
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
	entity->color = (vec3_t){0.4f,1.0f,0.4f};
	entity->texture_pos = (vec2_t){0.0f,0.0f};
	entity->texture_size = (vec2_t){0.01f,0.01f};
	entity->size = 0.3f;
	entity->health = 100;
	entity->flags = ENTITY_FLAG_ENEMY;
}