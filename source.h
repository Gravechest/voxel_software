#pragma once

#include <stdbool.h>

#include "dda.h"
#include "vec2.h"

#define RD_DISTANCE 120

#define TEXTURE_AMMOUNT 6

#define LIGHT_NEW_STACK_SIZE 16

#define TEXTURE_SCALE (2048.0f * MAP_SIZE)
#define FOG_DISTANCE 0.0005f
#define SUN_ANGLE (vec3){-3.0f,-0.5f,1.0f}
#define RES_SCALE 1
#define FOV (vec2){600.0f / RES_SCALE,600.0f / RES_SCALE}

#define WND_RESOLUTION (vec2){1024 / RES_SCALE,1920 / RES_SCALE}
#define WND_RESOLUTION_Y (1920 / RES_SCALE)
#define WND_RESOLUTION_X (1024 / RES_SCALE)

#define WND_SIZE_Y 1920
#define WND_SIZE_X 1080
#define WND_SIZE (vec2){WND_SIZE_Y,WND_SIZE_X}

#define VK_W 0x57
#define VK_S 0x53
#define VK_A 0x41
#define VK_D 0x44

#define MAP_SIZE_BIT 8

#define MAP_SIZE (1 << MAP_SIZE_BIT)

#define LM_QUALITY 32
#define LM_MAXDEPTH (MAP_SIZE_BIT + 1)
#define LM_UPDATE_RATE 14

#define MAP_SIZE_INV ((float)MAP_SIZE / (1 << 19))

#define DRAW_MULTITHREAD 1

typedef unsigned int uint;

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
	vec3 pos;
	vec2 dir;
	vec4 tri;
	vec3 vel;
	float pre[5];
}camera_t;

typedef struct{
	pixel_t* draw;
	pixel_t* render;
}vram_t;

typedef struct{
	vec3 pos;
	vec3 normal;
	int x,y,z;
}plane_t;

typedef struct{
	uint size;
	uint mipmap_level_count;
	pixel_t* data;
	pixel_t* data_rev;
}texture_t;

#define MAT_REFLECT (1 << 0)
#define MAT_REFRACT (1 << 1)
#define MAT_LUMINANT (1 << 2)

typedef struct{
	int flags;
	texture_t texture;
	vec3 luminance;
	uint texture_id;
}material_t;

typedef struct{
	vec3* luminance_p;
}block_side_t;

typedef struct block_t{
	uint material;
	block_side_t side[6];
	int padding[2];
}block_t;

typedef struct block_node_t{
	block_t* block;
	union{
		uint child[2][2][2];
		uint child_s[8];
	};
	uint parent;
	ivec3 pos;
	vec3 luminance;
	unsigned char depth;
	unsigned char marked;
	unsigned short shape;
}block_node_t;

typedef struct{
	block_node_t* node;
	uint side;
}light_new_stack_t;

extern texture_t texture[];
extern camera_t camera;
extern int test_bool;
extern material_t material_array[];
extern vram_t vram;
extern int global_tick;
extern float camera_exposure;
extern bool setting_smooth_lighting;
extern bool setting_fly;
extern volatile uint light_new_stack_ptr;
extern volatile light_new_stack_t light_new_stack[];
extern bool context_is_gl;
extern pixel_t* texture_atlas;
extern camera_t camera_rd;

vec3 pointToScreenZ(vec3 point);
plane_t getPlane(vec3 pos,vec3 dir,uint side,float block_size);
vec3 getLookAngle(vec2 angle);
vec2 sampleCube(vec3 v,uint* faceIndex);
float rayIntersectPlane(vec3 pos,vec3 dir,vec3 plane);
vec3 getLuminance(block_side_t side,vec2 uv,uint depth);
vec3 getReflectLuminance(vec3 pos,vec3 dir);
vec3 getRayAngle(uint x,uint y);
void drawLine(vec2 pos_1,vec2 pos_2,pixel_t color);