#include <Windows.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include "tmath.h"
#include "vec2.h"
#include "vec3.h"
#include "dda.h"
#include "source.h"
#include "tree.h"
#include "memory.h"
#include "physics.h"
#include "lighting.h"
#include "draw.h"

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = (WNDPROC)proc,.lpszClassName = "class",.lpszMenuName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,32,BI_RGB};
HWND window;
HDC context;
vram_t vram;
camera_t camera = {.pos.z = 30.0f,.pos.x = 3.0f,.pos.y = 3.0f};

int edit_depth = 5;

volatile bool is_rendering;

int test_bool;

int global_tick;

vec3 color_pallet[6] = {
	{0.9f,0.9f,0.9f},
	{0.9f,0.3f,0.3f},
	{0.3f,0.9f,0.3f},
	{0.3f,0.3f,0.9f},
	{0.9f,0.9f,0.3f},
	{0.9f,0.3f,0.9f}
};

volatile char frame_ready;

pixel_t* texture[16];

material_t material_array[] = {
	{0},
	{0},
	{0},
	{.flags = MAT_REFLECT},
	{.flags = MAT_REFLECT},
	{.flags = MAT_LUMINANT,.luminance = {2.0f,2.0f,2.0f}},
	{.flags = MAT_LUMINANT,.luminance = {2.0f,0.5f,0.5f}},
	{.flags = MAT_LUMINANT,.luminance = {0.5f,2.0f,0.5f}},
	{.flags = MAT_LUMINANT,.luminance = {0.5f,0.5f,2.0f}}
};

pixel_t* loadBMP(char* name){
	HANDLE h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	int fsize = GetFileSize(h,0);
	int offset;
	SetFilePointer(h,0x0a,0,0);
	ReadFile(h,&offset,4,0,0);
	SetFilePointer(h,offset,0,0);
	int size = sqrtf((fsize-offset) / 3);
	pixel_t* text = MALLOC(sizeof(pixel_t) * size * size);
	text[0].size = size;
	char* temp = MALLOC(fsize + 5);
	ReadFile(h,temp,fsize-offset,0,0);
	for(int i = 0;i < text[0].size * text[0].size;i++){
		text[i + 1].r = temp[i * 3 + 0];
		text[i + 1].g = temp[i * 3 + 1];
		text[i + 1].b = temp[i * 3 + 2];
	}	
	MFREE(temp);
	
	CloseHandle(h);
	return text;
}

void render(){
	for(;;){
		StretchDIBits(context,0,0,WND_SIZE.x,WND_SIZE.y,0,0,WND_RESOLUTION.y,WND_RESOLUTION.x,vram.render,&bmi,0,SRCCOPY);
		while(!frame_ready)
			Sleep(1);
		frame_ready = FALSE;
	}
}

vec2 sampleCube(vec3 v,int* faceIndex){
	vec3 vAbs = vec3absR(v);
	float ma;
	vec2 uv;
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y){
		*faceIndex = (v.z < 0.0f) + 4;
		ma = 0.5f / vAbs.z;
		uv = (vec2){v.z < 0.0f ? -v.x : v.x, -v.y};
	}
	else if(vAbs.y >= vAbs.x){
		*faceIndex = (v.y < 0.0f) + 2;
		ma = 0.5f / vAbs.y;
		uv = (vec2){v.x, v.y < 0.0f ? -v.z : v.z};
	}
	else{
		*faceIndex = v.x < 0.0f;
		ma = 0.5f / vAbs.x;
		uv = (vec2){v.x < 0.0f ? v.z : -v.z, -v.y};
	}
	return (vec2){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
}

vec3 getLookAngle(vec2 angle){
	vec3 ang;
	ang.x = cosf(angle.x) * cosf(angle.y);
	ang.y = sinf(angle.x) * cosf(angle.y);
	ang.z = sinf(angle.y);
	return ang;
}

