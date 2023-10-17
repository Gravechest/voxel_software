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
#include "draw.h"
#include "tree.h"
#include "menu.h"
#include "trace.h"
#include "opengl.h"
#include "world_triangles.h"
#include "fog.h"

#include <gl/GL.h>

#pragma comment(lib,"opengl32")

bool lighting_thread_done = true;

HANDLE physics_thread; 
HANDLE thread_render;  
HANDLE lighting_thread;
HANDLE entity_thread;

bool context_is_gl;

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = (WNDPROC)proc,.lpszClassName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,32,BI_RGB};
HWND window;
HDC context;
vram_t vram;
camera_t camera = {.pos.z = MAP_SIZE * 0.4f,.pos.x = 3.0f,.pos.y = 3.0f};
float camera_exposure = 1.0f;

camera_t camera_rd;

int edit_depth = 5;

volatile bool is_rendering;

int test_bool = 0;

unsigned int global_tick;

volatile uint light_new_stack_ptr;
volatile light_new_stack_t light_new_stack[LIGHT_NEW_STACK_SIZE];

volatile char frame_ready;

bool setting_smooth_lighting = true;
bool setting_fly = false;

texture_t texture[16];

pixel_t* texture_atlas;

material_t material_array[] = {
	{.luminance = {0.9f,0.9f,0.9f},.texture_id = 0},
	{.luminance = {1.0f,1.0f,1.0f},.texture_id = 1},
	{.luminance = {0.1f,0.1f,0.5f},.texture_id = 4},
	{.luminance = {0.9f,0.9f,0.9f},.texture_id = 2},
	{.luminance = {0.9f,0.9f,0.9f},.texture_id = 3},
	{.luminance = {0.9f,0.9f,0.9f},.texture_id = 4},
	{.flags = MAT_LUMINANT,.luminance = {4.0f,4.0f,4.0f}},
	{.flags = MAT_LUMINANT,.luminance = {4.0f,1.0f,1.0f}},
	{.flags = MAT_LUMINANT,.luminance = {1.0f,4.0f,1.0f}},
	{.flags = MAT_LUMINANT,.luminance = {1.0f,1.0f,4.0f}},
	{.flags = MAT_LUMINANT,.luminance = {1.0f,4.0f,4.0f}},	
};

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

void render(){
	for(;;){
		StretchDIBits(context,0,0,WND_SIZE.x,WND_SIZE.y,0,0,WND_RESOLUTION.y,WND_RESOLUTION.x,vram.render,&bmi,0,SRCCOPY);
		while(!frame_ready)
			Sleep(1);
		frame_ready = false;
	}
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
	ray_ang.x = camera_rd.tri.x * camera_rd.tri.z;
	ray_ang.y = camera_rd.tri.y * camera_rd.tri.z;
	ray_ang.x -= camera_rd.tri.x * camera_rd.tri.w * pixelOffsetY;
	ray_ang.y -= camera_rd.tri.y * camera_rd.tri.w * pixelOffsetY;
	ray_ang.x -= camera_rd.tri.y * pixelOffsetX;
	ray_ang.y += camera_rd.tri.x * pixelOffsetX;
	ray_ang.z = camera_rd.tri.w + camera_rd.tri.z * pixelOffsetY;
	return ray_ang;
}

vec3 pointTransform(vec3 point){
	vec3 screen_point;
	vec3 pos = vec3subvec3R(point,camera_rd.pos);
	float temp;
	temp  = pos.y * camera.tri.x - pos.x * camera.tri.y;
	pos.x = pos.y * camera.tri.y + pos.x * camera.tri.x;
	pos.y = temp;
	temp  = pos.z * camera.tri.z - pos.x * camera.tri.w;
	pos.x = pos.z * camera.tri.w + pos.x * camera.tri.z;
	pos.z = temp;
	
	if(pos.x <= 0.0f)
		return (vec3){0.0f,0.0f,0.0f};
	
	screen_point.y = FOV.x * pos.z / pos.x + WND_RESOLUTION.x / 2.0f;
	screen_point.x = FOV.y * pos.y / pos.x + WND_RESOLUTION.y / 2.0f;
	screen_point.z = pos.x;
	return screen_point;
}

vec3 pointToScreenZ(vec3 point){
	vec3 screen_point;
	vec3 pos = vec3subvec3R(point,camera_rd.pos);
	float temp;
	temp  = pos.y * camera_rd.tri.x - pos.x * camera_rd.tri.y;
	pos.x = pos.y * camera_rd.tri.y + pos.x * camera_rd.tri.x;
	pos.y = temp;
	temp  = pos.z * camera_rd.tri.z - pos.x * camera_rd.tri.w;
	pos.x = pos.z * camera_rd.tri.w + pos.x * camera_rd.tri.z;
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
	vec3 pos = vec3subvec3R(point,camera_rd.pos);
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

void addSubVoxel(int vx,int vy,int vz,int x,int y,int z,int depth,int block_id){
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
			addSubVoxel(pos_x,pos_y,pos_z,x,y,z,depth,block_id);
			continue;
		}
		setVoxel(vx + lx,vy + ly,vz + lz,edit_depth - depth,block_id);
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
		setVoxel(pos.x,pos.y,pos.z,depth,hold->block_id);
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

void playerAddBlock(){
	vec3 dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = traverseTree(ray3Create(init.pos,dir),init.node);
	if(!result.node)
		return;
	block_node_t node = node_root[result.node];
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
		setVoxel(block_pos.x,block_pos.y,block_pos.z,edit_depth,block_type);
		return;
	}
	holdStructureSet(&hold_structure,(ivec3){block_pos.x,block_pos.y,block_pos.z},edit_depth);
}

