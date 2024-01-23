#include <math.h>

#include "player.h"
#include "entity.h"
#include "sound.h"
#include "tmath.h"

player_t player = {
	.attack_animation.frame = {.element_size = sizeof(animation_t)}
};

void initAnimation(){
	animationframe_t animation;
	animation.offset = (vec3_t){0.7f,-0.7f,3.0f}; 
	animation.duration = 20;
	animation.type = ANIMATION_TRANSITION_SPEEDBEGIN;
	dynamicArrayAdd(&player.attack_animation.frame,&animation);
	animation.offset = (vec3_t){0.0f,0.0f,0.0f}; 
	animation.duration = 20;
	animation.type = ANIMATION_TRANSITION_SPEEDEND;
	dynamicArrayAdd(&player.attack_animation.frame,&animation);
}

void progressAnimation(animation_t* animation,int ammount){
	animation->state += ammount;
	int progress = 0;
	for(int i = 0;i < animation->frame.size;i++){
		animationframe_t* frame = dynamicArrayGet(animation->frame,i);
		if(animation->state < progress + frame->duration)
			return;
		progress += frame->duration;
	}
	animation->state = ANIMATION_INACTIVE;
}

vec3_t getAnimationOffset(animation_t* animation){
	int progress = 0;
	for(int i = 0;i < animation->frame.size;i++){
		animationframe_t* frame = dynamicArrayGet(animation->frame,i);
		if(animation->state < progress + frame->duration){
			animationframe_t previous_offset;
			if(!i)
				previous_offset = (animationframe_t){.offset = VEC3_ZERO,.duration = 0};
			else
				previous_offset = *(animationframe_t*)dynamicArrayGet(animation->frame,i - 1);
			float interpolate = (float)(progress + frame->duration - animation->state) / frame->duration;
			switch(frame->type){
			case ANIMATION_TRANSITION_SPEEDBEGIN: interpolate = sqrtf(interpolate); break;
			case ANIMATION_TRANSITION_SPEEDEND: interpolate *= interpolate; break;
			}
			return vec3mixR(frame->offset,previous_offset.offset,interpolate);
		}
		progress += frame->duration;
	}
	return VEC3_ZERO;
}

void playerAttack(){
	vec3_t ray_dir = getLookAngle(camera.dir);
	for(int i = 0;i < ENTITY_AMMOUNT;i++){
		entity_t* entity = &entity_array[i];
		if(!entity->alive)
			continue;
		if(!(entity->flags & ENTITY_FLAG_ENEMY))
			continue;
		float distance = rayIntersectSphere(camera.pos,entity->pos,ray_dir,entity->size * 1.5f);
		if(distance > PLAYER_REACH || distance < 0.0f)
			continue;
		spawnNumberParticle(entity->pos,30);
		playSound(SOUND_PUNCH,entity->pos);
		if(entity->health < 30){
			entity->alive = false;
			return;
		}
		entity->health -= 30;
		entity->dir.z += 0.06f;
		entity->dir.x += ray_dir.x * 0.06f;
		entity->dir.y += ray_dir.y * 0.06f;
		return;
	}
}