vec3 getRayAngle(int x,int y){
	vec3 ray_ang;
	float pxY = (((float)(x + 0.5f) * 2.0f / WND_RESOLUTION.x) - 1.0f);
	float pxX = (((float)(y + 0.5f) * 2.0f / WND_RESOLUTION.y) - 1.0f);
	float pixelOffsetY = pxY / (FOV.x / WND_RESOLUTION.x * 2.0f);
	float pixelOffsetX = pxX / (FOV.y / WND_RESOLUTION.y * 2.0f);
	ray_ang.x = (camera.tri.x * camera.tri.z - camera.tri.x * camera.tri.w * pixelOffsetY) - camera.tri.y * pixelOffsetX;
	ray_ang.y = (camera.tri.y * camera.tri.z - camera.tri.y * camera.tri.w * pixelOffsetY) + camera.tri.x * pixelOffsetX;
	ray_ang.z = camera.tri.w + camera.tri.z * pixelOffsetY;
	return ray_ang;
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

plane_t getPlane(vec3 pos,vec3 dir,int side,float block_size){
	switch(side){
	case VEC3_X: return (plane_t){{pos.x + (dir.x < 0.0f) * block_size,pos.y,pos.z},{1.0f,0.0f,0.0f},VEC3_Y,VEC3_Z};
	case VEC3_Y: return (plane_t){{pos.x,pos.y + (dir.y < 0.0f) * block_size,pos.z},{0.0f,1.0f,0.0f},VEC3_X,VEC3_Z};
	case VEC3_Z: return (plane_t){{pos.x,pos.y,pos.z + (dir.z < 0.0f) * block_size},{0.0f,0.0f,1.0f},VEC3_X,VEC3_Y};
	}
}

float rayIntersectPlane(vec3 pos,vec3 dir,vec3 plane){
	return vec3dotR(pos,plane) / vec3dotR(dir,plane);
}

int block_type;
portal_t portal_place_buffer;

void addSubVoxel(int vx,int vy,int vz,int x,int y,int z,int depth){
	if(--depth == -1)
		return;
	for(int lx = 0;lx < 2;lx++){
		for(int ly = 0;ly < 2;ly++){
			for(int lz = 0;lz < 2;lz++){
				bool offset_x = x >> depth & 1;
				bool offset_y = y >> depth & 1;
				bool offset_z = z >> depth & 1;
				if(offset_x == lx && offset_y == ly && offset_z == lz){
					addSubVoxel(vx + offset_x << 1,vy + offset_y << 1,vz + offset_z << 1,x,y,z,depth);
					continue;
				}
				setVoxelLM(vx + lx,vy + ly,vz + lz,edit_depth - depth,block_type);
			}
		}
	}
}

int tool_select;

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_KEYDOWN:
		switch(wParam){
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
			tool_select--;
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
	case WM_RBUTTONDOWN:{
		vec3 dir = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(vec3divR(camera.pos,MAP_SIZE));
		nodepointer_hit_t result = traverseTreeNode(ray3Create(init.pos,dir),init.node);
		if(!result.node)
			break;
		block_t* block = (*result.node)->block;
		removeVoxel(result.node);
		if(block->depth < edit_depth){
			ivec3 block_pos_i = block->pos;
			vec3 block_pos;
			block_pos = (vec3){block_pos_i.x << (edit_depth - block->depth),block_pos_i.y << (edit_depth - block->depth),block_pos_i.z << (edit_depth - block->depth)};
			vec3 pos;
			float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
			pos.x = block->pos.x * block_size;
			pos.y = block->pos.y * block_size;
			pos.z = block->pos.z * block_size;
			plane_t plane = getPlane(pos,dir,result.side,block_size);
			vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
			float dst = rayIntersectPlane(plane_pos,dir,plane.normal);
			vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(dir,dst));
			vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
			uv.x *= 1 << edit_depth - block->depth;
			uv.y *= 1 << edit_depth - block->depth;
			block_pos.a[result.side] += ((dir.a[result.side] < 0.0f) << (edit_depth - block->depth)) - (dir.a[result.side] < 0.0f);
			block_pos.a[plane.x] += tFloorf(uv.x);
			block_pos.a[plane.y] += tFloorf(uv.y);
			int block_count = 1 << edit_depth - block->depth;
			addSubVoxel(block_pos_i.x << 1,block_pos_i.y << 1,block_pos_i.z << 1,(int)block_pos.x & ((1 << edit_depth - block->depth) - 1),(int)block_pos.y & ((1 << edit_depth - block->depth) - 1),(int)block_pos.z & ((1 << edit_depth - block->depth) - 1),edit_depth - block->depth);
		}
		break;}
	case WM_LBUTTONDOWN:;
		vec3 dir = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(vec3divR(camera.pos,MAP_SIZE));
		blockray_t result = traverseTree(ray3Create(init.pos,dir),init.node);
		if(!result.node)
			break;
		block_t* block = result.node->block;
		if(tool_select){
			int side = result.side * 2 + (dir.a[result.side] < 0.0f);
			if(!portal_place_buffer.block || portal_place_buffer.block->depth != block->depth){
				portal_place_buffer.block = block;
				portal_place_buffer.side = side;
				break;
			}
			block->side[side].portal = portal_place_buffer;
			portal_place_buffer.block->side[portal_place_buffer.side].portal.block = block;
			portal_place_buffer.block->side[portal_place_buffer.side].portal.side = side;
			portal_place_buffer.block = 0;
			break;
		}
		ivec3 block_pos_i = block->pos;
		block_pos_i.a[result.side] += (dir.a[result.side] < 0.0f) * 2 - 1;
		vec3 block_pos;
		if(block->depth > edit_depth){
			int shift = block->depth - edit_depth;
			block_pos = (vec3){block_pos_i.x >> shift,block_pos_i.y >> shift,block_pos_i.z >> shift};
		}
		else{
			int shift = edit_depth - block->depth;
			block_pos = (vec3){block_pos_i.x << shift,block_pos_i.y << shift,block_pos_i.z << shift};
		}
		if(block->depth < edit_depth){
			vec3 pos;
			float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
			pos.x = block->pos.x * block_size;
			pos.y = block->pos.y * block_size;
			pos.z = block->pos.z * block_size;
			plane_t plane = getPlane(pos,dir,result.side,block_size);
			vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
			float dst = rayIntersectPlane(plane_pos,dir,plane.normal);
			vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(dir,dst));
			vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
			uv.x *= 1 << edit_depth - block->depth;
			uv.y *= 1 << edit_depth - block->depth;
			block_pos.a[result.side] += ((dir.a[result.side] > 0.0f) << (edit_depth - block->depth)) - (dir.a[result.side] > 0.0f);
			block_pos.a[plane.x] += tFloorf(uv.x);
			block_pos.a[plane.y] += tFloorf(uv.y);
		}
		setVoxelLM(block_pos.x,block_pos.y,block_pos.z,edit_depth,block_type);
		break;
	case WM_MOUSEMOVE:;
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
		break;
	case WM_DESTROY: case WM_CLOSE:
		ExitProcess(0);
	}
	return DefWindowProcA(hwnd,msg,wParam,lParam);
}

