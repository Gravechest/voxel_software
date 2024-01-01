#include <math.h>

#include "vec2.h"

vec2_t vec2subvec2R(vec2_t v1,vec2_t v2){
	return (vec2_t){v1.x - v2.x,v1.y - v2.y};
}

vec2_t vec2addvec2R(vec2_t v1,vec2_t v2){
	return (vec2_t){v1.x + v2.x,v1.y + v2.y};
}

vec2_t vec2mulvec2R(vec2_t v1,vec2_t v2){
	return (vec2_t){v1.x * v2.x,v1.y * v2.y};
}

void vec2add(vec2_t* v,float a){
	v->x += a;
	v->y += a;
}

void vec2div(vec2_t* v,float d){
	v->x /= d;
	v->y /= d;
}

void vec2mul(vec2_t* v,float m){
	v->x *= m;
	v->y *= m;
}

vec2_t vec2addR(vec2_t p,float a){
	return (vec2_t){p.x+a,p.y+a};
}

vec2_t vec2subR(vec2_t p,float s){
	return (vec2_t){p.x-s,p.y-s};
}

vec2_t vec2mulR(vec2_t p,float f){
	return (vec2_t){p.x*f,p.y*f};
}

vec2_t vec2divR(vec2_t p,float d){
	return (vec2_t){p.x/d,p.y/d};
}

void vec2subvec2(vec2_t* v1,vec2_t v2){
	v1->x -= v2.x;
	v1->y -= v2.y;
}

void vec2addvec2(vec2_t* v1,vec2_t v2){
	v1->x += v2.x;
	v1->y += v2.y;
}

void vec2mulvec2(vec2_t* v1,vec2_t v2){
	v1->x *= v2.x;
	v1->y *= v2.y;
}

float vec2dotR(vec2_t v1,vec2_t v2){
	return v1.x*v2.x+v1.y*v2.y;
}

float vec2length(vec2_t p){
	return sqrtf(p.x*p.x+p.y*p.y);
}

void vec2normalize(vec2_t* p){
	float d = vec2length(*p);
	p->x /= d;
	p->y /= d;
}

vec2_t vec2normalizeR(vec2_t p){
	float d = vec2length(p);
	p.x /= d;
	p.y /= d;
	return p;
}

float vec2distance(vec2_t p,vec2_t p2){
	return vec2length(vec2subvec2R(p,p2));
}

vec2_t vec2divFR(vec2_t p,float p2){
	return (vec2_t){p2/p.x,p2/p.y};
}

vec2_t vec2absR(vec2_t p){
	p.x = p.x < 0.0f ? -p.x : p.x;
	p.y = p.y < 0.0f ? -p.y : p.y;
	return p;	
}

void vec2rot(vec2_t* v,float r){
	float temp = v->x * cosf(r) - v->y * sinf(r);
	v->y       = v->x * sinf(r) + v->y * cosf(r);
	v->x = temp;
}

vec2_t vec2rotR(vec2_t v,float r){
	vec2_t result;
	float temp = v.x * cosf(r) - v.y * sinf(r);
	result.y   = v.x * sinf(r) + v.y * cosf(r);
	result.x = temp;
	return result;
}

vec2_t vec2mixR(vec2_t v1,vec2_t v2,float mix){
	return (vec2_t){v1.x * (1.0f - mix) + v2.x * mix,v1.y * (1.0f - mix) + v2.y * mix};
}

vec2_t vec2reflect(vec2_t i,vec2_t n){
	return vec2subvec2R(i,vec2mulR(n,2.0f * vec2dotR(n,i)));
}