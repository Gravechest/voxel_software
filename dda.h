#pragma once

#include "vec2.h"
#include "vec3.h"

typedef struct{
	int x;
	int y;
}IVEC2;

typedef struct{
	union{
		struct{
			int x;
			int y;
			int z;	
		};
		int a[3];
	};
}IVEC3;

typedef struct{
	VEC3 pos;
	VEC3 dir;
	VEC3 delta;
	VEC3 side;

	IVEC3 step;
	IVEC3 square_pos;

	int square_side;
}RAY3;

typedef struct{
	VEC2 pos;
	VEC2 dir;
	VEC2 delta;
	VEC2 side;

	IVEC2 step;
	IVEC2 square_pos;

	int square_side;
}RAY2;

RAY3 ray3Create(VEC3 pos,VEC3 dir);
RAY2 ray2Create(VEC2 pos,VEC2 dir);
void ray2Itterate(RAY2* ray);
void ray3Itterate(RAY3* ray);
VEC2 ray3UV(RAY3 ray);