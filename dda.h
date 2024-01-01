#pragma once

#include "vec2.h"
#include "vec3.h"

typedef struct{
	int x;
	int y;
}Ivec2;

typedef union{
	struct{
		int x;
		int y;
		int z;	
	};
	int a[3];
}ivec3;

typedef union{
	struct{
		unsigned int x;
		unsigned int y;
		unsigned int z;	
	};
	unsigned int a[3];
}uvec3;

typedef struct{
	vec3_t pos;
	vec3_t dir;
	vec3_t delta;
	vec3_t side;

	ivec3 step;
	ivec3 square_pos;

	unsigned int square_side;
}ray3_t;

typedef struct{
	vec2_t pos;
	vec2_t dir;
	vec2_t delta;
	vec2_t side;

	Ivec2 step;
	Ivec2 square_pos;

	int square_side;
}ray2_t;

ray3_t ray3Create(vec3_t pos,vec3_t dir);
ray2_t ray2Create(vec2_t pos,vec2_t dir);
void ray2Itterate(ray2_t* ray);
void ray3Itterate(ray3_t* ray);
vec2_t ray3UV(ray3_t ray);