vec3 getReflectLuminance(vec3 pos,vec3 dir){
	traverse_init_t init = initTraverse(vec3divR(pos,MAP_SIZE));
	blockray_t reflect = traverseTree(ray3Create(init.pos,dir),init.node);
	
	if(!reflect.node){
		int side;
		vec2 uv = sampleCube(dir,&side);
		if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f){
			return (vec3){1.0f,2.0f,4.0f};
		}
		else
			return (vec3){0.8f,0.4f,0.2f};
	}
	block_t* block = reflect.node->block;
	material_t material = material_array[block->material];
	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
	block_pos.x = block->pos.x * block_size;
	block_pos.y = block->pos.y * block_size;
	block_pos.z = block->pos.z * block_size;
	plane_t plane_r = getPlane(block_pos,dir,reflect.side,block_size);
	vec3 plane_pos_r = vec3subvec3R(plane_r.pos,pos);
	float dst = rayIntersectPlane(plane_pos_r,dir,plane_r.normal);
	vec3addvec3(&pos,vec3mulR(dir,dst));
	vec2 uv_r = {(pos.a[plane_r.x] - plane_r.pos.a[plane_r.x]) / block_size,(pos.a[plane_r.y] - plane_r.pos.a[plane_r.y]) / block_size};
	int side_r = reflect.side * 2 + (dir.a[reflect.side] < 0.0f);
				
	vec3 luminance_r = getLuminance(block->side[side_r],uv_r,block->depth);
	if(material.texture){
		float texture_x = uv_r.x * (32.0f / (1 << block->depth));
		float texture_y = uv_r.y * (32.0f / (1 << block->depth));
		pixel_t text = material.texture[(int)(tFract(texture_x) * material.texture[0].size) * material.texture[0].size + (int)(tFract(texture_y) * material.texture[0].size)];
		vec3mulvec3(&luminance_r,(vec3){text.r * 0.00392f,text.b * 0.00392f,text.g * 0.00392f});
	}
	return luminance_r;
}

