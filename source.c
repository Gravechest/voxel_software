#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "tmath.h"
#include "vec2.h"
#include "vec3.h"
#include "dda.h"
#include "source.h"
#include "memory.h"
#include "physics.h"
#include "lighting.h"
#include "tree.h"
#include "opengl.h"
#include "world_triangles.h"
#include "fog.h"
#include "sound.h"
#include "entity.h"
#include "player.h"
#include "powergrid.h"
#include "gui.h"
#include "cellular_automata.h"

#include <gl/GL.h>

#pragma comment(lib,"opengl32")

DYNAMIC_ARRAY(furnace_array,uint32_t);

vec3_t normal_table[] = {
	{-1.0f,0.0f,0.0f},
	{1.0f,0.0f,0.0f},
	{0.0f,-1.0f,0.0f},
	{0.0f,1.0f,0.0f},
	{0.0f,0.0f,-1.0f},
	{0.0f,0.0f,	1.0f}
};

bool lighting_thread_done = true;

void* physics_thread; 
void* thread_render;  
void* entity_thread;

vec3_t music_box_pos;

bool context_is_gl;

uint64_t global_tick;

int proc(HWND hwnd,uint32_t msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = (WNDPROC)proc,.lpszClassName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,32,BI_RGB};
void* window;
void* context;
vram_t vram;

ivec2 window_size;

#define SPAWN_POS (vec3_t){MAP_SIZE + 2.0f,MAP_SIZE + 2.0f,MAP_SIZE + 0.5f}

camera_t camera = {
	.pos.z = MAP_SIZE + 2.0f,
	.pos.x = MAP_SIZE + 0.5f,
	.pos.y = MAP_SIZE + 0.5f,
	.exposure = 1.0f
};

float camera_exposure = 1.0f;

int edit_depth = 7;

bool is_rendering;

int test_bool = 0;

uint32_t light_new_stack_ptr;
light_new_stack_t light_new_stack[LIGHT_NEW_STACK_SIZE];

char frame_ready;

bool setting_smooth_lighting = true;
bool setting_fly = false;

bool player_gamemode = GAMEMODE_CREATIVE;

pixel_t* texture_atlas;

uint32_t inventory_select = 1;

bool in_console;

vec3_t sphere_vertex[256] = {
	{0.942809f, 0.0f, -0.333333f},
	{-0.471405f, 0.816497f, -0.333333f},
	{-0.471405f, -0.816497f, -0.333333f},
	{0.0f, 0.0f, 1.0f}
};

uint8_t sphere_indices[256][3] = {
	{2,1,0},
	{0,1,3},
	{3,2,0},
	{1,2,3},
};

uint32_t border_block_table[][4] = {
	{0,2,4,6},
	{1,3,5,7},
	{0,1,4,5},
	{2,3,6,7},
	{0,1,2,3},
	{4,5,6,7}
};

int craft_ammount = 1;

void increaseCraftAmmount(){
	if(craft_ammount > (1 << 16))
		return;
	craft_ammount <<= 2;
}

void decreaseCraftAmmount(){
	if(craft_ammount == 1)
		return;
	craft_ammount >>= 2;
}

craftrecipy_t craft_recipy[] = {
	[RECIPY_IRON] = {
		.name = "furnace",
		.result = BLOCK_IRON,
		.cost[0] = {.type = BLOCK_IRON_ORE,.ammount = 16},
		.cost[1] = {.type = BLOCK_COAL,.ammount = 1},
		.duration = 1.0f,
	},
	[RECIPY_COPPER] = {
		.name = "furnace",
		.result = BLOCK_COPPER,
		.cost[0] = {.type = BLOCK_COPPER_ORE,.ammount = 16},
		.cost[1] = {.type = BLOCK_COAL,.ammount = 1},
		.duration = 1.0f,
	},
	[RECIPY_FURNACE] = {
		.name = "furnace",
		.result = BLOCK_FURNACE,
		.cost[0] = {.type = BLOCK_STONE,.ammount = 24},
		.cost[1] = {.type = ITEM_VOID},
		.duration = 1.0f,
	},
	[RECIPY_TORCH] = {
		.name = "torch",
		.result = BLOCK_TORCH,
		.cost[0] = {.type = BLOCK_COAL,.ammount = 1},
		.cost[1] = {.type = BLOCK_STONE,.ammount = 4},
		.duration = 1.0f,
	},
	[RECIPY_ELEKTRIC1] = {
		.name = "elektric 1",
		.result = BLOCK_ELEKTRIC_1,
		.cost[0] = {.type = BLOCK_STONE,.ammount = 16},
		.cost[1] = {.type = ITEM_VOID},
		.duration = 1.0f,
	},
	[RECIPY_RESEARCH1] = {
		.name = "research 1",
		.result = 0,
		.cost[0] = {.type = ITEM_VOID},
		.cost[1] = {.type = ITEM_VOID},
		.duration = 1.0f,
	},
	[RECIPY_BASIC] = {
		.name = "basic",
		.result = BLOCK_BASIC,
		.cost[0] = {.type = BLOCK_STONE,.ammount = 16},
		.cost[1] = {.type = ITEM_VOID},
		.duration = 1.0f,
	},
	[RECIPY_GENERATOR] = {
		.name = "generator",
		.result = BLOCK_GENERATOR,
		.cost[0] = {.type = BLOCK_COAL,.ammount = 1},
		.cost[1] = {.type = BLOCK_STONE,.ammount = 4},
		.duration = 1.0f,
	},
};

int block_type = BLOCK_RAIL;