void playerRemoveBlock(){
	vec3 dir = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera.pos);
	node_hit_t result = traverseTree(ray3Create(init.pos,dir),init.node);
	if(!result.node)
		return;
	block_node_t node = node_root[result.node];
	uint block_depth = node.depth;
	ivec3 block_pos_i = node.pos;
	
	block_t* block = node.block;
	uint material = block->material;

	removeVoxel(result.node);

	float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;

	if(block_depth >= edit_depth){
		vec3 pos = {block_pos_i.x,block_pos_i.y,block_pos_i.z};
		vec3add(&pos,0.5f);
		vec3mul(&pos,block_size);
		return;
	}

	int depht_difference = edit_depth - block_depth;
	vec3 block_pos = {
		block_pos_i.x << depht_difference,
		block_pos_i.y << depht_difference,
		block_pos_i.z << depht_difference
	};
	int edit_size = 1 << edit_depth;
	vec3 pos = {
		block_pos_i.x * block_size,
		block_pos_i.y * block_size,
		block_pos_i.z * block_size
	};

	plane_t plane = getPlane(pos,dir,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
	float dst = rayIntersectPlane(plane_pos,dir,plane.normal);
	vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(dir,dst));

	vec2 uv = {
		(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
		(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
	};
	uv.x *= 1 << depht_difference;
	uv.y *= 1 << depht_difference;
	block_pos.a[result.side] += ((dir.a[result.side] < 0.0f) << depht_difference) - (dir.a[result.side] < 0.0f);
	block_pos.a[plane.x] += tFloorf(uv.x);
	block_pos.a[plane.y] += tFloorf(uv.y);
	int x,y,z;
	int depht_difference_size = 1 << depht_difference;
	x = (int)block_pos.x & depht_difference_size - 1;
	y = (int)block_pos.y & depht_difference_size - 1;
	z = (int)block_pos.z & depht_difference_size - 1;

	addSubVoxel(block_pos_i.x << 1,block_pos_i.y << 1,block_pos_i.z << 1,x,y,z,depht_difference,material);
}

void updateLightMap(uint node_ptr,vec3 pos,uint depth,float radius){
	static uint square;
	static uint square_add;
	square += (tHash(square) & 1) + 1;
	if(square % 64 == 0)
		calculateNodeLuminance(&node_root[node_ptr],32);

	block_node_t node = node_root[node_ptr];
	float block_size = (float)((1 << 20) >> depth) * MAP_SIZE_INV * 0.5f;
	if(!node.block){
		for(int i = 0;i < 8;i++){
			if(!node.child_s[i])
				continue;
			vec3 pos_2 = {
				node_root[node.child_s[i]].pos.x,
				node_root[node.child_s[i]].pos.y,
				node_root[node.child_s[i]].pos.z
			};
			vec3add(&pos_2,0.5f);
			vec3mul(&pos_2,block_size);
			float distance = vec3distance(pos,pos_2);
			if(block_size + radius > distance)
				updateLightMap(node.child_s[i],pos,depth + 1,radius);
		}
		return;
	}
	block_t* block = node.block;
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT)
		return;
	float block_size_2 = (float)((1 << 20) >> depth) * MAP_SIZE_INV;
	for(int side = 0;side < 6;side++){
		vec3* lightmap = block->side[side].luminance_p;
		if(!lightmap)
			continue;
		vec3 lm_pos = {node.pos.x,node.pos.y,node.pos.z};

		vec3add(&lm_pos,0.5f);
		lm_pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.002f;

		int v_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
		int v_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];

		uint size = GETLIGHTMAPSIZE(depth);

		lm_pos.a[v_x] -= 0.5f - 0.5f / size;
		lm_pos.a[v_y] -= 0.5f - 0.5f / size;

		for(int sub_x = 0;sub_x < tMax(size / 8,1);sub_x++){
			for(int sub_y = 0;sub_y < tMax(size / 8,1);sub_y++){
				uint sized_square = square % (8 * 8);
				square += (tHash(square) & 1) + 1;

				uint x = sized_square / 8 + 1 + sub_x * 8;
				uint y = sized_square % 8 + 1 + sub_y * 8;

				if(x > size || y > size)
					continue;

				vec3 lm_pos_b = lm_pos;

				lm_pos_b.a[v_x] += 1.0f / size * (x - 1);
				lm_pos_b.a[v_y] += 1.0f / size * (y - 1);

				lm_pos_b = vec3divR(lm_pos_b,(float)(1 << depth) * 0.5f);
				vec3mul(&lm_pos_b,MAP_SIZE);
				uint lightmap_location = x * (size + 2) + y;
				vec3 pre = lightmap[lightmap_location];
				setLightMap(&lightmap[lightmap_location],lm_pos_b,side,depth,LM_QUALITY);
				vec3mulvec3(&lightmap[lightmap_location],material.luminance);
				
				if(x == 1)
					lightmap[y] = lightmapDownX(node,side,0,y - 1,size);
				if(y == 1)
					lightmap[x * (size + 2)] = lightmapDownY(node,side,x - 1,0,size);
				if(x == size)
					lightmap[(size + 1) * (size + 2) + y] = lightmapUpX(node,side,x - 1,y - 1,size);
				if(y == size)
					lightmap[x * (size + 2) + (size + 1)] = lightmapUpY(node,side,x - 1,y - 1,size);
				
				if(x == 1 && y == 1)
					lightmap[0] = vec3avgvec3R(lightmap[1],lightmap[size + 2]);
				if(x == 1 && y == size)
					lightmap[size + 1] = vec3avgvec3R(lightmap[size],lightmap[size + 2 + size + 1]);
				if(x == size && y == 1)
					lightmap[(size + 1) * (size + 2)] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + 1],lightmap[(size + 1) * (size + 1)]);
				if(x == size && y == size)
					lightmap[(size + 2) * (size + 2) - 1] = vec3avgvec3R(lightmap[(size + 2) * (size + 2) - 2],lightmap[(size + 2) * (size + 1) - 1]);
				
				vec3 post = lightmap[lightmap_location];
				if(!post.r && !post.g && !post.b){
					vec3 avg_color = VEC3_ZERO;
					uint count = 0;
					for(int i = 0;i < 4;i++){
						int dx = (int[]){0,0,-1,1}[i];
						int dy = (int[]){-1,1,0,0}[i];
						vec3 neighbour = lightmap[lightmap_location + dx * size + dy];
						bool red   = neighbour.r <= 0.0f;
						bool green = neighbour.g <= 0.0f;
						bool blue  = neighbour.b <= 0.0f;
						if(red || green || blue)
							continue;
						count++;
						vec3addvec3(&avg_color,neighbour);
					}
					if(!count)
						continue;
					lightmap[lightmap_location] = vec3mulR(avg_color,0.95f / count);
					continue;
				}
				if(tAbsf(pre.r - post.r) < 0.02f && tAbsf(pre.g - post.g) < 0.02f && tAbsf(pre.b - post.b) < 0.02f)
					continue;
				
				for(int x = sub_x * 8 + 1;x < sub_x * 8 + tMin(size,8) + 1;x++){
					for(int y = sub_y * 8 + 1;y < sub_y * 8 + tMin(size,8) + 1;y++){
						vec3 pos = {node.pos.x,node.pos.y,node.pos.z};
						uint location = x * (size + 2) + y;
						vec3 pre = lightmap[location];
						setLightSideBigSingle(&lightmap[location],pos,side,depth,x - 1,y - 1,LM_QUALITY);
						vec3mulvec3(&lightmap[location],material.luminance);
						if(!lightmap[location].r && !lightmap[location].g && !lightmap[location].b)
							lightmap[location] = pre;
					}
				}
				setEdgeLightSide(node,side);
			}
		}
	}
}