vec3 getLuminance(block_side_t side,vec2 uv,int depth){
	if(depth >= LM_MAXDEPTH)
		return side.luminance;
	int lm_size = 1 << LM_MAXDEPTH - depth;

	vec2 test_1 = {tFractUnsigned(uv.x * lm_size + 0.5f),tFractUnsigned(uv.y * lm_size + 0.5f)};

	int lm_offset_x,lm_offset_y;
	lm_offset_x = uv.x * lm_size + 0.5f;
	lm_offset_y = uv.y * lm_size + 0.5f;

	vec3 lm_1 = side.luminance_p[lm_offset_x * (lm_size + 2) + lm_offset_y];
	vec3 lm_2 = side.luminance_p[lm_offset_x * (lm_size + 2) + lm_offset_y + 1];
	vec3 lm_3 = side.luminance_p[lm_offset_x * (lm_size + 2) + lm_offset_y + (lm_size + 2)];
	vec3 lm_4 = side.luminance_p[lm_offset_x * (lm_size + 2) + lm_offset_y + (lm_size + 2) + 1];

	vec3 mix_1 = vec3mixR(lm_1,lm_3,test_1.x);
	vec3 mix_2 = vec3mixR(lm_2,lm_4,test_1.x);

	return vec3mixR(mix_1,mix_2,test_1.y);
}

void draw_1(int p){
	vec3 camera_pos = camera.pos;
	traverse_init_t init = initTraverse(vec3divR(camera_pos,MAP_SIZE));
	switch(p){
	case 0:
		for(int x = WND_RESOLUTION.x / 2;x >= 0;x -= RD_BLOCK)
			for(int y = WND_RESOLUTION.y / 2;y >= 0;y -= RD_BLOCK)
				drawPixelGroup(x,y,init,camera_pos);
		for(int x = ((int)WND_RESOLUTION.x / 2) % RD_BLOCK;x >= 0;x--){
			for(int y = WND_RESOLUTION.y / 2;y >= 0;y--){
				tracePixel(init,getRayAngle(x,y),x,y,camera_pos);
			}
		}
		break;
	case 1:
		for(int x = WND_RESOLUTION.x / 2;x >= 0;x -= RD_BLOCK)
			for(int y = WND_RESOLUTION.y / 2;y < WND_RESOLUTION.y;y += RD_BLOCK)
				drawPixelGroup(x,y,init,camera_pos);
		for(int x = ((int)WND_RESOLUTION.x / 2) % RD_BLOCK;x >= 0;x--){
			for(int y = WND_RESOLUTION.y / 2;y < WND_RESOLUTION.y;y++){
				tracePixel(init,getRayAngle(x,y),x,y,camera_pos);
			}
		}
		break;
	case 2:
		for(int x = WND_RESOLUTION.x / 2;x < WND_RESOLUTION.x;x += RD_BLOCK)
			for(int y = WND_RESOLUTION.y / 2;y >= 0;y -= RD_BLOCK)
				drawPixelGroup(x,y,init,camera_pos);
		break;
	case 3:
		for(int x = WND_RESOLUTION.x / 2;x < WND_RESOLUTION.x;x += RD_BLOCK)
			for(int y = WND_RESOLUTION.y / 2;y < WND_RESOLUTION.y;y += RD_BLOCK)
				drawPixelGroup(x,y,init,camera_pos);
		break;
	}
}

