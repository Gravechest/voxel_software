#include <stdbool.h>
#include <Windows.h>
#include <math.h>

#include "physics.h"
#include "source.h"
#include "tmath.h"
#include "vec3.h"
#include "tree.h"
#include "sound.h"

float physic_mouse_x;

typedef struct{
	char w;
	char s;
	char d;
	char a;
}key_t;

key_t key;

void bunnyhop(){
	static float the_same = 1.0f;
	static bool direction = false;
	if(direction == physic_mouse_x > 0.0f)
		the_same += 0.1f;
	else if(physic_mouse_x){
		the_same = 1.0f;
		direction ^= true;
	}
	if(insideBlock(vec3subvec3R(camera.pos,(vec3_t){0.0f,0.0f,1.8f})))
		return;
	bool key_a = GetKeyState('A') & 0x80;
	bool key_d = GetKeyState('D') & 0x80;
	if(physic_mouse_x < 0.0f && key_a && !key_d){
		camera.vel.x += cosf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		camera.vel.y += sinf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		vec2rot(&camera.vel.x,physic_mouse_x);
	}
	if(physic_mouse_x > 0.0f && key_d && !key_a){
		camera.vel.x += cosf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		camera.vel.y += sinf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		vec2rot(&camera.vel.x,physic_mouse_x);
	}
	physic_mouse_x = 0.0f;
}

#define PLAYER_THICKNESS 0.2f
#define PLAYER_HEIGHT 1.8f

#define PLAYER_OFFSET -1.0f
#define PLAYER_SIZE (vec3_t){0.2f,0.2f,1.8f}

bool boxCubeIntersect(vec3_t box_pos,vec3_t box_size,vec3_t cube_pos,float cube_size){
	vec3_t box_max = vec3addvec3R(box_pos,box_size);
	vec3_t cube_max = vec3addR(cube_pos,cube_size);
	bool x = box_pos.x <= cube_max.x && box_max.x >= cube_pos.x;
	bool y = box_pos.y <= cube_max.y && box_max.y >= cube_pos.y;
	bool z = box_pos.z <= cube_max.z && box_max.z >= cube_pos.z;
    return x && y && z;
}

float boxTreeCollision(vec3_t pos,vec3_t size,uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3mul(&block_pos,block_size);
	bool collision = boxCubeIntersect(pos,size,block_pos,block_size);
	if(!collision || node->type == BLOCK_AIR)
		return -1.0f;
	if(node->type != BLOCK_PARENT)
		return block_pos.z + block_size - pos.z;
	float height = -1.0f;
	for(int i = 0;i < 8;i++)
		height = tMaxf(boxTreeCollision(pos,size,node->child_s[i]),height);
	return height;
}

