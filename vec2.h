#pragma once

#define vec2_ZERO (vec2_t){0.0f,0.0f}
#define vec2_SQUARE(V) (vec2_t){V,V/1.77777777778f}

typedef union{
	struct{
		float x;
		float y;
	};
	float a[2];
}vec2_t;

enum{
	VEC2_X,
	VEC2_Y
};

void vec2add(vec2_t* v,float a);
void vec2div(vec2_t* v,float d);
void vec2mul(vec2_t* v,float m);
void vec2subvec2(vec2_t* v1,vec2_t v2);
void vec2addvec2(vec2_t* v1,vec2_t v2);
void vec2mulvec2(vec2_t* v1,vec2_t v2);
void vec2normalize(vec2_t* p);
void vec2rot(vec2_t* v,float r);
vec2_t vec2divFR(vec2_t p,float p2);
vec2_t vec2absR(vec2_t p);
vec2_t vec2subvec2R(vec2_t v1,vec2_t v2);
vec2_t vec2addvec2R(vec2_t v1,vec2_t v2);
vec2_t vec2mulvec2R(vec2_t v1,vec2_t v2);
vec2_t vec2addR(vec2_t p,float a);
vec2_t vec2subR(vec2_t p,float s);
vec2_t vec2mulR(vec2_t p,float f);	
vec2_t vec2divR(vec2_t p,float d);
vec2_t vec2normalizeR(vec2_t p);
vec2_t vec2mixR(vec2_t v1,vec2_t v2,float mix);
vec2_t vec2rotR(vec2_t v,float r);
float vec2dotR(vec2_t v1,vec2_t v2);
float vec2length(vec2_t p);
float vec2distance(vec2_t p,vec2_t p2);
vec2_t vec2reflect(vec2_t i,vec2_t n);