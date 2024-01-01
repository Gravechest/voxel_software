#include "vec3.h"
#include "math.h"
#include "tmath.h"

vec3_t vec3subvec3R(vec3_t v1,vec3_t v2){
	return (vec3_t){v1.x - v2.x,v1.y - v2.y,v1.z - v2.z};
}

vec3_t vec3addvec3R(vec3_t v1,vec3_t v2){
	return (vec3_t){v1.x + v2.x,v1.y + v2.y,v1.z + v2.z};
}

vec3_t vec3mulvec3R(vec3_t v1,vec3_t v2){
	return (vec3_t){v1.x * v2.x,v1.y * v2.y,v1.z * v2.z};
}

void vec3add(vec3_t* v,float a){
	v->x += a;
	v->y += a;
	v->z += a;
}

void vec3sub(vec3_t* v,float a){
	v->x -= a;
	v->y -= a;
	v->z -= a;
}

void vec3div(vec3_t* v,float d){
	v->x /= d;
	v->y /= d;
	v->z /= d;
}

void vec3mul(vec3_t* v1,float m){
	v1->x *= m;
	v1->y *= m;
	v1->z *= m;
}

vec3_t vec3addR(vec3_t p,float s){
	return (vec3_t){p.x + s,p.y + s,p.z + s};
}

vec3_t vec3subR(vec3_t p,float s){
	return (vec3_t){p.x - s,p.y - s,p.z - s};
}

vec3_t vec3mulR(vec3_t p,float f){
	return (vec3_t){p.x * f,p.y * f,p.z * f};
}

vec3_t vec3divR(vec3_t p,float d){
	return (vec3_t){p.x / d,p.y / d,p.z / d};
}

void vec3subvec3(vec3_t* v1,vec3_t v2){
	v1->x -= v2.x;
	v1->y -= v2.y;
	v1->z -= v2.z;
}

void vec3addvec3(vec3_t* v1,vec3_t v2){
	v1->x += v2.x;
	v1->y += v2.y;
	v1->z += v2.z;
}

void vec3mulvec3(vec3_t* v1,vec3_t v2){
	v1->x *= v2.x;
	v1->y *= v2.y;
	v1->z *= v2.z;
}

void vec3divvec3(vec3_t* v1,vec3_t v2){
	v1->x /= v2.x;
	v1->y /= v2.y;
	v1->z /= v2.z;
}

float vec3dotR(vec3_t v1,vec3_t v2){
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float vec3length(vec3_t p){
	return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
}

void vec3normalize(vec3_t* p){
	float d = 1.0f / vec3length(*p);
	p->x *= d;
	p->y *= d;
	p->z *= d;
}

vec3_t vec3normalizeR(vec3_t p){
	float d = 1.0f / vec3length(p);
	p.x *= d;
	p.y *= d;
	p.z *= d;
	return p;
}

float vec3distance(vec3_t p,vec3_t p2){
	return vec3length(vec3subvec3R(p,p2));
}

vec3_t vec3divFR(vec3_t p,float p2){
	return (vec3_t){p2/p.x,p2/p.y,p2/p.z};
}

vec3_t vec3absR(vec3_t p){
	p.x = p.x < 0.0f ? -p.x : p.x;
	p.y = p.y < 0.0f ? -p.y : p.y;
	p.z = p.z < 0.0f ? -p.z : p.z;
	return p;	
}

vec3_t vec3avgvec3R(vec3_t v1,vec3_t v2){
	return (vec3_t){(v1.x+v2.x)*0.5f,(v1.y+v2.y)*0.5f,(v1.z+v2.z)*0.5f};
}

vec3_t vec3avgvec3R4(vec3_t v1,vec3_t v2,vec3_t v3,vec3_t v4){
	return (vec3_t){(v1.x + v2.x + v3.x + v4.x) * 0.25f,(v1.y + v2.y + v3.y + v4.y) * 0.25f,(v1.z + v2.z + v3.z + v4.z) * 0.25f};
}

vec3_t vec3mixR(vec3_t v1,vec3_t v2,float mix){
	return (vec3_t){v1.x * (1.0f - mix) + v2.x * mix,v1.y * (1.0f - mix) + v2.y * mix,v1.z * (1.0f - mix) + v2.z * mix};
}

vec3_t vec3mixvec3R(vec3_t v1,vec3_t v2,vec3_t mix){
	return (vec3_t){v1.x * (1.0f - mix.x) + v2.x * mix.x,v1.y * (1.0f - mix.y) + v2.y * mix.y,v1.z * (1.0f - mix.z) + v2.z * mix.z};
}

vec3_t vec3cross(vec3_t v1,vec3_t v2){
	vec3_t v3;
	v3.x = v1.y * v2.z - v2.y * v1.z;
	v3.y = v1.z * v2.x - v2.z * v1.x;
	v3.z = v1.x * v2.y - v2.x * v1.y;
	return v3;
}

vec3_t vec3rnd(){
	return (vec3_t){tRnd()+tRnd()+tRnd()-4.5f,tRnd()+tRnd()+tRnd()-4.5f,tRnd()+tRnd()+tRnd()-4.5f};
}

vec3_t vec3negR(vec3_t v){
	return (vec3_t){-v.x,-v.y,-v.z};
}

vec3_t vec3reflect(vec3_t i,vec3_t n){
	return vec3subvec3R(i,vec3mulR(n,2.0f * vec3dotR(n,i)));
}

vec3_t vec3refract(vec3_t I,vec3_t N,float eta){
    float k = 1.0f - eta * eta * (1.0f - vec3dotR(N, I) * vec3dotR(N, I));
    if (k < 0.0f)
        return VEC3_ZERO;
    else
        return vec3subvec3R(vec3mulR(I,eta),vec3mulR(N,(vec3dotR(N, I) * eta + sqrtf(k))));
}