void walkPhysics(uint32_t tick_ammount){
	bunnyhop();
	for(int i = 0;i < tick_ammount;i++){
		float hit[3];
		bool is_stair_x = true,is_stair_y = true;
		float stair_x = 0.0f,stair_y = 0.0f;
		bool down_z = camera.vel.z < 0.0f;
		vec3_t hitbox = vec3subvec3R(camera.pos,vec3mulR(PLAYER_SIZE,0.5f));
		hitbox.z += PLAYER_OFFSET;
		hit[VEC3_X] = boxTreeCollision(vec3addvec3R(hitbox,(vec3_t){camera.vel.x,0.0f,0.0f}),PLAYER_SIZE,0);
		hit[VEC3_Y] = boxTreeCollision(vec3addvec3R(hitbox,(vec3_t){0.0f,camera.vel.y,0.0f}),PLAYER_SIZE,0);
		hit[VEC3_Z] = boxTreeCollision(vec3addvec3R(hitbox,(vec3_t){0.0f,0.0f,camera.vel.z}),PLAYER_SIZE,0);
		vec3addvec3(&camera.pos,camera.vel);
		if(hit[VEC3_X] != -1.0f){
			if(hit[VEC3_X] < 0.6f){
				vec3_t lifted = vec3addvec3R(hitbox,(vec3_t){camera.vel.x,0.0f,hit[VEC3_X] + 0.001f});
				if(boxTreeCollision(lifted,PLAYER_SIZE,0) == -1.0f)
					camera.pos.z += hit[VEC3_X] + 0.001f;
				else{
					camera.pos.x -= camera.vel.x;
					camera.vel.x = 0.0f;
				}
			}
			else{
				camera.pos.x -= camera.vel.x;
				camera.vel.x = 0.0f;
			}
		}
		if(hit[VEC3_Y] != -1.0f){
			if(hit[VEC3_Y] < 0.6f){
				vec3_t lifted = vec3addvec3R(hitbox,(vec3_t){0.0f,camera.vel.y,hit[VEC3_Y] + 0.001f});
				if(boxTreeCollision(lifted,PLAYER_SIZE,0) == -1.0f)
					camera.pos.z += hit[VEC3_Y] + 0.001f;
				else{
					camera.pos.y -= camera.vel.y;
					camera.vel.y = 0.0f;
				}
			}
			else{
				camera.pos.y -= camera.vel.y;
				camera.vel.y = 0.0f;
			}
		}
		if(hit[VEC3_Z] != -1.0f){
			camera.pos.z -= camera.vel.z;
			camera.vel.z = 0.0f;
			if(down_z){
				if(!in_menu && !block_menu_block){
					if(GetKeyState(VK_SPACE) & 0x80){
						playSound(SOUND_JUMP,vec3addvec3R(camera.pos,(vec3_t){0.01f,0.01f,-1.55f}));
						camera.vel.z += MV_JUMPHEIGHT;
					}
				}
				static float step_sound_cooldown;
				step_sound_cooldown += tAbsf(camera.vel.y) + tAbsf(camera.vel.x);
				if(step_sound_cooldown > 2.5f){
					step_sound_cooldown = 0.0f;
					playSound(SOUND_STEP,vec3addvec3R(camera.pos,(vec3_t){0.01f,0.01f,-1.55f}));
				}
			}
		}
		if(!in_menu && !block_menu_block){
			if(key.w){
				float mod = key.d || key.a ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
				if(hit[VEC3_Z] == -1.0f || hit[VEC3_Z] && !down_z)
					mod *= 0.01f;
				camera.vel.x += cosf(camera.dir.x) * mod * 2.0f;
				camera.vel.y += sinf(camera.dir.x) * mod * 2.0f;
			}
			if(key.s){
				float mod = key.d || key.a ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
				if(hit[VEC3_Z] == -1.0f || hit[VEC3_Z] && !down_z)
					mod *= 0.01f;
				camera.vel.x -= cosf(camera.dir.x) * mod * 2.0f;
				camera.vel.y -= sinf(camera.dir.x) * mod * 2.0f;
			}
			if(key.d){
				float mod = key.s || key.w ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
				if(hit[VEC3_Z] == -1.0f || hit[VEC3_Z] && !down_z)
					mod *= 0.01f;
				camera.vel.x += cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
				camera.vel.y += sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
			}
			if(key.a){
				float mod = key.s || key.w ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
				if(hit[VEC3_Z] == -1.0f || hit[VEC3_Z] && !down_z)
					mod *= 0.01f;
				camera.vel.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
				camera.vel.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
			}
		}
		vec3mul(&camera.vel,hit[VEC3_Z] == -1.0f || hit[VEC3_Z] && !down_z ? AIR_FRICTION : GROUND_FRICTION); 
		camera.vel.z -= MV_GRAVITY;
	}
}

void physics(uint32_t tick_ammount){
	key.w = GetKeyState('W') & 0x80;
	key.s = GetKeyState('S') & 0x80;
	key.d = GetKeyState('D') & 0x80;
	key.a = GetKeyState('A') & 0x80;
	camera.tri = (vec4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
	global_tick++;
	if(!setting_fly){
		walkPhysics(tick_ammount);
		return;
	}
	float mod = (GetKeyState(VK_LCONTROL) & 0x80) ? 3.0f : 1.0f;
	mod *= tick_ammount;
	if(!in_menu && !block_menu_block){
		if(key.w){
			float mod_2 = mod * (key.d || key.a ? MV_FLYSPEED * MV_DIAGONAL : MV_FLYSPEED);
			camera.pos.x += cosf(camera.dir.x) * mod_2;
			camera.pos.y += sinf(camera.dir.x) * mod_2;
		}
		if(key.s){
			float mod_2 = mod * (key.d || key.a ? MV_FLYSPEED * MV_DIAGONAL : MV_FLYSPEED);
			camera.pos.x -= cosf(camera.dir.x) * mod_2;
			camera.pos.y -= sinf(camera.dir.x) * mod_2;
		}
		if(key.d){
			float mod_2 = mod * (key.s || key.w ? MV_FLYSPEED * MV_DIAGONAL : MV_FLYSPEED);
			camera.pos.x += cosf(camera.dir.x + M_PI * 0.5f) * mod_2;
			camera.pos.y += sinf(camera.dir.x + M_PI * 0.5f) * mod_2;
		}
		if(key.a){
			float mod_2 = mod * (key.s || key.w ? MV_FLYSPEED * MV_DIAGONAL : MV_FLYSPEED);
			camera.pos.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod_2;
			camera.pos.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod_2;
		}
		if(GetKeyState(VK_SPACE) & 0x80) 
			camera.pos.z += MV_FLYSPEED * mod;
		if(GetKeyState(VK_LSHIFT) & 0x80) 
			camera.pos.z -= MV_FLYSPEED * mod;
	}
}