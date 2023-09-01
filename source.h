#pragma once

#include "dda.h"
#include "vec2.h"

#define RD_BLOCK 4

#define RES_SCALE 2
#define FOV (vec2){600.0f / RES_SCALE,600.0f / RES_SCALE}
#define WND_RESOLUTION (vec2){1080 / RES_SCALE,1920 / RES_SCALE}
#define WND_SIZE (vec2){1920,1080}

#define VK_W 0x57
#define VK_S 0x53
#define VK_A 0x41
#define VK_D 0x44

#define MAP_SIZE 64

#define LM_MAXDEPTH 7
#define LM_UPDATE_RATE 12

typedef struct{
	float x;
	float y;
	float z;
	float w;
}vec4;

typedef struct{
	union{
		struct{
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};
		unsigned int size;
	};
}pixel_t;

typedef struct{
	vec3 pos;
	vec2 dir;
	vec4 tri;
	vec3 vel;
}camera_t;

typedef struct{
	pixel_t* draw;
	pixel_t* render;
}vram_t;

typedef struct{
	vec3 pos;
	vec3 normal;
	int x,y;
}plane_t;

#define MAT_REFLECT (1 << 0)
#define MAT_REFRACT (1 << 1)
#define MAT_LUMINANT (1 << 2)

typedef struct{
	int flags;
	pixel_t* texture;
	vec3 luminance;
}material_t;

typedef struct{
	struct block_t* block;
	int side;
}portal_t;

typedef struct{
	union{
		vec3 luminance;
		vec3* luminance_p;
	};
	union{
		int* luminance_update_priority_p;
		int luminance_update_priority;
	};
	portal_t portal;
}block_side_t;

typedef struct block_t{
	int material;
	int depth;
	ivec3 pos;
	block_side_t side[6];
}block_t;

typedef struct block_node_t{
	block_t* block;
	union{
		struct block_node_t* child[2][2][2];
		struct block_node_t* child_s[8];
	};
	struct block_node_t* parent;
	ivec3 pos;
}block_node_t;

extern pixel_t* texture[];
extern camera_t camera;
extern pixel_t* texture[];
extern int test_bool;
extern material_t material_array[];
extern vram_t vram;
extern int global_tick;

plane_t getPlane(vec3 pos,vec3 dir,int side,float block_size);
vec3 getLookAngle(vec2 angle);
vec2 sampleCube(vec3 v,int* faceIndex);
float rayIntersectPlane(vec3 pos,vec3 dir,vec3 plane);
vec3 getLuminance(block_side_t side,vec2 uv,int depth);
vec3 getReflectLuminance(vec3 pos,vec3 dir);
vec3 getRayAngle(int x,int y);