void drawLine(vec2 pos_1,vec2 pos_2,pixel_t color){
	if(pos_1.x == 0.0f || pos_2.x == 0.0f)
		return;
	bool bound_1_x = pos_1.x > WND_RESOLUTION.x - 1.0f || pos_1.x < 0.0f;
	bool bound_1_y = pos_1.y > WND_RESOLUTION.y - 1.0f || pos_1.y < 0.0f;
	bool bound_2_x = pos_2.x > WND_RESOLUTION.x - 1.0f || pos_2.x < 0.0f;
	bool bound_2_y = pos_2.y > WND_RESOLUTION.y - 1.0f || pos_2.y < 0.0f;
	bool bound_1 = bound_1_x || bound_1_y;
	bool bound_2 = bound_2_x || bound_2_y;
	if(bound_1 && bound_2)
		return;
	if(bound_1){
		vec2 temp = pos_1;
		pos_1 = pos_2;
		pos_2 = temp;
	}
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
}

void draw(){
	for(;;){
		//printf("%f:%f:%f\n",camera.pos.x,camera.pos.y,camera.pos.z);
		is_rendering = true;
		unsigned long long t = __rdtsc();
		HANDLE draw_threads[4];
		draw_threads[0] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)0,0,0);
		draw_threads[1] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)1,0,0);
		draw_threads[2] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)2,0,0);
		draw_threads[3] = CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw_1,(void*)3,0,0);
		WaitForMultipleObjects(4,draw_threads,TRUE,INFINITE);

		int f = tMin(__rdtsc() - t >> 18,WND_RESOLUTION.x - 1);

		for(int i = 0;i < f;i++){
			for(int j = 10;j < 20;j++){
				vram.draw[i * (int)WND_RESOLUTION.y + j].g = 255;
			}
		}

		vec3 look_direction = getLookAngle(camera.dir);
		traverse_init_t init = initTraverse(vec3divR(camera.pos,MAP_SIZE));
		blockray_t result = traverseTree(ray3Create(init.pos,look_direction),init.node);
		if(result.node){
			block_t* block = result.node->block;
			float block_size = (float)MAP_SIZE / (1 << edit_depth) * 2.0f;

			ivec3 block_pos_i = block->pos;
			
			block_pos_i.a[result.side] += (look_direction.a[result.side] < 0.0f) * 2 - 1;
			vec3 block_pos;
			if(block->depth > edit_depth)
				block_pos = (vec3){block_pos_i.x >> (block->depth - edit_depth),block_pos_i.y >> (block->depth - edit_depth),block_pos_i.z >> (block->depth - edit_depth)};
			else
				block_pos = (vec3){block_pos_i.x << (edit_depth - block->depth),block_pos_i.y << (edit_depth - block->depth),block_pos_i.z << (edit_depth - block->depth)};
			vec3 point[8];
			vec2 screen_point[8];

			if(block->depth < edit_depth){
				vec3 pos;
				float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
				pos.x = block->pos.x * block_size;
				pos.y = block->pos.y * block_size;
				pos.z = block->pos.z * block_size;
				plane_t plane = getPlane(pos,look_direction,result.side,block_size);
				vec3 plane_pos = vec3subvec3R(plane.pos,camera.pos);
				float dst = rayIntersectPlane(plane_pos,look_direction,plane.normal);
				vec3 hit_pos = vec3addvec3R(camera.pos,vec3mulR(look_direction,dst));
				vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
				uv.x *= 1 << edit_depth - block->depth;
				uv.y *= 1 << edit_depth - block->depth;
				block_pos.a[result.side] += ((look_direction.a[result.side] > 0.0f) << (edit_depth - block->depth)) - (look_direction.a[result.side] > 0.0f);
				block_pos.a[plane.x] += tFloorf(uv.x);
				block_pos.a[plane.y] += tFloorf(uv.y);
			}

			vec3mul(&block_pos,block_size);

			point[0] = (vec3){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + 0.0f};
			point[1] = (vec3){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + block_size};
			point[2] = (vec3){block_pos.x + 0.0f,block_pos.y + block_size,block_pos.z + 0.0f};
			point[3] = (vec3){block_pos.x + 0.0f,block_pos.y + block_size,block_pos.z + block_size};
			point[4] = (vec3){block_pos.x + block_size,block_pos.y + 0.0f,block_pos.z + 0.0f};
			point[5] = (vec3){block_pos.x + block_size,block_pos.y + 0.0f,block_pos.z + block_size};
			point[6] = (vec3){block_pos.x + block_size,block_pos.y + block_size,block_pos.z + 0.0f};
			point[7] = (vec3){block_pos.x + block_size,block_pos.y + block_size,block_pos.z + block_size};
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

		if(!tool_select){
			if(material_array[block_type].texture){
				int offset_x = 20 / RES_SCALE;
				int offset_y = WND_RESOLUTION.y - 140 / RES_SCALE;
				int size = 120 / RES_SCALE;
				pixel_t* texture = material_array[block_type].texture;
				for(int i = offset_x;i < offset_x + size;i++){
					for(int j = offset_y;j < offset_y + size;j++){
						int tx = (i - offset_x) * (texture[0].size) / size;
						int ty = (j - offset_y) * (texture[0].size) / size;
						vram.draw[i * (int)WND_RESOLUTION.y + j] = texture[tx * texture[0].size + ty + 1];
					}
				}
			}
		}

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

void main(){
#pragma comment(lib,"Winmm.lib")
	timeBeginPeriod(1);

	bmi.bmiHeader.biWidth  = WND_RESOLUTION.y;
	bmi.bmiHeader.biHeight = WND_RESOLUTION.x;
	
	int depth = 4;
	int m = 1 << depth;
	for(int y = 0;y < m;y++){
		for(int x = 0;x < m;x++){
			setVoxel(x,y,0,depth);
		}
	}
	texture[0] = loadBMP("img/default.bmp");
	texture[1] = loadBMP("img/marble.bmp");
	texture[2] = loadBMP("img/ice.bmp");

	material_array[1].texture = texture[0];
	material_array[2].texture = texture[1];
	material_array[3].texture = texture[2];

	vram.draw = MALLOC(sizeof(vram_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);
	vram.render = MALLOC(sizeof(vram_t) * WND_RESOLUTION.x * WND_RESOLUTION.y);

	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE|WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(FALSE);

	CreateThread(0,0,(LPTHREAD_START_ROUTINE)draw,0,0,0);
	HANDLE thread_render = CreateThread(0,0,(LPTHREAD_START_ROUTINE)render,0,0,0);
	HANDLE physics_thread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)physics,0,0,0);
	SetThreadPriority(physics_thread,THREAD_PRIORITY_ABOVE_NORMAL);
	MSG msg;
	while(GetMessageA(&msg,window,0,0)){
		TranslateMessage(&msg);
 		DispatchMessageA(&msg);
	}
}