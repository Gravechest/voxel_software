#include <math.h>
#include "vec2.h"


vec2 vec2subvec2R(vec2 v1,vec2 v2){
	return (vec2){v1.x-v2.x,v1.y-v2.y};
}

vec2 vec2addvec2R(vec2 v1,vec2 v2){
	return (vec2){v1.x+v2.x,v1.y+v2.y};
}

vec2 vec2mulvec2R(vec2 v1,vec2 v2){
	return (vec2){v1.x*v2.x,v1.y*v2.y};
}

void vec2add(vec2* v,float a){
	v->x += a;
	v->y += a;
}

void vec2div(vec2* v,float d){
	v->x /= d;
	v->y /= d;
}

void vec2mul(vec2* v,float m){
	v->x *= m;
	v->y *= m;
}

vec2 vec2addR(vec2 p,float a){
	return (vec2){p.x+a,p.y+a};
}

vec2 vec2subR(vec2 p,float s){
	return (vec2){p.x-s,p.y-s};
}

vec2 vec2mulR(vec2 p,float f){
	return (vec2){p.x*f,p.y*f};
}

vec2 vec2divR(vec2 p,float d){
	return (vec2){p.x/d,p.y/d};
}

void vec2subvec2(vec2* v1,vec2 v2){
	v1->x -= v2.x;
	v1->y -= v2.y;
}

void vec2addvec2(vec2* v1,vec2 v2){
	v1->x += v2.x;
	v1->y += v2.y;
}

void vec2mulvec2(vec2* v1,vec2 v2){
	v1->x *= v2.x;
	v1->y *= v2.y;
}

float vec2dotR(vec2 v1,vec2 v2){
	return v1.x*v2.x+v1.y*v2.y;
}

float vec2length(vec2 p){
	return sqrtf(p.x*p.x+p.y*p.y);
}

void vec2normalize(vec2* p){
	float d = vec2length(*p);
	p->x /= d;
	p->y /= d;
}

vec2 vec2normalizeR(vec2 p){
	float d = vec2length(p);
	p.x /= d;
	p.y /= d;
	return p;
}

float vec2distance(vec2 p,vec2 p2){
	return vec2length(vec2subvec2R(p,p2));
}

vec2 vec2divFR(vec2 p,float p2){
	return (vec2){p2/p.x,p2/p.y};
}

vec2 vec2absR(vec2 p){
	p.x = p.x < 0.0f ? -p.x : p.x;
	p.y = p.y < 0.0f ? -p.y : p.y;
	return p;	
}

void vec2rot(vec2* v,float r){
	float temp = v->x * cosf(r) - v->y * sinf(r);
	v->y       = v->x * sinf(r) + v->y * cosf(r);
	v->x = temp;
}

vec2 vec2rotR(vec2 v,float r){
	vec2 result;
	float temp = v.x * cosf(r) - v.y * sinf(r);
	result.y   = v.x * sinf(r) + v.y * cosf(r);
	result.x = temp;
	return result;
}

vec2 vec2mixR(vec2 v1,vec2 v2,float mix){
	return (vec2){v1.x * (1.0f - mix) + v2.x * mix,v1.y * (1.0f - mix) + v2.y * mix};
}

vec2 vec2reflect(vec2 i,vec2 n){
	return vec2subvec2R(i,vec2mulR(n,2.0f * vec2dotR(n,i)));
}