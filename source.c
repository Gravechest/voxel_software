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
#include "menu.h"
#include "opengl.h"
#include "world_triangles.h"
#include "fog.h"
#include "sound.h"
#include "entity.h"
#include "player.h"
#include "draw.h"

#include <gl/GL.h>

#pragma comment(lib,"opengl32")

bool lighting_thread_done = true;

HANDLE physics_thread; 
HANDLE thread_render;  
HANDLE lighting_thread;
HANDLE entity_thread;

vec3 music_box_pos;

bool context_is_gl;

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = (WNDPROC)proc,.lpszClassName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,32,BI_RGB};
HWND window;
HDC context;
vram_t vram;
camera_t camera = {.pos.z = MAP_SIZE + 2.0f,.pos.x = MAP_SIZE + 0.5f,.pos.y = MAP_SIZE + 0.5f};
float camera_exposure = 1.0f;

int edit_depth = 5;

volatile bool is_rendering;

int test_bool = 0;

unsigned int global_tick;

volatile uint light_new_stack_ptr;
volatile light_new_stack_t light_new_stack[LIGHT_NEW_STACK_SIZE];

volatile char frame_ready;

bool setting_smooth_lighting = true;
bool setting_fly = false;

bool player_gamemode = GAMEMODE_CREATIVE;

texture_t texture[16];

pixel_t* texture_atlas;

uint inventory_select = 1;

bool in_console;

uint border_block_table[6][4] = {
	{0,2,4,6},
	{1,3,5,7},
	{0,1,4,5},
	{2,3,6,7},
	{0,1,2,3},
	{4,5,6,7}
};

inventoryslot_t inventory_slot[9] = {
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1}
};

enum{
	BLOCK_WHITE,
	BLOCK_SANDSTONE,
	BLOCK_SKIN,
	BLOCK_SPAWNLIGHT
};

