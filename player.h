#pragma once

#include "source.h"
#include "dynamic_array.h"

typedef struct{
	vec3_t offset;
	uint32_t duration;
	int type;
}animationframe_t;

#define ANIMATION_INACTIVE 0

enum{
	ANIMATION_TRANSITION_LINEAR,
	ANIMATION_TRANSITION_SPEEDEND,
	ANIMATION_TRANSITION_SPEEDBEGIN,
};

typedef struct{
	dynamic_array_t frame;
	int32_t state;
}animation_t;

typedef struct{
	animation_t attack_animation;
}player_t;

extern player_t player;

void playerAttack();
void initAnimation();
void progressAnimation(animation_t* animation,int ammount);
vec3_t getAnimationOffset(animation_t* animation);