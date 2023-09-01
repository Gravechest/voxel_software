#pragma once

#define vec2_ZERO (vec2){0.0f,0.0f}
#define vec2_SQUARE(V) (vec2){V,V/1.77777777778f}

typedef union{
	struct{
		float x;
		float y;
	};
	float a[2];
}vec2;

enum{
	vec2_X,
	vec2_Y
};

void vec2add(vec2* v,float a);
void vec2div(vec2* v,float d);
void vec2mul(vec2* v,float m);
void vec2subvec2(vec2* v1,vec2 v2);
void vec2addvec2(vec2* v1,vec2 v2);
void vec2mulvec2(vec2* v1,vec2 v2);
void vec2normalize(vec2* p);
void vec2rot(vec2* v,float r);
vec2 vec2divFR(vec2 p,float p2);
vec2 vec2absR(vec2 p);
vec2 vec2subvec2R(vec2 v1,vec2 v2);
vec2 vec2addvec2R(vec2 v1,vec2 v2);
vec2 vec2mulvec2R(vec2 v1,vec2 v2);
vec2 vec2addR(vec2 p,float a);
vec2 vec2subR(vec2 p,float s);
vec2 vec2mulR(vec2 p,float f);	
vec2 vec2divR(vec2 p,float d);
vec2 vec2normalizeR(vec2 p);
vec2 vec2mixR(vec2 v1,vec2 v2,float mix);
vec2 vec2rotR(vec2 v,float r);
float vec2dotR(vec2 v1,vec2 v2);
float vec2length(vec2 p);
float vec2distance(vec2 p,vec2 p2);
vec2 vec2reflect(vec2 i,vec2 n);