material_t material_array[] = {
	{
		.luminance = {0.9f,0.9f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f}
	},
	{
		.luminance = {0.1f,0.6f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_POWDER
	},
	{
		.luminance = {1.0f,1.0f,1.0f},
		.texture_pos = {1.0f / 6.0f,0.0f},
		.texture_size = {1.0f / 6.0f,0.5f},
		.hardness = 1.0f,
	},
	{
		.luminance = {1.0f,0.8f,0.8f},
		.texture_pos = {1.0f / 6.0f,0.0f},
		.texture_size = {1.0f / 6.0f,0.5f},
		.flags = MAT_REFLECT
	},
	{
		.luminance = {1.0f,0.8f,0.8f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_LIQUID
	},
	{
		.luminance = {9.0f,9.0f,9.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.0f,0.0f},
		.flags = MAT_LUMINANT
	},
	{
		.luminance = {9.0f,9.0f,9.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.0f,0.0f},
		.flags = MAT_LUMINANT
	},
	{
		.luminance = {3.0f,9.0f,3.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.0f,0.0f},
		.flags = MAT_LUMINANT
	},
	{
		.luminance = {9.0f,3.0f,3.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.0f,0.0f},
		.flags = MAT_LUMINANT
	},
	{
		.luminance = {3.0f,3.0f,9.0f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.0f,0.0f},
		.flags = MAT_LUMINANT
	},
	{
		.luminance = {0.9f,0.9f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_REFRACT
	},
	{
		.luminance = {0.3f,0.3f,0.9f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_REFRACT
	},
	{
		.luminance = {0.3f,0.9f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_REFRACT
	},
	{
		.luminance = {0.9f,0.3f,0.3f},
		.texture_pos = {0.0f,0.0f},
		.texture_size = {0.01f,0.01f},
		.flags = MAT_REFRACT
	},
};

float sdCube(vec3 point,vec3 cube,float cube_size){
	vec3 distance;
	distance.x = tMaxf(cube.x - cube_size * 0.5f - point.x,-(cube.x + cube_size * 0.5f - point.x));
	distance.y = tMaxf(cube.y - cube_size * 0.5f - point.y,-(cube.y + cube_size * 0.5f - point.y));
	distance.z = tMaxf(cube.z - cube_size * 0.5f - point.z,-(cube.z + cube_size * 0.5f - point.z));
	distance.x = tMaxf(distance.x,0.0f);
	distance.y = tMaxf(distance.y,0.0f);
	distance.z = tMaxf(distance.z,0.0f);
	return sqrtf(distance.x * distance.x + distance.y * distance.y + distance.z * distance.z);
}

void spawnLuminantParticle(vec3 pos,vec3 dir,vec3 luminance,uint fuse){
	entity_t* entity = getNewEntity();
	entity->pos = pos;
	entity->dir = dir;
	entity->color = luminance;
	entity->type = 1;
	entity->fuse = fuse;
}

void spawnEntity(vec3 pos,vec3 dir,uint type){
	entity_t* entity = getNewEntity();
	entity->pos = pos;
	entity->dir = dir;
	entity->type = type;
	entity->fuse = 0;
	switch(type){
	case 0:
		entity->color = (vec3){0.5f,0.5f,0.5f};
		break;
	case 1:
		entity->color = TRND1 < 0.5f ? (vec3){0.2f,1.0f,0.2f} : (vec3){0.2f,0.2f,1.0f};
		break;
	}
}

texture_t loadBMP(char* name){
	HANDLE h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if(!h)
		return;
	int fsize = GetFileSize(h,0);
	int offset;
	SetFilePointer(h,0x0a,0,0);
	ReadFile(h,&offset,4,0,0);
	SetFilePointer(h,offset,0,0);
	int size = sqrtf((fsize-offset) / 3);
	texture_t texture;
	texture.data = MALLOC(sizeof(pixel_t) * size * size * 2);
	texture.data_rev = MALLOC(sizeof(pixel_t) * size * size * 2);
	texture.size = size;
	_BitScanReverse(&texture.mipmap_level_count,size);
	 
	char* temp = MALLOC(fsize + 5);
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

	MFREE(temp);
	CloseHandle(h);
	return texture;
}

vec2 sampleCube(vec3 v,uint* faceIndex){
	float ma;
	vec2 uv;
	vec3 vAbs = vec3absR(v);
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y){
		*faceIndex = (v.z < 0.0f) + 4;
		ma = 0.5f / vAbs.z;
		uv = (vec2){v.z < 0.0f ? -v.x : v.x, -v.y};
		return (vec2){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
	}
	if(vAbs.y >= vAbs.x){
		*faceIndex = (v.y < 0.0f) + 2;
		ma = 0.5f / vAbs.y;
		uv = (vec2){v.x, v.y < 0.0f ? -v.z : v.z};
		return (vec2){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
	}
	*faceIndex = v.x < 0.0f;
	ma = 0.5f / vAbs.x;
	uv = (vec2){v.x < 0.0f ? v.z : -v.z, -v.y};
	return (vec2){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
}

vec3 getLookAngle(vec2 angle){
	vec3 ang;
	ang.x = cosf(angle.x) * cosf(angle.y);
	ang.y = sinf(angle.x) * cosf(angle.y);
	ang.z = sinf(angle.y);
	return ang;
}

vec3 getRayAngle(uint x,uint y){
	vec3 ray_ang;
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

vec3 pointToScreenRenderer(vec3 point){
	vec3 screen_point;
	vec3 pos = vec3subvec3R(point,camera.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec3){0.0f,0.0f,0.0f};
	
	screen_point.x = pos.z * (FOV.x / WND_RESOLUTION.x * 2.0f);
	screen_point.y = pos.y * (FOV.y / WND_RESOLUTION.y * 2.0f);
	screen_point.z = pos.x;
	return screen_point;
}

vec3 pointToScreenZ(vec3 point){
	vec3 screen_point;
	vec3 pos = vec3subvec3R(point,camera.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec3){0.0f,0.0f,0.0f};
	
	screen_point.x = FOV.x * pos.z / pos.x + WND_RESOLUTION.x / 2.0f;
	screen_point.y = FOV.y * pos.y / pos.x + WND_RESOLUTION.y / 2.0f;
	screen_point.z = pos.x;
	return screen_point;
}

vec2 pointToScreen(vec3 point){
	vec2 screen_point;
	vec3 pos = vec3subvec3R(point,camera.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec2){0.0f,0.0f};
	
	screen_point.x = FOV.x * pos.z / pos.x + WND_RESOLUTION.x / 2.0f;
	screen_point.y = FOV.y * pos.y / pos.x + WND_RESOLUTION.y / 2.0f;
	return screen_point;
}

plane_t getPlane(vec3 pos,vec3 dir,uint side,float block_size){
	switch(side){
	case VEC3_X: return (plane_t){{pos.x + (dir.x < 0.0f) * block_size,pos.y,pos.z},{(dir.x < 0.0f) ? 1.0f : -1.0f,0.0f,0.0f},VEC3_Y,VEC3_Z,VEC3_X};
	case VEC3_Y: return (plane_t){{pos.x,pos.y + (dir.y < 0.0f) * block_size,pos.z},{0.0f,(dir.y < 0.0f) ? 1.0f : -1.0f,0.0f},VEC3_X,VEC3_Z,VEC3_Y};
	case VEC3_Z: return (plane_t){{pos.x,pos.y,pos.z + (dir.z < 0.0f) * block_size},{0.0f,0.0f,(dir.z < 0.0f) ? 1.0f : -1.0f},VEC3_X,VEC3_Y,VEC3_Z};
	}
}

float rayIntersectPlane(vec3 pos,vec3 dir,vec3 plane){
	return vec3dotR(pos,plane) / vec3dotR(dir,plane);
}

int block_type;

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

int tool_select;

typedef struct hold_structure{
	bool is_filled;
	uint block_id;
	union{
		struct hold_structure* child_s[8];
		struct hold_structure* child[2][2][2];
	};
}hold_structure_t;

hold_structure_t hold_structure;
 
void holdStructureSet(hold_structure_t* hold,ivec3 pos,uint depth){
	if(hold->is_filled){
		setVoxelSolid(pos.x,pos.y,pos.z,depth,hold->block_id);
		return;
	}
	pos.x <<= 1;
	pos.y <<= 1;
	pos.z <<= 1;
	for(int i = 0;i < 8;i++){
		if(!hold->child_s[i])
			continue;
		uint dx = (i & 1) != 0;
		uint dy = (i & 2) != 0;
		uint dz = (i & 4) != 0;
		holdStructureSet(hold->child_s[i],(ivec3){pos.x + dx,pos.y + dy,pos.z + dz},depth + 1);
	}
}

void rayRemoveVoxel(vec3 ray_pos,vec3 direction,int remove_depth){
	traverse_init_t init = initTraverse(ray_pos);
	node_hit_t result = treeRay(ray3Create(init.pos,direction),init.node,ray_pos);
	if(!result.node)
		return;
	node_t node = node_root[result.node];
	uint block_depth = node.depth;
	ivec3 block_pos_i = node.pos;
	
	block_t* block = node.block;
	uint material = block->material;

	removeVoxel(result.node);

	float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;

	if(block_depth >= remove_depth)
		return;

	int depht_difference = remove_depth - block_depth;
	vec3 block_pos = {
		block_pos_i.x << depht_difference,
		block_pos_i.y << depht_difference,
		block_pos_i.z << depht_difference
	};
	int edit_size = 1 << remove_depth;
	vec3 pos = {
		block_pos_i.x * block_size,
		block_pos_i.y * block_size,
		block_pos_i.z * block_size
	};

	plane_t plane = getPlane(pos,direction,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,ray_pos);
	float dst = rayIntersectPlane(plane_pos,direction,plane.normal);
	vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(direction,dst));

	vec2 uv = {
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
		vec2 rnd_dir = (vec2){TRND1 - 0.5f,TRND1 - 0.5f};
		vec3mul(&rnd_dir,0.1f);
		for(int i = 0;i < 10;i++){
			vec3 dir = getLookAngle(vec2addvec2R(camera.dir,rnd_dir));
			rayRemoveVoxel(camera.pos,dir,LM_MAXDEPTH + 4);
		}
	}
}
	
void playerAddBlock(){
	if(player_gamemode == GAMEMODE_SURVIVAL){
		uint cost = 1 << tMax((12 - edit_depth) * 3,0);
		if(cost > inventory_slot[inventory_select].ammount)
			return;
		inventory_slot[inventory_select].ammount -= cost;
	}
	vec3 dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRay(ray3Create(init.pos,dir),init.node,camera.pos);
	if(!result.node)
		return;
	node_t node = node_root[result.node];
	block_t* block = node.block;
	ivec3 block_pos_i = node.pos;
	uint block_depth = node.depth;
	block_pos_i.a[result.side] += (dir.a[result.side] < 0.0f) * 2 - 1;
	vec3 block_pos;
	if(block_depth > edit_depth){
		int shift = block_depth - edit_depth;
		block_pos = (vec3){
			block_pos_i.x >> shift,
			block_pos_i.y >> shift,
			block_pos_i.z >> shift
		};
	}
	else{
		int shift = edit_depth - block_depth;
		block_pos = (vec3){
			block_pos_i.x << shift,
			block_pos_i.y << shift,
			block_pos_i.z << shift
		};
	}
	if(block_depth < edit_depth){
		vec3 pos;
		float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;
		pos.x = node.pos.x * block_size;
		pos.y = node.pos.y * block_size;
		pos.z = node.pos.z * block_size;
		plane_t plane = getPlane(pos,dir,result.side,block_size);
		vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
		float dst = rayIntersectPlane(plane_pos,dir,plane.normal);
		vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(dir,dst));
		vec2 uv = {
			(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
			(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
		};
		uv.x *= 1 << edit_depth - block_depth;
		uv.y *= 1 << edit_depth - block_depth;
		block_pos.a[result.side] += ((dir.a[result.side] > 0.0f) << (edit_depth - block_depth)) - (dir.a[result.side] > 0.0f);
		block_pos.a[plane.x] += tFloorf(uv.x);
		block_pos.a[plane.y] += tFloorf(uv.y);
	}
	if(!tool_select){
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
	holdStructureSet(&hold_structure,(ivec3){block_pos.x,block_pos.y,block_pos.z},edit_depth);
}

float rayIntersectSphere(vec3 ray_pos,vec3 sphere_pos,vec3 ray_dir,float radius){
    vec3 rel_pos = vec3subvec3R(ray_pos,sphere_pos);
	float a = vec3dotR(ray_dir,ray_dir);
	float b = 2.0f * vec3dotR(rel_pos,ray_dir);
	float c = vec3dotR(rel_pos,rel_pos) - radius * radius;
	float h = b * b - 4.0f * a * c;
    if(h < 0.0f)
		return -1.0f;
	return (-b - sqrtf(h)) / (2.0f * a);
}

void spawnNumberParticle(vec3 pos,vec3 direction,uint number){
	vec2 dir = {direction.x,direction.y};
	dir = vec2normalizeR(dir);
	char str[8];
	if(!number)
		return;
	uint length = 0;
	for(uint i = number;i;i /= 10)
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
		entity->dir = (vec3){0.0f,0.0f,0.01f};
		entity->size = 0.1f;
		entity->health = 300;
		entity->texture_pos = (vec2){t_x,t_y};
		entity->texture_size = (vec2){(1.0f / (2048 * 3)) * 8,(1.0f / (2048)) * 8};
		entity->color = (vec3){1.0f,0.2f,0.2f};
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
	switch(tool_select){
	case 2:  playerShoot(); break;
	default: playerAddBlock();
	}
}

float block_break_progress;
uint block_break_ptr;

void playerRemoveBlock(){
	vec3 dir = getLookAngle(camera.dir);
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

void fillHoldStructure(hold_structure_t* hold,uint node_ptr){
	node_t node = node_root[node_ptr];
	if(node.block){
		hold->is_filled = true;
		hold->block_id = node.block->material;
		return;
	}
	for(int i = 0;i < 8;i++){
		if(!node.child_s[i])
			continue;
		hold->child_s[i] = MALLOC_ZERO(sizeof(hold_structure_t));
		fillHoldStructure(hold->child_s[i],node.child_s[i]);
	}
}

void holdStructureFree(hold_structure_t* hold){
	for(int i = 0;i < 8;i++){
		if(!hold->child_s[i])
			continue;
		holdStructureFree(hold->child_s[i]);
		hold->child_s[i] = 0;
	}
	memset(hold,0,sizeof(hold_structure_t));
	MFREE(hold);
}

int console_cursor;
#define CONSOLE_BUFFER_SIZE 40
char console_buffer[40];

void executeCommand(){
	if(!strcmp(console_buffer,"GAMEMODE")){
		player_gamemode ^= true;
	}
	if(!strcmp(console_buffer,"O2")){
		node_t node = treeTraverse(camera.pos);
		if(node.air){
			node.air->o2 += 0.1f;
		}
	}
	if(!strcmp(console_buffer,"QUIT"))
		ExitProcess(0);
	if(!strcmp(console_buffer,"FLY"))
		setting_fly ^= true;
	memset(console_buffer,0,CONSOLE_BUFFER_SIZE);
	console_cursor = 0;
}

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_KEYDOWN:
		if(in_console){
			if(wParam >= '0' && wParam <= '9')
				console_buffer[console_cursor++] = wParam;
			else if(wParam >= 'A' && wParam <= 'Z')
				console_buffer[console_cursor++] = wParam;
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
		case VK_ESCAPE:
			menu_select ^= 1;
			ShowCursor(menu_select);
			if(menu_select)
				break;
			SetCursorPos(WND_SIZE.x / 2,WND_SIZE.y / 2);
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
			tool_select += true;
			if(tool_select == 3)
				tool_select = 0;
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
		vec3 dir = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(camera.pos);
		node_hit_t result = treeRay(ray3Create(init.pos,dir),init.node,camera.pos);
		if(!result.node)
			break;
		if(!(GetKeyState(VK_CONTROL) & 0x80)){
			block_type = node_root[result.node].block->material;
			break;
		}
		node_t node = node_root[result.node];
		float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
		vec3 block_pos = vec3mulR((vec3){node.pos.x,node.pos.y,node.pos.z},block_size);
		plane_t plane = getPlane(block_pos,dir,result.side,block_size);
		vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
		int side = result.side * 2 + (dir.a[result.side] < 0.0f);
		float distance = rayIntersectPlane(plane_pos,dir,plane.normal);
		vec3 pos = vec3addvec3R(camera.pos,vec3mulR(dir,distance - 0.01f));
		uint node_select = getNodeFromPos(pos,edit_depth);
		if(!node_select)
			break;
		tool_select = 0;
		for(int i = 0;i < 8;i++){
			if(!hold_structure.child_s[i])
				continue;
			holdStructureFree(hold_structure.child_s[i]);
			hold_structure.child_s[i] = 0;
		}
		fillHoldStructure(&hold_structure,node_select);
		tool_select = 1;
		break;
	case WM_RBUTTONDOWN:
		if(menu_select)
			break;
		r_click = true;
		break;
	case WM_LBUTTONDOWN:;
		if(menu_select){
			POINT cur;
			GetCursorPos(&cur);
			cur.y = WND_SIZE.y - cur.y - 1;
			cur.x = cur.x * WND_RESOLUTION.y / WND_SIZE.x;
			cur.y = cur.y * WND_RESOLUTION.x / WND_SIZE.y;
			executeButton((ivec2){cur.x,cur.y});
			break;
		}
		l_click = true;
		break;
	case WM_MOUSEMOVE:;
		if(menu_select)
			break;
		POINT cur;
		RECT offset;
		vec2 r;
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

void drawBresenhamLine(int x1,int y1,int x2,int y2,pixel_t color){
    int dx = tAbs(x2 - x1);
    int dy = tAbs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    for(;;){
		vram.draw[x1 * (int)WND_RESOLUTION.y + y1] = color;
        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if(e2 > -dy){
            err -= dy;
            x1 += sx;
        }
        if(e2 < dx){
            err += dx;
            y1 += sy;
        }
    }
}

ivec2 scanline_vram[WND_RESOLUTION_X];

void setScanline(vec2 pos_1,vec2 pos_2){
	int p_begin,p_end;
	float delta,delta_pos;
	if(pos_1.x > pos_2.x){
		p_end = pos_2.x;
		p_begin = pos_1.x;
		delta = (pos_2.y - pos_1.y) / (pos_2.x - pos_1.x);
		delta_pos = pos_1.y + delta * ((int)pos_1.x-pos_1.x);
	}
	else{
		p_begin = pos_2.x;
		p_end = pos_1.x;
		delta = (pos_1.y - pos_2.y) / (pos_1.x - pos_2.x);
		delta_pos = pos_2.y + delta * ((int)pos_2.x-pos_2.x);
	}
	if(p_begin < 0 || p_end >= WND_RESOLUTION.x) return;
	if(p_end < 0) p_end = 0;
	if(p_begin >= WND_RESOLUTION.x){
		delta_pos -= delta * (p_begin-WND_RESOLUTION.x);
		p_begin = WND_RESOLUTION.x;
	}
	while(p_begin-- > p_end){
		scanline_vram[p_begin].x = tMin(scanline_vram[p_begin].x,delta_pos);
		scanline_vram[p_begin].y = tMax(scanline_vram[p_begin].y,delta_pos);
		delta_pos -= delta;
	}
}
	
void drawQuad(vec2 pos[4],pixel_t color){
	for(int i = 0;i < 4;i++){
		if(pos[i].x == 0.0f || pos[i].y == 0.0f)
			return;
	}
	int max_x = tMax(tMax(pos[0].x,pos[1].x),tMax(pos[2].x,pos[3].x));
	int min_x = tMin(tMin(pos[0].x,pos[1].x),tMin(pos[2].x,pos[3].x));
	max_x = tMin(max_x,WND_RESOLUTION.x);
	min_x = tMax(min_x,0);
	for(int i = min_x;i < max_x;i++){
		scanline_vram[i].x = WND_RESOLUTION.y;
		scanline_vram[i].y = 0;
	}
	setScanline(pos[0],pos[1]);
	setScanline(pos[1],pos[2]);
	setScanline(pos[2],pos[3]);
	setScanline(pos[3],pos[0]);
	for(int x = min_x;x < max_x;x++){
		int min_y = tMax(scanline_vram[x].x,0);
		int max_y = tMin(scanline_vram[x].y,WND_RESOLUTION.y);
		for(int y = min_y;y < max_y;y++){
			if(vram.draw[x * (int)WND_RESOLUTION.y + y].a)
				continue;
			vram.draw[x * (int)WND_RESOLUTION.y + y] = color;
		}
	}
}

void drawTriangle(vec2 pos[3],pixel_t color){
	for(int i = 0;i < 4;i++){
		if(pos[i].x == 0.0f || pos[i].y == 0.0f)
			return;
	}
	int max_x = tMax(tMax(pos[0].x,pos[1].x),tMax(pos[2].x,pos[3].x));
	int min_x = tMin(tMin(pos[0].x,pos[1].x),tMin(pos[2].x,pos[3].x));
	max_x = tMin(max_x,WND_RESOLUTION.x);
	min_x = tMax(min_x,0);
	for(int i = min_x;i < max_x;i++){
		scanline_vram[i].x = WND_RESOLUTION.y;
		scanline_vram[i].y = 0;
	}
	setScanline(pos[0],pos[1]);
	setScanline(pos[1],pos[2]);
	setScanline(pos[2],pos[0]);
	for(int x = min_x;x < max_x;x++){
		int min_y = tMax(scanline_vram[x].x,0);
		int max_y = tMin(scanline_vram[x].y,WND_RESOLUTION.y);
		for(int y = min_y;y < max_y;y++){
			if(vram.draw[x * (int)WND_RESOLUTION.y + y].a)
				continue;
			vram.draw[x * (int)WND_RESOLUTION.y + y] = color;
		}
	}
}

void drawLine(vec2 pos_1,vec2 pos_2,pixel_t color){
	if(pos_1.x == 0.0f || pos_2.x == 0.0f)
		return;
	bool bound_1_x = pos_1.x > WND_RESOLUTION.x - 1.0f || pos_1.x <= 0.0f;
	bool bound_1_y = pos_1.y > WND_RESOLUTION.y - 1.0f || pos_1.y <= 0.0f;
	bool bound_2_x = pos_2.x > WND_RESOLUTION.x - 1.0f || pos_2.x <= 0.0f;
	bool bound_2_y = pos_2.y > WND_RESOLUTION.y - 1.0f || pos_2.y <= 0.0f;
	bool bound_1 = bound_1_x || bound_1_y;
	bool bound_2 = bound_2_x || bound_2_y;
	if(bound_1 || bound_2)
		return;
	if(bound_1){
		vec2 temp = pos_1;
		pos_1 = pos_2;
		pos_2 = temp;
	}
	drawBresenhamLine(pos_1.x,pos_1.y,pos_2.x,pos_2.y,color);
}

void generateBlockOutline(){
	vec3 look_direction = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRay(ray3Create(init.pos,look_direction),init.node,camera.pos);
	if(!result.node)
		return;
	node_t node = node_root[result.node];
	block_t* block = node.block;
	float edit_size = (float)MAP_SIZE / (1 << edit_depth) * 2.0f;

	uint block_depth = node.depth;
	ivec3 block_pos_i = node.pos;
			
	block_pos_i.a[result.side] += (look_direction.a[result.side] < 0.0f) * 2 - 1;
	vec3 block_pos;
	if(block_depth > edit_depth)
		block_pos = (vec3){block_pos_i.x >> (block_depth - edit_depth),block_pos_i.y >> (block_depth - edit_depth),block_pos_i.z >> (block_depth - edit_depth)};
	else
		block_pos = (vec3){block_pos_i.x << (edit_depth - block_depth),block_pos_i.y << (edit_depth - block_depth),block_pos_i.z << (edit_depth - block_depth)};
	vec3 point[8];
	vec2 screen_point[8];

	if(block_depth < edit_depth){
		vec3 pos;
		float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;
		pos.x = node.pos.x * block_size;
		pos.y = node.pos.y * block_size;
		pos.z = node.pos.z * block_size;
		plane_t plane = getPlane(pos,look_direction,result.side,block_size);
		vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
		float dst = rayIntersectPlane(plane_pos,look_direction,plane.normal);
		vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(look_direction,dst));
		vec2 uv = {
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

	point[0] = (vec3){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[1] = (vec3){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + edit_size};
	point[2] = (vec3){block_pos.x + 0.0f,block_pos.y + edit_size,block_pos.z + 0.0f};
	point[3] = (vec3){block_pos.x + 0.0f,block_pos.y + edit_size,block_pos.z + edit_size};
	point[4] = (vec3){block_pos.x + edit_size,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[5] = (vec3){block_pos.x + edit_size,block_pos.y + 0.0f,block_pos.z + edit_size};
	point[6] = (vec3){block_pos.x + edit_size,block_pos.y + edit_size,block_pos.z + 0.0f};
	point[7] = (vec3){block_pos.x + edit_size,block_pos.y + edit_size,block_pos.z + edit_size};
	triangles_t triang[8];
	for(int i = 0;i < 8;i++){
		vec3 screen = pointToScreenZ(point[i]);
		screen.x = screen.x / WND_RESOLUTION.x * 2.0f - 1.0f;
		screen.y = screen.y / WND_RESOLUTION.y * 2.0f - 1.0f;
		triang[i] = (triangles_t){screen.y,screen.x,0.0f,0.0f,0.0f,0.0f,0.0f};
	}
	drawBlockOutline(triang);
}

typedef struct{
	vec2 pos;
	vec2 texture_pos;
	float padding[7];
}triangle_plain_texture;

void guiRectangle(vec2 pos,vec2 size,vec3 color){
	triangles[triangle_count + 0] = (triangles_t){pos.x,pos.y,1.0f,0.0f,0.0f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 1] = (triangles_t){pos.x + size.x,pos.y,1.0f,0.01f,0.0f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 2] = (triangles_t){pos.x + size.x,pos.y + size.y,1.0f,0.01f,0.01f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 3] = (triangles_t){pos.x,pos.y,1.0f,0.0f,0.0f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 4] = (triangles_t){pos.x,pos.y + size.y,1.0f,0.0f,0.01f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 5] = (triangles_t){pos.x + size.x,pos.y + size.y,1.0f,0.01f,0.01f,color.r,color.g,color.b,1.0f};
	triangle_count += 6;
}

void castVisibilityRays(){
	traverse_init_t init = initTraverse(camera.pos);
	static uint counter;
	for(int x = counter / 16;x < WND_RESOLUTION.x;x += 16){
		for(int y = counter % 16;y < WND_RESOLUTION.y;y += 16){
			vec3 ray_angle = getRayAngle(x,y);
			node_hit_t hit = treeRay(ray3Create(init.pos,ray_angle),init.node,camera.pos);
			if(!hit.node)
				continue;
			node_t* node = &node_root[hit.node];
			uint side = hit.side * 2 + (ray_angle.a[hit.side] < 0.0f);
			if(!node->block)
				continue;
			if(!node->block->side[side].luminance_p)
				lightNewStackPut(node,node->block,side);
		}
	}
	/*
	for(int x = 0;x < WND_RESOLUTION.x;x += 16){
		for(int y = 0;y < WND_RESOLUTION.y;y += 16){
			uint c = traverseTreeItt(ray3CreateI(init.pos,getRayAngle(x,y)),init.node);
			guiRectangle((vec2){y / 16 * 0.01f - 1.0f,x / 16 * 0.01f - 1.0f},(vec2){0.01f,0.01f},(vec3){c * 0.02f,0.0f,0.0f});
		}
	}
	*/
	counter += TRND1 * 500.0f;
	counter %= 16 * 16;
}

#define RD_SOFTWARE 0

void calibrateExposure(){
	float accumulate = 0.0f;
	for(int i = 0;i < triangle_count;i++){
		accumulate += tMaxf(tMaxf(triangles[i].lighting.r,triangles[i].lighting.g),triangles[i].lighting.b);
	}
	accumulate /= triangle_count;
	accumulate = (1.0f / accumulate);
	camera_exposure = (camera_exposure * 99.0f + accumulate) / 100.0f;
	camera_exposure = (camera_exposure * 99.0f + 1.0f) / 100.0f;
}

void drawSoftware(){
	glUseProgram(shader_program_plain_texture);
	for(int i = 0;i < triangle_count;i+=3){
		vec2 verticles[3];
		verticles[0].x = (triangles[i + 0].pos.y + 1.0f) * WND_RESOLUTION.x / 2.0f;
		verticles[0].y = (triangles[i + 0].pos.x + 1.0f) * WND_RESOLUTION.y / 2.0f;
		verticles[1].x = (triangles[i + 1].pos.y + 1.0f) * WND_RESOLUTION.x / 2.0f;
		verticles[1].y = (triangles[i + 1].pos.x + 1.0f) * WND_RESOLUTION.y / 2.0f;
		verticles[2].x = (triangles[i + 2].pos.y + 1.0f) * WND_RESOLUTION.x / 2.0f;
		verticles[2].y = (triangles[i + 2].pos.x + 1.0f) * WND_RESOLUTION.y / 2.0f;
		pixel_t color = {triangles[i].lighting.x * 255.0f,triangles[i].lighting.y * 255.0f,triangles[i].lighting.z * 255.0f};
		color.a = true;
		drawTriangle(verticles,color);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,WND_RESOLUTION.y,WND_RESOLUTION.x, 0, GL_RGBA, GL_UNSIGNED_BYTE,vram.draw);
	glGenerateMipmap(GL_TEXTURE_2D);
	triangle_plain_texture triangle[] = {
		{-1.0f,-1.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f},
		{-1.0f,1.0f,0.0f,1.0f,1.0f,1.0f,1.0f,1.0f},
		{1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f},
		{-1.0f,-1.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f},
		{1.0f,-1.0f,1.0f,0.0f,1.0f,1.0f,1.0f,1.0f},
		{1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f},
	};
	glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_plain_texture) * 6,triangle,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES,0,6);
	memset(vram.draw,0,sizeof(pixel_t) * WND_RESOLUTION.y * WND_RESOLUTION.x);
}

void drawHardware(){
	triangles_t triang[6];
	triang[0] = (triangles_t){-0.01f,-0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[1] = (triangles_t){-0.01f, 0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[2] = (triangles_t){ 0.01f, 0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[3] = (triangles_t){-0.01f,-0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[4] = (triangles_t){ 0.01f,-0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};
	triang[5] = (triangles_t){ 0.01f, 0.01f,1.0f,0.0f,0.0f,0.0f,0.0f};

	glUseProgram(shader_program_ui);
	glBufferData(GL_ARRAY_BUFFER,sizeof(triangles_t) * 6,triang,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES,0,6);

	generateBlockOutline();
			
	glUseProgram(shaderprogram);
	glBufferData(GL_ARRAY_BUFFER,triangle_count * sizeof(triangles_t),triangles,GL_DYNAMIC_DRAW);
	if(!test_bool){
		glDrawArrays(GL_TRIANGLES,0,triangle_count);
	}
	else{
		glDrawArrays(GL_LINES,0,triangle_count);
		/*
		for(int i = 0;i < RD_MASK_X;i++){
			for(int j = 0;j < RD_MASK_Y;j++){
				
				if(!mask[i][j])
					continue;
					

				guiRectangle((vec2){(float)j / RD_MASK_Y * 2.0f - 1.0f,(float)i / RD_MASK_X * 2.0f - 1.0f},(vec2){2.0f / (WND_RESOLUTION.y / RD_MASK_SIZE),2.0f / (WND_RESOLUTION.x / RD_MASK_SIZE)},(vec3){1.0f,0.0f,0.0f});
			}
		}
		glBufferData(GL_ARRAY_BUFFER,triangle_count * sizeof(triangles_t),triangles,GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES,0,triangle_count);
		*/
	}
}

#define BLOCK_SELECT_LIGHT_QUALITY 16

vec3 getBlockSelectLight(vec2 direction){
	vec3 normal = getLookAngle(direction);
	vec3 s_color = VEC3_ZERO;
	float weight_total = 0.0f;
	direction.x -= M_PI * 0.5f - M_PI * 0.5f / BLOCK_SELECT_LIGHT_QUALITY;
	direction.y -= M_PI * 0.5f - M_PI * 0.5f / BLOCK_SELECT_LIGHT_QUALITY;
	traverse_init_t init = initTraverse(camera.pos);
	for(int i = 0;i < BLOCK_SELECT_LIGHT_QUALITY;i++){
		for(int j = 0;j < BLOCK_SELECT_LIGHT_QUALITY;j++){
			vec3 angle = getLookAngle(direction);
			float weight = tAbsf(vec3dotR(angle,normal));
			vec3 luminance = rayGetLuminanceDir(init.pos,direction,init.node,camera.pos);
			vec3mul(&luminance,weight);
			vec3addvec3(&s_color,luminance);
			weight_total += weight;
			direction.y += M_PI / BLOCK_SELECT_LIGHT_QUALITY;
		}
		direction.y -= M_PI;
		direction.x += M_PI / BLOCK_SELECT_LIGHT_QUALITY;
	}
	vec3div(&s_color,weight_total);
	return (vec3){s_color.b,s_color.g,s_color.r};
}

void drawBlockSelect(vec3 block_pos,vec3 luminance[3],float block_size,uint material_id){
	material_t material = material_array[material_id];
	vec2 x_direction = vec2addvec2R((vec2){camera.dir.x + 0.1f,0.1f},(vec2){M_PI * 0.5f,0.0f});
	vec2 y_direction = vec2addvec2R((vec2){camera.dir.x + 0.1f,camera.dir.y + 0.1f},(vec2){0.0f,M_PI * 0.5f});
	vec2 z_direction = vec2addvec2R((vec2){camera.dir.x + 0.1f,camera.dir.y + 0.1f},(vec2){0.0f,0.0f});
	vec3 y_angle = getLookAngle(x_direction);
	vec3 x_angle = getLookAngle(y_direction);
	vec3 z_angle = getLookAngle(z_direction);
	
	vec3 pos = camera.pos;
	vec3addvec3(&pos,vec3mulR(x_angle,-2.5f + block_pos.x));
	vec3addvec3(&pos,vec3mulR(y_angle, 1.5f + block_pos.y));
	vec3addvec3(&pos,vec3mulR(z_angle, 2.5f + block_pos.z));
	vec3 verticle_screen[8];
	for(int i = 0;i < 8;i++){
		vec3 x = i & 1 ? vec3mulR(x_angle,block_size) : VEC3_ZERO;
		vec3 y = i & 2 ? vec3mulR(y_angle,block_size) : VEC3_ZERO;
		vec3 z = i & 4 ? vec3mulR(z_angle,block_size) : VEC3_ZERO;
		vec3 verticle_pos = pos;
		vec3addvec3(&verticle_pos,x);
		vec3addvec3(&verticle_pos,y);
		vec3addvec3(&verticle_pos,z);
		vec3 screen = pointToScreenRenderer(verticle_pos);
		verticle_screen[i].y = screen.x;
		verticle_screen[i].x = screen.y;
		verticle_screen[i].z = screen.z;
	}

	vec2 texture_coord[4];
	texture_coord[0] = material.texture_pos;
	texture_coord[1] = (vec2){material.texture_pos.x,material.texture_pos.y + material.texture_size.y};
	texture_coord[2] = (vec2){material.texture_pos.x + material.texture_size.x,material.texture_pos.y};
	texture_coord[3] = (vec2){material.texture_pos.x + material.texture_size.x,material.texture_pos.y + material.texture_size.y};

	triangles[triangle_count + 0 ] = (triangles_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 1 ] = (triangles_t){verticle_screen[1],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 2 ] = (triangles_t){verticle_screen[3],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 3 ] = (triangles_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 4 ] = (triangles_t){verticle_screen[2],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 5 ] = (triangles_t){verticle_screen[3],texture_coord[3],1.0f,1.0f,1.0f,1.0f};

	triangles[triangle_count + 6 ] = (triangles_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 7 ] = (triangles_t){verticle_screen[1],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 8 ] = (triangles_t){verticle_screen[5],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 9 ] = (triangles_t){verticle_screen[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 10] = (triangles_t){verticle_screen[4],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 11] = (triangles_t){verticle_screen[5],texture_coord[3],1.0f,1.0f,1.0f,1.0f};

	triangles[triangle_count + 12] = (triangles_t){verticle_screen[1],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 13] = (triangles_t){verticle_screen[3],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 14] = (triangles_t){verticle_screen[7],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 15] = (triangles_t){verticle_screen[1],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 16] = (triangles_t){verticle_screen[5],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 17] = (triangles_t){verticle_screen[7],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	
	for(int i = 0;i < 6;i++){
		triangles[triangle_count + i + 0 ].lighting = vec3mulvec3R(material.luminance,luminance[0]);
		triangles[triangle_count + i + 6 ].lighting = vec3mulvec3R(material.luminance,luminance[1]);
		triangles[triangle_count + i + 12].lighting = vec3mulvec3R(material.luminance,luminance[2]);
	}
	triangle_count += 18;
}

void genHoldStructure(hold_structure_t* hold,vec3 luminance[3],vec3 block_pos,float block_size){
	if(hold->is_filled){
		drawBlockSelect(block_pos,luminance,block_size,hold->block_id);
		return;
	}
	block_size *= 0.5f;
	for(int i = 0;i < 8;i++){
		if(!hold->child_s[i])
			continue;
		float dz = i & 1 ? 0.0f : block_size;
		float dy = i & 2 ? 0.0f : block_size;
		float dx = i & 4 ? 0.0f : block_size;
		vec3 block_pos_child = vec3addvec3R(block_pos,(vec3){dx,dy,dz});
		genHoldStructure(hold->child_s[i],luminance,block_pos_child,block_size);
	}
}

void genBlockSelect(){
	vec3 luminance[3];
	material_t material = material_array[block_type];
	if(material.flags & MAT_LUMINANT){
		luminance[0] = (vec3){material.luminance.b,material.luminance.g,material.luminance.r};
		luminance[1] = (vec3){material.luminance.b,material.luminance.g,material.luminance.r};
		luminance[2] = (vec3){material.luminance.b,material.luminance.g,material.luminance.r};
	}
	else{
		luminance[0] = getBlockSelectLight((vec2){camera.dir.x + M_PI,camera.dir.y});
		luminance[1] = getBlockSelectLight((vec2){camera.dir.x - M_PI * 0.5f,camera.dir.y});
		luminance[2] = getBlockSelectLight((vec2){camera.dir.x,camera.dir.y + M_PI * 0.5f});
	}
	switch(player_gamemode){
	case GAMEMODE_CREATIVE:
		switch(tool_select){
		case 0:
			drawBlockSelect(VEC3_ZERO,luminance,1.0f,block_type);
			break;
		case 1:
			genHoldStructure(&hold_structure,luminance,(vec3){0.0f,0.0f,0.0f},1.0f);
			break;
		case 2:
			drawBlockSelect((vec3){0.75f,0.0f,0.0f},luminance,0.25f,0);
			drawBlockSelect((vec3){0.75f,0.0f,0.25f},luminance,0.25f,0);
			drawBlockSelect((vec3){0.75f,0.0f,0.5f},luminance,0.25f,0);
			drawBlockSelect((vec3){0.75f,0.0f,0.75f},luminance,0.25f,0);
			break;
		}
		break;
	case GAMEMODE_SURVIVAL:
		vec3 block_pos[4] = {
			{0.75f,0.0f,0.0f},
			{0.75f,0.0f,0.25f},
			{0.75f,0.0f,0.5f},
			{0.75f,0.0f,0.75f}
		};
		if(GetKeyState(VK_RBUTTON) & 0x80){
			vec3addvec3(&block_pos[0],(vec3){0.25f,-1.5f,0.0f});
			vec3addvec3(&block_pos[1],(vec3){0.25f,-1.5f,0.0f});
			vec3addvec3(&block_pos[2],(vec3){0.25f,-1.5f,0.0f});
			vec3addvec3(&block_pos[3],(vec3){0.25f + (TRND1 - 0.5f) * 0.1f,-1.5f + (TRND1 - 0.5f) * 0.1f,0.0f});
		}
		if(player_attack_state){
			float z_offset = (PLAYER_FIST_DURATION / 2 - tAbs(PLAYER_FIST_DURATION / 2 - player_attack_state)) * 0.1f;
			vec3addvec3(&block_pos[0],(vec3){0.25f,-1.5f,z_offset});
			vec3addvec3(&block_pos[1],(vec3){0.25f,-1.5f,z_offset});
			vec3addvec3(&block_pos[2],(vec3){0.25f,-1.5f,z_offset});
			vec3addvec3(&block_pos[3],(vec3){0.25f,-1.5f,z_offset});
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
	vec3 dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = treeRay(ray3Create(init.pos,dir),init.node,camera.pos);
	if(!result.node)
		return;
	node_t node = node_root[result.node];
	material_t material = material_array[node.block->material];
	if(block_break_ptr != result.node){
		block_break_progress = 0.0f;
		block_break_ptr = result.node;
		return;
	}
	block_break_progress += 0.01f * material.hardness * (1 << tMax((node.depth - 9) * 3,0));
	if(block_break_progress < 1.0f)
		return;
	float distance = rayNodeGetDistance(camera.pos,node.pos,node.depth,dir,result.side);
	vec3 spawn_pos = vec3addvec3R(camera.pos,vec3mulR(dir,distance));
	block_break_progress = 0.0f;
	if(node.depth <= 12){
		entity_t* entity = getNewEntity();
		entity->type = 2;
		entity->pos = spawn_pos;
		entity->dir = VEC3_SINGLE((TRND1 - 0.5f) * 0.1f);
		entity->block_type = node.block->material;
		entity->texture_pos = material.texture_pos;
		entity->texture_size = material.texture_size;
		entity->color = material.luminance;
		entity->size = 0.3f;
		entity->block_ammount = 1 << tMax((12 - tMax(node.depth,9)) * 3,0);
	}
	rayRemoveVoxel(camera.pos,dir,9);
}

void guiFrame(vec2 pos,vec2 size,vec3 color,float thickness){
	guiRectangle(pos,(vec2){size.x,thickness},color);
	guiRectangle(pos,(vec2){thickness,size.y},color);
	guiRectangle((vec2){pos.x,pos.y + size.y},(vec2){size.x + thickness,thickness},color);
	guiRectangle((vec2){pos.x + size.x,pos.y},(vec2){thickness,size.y},color);
}

void drawChar(vec2 pos,vec2 size,char c){
	c -= 0x21;
	int x = 9 - c / 10;
	int y = c % 10;

	float t_y = (1.0f / (2048)) * (1024 + x * 8);
	float t_x = (1.0f / (2048 * 3)) * y * 8;

	vec2 texture_coord[4];
	texture_coord[0] = (vec2){t_x,t_y};
	texture_coord[1] = (vec2){t_x,t_y + (1.0f / (2048)) * 8};
	texture_coord[2] = (vec2){t_x + (1.0f / (2048 * 3)) * 8,t_y};
	texture_coord[3] = (vec2){t_x + (1.0f / (2048 * 3)) * 8,t_y + (1.0f / (2048)) * 8};

	triangles[triangle_count++] = (triangles_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x,pos.y + size.y,1.0f,texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x + size.x,pos.y,1.0f,texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
}

void drawNumber(vec2 pos,vec2 size,uint number){
	char str[8];
	if(!number){
		drawChar(pos,size,'0');
		return;
	}
	uint length = 0;
	for(uint i = number;i;i /= 10)
		length++;
	pos.x += size.x * length;
	for(int i = 0;i < length;i++){
		drawChar(pos,size,number % 10 + 0x30);
		number /= 10;
		pos.x -= size.x;
	}
}

void drawString(vec2 pos,float size,char* str){
	while(*str){
		drawChar(pos,(vec2){size,size},*str++);
		pos.x += size;
	}
}

void guiInventoryContent(inventoryslot_t* slot,vec2 pos,vec2 size){
	if(slot->type == -1)
		return;
	drawNumber(vec2addvec2R(pos,(vec2){-0.04f,0.065f}),vec2mulR(size,0.3f),slot->ammount);
	material_t material = material_array[slot->type];
	vec2 texture_coord[4];
	texture_coord[0] = material.texture_pos;
	texture_coord[1] = (vec2){material.texture_pos.x,material.texture_pos.y + material.texture_size.y};
	texture_coord[2] = (vec2){material.texture_pos.x + material.texture_size.x,material.texture_pos.y};
	texture_coord[3] = (vec2){material.texture_pos.x + material.texture_size.x,material.texture_pos.y + material.texture_size.y};

	triangles[triangle_count++] = (triangles_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x,pos.y + size.y,1.0f,texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x + size.x,pos.y,1.0f,texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangles_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
}

void liquidUnderSub(uint node_ptr,node_t* liquid){
	node_t* node = &node_root[node_ptr];
	if(node->air){
		float ammount = liquid->block->ammount;
		liquid->block->ammount = 0.0f;
		int depth_difference = node->depth - liquid->depth;
		float cube_volume_rel = 1 << depth_difference * 2;
		float ammount_2 = ammount * cube_volume_rel;
		if(ammount_2 > 1.0f){
			float overflow = ammount_2 - 1.0f;
			ammount_2 = 1.0f;
			liquid->block->ammount = overflow / cube_volume_rel;
		}
		setVoxel(node->pos.x,node->pos.y,node->pos.z,node->depth,liquid->block->material,ammount_2);
		return;
	}
	if(node->block){
		if(!(material_array[node->block->material].flags & MAT_LIQUID))
			return;
		float ammount = liquid->block->ammount;
		liquid->block->ammount = 0.0f;
		int depth_difference = node->depth - liquid->depth;
		float cube_volume_rel = 1 << depth_difference * 2;
		float ammount_2 = ammount * cube_volume_rel;
		node->block->ammount_buffer += ammount_2;
		float total_ammount = node->block->ammount_buffer + node->block->ammount_buffer;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			node->block->ammount_buffer -= overflow;
			liquid->block->ammount = overflow / cube_volume_rel;
		}
		return;
	}
	for(int i = 4;i < 8;i++)
		liquidUnderSub(node->child_s[i],liquid);
}

void liquidUnder(int x,int y,int z,uint depth){
	if(z - 1 < 0)
		return;
	node_t* liquid = &node_root[getNode(x,y,z,depth)];
	uint node = getNode(x,y,z - 1,depth);
	if(node_root[node].block){
		if(!(material_array[node_root[node].block->material].flags & MAT_LIQUID))
			return;
		float ammount = liquid->block->ammount;
		liquid->block->ammount = 0.0f;
		node_root[node].block->ammount_buffer += ammount;
		float total_ammount = node_root[node].block->ammount + node_root[node].block->ammount_buffer;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			liquid->block->ammount = overflow;
			node_root[node].block->ammount_buffer -= overflow;
		}
		return;
	}
	if(node_root[node].air){
		return;
		float ammount = liquid->block->ammount;
		liquid->block->ammount = 0.0f;
		setVoxel(x,y,z - 1,depth,liquid->block->material,ammount);
		return;
	}
	for(int i = 4;i < 8;i++)
		liquidUnderSub(node_root[node].child_s[i],liquid);
}

void liquidSideSub(uint node_ptr,node_t* liquid,uint* neighbour,float level,float dx,float dy){
	node_t* node = &node_root[node_ptr];
	int depth_difference = node->depth - liquid->depth;
	float cube_volume = 1 << depth_difference * 2;
	float square_volume = 1 << depth_difference;
	if(node->block){
		if(!(material_array[node->block->material].flags & MAT_LIQUID))
			return;

		float ammount = liquid->block->ammount;
		float n = (ammount - level - node->block->ammount / square_volume) / (square_volume + 1.0f);
		if(n < 0.0f)
			return;
		float ammount_3 = n * cube_volume;
		liquid->block->ammount -= n;
		node->block->ammount_buffer += ammount_3;
		float total_ammount = node->block->ammount_buffer + node->block->ammount;
		if(total_ammount > 1.0f){
			float overflow = total_ammount - 1.0f;
			node->block->ammount_buffer -= overflow;
			liquid->block->ammount += overflow / cube_volume;
		}
		return;
	}
	if(node->air){
		float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
		vec3 block_pos = {node->pos.x,node->pos.y,node->pos.z};
		float o_x = (float)dx * 0.499f + 0.5f;
		float o_y = (float)dy * 0.499f + 0.5f;
		if(!dx)
			o_x = TRND1;
		if(!dy)
			o_y = TRND1;
		vec3addvec3(&block_pos,(vec3){o_x,o_y,liquid->block->ammount * TRND1});
		vec3mul(&block_pos,block_size);
		entity_t* entity = getNewEntity();
		entity->type = ENTITY_WATER;
		entity->size = 0.2f;
		entity->texture_pos = (vec2){0.0f,0.0f};
		entity->texture_size = (vec2){0.01f,0.01f};
		entity->color = (vec3){0.2f,0.4f,1.0f};
		entity->dir = (vec3){dx * 0.03f,dy * 0.03f,0.0f};
		entity->pos = block_pos;
		entity->ammount = liquid->block->ammount * 0.03f;
		entity->block_type = liquid->block->material;
		entity->block_depth = liquid->depth;
		liquid->block->ammount -= liquid->block->ammount * 0.03f;
		return;
		float ammount = liquid->block->ammount;
		float n = (ammount - level) / (square_volume + 1.0f);
		if(n < 0.0f)
			return;
		float ammount_3 = n * cube_volume;
		liquid->block->ammount -= n;
		if(ammount_3 > 1.0f){
			float overflow = ammount_3 - 1.0f;
			ammount_3 = 1.0f;
			liquid->block->ammount += overflow / cube_volume;
		}
		setVoxel(node->pos.x,node->pos.y,node->pos.z,node->depth,liquid->block->material,ammount_3);
		return;
	}
	float level_add = 1.0f / (2 << depth_difference) + level;
	for(int i = 0;i < 4;i++)
		liquidSideSub(node->child_s[neighbour[i]],liquid,neighbour,i < 2 ? level : level_add,dx,dy);
}

void liquidSide(uint x,uint y,uint z,uint depth,uint side,uint* neighbour){
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	node_t* node   = &node_root[getNode(x + dx,y + dy,z + dz,depth)];
	node_t* liquid = &node_root[getNode(x,y,z,depth)];
	int depth_difference = liquid->depth - node->depth;
	float cube_volume = 1 << depth_difference * 2;
	float square_volume = 1 << depth_difference;
	if(node->block){
		if(!(material_array[node->block->material].flags & MAT_LIQUID))
			return;
		float level = (liquid->pos.z - node->pos.z * square_volume); 
		float n = ((liquid->block->ammount + level) / square_volume - node->block->ammount) / (square_volume + 1.0f);
		if(n < 0.0f)
			return;
		liquid->block->ammount -= n * cube_volume;
		if(liquid->block->ammount < 0.0f){
			float underflow = -liquid->block->ammount;
			liquid->block->ammount = 0.0f;
			n -= underflow / cube_volume;
		}
		node->block->ammount_buffer += n;
		return;
	}
	if(node->air){
		float block_size = (float)((1 << 20) >> liquid->depth) * MAP_SIZE_INV;
		vec3 block_pos = {liquid->pos.x,liquid->pos.y,liquid->pos.z};
		float o_x = (float)dx * 0.5f + 0.5f;
		float o_y = (float)dy * 0.5f + 0.5f;
		if(!dx)
			o_x = TRND1;
		if(!dy)
			o_y = TRND1;
		vec3addvec3(&block_pos,(vec3){o_x,o_y,liquid->block->ammount * TRND1});
		vec3mul(&block_pos,block_size);
		entity_t* entity = getNewEntity();
		entity->type = ENTITY_WATER;
		entity->size = 0.2f;
		entity->texture_pos = (vec2){0.0f,0.0f};
		entity->texture_size = (vec2){0.01f,0.01f};
		entity->color = (vec3){0.2f,0.4f,1.0f};
		entity->dir = (vec3){dx * 0.03f,dy * 0.03f,0.0f};
		entity->pos = block_pos;
		entity->ammount = liquid->block->ammount * 0.03f;
		entity->block_type = liquid->block->material;
		entity->block_depth = liquid->depth;
		liquid->block->ammount -= liquid->block->ammount * 0.03f;
		return;
	}
	for(int i = 0;i < 4;i++)
		liquidSideSub(node->child_s[neighbour[i]],liquid,neighbour,i < 2 ? 0.0f : 0.5f,-dx,-dy);
}

#define BLOCKTICK_RADIUS 40.0f

void gasSpread(uint x,uint y,uint z,uint depth,uint side,uint* neighbour){
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	node_t* node = &node_root[getNode(x + dx,y + dy,z + dz,depth)];
	node_t* base = &node_root[getNode(x,y,z,depth)];
	if(node->block)
		return;
	if(node->air){
		float ammount_1 = node->air->o2;
		float ammount_2 = base->air->o2;
		float avg = (ammount_1 + ammount_2) * 0.5f;
		node->air->o2 = avg;
		base->air->o2 = avg;
		return;
	}
}

#define POWDER_DEPTH 11

bool powderSub(uint node_ptr,uint* neighbour,int dx,int dy,int dz){
	node_t node = node_root[node_ptr];
	if(node.block)
		return false;
	if(node.air){
		int x = node.pos.x;
		int y = node.pos.y;
		int z = node.pos.z;
		if(node.depth == POWDER_DEPTH){
			if(node_root[getNode(x,y,z - 1,POWDER_DEPTH)].block)
				return false;
			setVoxelSolid(x,y,z,POWDER_DEPTH,1);
			removeVoxel(node_ptr);
			return true;
		}
		for(int i = node.depth;i < POWDER_DEPTH;i++){
			x <<= 1;
			y <<= 1;
			z <<= 1;
			x += dx ? dx == -1 ? 1 : 0 : TRND1 < 0.5f;
			y += dy ? dy == -1 ? 1 : 0 : TRND1 < 0.5f;
			z += dz ? dz == -1 ? 1 : 0 : TRND1 < 0.5f;
		}
		if(node_root[getNode(x,y,z - 1,POWDER_DEPTH)].block)
			return false;
		setVoxelSolid(x,y,z,POWDER_DEPTH,1);
		removeVoxel(node_ptr);
		int depth_mask = (1 << POWDER_DEPTH - node.depth) - 1;
		addSubVoxel(node.pos.x << 1,node.pos.y << 1,node.pos.z << 1,x - dx & depth_mask,y - dy & depth_mask,z - dz & depth_mask,POWDER_DEPTH - node.depth,POWDER_DEPTH,1);
		return true;
	}
	for(int i = 0;i < 4;i++)
		if(powderSub(node.child_s[neighbour[i]],neighbour,dx,dy,dz))
			return true;
	return false;
}

bool powderSide(uint node_ptr,int side){
	node_t base = node_root[node_ptr];
	int dx = side >> 1 == VEC3_X ? side & 1 ? -1 : 1 : 0;
	int dy = side >> 1 == VEC3_Y ? side & 1 ? -1 : 1 : 0;
	int dz = side >> 1 == VEC3_Z ? side & 1 ? -1 : 1 : 0;
	node_t* node   = &node_root[getNode(base.pos.x + dx,base.pos.y + dy,base.pos.z + dz,base.depth)];
	uint* neighbour = border_block_table[side];
	if(node->block)
		return false;
	if(node->air){
		int x = base.pos.x + dx;
		int y = base.pos.y + dy;
		int z = base.pos.z + dz;
		if(base.depth == POWDER_DEPTH){
			if(node_root[getNode(x,y,z - 1,POWDER_DEPTH)].block)
				return false;
			setVoxelSolid(x,y,z,POWDER_DEPTH,base.block->material);
			removeVoxel(node_ptr);
			return true;
		}
		for(int i = base.depth;i < POWDER_DEPTH;i++){
			x <<= 1;
			y <<= 1;
			z <<= 1;
			x += dx ? dx == -1 ? 1 : 0 : TRND1 < 0.5f;
			y += dy ? dy == -1 ? 1 : 0 : TRND1 < 0.5f;
			z += dz ? dz == -1 ? 1 : 0 : TRND1 < 0.5f;
		}
		if(node_root[getNode(x,y,z - 1,POWDER_DEPTH)].block)
			return false;
		setVoxelSolid(x,y,z,POWDER_DEPTH,base.block->material);
		removeVoxel(node_ptr);
		int depth_mask = (1 << POWDER_DEPTH - base.depth) - 1;
		addSubVoxel(base.pos.x << 1,base.pos.y << 1,base.pos.z << 1,x - dx & depth_mask,y - dy & depth_mask,z - dz & depth_mask,POWDER_DEPTH - base.depth,POWDER_DEPTH,1);
		return true;
	}
	for(int i = 0;i < 4;i++)
		if(powderSub(node->child_s[neighbour[i]],neighbour,dx,dy,dz))
			return true;
	return false;
}

bool powderUnderSub(uint node_ptr){
	node_t node = node_root[node_ptr];
	if(node.block)
		return false;
	if(node.air){
		int x = node.pos.x;
		int y = node.pos.y;
		int z = node.pos.z;
		if(node.depth == POWDER_DEPTH){
			setVoxelSolid(x,y,z,POWDER_DEPTH,1);
			removeVoxel(node_ptr);
			return true;
		}
		for(int i = node.depth;i < POWDER_DEPTH;i++){
			x <<= 1;
			y <<= 1;
			z <<= 1;
			x += TRND1 < 0.5f;
			y += TRND1 < 0.5f;
			z++;
		}
		setVoxelSolid(x,y,z,POWDER_DEPTH,1);
		removeVoxel(node_ptr);
		int depth_mask = (1 << POWDER_DEPTH - node.depth) - 1;
		addSubVoxel(node.pos.x << 1,node.pos.y << 1,node.pos.z << 1,x & depth_mask,y & depth_mask,z + 1 & depth_mask,POWDER_DEPTH - node.depth,POWDER_DEPTH,1);
		return true;
	}
	for(int i = 0;i < 4;i++)
		if(powderUnderSub(node.child_s[border_block_table[4][i]]))
			return true;
	return false;
}

bool powderUnder(uint node_ptr){
	node_t base = node_root[node_ptr];
	node_t* node = &node_root[getNode(base.pos.x,base.pos.y,base.pos.z - 1,base.depth)];
	if(node->block)
		return false;
	if(node->air){
		int x = base.pos.x;
		int y = base.pos.y;
		int z = base.pos.z - 1;
		if(base.depth == POWDER_DEPTH){
			setVoxelSolid(x,y,z,POWDER_DEPTH,base.block->material);
			removeVoxel(node_ptr);
			return true;
		}
		for(int i = base.depth;i < POWDER_DEPTH;i++){
			x <<= 1;
			y <<= 1;
			z <<= 1;
			x += TRND1 < 0.5f;
			y += TRND1 < 0.5f;
			z++;
		}
		setVoxelSolid(x,y,z,POWDER_DEPTH,base.block->material);
		removeVoxel(node_ptr);
		int depth_mask = (1 << POWDER_DEPTH - base.depth) - 1;
		addSubVoxel(base.pos.x << 1,base.pos.y << 1,base.pos.z << 1,x & depth_mask,y & depth_mask,z + 1 & depth_mask,POWDER_DEPTH - base.depth,POWDER_DEPTH,1);
		return true;
	}
	for(int i = 0;i < 4;i++)
		if(powderUnderSub(node->child_s[border_block_table[4][i]]))
			return true;
	return false;
}

void blockTick(uint node_ptr){
	node_t* node = &node_root[node_ptr];
	if(node->air){
		for(int i = 0;i < 6;i++){
			gasSpread(node->pos.x,node->pos.y,node->pos.z,node->depth,i,border_block_table[i]);
		}
		return;
	}
	if(node->block){
		material_t material = material_array[node->block->material];
		if(material.flags & MAT_LIQUID){
			liquidUnder(node->pos.x,node->pos.y,node->pos.z,node->depth);
			if(node->block->ammount > 0.001f){
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
		return;
	}
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3 block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	float distance = sdCube(camera.pos,block_pos,block_size);
	if(distance > BLOCKTICK_RADIUS)
		return;
	for(int i = 0;i < 8;i++){
		if(!node->child_s[i])
			continue;
		blockTick(node->child_s[i]);
	}
}

void blockTransferBuffer(uint node_ptr){
	node_t* node = &node_root[node_ptr];
	if(node->air){
		return;
	}
	if(node->block){
		material_t material = material_array[node->block->material];
		if(material.flags & MAT_LIQUID){
			node->block->ammount += node->block->ammount_buffer;
			node->block->ammount_buffer = 0.0f;
			if(!node->block->ammount)
				removeVoxel(node_ptr);
		}
		return;
	}
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3 block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	float distance = sdCube(camera.pos,block_pos,block_size);
	if(distance > BLOCKTICK_RADIUS)
		return;
	for(int i = 0;i < 8;i++){
		if(!node->child_s[i])
			continue;
		blockTransferBuffer(node->child_s[i]);
	}
}

void draw(){
	unsigned long long global_tick = GetTickCount64();
	unsigned long long time;
	for(;;){
		is_rendering = true;

		static bool hand_light;
		vec3 hand_light_luminance;
		vec3 hand_light_pos;

		if(in_console){
			drawString((vec2){-0.8f,0.8f},0.03f,"console");
			drawString((vec2){-0.8f,0.7f},0.02f,console_buffer);
			guiRectangle((vec2){-1.0f,0.2f},(vec2){0.8f,0.8f},(vec3){0.1f,0.2f,0.25f});
		}
		if(!context_is_gl){
			memset(vram.draw,0,sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
			memset(vram.render,0,sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
			initGL(context);
		}
		for(int i = 0;i < 8;i++){
			guiFrame((vec2){-0.5f + i * 0.17f,-0.85f},(vec2){0.15f,0.15f},VEC3_ZERO,0.01f);
			guiInventoryContent(&inventory_slot[i],(vec2){-0.475f + i * 0.17f,-0.825f},(vec2){0.1f,0.1f});
		}
		for(int i = 0;i < 8;i++){
			vec3 color;
			if(inventory_select == i)
				color = (vec3){1.0f,0.0f,0.0f};
			else if(i == 0)
				color = (vec3){1.0f,1.0f,0.0f};
			else
				color = (vec3){1.0f,1.0f,1.0f};
			guiFrame((vec2){-0.505f + i * 0.17f,-0.855f},(vec2){0.15f,0.15f},color,0.02f);
		}
		castVisibilityRays();
		genBlockSelect();
		sceneGatherTriangles(0);
		if(RD_SOFTWARE)
			drawSoftware();
		else
			drawHardware();
		//calibrateExposure();
		memset(mask,0,RD_MASK_X * RD_MASK_Y);
		triangle_count = 0;
		float f = tMin(__rdtsc() - time >> 18,WND_RESOLUTION.x - 1);
		f /= WND_RESOLUTION_X / 2.0f;
		f -= 1.0f;
		SwapBuffers(context);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(LUMINANCE_SKY.r,LUMINANCE_SKY.g,LUMINANCE_SKY.b,1.0f);
		glColor3f(0.0f,1.0f,0.0f);
		glBegin(GL_TRIANGLES);
		glVertex2f(-0.9f,-1.0f);
		glVertex2f(-0.9f,f);
		glVertex2f(-0.89f,-1.0f);
		glVertex2f(-0.89f,-1.0f);
		glVertex2f(-0.9f,f);
		glVertex2f(-0.89f,f);
		glEnd();
		time = __rdtsc();
		unsigned long long tick_new = GetTickCount64();
		unsigned int difference = tick_new - global_tick;
		if(hand_light){
			calculateDynamicLight(hand_light_pos,0,20.0f,vec3negR(hand_light_luminance));
			hand_light = false;
		}
		blockTick(0);
		blockTransferBuffer(0);
		entityTick(difference / 4);
		if(!in_console)
			physics(difference / 4);
		if(player_attack_state){
			playerAttack();
			player_attack_state -= difference / 4;
			if(player_attack_state < 0)
				player_attack_state = 0;
		}
		lighting();
		if(tool_select == 0 && material_array[block_type].flags & MAT_LUMINANT){
			vec3 luminance = material_array[block_type].luminance;
			hand_light_luminance = luminance;
			hand_light_pos = camera.pos;
			calculateDynamicLight(camera.pos,0,20.0f,luminance);
			hand_light = true;
		}
		for(int i = 0;i < ENTITY_AMMOUNT;i++){
			if(!entity_array[i].alive)
				continue;
			if(entity_array[i].flags & ENTITY_LUMINANT)
				calculateDynamicLight(entity_array[i].pos,0,10.0f,entity_array[i].color);
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

uint tNoise(uint seed){
	seed += seed << 10;
	seed ^= seed >> 6;
	seed += seed << 3;
	seed ^= seed >> 11;
	seed += seed << 15;
	return seed;
}

void main(){ 
#pragma comment(lib,"Winmm.lib")
	timeBeginPeriod(1);

	node_root = MALLOC(sizeof(node_t) * 1024 * 1024 * 32);
	triangles = MALLOC(sizeof(triangles_t) * 10000000);
	texture_atlas = MALLOC(sizeof(pixel_t) * 1024 * 1024 * 2 * TEXTURE_AMMOUNT);

	bmi.bmiHeader.biWidth  = WND_RESOLUTION.y;
	bmi.bmiHeader.biHeight = WND_RESOLUTION.x;
	
	texture[0].size = 1;
	texture[0].data = MALLOC(1);
	texture[0].data[0] = (pixel_t){220,220,220};

	texture[1] = loadBMP("img/default.bmp");
	texture[2] = loadBMP("img/marble.bmp");
	texture[3] = loadBMP("img/ice.bmp");
	texture[4] = loadBMP("img/geurt.bmp");
	texture[5] = loadBMP("img/grass.bmp");
	texture[6] = loadBMP("img/font.bmp");

	for(int i = 0;i < TEXTURE_AMMOUNT;i++){
		for(int x = 0;x < 1024;x++){
			for(int y = 0;y < 1024;y++){
				uint texture_pos = x * texture[i].size / 1024 * texture[i].size + y * texture[i].size / 1024;
				pixel_t color = texture[i].data[texture_pos];
				texture_atlas[x * 1024 * TEXTURE_AMMOUNT + i * 1024 + y] = (pixel_t){color.b,color.g,color.r};
			}
		}
	}

	for(int x = 0;x < 80;x++){
		for(int y = 0;y < 80;y++){
			pixel_t color = texture[6].data[x * 80 + y];
			texture_atlas[(x + 1024) * 1024 * TEXTURE_AMMOUNT + y] = (pixel_t){color.b,color.g,color.r};
			if(color.b)
				texture_atlas[(x + 1024) * 1024 * TEXTURE_AMMOUNT + y].a = 0xff;
		}
	}

	for(int x = 0;x < 64;x++){
		for(int y = 0;y < 64;y++){
			if(x > 4 && x < 12 && y > 4 && y < 12){
				continue;
			}
			if(y > 4 && y < 12 && x > 48 && x < 60){
				continue;
			}
			texture_atlas[(x + 1024 + 80) * 1024 * TEXTURE_AMMOUNT + y] = (pixel_t){50,180,50};
		}
	}

	material_array[0].texture = texture[0];
	material_array[1].texture = texture[1];
	material_array[2].texture = texture[2];
	material_array[3].texture = texture[3];

	int depth = 7;
	int m = 1 << depth;
	/*
	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			for(int z = 0;z < m;z++){
				if(z == m / 2 && y == m / 2 && x == m / 2)
					continue;
				setVoxelSolid(x,y,z,depth,BLOCK_SANDSTONE);
			}
		}
	}
	setVoxelSolid(m * 4,m * 4 + 2,m * 4 + 4,depth + 3,BLOCK_SANDSTONE);
	setVoxelSolid(m * 2 + 1,m * 2 + 1,m * 2,depth + 2,BLOCK_SPAWNLIGHT);
	*/
	
	uint n_size_bits = 4;
	uint n_size = (1 << n_size_bits);
	uint n_height_bits = 4;
	uint n_height = (1 << n_height_bits);
	for(int x = 0;x < m;x++){
		for(int y = 0;y < m;y++){
			uint c_x = x >> n_size_bits;
			uint c_y = y >> n_size_bits;
			uint n[4];
			n[0] = tNoise((c_x + 0 << 16) + c_y + 0) & n_height - 1;
			n[1] = tNoise((c_x + 0 << 16) + c_y + 1) & n_height - 1;
			n[2] = tNoise((c_x + 1 << 16) + c_y + 0) & n_height - 1;
			n[3] = tNoise((c_x + 1 << 16) + c_y + 1) & n_height - 1;
			uint c2_x = (y & n_size - 1);
			uint c2_y = (x & n_size - 1);
			n[0] = n[0] * c2_x * c2_y / n_size / n_size;
			n[1] = n[1] * c2_x * (n_size - c2_y) / n_size / n_size;
			n[2] = n[2] * (n_size - c2_x) * c2_y / n_size / n_size;
			n[3] = n[3] * (n_size - c2_x) * (n_size - c2_y) / n_size / n_size;

			uint f = (n[0] + n[1] + n[2] + n[3]);
			for(int z = 0;z < m / 2;z++){
			if(f + 1 > z)
				setVoxelSolid(x,y,z,depth,0);	
			}
		}	
	}

	vec3 spawnPos = (vec3){camera.pos.x,camera.pos.y,MAP_SIZE};
	float distance = rayGetDistance(spawnPos,(vec3){0.01f,0.01f,-1.0f}) - 15.0f;
	spawnPos.z -= distance;
	camera.pos = spawnPos;

	vram.draw = MALLOC(sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
	vram.render = MALLOC(sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);

	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE | WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(FALSE);

	initSound(window);

	CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw,0,0,0);

	SetThreadPriority(lighting_thread,THREAD_PRIORITY_BELOW_NORMAL);
	SetThreadPriority(physics_thread,THREAD_PRIORITY_ABOVE_NORMAL);

	MSG msg;
	while(GetMessageA(&msg,window,0,0)){
		TranslateMessage(&msg);
 		DispatchMessageA(&msg); 
	}
}