static bool r_click;
static bool l_click;

void lighting(){
	static uint i;
	if(i % 32 == 0)
		updateLightMap(0,camera.pos,0,32.0f);
	else if(i % 8 == 0)
		updateLightMap(0,camera.pos,0,16.0f);
	else
		updateLightMap(0,camera.pos,0,8.0f);
	while(light_new_stack_ptr){
		light_new_stack_t stack = light_new_stack[--light_new_stack_ptr];
		if(!stack.node)
			continue;
		uint size = GETLIGHTMAPSIZE(stack.node->depth);
		block_t* block = stack.node->block;
		if(!block)
			continue;
		uint side = stack.side;
		ivec3 pos_i = stack.node->pos;
		vec3 pos = {pos_i.x,pos_i.y,pos_i.z};
		vec3* lightmap = block->side[side].luminance_p;
		if(!lightmap)
			continue;
		for(int x = 1;x < size + 1;x+=2){
			for(int y = 1;y < size + 1;y+=2){
				setLightSideBigSingle(&lightmap[x * (size + 2) + y],pos,side,stack.node->depth,x - 1,y - 1,LM_SIZE);
				vec3mulvec3(&lightmap[x * (size + 2) + y],material_array[block->material].luminance);
				if(size == 1)
					continue;
				lightmap[x * (size + 2) + y + 1] = lightmap[x * (size + 2) + y];
				lightmap[x * (size + 2) + y + (size + 2)] = lightmap[x * (size + 2) + y];
				lightmap[x * (size + 2) + y + (size + 2) + 1] = lightmap[x * (size + 2) + y];
			}
		}
		setEdgeLightSide(*stack.node,side);
	}
	i++;
	lighting_thread_done = true;
}