material_t material_array[] = {
	[BLOCK_TRAIN] = {
		.luminance = {1.0f,0.4f,0.2f},
		.texture_pos = TEXTURE_POS_CONVERT(0,0),
		.texture_size = TEXTURE_POS_CONVERT(1,1),
	},
	[BLOCK_RAIL] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = TEXTURE_POS_CONVERT(0,0),
		.texture_size = TEXTURE_POS_CONVERT(1,1),
	},
	[BLOCK_COPPER] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = TEXTURE_POS_CONVERT(128,2048 - 128),
		.texture_size = TEXTURE_POS_CONVERT(128,128),
	},
	[BLOCK_IRON] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = TEXTURE_POS_CONVERT(256 + 128,2048 - 128),
		.texture_size = TEXTURE_POS_CONVERT(128,128),
	},
	[BLOCK_COPPER_ORE] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = TEXTURE_POS_CONVERT(128,2048 - 128),
		.texture_size = TEXTURE_POS_CONVERT(128,128),
		.hardness = 1024.0f
	},
	[BLOCK_IRON_ORE] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = TEXTURE_POS_CONVERT(256 + 128,2048 - 128),
		.texture_size = TEXTURE_POS_CONVERT(128,128),
		.hardness = 1024.0f
	},
	[BLOCK_MINER] = {
		.luminance = {0.5f,0.3f,0.5f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_POWER | MAT_MENU | MAT_POWER_CONSUMER,
		.menu_index = GUI_MINER,
		.power_convert = 0.01f,
		.power_need = 0.2f,
	},
	[BLOCK_FURNACE] = {
		.luminance = {0.5f,0.5f,0.5f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_MENU,
		.menu_index = GUI_FURNACE
	},
	[BLOCK_BASIC] = {
		.luminance = {0.6f,0.5f,0.4f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_MENU,
		.menu_index = GUI_CRAFT_BASIC
	},
	[BLOCK_WATER] = {
		.luminance = {0.9f,0.9f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LIQUID
	},
	[BLOCK_ELEKTRIC_1] = {
		.luminance = {0.9f,0.9f,0.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_MENU,
		.menu_index = GUI_CRAFT_ELEKTRIC1
	},
	[BLOCK_TORCH] = {
		.luminance = {6.0f,2.0f,0.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_ITEM | MAT_LUMINANT
	},
	[BLOCK_SPHERE] = {
		.luminance = {0.9f,0.9f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_PIPE
	},
	[BLOCK_GENERATOR] = {
		.luminance = {0.1f,0.5f,0.5f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_MENU | MAT_POWER,
		.menu_index = GUI_GENERATOR,
	},
	[BLOCK_COAL] = {
		.luminance = {0.1f,0.1f,0.1f},
		.texture_pos = TEXTURE_POS_CONVERT(256,2048 - 128),
		.texture_size = TEXTURE_POS_CONVERT(128,128),
		.hardness = 256.0f
	},
	[BLOCK_EMIT_RED] = {
		.luminance = {9.0f,3.0f,3.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LUMINANT
	},
	[BLOCK_EMIT_GREEN] = {
		.luminance = {3.0f,9.0f,3.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LUMINANT
	},
	[BLOCK_EMIT_BLUE] = {
		.luminance = {3.0f,3.0f,9.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LUMINANT
	},
	[BLOCK_RED] = {
		.luminance = {0.9f,0.3f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_GREEN] = {
		.luminance = {0.3f,0.9f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_BLUE] = {
		.luminance = {0.3f,0.3f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_WHITE] = {
		.luminance = {0.9f,0.9f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_POWDER] = {
		.luminance = {0.1f,0.6f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_POWDER
	},
	[BLOCK_BATTERY] = {
		.luminance = {0.3f,0.3f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_POWER
	},
	[BLOCK_WIRE] = {
		.luminance = {0.3f,0.9f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_POWER
	},
	[BLOCK_LAMP] = {
		.luminance = {0.9f,0.9f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_POWER | MAT_POWER_CONSUMER,
		.power_convert = 200.0f,
		.power_need = 0.02f,
	},
	[BLOCK_SWITCH] = {
		.luminance = {0.5f,0.5f,0.5f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.light_emit = 5.0f,
		.flags = MAT_POWER,
	},
	[BLOCK_STONE] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = TEXTURE_POS_CONVERT(0,2048 - 128),
		.texture_size = TEXTURE_POS_CONVERT(128,128),
		.hardness = 1.0f,
	},
	[BLOCK_SKIN] = {
		.luminance = {1.0f,0.8f,0.8f},
		.texture_pos = {1.0f / 6.0f,0.0f},
		.texture_size = {1.0f / 6.0f,0.5f},
		.flags = MAT_REFLECT
	},
	[BLOCK_SPAWNLIGHT] = {
		.luminance = {3.0f,3.0f,3.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LUMINANT
	},
};

float sdCube(vec3_t point,vec3_t cube,float cube_size){
	vec3_t distance;
	distance.x = tMaxf(cube.x - cube_size * 0.5f - point.x,-(cube.x + cube_size * 0.5f - point.x));
	distance.y = tMaxf(cube.y - cube_size * 0.5f - point.y,-(cube.y + cube_size * 0.5f - point.y));
	distance.z = tMaxf(cube.z - cube_size * 0.5f - point.z,-(cube.z + cube_size * 0.5f - point.z));
	distance.x = tMaxf(distance.x,0.0f);
	distance.y = tMaxf(distance.y,0.0f);
	distance.z = tMaxf(distance.z,0.0f);
	return sqrtf(distance.x * distance.x + distance.y * distance.y + distance.z * distance.z);
}

void spawnLuminantParticle(vec3_t pos,vec3_t dir,vec3_t luminance,uint32_t fuse){
	entity_t* entity = getNewEntity();
	entity->pos = pos;
	entity->dir = dir;
	entity->color = luminance;
	entity->type = 1;
	entity->fuse = fuse;
}

void spawnEntity(vec3_t pos,vec3_t dir,uint32_t type){
	entity_t* entity = getNewEntity();
	entity->pos = pos;
	entity->dir = dir;
	entity->type = type;
	entity->fuse = 0;
	switch(type){
	case 0:
		entity->color = (vec3_t){0.5f,0.5f,0.5f};
		break;
	case 1:
		entity->color = TRND1 < 0.5f ? (vec3_t){0.2f,1.0f,0.2f} : (vec3_t){0.2f,0.2f,1.0f};
		break;
	}
}

texture_t loadBMP(char* name){
	void* h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if(!h)
		return;
	int fsize = GetFileSize(h,0);
	int offset;
	SetFilePointer(h,0x0a,0,0);
	ReadFile(h,&offset,4,0,0);
	SetFilePointer(h,offset,0,0);
	int size = sqrtf((fsize-offset) / 3);
	texture_t texture;
	texture.data = tMalloc(sizeof(pixel_t) * size * size * 2);
	texture.data_rev = tMalloc(sizeof(pixel_t) * size * size * 2);
	texture.size = size;
	_BitScanReverse(&texture.mipmap_level_count,size);
	 
	char* temp = tMalloc(fsize + 5);
	ReadFile(h,temp,fsize-offset,0,0);
	int fp = 0;
	for(;fp < texture.size * texture.size;fp++){
		texture.data[fp].r = temp[fp * 3 + 0];
		texture.data[fp].g = temp[fp * 3 + 1];
		texture.data[fp].b = temp[fp * 3 + 2];
	}
	fp = 0;
	for(;fp < texture.size * texture.size;fp++){
		texture.data_rev[fp].b = temp[fp * 3 + 0];
		texture.data_rev[fp].g = temp[fp * 3 + 1];
		texture.data_rev[fp].r = temp[fp * 3 + 2];
	}
	offset = 0;

	//generate mipmaps
	for(int size = texture.size >> 1;size;){	
		for(int x = 0;x < size;x++){
			for(int y = 0;y < size;y++){
				ivec3 color = {0,0,0};
				int zoom_up = (x * 2) * (size * 2) + y * 2;
				int pos[] = {zoom_up,zoom_up + 1,zoom_up + size * 2,zoom_up + size * 2 + 1};
				for(int j = 0;j < 4;j++){
					color.x += texture.data[pos[j] + offset].r;
					color.y += texture.data[pos[j] + offset].g;
					color.z += texture.data[pos[j] + offset].b;
				}
				texture.data[++fp] = (pixel_t){color.x >> 2,color.y >> 2,color.z >> 2};
			}
		}
		offset += (size * 2) * (size * 2);
		size >>= 1;
	}

	tFree(temp);
	CloseHandle(h);
	return texture;
}

vec2_t sampleCube(vec3_t v,uint32_t* faceIndex){
	float ma;
	vec2_t uv;
	vec3_t vAbs = vec3absR(v);
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y){
		*faceIndex = (v.z < 0.0f) + 4;
		ma = 0.5f / vAbs.z;
		uv = (vec2_t){v.z < 0.0f ? -v.x : v.x, -v.y};
		return (vec2_t){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
	}
	if(vAbs.y >= vAbs.x){
		*faceIndex = (v.y < 0.0f) + 2;
		ma = 0.5f / vAbs.y;
		uv = (vec2_t){v.x, v.y < 0.0f ? -v.z : v.z};
		return (vec2_t){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
	}
	*faceIndex = v.x < 0.0f;
	ma = 0.5f / vAbs.x;
	uv = (vec2_t){v.x < 0.0f ? v.z : -v.z, -v.y};
	return (vec2_t){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
}

vec3_t getLookAngle(vec2_t angle){
	vec3_t ang;
	ang.x = cosf(angle.x) * cosf(angle.y);
	ang.y = sinf(angle.x) * cosf(angle.y);
	ang.z = sinf(angle.y);
	return ang;
}

vec3_t getRayAngle(vec2_t direction,vec2_t pos){
	vec4 tri = {cosf(direction.x),sinf(direction.x),cosf(direction.y),sinf(direction.y)};
	vec3_t ray_ang;
	float pixelOffsetY = pos.x;
	float pixelOffsetX = pos.y;
	ray_ang.x = tri.x * tri.z;
	ray_ang.y = tri.y * tri.z;
	ray_ang.x -= tri.x * tri.w * pixelOffsetY;
	ray_ang.y -= tri.y * tri.w * pixelOffsetY;
	ray_ang.x -= tri.y * pixelOffsetX;
	ray_ang.y += tri.x * pixelOffsetX;
	ray_ang.z = tri.w + tri.z * pixelOffsetY;
	return ray_ang;
}

vec3_t getRayAngleCamera(uint32_t x,uint32_t y){
	vec3_t ray_ang;
	float pxY = (((float)x * 2.0f * (1.0f / window_size.x)) - 1.0f);
	float pxX = (((float)y * 2.0f * (1.0f / window_size.y)) - 1.0f);
	float pixelOffsetY = pxY * (1.0f / (FOV.x / window_size.x * 2.0f));
	float pixelOffsetX = pxX * (1.0f / (FOV.y / window_size.y * 2.0f));
	ray_ang.x = camera.tri.x * camera.tri.z;
	ray_ang.y = camera.tri.y * camera.tri.z;
	ray_ang.x -= camera.tri.x * camera.tri.w * pixelOffsetY;
	ray_ang.y -= camera.tri.y * camera.tri.w * pixelOffsetY;
	ray_ang.x -= camera.tri.y * pixelOffsetX;
	ray_ang.y += camera.tri.x * pixelOffsetX;
	ray_ang.z = camera.tri.w + camera.tri.z * pixelOffsetY;
	return ray_ang;
}

vec3_t getRayAnglePlane(vec3_t normal,float x,float y){
	vec2_t r;
	vec3_t v_x;
	int z_biggest = tAbsf(normal.x) < tAbsf(normal.z) && tAbsf(normal.y) < tAbsf(normal.z);
	if(z_biggest){
		r = vec2rotR((vec2_t){normal.x,normal.z},M_PI * 0.5f);
		v_x = (vec3_t){r.x,normal.y,r.y};
	}
	else{
		r = vec2rotR((vec2_t){normal.x,normal.y},M_PI * 0.5f);
		v_x = (vec3_t){r.x,r.y,normal.z};
	}
	vec3_t v_y = vec3cross(normal,v_x);	
	vec3addvec3(&normal,vec3mulR(v_x,x));
	vec3addvec3(&normal,vec3mulR(v_y,y));
	return vec3normalizeR(normal);
}

vec3_t pointToScreenRenderer(vec3_t point){
	vec3_t screen_point;
	vec3_t pos = vec3subvec3R(point,camera.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec3_t){0.0f,0.0f,0.0f};
	
	screen_point.x = pos.z * (FOV.x / window_size.x * 2.0f);
	screen_point.y = pos.y * (FOV.y / window_size.y * 2.0f);
	screen_point.z = pos.x;
	return screen_point;
}

vec3_t pointToScreenZ(vec3_t point){
	vec3_t screen_point;
	vec3_t pos = vec3subvec3R(point,camera.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec3_t){0.0f,0.0f,0.0f};

	screen_point.x = (pos.z / pos.x * (FOV.x / window_size.x)) * window_size.x + window_size.x / 2.0f;
	screen_point.y = (pos.y / pos.x * (FOV.y / window_size.y)) * window_size.y + window_size.y / 2.0f;
	screen_point.z = pos.x;
	return screen_point;
}

vec2_t pointToScreen(vec3_t point){
	vec2_t screen_point;
	vec3_t pos = vec3subvec3R(point,camera.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec2_t){0.0f,0.0f};
	
	screen_point.x = FOV.x * pos.z / pos.x + window_size.x / 2.0f;
	screen_point.y = FOV.y * pos.y / pos.x + window_size.y / 2.0f;
	return screen_point;
}

plane_ray_t getPlane(vec3_t pos,vec3_t dir,uint32_t side,float block_size){
	switch(side){
	case VEC3_X: return (plane_ray_t){{pos.x + (dir.x < 0.0f) * block_size,pos.y,pos.z},{(dir.x < 0.0f) ? 1.0f : -1.0f,0.0f,0.0f},VEC3_Y,VEC3_Z,VEC3_X};
	case VEC3_Y: return (plane_ray_t){{pos.x,pos.y + (dir.y < 0.0f) * block_size,pos.z},{0.0f,(dir.y < 0.0f) ? 1.0f : -1.0f,0.0f},VEC3_X,VEC3_Z,VEC3_Y};
	case VEC3_Z: return (plane_ray_t){{pos.x,pos.y,pos.z + (dir.z < 0.0f) * block_size},{0.0f,0.0f,(dir.z < 0.0f) ? 1.0f : -1.0f},VEC3_X,VEC3_Y,VEC3_Z};
	}
}

float rayIntersectPlane(vec3_t pos,vec3_t dir,vec3_t plane){
	return vec3dotR(pos,plane) / vec3dotR(dir,plane);
}
//TODO simplify
void addSubVoxel(int vx,int vy,int vz,int x,int y,int z,int depth,int remove_depth,int block_id){
	if(--depth == -1)
		return;
	bool offset_x = x >> depth & 1;
	bool offset_y = y >> depth & 1;
	bool offset_z = z >> depth & 1;
	for(int i = 0;i < 8;i++){
		int lx = (i & 1 << 2) >> 2;
		int ly = (i & 1 << 1) >> 1;
		int lz = (i & 1 << 0) >> 0;
		if(offset_x == lx && offset_y == ly && offset_z == lz){
			int pos_x = vx + offset_x << 1;
			int pos_y = vy + offset_y << 1;
			int pos_z = vz + offset_z << 1;
			addSubVoxel(pos_x,pos_y,pos_z,x,y,z,depth,remove_depth,block_id);
			continue;
		}
		setVoxelSolid(vx + lx,vy + ly,vz + lz,remove_depth - depth,block_id,0);
	}
}

bool model_mode;

typedef struct hold_structure{
	uint32_t block_id;
	union{
		struct hold_structure* child_s[8];
		struct hold_structure* child[2][2][2];
	};
}hold_structure_t;

hold_structure_t* hold_structure;
 
void holdStructureSet(hold_structure_t* hold,ivec3 pos,uint32_t depth){
	if(hold->block_id == BLOCK_PARENT){
		pos.x <<= 1;
		pos.y <<= 1;
		pos.z <<= 1;
		for(int i = 0;i < 8;i++){
			uint32_t dx = (i & 1) != 0;
			uint32_t dy = (i & 2) != 0;
			uint32_t dz = (i & 4) != 0;
			holdStructureSet(hold->child_s[i],(ivec3){pos.x + dx,pos.y + dy,pos.z + dz},depth + 1);
		}
		return;
	}
	if(hold->block_id == BLOCK_AIR)
		return;
	setVoxelSolid(pos.x,pos.y,pos.z,depth,hold->block_id,0);
}

void rayRemoveVoxel(vec3_t ray_pos,vec3_t direction,int remove_depth){
	traverse_init_t init = initTraverse(ray_pos);
	node_hit_t result = treeRayFlags(ray3Create(init.pos,direction),init.node,ray_pos,TREE_RAY_PLACE);
	if(!result.node)
		return;
	node_t node = *(node_t*)dynamicArrayGet(node_root,result.node);
	uint32_t block_depth = node.depth;
	ivec3 block_pos_i = node.pos;
	
	block_t* block = dynamicArrayGet(block_array,node.index);
	uint32_t material = node.type;

	setVoxelSolid(node.pos.x,node.pos.y,node.pos.z,node.depth,BLOCK_AIR,0);

	float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;

	if(block_depth >= remove_depth)
		return;

	int depht_difference = remove_depth - block_depth;
	vec3_t block_pos = {
		block_pos_i.x << depht_difference,
		block_pos_i.y << depht_difference,
		block_pos_i.z << depht_difference
	};
	int edit_size = 1 << remove_depth;
	vec3_t pos = {
		block_pos_i.x * block_size,
		block_pos_i.y * block_size,
		block_pos_i.z * block_size
	};

	plane_ray_t plane = getPlane(pos,direction,result.side,block_size);
	vec3_t plane_pos = vec3subvec3R(plane.pos,ray_pos);
	float dst = rayIntersectPlane(plane_pos,direction,plane.normal);
	vec3_t hit_pos = vec3addvec3R(camera.pos,vec3mulR(direction,dst));

	vec2_t uv = {
		(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
		(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
	};
	uv.x *= 1 << depht_difference;
	uv.y *= 1 << depht_difference;
	block_pos.a[result.side] += ((direction.a[result.side] < 0.0f) << depht_difference) - (direction.a[result.side] < 0.0f);
	block_pos.a[plane.x] += tFloorf(uv.x);
	block_pos.a[plane.y] += tFloorf(uv.y);
	int x,y,z;
	int depht_difference_size = 1 << depht_difference;
	x = (int)block_pos.x & depht_difference_size - 1;
	y = (int)block_pos.y & depht_difference_size - 1;
	z = (int)block_pos.z & depht_difference_size - 1;

	addSubVoxel(block_pos_i.x << 1,block_pos_i.y << 1,block_pos_i.z << 1,x,y,z,depht_difference,remove_depth,material);
}

void playerShoot(){
	for(int i = 0;i < 12;i++){
		vec2_t rnd_dir = (vec2_t){TRND1 - 0.5f,TRND1 - 0.5f};
		vec3mul(&rnd_dir,0.1f);
		for(int i = 0;i < 10;i++){
			vec3_t dir = getLookAngle(vec2addvec2R(camera.dir,rnd_dir));
			rayRemoveVoxel(camera.pos,dir,LM_MAXDEPTH + 4);
		}
	}
}

enum{
	MENU_BLOCK,
	MENU_CRAFT
};

uint32_t block_menu_block;
int in_menu;

void playerAddBlock(node_t* node,int side,vec3_t dir,int orientation){
	block_t* block = dynamicArrayGet(block_array,node->index);
	ivec3 block_pos_i = node->pos;
	uint32_t block_depth = node->depth;
	block_pos_i.a[side] += (dir.a[side] < 0.0f) * 2 - 1;
	vec3_t block_pos;
	if(block_depth > edit_depth){
		int shift = block_depth - edit_depth;
		block_pos = (vec3_t){
			block_pos_i.x >> shift,
			block_pos_i.y >> shift,
			block_pos_i.z >> shift
		};
	}
	else{
		int shift = edit_depth - block_depth;
		block_pos = (vec3_t){
			block_pos_i.x << shift,
			block_pos_i.y << shift,
			block_pos_i.z << shift
		};
	}
	if(block_depth < edit_depth){
		vec3_t pos;
		float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;
		pos.x = node->pos.x * block_size;
		pos.y = node->pos.y * block_size;
		pos.z = node->pos.z * block_size;
		plane_ray_t plane = getPlane(pos,dir,side,block_size);
		vec3_t plane_pos = vec3subvec3R(plane.pos,camera.pos);
		float dst = rayIntersectPlane(plane_pos,dir,plane.normal);
		vec3_t hit_pos = vec3addvec3R(camera.pos,vec3mulR(dir,dst));
		vec2_t uv = {
			(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
			(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
		};
		uv.x *= 1 << edit_depth - block_depth;
		uv.y *= 1 << edit_depth - block_depth;
		block_pos.a[side] += ((dir.a[side] > 0.0f) << (edit_depth - block_depth)) - (dir.a[side] > 0.0f);
		block_pos.a[plane.x] += tFloorf(uv.x);
		block_pos.a[plane.y] += tFloorf(uv.y);
	}
	if(!model_mode){
		if(player_gamemode == GAMEMODE_SURVIVAL){
			uint32_t cost = 1 << tMax((12 - edit_depth) * 3,0);
			if(cost > inventory_slot[inventory_select].ammount)
				return;
			setVoxelSolid(block_pos.x,block_pos.y,block_pos.z,edit_depth,inventory_slot[inventory_select].type,orientation);
			itemChange(&inventory_slot[inventory_select],-cost);
			return;
		}
		if(material_array[block_type].flags & MAT_LIQUID){
			setVoxel(block_pos.x,block_pos.y,block_pos.z,edit_depth,block_type,0.8f,orientation);
			return;
		}
		setVoxelSolid(block_pos.x,block_pos.y,block_pos.z,edit_depth,block_type,orientation);
		return;
	}
	holdStructureSet(hold_structure,(ivec3){block_pos.x,block_pos.y,block_pos.z},edit_depth);
}

float rayIntersectSphere(vec3_t ray_pos,vec3_t sphere_pos,vec3_t ray_dir,float radius){
    vec3_t rel_pos = vec3subvec3R(ray_pos,sphere_pos);
	float a = vec3dotR(ray_dir,ray_dir);
	float b = 2.0f * vec3dotR(rel_pos,ray_dir);
	float c = vec3dotR(rel_pos,rel_pos) - radius * radius;
	float h = b * b - 4.0f * a * c;
    if(h < 0.0f)
		return -1.0f;
	return (-b - sqrtf(h)) / (2.0f * a);
}

void clickLeft(){
	vec3_t dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRayFlags(ray3Create(init.pos,dir),init.node,camera.pos,TREE_RAY_PLACE);
	if(!result.node)
		return;
	node_t* node = dynamicArrayGet(node_root,result.node);
	block_t* block = dynamicArrayGet(block_array,node->index);
	material_t material = material_array[node->type];
	entity_hit_t hit = rayEntityIntersection(camera.pos,dir);
	if(hit.entity != -1){
		entity_t* entity = &entity_array[hit.entity];
		if(entity->type == ENTITY_TRAIN){
			camera.pos = entity->pos;
			player.train = hit.entity;
		}
		return;
	}
	if(material.flags & MAT_MENU){
		block_menu_block = result.node;
		dynamic_array_t gui = gui_array[material.menu_index];
		for(int i = 0;i < gui.size;i++){
			gui_t* element = dynamicArrayGet(gui,i);
			if(element->type == GUI_ELEMENT_ITEM)
				element->item = &block->inventory[element->value];
			if(element->type == GUI_ELEMENT_PROGRESS)
				element->progress = &block->progress;
		}
		if(node->type != BLOCK_MINER){
			if(result.side != VEC3_Z)
				block->gui_side = result.side * 2 + (dir.a[result.side] > 0.0f); 
		}
		return;
	}
	if(node->type == BLOCK_RAIL && block_type == BLOCK_TRAIN){
		entity_t* entity = getNewEntity();
		air_t* air = dynamicArrayGet(air_array,node->index);
		float offset = getBlockSize(node->depth) * 0.5f;
		entity->pos = vec3addvec3R(getPosFromGridPos(node->pos,node->depth),(vec3_t){offset,offset,offset * 0.5f});
		entity->train_orientation = air->orientation & 0b000011 ? 0 : 2;
		entity->type = ENTITY_TRAIN;
		entity->dir = VEC3_ZERO;
		entity->texture_pos = (vec2_t)TEXTURE_POS_CONVERT(0,2048 - 128 - 32);
		entity->texture_size = (vec2_t)TEXTURE_POS_CONVERT(32,32);
		entity->size = offset * 0.5f;
		entity->flags = 0;
		entity->color = (vec3_t){1.0f,1.0f,1.0f};
		return;
	}
	if(node->type == BLOCK_SWITCH){
		block_t* block = dynamicArrayGet(block_array,node->index);
		block->on ^= true;
		if(result.side != VEC3_Z)
			block->gui_side = result.side * 2 + (dir.a[result.side] > 0.0f);
		return;
	}
	int orientation;
	if(node->type == BLOCK_MINER){
		orientation = result.side * 2 + (dir.a[result.side] > 0.0f);
	}
	else{
		if(result.side != VEC3_Z)
			orientation = result.side * 2 + (dir.a[result.side] > 0.0f);
		else
			orientation = 0;
	}
	if(player_gamemode == GAMEMODE_SURVIVAL){
		if(inventory_slot[inventory_select].type == ITEM_VOID){
			if(player.attack_animation.state)
				return;
			player.attack_animation.state = ANIMATION_ACTIVE;
			playerAttack();
			return;
		}
		playerAddBlock(node,result.side,dir,orientation);
		return;
	}
	playerAddBlock(node,result.side,dir,orientation);
}

float block_break_progress;
ivec3 block_break_pos;

void playerRemoveBlock(){
	vec3_t dir = getLookAngle(camera.dir);
	switch(player_gamemode){
	case GAMEMODE_CREATIVE:
		rayRemoveVoxel(camera.pos,dir,edit_depth);
		break;
	case GAMEMODE_SURVIVAL:
		break;
	}
}

static bool r_click;
static bool l_click;

void fillHoldStructure(hold_structure_t* hold,uint32_t node_ptr){
	node_t node = *(node_t*)dynamicArrayGet(node_root,node_ptr);
	hold->block_id = node.type;
	if(node.type == BLOCK_PARENT){
		for(int i = 0;i < 8;i++){
			hold->child_s[i] = tMallocZero(sizeof(hold_structure_t));
			fillHoldStructure(hold->child_s[i],node.child_s[i]);
		}
	}
}

void holdStructureFree(hold_structure_t* hold){
	if(hold->block_id == BLOCK_PARENT){
		for(int i = 0;i < 8;i++){
			holdStructureFree(hold->child_s[i]);
			hold->child_s[i] = 0;
		}
	}
	memset(hold,0,sizeof(hold_structure_t));
	tFree(hold);
}

int rotate_model_table[][8] = {
	{4,0,6,2,5,1,7,3}
};

void rotateModel(hold_structure_t* model,hold_structure_t* buffer,int rotate_direction){
	buffer->block_id = model->block_id;
	if(model->block_id == BLOCK_PARENT){
		for(int i = 0;i < 8;i++){
			int buffer_index = rotate_model_table[rotate_direction][i];
			buffer->child_s[buffer_index] = tMallocZero(sizeof(hold_structure_t));
			rotateModel(model->child_s[i],buffer->child_s[buffer_index],rotate_direction);
		}
	}
}

uint32_t tNoise(uint32_t seed){
	seed += seed << 10;
	seed ^= seed >> 6;
	seed += seed << 3;
	seed ^= seed >> 11;
	seed += seed << 15;
	return seed;
}

void createSurvivalWorld(){
	/*
	int depth = 7;
	int m = 1 << depth;

	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			for(int z = 0;z < m;z++){
				int table[] = {1,2,3,5,8,13,21,34};
				int value = 0;
				for(int i = 0;i < 8;i++){
					value += tNoise((x / table[i]) * m * m + (y / table[i]) * m + (z / table[i])) % 0x2f;
				}
				if(value > 180)
					setVoxelSolid(x,y,z,depth,BLOCK_STONE,0);
			}
		}
	}
	*/
	
	int depth = 7;
	int m = 1 << depth;

	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			for(int z = 0;z < m;z++){
				bool x_condition = x >= m / 2 && x < m / 2 + 3;
				bool y_condition = y >= m / 2 && y < m / 2 + 3;
				bool z_condition = z >= m / 2 && z < m / 2 + 3;
				if(x_condition && y_condition && z_condition){
					continue;
				}
				int roll = TRND1 * 2500;
				roll -= 80;
				if(roll < 0){
					setVoxelSolid(x,y,z,depth,BLOCK_COAL,0);
					continue;
				}
				roll -= 20;
				if(roll < 0){
					setVoxelSolid(x,y,z,depth,BLOCK_COPPER_ORE,0);
					continue;
				}
				roll -= 20;
				if(roll < 0){
					setVoxelSolid(x,y,z,depth,BLOCK_IRON_ORE,0);
					continue;
				}
				setVoxelSolid(x,y,z,depth,BLOCK_STONE,0);
			}
		}
	}
	setVoxelSolid(m + 2,m + 2,m,depth + 1,BLOCK_SPAWNLIGHT,0);
	setVoxelSolid(m + 2,m + 3,m,depth + 1,BLOCK_SPAWNLIGHT,0);
	setVoxelSolid(m + 3,m + 2,m,depth + 1,BLOCK_SPAWNLIGHT,0);
	setVoxelSolid(m + 3,m + 3,m,depth + 1,BLOCK_SPAWNLIGHT,0);
	
}

void createFlatWorld(){
	int depth = 7;
	int m = 1 << depth;
	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			setVoxelSolid(x,y,0,depth,BLOCK_STONE,0);
		}
	}
}

void eraseWorld(){
	dynamicArrayReset(&node_root);
	dynamicArrayReset(&air_array);
	dynamicArrayReset(&block_array);
	camera.pos = SPAWN_POS;
	vec3_t spawnPos = (vec3_t){camera.pos.x,camera.pos.y,MAP_SIZE};
	float distance = rayGetDistance(spawnPos,(vec3_t){0.01f,0.01f,-1.0f}) - 15.0f;
	spawnPos.z -= distance;
	camera.pos = spawnPos;
}

#define CONSOLE_BUFFER_SIZE 40

char console_buffer[CONSOLE_BUFFER_SIZE];
int console_cursor;
float test_float;

enum{
	RENDER_MODE_NORMAL,
	RENDER_MODE_WIREFRAME,
	RENDER_MODE_MASK,
	RENDER_MODE_TREE_ITERATION,
};

int render_mode;
int render_mode_change;

static bool commandCompare(char* command,char* str){
	for(int i = 0;*command != ' ' && *command != '\0' && *str != '\0';command++,str++){
		if(*command != *str)
			return false;
	}
	return true;
}

ivec2 window_offset;

void executeCommand(){
	if(commandCompare(console_buffer,"FULLSCREEN")){
		printf("%i\n",GetWindowLongPtrA(window,GWL_STYLE));
		SetWindowLongPtrA(window,GWL_STYLE,WS_VISIBLE);
		SetWindowPos(window,0,window_offset.y,window_offset.x,window_size.y,window_size.x,0);
	}
	if(commandCompare(console_buffer,"WINDOW_OFFSET_Y")){
		window_offset.y = atoi(console_buffer + 16);
		SetWindowPos(window,0,window_offset.y,window_offset.x,window_size.y,window_size.x,0);
		glViewport(0,0,window_size.y,window_size.x);
	}
	if(commandCompare(console_buffer,"WINDOW_OFFSET_X")){
		window_offset.x = atoi(console_buffer + 16);
		SetWindowPos(window,0,window_offset.y,window_offset.x,window_size.y,window_size.x,0);
		glViewport(0,0,window_size.y,window_size.x);
	}
	if(commandCompare(console_buffer,"WINDOW_SIZE")){
		window_size.y = atoi(console_buffer + 12);
		window_size.x = window_size.y * RD_RATIO;
		SetWindowPos(window,0,window_offset.y,window_offset.x,window_size.y,window_size.x,0);
		glViewport(0,0,window_size.y,window_size.x);
	}
	if(commandCompare(console_buffer,"RD_WIREFRAME"))
		render_mode_change = RENDER_MODE_WIREFRAME;
	if(commandCompare(console_buffer,"RD_TREE_ITERATION"))
		render_mode_change = RENDER_MODE_TREE_ITERATION;
	if(commandCompare(console_buffer,"RD_MASK"))
		render_mode_change = RENDER_MODE_MASK;
	if(commandCompare(console_buffer,"RD_NORMAL"))
		render_mode_change = RENDER_MODE_NORMAL;
	if(commandCompare(console_buffer,"CREATE_FLAT")){
		eraseWorld();
		createFlatWorld();
	}
	if(commandCompare(console_buffer,"CREATE_SURVIVAL")){
		eraseWorld();
		createSurvivalWorld();
	}
	if(commandCompare(console_buffer,"TEST_FLOAT")){
		test_float = atof(console_buffer + 11);
	}
	if(commandCompare(console_buffer,"GAMEMODE")){
		player_gamemode ^= true;
	}
	if(commandCompare(console_buffer,"O2")){
		node_t* node = dynamicArrayGet(node_root,treeTraverse(camera.pos));
		if(node->type == BLOCK_AIR)
			((air_t*)dynamicArrayGet(air_array,node->index))->o2 += 0.1f;
	}
	if(commandCompare(console_buffer,"QUIT"))
		ExitProcess(0);
	if(commandCompare(console_buffer,"FLY"))
		setting_fly ^= true;
	memset(console_buffer,0,CONSOLE_BUFFER_SIZE);
	console_cursor = 0;
}

bool pointInRectangle(vec2_t point,vec2_t rect_pos,vec2_t rect_size){
	vec2_t relative = vec2subvec2R(point,rect_pos);
	bool x = relative.x > 0.0f && relative.x < rect_size.x;
	bool y = relative.y > 0.0f && relative.y < rect_size.y;
	return x && y;
}

bool pointInSquare(vec2_t point,vec2_t square_pos,float square_size){
	vec2_t relative = vec2subvec2R(square_pos,point);
	bool x = relative.x > 0.0f && relative.x < square_size;
	bool y = relative.y > 0.0f && relative.y < square_size;
	return x && y;
}

void playerCraft(int recipy_index){
	craftrecipy_t recipy = craft_recipy[recipy_index];
	item_t requirements[2] = {recipy.cost[0],recipy.cost[1]};
	requirements[0].ammount *= craft_ammount;
	requirements[1].ammount *= craft_ammount;
	if(recipy.result == ITEM_VOID)
		return;
	//check if enought materials to craft
	for(int r = 0;r < 2;r++){
		if(requirements[r].type == ITEM_VOID)
			continue;
		for(int i = 0;i < 9;i++){
			if(requirements[r].type == inventory_slot[i].type)
				requirements[r].ammount -= tMin(requirements[r].ammount,inventory_slot[i].ammount);
		}
	}
	if(requirements[0].ammount || requirements[1].ammount)
		return;
	requirements[0] = recipy.cost[0];
	requirements[1] = recipy.cost[1];
	requirements[0].ammount *= craft_ammount;
	requirements[1].ammount *= craft_ammount;
	for(int r = 0;r < 2;r++){
		if(requirements[r].type == ITEM_VOID)
			continue;
		for(int i = 0;i < 8;i++){
			if(requirements[r].type == inventory_slot[i].type)
				itemChange(&inventory_slot[i],-tMin(requirements[r].ammount,inventory_slot[i].ammount));
		}
	}
	inventoryAdd((item_t){recipy.result,craft_ammount});
}

void changeCraftRecipy(int recipy){
	node_t* node = dynamicArrayGet(node_root,block_menu_block);
	block_t* block = dynamicArrayGet(block_array,node->index);
	block->recipy_select = recipy;
}

item_t* item_hold_ptr;
item_t item_hold;

vec2_t getCursorGUI(){
	POINT cursor;
	GetCursorPos(&cursor);
	vec2_t pos = {
		(float)cursor.x / window_size.y * 2.0f - 1.0f,
		-((float)cursor.y / window_size.x * 2.0f - 1.0f)
	};
	return pos;
}

void checkButton(vec2_t point,vec2_t button_pos,button_t button){
	if(pointInRectangle(point,button_pos,button.size)){
		if(button.value == -1)
			button.f_void();
		else
			button.f_int(button.value);
	}
}

bool execute_command;

int proc(HWND hwnd,uint32_t msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_KEYDOWN:
		if(in_console){
			if(wParam >= '0' && wParam <= '9')
				console_buffer[console_cursor++] = wParam;
			else if(wParam >= 'A' && wParam <= 'Z')
				console_buffer[console_cursor++] = wParam;
			else if(wParam == VK_OEM_PERIOD)
				console_buffer[console_cursor++] = '.';
			else if(wParam == VK_SPACE)
				console_buffer[console_cursor++] = ' ';
			else if(wParam == VK_OEM_MINUS && GetKeyState(VK_SHIFT) & 0x80)
				console_buffer[console_cursor++] = '_';
			else if(wParam == VK_OEM_MINUS)
				console_buffer[console_cursor++] = '-';
			else{
				switch(wParam){
				case VK_F3: in_console ^= true; break;
				case VK_RETURN: execute_command = true; break;
				case VK_BACK: 
					if(!console_cursor)
						break;
					console_buffer[--console_cursor] = 0; 
					break;
				}
			}
			break;
		}
		switch(wParam){
		case 'C': 
			in_menu = in_menu == MENU_CRAFT ? MENU_BLOCK : MENU_CRAFT; 
			break;
		case VK_F3: in_console ^= true; break;
		case '1': changeInventorySelect(2); break;
		case '2': changeInventorySelect(3); break;
		case '3': changeInventorySelect(4); break;
		case '4': changeInventorySelect(5); break;
		case '5': changeInventorySelect(6); break;
		case '6': changeInventorySelect(7); break;
		case '7': changeInventorySelect(8); break;
		case 'Q': changeInventorySelect(1); break;
		case 'R':
			if(!model_mode)
				break;
			hold_structure_t* buffer = tMallocZero(sizeof(hold_structure_t));
			rotateModel(hold_structure,buffer,0);
			holdStructureFree(hold_structure);
			hold_structure = buffer;
			break;
		case VK_ESCAPE:;
			if(!in_menu && !block_menu_block)
				break;
			POINT cur;
			RECT offset;
			GetCursorPos(&cur);
			GetWindowRect(window,&offset);
			SetCursorPos(offset.left + window_size.y / 2,offset.top + window_size.x / 2);
			in_menu = 0;
			block_menu_block = 0;
			break;
		case VK_F5:
			spawnEntity(camera.pos,vec3mulR(getLookAngle(camera.dir),0.04f),0);
			break;
		case VK_F9: test_bool ^= true; break;
		case VK_UP: model_mode ^= true; break;
		case VK_DOWN: break;
		case VK_RIGHT:
			block_type++;
			if(block_type > sizeof(material_array) / sizeof(material_t))
				block_type = sizeof(material_array) / sizeof(material_t);
			if(material_array[block_type].fixed_size)
				edit_depth = material_array[block_type].fixed_size;
			break;
		case VK_LEFT:
			block_type--;
			if(block_type < 0)
				block_type = 0;
			if(material_array[block_type].fixed_size)
				edit_depth = material_array[block_type].fixed_size;
			break;
		case VK_ADD:
			if(player_gamemode == GAMEMODE_SURVIVAL){
				uint32_t type = inventory_slot[inventory_select].type;
				if(type != ITEM_VOID && material_array[type].fixed_size)
					break;
			}
			if(material_array[block_type].fixed_size)
				break;
			if(player_gamemode == GAMEMODE_SURVIVAL && edit_depth == DEPTH_MAX)
				break;
			edit_depth++;
			break;
		case VK_SUBTRACT:
			if(player_gamemode == GAMEMODE_SURVIVAL){
				uint32_t type = inventory_slot[inventory_select].type;
				if(type != ITEM_VOID && material_array[type].fixed_size)
					break;
			}
			if(material_array[block_type].fixed_size)
				break;
			if(player_gamemode == GAMEMODE_SURVIVAL && edit_depth == 7)
				break;
			edit_depth--;
			break;
		}
		break;
	case WM_MBUTTONDOWN:
		if(in_menu)
			break;
		vec3_t dir = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(camera.pos);
		node_hit_t result = treeRayFlags(ray3Create(init.pos,dir),init.node,camera.pos,TREE_RAY_LIQUID);
		if(!result.node)
			break;
		if(!(GetKeyState(VK_CONTROL) & 0x80)){
			block_type = ((node_t*)dynamicArrayGet(node_root,result.node))->type;
			break;
		}
		node_t node = *(node_t*)dynamicArrayGet(node_root,result.node);
		float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
		vec3_t block_pos = vec3mulR((vec3_t){node.pos.x,node.pos.y,node.pos.z},block_size);
		plane_ray_t plane = getPlane(block_pos,dir,result.side,block_size);
		vec3_t plane_pos = vec3subvec3R(plane.pos,camera.pos);
		int side = result.side * 2 + (dir.a[result.side] < 0.0f);
		float distance = rayIntersectPlane(plane_pos,dir,plane.normal);
		vec3_t pos = vec3addvec3R(camera.pos,vec3mulR(dir,distance - 0.01f));
		uint32_t node_select = getNodeFromPos(pos,edit_depth);
		if(!node_select)
			break;
		if(hold_structure){
			holdStructureFree(hold_structure);
			hold_structure = 0;
		}
		hold_structure = tMallocZero(sizeof(hold_structure_t));
		fillHoldStructure(hold_structure,node_select);
		model_mode = true;
		break;
	case WM_RBUTTONDOWN:
		if(in_menu)
			break;
		r_click = true;
		break;
	case WM_LBUTTONUP:
		if(item_hold_ptr){
			vec2_t pos = getCursorGUI();
			if(block_menu_block){
				node_t* node = dynamicArrayGet(node_root,block_menu_block);
				dynamic_array_t gui = gui_array[material_array[node->type].menu_index];
				for(int i = 0;i < gui.size;i++){
					gui_t* element = dynamicArrayGet(gui,i);
					if(element->type == GUI_ELEMENT_ITEM){
						if(pointInSquare(pos,vec2addvec2R(element->offset,RD_SQUARE(0.15f)),0.15f)){
							if(element->item->type == item_hold.type)
								itemChange(element->item,item_hold.ammount);
							else if(inventory_slot[i].type != ITEM_VOID)
								*item_hold_ptr = inventory_slot[i];
							*element->item = item_hold;
							item_hold_ptr = 0;
							break;
						}
					}
				}
			}
			if(!item_hold_ptr)
				break;
			for(int i = 0;i < 9;i++){
				vec2_t offset = (vec2_t){-0.3 + i * 0.17f * RD_RATIO,-0.8f};
				if(pointInSquare(pos,offset,0.15f)){
					if(inventory_slot[i].type == item_hold.type)
						itemChange(&inventory_slot[i],item_hold.ammount);
					else if(inventory_slot[i].type != ITEM_VOID)
						*item_hold_ptr = inventory_slot[i];
					inventory_slot[i] = item_hold;
					item_hold_ptr = 0;
					break;
				}
				if(i == 8){
					*item_hold_ptr = item_hold;
					item_hold_ptr = 0;
				}
			}
		}
		break;
	case WM_LBUTTONDOWN:;
		if(in_menu){
			vec2_t pos = getCursorGUI();
			if(in_menu == MENU_CRAFT){
				for(int i = 0;i < gui_array[GUI_CRAFT_MENU].size;i++){
					gui_t* element = dynamicArrayGet(gui_array[GUI_CRAFT_MENU],i);
					if(element->type == GUI_ELEMENT_BUTTON)
						checkButton(pos,element->offset,element->button);
				}
				for(int i = 0;i < 9;i++){
					vec2_t offset = (vec2_t){-0.3 + i * 0.17f * RD_RATIO,-0.8f};
					if(pointInSquare(pos,offset,0.15f)){
						item_hold_ptr = &inventory_slot[i];
						item_hold = inventory_slot[i];
						inventory_slot[i] = (item_t){.type = ITEM_VOID,.ammount = 0};
						break;
					}
				}
			}
			break;
		}
		if(block_menu_block){
			vec2_t pos = getCursorGUI();
			node_t* node = dynamicArrayGet(node_root,block_menu_block);
			dynamic_array_t gui = gui_array[material_array[node->type].menu_index];
			for(int i = 0;i < gui.size;i++){
				gui_t* element = dynamicArrayGet(gui,i);
				if(element->type == GUI_ELEMENT_BUTTON)
					checkButton(pos,element->offset,element->button);
				if(element->type == GUI_ELEMENT_ITEM){
					if(pointInSquare(pos,vec2addvec2R(element->offset,RD_SQUARE(0.15f)),0.15f)){
						item_hold_ptr = element->item;
						item_hold = *element->item;
						*element->item = (item_t){.type = ITEM_VOID,.ammount = 0};
						break;
					}
				}
			}
			for(int i = 0;i < 9;i++){
				vec2_t offset = (vec2_t){-0.3 + i * 0.17f * RD_RATIO,-0.8f};
				if(pointInSquare(pos,offset,0.15f)){
					item_hold_ptr = &inventory_slot[i];
					item_hold = inventory_slot[i];
					inventory_slot[i] = (item_t){.type = ITEM_VOID,.ammount = 0};
					break;
				}
			}
		}
		l_click = true;
		break;
	case WM_MOUSEMOVE:;
		if(in_menu || block_menu_block)
			break;
		POINT cur;
		RECT offset;
		vec2_t r;
		GetCursorPos(&cur);
		GetWindowRect(window,&offset);
		float mx = (float)(cur.x - (offset.left + window_size.y / 2)) * 0.005f;
		float my = (float)(cur.y - (offset.top  + window_size.x / 2)) * 0.005f;
		camera.dir.x += mx;
		camera.dir.y -= my;
		SetCursorPos(offset.left + window_size.y / 2,offset.top + window_size.x / 2);
		physic_mouse_x += mx;
		break;
	case WM_DESTROY: case WM_CLOSE:
		ExitProcess(0);
	}
	return DefWindowProcA(hwnd,msg,wParam,lParam);
}

void generateBlockOutline(){
	vec3_t look_direction = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRay(ray3Create(init.pos,look_direction),init.node,camera.pos);
	if(player_gamemode == GAMEMODE_SURVIVAL){
		if(rayGetDistance(camera.pos,look_direction) > PLAYER_BUILDREACH)
			return;
	}
	if(!result.node)
		return;
	node_t node = *(node_t*)dynamicArrayGet(node_root,result.node);
	block_t* block = dynamicArrayGet(block_array,node.index);
	float edit_size = (float)MAP_SIZE / (1 << edit_depth) * 2.0f;

	uint32_t block_depth = node.depth;
	ivec3 block_pos_i = node.pos;
			
	block_pos_i.a[result.side] += (look_direction.a[result.side] < 0.0f) * 2 - 1;
	vec3_t block_pos;
	if(block_depth > edit_depth){
		int difference = block_depth - edit_depth;
		block_pos = (vec3_t){block_pos_i.x >> difference,block_pos_i.y >> difference,block_pos_i.z >> difference};
	}
	else{
		int difference = edit_depth - block_depth;
		block_pos = (vec3_t){block_pos_i.x << difference,block_pos_i.y << difference,block_pos_i.z << difference};
	}
	vec3_t point[8];
	vec2_t screen_point[8];

	if(block_depth < edit_depth){
		vec3_t pos;
		float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;
		pos.x = node.pos.x * block_size;
		pos.y = node.pos.y * block_size;
		pos.z = node.pos.z * block_size;
		plane_ray_t plane = getPlane(pos,look_direction,result.side,block_size);
		vec3_t plane_pos = vec3subvec3R(plane.pos,camera.pos);
		float dst = rayIntersectPlane(plane_pos,look_direction,plane.normal);
		vec3_t hit_pos = vec3addvec3R(camera.pos,vec3mulR(look_direction,dst));
		vec2_t uv = {
			(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
			(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
		};
		uv.x *= 1 << edit_depth - block_depth;
		uv.y *= 1 << edit_depth - block_depth;
		block_pos.a[result.side] += ((look_direction.a[result.side] > 0.0f) << (edit_depth - block_depth)) - (look_direction.a[result.side] > 0.0f);
		block_pos.a[plane.x] += tFloorf(uv.x);
		block_pos.a[plane.y] += tFloorf(uv.y);
	}

	vec3mul(&block_pos,edit_size);

	point[0] = (vec3_t){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[1] = (vec3_t){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + edit_size};
	point[2] = (vec3_t){block_pos.x + 0.0f,block_pos.y + edit_size,block_pos.z + 0.0f};
	point[3] = (vec3_t){block_pos.x + 0.0f,block_pos.y + edit_size,block_pos.z + edit_size};
	point[4] = (vec3_t){block_pos.x + edit_size,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[5] = (vec3_t){block_pos.x + edit_size,block_pos.y + 0.0f,block_pos.z + edit_size};
	point[6] = (vec3_t){block_pos.x + edit_size,block_pos.y + edit_size,block_pos.z + 0.0f};
	point[7] = (vec3_t){block_pos.x + edit_size,block_pos.y + edit_size,block_pos.z + edit_size};
	vec3_t triang[8];
	int triangle_c = 0;
	triangle_t triangle[3 * 6];
	for(int i = 0;i < 8;i++){
		vec3_t screen = pointToScreenRenderer(point[i]);
		vec3_t s_point;
		s_point.z = screen.z;
		s_point.y = screen.x;
		s_point.x = screen.y;
		triang[i] = s_point;
	}
	int table[][4] = {
		{0,1,2,3},
		{4,5,6,7},
		{0,1,4,5},
		{2,3,6,7},
		{0,2,4,6},
		{1,3,5,7},
	};
	float fluctuate = cosf(global_tick * 0.003f) * 0.5f + 0.5f;
	for(int i = 0;i < 6;i++){
		//backface culling
		if(vec3dotR(normal_table[i],vec3subvec3R(camera.pos,point[table[i][0]])) < 0.0f)
			continue;
		triangle[triangle_c++] = (triangle_t){triang[table[i][0]],(vec2_t){0.0f,0.0f},  (vec3_t){fluctuate,fluctuate,fluctuate},1.0f};
		triangle[triangle_c++] = (triangle_t){triang[table[i][1]],(vec2_t){0.01f,0.0f}, (vec3_t){fluctuate,fluctuate,fluctuate},1.0f};
		triangle[triangle_c++] = (triangle_t){triang[table[i][3]],(vec2_t){0.01f,0.01f},(vec3_t){fluctuate,fluctuate,fluctuate},1.0f};
		triangle[triangle_c++] = (triangle_t){triang[table[i][0]],(vec2_t){0.0f,0.0f},  (vec3_t){fluctuate,fluctuate,fluctuate},1.0f};
		triangle[triangle_c++] = (triangle_t){triang[table[i][2]],(vec2_t){0.0f,0.01f}, (vec3_t){fluctuate,fluctuate,fluctuate},1.0f};
		triangle[triangle_c++] = (triangle_t){triang[table[i][3]],(vec2_t){0.01f,0.01f},(vec3_t){fluctuate,fluctuate,fluctuate},1.0f};
	}
	if(render_mode != RENDER_MODE_WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glBufferData(GL_ARRAY_BUFFER,triangle_c * sizeof(triangle_t),triangle,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES,0,triangle_c);
	if(render_mode != RENDER_MODE_WIREFRAME)
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
}

void castVisibilityRays(){
	traverse_init_t init = initTraverse(camera.pos);
	static uint32_t counter;
	float luminance_accumulate = 0.0f;
	for(int x = counter / 16;x < window_size.x;x += 16){
		for(int y = counter % 16;y < window_size.y;y += 16){
			vec3_t ray_angle = getRayAngleCamera(x,y);
			node_hit_t hit = treeRay(ray3Create(init.pos,ray_angle),init.node,camera.pos);
			if(!hit.node)
				continue;
			vec3_t luminance = sideGetLuminance(camera.pos,ray_angle,hit,0);
			luminance_accumulate += tMaxf(luminance.r,tMaxf(luminance.g,luminance.b));
			node_t* node = dynamicArrayGet(node_root,hit.node);
			uint32_t side = hit.side * 2 + (ray_angle.a[hit.side] < 0.0f);
			if(node->type == BLOCK_PARENT || node->type == BLOCK_AIR)
				continue;
			material_t material = material_array[node->type];
			if(material.flags & MAT_LUMINANT)
				continue;
			block_t* block = dynamicArrayGet(block_array,node->index);
			if(node->type == BLOCK_SPHERE)
				side = 0;
			if(!block->luminance[side])
				lightNewStackPut(hit.node,block,side);
		}
	}
	luminance_accumulate /= (window_size.x / 16) * (window_size.y / 16);
	camera.exposure = camera.exposure * 0.99f + (2.0f - tMinf(luminance_accumulate,1.9f)) * 0.01f;
	camera.exposure = camera.exposure * 0.99f + 0.01f;
	
	counter += TRND1 * 500.0f;
	counter %= 16 * 16;
}

#define TREEITERATION_SQUARE_SIZE 16

void collectTreeIteration(){
	traverse_init_t init = initTraverse(camera.pos);
	for(int x = 0;x < window_size.x;x += TREEITERATION_SQUARE_SIZE){
		for(int y = 0;y < window_size.y;y += TREEITERATION_SQUARE_SIZE){
			uint32_t c = traverseTreeItt(ray3CreateI(init.pos,getRayAngleCamera(x,y)),init.node);
			guiRectangle((vec2_t){y * (2.0f / window_size.y) - 1.0f,x * (2.0f / window_size.x) - 1.0f},(vec2_t){(2.0f / window_size.y) * TREEITERATION_SQUARE_SIZE,(2.0f / window_size.x) * TREEITERATION_SQUARE_SIZE},(vec3_t){c * 0.02f,0.0f,0.0f});
		}
	}
}

void gatherMaskTriangles(){
	triangle_count = 0;
	for(int x = 0;x < RD_MASK_X;x++){
		for(int y = 0;y < RD_MASK_Y;y++){
			vec3_t color = occlusion_mask[x * RD_MASK_Y + y] ? (vec3_t){1.0f,1.0f,1.0f} : (vec3_t){0.0f,0.0f,0.0f}; 
			guiRectangle((vec2_t){y * (2.0f / window_size.y) * RD_MASK_SIZE - 1.0f,x * (2.0f / window_size.x) * RD_MASK_SIZE - 1.0f},(vec2_t){(2.0f / window_size.y) * RD_MASK_SIZE,(2.0f / window_size.x) * RD_MASK_SIZE},color);
		}
	}
}

void drawHardware(){
	glBufferData(GL_ARRAY_BUFFER,triangle_count * sizeof(triangle_t),triangles,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES,0,triangle_count);
	/*
	for(int i = 0;i < RD_MASK_X;i++){
		for(int j = 0;j < RD_MASK_Y;j++){
			if(!mask[i][j])
				continue;
			guiRectangle((vec2_t){(float)j / RD_MASK_Y * 2.0f - 1.0f,(float)i / RD_MASK_X * 2.0f - 1.0f},(vec2_t){2.0f / (window_size.y / RD_MASK_SIZE),2.0f / (window_size.x / RD_MASK_SIZE)},(vec3_t){1.0f,0.0f,0.0f});
		}
	}
	glBufferData(GL_ARRAY_BUFFER,triangle_count * sizeof(triangle_t),triangles,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES,0,triangle_count);
	*/
}

#define BLOCK_SELECT_LIGHT_QUALITY 16

vec3_t getBlockSelectLight(vec2_t direction){
	vec3_t normal = getLookAngle(direction);
	vec3_t s_color = VEC3_ZERO;
	float weight_total = 0.0f;
	direction.x -= M_PI * 0.5f - M_PI * 0.5f / BLOCK_SELECT_LIGHT_QUALITY;
	direction.y -= M_PI * 0.5f - M_PI * 0.5f / BLOCK_SELECT_LIGHT_QUALITY;
	traverse_init_t init = initTraverse(camera.pos);
	for(int i = 0;i < BLOCK_SELECT_LIGHT_QUALITY;i++){
		for(int j = 0;j < BLOCK_SELECT_LIGHT_QUALITY;j++){
			vec3_t angle = getLookAngle(direction);
			float weight = tAbsf(vec3dotR(angle,normal));
			vec3_t luminance = rayGetLuminanceDir(init.pos,direction,init.node,camera.pos);
			vec3mul(&luminance,weight);
			vec3addvec3(&s_color,luminance);
			weight_total += weight;
			direction.y += M_PI / BLOCK_SELECT_LIGHT_QUALITY;
		}
		direction.y -= M_PI;
		direction.x += M_PI / BLOCK_SELECT_LIGHT_QUALITY;
	}
	vec3div(&s_color,weight_total);
	return s_color;
}

typedef struct{
	vec2_t direction;
	vec3_t position;
}holdable_t;

holdable_t holdable;

void drawBlockSelect(vec3_t block_pos,vec3_t luminance[3],float block_size,uint32_t material_id){
	if(material_id == BLOCK_RAIL){
		return;
	}
	material_t material = material_array[material_id];
	vec2_t x_direction = vec2addvec2R((vec2_t){holdable.direction.x + 0.1f,0.1f},(vec2_t){M_PI * 0.5f,0.0f});
	vec2_t y_direction = vec2addvec2R((vec2_t){holdable.direction.x + 0.1f,holdable.direction.y + 0.1f},(vec2_t){0.0f,M_PI * 0.5f});
	vec2_t z_direction = vec2addvec2R((vec2_t){holdable.direction.x + 0.1f,holdable.direction.y + 0.1f},(vec2_t){0.0f,0.0f});
	vec3_t y_angle = getLookAngle(x_direction);
	vec3_t x_angle = getLookAngle(y_direction);
	vec3_t z_angle = getLookAngle(z_direction);
	
	vec3_t pos = holdable.position;
	vec3addvec3(&pos,vec3mulR(x_angle,-2.5f + block_pos.x));
	vec3addvec3(&pos,vec3mulR(y_angle, 1.5f + block_pos.y));
	vec3addvec3(&pos,vec3mulR(z_angle, 2.5f + block_pos.z));
	vec3_t verticle_screen[8];
	for(int i = 0;i < 8;i++){
		vec3_t x = i & 1 ? vec3mulR(x_angle,block_size) : VEC3_ZERO;
		vec3_t y = i & 2 ? vec3mulR(y_angle,block_size) : VEC3_ZERO;
		vec3_t z = i & 4 ? vec3mulR(z_angle,block_size) : VEC3_ZERO;
		vec3_t verticle_pos = pos;
		vec3addvec3(&verticle_pos,x);
		vec3addvec3(&verticle_pos,y);
		vec3addvec3(&verticle_pos,z);
		vec3_t screen = pointToScreenRenderer(verticle_pos);
		verticle_screen[i].y = screen.x;
		verticle_screen[i].x = screen.y;
		verticle_screen[i].z = screen.z;
	}

	vec2_t texture_coord[4];
	texture_coord[0] = material.texture_pos;
	texture_coord[1] = (vec2_t){material.texture_pos.x,material.texture_pos.y + material.texture_size.y};
	texture_coord[2] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y};
	texture_coord[3] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y + material.texture_size.y};

	triangles[triangle_count + 0 ] = (triangle_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 1 ] = (triangle_t){verticle_screen[1],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 2 ] = (triangle_t){verticle_screen[3],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 3 ] = (triangle_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 4 ] = (triangle_t){verticle_screen[2],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 5 ] = (triangle_t){verticle_screen[3],texture_coord[3],1.0f,1.0f,1.0f,1.0f};

	triangles[triangle_count + 6 ] = (triangle_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 7 ] = (triangle_t){verticle_screen[1],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 8 ] = (triangle_t){verticle_screen[5],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 9 ] = (triangle_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 10] = (triangle_t){verticle_screen[4],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 11] = (triangle_t){verticle_screen[5],texture_coord[3],1.0f,1.0f,1.0f,1.0f};

	triangles[triangle_count + 12] = (triangle_t){verticle_screen[1],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 13] = (triangle_t){verticle_screen[3],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 14] = (triangle_t){verticle_screen[7],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 15] = (triangle_t){verticle_screen[1],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 16] = (triangle_t){verticle_screen[5],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 17] = (triangle_t){verticle_screen[7],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	
	for(int i = 0;i < 6;i++){
		triangles[triangle_count + i + 0 ].lighting = vec3mulvec3R(material.luminance,luminance[0]);
		triangles[triangle_count + i + 6 ].lighting = vec3mulvec3R(material.luminance,luminance[1]);
		triangles[triangle_count + i + 12].lighting = vec3mulvec3R(material.luminance,luminance[2]);
	}
	triangle_count += 18;
}

void genHoldStructure(hold_structure_t* hold,vec3_t luminance[3],vec3_t block_pos,float block_size){
	if(hold->block_id == BLOCK_PARENT){
		block_size *= 0.5f;
		int order[] = {4,5,6,0,7,1,2,3};
		for(int i = 0;i < 8;i++){
			int index = order[i];
			float dx = index & 4 ? block_size : 0.0f;
			float dy = index & 2 ? block_size : 0.0f;
			float dz = index & 1 ? block_size : 0.0f;
			vec3_t block_pos_child = vec3addvec3R(block_pos,(vec3_t){dx,dy,dz});
			genHoldStructure(hold->child_s[index],luminance,block_pos_child,block_size);
		}
		return;
	}
	if(hold->block_id == BLOCK_AIR)
		return;
	drawBlockSelect(block_pos,luminance,block_size,hold->block_id);
}

void genBlockSelect(){
	holdable.direction = vec2addvec2R(vec2mulR(holdable.direction,0.5f),vec2mulR(camera.dir,0.5f));
	holdable.position = vec3addvec3R(vec3mulR(holdable.position,0.5f),vec3mulR(camera.pos,0.5f));
	vec3_t luminance[3];
	material_t material = material_array[block_type];
	if(material.flags & MAT_LUMINANT){
		luminance[0] = (vec3_t){material.luminance.r,material.luminance.g,material.luminance.b};
		luminance[1] = (vec3_t){material.luminance.r,material.luminance.g,material.luminance.b};
		luminance[2] = (vec3_t){material.luminance.r,material.luminance.g,material.luminance.b};
	}
	else{
		luminance[0] = getBlockSelectLight((vec2_t){holdable.direction.x + M_PI,holdable.direction.y});
		luminance[1] = getBlockSelectLight((vec2_t){holdable.direction.x - M_PI * 0.5f,holdable.direction.y});
		luminance[2] = getBlockSelectLight((vec2_t){holdable.direction.x,holdable.direction.y + M_PI * 0.5f});
	}
	switch(player_gamemode){
	case GAMEMODE_CREATIVE:
		if(model_mode)
			genHoldStructure(hold_structure,luminance,(vec3_t){0.0f,0.0f,0.0f},1.0f);
		else
			drawBlockSelect(VEC3_ZERO,luminance,1.0f,block_type);
		break;
	case GAMEMODE_SURVIVAL:
		if(inventory_slot[inventory_select].type != ITEM_VOID){
			drawBlockSelect(VEC3_ZERO,luminance,1.0f,inventory_slot[inventory_select].type);
			break;
		}
		vec3_t block_pos[4] = {
			{0.75f,0.0f,0.0f},
			{0.75f,0.0f,0.25f},
			{0.75f,0.0f,0.5f},
			{0.75f,0.0f,0.75f}
		};
		if(GetKeyState(VK_RBUTTON) & 0x80){
			vec3addvec3(&block_pos[0],(vec3_t){0.25f,-1.5f,0.0f});
			vec3addvec3(&block_pos[1],(vec3_t){0.25f,-1.5f,0.0f});
			vec3addvec3(&block_pos[2],(vec3_t){0.25f,-1.5f,0.0f});
			vec3addvec3(&block_pos[3],(vec3_t){0.25f + (TRND1 - 0.5f) * 0.1f,-1.5f + (TRND1 - 0.5f) * 0.1f,0.0f});
		}
		if(player.attack_animation.state != ANIMATION_INACTIVE){
			vec3_t offset = getAnimationOffset(&player.attack_animation);
			vec3addvec3(&block_pos[0],offset);
			vec3addvec3(&block_pos[1],offset);
			vec3addvec3(&block_pos[2],offset);
			vec3addvec3(&block_pos[3],offset);
		}
		drawBlockSelect(block_pos[0],luminance,0.25f,BLOCK_SKIN);
		drawBlockSelect(block_pos[1],luminance,0.25f,BLOCK_SKIN);
		drawBlockSelect(block_pos[2],luminance,0.25f,BLOCK_SKIN);
		drawBlockSelect(block_pos[3],luminance,0.25f,BLOCK_SKIN);
		break;
	}
}

void mouseDown(){
	if(player_gamemode != GAMEMODE_SURVIVAL)
		return;
	vec3_t dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRay(ray3Create(init.pos,dir),init.node,camera.pos);
	if(!result.node)
		return;
	node_t* node = dynamicArrayGet(node_root,result.node);
	material_t material = material_array[node->type];
	float distance = rayNodeGetDistance(camera.pos,node->pos,node->depth,dir,result.side);
	if(distance > PLAYER_REACH){
		block_break_progress = 0.0f;
		return;
	}
	vec3_t pos = vec3addvec3R(camera.pos,vec3mulR(dir,distance + 0.001f));
	ivec3 grid_pos = getGridPosFromPos(pos,edit_depth);
	if(block_break_pos.x != grid_pos.x || block_break_pos.y != grid_pos.y || block_break_pos.z != grid_pos.z){
		block_break_progress = 0.0f;
		block_break_pos = grid_pos;
		return;
	}
	int depth = tMax(node->depth,edit_depth);
	block_break_progress += 0.01f / material.hardness * (1 << (depth - 5) * 3) * 0.001f;
	if(block_break_progress < 1.0f){
		guiProgressBar((vec2_t){-0.1f,-0.2f},(vec2_t){0.2f,0.05f},block_break_progress);
		return;
	}
	vec3_t spawn_pos = vec3addvec3R(camera.pos,vec3mulR(dir,distance));
	block_break_progress = 0.0f;
	inventoryAdd((item_t){.ammount = 1 << (DEPTH_MAX - depth) * 3,.type = node->type});
	rayRemoveVoxel(camera.pos,dir,tMax(node->depth,edit_depth));
	for(int i = 0;i < 16;i++){
		entity_t* entity = getNewEntity();
		entity->type = ENTITY_PARTICLE;
		float block_size = getBlockSize(depth);
		vec3_t block_pos = {block_break_pos.x,block_break_pos.y,block_break_pos.z};
		vec3mul(&block_pos,block_size);
		entity->pos = vec3addvec3R(block_pos,(vec3_t){TRND1 * block_size,TRND1 * block_size,TRND1 * block_size});
		entity->size = 0.1f;
		entity->texture_pos = material.texture_pos;
		entity->texture_size = material.texture_size;
		entity->color = (vec3_t){1.0f,1.0f,1.0f};
		entity->flags = ENTITY_FLAG_GRAVITY | ENTITY_FLAG_FUSE;
		entity->fuse = TRND1 * 200.0f;
		entity->dir = (vec3_t){(TRND1 - 0.5f) * 0.03f,(TRND1 - 0.5f) * 0.03f,(TRND1 - 0.5f) * 0.03f};
	}
}

#define BLOCKTICK_RADIUS 120.0f

uint32_t block_tick_filter;

void blockTick(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	if(node->type == BLOCK_PARENT){
		float distance2 = sdCube(camera.pos,block_pos,block_size);
		if(distance2 > BLOCKTICK_RADIUS)
			return;
		for(int i = 0;i < 8;i++)
			blockTick(node->child_s[i]);
		return;
	}
	if((node_ptr & 0x3f) != block_tick_filter)
		return;
	if(node->type == BLOCK_AIR){
		for(int i = 0;i < 6;i++)
			gasSpread(node->pos.x,node->pos.y,node->pos.z,node->depth,i,border_block_table[i]);
		return;
	}
	material_t material = material_array[node->type];
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(material.flags & MAT_LIQUID){
		liquidUnder(node->pos.x,node->pos.y,node->pos.z,node->depth);
		if(block->ammount > 0.001f){
			liquidSide(node_ptr,0,border_block_table[0]);
			liquidSide(node_ptr,1,border_block_table[1]);
			liquidSide(node_ptr,2,border_block_table[2]);
			liquidSide(node_ptr,3,border_block_table[3]);
		}
	}
	if(material.flags & MAT_POWDER){
		if(powderUnder(node_ptr))
			return;
		if(TRND1 > 0.05f)
			return;
		for(int i = 0;i < 4;i++)
			if(powderSide(node_ptr,i))
				return;
	}
}

int main_thread_status;

void drawCraftMenu(){
	drawString((vec2_t){-0.247f,0.6f},0.028f,"ammount");
	drawNumber((vec2_t){-0.14f,0.51f},RD_SQUARE(0.032f),craft_ammount);

	guiRectangle((vec2_t){-0.45f * RD_RATIO,0.45f},(vec2_t){0.9f * RD_RATIO,0.02f},(vec3_t){0.2f,0.2f,0.2f});
	guiFrame(RD_SQUARE(-0.47f),(vec2_t){0.92f * RD_RATIO,1.12f},(vec3_t){0.2f,0.2f,0.2f},0.02f);
	guiFrame(RD_SQUARE(-0.55f),(vec2_t){1.0f * RD_RATIO,1.2f},(vec3_t){0.6f,0.5f,0.1f},0.1f);
	guiRectangle(RD_SQUARE(-0.45f),(vec2_t){0.9f * RD_RATIO,1.2f},(vec3_t){0.4f,0.4f,0.4f});
}

void menu(){
	switch(in_menu){
	case MENU_CRAFT:
		drawGUI(gui_array[GUI_CRAFT_MENU]);
		drawCraftMenu();
		break;
	}
	if(block_menu_block){
		node_t* node = dynamicArrayGet(node_root,block_menu_block);
		drawGUI(gui_array[material_array[node->type].menu_index]);
		switch(node->type){
		case BLOCK_GENERATOR:
			guiRectangle((vec2_t){-0.45f * RD_RATIO,0.45f},(vec2_t){0.9f * RD_RATIO,0.02f},(vec3_t){0.2f,0.2f,0.2f});
			guiFrame(RD_SQUARE(-0.47f),(vec2_t){0.92f * RD_RATIO,1.12f},(vec3_t){0.2f,0.2f,0.2f},0.02f);
			guiFrame(RD_SQUARE(-0.55f),(vec2_t){1.0f * RD_RATIO,1.2f},(vec3_t){0.6f,0.5f,0.1f},0.1f);
			guiRectangle(RD_SQUARE(-0.45f),(vec2_t){0.9f * RD_RATIO,1.2f},(vec3_t){0.4f,0.4f,0.4f});
			break;
		case BLOCK_FURNACE:
		case BLOCK_BASIC:
		case BLOCK_ELEKTRIC_1:
			guiFrame(RD_SQUARE(-0.47f),RD_SQUARE(0.92f),(vec3_t){0.2f,0.2f,0.2f},0.02f);
			guiFrame(RD_SQUARE(-0.55f),RD_SQUARE(1.0f),(vec3_t){0.6f,0.5f,0.1f},0.1f);
			guiRectangle((vec2_t){-0.45f * RD_RATIO,0.25f},(vec2_t){0.9f * RD_RATIO,0.02f},(vec3_t){0.2f,0.2f,0.2f});
			guiRectangle(RD_SQUARE(-0.45f),RD_SQUARE(0.9f),(vec3_t){0.4f,0.4f,0.4f});
			break;
		}
	}
}

void blockCraftRecipy(int num_tick){
	node_t* node = dynamicArrayGet(node_root,block_menu_block);
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(block->recipy_select == 0xff)
		return;
	craftrecipy_t recipy = craft_recipy[block->recipy_select];
	if(!block->progress){
		for(int i = 0;i < 2;i++){
			for(int j = 0;j < 2;j++){
				if(recipy.cost[j].type == ITEM_VOID)
					continue;
				if(block->inventory[i].type == recipy.cost[j].type)
					itemChange(&recipy.cost[j],-block->inventory[i].ammount);
			}
		}
		if(!recipy.cost[0].ammount && !recipy.cost[1].ammount){
			if(block->inventory[2].type != recipy.result && block->inventory[2].type != ITEM_VOID)
				return;
			recipy = craft_recipy[block->recipy_select];
			for(int i = 0;i < 2;i++){
				for(int j = 0;j < 2;j++){
					if(recipy.cost[j].type == ITEM_VOID)
						continue;
					if(block->inventory[i].type == recipy.cost[j].type)
						itemChange(&block->inventory[i],-recipy.cost[j].ammount);
				}
			}
			block->progress += 0.01f * num_tick;
		}
		return;
	}
	block->progress += 0.01f * num_tick;
	if(block->progress > 1.0f){
		if(block->inventory[2].type != recipy.result && block->inventory[2].type != ITEM_VOID){
			block->progress = 1.0f;
			return;
		}
		block->progress = 0.0f;
		block->inventory[2].type = recipy.result;
		block->inventory[2].ammount += 1;
	}
}

void draw(){
	global_tick = GetTickCount64();
	uint64_t time = 0;
	HANDLE lighting_thread = CreateThread(0,0,lighting,0,0,0);
	initGL(context);
	for(;;){
		camera.tri = (vec4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
		is_rendering = true;
		if(player_gamemode == GAMEMODE_SURVIVAL){
			if(item_hold_ptr)
				guiInventoryContent(&item_hold,vec2subvec2R(getCursorGUI(),RD_SQUARE(0.05f)),RD_SQUARE(0.1f));
			for(int i = 0;i < 8;i++){
				vec3_t color;
				if(inventory_select == i)
					color = (vec3_t){1.0f,0.0f,0.0f};
				else if(i == 0)
					color = (vec3_t){1.0f,1.0f,0.0f};
				else
					color = (vec3_t){1.0f,1.0f,1.0f};
				guiInventory(&inventory_slot[i],(vec2_t){-0.363f + i * 0.17f * RD_RATIO,-0.925f},color);
			}
		}
		if(in_console){
			drawString((vec2_t){-0.8f,0.8f},0.03f,"console");
			drawString((vec2_t){-0.8f,0.7f},0.02f,console_buffer);
			guiRectangle((vec2_t){-0.9f,0.1f},RD_SQUARE(0.8f),(vec3_t){0.1f,0.2f,0.25f});
		}
		if(!block_menu_block && !in_menu){
			guiRectangle((vec2_t){-0.0025f * RD_RATIO,-0.02f},(vec2_t){0.005f * RD_RATIO,0.04f},(vec3_t){0.0f,1.0f,0.0f});
			guiRectangle((vec2_t){-0.02f * RD_RATIO,-0.0025f},(vec2_t){0.04f * RD_RATIO,0.005f},(vec3_t){0.0f,1.0f,0.0f});
			guiRectangle((vec2_t){-0.005f * RD_RATIO,-0.025f},(vec2_t){0.01f * RD_RATIO,0.05f},(vec3_t){0.0f,0.0f,0.0f});
			guiRectangle((vec2_t){-0.025f * RD_RATIO,-0.005f},(vec2_t){0.05f * RD_RATIO,0.01f},(vec3_t){0.0f,0.0f,0.0f});
		}
		else{
			vec2_t offset = getCursorGUI();
			guiRectangle(vec2addvec2R(offset,(vec2_t){-0.0025f * RD_RATIO,-0.02f}),(vec2_t){0.005f * RD_RATIO,0.04f},(vec3_t){0.0f,0.0f,0.0f});
			guiRectangle(vec2addvec2R(offset,(vec2_t){-0.02f * RD_RATIO,-0.0025f}),(vec2_t){0.04f * RD_RATIO,0.005f},(vec3_t){0.0f,0.0f,0.0f});
			menu();
		}
		if(!in_menu && !block_menu_block)
			generateBlockOutline();
		if(render_mode_change != render_mode){
			render_mode = render_mode_change;
			glPolygonMode(GL_FRONT_AND_BACK,render_mode == RENDER_MODE_WIREFRAME ? GL_LINE : GL_FILL);
		}
		float f = tMin(__rdtsc() - time >> 18,window_size.x - 1);
		f /= WND_RESOLUTION_X / 2.0f;
		time = __rdtsc();
		guiRectangle((vec2_t){-0.9f,-1.0f},(vec2_t){0.01f,f},(vec3_t){0.0f,1.0f,0.0f});
		main_thread_status = 0;
		genBlockSelect();
		castVisibilityRays();
		switch(render_mode){
		default:
			setViewPlanes();
			sceneGatherTriangles(0);
			break;
		case RENDER_MODE_TREE_ITERATION:
			collectTreeIteration();
			break;
		case RENDER_MODE_MASK:
			setViewPlanes();
			sceneGatherTriangles(0);
			gatherMaskTriangles();
			break;
		}
		main_thread_status = 1;
		drawHardware();
		
		while(main_thread_status == 1)
			Sleep(1);
		memset(occlusion_mask,0,RD_MASK_X * RD_MASK_Y);
		triangle_count = 0;
		SwapBuffers(context);
		glClearColor(LUMINANCE_SKY.r,LUMINANCE_SKY.g,LUMINANCE_SKY.b,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		uint64_t tick_new = GetTickCount64();
		if(tick_new >> 12 != global_tick >> 12){
			if(inventory_slot[INVENTORY_PASSIVE].type == BLOCK_TORCH)
				itemChange(&inventory_slot[INVENTORY_PASSIVE],-1);
		}
		int difference = tick_new - global_tick;
		int num_tick = (tick_new >> 2) - (global_tick >> 2);
		for(int i = 0;i < num_tick;i++){
			for(int j = 0;j < furnace_array.size;j++){
				uint32_t* index = dynamicArrayGet(furnace_array,j);
				if(!*index)
					continue;
				node_t* node = dynamicArrayGet(node_root,*index);
				block_t* block = dynamicArrayGet(block_array,node->index);
				if(block->progress && block->inventory[1].type == BLOCK_IRON_ORE || block->inventory[1].type == BLOCK_COPPER_ORE){
					block->progress += 0.0003f;
				}
			}
		}
		for(int i = 0;i < (tick_new >> 4) - (global_tick >> 4);i++)
			blockTick(0);
		entityTick(num_tick);
		if(block_menu_block)
			blockCraftRecipy(num_tick);
		if(player.attack_animation.state)
			progressAnimation(&player.attack_animation,num_tick);
		for(int i = 0;i < num_tick;i++)
			powerGridTick();
		if(!in_console)
			physics(num_tick);
		for(int i = 0;i < ENTITY_AMMOUNT;i++){
			if(!entity_array[i].alive)
				continue;
		}
		if(r_click){
			r_click = false;
			playerRemoveBlock();
		}
		if(l_click){
			l_click = false;
			clickLeft();
		}
		if(GetKeyState(VK_RBUTTON) & 0x80)
			mouseDown();
		else
			block_break_progress = 0.0f;
		if(player_gamemode == GAMEMODE_SURVIVAL)
			trySpawnEnemy();
		if(execute_command){
			executeCommand();
			execute_command = false;
		}
		global_tick = tick_new;
	}	
}

float rayBoxIntersection(vec3_t ro,vec3_t rd,vec3_t boxSize) {
    vec3_t m = vec3divFR(rd,1.0f); // can precompute if traversing a set of aligned boxes
    vec3_t n = vec3mulvec3R(m,ro);   // can precompute if traversing a set of aligned boxes
    vec3_t k = vec3mulvec3R(vec3absR(m),boxSize);
    vec3_t t1 = vec3subvec3R(vec3negR(n),k);
    vec3_t t2 = vec3addvec3R(vec3negR(n),k);
    float tN = tMaxf(tMaxf(t1.x,t1.y),t1.z);
    float tF = tMinf(tMinf(t2.x,t2.y),t2.z);
    if(tN > tF || tF < 0.0f) 
		return -1.0f; // no intersection
    return tN;
}

typedef struct{
	vec3_t size;
	vec3_t offset;
	vec3_t color[6];
}box_t;

box_t box[] = {
	{
		.offset = {1.0f,0.0f,0.0f},
		.size = {0.1f,1.0f,0.7f},
		.color = {
			{1.0f,1.0f,1.0f},
			{0.5f,0.5f,0.5f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
		}
	},
	{
		.offset = {-1.0f,0.0f,0.0f},
		.size = {0.1f,1.0f,0.7f},
		.color = {
			{0.5f,0.5f,0.5f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
		}
	},
	
	{
		.offset = {0.0f,1.0f,0.0f},
		.size = {1.0f,0.1f,0.7f},
		.color = {
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{0.5f,0.5f,0.5f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
		}
	},
	{
		.offset = {0.0f,-1.0f,0.0f},
		.size = {1.0f,0.1f,0.7f},
		.color = {
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{0.5f,0.5f,0.5f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
			{1.0f,1.0f,1.0f},
		}
	},
};

void initSprite(){
	vec3_t ray_pos = {2.5f,0.0f,0.0f};
	vec2_t ray_dir = {1.0f,0.0f};
	for(int r = 0;r < 16;r++){
		for(int x = 0;x < 32;x++){
			for(int y = 0;y < 32;y++){
				float min_distance = 999999.0f;
				int min_object = -1;
				vec3_t ray_angle = getRayAngle(ray_dir,(vec2_t){(1.0f / 16.0f) * y - 1.0f,(1.0f / 16.0f) * x - 1.0f});
				//printf("%f\n",ray_angle.z);
				for(int i = 0;i < sizeof(box) / sizeof(box_t);i++){
					float distance = rayBoxIntersection(vec3subvec3R(box[i].offset,ray_pos),ray_angle,box[i].size);
					if(distance == -1.0f || distance > min_distance)
						continue;
					min_distance = distance;
					min_object = i;
				}
				int location = (x + TEXTURE_ATLAS_SIZE - 128 - 32 - r * 32) * TEXTURE_ATLAS_SIZE + y;
				if(min_object == -1){
					texture_atlas[location].a = 0xff;
					continue;
				}
				vec3_t pos = vec3subvec3R(ray_pos,vec3mulR(ray_angle,min_distance));
				int index = -1;
				if(tAbsf(pos.x - box[min_object].size.x - box[min_object].offset.x) < 0.001f)
					index = 0;
				if(tAbsf(pos.x + box[min_object].size.x - box[min_object].offset.x) < 0.001f)
					index = 1;
				if(tAbsf(pos.y - box[min_object].size.y - box[min_object].offset.y) < 0.001f)
					index = 2;
				if(tAbsf(pos.y + box[min_object].size.y - box[min_object].offset.y) < 0.001f)
					index = 3;
				if(tAbsf(pos.z - box[min_object].size.z - box[min_object].offset.z) < 0.001f)
					index = 4;
				if(tAbsf(pos.z + box[min_object].size.z - box[min_object].offset.z) < 0.001f)
					index = 5;
				if(index == -1)
					continue;
				texture_atlas[location].r = box[min_object].color[index].r * 255.0f;
				texture_atlas[location].g = box[min_object].color[index].g * 255.0f;
				texture_atlas[location].b = box[min_object].color[index].b * 255.0f;
			}
		}
		vec2rot(&ray_pos,M_PI * 0.125f);
		ray_dir.x += M_PI * 0.125f;
	}
}

void main(){ 
	initGUI();
	initAnimation();
	window_size.x = GetSystemMetrics(SM_CYSCREEN);
	window_size.y = GetSystemMetrics(SM_CXSCREEN);
	occlusion_mask = tMallocZero(RD_MASK_X * RD_MASK_Y);
	occlusion_scanline = tMallocZero(RD_MASK_X * sizeof(ivec2));
 	//generate sphere mesh from tetrahedron
	int indic_c = 4;
	int vertex_c = 4;
	for(int j = 0;j < 3;j++){
		int c = indic_c;
		for(int i = 0;i < c;i++){
			vec3_t point[3] = {
				sphere_vertex[sphere_indices[i][0]],
				sphere_vertex[sphere_indices[i][1]],
				sphere_vertex[sphere_indices[i][2]],
			};

			sphere_vertex[vertex_c + 0] = vec3normalizeR(vec3avgvec3R(point[0],point[1]));
			sphere_vertex[vertex_c + 1] = vec3normalizeR(vec3avgvec3R(point[0],point[2]));
			sphere_vertex[vertex_c + 2] = vec3normalizeR(vec3avgvec3R(point[1],point[2]));

			sphere_indices[indic_c  ][0] = vertex_c + 0;
			sphere_indices[indic_c  ][1] = vertex_c + 1;
			sphere_indices[indic_c++][2] = sphere_indices[i][0];

			sphere_indices[indic_c  ][0] = vertex_c + 0;
			sphere_indices[indic_c  ][1] = vertex_c + 2;
			sphere_indices[indic_c++][2] = sphere_indices[i][1];

			sphere_indices[indic_c  ][0] = vertex_c + 1;
			sphere_indices[indic_c  ][1] = vertex_c + 2;
			sphere_indices[indic_c++][2] = sphere_indices[i][2];

			sphere_indices[i][0] = vertex_c + 0;
			sphere_indices[i][1] = vertex_c + 1;
			sphere_indices[i][2] = vertex_c + 2;

			vertex_c += 3;
		}
	}

#pragma comment(lib,"Winmm.lib")
	timeBeginPeriod(1);
	node_t genesis;
	genesis.depth = 0;
	genesis.pos = (ivec3){0,0,0};
	for(int i = 0;i < 8;i++)
		genesis.child_s[i] = 0;
	genesis.parent = 0;
	genesis.type = BLOCK_AIR;
	dynamicArrayAdd(&node_root,&genesis);
	node_t* node = node_root.data;
	triangles = tMalloc(sizeof(triangle_t) * 1000000);
	texture_atlas = tMalloc(sizeof(pixel_t) * TEXTURE_ATLAS_SIZE * TEXTURE_ATLAS_SIZE);

	bmi.bmiHeader.biWidth  = window_size.y;
	bmi.bmiHeader.biHeight = window_size.x;

	for(int x = 0;x < TEXTURE_ATLAS_SIZE * TEXTURE_ATLAS_SIZE;x++)
		texture_atlas[x] = (pixel_t){255,255,255,0};

	texture_t font = loadBMP("img/vram.bmp");

	for(int x = 0;x <  TEXTURE_ATLAS_SIZE;x++){
		for(int y = 0;y < TEXTURE_ATLAS_SIZE;y++){
			pixel_t color = font.data[(TEXTURE_ATLAS_SIZE - x - 1) * TEXTURE_ATLAS_SIZE + y];
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y] = (pixel_t){color.b,color.g,color.r,0};
			if(!color.b && !color.r && !color.g){
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].a = 0xff;
			}
		}
	}
	//stone
	for(int x = 2048 - 128;x < 2048;x++){
		for(int y = 0;y < 128;y++){
			int table[] = {1,2,3,5,8,13,21,34};
			for(int i = 0;i < 3;i++){
				int value = tNoise((x / table[i]) * TEXTURE_ATLAS_SIZE + y / table[i]) % 0x2f;
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].r += value + 0x20;
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].g += value + 0x20;
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].b += value + 0x20;
			}
		}
	}
	//copper
	for(int x = 2048 - 128;x < 2048;x++){
		for(int y = 128;y < 256;y++){
			int variation = tNoise((x + y) % 128) % 0x2f;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].r = 0xa0 + variation;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].g = 0x40 + variation;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].b = 0x20 + variation;
		}
	}
	//iron
	for(int x = 2048 - 128;x < 2048;x++){
		for(int y = 256 + 128;y < 512;y++){
			int variation = tNoise((x + y) % 128) % 0x2f;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].r = 0x40 + variation;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].g = 0x60 + variation;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].b = 0x70 + variation;
		}
	}
	//coal
	for(int x = 2048 - 128;x < 2048;x++){
		for(int y = 256;y < 256 + 128;y++){
			int table[] = {1,2,3,5,8,13,21,34};
			for(int i = 0;i < 8;i++){
				int value = tNoise((x / table[i]) * TEXTURE_ATLAS_SIZE + y / table[i]) % 0x1c;
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].r += value;
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].g += value;
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].b += value;
			}
		}
	}

	initSprite();

	//createFlatWorld();
	createSurvivalWorld();


	vec3_t spawnPos = (vec3_t){camera.pos.x,camera.pos.y,MAP_SIZE};
	float distance = rayGetDistance(spawnPos,(vec3_t){0.01f,0.01f,-1.0f}) - 5.0f;
	spawnPos.z -= distance;
	camera.pos = spawnPos;
	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE | WS_POPUP,0,0,window_size.y,window_size.x,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(false);

	initSound(window);
	CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw,0,0,0);

	MSG msg;
	while(GetMessageA(&msg,window,0,0)){
		TranslateMessage(&msg);
 		DispatchMessageA(&msg); 
	}
}