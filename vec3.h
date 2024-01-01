#pragma once

#define VEC3_ZERO (vec3_t){0.0f,0.0f,0.0f}
#define VEC3_WHITE (vec3_t){1.0f,1.0f,1.0f}
#define VEC3_RED (vec3_t){1.0f,0.0f,0.0f}
#define VEC3_GREEN (vec3_t){0.0f,1.0f,0.0f}
#define VEC3_BLUE (vec3_t){0.0f,0.0f,1.0f}

#define VEC3_SINGLE(VALUE) (vec3_t){VALUE,VALUE,VALUE}

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
}vec3_t;

enum{
	VEC3_X,
	VEC3_Y,
	VEC3_Z
};

vec3_t vec3subvec3R(vec3_t v1,vec3_t v2);
vec3_t vec3addvec3R(vec3_t v1,vec3_t v2);
vec3_t vec3mulvec3R(vec3_t v1,vec3_t v2);
void vec3add(vec3_t* v,float a);
void vec3sub(vec3_t* v,float a);
void vec3mul(vec3_t* v1,float m);
void vec3div(vec3_t* v,float d);
vec3_t vec3addR(vec3_t p,float s);
vec3_t vec3subR(vec3_t p,float s);
vec3_t vec3mulR(vec3_t p,float f);
vec3_t vec3divR(vec3_t p,float d);
void vec3subvec3(vec3_t* v1,vec3_t v2);
void vec3addvec3(vec3_t* v1,vec3_t v2);
void vec3mulvec3(vec3_t* v1,vec3_t v2);
void vec3divvec3(vec3_t* v1,vec3_t v2);
float vec3dotR(vec3_t v1,vec3_t v2);
float vec3length(vec3_t p);
void vec3normalize(vec3_t* p);
float vec3distance(vec3_t p,vec3_t p2);
vec3_t vec3absR(vec3_t p);
vec3_t vec3divFR(vec3_t p,float p2);
vec3_t vec3avgvec3R(vec3_t v1,vec3_t v2);
vec3_t vec3avgvec3R4(vec3_t v1,vec3_t v2,vec3_t v3,vec3_t v4);
vec3_t vec3cross(vec3_t v1,vec3_t v2);
vec3_t vec3normalizeR(vec3_t p);
vec3_t vec3negR(vec3_t v);
vec3_t vec3rnd();
vec3_t vec3mixR(vec3_t v1,vec3_t v2,float mix);
vec3_t vec3mixvec3R(vec3_t v1,vec3_t v2,vec3_t mix);
vec3_t vec3reflect(vec3_t i,vec3_t n);
vec3_t vec3refract(vec3_t I,vec3_t N,float eta);