void fillHoldStructure(hold_structure_t* hold,uint node_ptr){
	block_node_t node = node_root[node_ptr];
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

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_KEYDOWN:
		switch(wParam){
		case VK_ESCAPE:
			menu_select ^= 1;
			ShowCursor(menu_select);
			if(menu_select)
				break;
			SetCursorPos(WND_SIZE.x / 2,WND_SIZE.y / 2);
			break;
		case VK_F5:
			calculateNodeLuminanceTree(0,32);
			break;
		case VK_F8:
			test_bool = 0;
			break;
		case VK_F9:
			test_bool = 1;
			break;
		case VK_UP:
			tool_select ^= true;
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
		node_hit_t result = traverseTree(ray3Create(init.pos,dir),init.node);
		if(!result.node)
			break;
		if(!(GetKeyState(VK_CONTROL) & 0x80)){
			block_type = node_root[result.node].block->material;
			break;
		}
		block_node_t node = node_root[result.node];
		float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
		vec3 block_pos = vec3mulR((vec3){node.pos.x,node.pos.y,node.pos.z},block_size);
		plane_t plane = getPlane(block_pos,dir,result.side,block_size);
		vec3 plane_pos = vec3subvec3R(plane.pos,camera_rd.pos);
		int side = result.side * 2 + (dir.a[result.side] < 0.0f);
		float distance = rayIntersectPlane(plane_pos,dir,plane.normal);
		vec3 pos = vec3addvec3R(camera_rd.pos,vec3mulR(dir,distance - 0.01f));
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

vec3 getReflectLuminance(vec3 pos,vec3 dir){
	traverse_init_t init = initTraverse(pos);
	node_hit_t reflect = traverseTree(ray3Create(init.pos,dir),init.node);
	block_node_t* node = node_root;

	if(!reflect.node){
		int side;
		vec2 uv = sampleCube(dir,&side);
		if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f)
			return LUMINANCE_SUN;
		return LUMINANCE_SKY;
	}

	block_t* block = node[reflect.node].block;
	uint block_depth = node[reflect.node].depth;
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT)
		return material.luminance;
	vec3 block_pos = {node[reflect.node].pos.x,node[reflect.node].pos.y,node[reflect.node].pos.z};
	float block_size = (float)MAP_SIZE / (1 << block_depth) * 2.0f;
	block_pos.x *= block_size;
	block_pos.y *= block_size;
	block_pos.z *= block_size;
	plane_t plane_r = getPlane(block_pos,dir,reflect.side,block_size);
	vec3 plane_pos_r = vec3subvec3R(plane_r.pos,pos);
	float dst = rayIntersectPlane(plane_pos_r,dir,plane_r.normal);
	vec3addvec3(&pos,vec3mulR(dir,dst));
	vec2 uv_r = {
		(pos.a[plane_r.x] - plane_r.pos.a[plane_r.x]) / block_size,
		(pos.a[plane_r.y] - plane_r.pos.a[plane_r.y]) / block_size
	};
	uv_r.x = tMaxf(tMinf(uv_r.x,block_size),0.0f);
	uv_r.y = tMaxf(tMinf(uv_r.y,block_size),0.0f);
	int side_r = reflect.side * 2 + (dir.a[reflect.side] < 0.0f);
				
	vec3 luminance_r = getLuminance(block->side[side_r],uv_r,block_depth);
	if(material.texture.data){
		vec2 texture_uv = {
			pos.a[plane_r.x] / block_size,
			pos.a[plane_r.y] / block_size
		};
		texture_uv.x = tMaxf(texture_uv.x,0.0f);
		texture_uv.y = tMaxf(texture_uv.y,0.0f);
		float scale = TEXTURE_SCALE / material.texture.size / (8 << block_depth);
		float texture_x = texture_uv.x * scale;
		float texture_y = texture_uv.y * scale;
		uint x = tFractUnsigned(texture_x) * material.texture.size;
		uint y = tFractUnsigned(texture_y) * material.texture.size;
		uint location = x * material.texture.size + y;
		pixel_t text = material.texture.data[location];
		vec3mulvec3(&luminance_r,(vec3){text.r * 0.00392f,text.b * 0.00392f,text.g * 0.00392f});
	}
	return luminance_r;
}

vec3 getLuminance(block_side_t side,vec2 uv,uint depth){
	if(!side.luminance_p)
		return VEC3_ZERO;
	uint lm_size = GETLIGHTMAPSIZE(depth);

	vec2 fract_pos = {
		tFractUnsigned(uv.x * lm_size + 0.5f),
		tFractUnsigned(uv.y * lm_size + 0.5f)
	};

	uint lm_offset_x = uv.x * lm_size + 0.5f; 
	uint lm_offset_y = uv.y * lm_size + 0.5f;

	uint location = lm_offset_x * (lm_size + 2) + lm_offset_y;

	vec3 lm_1 = side.luminance_p[location];
	vec3 lm_2 = side.luminance_p[location + 1];
	vec3 lm_3 = side.luminance_p[location + (lm_size + 2)];
	vec3 lm_4 = side.luminance_p[location + (lm_size + 2) + 1];

	vec3 mix_1 = vec3mixR(lm_1,lm_3,fract_pos.x);
	vec3 mix_2 = vec3mixR(lm_2,lm_4,fract_pos.x);

	return vec3mixR(mix_1,mix_2,fract_pos.y);
}

