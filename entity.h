#pragma once

#include "source.h"

#define ENTITY_AMMOUNT (1 << 12)

enum{
	ENTITY_ENEMY = 2,
	ENTITY_WATER = 4
};

typedef struct{
	union{
		vec3_t pos;
		int alive;
	};
	vec3_t dir;
	vec3_t color;
	uint32_t flags;
	unsigned char type;
	union{
		unsigned short fuse;
		unsigned short health;
	};
	uint32_t block_depth;
	uint32_t block_type;
	uint32_t block_ammount;
	vec2_t texture_pos;
	vec2_t texture_size;
	float size;
	float ammount;
	int next_entity;
}entity_t;

extern entity_t entity_array[];

void entityBehavior(entity_t* entity);
void entityTick(uint32_t tick_ammount);
void trySpawnEnemy();
entity_t* getNewEntity();