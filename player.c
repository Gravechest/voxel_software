#include "player.h"
#include "entity.h"
#include "sound.h"
#include "tmath.h"

int player_attack_state;

void playerAttack(){
	vec3 ray_dir = getLookAngle(camera.dir);
	for(int i = 0;i < ENTITY_AMMOUNT;i++){
		entity_t* entity = &entity_array[i];
		if(!entity->alive)
			continue;
		if(!entity->type == 3)
			continue;
		float distance = rayIntersectSphere(camera.pos,entity->pos,ray_dir,entity->size * 2.0f);
		if(distance < 0.0f)
			continue;
		if(distance > (PLAYER_FIST_DURATION / 2 - tAbsf(PLAYER_FIST_DURATION / 2 - player_attack_state)) * 0.15f)
			continue;
		playSound(SOUND_PUNCH,entity->pos);
		spawnNumberParticle(entity->pos,getLookAngle((vec2){camera.dir.x + M_PI * 0.5f,camera.dir.y}),30);
		if(entity->health < 30){
			entity->alive = false;
			return;
		}
		entity->health -= 30;
		entity->dir.z += 0.06f;
		entity->dir.x += ray_dir.x * 0.06f;
		entity->dir.y += ray_dir.y * 0.06f;
		player_attack_state = 0;
		return;
	}
}