#define BEAM_SIZE (1 << 4)

volatile uint draw_pool;

void draw_1(int p){
	vec3 camera_pos = camera.pos;
	traverse_init_t init_test = initTraverse(camera_pos);
	while(draw_pool < 16){
		uint piece = draw_pool++;
		uint x = piece / 4;
		uint y = piece % 4;
		uint piece_size_x = WND_RESOLUTION.x / 4;
		uint piece_size_y = WND_RESOLUTION.y / 4;
		for(int d_x = x * piece_size_x;d_x < x * piece_size_x + piece_size_x;d_x += BEAM_SIZE){
			for(int d_y = y * piece_size_y;d_y < y * piece_size_y + piece_size_y;d_y += BEAM_SIZE){
				beamTrace16(init_test,camera_pos,d_x,d_y,p);
			}
		}
	}
	return;
}

void drawBresenhamLine(int x1,int y1,int x2,int y2,pixel_t color){
    int dx = tAbs(x2 - x1);
    int dy = tAbs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    for(;;){
        // Plot the current point (x1, y1)
		vram.draw[x1 * (int)WND_RESOLUTION.y + y1] = color;

        // If the current point is the end point, exit the loop
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
	/*
	vec2 dir = vec2normalizeR(vec2subvec2R(pos_2,pos_1));
	ray2_t ray = ray2Create(pos_1,dir);
	while(ray.square_pos.x != (int)pos_2.x && ray.square_pos.y != (int)pos_2.y){
		vram.draw[ray.square_pos.x * (int)WND_RESOLUTION.y + ray.square_pos.y] = color;
		ray2Itterate(&ray);
		bool bound_x = ray.square_pos.x < 0 || ray.square_pos.x >= WND_RESOLUTION.x;
		bool bound_y = ray.square_pos.y < 0 || ray.square_pos.y >= WND_RESOLUTION.y;
		if(bound_x || bound_y)
			break;
	}
	*/
}

void generateBlockOutline(){
	vec3 look_direction = getLookAngle(camera.dir);
	traverse_init_t init = initTraverse(camera_rd.pos);
	node_hit_t result = traverseTree(ray3Create(init.pos,look_direction),init.node);
	if(!result.node)
		return;
	block_node_t node = node_root[result.node];
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

void castVisibilityRays(){
	traverse_init_t init = initTraverse(camera_rd.pos);
	static uint counter = 0;
	for(int x = counter / 64;x < WND_RESOLUTION.x;x += 64){
		for(int y = counter % 64;y < WND_RESOLUTION.y;y += 64){
			vec3 ray_angle = getRayAngle(x,y);
			node_hit_t hit = traverseTree(ray3Create(init.pos,ray_angle),init.node);
			block_node_t* node = &node_root[hit.node];
			uint side = hit.side * 2 + (ray_angle.a[hit.side] < 0.0f);
			if(!node->block)
				continue;
			if(!node->block->side[side].luminance_p)
				lightNewStackPut(node,node->block,side);
		}
	}
	counter += TRND1 * 500.0f;
	counter %= 64 * 64;
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
				glBegin(GL_TRIANGLES);
				glVertex2f((float)j / WND_RESOLUTION.y * RD_MASK_SIZE * 2.0f - 1.0f,(float)i / WND_RESOLUTION.x * RD_MASK_SIZE * 2.0f - 1.0f);
				glVertex2f((float)j / WND_RESOLUTION.y * RD_MASK_SIZE * 2.0f - 1.0f,(float)i / WND_RESOLUTION.x * RD_MASK_SIZE * 2.0f - 1.0f + 2.0f / (WND_RESOLUTION.x / RD_MASK_SIZE));
				glVertex2f((float)j / WND_RESOLUTION.y * RD_MASK_SIZE * 2.0f - 1.0f + 2.0f / (WND_RESOLUTION.x / RD_MASK_SIZE),(float)i / WND_RESOLUTION.x * RD_MASK_SIZE * 2.0f - 1.0f + 2.0f / (WND_RESOLUTION.x / RD_MASK_SIZE));
				glEnd();
			}
		}
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
	traverse_init_t init = initTraverse(camera_rd.pos);
	for(int i = 0;i < BLOCK_SELECT_LIGHT_QUALITY;i++){
		for(int j = 0;j < BLOCK_SELECT_LIGHT_QUALITY;j++){
			vec3 angle = getLookAngle(direction);
			float weight = tAbsf(vec3dotR(angle,normal));
			vec3 luminance = rayGetLuminance(init.pos,direction,init.node);
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

void drawBlockSelect(vec3 block_pos,float block_size,uint material_id){
	material_t material = material_array[material_id];
	vec2 x_direction = vec2addvec2R((vec2){camera_rd.dir.x + 0.1f,0.1f},(vec2){M_PI * 0.5f,0.0f});
	vec2 y_direction = vec2addvec2R((vec2){camera_rd.dir.x + 0.1f,camera_rd.dir.y + 0.1f},(vec2){0.0f,M_PI * 0.5f});
	vec2 z_direction = vec2addvec2R((vec2){camera_rd.dir.x + 0.1f,camera_rd.dir.y + 0.1f},(vec2){0.0f,0.0f});
	vec3 x_luminance;
	vec3 y_luminance;
	vec3 z_luminance;
	if(material.flags & MAT_LUMINANT){
		x_luminance = (vec3){material.luminance.b,material.luminance.g,material.luminance.r};
		y_luminance = (vec3){material.luminance.b,material.luminance.g,material.luminance.r};
		z_luminance = (vec3){material.luminance.b,material.luminance.g,material.luminance.r};
	}
	else{
		x_luminance = getBlockSelectLight((vec2){camera_rd.dir.x + M_PI,camera_rd.dir.y});
		y_luminance = getBlockSelectLight((vec2){camera_rd.dir.x - M_PI * 0.5f,camera_rd.dir.y});
		z_luminance = getBlockSelectLight((vec2){camera_rd.dir.x,camera_rd.dir.y + M_PI * 0.5f});
	}
	vec3 y_angle = getLookAngle(x_direction);
	vec3 x_angle = getLookAngle(y_direction);
	vec3 z_angle = getLookAngle(z_direction);
	
	vec3 pos = camera_rd.pos;
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
		vec3 screen = pointToScreenZ(verticle_pos);
		verticle_screen[i].y = screen.x / WND_RESOLUTION.x * 2.0f - 1.0f;
		verticle_screen[i].x = screen.y / WND_RESOLUTION.y * 2.0f - 1.0f;
	}

	float texture_offset = (1.0f / TEXTURE_AMMOUNT) * material.texture_id;
	float texture_size = (1.0f / TEXTURE_AMMOUNT);

	triangles[triangle_count + 0 ] = (triangles_t){verticle_screen[0].x,verticle_screen[0].y,texture_offset,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 1 ] = (triangles_t){verticle_screen[1].x,verticle_screen[1].y,texture_offset,1.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 2 ] = (triangles_t){verticle_screen[3].x,verticle_screen[3].y,texture_offset + texture_size,1.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 3 ] = (triangles_t){verticle_screen[0].x,verticle_screen[0].y,texture_offset,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 4 ] = (triangles_t){verticle_screen[2].x,verticle_screen[2].y,texture_offset + texture_size,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 5 ] = (triangles_t){verticle_screen[3].x,verticle_screen[3].y,texture_offset + texture_size,1.0f,1.0f,1.0f,1.0f,1.0f};

	triangles[triangle_count + 6 ] = (triangles_t){verticle_screen[0].x,verticle_screen[0].y,texture_offset,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 7 ] = (triangles_t){verticle_screen[1].x,verticle_screen[1].y,texture_offset,1.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 8 ] = (triangles_t){verticle_screen[5].x,verticle_screen[5].y,texture_offset + texture_size,1.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 9 ] = (triangles_t){verticle_screen[0].x,verticle_screen[0].y,texture_offset,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 10] = (triangles_t){verticle_screen[4].x,verticle_screen[4].y,texture_offset + texture_size,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 11] = (triangles_t){verticle_screen[5].x,verticle_screen[5].y,texture_offset + texture_size,1.0f,1.0f,1.0f,1.0f,1.0f};

	triangles[triangle_count + 12] = (triangles_t){verticle_screen[1].x,verticle_screen[1].y,texture_offset,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 13] = (triangles_t){verticle_screen[3].x,verticle_screen[3].y,texture_offset,1.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 14] = (triangles_t){verticle_screen[7].x,verticle_screen[7].y,texture_offset + texture_size,1.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 15] = (triangles_t){verticle_screen[1].x,verticle_screen[1].y,texture_offset,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 16] = (triangles_t){verticle_screen[5].x,verticle_screen[5].y,texture_offset + texture_size,0.0f,1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count + 17] = (triangles_t){verticle_screen[7].x,verticle_screen[7].y,texture_offset + texture_size,1.0f,1.0f,1.0f,1.0f,1.0f};
	
	triangles[triangle_count + 0 ].lighting = x_luminance;
	triangles[triangle_count + 1 ].lighting = x_luminance;
	triangles[triangle_count + 2 ].lighting = x_luminance;
	triangles[triangle_count + 3 ].lighting = x_luminance;
	triangles[triangle_count + 4 ].lighting = x_luminance;
	triangles[triangle_count + 5 ].lighting = x_luminance;

	triangles[triangle_count + 6 ].lighting = y_luminance;
	triangles[triangle_count + 7 ].lighting = y_luminance;
	triangles[triangle_count + 8 ].lighting = y_luminance;
	triangles[triangle_count + 9 ].lighting = y_luminance;
	triangles[triangle_count + 10].lighting = y_luminance;
	triangles[triangle_count + 11].lighting = y_luminance;

	triangles[triangle_count + 12].lighting = z_luminance;
	triangles[triangle_count + 13].lighting = z_luminance;
	triangles[triangle_count + 14].lighting = z_luminance;
	triangles[triangle_count + 15].lighting = z_luminance;
	triangles[triangle_count + 16].lighting = z_luminance;
	triangles[triangle_count + 17].lighting = z_luminance;

	triangle_count += 18;
}

void genHoldStructure(hold_structure_t* hold,vec3 block_pos,float block_size){
	if(hold->is_filled){
		drawBlockSelect(block_pos,block_size,hold->block_id);
		return;
	}
	block_size *= 0.5f;
	for(int i = 0;i < 8;i++){
		if(!hold->child_s[i])
			continue;
		float dz = i & 1 ? 0.0f : block_size;
		float dy = i & 2 ? 0.0f : block_size;
		float dx = i & 4 ? 0.0f : block_size;
		genHoldStructure(hold->child_s[i],(vec3){block_pos.x + dx,block_pos.y + dy,block_pos.z + dz},block_size);
	}
}

void genBlockSelect(){
	if(tool_select){
		genHoldStructure(&hold_structure,(vec3){0.0f,0.0f,0.0f},1.0f);
		return;
	}
	drawBlockSelect(VEC3_ZERO,1.0f,block_type);
}

void draw(){
	for(;;){
		//printf("%f:%f:%f\n",camera.pos.x,camera.pos.y,camera.pos.z);
		is_rendering = true;
		unsigned long long t = __rdtsc();
		camera_rd = camera;
		camera_rd.pre[0] = camera_rd.tri.x * camera_rd.tri.z;
		camera_rd.pre[1] = camera_rd.tri.y * camera_rd.tri.z;
		camera_rd.pre[2] = camera_rd.tri.x * camera_rd.tri.w;
		camera_rd.pre[3] = camera_rd.tri.y * camera_rd.tri.w;
		camera_rd.pre[4] = camera_rd.tri.w + camera_rd.tri.z;

		if(1){
			unsigned long long time = __rdtsc();
			if(!context_is_gl){
				memset(vram.draw,0,sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
				memset(vram.render,0,sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
				initGL(context);
			}
			if(lighting_thread_done){
				lighting_thread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)lighting,0,0,0);
				lighting_thread_done = false;
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
			
			if(r_click){
				r_click = false;
				playerRemoveBlock();
			}
			if(l_click){
				l_click = false;
				playerAddBlock();
			}
			continue;
		}
		else{
			if(context_is_gl){
				context_is_gl = false;
				wglDeleteContext(context);
			}
		if(DRAW_MULTITHREAD){
			HANDLE draw_threads[4];
			draw_threads[0] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)0,0,0);
			draw_threads[1] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)1,0,0);
			draw_threads[2] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)2,0,0);
			draw_threads[3] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)3,0,0);
			WaitForMultipleObjects(4,draw_threads,TRUE,INFINITE);
		}
		else{
			vec3 camera_pos = camera.pos;
			traverse_init_t init_test = initTraverse(camera_pos);
			for(int x = 0;x < WND_RESOLUTION.x;x += BEAM_SIZE){
				for(int y = 0;y < WND_RESOLUTION.y;y += BEAM_SIZE){
					beamTrace16(init_test,camera_pos,x,y,0);
				}
			}
		}
		}
		draw_pool = 0;
		
		float f = tMin(__rdtsc() - t >> 18,WND_RESOLUTION.x - 1);
		static float avg;
		avg = avg * 0.95f + f * 0.05f;
		for(int i = 0;i < avg;i++){
			for(int j = 10;j < 20;j++){
				vram.draw[i * (int)WND_RESOLUTION.y + j] = (pixel_t){0,170,0};
			}
		}

		if(r_click){
			r_click = false;
			playerRemoveBlock();
		}

		if(l_click){
			l_click = false;
			playerAddBlock();
		}

		vec3 look_direction = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(camera_rd.pos);
		node_hit_t result = traverseTree(ray3Create(init.pos,look_direction),init.node);
		if(result.node){
			block_node_t node = node_root[result.node];
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
			for(int i = 0;i < 8;i++)
				screen_point[i] = pointToScreen(point[i]);
			pixel_t green = {0,255,0};
			drawLine(screen_point[0],screen_point[1],green);
			drawLine(screen_point[0],screen_point[2],green);
			drawLine(screen_point[0],screen_point[4],green);
			drawLine(screen_point[1],screen_point[3],green);
			drawLine(screen_point[1],screen_point[5],green);
			drawLine(screen_point[2],screen_point[3],green);
			drawLine(screen_point[2],screen_point[6],green);
			drawLine(screen_point[3],screen_point[7],green);
			drawLine(screen_point[4],screen_point[5],green);
			drawLine(screen_point[4],screen_point[6],green);
			drawLine(screen_point[5],screen_point[7],green);
			drawLine(screen_point[6],screen_point[7],green);
		}

		if(menu_select){
			for(int x = WND_RESOLUTION.x / 2 - 200 / RES_SCALE;x < WND_RESOLUTION.x / 2 + 200 / RES_SCALE;x++){
				for(int y = WND_RESOLUTION.y / 2 - 400 / RES_SCALE;y < WND_RESOLUTION.y / 2 + 400 / RES_SCALE;y++){
					pixel_t pixel = vram.draw[x * (int)WND_RESOLUTION.y + y];
					pixel.r = pixel.r + 80 >> 1;
					pixel.g = pixel.g + 80 >> 1;
					pixel.b = pixel.b + 80 >> 1;
					vram.draw[x * (int)WND_RESOLUTION.y + y] = pixel;
				}
			}
			drawButton();
		}

		if(!tool_select){
			if(material_array[block_type].texture.data){
				int offset_x = 20 / RES_SCALE;
				int offset_y = WND_RESOLUTION.y - 140 / RES_SCALE;
				int size = 120 / RES_SCALE;
				texture_t texture = material_array[block_type].texture;
				for(int i = offset_x;i < offset_x + size;i++){
					for(int j = offset_y;j < offset_y + size;j++){
						int tx = (i - offset_x) * (texture.size) / size;
						int ty = (j - offset_y) * (texture.size) / size;
						vram.draw[i * (int)WND_RESOLUTION.y + j] = texture.data[tx * texture.size + ty];
					}
				}
			}
		}

		uint brightness = 0;
		for(int i = 0;i < WND_RESOLUTION.x;i += WND_RESOLUTION.x / 32){
			for(int j = 0;j < WND_RESOLUTION.y;j += WND_RESOLUTION.y / 32){
				pixel_t pixel = vram.draw[i * (int)WND_RESOLUTION.y + j];
				brightness += tMax(tMax(pixel.r,pixel.g),pixel.b);
			}
		}
		float brightness_f = brightness / (float)(32 * 32 * 127);
		camera_exposure += (1.0f - brightness_f) * 0.01f;
		camera_exposure = camera_exposure * 0.99f + 0.01f;
		//camera_exposure = camera_exposure * 0.99f + 0.01f;

		is_rendering = false;
		//printf("%i\n",__rdtsc() - t);
		int middle_pixel = WND_RESOLUTION.x * WND_RESOLUTION.y / 2 + WND_RESOLUTION.y / 2;
		vram.draw[middle_pixel] = (pixel_t){0,255,0};
		vram.draw[middle_pixel + 1] = (pixel_t){0,0,0};
		vram.draw[middle_pixel - 1] = (pixel_t){0,0,0};
		vram.draw[middle_pixel + (int)WND_RESOLUTION.y] = (pixel_t){0,0,0};
		vram.draw[middle_pixel - (int)WND_RESOLUTION.y] = (pixel_t){0,0,0};
		pixel_t* temp = vram.render;
		while(frame_ready)
			Sleep(1);
		vram.render = vram.draw;
		vram.draw = temp;
		frame_ready = TRUE;
	}	
}

