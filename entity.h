#pragma once

#include "source.h"

#define ENTITY_AMMOUNT (1 << 12)

enum{
	ENTITY_ENEMY = 2,
	ENTITY_WATER = 4
};

typedef struct{
	union{
		vec3 pos;
		int alive;
	};
	vec3 dir;
	vec3 color;
	uint flags;
	unsigned char type;
	union{
		unsigned short fuse;
		unsigned short health;
	};
	uint block_depth;
	uint block_type;
	uint block_ammount;
	vec2 texture_pos;
	vec2 texture_size;
	float size;
	float ammount;
	int next_entity;
}entity_t;

extern entity_t entity_array[];

void entityBehavior(entity_t* entity);
void entityTick(uint tick_ammount);
void trySpawnEnemy();
entity_t* getNewEntity();