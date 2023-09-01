#pragma once

#define VEC3_ZERO (vec3){0.0f,0.0f,0.0f}
#define VEC3_WHITE (vec3){1.0f,1.0f,1.0f}
#define VEC3_RED (vec3){1.0f,0.0f,0.0f}
#define VEC3_GREEN (vec3){0.0f,1.0f,0.0f}
#define VEC3_BLUE (vec3){0.0f,0.0f,1.0f}

#define VEC3_SINGLE(VALUE) (vec3){VALUE,VALUE,VALUE}

typedef union{
	struct{
		union{
			float x;
			float r;
		};
		union{
			float y;
			float g;
		};
		union{
			float z;
			float b;
		};
	};
	float a[3];
}vec3;

enum{
	VEC3_X,
	VEC3_Y,
	VEC3_Z
};

vec3 vec3subvec3R(vec3 v1,vec3 v2);
vec3 vec3addvec3R(vec3 v1,vec3 v2);
vec3 vec3mulvec3R(vec3 v1,vec3 v2);
void vec3add(vec3* v,float a);
void vec3mul(vec3* v1,float m);
void vec3div(vec3* v,float d);
vec3 vec3addR(vec3 p,float s);
vec3 vec3subR(vec3 p,float s);
vec3 vec3mulR(vec3 p,float f);
vec3 vec3divR(vec3 p,float d);
void vec3subvec3(vec3* v1,vec3 v2);
void vec3addvec3(vec3* v1,vec3 v2);
void vec3mulvec3(vec3* v1,vec3 v2);
void vec3divvec3(vec3* v1,vec3 v2);
float vec3dotR(vec3 v1,vec3 v2);
float vec3length(vec3 p);
void vec3normalize(vec3* p);
float vec3distance(vec3 p,vec3 p2);
vec3 vec3absR(vec3 p);
vec3 vec3divFR(vec3 p,float p2);
vec3 vec3avgvec3R(vec3 v1,vec3 v2);
vec3 vec3avgvec3R4(vec3 v1,vec3 v2,vec3 v3,vec3 v4);
vec3 vec3cross(vec3 v1,vec3 v2);
vec3 vec3normalizeR(vec3 p);
vec3 vec3negR(vec3 v);
vec3 vec3rnd();
vec3 vec3mixR(vec3 v1,vec3 v2,float mix);
vec3 vec3mixvec3R(vec3 v1,vec3 v2,vec3 mix);
vec3 vec3reflect(vec3 i,vec3 n);