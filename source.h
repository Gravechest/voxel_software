#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "dda.h"
#include "vec2.h"

#define RD_DISTANCE 120

#define TEXTURE_AMMOUNT 6

#define LIGHT_NEW_STACK_SIZE 64

#define TEXTURE_SCALE (2048.0f * MAP_SIZE)
#define FOG_DISTANCE 0.0005f
#define SUN_ANGLE (vec3_t){-3.0f,-0.5f,1.0f}
#define RES_SCALE 1
#define FOV (vec2_t){600.0f / RES_SCALE,600.0f / RES_SCALE}

#define WND_RESOLUTION (vec2_t){1024 / RES_SCALE,1920 / RES_SCALE}
#define WND_RESOLUTION_Y (1920 / RES_SCALE)
#define WND_RESOLUTION_X (1024 / RES_SCALE)

#define WND_SIZE_Y 1920
#define WND_SIZE_X 1080
#define WND_SIZE (vec2_t){WND_SIZE_Y,WND_SIZE_X}

#define MAP_SIZE_BIT 8

#define MAP_SIZE (1 << MAP_SIZE_BIT)

#define LM_QUALITY 32
#define LM_MAXDEPTH (MAP_SIZE_BIT + 1)
#define LM_UPDATE_RATE 14

#define MAP_SIZE_INV ((float)MAP_SIZE / (1 << 19))

#define DRAW_MULTITHREAD 1

#define BLOCK_AIR 0xff
#define BLOCK_PARENT 0xfe

enum{
	BLOCK_WHITE,
	BLOCK_POWDER,
	BLOCK_BATTERY,
	BLOCK_WIRE,
	BLOCK_LAMP,
	BLOCK_SWITCH,
	BLOCK_STONE,
	BLOCK_SKIN,
	BLOCK_SPAWNLIGHT,
	BLOCK_COAL,
	BLOCK_GENERATOR
};

enum{
	GAMEMODE_SURVIVAL,
	GAMEMODE_CREATIVE
};

typedef union{
	struct{
		int x;
		int y;
	};
	int a[2];
}ivec2;

typedef struct{
	float x;
	float y;
	float z;
	float w;
}vec4;

typedef union{
	struct{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	unsigned int size;
}pixel_t;

typedef struct{
	vec3_t pos;
	vec2_t dir;
	vec4 tri;
	vec3_t vel;
	float exposure;
}camera_t;

typedef struct{
	pixel_t* draw;
	pixel_t* render;
}vram_t;

typedef struct{
	vec3_t pos;
	vec3_t normal;
	int x,y,z;
}plane_t;

typedef struct{
	uint32_t size;
	uint32_t mipmap_level_count;
	pixel_t* data;
	pixel_t* data_rev;
}texture_t;

#define MAT_REFLECT (1 << 0)
#define MAT_REFRACT (1 << 1)
#define MAT_LUMINANT (1 << 2)
#define MAT_LIQUID (1 << 3)
#define MAT_POWDER (1 << 4)
#define MAT_POWER (1 << 5)

typedef struct{
	int flags;
	texture_t texture;
	vec3_t luminance;
	float light_emit;
	vec2_t texture_pos;
	vec2_t texture_size;
	float hardness;
}material_t;

typedef struct{
	uint32_t node;
	union{
		vec3_t* luminance[6];
		struct{
			float ammount;
			float ammount_buffer;
			float disturbance;
		};
	};
	uint16_t power_grid[2];
	float power;
}block_t;

typedef struct{
	uint32_t node;
	vec3_t luminance;
	int entity;
	float o2;
	/*
	float co2;
	float h2o;
	*/
}air_t;

typedef struct{
	union{
		uint32_t child[2][2][2];
		uint32_t child_s[8];
	};
	ivec3 pos;
	uint32_t parent;
	uint32_t index;
	uint8_t type;
	uint8_t depth;
}node_t;

typedef struct{
	node_t* node;
	uint32_t side;
}light_new_stack_t;

typedef struct{
	int type;
	uint32_t ammount;
}inventoryslot_t;

#define ENTITY_LUMINANT (1 << 0)

extern texture_t texture[];
extern camera_t camera;
extern int test_bool;
extern material_t material_array[];
extern vram_t vram;
extern int global_tick;
extern float camera_exposure;
extern bool setting_smooth_lighting;
extern bool setting_fly;
extern volatile uint32_t light_new_stack_ptr;
extern volatile light_new_stack_t light_new_stack[];
extern bool context_is_gl;
extern pixel_t* texture_atlas;
extern int block_type;
extern int tool_select;
extern inventoryslot_t inventory_slot[9];
extern uint32_t border_block_table[6][4];
extern int main_thread_status;

vec3_t pointToScreenZ(vec3_t point);
plane_t getPlane(vec3_t pos,vec3_t dir,uint32_t side,float block_size);
vec3_t getLookAngle(vec2_t angle);
vec2_t sampleCube(vec3_t v,uint32_t* faceIndex);
float rayIntersectPlane(vec3_t pos,vec3_t dir,vec3_t plane);
vec3_t getLuminance(vec3_t* luminance,vec2_t uv,uint32_t depth);
vec3_t getRayAngle(uint32_t x,uint32_t y);
void drawLine(vec2_t pos_1,vec2_t pos_2,pixel_t color);
float sdCube(vec3_t point,vec3_t cube,float cube_size);
void calculateDynamicLight(vec3_t pos,uint32_t node_ptr,float radius,vec3_t color);
vec3_t pointToScreenRenderer(vec3_t point);
