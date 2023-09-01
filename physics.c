#include <stdbool.h>
#include <Windows.h>
#include <math.h>

#include "physics.h"
#include "source.h"
#include "tmath.h"
#include "vec3.h"
#include "tree.h"

typedef struct{
	char w;
	char s;
	char d;
	char a;
}key_t;

key_t key;

void checkAxis(vec3 pos,portal_t* portal,float* stair,bool* is_stair,bool* hit,float height,int axis){
	block_t* block = insideBlock(pos);
	if(block){
		if(block->side[axis * 2 + (camera.vel.a[axis] < 0.0f)].portal.block)
			*portal = block->side[axis * 2 + (camera.vel.a[axis] < 0.0f)].portal;
		else{
			*hit = true;
			if(height > -1.2f)
				*is_stair = false;
			else
				*stair = height + 1.9f;
		}
	}
}

void walkPhysics(){
	bool hit[3];
	bool is_stair_x = true,is_stair_y = true;
	float stair_x = 0.0f,stair_y = 0.0f;

	portal_t portal = {.block = 0};

	for(float x = -0.2f;x <= 0.2f;x += 0.05f){
		for(float y = -0.2f;y <= 0.2f;y += 0.05f){
			if(insideBlock((vec3){camera.pos.x + x,camera.pos.y + y,camera.pos.z - 1.7f + camera.vel.z}))
				hit[VEC3_Z] = true;
			if(insideBlock((vec3){camera.pos.x + x,camera.pos.y + y,camera.pos.z + 0.2f + camera.vel.z}))
				hit[VEC3_Z] = true;
		}
	}
	for(float x = -1.7f;x <= 0.1f;x += 0.05f){
		for(float y = -0.2f;y <= 0.2f;y += 0.05f){
			vec3 pos;
			pos = (vec3){camera.pos.x - 0.2f + camera.vel.x,camera.pos.y + y,camera.pos.z + x};
			checkAxis(pos,&portal,&stair_x,&is_stair_x,&hit[VEC3_X],x,VEC3_X);
			pos = (vec3){camera.pos.x + 0.2f + camera.vel.x,camera.pos.y + y,camera.pos.z + x};
			checkAxis(pos,&portal,&stair_x,&is_stair_x,&hit[VEC3_X],x,VEC3_X);
			pos = (vec3){camera.pos.x + y,camera.pos.y + camera.vel.y - 0.2f,camera.pos.z + x};
			checkAxis(pos,&portal,&stair_y,&is_stair_y,&hit[VEC3_Y],x,VEC3_Y);
			pos = (vec3){camera.pos.x + y,camera.pos.y + camera.vel.y + 0.2f,camera.pos.z + x};
			checkAxis(pos,&portal,&stair_y,&is_stair_y,&hit[VEC3_Y],x,VEC3_Y);
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
	if(hit[VEC3_Z]){
		camera.pos.z -= camera.vel.z;
		camera.vel.z = 0.0f;
		if(GetKeyState(VK_SPACE) & 0x80)
			camera.vel.z = 0.08f;
	}
	if(portal.block && !hit[portal.side >> 1]){
		portal_t portal_2 = portal.block->side[portal.side].portal;

		int x_table[] = {VEC3_Y,VEC3_X,VEC3_X};
		int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
		int portal_x = x_table[portal_2.side >> 1];
		int portal_y = y_table[portal_2.side >> 1];
		vec3 pos = {portal.block->pos.x,portal.block->pos.y,portal.block->pos.z};
		float block_size = MAP_SIZE / (1 << portal.block->depth) * 2.0f;
		vec3mul(&pos,block_size);
		vec2 uv;
		camera.pos.a[portal_x] = pos.a[portal_x] + camera.pos.a[portal_x] - portal_2.block->pos.a[portal_x] * block_size;
		camera.pos.a[portal_y] = pos.a[portal_y] + camera.pos.a[portal_y] - portal_2.block->pos.a[portal_y] * block_size;
		camera.pos.a[portal.side >> 1] = pos.a[portal.side >> 1];
		
		//flip direction vector
		if(portal.side == portal_2.side){
			camera.vel.a[portal_2.side >> 1] = -camera.vel.a[portal_2.side >> 1];
			
			camera.dir.a[portal_2.side >> 2] -= M_PI;
		}

		//rotate direction vector 90 degrees
		else if(portal.side >> 1 != portal_2.side >> 1){
			float temp = camera.vel.a[portal_2.side >> 1] * -((portal_2.side & 1) * 2 - 1);
			camera.vel.a[portal_2.side >> 1] = -camera.vel.a[portal.side >> 1] * ((portal.side & 1) * 2 - 1);
			camera.vel.a[portal.side >> 1] = temp;

			if(!(portal.side >> 2))
				camera.dir.x += (M_PI * 0.5f) * (portal_2.side & 1) * 0.4f - 0.2f;
		}
		camera.pos.a[portal_2.side >> 1] += (portal_2.side & 1) * 0.4f - 0.2f;
	}
	if(key.w){
		float mod = key.d || key.a ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit[VEC3_Z])
			mod *= 0.01f;
		camera.vel.x += cosf(camera.dir.x) * mod * 2.0f;
		camera.vel.y += sinf(camera.dir.x) * mod * 2.0f;
	}
	if(key.s){
		float mod = key.d || key.a ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit[VEC3_Z])
			mod *= 0.01f;
		camera.vel.x -= cosf(camera.dir.x) * mod * 2.0f;
		camera.vel.y -= sinf(camera.dir.x) * mod * 2.0f;
	}
	if(key.d){
		float mod = key.s || key.w ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit[VEC3_Z])
			mod *= 0.01f;
		camera.vel.x += cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		camera.vel.y += sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
	}
	if(key.a){
		float mod = key.s || key.w ? MV_WALKSPEED * MV_DIAGONAL : MV_WALKSPEED;
		if(!hit[VEC3_Z])
			mod *= 0.01f;
		camera.vel.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		camera.vel.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
	}
	vec3mul(&camera.vel,hit[VEC3_Z] ? GROUND_FRICTION : 0.995f); 
	camera.vel.z -= 0.0015f;
}

void physics(){
	for(;;){
		key.w = GetKeyState(VK_W) & 0x80;
		key.s = GetKeyState(VK_S) & 0x80;
		key.d = GetKeyState(VK_D) & 0x80;
		key.a = GetKeyState(VK_A) & 0x80;
		camera.tri = (vec4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
		global_tick++;
		if(MV_MODE){
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