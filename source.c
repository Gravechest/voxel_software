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
#include "dynamic_array.h"
#include "gui.h"

#include <gl/GL.h>

#pragma comment(lib,"opengl32")

bool lighting_thread_done = true;

void* physics_thread; 
void* thread_render;  
void* entity_thread;

vec3_t music_box_pos;

bool context_is_gl;

int proc(HWND hwnd,uint32_t msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = (WNDPROC)proc,.lpszClassName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,32,BI_RGB};
void* window;
void* context;
vram_t vram;

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

unsigned int global_tick;

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

inventoryslot_t inventory_slot[] = {
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1}
};

material_t material_array[] = {
	[BLOCK_SPHERE] = {
		.luminance = {0.1f,0.5f,0.5f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_GENERATOR] = {
		.luminance = {0.1f,0.5f,0.5f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_COAL] = {
		.luminance = {0.1f,0.1f,0.1f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	[BLOCK_EMIT_RED] = {
		.luminance = {0.9f,0.3f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LUMINANT
	},
	[BLOCK_EMIT_GREEN] = {
		.luminance = {0.3f,0.9f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LUMINANT
	},
	[BLOCK_EMIT_BLUE] = {
		.luminance = {0.3f,0.3f,0.9f},
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
		.flags = MAT_POWER
	},
	[BLOCK_SWITCH] = {
		.luminance = {0.9f,0.3f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.light_emit = 5.0f,
		.flags = MAT_POWER
	},
	[BLOCK_STONE] = {
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = {0.0f,1.0f / 16.0f * 15.0f},
		.texture_size = {1.0f / 16.0f,1.0f / 16.0f},
		.hardness = 1.0f,
	},
	[BLOCK_SKIN] = {
		.luminance = {1.0f,0.8f,0.8f},
		.texture_pos = {1.0f / 6.0f,0.0f},
		.texture_size = {1.0f / 6.0f,0.5f},
		.flags = MAT_REFLECT
	},
	[BLOCK_SPAWNLIGHT] = {
		.luminance = {1.0f,0.8f,0.8f},
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

vec3_t getRayAngleCamera(uint32_t x,uint32_t y){
	vec3_t ray_ang;
	float pxY = (((float)x * 2.0f * (1.0f / WND_RESOLUTION.x)) - 1.0f);
	float pxX = (((float)y * 2.0f * (1.0f / WND_RESOLUTION.y)) - 1.0f);
	float pixelOffsetY = pxY * (1.0f / (FOV.x / WND_RESOLUTION.x * 2.0f));
	float pixelOffsetX = pxX * (1.0f / (FOV.y / WND_RESOLUTION.y * 2.0f));
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
	
	screen_point.x = pos.z * (FOV.x / WND_RESOLUTION.x * 2.0f);
	screen_point.y = pos.y * (FOV.y / WND_RESOLUTION.y * 2.0f);
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
	
	screen_point.x = FOV.x * pos.z / pos.x + WND_RESOLUTION.x / 2.0f;
	screen_point.y = FOV.y * pos.y / pos.x + WND_RESOLUTION.y / 2.0f;
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
	
	screen_point.x = FOV.x * pos.z / pos.x + WND_RESOLUTION.x / 2.0f;
	screen_point.y = FOV.y * pos.y / pos.x + WND_RESOLUTION.y / 2.0f;
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

int block_type = BLOCK_SPHERE;

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
		setVoxelSolid(vx + lx,vy + ly,vz + lz,remove_depth - depth,block_id);
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
	setVoxelSolid(pos.x,pos.y,pos.z,depth,hold->block_id);
}

void rayRemoveVoxel(vec3_t ray_pos,vec3_t direction,int remove_depth){
	traverse_init_t init = initTraverse(ray_pos);
	node_hit_t result = treeRay(ray3Create(init.pos,direction),init.node,ray_pos);
	if(!result.node)
		return;
	node_t node = *(node_t*)dynamicArrayGet(node_root,result.node);
	uint32_t block_depth = node.depth;
	ivec3 block_pos_i = node.pos;
	
	block_t* block = dynamicArrayGet(block_array,node.index);
	uint32_t material = node.type;

	removeVoxel(result.node);

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

uint32_t block_menu_block;
bool in_block_menu;

void playerAddBlock(){
	if(player_gamemode == GAMEMODE_SURVIVAL){
		uint32_t cost = 1 << tMax((12 - edit_depth) * 3,0);
		if(cost > inventory_slot[inventory_select].ammount)
			return;
		inventory_slot[inventory_select].ammount -= cost;
	}
	vec3_t dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRay(ray3Create(init.pos,dir),init.node,camera.pos);
	if(!result.node)
		return;
	node_t node = *(node_t*)dynamicArrayGet(node_root,result.node);
	if(node.type == BLOCK_GENERATOR){
		in_block_menu = true;
		block_menu_block = result.node;
		return;
	}
	block_t* block = dynamicArrayGet(block_array,node.index);
	ivec3 block_pos_i = node.pos;
	uint32_t block_depth = node.depth;
	block_pos_i.a[result.side] += (dir.a[result.side] < 0.0f) * 2 - 1;
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
		pos.x = node.pos.x * block_size;
		pos.y = node.pos.y * block_size;
		pos.z = node.pos.z * block_size;
		plane_ray_t plane = getPlane(pos,dir,result.side,block_size);
		vec3_t plane_pos = vec3subvec3R(plane.pos,camera.pos);
		float dst = rayIntersectPlane(plane_pos,dir,plane.normal);
		vec3_t hit_pos = vec3addvec3R(camera.pos,vec3mulR(dir,dst));
		vec2_t uv = {
			(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
			(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
		};
		uv.x *= 1 << edit_depth - block_depth;
		uv.y *= 1 << edit_depth - block_depth;
		block_pos.a[result.side] += ((dir.a[result.side] > 0.0f) << (edit_depth - block_depth)) - (dir.a[result.side] > 0.0f);
		block_pos.a[plane.x] += tFloorf(uv.x);
		block_pos.a[plane.y] += tFloorf(uv.y);
	}
	if(!model_mode){
		if(player_gamemode == GAMEMODE_SURVIVAL){
			setVoxelSolid(block_pos.x,block_pos.y,block_pos.z,edit_depth,inventory_slot[inventory_select].type);
			return;
		}
		if(material_array[block_type].flags & MAT_LIQUID){
			setVoxel(block_pos.x,block_pos.y,block_pos.z,edit_depth,block_type,0.8f);
			return;
		}
		setVoxelSolid(block_pos.x,block_pos.y,block_pos.z,edit_depth,block_type);
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

void spawnNumberParticle(vec3_t pos,vec3_t direction,uint32_t number){
	vec2_t dir = {direction.x,direction.y};
	dir = vec2normalizeR(dir);
	char str[8];
	if(!number)
		return;
	uint32_t length = 0;
	for(uint32_t i = number;i;i /= 10)
		length++;
	pos.x += dir.x * length * 0.05f;
	pos.y += dir.y * length * 0.05f;
	for(int i = 0;i < length;i++){
		char c = number % 10 + 0x30 - 0x21;
		int x = 9 - c / 10;
		int y = c % 10;

		float t_y = (1.0f / (2048)) * (1024 + x * 8);
		float t_x = (1.0f / (2048 * 3)) * y * 8;
		entity_t* entity = getNewEntity();
		entity->type = 1;
		entity->pos = pos;
		entity->dir = (vec3_t){0.0f,0.0f,0.01f};
		entity->size = 0.1f;
		entity->health = 300;
		entity->texture_pos = (vec2_t){t_x,t_y};
		entity->texture_size = (vec2_t){(1.0f / (2048 * 3)) * 8,(1.0f / (2048)) * 8};
		entity->color = (vec3_t){1.0f,0.2f,0.2f};
		number /= 10;
		pos.x -= dir.x * 0.1f;
		pos.y -= dir.y * 0.1f;
	}
}

void clickLeft(){
	if(player_gamemode == GAMEMODE_SURVIVAL){
		if(inventory_slot[inventory_select].type == -1){
			if(player_attack_state)
				return;
			player_attack_state = PLAYER_FIST_DURATION;
			return;
		}
		playerAddBlock();
		return;
	}
	playerAddBlock();
}

float block_break_progress;
uint32_t block_break_ptr;

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

void createSurvivalWorld(){
	int depth = 7;
	int m = 1 << depth;

	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			for(int z = 0;z < m;z++){
				bool x_condition = x >= m / 2 && x < m / 2 + 4;
				bool y_condition = y >= m / 2 && y < m / 2 + 4;
				bool z_condition = z >= m / 2 && z < m / 2 + 4;
				if(x_condition && y_condition && z_condition){
					continue;
				}
				int roll = TRND1 * 1000;
				if(roll < 30){
					setVoxelSolid(x,y,z,depth,BLOCK_COAL);
					continue;
				}
				setVoxelSolid(x,y,z,depth,BLOCK_STONE);
			}
		}
	}
	setVoxelSolid(m * 2 + 1,m * 2 + 1,m * 2,depth + 2,BLOCK_SPAWNLIGHT);
}

void createFlatWorld(){
	int depth = 7;
	int m = 1 << depth;
	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			setVoxelSolid(x,y,0,depth,BLOCK_STONE);
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
bool show_tree_iteration;
float test_float;

void executeCommand(){
	if(!strcmp(console_buffer,"TREE_ITERATION"))
		show_tree_iteration ^= true;
	if(!strcmp(console_buffer,"CREATE_FLAT")){
		eraseWorld();
		createFlatWorld();
	}
	if(!strcmp(console_buffer,"CREATE_SURVIVAL")){
		eraseWorld();
		createSurvivalWorld();
	}
	if(!strcmp(console_buffer,"TEST_FLOAT")){
		test_float = atof(console_buffer + 11);
		printf(console_buffer + 11);
	}
	if(!strcmp(console_buffer,"GAMEMODE")){
		player_gamemode ^= true;
	}
	if(!strcmp(console_buffer,"O2")){
		node_t node = treeTraverse(camera.pos);
		if(node.type == BLOCK_AIR)
			((air_t*)dynamicArrayGet(air_array,node.index))->o2 += 0.1f;
	}
	if(!strcmp(console_buffer,"QUIT"))
		ExitProcess(0);
	if(!strcmp(console_buffer,"FLY"))
		setting_fly ^= true;
	memset(console_buffer,0,CONSOLE_BUFFER_SIZE);
	console_cursor = 0;
}

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
				console_buffer[console_cursor++] = '\0';
			else if(wParam == VK_OEM_MINUS && GetKeyState(VK_SHIFT) & 0x80)
				console_buffer[console_cursor++] = '_';
			else if(wParam == VK_OEM_MINUS)
				console_buffer[console_cursor++] = '-';
			else{
				switch(wParam){
				case VK_F3: in_console ^= true; break;
				case VK_RETURN: executeCommand(); break;
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
		case VK_F3:
			in_console ^= true;
			break;
		case '1': inventory_select = 2; break;
		case '2': inventory_select = 3; break;
		case '3': inventory_select = 4; break;
		case '4': inventory_select = 5; break;
		case '5': inventory_select = 6; break;
		case '6': inventory_select = 7; break;
		case '7': inventory_select = 8; break;
		case 'Q': inventory_select = 1; break;
		case 'R':
			if(!model_mode)
				break;
			hold_structure_t* buffer = tMallocZero(sizeof(hold_structure_t));
			rotateModel(hold_structure,buffer,0);
			holdStructureFree(hold_structure);
			hold_structure = buffer;
			break;
		case VK_ESCAPE:
			break;
		case VK_F5:
			spawnEntity(camera.pos,vec3mulR(getLookAngle(camera.dir),0.04f),0);
			break;
		case VK_F8:
			test_bool = 0;
			break;
		case VK_F9:
			test_bool = 1;
			break;
		case VK_UP:
			model_mode ^= true;
			break;
		case VK_DOWN:
			break;
		case VK_RIGHT:
			block_type++;
			if(block_type > sizeof(material_array) / sizeof(material_t))
				block_type = sizeof(material_array) / sizeof(material_t);
			break;
		case VK_LEFT:
			block_type--;
			if(block_type < 0)
				block_type = 0;
			break;
		case VK_ADD:
			edit_depth++;
			break;
		case VK_SUBTRACT:
			edit_depth--;
			break;
		}
		break;
	case WM_MBUTTONDOWN:
		vec3_t dir = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(camera.pos);
		node_hit_t result = treeRay(ray3Create(init.pos,dir),init.node,camera.pos);
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
		r_click = true;
		break;
	case WM_LBUTTONDOWN:;
		l_click = true;
		break;
	case WM_MOUSEMOVE:;
		POINT cur;
		RECT offset;
		vec2_t r;
		GetCursorPos(&cur);
		GetWindowRect(window,&offset);
		float mx = (float)(cur.x - (offset.left + WND_SIZE.x / 2)) * 0.005f;
		float my = (float)(cur.y - (offset.top  + WND_SIZE.y / 2)) * 0.005f;
		camera.dir.x += mx;
		camera.dir.y -= my;
		SetCursorPos(offset.left + WND_SIZE.x / 2,offset.top + WND_SIZE.y / 2);
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
	triangle_t triang[8];
	for(int i = 0;i < 8;i++){
		vec3_t screen = pointToScreenZ(point[i]);
		screen.x = screen.x / WND_RESOLUTION.x * 2.0f - 1.0f;
		screen.y = screen.y / WND_RESOLUTION.y * 2.0f - 1.0f;
		triang[i] = (triangle_t){screen.y,screen.x,0.0f,0.0f,0.0f,0.0f,0.0f};
	}
	drawBlockOutline(triang);
}

void castVisibilityRays(){
	traverse_init_t init = initTraverse(camera.pos);
	static uint32_t counter;
	float luminance_accumulate = 0.0f;
	for(int x = counter / 16;x < WND_RESOLUTION.x;x += 16){
		for(int y = counter % 16;y < WND_RESOLUTION.y;y += 16){
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
	luminance_accumulate /= (WND_RESOLUTION.x / 16) * (WND_RESOLUTION.y / 16);
	camera.exposure = camera.exposure * 0.99f + (2.0f - tMinf(luminance_accumulate,1.9f)) * 0.01f;
	camera.exposure = camera.exposure * 0.99f + 0.01f;
	
	counter += TRND1 * 500.0f;
	counter %= 16 * 16;
}

int draw_mode;
#define TREEITERATION_SQUARE_SIZE 16

void collectTreeIteration(){
	traverse_init_t init = initTraverse(camera.pos);
	for(int x = 0;x < WND_RESOLUTION.x;x += TREEITERATION_SQUARE_SIZE){
		for(int y = 0;y < WND_RESOLUTION.y;y += TREEITERATION_SQUARE_SIZE){
			uint32_t c = traverseTreeItt(ray3CreateI(init.pos,getRayAngleCamera(x,y)),init.node);
			guiRectangle((vec2_t){y * (2.0f / WND_RESOLUTION.y) - 1.0f,x * (2.0f / WND_RESOLUTION.x) - 1.0f},(vec2_t){(2.0f / WND_RESOLUTION.y) * TREEITERATION_SQUARE_SIZE,(2.0f / WND_RESOLUTION.x) * TREEITERATION_SQUARE_SIZE},(vec3_t){c * 0.02f,0.0f,0.0f});
		}
	}
}

void drawHardware(){
	triangle_t triang[6];
	triang[0] = (triangle_t){-0.01f,-0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[1] = (triangle_t){-0.01f, 0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[2] = (triangle_t){ 0.01f, 0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[3] = (triangle_t){-0.01f,-0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[4] = (triangle_t){ 0.01f,-0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[5] = (triangle_t){ 0.01f, 0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	
	glUseProgram(shader_program_ui);
	glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_t) * 6,triang,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES,0,6);

	generateBlockOutline();
		
	glUseProgram(shaderprogram);
	glBufferData(GL_ARRAY_BUFFER,triangle_count * sizeof(triangle_t),triangles,GL_DYNAMIC_DRAW);
	if(!test_bool)
		glDrawArrays(GL_TRIANGLES,0,triangle_count);
	else{
		glDrawArrays(GL_LINES,0,triangle_count);
		/*
		for(int i = 0;i < RD_MASK_X;i++){
			for(int j = 0;j < RD_MASK_Y;j++){
				if(!mask[i][j])
					continue;
				guiRectangle((vec2_t){(float)j / RD_MASK_Y * 2.0f - 1.0f,(float)i / RD_MASK_X * 2.0f - 1.0f},(vec2_t){2.0f / (WND_RESOLUTION.y / RD_MASK_SIZE),2.0f / (WND_RESOLUTION.x / RD_MASK_SIZE)},(vec3_t){1.0f,0.0f,0.0f});
			}
		}
		glBufferData(GL_ARRAY_BUFFER,triangle_count * sizeof(triangle_t),triangles,GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,triangle_count);
		*/
	}
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
	return (vec3_t){s_color.b,s_color.g,s_color.r};
}

void drawBlockSelect(vec3_t block_pos,vec3_t luminance[3],float block_size,uint32_t material_id){
	material_t material = material_array[material_id];
	vec2_t x_direction = vec2addvec2R((vec2_t){camera.dir.x + 0.1f,0.1f},(vec2_t){M_PI * 0.5f,0.0f});
	vec2_t y_direction = vec2addvec2R((vec2_t){camera.dir.x + 0.1f,camera.dir.y + 0.1f},(vec2_t){0.0f,M_PI * 0.5f});
	vec2_t z_direction = vec2addvec2R((vec2_t){camera.dir.x + 0.1f,camera.dir.y + 0.1f},(vec2_t){0.0f,0.0f});
	vec3_t y_angle = getLookAngle(x_direction);
	vec3_t x_angle = getLookAngle(y_direction);
	vec3_t z_angle = getLookAngle(z_direction);
	
	vec3_t pos = camera.pos;
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
	vec3_t luminance[3];
	material_t material = material_array[block_type];
	if(material.flags & MAT_LUMINANT){
		luminance[0] = (vec3_t){material.luminance.r,material.luminance.g,material.luminance.b};
		luminance[1] = (vec3_t){material.luminance.r,material.luminance.g,material.luminance.b};
		luminance[2] = (vec3_t){material.luminance.r,material.luminance.g,material.luminance.b};
	}
	else{
		luminance[0] = getBlockSelectLight((vec2_t){camera.dir.x + M_PI,camera.dir.y});
		luminance[1] = getBlockSelectLight((vec2_t){camera.dir.x - M_PI * 0.5f,camera.dir.y});
		luminance[2] = getBlockSelectLight((vec2_t){camera.dir.x,camera.dir.y + M_PI * 0.5f});
	}
	switch(player_gamemode){
	case GAMEMODE_CREATIVE:
		if(model_mode)
			genHoldStructure(hold_structure,luminance,(vec3_t){0.0f,0.0f,0.0f},1.0f);
		else
			drawBlockSelect(VEC3_ZERO,luminance,1.0f,block_type);
		break;
	case GAMEMODE_SURVIVAL:
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
		if(player_attack_state){
			float z_offset = (PLAYER_FIST_DURATION / 2 - tAbs(PLAYER_FIST_DURATION / 2 - player_attack_state)) * 0.1f;
			vec3addvec3(&block_pos[0],(vec3_t){0.25f,-1.5f,z_offset});
			vec3addvec3(&block_pos[1],(vec3_t){0.25f,-1.5f,z_offset});
			vec3addvec3(&block_pos[2],(vec3_t){0.25f,-1.5f,z_offset});
			vec3addvec3(&block_pos[3],(vec3_t){0.25f,-1.5f,z_offset});
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
	if(block_break_ptr != result.node){
		block_break_progress = 0.0f;
		block_break_ptr = result.node;
		return;
	}
	block_break_progress += 0.01f * material.hardness * (1 << tMax((node->depth - 9) * 3,0));
	if(block_break_progress < 1.0f)
		return;
	float distance = rayNodeGetDistance(camera.pos,node->pos,node->depth,dir,result.side);
	vec3_t spawn_pos = vec3addvec3R(camera.pos,vec3mulR(dir,distance));
	block_break_progress = 0.0f;
	if(node->depth <= 12){
		entity_t* entity = getNewEntity();
		entity->type = 2;
		entity->pos = spawn_pos;
		entity->dir = VEC3_SINGLE((TRND1 - 0.5f) * 0.1f);
		entity->block_type = node->type;
		entity->texture_pos = material.texture_pos;
		entity->texture_size = material.texture_size;
		entity->color = material.luminance;
		entity->size = 0.3f;
		entity->block_ammount = 1 << tMax((12 - tMax(node->depth,9)) * 3,0);
	}
	rayRemoveVoxel(camera.pos,dir,9);
}

void liquidUnderSub(uint32_t node_ptr,node_t* liquid_node){
	block_t* liquid = dynamicArrayGet(block_array,liquid_node->index);
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_AIR){
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		int depth_difference = node->depth - liquid_node->depth;
		float cube_volume_rel = 1 << depth_difference * 2;
		float ammount_2 = ammount * cube_volume_rel;
		if(ammount_2 > 1.0f){
			float overflow = ammount_2 - 1.0f;
			ammount_2 = 1.0f;
			liquid->ammount = overflow / cube_volume_rel;
		}
		setVoxel(node->pos.x,node->pos.y,node->pos.z,node->depth,node->type,ammount_2);
		return;
	}
	if(node->type != BLOCK_PARENT){
		block_t* block = dynamicArrayGet(block_array,node->index);
		if(!(material_array[node->type].flags & MAT_LIQUID))
			return;
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		int depth_difference = node->depth - liquid_node->depth;
		float cube_volume_rel = 1 << depth_difference * 2;
		float ammount_2 = ammount * cube_volume_rel;
		block->ammount_buffer += ammount_2;
		float total_ammount = block->ammount_buffer + block->ammount_buffer;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			block->ammount_buffer -= overflow;
			liquid->ammount = overflow / cube_volume_rel;
		}
		return;
	}
	for(int i = 4;i < 8;i++)
		liquidUnderSub(node_ptr,node->child_s[i]);
}

void liquidUnder(int x,int y,int z,uint32_t depth){
	if(z - 1 < 0)
		return;
	node_t* liquid_node = dynamicArrayGet(node_root,getNode(x,y,z,depth));
	node_t* node = dynamicArrayGet(node_root,getNode(x,y,z - 1,depth));
	block_t* block = dynamicArrayGet(block_array,node->index);
	block_t* liquid = dynamicArrayGet(block_array,liquid_node->index);
	if(node->type == BLOCK_AIR){
		return;
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		setVoxel(x,y,z - 1,depth,liquid_node->type,ammount);
		return;
	}
	if(node->type != BLOCK_PARENT){
		if(!(material_array[node->type].flags & MAT_LIQUID))
			return;
		float ammount = liquid->ammount;
		liquid->ammount = 0.0f;
		block->ammount_buffer += ammount;
		float total_ammount = block->ammount + block->ammount_buffer;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			liquid->ammount = overflow;
			block->ammount_buffer -= overflow;
		}
		return;
	}
	for(int i = 4;i < 8;i++)
		liquidUnderSub(getNode(x,y,z,depth),node->child_s[i],liquid);
}

void liquidSideSub(uint32_t node_ptr,node_t* liquid,uint32_t* neighbour,float level,float dx,float dy){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	block_t* liquid_block = dynamicArrayGet(block_array,liquid->index);
	block_t* block = dynamicArrayGet(block_array,node->index);
	int depth_difference = node->depth - liquid->depth;
	float cube_volume = 1 << depth_difference * 2;
	float square_volume = 1 << depth_difference;
	if(node->type == BLOCK_PARENT){
		float level_add = 1.0f / (2 << depth_difference) + level;
		for(int i = 0;i < 4;i++)
			liquidSideSub(node->child_s[neighbour[i]],liquid,neighbour,i < 2 ? level : level_add,dx,dy);
		return;
	}
	if(node->type == BLOCK_AIR){
		float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
		vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
		float o_x = (float)dx * 0.499f + 0.5f;
		float o_y = (float)dy * 0.499f + 0.5f;
		if(!dx)
			o_x = TRND1;
		if(!dy)
			o_y = TRND1;
		
		vec3addvec3(&block_pos,(vec3_t){o_x,o_y,block->ammount * TRND1});
		vec3mul(&block_pos,block_size);
		entity_t* entity = getNewEntity();
		entity->type = ENTITY_WATER;
		entity->size = 0.2f;
		entity->texture_pos = (vec2_t){0.0f,0.0f};
		entity->texture_size = (vec2_t){0.01f,0.01f};
		entity->color = (vec3_t){0.2f,0.4f,1.0f};
		entity->dir = (vec3_t){dx * 0.03f,dy * 0.03f,0.0f};
		entity->pos = block_pos;
		entity->ammount = liquid_block->ammount * 0.03f;
		entity->block_type = liquid->type;
		entity->block_depth = liquid->depth;
		liquid_block->ammount -= liquid_block->ammount * 0.03f;
		return;
		float ammount = liquid_block->ammount;
		float n = (ammount - level) / (square_volume + 1.0f);
		if(n < 0.0f)
			return;
		float ammount_3 = n * cube_volume;
		liquid_block->ammount -= n;
		if(ammount_3 > 1.0f){
			float overflow = ammount_3 - 1.0f;
			ammount_3 = 1.0f;
			liquid_block->ammount += overflow / cube_volume;
		}
		setVoxel(node->pos.x,node->pos.y,node->pos.z,node->depth,liquid->type,ammount_3);
		return;
	}
	if(!(material_array[node->type].flags & MAT_LIQUID))
		return;

	float ammount = liquid_block->ammount;
	float n = (ammount - level - block->ammount / square_volume) / (square_volume + 1.0f);
	if(n < 0.0f)
		return;
	float ammount_3 = n * cube_volume;
	liquid_block->ammount -= n;
	block->ammount_buffer += ammount_3;
	float total_ammount = block->ammount_buffer + block->ammount;
	if(total_ammount > 1.0f){
		float overflow = total_ammount - 1.0f;
		block->ammount_buffer -= overflow;
		liquid_block->ammount += overflow / cube_volume;
	}
}

void liquidSide(uint32_t x,uint32_t y,uint32_t z,uint32_t depth,uint32_t side,uint32_t* neighbour){
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	node_t* node   = dynamicArrayGet(node_root,getNode(x + dx,y + dy,z + dz,depth));
	node_t* liquid = dynamicArrayGet(node_root,getNode(x,y,z,depth));
	block_t* block = dynamicArrayGet(block_array,node->index);
	block_t* liquid_block = dynamicArrayGet(block_array,liquid->index);
	int depth_difference = liquid->depth - node->depth;
	float cube_volume = 1 << depth_difference * 2;
	float square_volume = 1 << depth_difference;
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			liquidSideSub(node->child_s[neighbour[i]],liquid,neighbour,i < 2 ? 0.0f : 0.5f,-dx,-dy);
		return;
	}
	if(node->type == BLOCK_AIR){
		float block_size = (float)((1 << 20) >> liquid->depth) * MAP_SIZE_INV;
		vec3_t block_pos = {liquid->pos.x,liquid->pos.y,liquid->pos.z};
		float o_x = (float)dx * 0.5f + 0.5f;
		float o_y = (float)dy * 0.5f + 0.5f;
		if(!dx)
			o_x = TRND1;
		if(!dy)
			o_y = TRND1;
		
		vec3addvec3(&block_pos,(vec3_t){o_x,o_y,liquid_block->ammount * TRND1});
		vec3mul(&block_pos,block_size);
		entity_t* entity = getNewEntity();
		entity->type = ENTITY_WATER;
		entity->size = 0.2f;
		entity->texture_pos = (vec2_t){0.0f,0.0f};
		entity->texture_size = (vec2_t){0.01f,0.01f};
		entity->color = (vec3_t){0.2f,0.4f,1.0f};
		entity->dir = (vec3_t){dx * 0.03f,dy * 0.03f,0.0f};
		entity->pos = block_pos;
		entity->ammount = liquid_block->ammount * 0.03f;
		entity->block_type = liquid->type;
		entity->block_depth = liquid->depth;
		liquid_block->ammount -= liquid_block->ammount * 0.03f;
		return;
	}
	if(!(material_array[node->type].flags & MAT_LIQUID))
		return;
	float level = (liquid->pos.z - node->pos.z * square_volume); 
	float n = ((liquid_block->ammount + level) / square_volume - block->ammount) / (square_volume + 1.0f);
	if(n < 0.0f)
		return;
	liquid_block->ammount -= n * cube_volume;
	if(liquid_block->ammount < 0.0f){
		float underflow = -liquid_block->ammount;
		liquid_block->ammount = 0.0f;
		n -= underflow / cube_volume;
	}
	block->ammount_buffer += n;
}

#define BLOCKTICK_RADIUS 40.0f

void gasSpread(uint32_t x,uint32_t y,uint32_t z,uint32_t depth,uint32_t side,uint32_t* neighbour){
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	node_t* node = dynamicArrayGet(node_root,getNode(x + dx,y + dy,z + dz,depth));
	node_t* base = dynamicArrayGet(node_root,getNode(x,y,z,depth));
	if(node->type != BLOCK_AIR)
		return;
	air_t* node_air = dynamicArrayGet(air_array,node->index);
	air_t* base_air = dynamicArrayGet(air_array,base->index);
	float ammount_1 = node_air->o2;
	float ammount_2 = base_air->o2;
	float avg = (ammount_1 + ammount_2) * 0.5f;
	node_air->o2 = avg;
	base_air->o2 = avg;
}

#define POWDER_DEPTH 11

bool powderSideSub(uint32_t base_ptr,uint32_t node_ptr,uint32_t* neighbour,int dx,int dy,int dz){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			if(powderSideSub(base_ptr,node->child_s[neighbour[i]],neighbour,dx,dy,dz))
				return true;
		return false;
	}
	if(node->type != BLOCK_AIR)
		return false;
	node_t* base = dynamicArrayGet(node_root,base_ptr);
	int x = node->pos.x;
	int y = node->pos.y;
	int z = node->pos.z;
	int depth = node->depth;
	for(;depth < base->depth && depth < POWDER_DEPTH;depth++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += dx ? dx == -1 ? 1 : 0 : !!(base->pos.x & 1 << POWDER_DEPTH - depth - 1);
		y += dy ? dy == -1 ? 1 : 0 : !!(base->pos.y & 1 << POWDER_DEPTH - depth - 1);
		z += dz ? dz == -1 ? 1 : 0 : !!(base->pos.z & 1 << POWDER_DEPTH - depth - 1);
	}
	if(base->depth == POWDER_DEPTH){
		node_t* below = dynamicArrayGet(node_root,getNode(x,y,z - 1,POWDER_DEPTH));
		if(below->type < BLOCK_PARENT)
			return false;
		setVoxelSolid(x,y,z,POWDER_DEPTH,1);
		removeVoxel(base_ptr);
		return true;
	}
	for(int i = node->depth;i < POWDER_DEPTH;i++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += dx ? dx == -1 ? 1 : 0 : TRND1 < 0.5f;
		y += dy ? dy == -1 ? 1 : 0 : TRND1 < 0.5f;
		z += dz ? dz == -1 ? 1 : 0 : TRND1 < 0.5f;
	}
	node_t* below = dynamicArrayGet(node_root,getNode(x,y,z - 1,POWDER_DEPTH));
	if(below->type < BLOCK_PARENT)
		return false;
	setVoxelSolid(x,y,z,POWDER_DEPTH,1);
	removeVoxel(base_ptr);
	int depth_mask = (1 << POWDER_DEPTH - base->depth) - 1;
	addSubVoxel(base->pos.x << 1,base->pos.y << 1,base->pos.z << 1,x - dx & depth_mask,y - dy & depth_mask,z - dz & depth_mask,POWDER_DEPTH - base->depth,POWDER_DEPTH,1);
	return true;
}

bool powderSide(uint32_t node_ptr,int side){
	node_t* base = dynamicArrayGet(node_root,node_ptr);
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	uint32_t adjent = getNode(base->pos.x + dx,base->pos.y + dy,base->pos.z + dz,base->depth);
	return powderSideSub(node_ptr,adjent,border_block_table[side],dx,dy,dz);
}

bool powderUnderSub(uint32_t base_ptr,uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 4;i++)
			if(powderUnderSub(base_ptr,node->child_s[border_block_table[4][i]]))
				return true;
		return false;
	}
	if(node->type != BLOCK_AIR)
		return false;
	node_t* base = dynamicArrayGet(node_root,base_ptr);
	int x = node->pos.x;
	int y = node->pos.y;
	int z = node->pos.z;
	int depth = node->depth;
	for(;depth < base->depth && depth < POWDER_DEPTH;depth++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += !!(base->pos.x & 1 << POWDER_DEPTH - depth - 1);
		y += !!(base->pos.y & 1 << POWDER_DEPTH - depth - 1);
		z++;
	}
	if(base->depth >= POWDER_DEPTH){
		setVoxelSolid(x,y,z,base->depth,1);
		removeVoxel(base_ptr);
		return true;
	}
	for(;depth < POWDER_DEPTH;depth++){
		x <<= 1;
		y <<= 1;
		z <<= 1;
		x += TRND1 < 0.5f;
		y += TRND1 < 0.5f;
		z++;
	}
	setVoxelSolid(x,y,z,POWDER_DEPTH,1);
	removeVoxel(base_ptr);
	int depth_mask = (1 << POWDER_DEPTH - base->depth) - 1;
	addSubVoxel(base->pos.x << 1,base->pos.y << 1,base->pos.z << 1,x & depth_mask,y & depth_mask,z + 1 & depth_mask,POWDER_DEPTH - base->depth,POWDER_DEPTH,1);
	return true;
}

bool powderUnder(uint32_t node_ptr){
	node_t* base = dynamicArrayGet(node_root,node_ptr);
	return powderUnderSub(node_ptr,getNode(base->pos.x,base->pos.y,base->pos.z - 1,base->depth));
}

void blockTick(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	float distance = sdCube(camera.pos,block_pos,block_size);
	if(distance > BLOCKTICK_RADIUS)
		return;
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 8;i++)
			blockTick(node->child_s[i]);
		return;
	}
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
			liquidSide(node->pos.x,node->pos.y,node->pos.z,node->depth,0,border_block_table[0]);
			liquidSide(node->pos.x,node->pos.y,node->pos.z,node->depth,1,border_block_table[1]);
			liquidSide(node->pos.x,node->pos.y,node->pos.z,node->depth,2,border_block_table[2]);
			liquidSide(node->pos.x,node->pos.y,node->pos.z,node->depth,3,border_block_table[3]);
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

void blockTransferBuffer(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	float distance = sdCube(camera.pos,block_pos,block_size);
	if(distance > BLOCKTICK_RADIUS)
		return;
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 8;i++)
			blockTransferBuffer(node->child_s[i]);
		return;
	}
	if(node->type == BLOCK_AIR)
		return;
	material_t material = material_array[node->type];
	if(material.flags & MAT_LIQUID){
		block_t* block = dynamicArrayGet(block_array,node->index);
		block->ammount += block->ammount_buffer;
		block->ammount_buffer = 0.0f;
		if(!block->ammount)
			removeVoxel(node_ptr);
	}
}

int main_thread_status;

void draw(){
	uint64_t global_tick = GetTickCount64();
	uint64_t time = 0;
	HANDLE lighting_thread = CreateThread(0,0,lighting,0,0,0);
	for(;;){
		is_rendering = true;

		static bool hand_light;
		vec3_t hand_light_luminance;
		vec3_t hand_light_pos;
		if(in_console){
			drawString((vec2_t){-0.8f,0.8f},0.03f,"console");
			drawString((vec2_t){-0.8f,0.7f},0.02f,console_buffer);
			guiRectangle((vec2_t){-0.9f,0.1f},RD_SQUARE(0.8f),(vec3_t){0.1f,0.2f,0.25f});
		}
		if(!context_is_gl){
			initGL(context);
		}
		for(int i = 0;i < 8;i++){
			guiFrame((vec2_t){-0.383f + i * 0.17f * RD_RATIO,-0.95f},RD_SQUARE(0.15f),VEC3_ZERO,0.01f);
			guiInventoryContent(&inventory_slot[i],(vec2_t){-0.388f + i * 0.17f * RD_RATIO,-0.925f},RD_SQUARE(0.1f));
		}
		for(int i = 0;i < 8;i++){
			vec3_t color;
			if(inventory_select == i)
				color = (vec3_t){1.0f,0.0f,0.0f};
			else if(i == 0)
				color = (vec3_t){1.0f,1.0f,0.0f};
			else
				color = (vec3_t){1.0f,1.0f,1.0f};
			guiFrame((vec2_t){-0.385 + i * 0.17f * RD_RATIO,-0.955f},RD_SQUARE(0.15f),color,0.02f);
		}
		float f = tMin(__rdtsc() - time >> 18,WND_RESOLUTION.x - 1);
		f /= WND_RESOLUTION_X / 2.0f;
		time = __rdtsc();
		guiRectangle((vec2_t){-0.9f,-1.0f},(vec2_t){0.01f,f},(vec3_t){0.0f,1.0f,0.0f});
		main_thread_status = 0;
		genBlockSelect();
		castVisibilityRays();
		if(show_tree_iteration)
			collectTreeIteration();
		else{
			setViewPlanes();
			sceneGatherTriangles(0);
		}
		main_thread_status = 1;
		drawHardware();
		
		while(main_thread_status == 1)
			Sleep(1);
		memset(mask,0,RD_MASK_X * RD_MASK_Y);
		triangle_count = 0;

		SwapBuffers(context);
		glClearColor(LUMINANCE_SKY.r,LUMINANCE_SKY.g,LUMINANCE_SKY.b,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		uint64_t tick_new = GetTickCount64();
		int difference = tick_new - global_tick;
		if(hand_light){
			hand_light = false;
		}
		blockTick(0);
		blockTransferBuffer(0);
		entityTick(difference / 4);
		for(int i = 0;i < difference / 4;i++)
			powerGridTick();
		if(!in_console)
			physics(difference / 4);
		if(player_attack_state){
			playerAttack();
			player_attack_state -= difference / 4;
			if(player_attack_state < 0)
				player_attack_state = 0;
		}
		if(!model_mode && material_array[block_type].flags & MAT_LUMINANT){
			vec3_t luminance = material_array[block_type].luminance;
			hand_light_luminance = luminance;
			hand_light_pos = camera.pos;
			hand_light = true;
		}
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

		//trySpawnEnemy();
		global_tick = tick_new;
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

void main(){ 
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
	triangles = tMalloc(sizeof(triangle_t) * 10000000);
	texture_atlas = tMalloc(sizeof(pixel_t) * TEXTURE_ATLAS_SIZE * TEXTURE_ATLAS_SIZE);

	bmi.bmiHeader.biWidth  = WND_RESOLUTION.y;
	bmi.bmiHeader.biHeight = WND_RESOLUTION.x;

	for(int x = 0;x < TEXTURE_ATLAS_SIZE * TEXTURE_ATLAS_SIZE;x++)
		texture_atlas[x] = (pixel_t){255,255,255,0};

	texture_t font = loadBMP("img/font.bmp");

	for(int x = 256;x < 256 + 80;x++){
		for(int y = 0;y < 80;y++){
			pixel_t color = font.data[(x - 256) * 80 + y];
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y] = (pixel_t){color.b,color.g,color.r,0};
			if(color.b)
				texture_atlas[x * TEXTURE_ATLAS_SIZE + y].a = 0xff;
		}
	}

	for(int x = 2048 - 128;x < 2048;x++){
		for(int y = 0;y < 128;y++){
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].r = tNoise(x * TEXTURE_ATLAS_SIZE + y) % 0x7f + 0x80;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].g = tNoise(x * TEXTURE_ATLAS_SIZE + y) % 0x7f + 0x80;
			texture_atlas[x * TEXTURE_ATLAS_SIZE + y].b = tNoise(x * TEXTURE_ATLAS_SIZE + y) % 0x7f + 0x80;
		}
	}

	createFlatWorld();
	//createSurvivalWorld();

	vec3_t spawnPos = (vec3_t){camera.pos.x,camera.pos.y,MAP_SIZE};
	float distance = rayGetDistance(spawnPos,(vec3_t){0.01f,0.01f,-1.0f}) - 15.0f;
	spawnPos.z -= distance;
	camera.pos = spawnPos;
	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE | WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(false);

	initSound(window);
	CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw,0,0,0);

	SetThreadPriority(physics_thread,THREAD_PRIORITY_ABOVE_NORMAL);

	MSG msg;
	while(GetMessageA(&msg,window,0,0)){
		TranslateMessage(&msg);
 		DispatchMessageA(&msg); 
	}
}