void entity(){
	return;
	/*
	for(;;){
		for(uint i = 0;i < block_node_test_c + 1;i++){
			if(!node_root[i].block)
				continue;
			material_t material = material_array[node_root[i].block->material];
			ivec3 pos = node_root[i].block->pos;
			int depth = node_root[i].block->depth;
			if(node_root[i].block->material){
				if(TRND1 > 0.01f || getNode(pos.x,pos.y,pos.z + 1,depth))
					continue;
				setVoxel(pos.x,pos.y,pos.z + 1,node_root[i].block->depth,node_root[i].block->material);
				removeVoxel(i);
			} 
		}
		Sleep(16);
	}
	*/
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

	node_root = MALLOC(sizeof(block_node_t) * 1024 * 1024);
	triangles = MALLOC(sizeof(triangles) * 10000000);
	texture_atlas = MALLOC(sizeof(pixel_t) * 1024 * 1024 * TEXTURE_AMMOUNT);
	node_root[0].parent = 0xffffffff;

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

	for(int i = 0;i < TEXTURE_AMMOUNT;i++){
		for(int x = 0;x < 1024;x++){
			for(int y = 0;y < 1024;y++){
				uint texture_pos = x * texture[i].size / 1024 * texture[i].size + y * texture[i].size / 1024;
				pixel_t color = texture[i].data[texture_pos];
				texture_atlas[x * 1024 * TEXTURE_AMMOUNT + i * 1024 + y] = (pixel_t){color.b,color.g,color.r};
			}
		}
	}

	material_array[0].texture = texture[0];
	material_array[1].texture = texture[1];
	material_array[2].texture = texture[2];
	material_array[3].texture = texture[3];

	int depth = 7;
	int m = 1 << depth;
	
	
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
				setVoxel(x,y,z,depth,1);	
			}
		}
	}
	
	calculateNodeLuminanceTree(0,16);
	
	vram.draw = MALLOC(sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
	vram.render = MALLOC(sizeof(pixel_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);

	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE | WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(FALSE);

	physics_thread  = CreateThread(0,0,(LPTHREAD_START_ROUTINE)physics,0,0,0);	
	thread_render   = CreateThread(0,0,(LPTHREAD_START_ROUTINE)render,0,0,0);
	lighting_thread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)lighting,0,0,0);
	entity_thread   = CreateThread(0,0,(LPTHREAD_START_ROUTINE)entity,0,0,0);

	CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw,0,0,0);

	SetThreadPriority(lighting_thread,THREAD_PRIORITY_BELOW_NORMAL);
	SetThreadPriority(physics_thread,THREAD_PRIORITY_ABOVE_NORMAL);

	MSG msg;
	while(GetMessageA(&msg,window,0,0)){
		TranslateMessage(&msg);
 		DispatchMessageA(&msg); 
	}
}