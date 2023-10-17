#include <stdbool.h>
#include <Windows.h>
#include <math.h>

#include "physics.h"
#include "source.h"
#include "tmath.h"
#include "vec3.h"
#include "tree.h"

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
	if(insideBlock(vec3subvec3R(camera.pos,(vec3){0.0f,0.0f,1.8f})))
		return;
	if(physic_mouse_x < 0.0f && (GetKeyState(VK_A) & 0x80)){
		camera.vel.x += cosf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		camera.vel.y += sinf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		vec2rot(&camera.vel.x,physic_mouse_x);
	}
	if(physic_mouse_x > 0.0f && (GetKeyState(VK_D) & 0x80)){
		camera.vel.x += cosf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		camera.vel.y += sinf(camera.dir.x) * MV_BHOP_BONUS / the_same;
		vec2rot(&camera.vel.x,physic_mouse_x);
	}
	
	physic_mouse_x = 0.0f;
}

#define PLAYER_THICKNESS 0.2f
#define PLAYER_HEIGHT 1.8f

void checkAxis(vec3 pos,float* stair,bool* is_stair,bool* hit,float height,int axis){
	block_t* block = insideBlock(pos);
	if(block){
		*hit = true;
		if(height > -1.1f)
			*is_stair = false;
		else
			*stair = height + PLAYER_HEIGHT;
	}
}

void walkPhysics(){
	bool hit[3];
	bool hit_z_down = false,hit_z_up = false;
	bool is_stair_x = true,is_stair_y = true;
	float stair_x = 0.0f,stair_y = 0.0f;

	bunnyhop();

	for(float x = - PLAYER_THICKNESS;x <=  PLAYER_THICKNESS;x += 0.05f){
		for(float y = - PLAYER_THICKNESS;y <=  PLAYER_THICKNESS;y += 0.05f){
			if(insideBlock((vec3){camera.pos.x + x,camera.pos.y + y,camera.pos.z - PLAYER_HEIGHT + 0.2f + camera.vel.z}))
				hit_z_down = true;
			if(insideBlock((vec3){camera.pos.x + x,camera.pos.y + y,camera.pos.z + 0.2f + camera.vel.z}))
				hit_z_up = true;
		}
	}
	for(float x = -PLAYER_HEIGHT + 0.2f;x <= 0.2f;x += 0.05f){
		for(float y = - PLAYER_THICKNESS;y <=  PLAYER_THICKNESS;y += 0.05f){
			vec3 pos;
			pos = (vec3){camera.pos.x - PLAYER_THICKNESS + camera.vel.x,camera.pos.y + y,camera.pos.z + x};
			checkAxis(pos,&stair_x,&is_stair_x,&hit[VEC3_X],x,VEC3_X);
			pos = (vec3){camera.pos.x + PLAYER_THICKNESS + camera.vel.x,camera.pos.y + y,camera.pos.z + x};
			checkAxis(pos,&stair_x,&is_stair_x,&hit[VEC3_X],x,VEC3_X);
			pos = (vec3){camera.pos.x + y,camera.pos.y + camera.vel.y - PLAYER_THICKNESS,camera.pos.z + x};
			checkAxis(pos,&stair_y,&is_stair_y,&hit[VEC3_Y],x,VEC3_Y);
			pos = (vec3){camera.pos.x + y,camera.pos.y + camera.vel.y + PLAYER_THICKNESS,camera.pos.z + x};
			checkAxis(pos,&stair_y,&is_stair_y,&hit[VEC3_Y],x,VEC3_Y);
		}
	}
	vec3addvec3(&camera.pos,camera.vel);
	if(hit[VEC3_X]){
		if(is_stair_x)
			camera.pos.z += stair_x;
		else{
			camera.pos.x -= camera.vel.x;
			camera.vel.x = 0.0f;
		}
	}
	if(hit[VEC3_Y]){
		if(is_stair_y)
			camera.pos.z += stair_y;
		else{
			camera.pos.y -= camera.vel.y;
			camera.vel.y = 0.0f;
		}
	}
	if(hit_z_down || hit_z_up){
		camera.pos.z -= camera.vel.z;
		camera.vel.z = 0.0f;
		if(hit_z_down && GetKeyState(VK_SPACE) & 0x80){
			camera.vel.z = MV_JUMPHEIGHT;
		}
	}
	if(key.w){
		float mod = key.d || key.a ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit_z_down)
			mod *= 0.01f;
		camera.vel.x += cosf(camera.dir.x) * mod * 2.0f;
		camera.vel.y += sinf(camera.dir.x) * mod * 2.0f;
	}
	if(key.s){
		float mod = key.d || key.a ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit_z_down)
			mod *= 0.01f;
		camera.vel.x -= cosf(camera.dir.x) * mod * 2.0f;
		camera.vel.y -= sinf(camera.dir.x) * mod * 2.0f;
	}
	if(key.d){
		float mod = key.s || key.w ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit_z_down)
			mod *= 0.01f;
		camera.vel.x += cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		camera.vel.y += sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
	}
	if(key.a){
		float mod = key.s || key.w ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit_z_down)
			mod *= 0.01f;
		camera.vel.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		camera.vel.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
	}
	vec3mul(&camera.vel,hit_z_down ? GROUND_FRICTION : AIR_FRICTION); 
	camera.vel.z -= 0.0007f;
}

void physics(){
	for(;;){
		key.w = GetKeyState(VK_W) & 0x80;
		key.s = GetKeyState(VK_S) & 0x80;
		key.d = GetKeyState(VK_D) & 0x80;
		key.a = GetKeyState(VK_A) & 0x80;
		camera.tri = (vec4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
		global_tick++;

		if(!setting_fly){
			walkPhysics();
			Sleep(3);
			continue;
		}
		float mod = (GetKeyState(VK_LCONTROL) & 0x80) ? 3.0f : 1.0f;
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
		Sleep(3);
	}
}