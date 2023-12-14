#include "entity.h"
#include "physics.h"
#include "tree.h"
#include "tmath.h"

entity_t entity_array[ENTITY_AMMOUNT];

entity_t* getNewEntity(){
	uint entity_id = 0;
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
	vec3addvec3(&entity->pos,entity->dir);
	bool bound_x = entity->pos.x < 0.0f || entity->pos.x > MAP_SIZE * 2.0f;
	bool bound_y = entity->pos.y < 0.0f || entity->pos.y > MAP_SIZE * 2.0f;
	bool bound_z = entity->pos.z < 0.0f || entity->pos.z > MAP_SIZE * 2.0f;
	if(bound_x || bound_y || bound_z){
		entity->alive = false;
		return;
	}
	switch(entity->type){
	case ENTITY_WATER:
		node_t node = treeTraverse(entity->pos);
		if(node.block){
			if(material_array[node.block->material].flags & MAT_LIQUID){
				float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
				float water_height = (node.pos.z + node.block->ammount) * block_size;
				if(entity->pos.z < water_height){
					vec3subvec3(&entity->pos,entity->dir);
					if(treeTraverse(entity->pos).block){
						float ammount;
						if(entity->block_depth > node.depth)
							ammount = entity->ammount / (1 << (entity->block_depth - node.depth) * 2);
						else
							ammount = entity->ammount * (1 << (node.depth - entity->block_depth) * 2);
						node.block->ammount_buffer += ammount;
						entity->alive = false;
						return;
					}
					goto water_new;
				}
				break;
			}
			vec3subvec3(&entity->pos,entity->dir);
		water_new:
			node_t node = treeTraverse(entity->pos);
			float ammount;
			if(entity->block_depth > node.depth)
				ammount = entity->ammount / (1 << (entity->block_depth - node.depth) * 2);
			else
				ammount = entity->ammount * (1 << (node.depth - entity->block_depth) * 2);
			setVoxel(node.pos.x,node.pos.y,node.pos.z,node.depth,entity->block_type,ammount);
			entity->alive = false;
			return;
		}
		entity->dir.z -= MV_GRAVITY;
		break;
	case 3:
		if(treeTraverse(entity->pos).block){
			vec3subvec3(&entity->pos,entity->dir);
			entity->dir = VEC3_ZERO;
			if(TRND1 < 0.01f)
				entity->dir = (vec3){(TRND1 - 0.5f) * 0.1f,(TRND1 - 0.5f) * 0.1f,TRND1 * 0.1f};
		}
		entity->dir.z -= MV_GRAVITY;
		break;
	case 2:
		entity->dir.z -= MV_GRAVITY;
		float distance_to_player = vec3distance(camera.pos,entity->pos);
		if(distance_to_player < 0.5f){
			entity->alive = false;
			for(int i = 2;i < 8;i++){
				if(inventory_slot[i].type == -1){
					inventory_slot[i].type = entity->block_type;
					inventory_slot[i].ammount = entity->block_ammount;
					break;
				}
				if(inventory_slot[i].type == entity->block_type){
					inventory_slot[i].ammount += entity->block_ammount;
					break;
				}
			}
			return;
		}
		if(distance_to_player < 5.0f){
			float pull = 0.03f / distance_to_player;
			vec3 to_player = vec3normalizeR(vec3subvec3R(camera.pos,entity->pos));
			vec3addvec3(&entity->dir,vec3mulR(to_player,pull));
		}
		if(node_root[initTraverse(entity->pos).node].block){
			vec3subvec3(&entity->pos,entity->dir);
			entity->dir = VEC3_ZERO;
		}
		break;
	case 0:
		vec3addvec3(&entity->dir,vec3mulR(vec3normalizeR(entity->dir),0.1f));
		if(entity->fuse++ != 30)
			break;
		for(int j = 0;j < 2048;j++){
			vec3 color = TRND1 < 0.5f ? (vec3){0.2f,1.0f,0.2f} : (vec3){0.2f,0.2f,1.0f};
			spawnLuminantParticle(entity->pos,vec3mulR(vec3rnd(),0.05f),color,TRND1 * 1000);
		}
		entity->alive = false;
		return;
	case 1:
		uint node_ptr = initTraverse(entity->pos).node;
		if(node_root[node_ptr].block){
			entity->alive = false;
			return;
		}
		if(!entity->fuse--)
			entity->alive = false;
		break;
	}
}

void entityTick(uint tick_ammount){
	for(int i = 0;i < ENTITY_AMMOUNT;i++){
		if(!entity_array[i].alive)
			continue;
		if(entity_array[i].flags & ENTITY_LUMINANT)
			calculateDynamicLight(entity_array[i].pos,0,10.0f,vec3negR(entity_array[i].color));
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
		if(!node.air)
			continue;
		if(node.air->entity != -1)
			entity_array[i].next_entity = node.air->entity;
		else
			entity_array[i].next_entity = -1;
		node.air->entity = i;
	}
}

void trySpawnEnemy(){
	if(TRND1 < 0.9f)
		return;
	vec3 pos = vec3addvec3R(camera.pos,VEC3_SINGLE((TRND1 - 0.5f) * 30.0f));
	traverse_init_t init = initTraverse(pos);
	node_t node = node_root[init.node];
	if(node.block)
		return;
	vec3 dir = {0.01f,0.01f,-1.0f};
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
	entity->color = (vec3){1.0f,1.0f,1.0f};
	entity->texture_pos = (vec2){0.0f,(1.0f / 2048) * (1024 + 80)};
	entity->texture_size = (vec2){(1.0f / (2048 * 3)) * 64,(1.0f / 2048) * 64};
	entity->size = 0.3f;
	entity->health = 100;
}