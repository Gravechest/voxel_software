#include <Windows.h>
#include <math.h>
#include <stdio.h>

#include "tmath.h"
#include "vec2.h"
#include "vec3.h"
#include "dda.h"

#define MV_MODE 0

#define WALK_SPEED 0.012f
#define WALK_SPEED_DIA (WALK_SPEED * 0.5625f)

#define GROUND_FRICTION 0.78f

#define RD_BLOCK 4

#define RES_SCALE 2
#define FOV (VEC2){600.0f / RES_SCALE,600.0f / RES_SCALE}
#define WND_RESOLUTION (VEC2){1080 / RES_SCALE,1920 / RES_SCALE}
#define WND_SIZE (VEC2){1920,1080}

#define VK_W 0x57
#define VK_S 0x53
#define VK_A 0x41
#define VK_D 0x44

#define MAP_SIZE 32

#define LM_RES 16

#define MALLOC(AMMOUNT) HeapAlloc(GetProcessHeap(),0,AMMOUNT)
#define MALLOC_ZERO(AMMOUNT) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,AMMOUNT)
#define MFREE(POINTER) HeapFree(GetProcessHeap(),0,POINTER)

#define true 1;
#define false 0;

typedef char bool;

typedef struct{
	float x;
	float y;
	float z;
	float w;
}VEC4;

typedef struct{
	unsigned char r;
	unsigned char g;
	unsigned char b;
}PIXEL;

typedef struct{
	VEC3 pos;
	VEC2 dir;
	VEC4 tri;
	VEC3 vel;
}CAMERA;

typedef struct{
	PIXEL* draw;
	PIXEL* render;
}VRAM;

IVEC3 block_deleted = {-1,-1,-1};

int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam);

WNDCLASSA wndclass = {.lpfnWndProc = proc,.lpszClassName = "class",.lpszMenuName = "class"};
BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER),0,0,1,24,BI_RGB};
HWND window;
HDC context;
volatile VRAM vram;
CAMERA camera = {.pos.z = 5.0f,.pos.x = 3.0f,.pos.y = 3.0f};

int blocktype_select;
int color_select;
int edit_mode;

volatile bool is_rendering;

VEC3 color_pallet[6] = {
	{0.9f,0.9f,0.9f},
	{0.9f,0.3f,0.3f},
	{0.3f,0.9f,0.3f},
	{0.3f,0.3f,0.9f},
	{0.9f,0.9f,0.3f},
	{0.9f,0.3f,0.9f}
};

typedef struct{
	int lm_updated;
	float lm_update_priority;
	VEC3 base_color;
	VEC3 color[4][4];
}BLOCK_SIDE;

typedef struct{
	int type;
	BLOCK_SIDE side[6];
}BLOCK;

typedef struct BLOCK_NODE{
	BLOCK* block;
	union{
		struct BLOCK_NODE* block_node[2][2][2];
		struct BLOCK_NODE* block_node_s[8];
	};
}BLOCK_NODE;

BLOCK_NODE block_node;
	
volatile char frame_ready;

void render(){
	for(;;){
		StretchDIBits(context,0,0,WND_SIZE.x,WND_SIZE.y,0,0,WND_RESOLUTION.y,WND_RESOLUTION.x,vram.render,&bmi,0,SRCCOPY);
		frame_ready = FALSE;
	}
}

VEC2 sampleCube(VEC3 v,int* faceIndex){
	VEC3 vAbs = VEC3absR(v);
	float ma;
	VEC2 uv;
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y){
		*faceIndex = (v.z < 0.0f) + 4;
		ma = 0.5f / vAbs.z;
		uv = (VEC2){v.z < 0.0f ? -v.x : v.x, -v.y};
	}
	else if(vAbs.y >= vAbs.x){
		*faceIndex = (v.y < 0.0f) + 2;
		ma = 0.5f / vAbs.y;
		uv = (VEC2){v.x, v.y < 0.0f ? -v.z : v.z};
	}
	else{
		*faceIndex = v.x < 0.0f;
		ma = 0.5f / vAbs.x;
		uv = (VEC2){v.x < 0.0f ? v.z : -v.z, -v.y};
	}
	return (VEC2){uv.x * ma + 0.5f,uv.y * ma + 0.5f};
}

VEC3 getLookAngle(VEC2 angle){
	VEC3 ang;
	ang.x = cosf(angle.x) * cosf(angle.y);
	ang.y = sinf(angle.x) * cosf(angle.y);
	ang.z = sinf(angle.y);
	return ang;
}
/*
void genLightSurface(float* lm_update_priority,VEC3* color,VEC3 base_color,VEC3 pos,VEC2 ang){
	VEC3 c_color = VEC3_ZERO;
	VEC3 normal = getLookAngle(ang);
	float weight = 0.0f;
	ang.x -= 1.0f - 1.0f / LM_RES;
	ang.y -= 1.0f - 1.0f / LM_RES;
	for(int x = 0;x < LM_RES;x++){
		for(int y = 0;y < LM_RES;y++){
			RAY3 ray = ray3Create(pos,getLookAngle(ang));
			ray3Itterate(&ray);
			for(;;){
				bool bound_x = ray.square_pos.x < 0 || ray.square_pos.x >= MAP_SIZE;
				bool bound_y = ray.square_pos.y < 0 || ray.square_pos.y >= MAP_SIZE;
				bool bound_z = ray.square_pos.z < 0 || ray.square_pos.z >= MAP_SIZE;
				if(bound_x || bound_y || bound_z){
					int side;
					VEC2 uv = sampleCube(ray.dir,&side);
					if(side == 1 && uv.x > 0.7f && uv.x < 0.9f && uv.y > 0.4f && uv.y < 0.6f){
						VEC3addVEC3(&c_color,VEC3divR((VEC3){9.0f,15.0f,18.0f},LM_RES * LM_RES));
						break;
					}
					VEC3addVEC3(&c_color,VEC3divR((VEC3){0.6f,0.5f,0.3f},LM_RES * LM_RES));
					break;
				}
				BLOCK* block = map[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
				if(!block){
					ray3Itterate(&ray);
					continue;
				}
				int side = ray.square_side * 2 + (ray.dir.a[ray.square_side] < 0.0f);
				float diffuse = VEC3dotR(ray.dir,normal);
				weight += diffuse;
				if(block->type){
					VEC3 m_color = block->side[side].base_color;
					VEC3mul(&m_color,3.0f);
					VEC3addVEC3(&c_color,VEC3mulR(VEC3divR(m_color,LM_RES * LM_RES),diffuse));
					break;
				}
				VEC2 uv = ray3UV(ray);
				VEC3 m_color = VEC3mulVEC3R(block->side[side].color[(int)(uv.x * 2.0f) + 1][(int)(uv.y * 2.0f) + 1],block->side[side].base_color);
				VEC3addVEC3(&c_color,VEC3mulR(VEC3divR(m_color,LM_RES * LM_RES),diffuse));
				ray3Itterate(&ray);
				break;
			}
			ang.x += 2.0f / LM_RES;
		}
		ang.x -= 2.0f;
		ang.y += 2.0f / LM_RES;
	}
	*lm_update_priority += tMaxf(tAbsf(tAbsf(c_color.b - color->b)),tMaxf(tAbsf(c_color.r - color->r),tAbsf(c_color.g - color->g)));
	VEC3div(color,weight);
	*color = c_color;
}*/
/*
void filterSurface(IVEC3 crd,int side,int x,int y,int dx,int dy,int dz){
	if(crd.a[side >> 1] < 0)
		return;
	VEC3 (*lightmap)[4][4] = &map[crd.z][crd.y][crd.x]->side[side].color;
	crd.a[x]--;
	if(crd.a[x] > 0){
		if(map[crd.z][crd.y][crd.x] && !map[crd.z][crd.y][crd.x]->type && !map[crd.z + dz][crd.y + dy][crd.x + dx]){
			(*lightmap)[0][0] = VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[2][1],0.5f);
			(*lightmap)[0][1] = map[crd.z][crd.y][crd.x]->side[side].color[2][1];
			(*lightmap)[0][2] = map[crd.z][crd.y][crd.x]->side[side].color[2][2];
			(*lightmap)[0][3] = VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[2][2],0.5f);
		}
		else{
			(*lightmap)[0][0] = VEC3mulR((*lightmap)[1][1],0.5f);
			(*lightmap)[0][1] = (*lightmap)[1][1];
			(*lightmap)[0][2] = (*lightmap)[1][2];
			(*lightmap)[0][3] = VEC3mulR((*lightmap)[1][1],0.5f);
		}
	}
	crd.a[x] += 2;
	if(crd.a[x] != MAP_SIZE){
		if(map[crd.z][crd.y][crd.x] && !map[crd.z][crd.y][crd.x]->type && !map[crd.z + dz][crd.y + dy][crd.x + dx]){
			(*lightmap)[3][0] = VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[1][1],0.5f);
			(*lightmap)[3][1] = map[crd.z][crd.y][crd.x]->side[side].color[1][1];
			(*lightmap)[3][2] = map[crd.z][crd.y][crd.x]->side[side].color[1][2];
			(*lightmap)[3][3] = VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[1][2],0.5f);
		}
		else{
			(*lightmap)[3][0] = VEC3mulR((*lightmap)[2][1],0.5f);
			(*lightmap)[3][1] = (*lightmap)[2][1];
			(*lightmap)[3][2] = (*lightmap)[2][2];
			(*lightmap)[3][3] = VEC3mulR((*lightmap)[2][2],0.5f);
		}
	}
	crd.a[x]--;
	crd.a[y]++;
	if(crd.a[y] != MAP_SIZE){
		if(map[crd.z][crd.y][crd.x] && !map[crd.z][crd.y][crd.x]->type && !map[crd.z + dz][crd.y + dy][crd.x + dx]){
			VEC3addVEC3(&(*lightmap)[0][3],VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[1][1],0.5f));
			(*lightmap)[1][3] = map[crd.z][crd.y][crd.x]->side[side].color[1][1];
			(*lightmap)[2][3] = map[crd.z][crd.y][crd.x]->side[side].color[2][1];
			VEC3addVEC3(&(*lightmap)[3][3],VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[2][1],0.5f));
		}
		else{
			VEC3addVEC3(&(*lightmap)[0][3],VEC3mulR((*lightmap)[1][2],0.5f));
			(*lightmap)[1][3] = (*lightmap)[1][2];
			(*lightmap)[2][3] = (*lightmap)[2][2];
			VEC3addVEC3(&(*lightmap)[3][3],VEC3mulR((*lightmap)[2][2],0.5f));
		}
	}
	crd.a[y] -= 2;
	if(crd.a[y] > 0){
		if(map[crd.z][crd.y][crd.x] && !map[crd.z][crd.y][crd.x]->type && !map[crd.z + dz][crd.y + dy][crd.x + dx]){
			VEC3addVEC3(&(*lightmap)[0][0],VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[1][2],0.5f));
			(*lightmap)[1][0] = map[crd.z][crd.y][crd.x]->side[side].color[1][2];
			(*lightmap)[2][0] = map[crd.z][crd.y][crd.x]->side[side].color[2][2];
			VEC3addVEC3(&(*lightmap)[3][0],VEC3mulR(map[crd.z][crd.y][crd.x]->side[side].color[2][2],0.5f));
			return;
		}
		VEC3addVEC3(&(*lightmap)[0][0],VEC3mulR((*lightmap)[1][1],0.5f));
		(*lightmap)[1][0] = (*lightmap)[1][1];
		(*lightmap)[2][0] = (*lightmap)[2][1];
		VEC3addVEC3(&(*lightmap)[3][0],VEC3mulR((*lightmap)[2][1],0.5f));
	}
}*/
/*
void lightmap(){
	for(;;){
		float max_priority = 0.0f;
		for(int x = 0;x < MAP_SIZE;x++){
			for(int y = 0;y < MAP_SIZE;y++){
				for(int z = 0;z < MAP_SIZE;z++){
					if(!map[z][y][x] || map[z][y][x]->type)
						continue;
					for(int i = 0;i < 6;i++)
						max_priority = tMaxf(max_priority,map[z][y][x]->side[i].lm_update_priority);
				}
			}
		}
		max_priority *= 0.5f;
		int n = 0;
		for(int x = 0;x < MAP_SIZE;x++){
			for(int y = 0;y < MAP_SIZE;y++){
				for(int z = 0;z < MAP_SIZE;z++){
					if(!map[z][y][x] || map[z][y][x]->type)
						continue;
					for(int i = 0;i < 6;i++){
						float priority = map[z][y][x]->side[i].lm_update_priority;
						if(!priority || priority < max_priority)
							continue;
						if(map[z][y][x]->side[i].lm_updated--)
							continue;
						map[z][y][x]->side[i].lm_updated = 4;
						if(n++ > 1024)
							goto end;
						map[z][y][x]->side[i].lm_update_priority = 0.0f;
						switch(i >> 1){
						case VEC3_X:
							if(i & 1 && x && !map[z][y][x - 1]){
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[0].color[1][1],map[z][y][x]->side[0].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.01f,0.25f,0.25f}),(VEC2){M_PI,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[0].color[1][2],map[z][y][x]->side[0].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.01f,0.25f,0.75f}),(VEC2){M_PI,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[0].color[2][1],map[z][y][x]->side[0].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.01f,0.75f,0.25f}),(VEC2){M_PI,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[0].color[2][2],map[z][y][x]->side[0].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.01f,0.75f,0.75f}),(VEC2){M_PI,0.0f});
							}
							if(!(i & 1) && !map[z][y][x + 1]){
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[1].color[1][1],map[z][y][x]->side[1].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.99f,0.25f,0.25f}),(VEC2){0.0f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[1].color[1][2],map[z][y][x]->side[1].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.99f,0.25f,0.75f}),(VEC2){0.0f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[1].color[2][1],map[z][y][x]->side[1].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.99f,0.75f,0.25f}),(VEC2){0.0f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[1].color[2][2],map[z][y][x]->side[1].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.99f,0.75f,0.75f}),(VEC2){0.0f,0.0f});
							}
							break;
						case VEC3_Y:
							if(i & 1 && y && !map[z][y - 1][x]){
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[2].color[1][1],map[z][y][x]->side[2].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.01f,0.25f}),(VEC2){-M_PI*0.5f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[2].color[1][2],map[z][y][x]->side[2].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.01f,0.75f}),(VEC2){-M_PI*0.5f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[2].color[2][1],map[z][y][x]->side[2].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.01f,0.25f}),(VEC2){-M_PI*0.5f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[2].color[2][2],map[z][y][x]->side[2].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.01f,0.75f}),(VEC2){-M_PI*0.5f,0.0f});
							}
							if(!(i & 1) && !map[z][y + 1][x]){
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[3].color[1][1],map[z][y][x]->side[3].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.99f,0.25f}),(VEC2){M_PI*0.5f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[3].color[1][2],map[z][y][x]->side[3].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.99f,0.75f}),(VEC2){M_PI*0.5f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[3].color[2][1],map[z][y][x]->side[3].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.99f,0.25f}),(VEC2){M_PI*0.5f,0.0f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[3].color[2][2],map[z][y][x]->side[3].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.99f,0.75f}),(VEC2){M_PI*0.5f,0.0f});
							}
							break;
						case VEC3_Z:
							if(i & 1 && z && !map[z - 1][y][x]){
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[4].color[1][1],map[z][y][x]->side[4].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.25f,0.01f}),(VEC2){0.0f,-M_PI*0.5f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[4].color[1][2],map[z][y][x]->side[4].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.75f,0.01f}),(VEC2){0.0f,-M_PI*0.5f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[4].color[2][1],map[z][y][x]->side[4].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.25f,0.01f}),(VEC2){0.0f,-M_PI*0.5f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[4].color[2][2],map[z][y][x]->side[4].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.75f,0.01f}),(VEC2){0.0f,-M_PI*0.5f});
							}
							if(!(i & 1) && !map[z + 1][y][x]){
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[5].color[1][1],map[z][y][x]->side[5].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.25f,0.99f}),(VEC2){0.0f,M_PI*0.5f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[5].color[1][2],map[z][y][x]->side[5].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.25f,0.75f,0.99f}),(VEC2){0.0f,M_PI*0.5f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[5].color[2][1],map[z][y][x]->side[5].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.25f,0.99f}),(VEC2){0.0f,M_PI*0.5f});
								genLightSurface(&map[z][y][x]->side[i].lm_update_priority,&map[z][y][x]->side[5].color[2][2],map[z][y][x]->side[5].base_color,VEC3addVEC3R((VEC3){x,y,z},(VEC3){0.75f,0.75f,0.99f}),(VEC2){0.0f,M_PI*0.5f});
							}
							break;
						}
					}
				}
			}
		}
	end:
		for(int x = 0;x < MAP_SIZE;x++){
			for(int y = 0;y < MAP_SIZE;y++){
				for(int z = 0;z < MAP_SIZE;z++){
					if(!map[z][y][x] || map[z][y][x]->type)
						continue;
					for(int i = 0;i < 6;i++){
						switch(i >> 1){
						case VEC3_X:
							filterSurface((IVEC3){x,y,z},i,VEC3_Y,VEC3_Z,(i & 1) * 2 - 1,0,0);
							break;
						case VEC3_Y:
							filterSurface((IVEC3){x,y,z},i,VEC3_X,VEC3_Z,0,(i & 1) * 2 - 1,0);
							break;
						case VEC3_Z:
							filterSurface((IVEC3){x,y,z},i,VEC3_X,VEC3_Y,0,0,(i & 1) * 2 - 1);
							break;
						}
					}
				}
			}
		}
		if(n < 512)
			Sleep(9);
	}
}*/
/*
void setUpdatePriority(VEC3 block_pos){
	for(int x = 0;x < MAP_SIZE;x++){
		for(int y = 0;y < MAP_SIZE;y++){
			for(int z = 0;z < MAP_SIZE;z++){
				if(!map[z][y][x])
					continue;
				float dst = VEC3distance((VEC3){x,y,z},block_pos);
				float inv_dst = 50.0f / (dst * dst);
				for(int i = 0;i < 6;i++)
					map[z][y][x]->side[i].lm_update_priority = inv_dst;
			}
		}
	}
}
*/
int proc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
	switch(msg){
	case WM_KEYDOWN:
		switch(wParam){
		case VK_RIGHT:
			edit_mode++;
			break;
		case VK_LEFT:
			edit_mode--;
			break;
		case VK_ADD:
			if(edit_mode)
				color_select++;
			else
				blocktype_select++;
			break;
		case VK_SUBTRACT:
			if(edit_mode)
				color_select--;
			else
				blocktype_select--;
			break;
		}
		break;
	case WM_RBUTTONDOWN:{/*
		RAY3 ray = ray3Create(camera.pos,getLookAngle(camera.dir));
		ray3Itterate(&ray);
		for(;;){
			bool bound_x = ray.square_pos.x < 0 || ray.square_pos.x >= MAP_SIZE;
			bool bound_y = ray.square_pos.y < 0 || ray.square_pos.y >= MAP_SIZE;
			bool bound_z = ray.square_pos.z < 0 || ray.square_pos.z >= MAP_SIZE;
			if(bound_x || bound_y || bound_z)
				break;
			if(map[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x]){
				block_deleted = ray.square_pos;
				break;
			}
			ray3Itterate(&ray);
		}
		break;*/}
	case WM_LBUTTONDOWN:;/*
		RAY3 ray = ray3Create(camera.pos,getLookAngle(camera.dir));
		ray3Itterate(&ray);
		for(;;){
			int bound_x = ray.square_pos.x < 0 | ray.square_pos.x >= MAP_SIZE;
			int bound_y = ray.square_pos.y < 0 | ray.square_pos.y >= MAP_SIZE;
			int bound_z = ray.square_pos.z < 0 | ray.square_pos.z >= MAP_SIZE;
			if(bound_x || bound_y || bound_z)
				break;
			if(map[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x]){
				switch(edit_mode){
				case 1:;
					int side = ray.square_side * 2 + (ray.dir.a[ray.square_side] < 0.0f);
					map[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x]->side[side].base_color = color_pallet[color_select];
					setUpdatePriority((VEC3){ray.square_pos.x,ray.square_pos.y,ray.square_pos.z});
					break;
				default:
					ray.square_pos.a[ray.square_side] += (ray.dir.a[ray.square_side] < 0.0f) * 2 - 1;
					map[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x] = MALLOC_ZERO(sizeof(BLOCK));
					BLOCK* block = map[ray.square_pos.z][ray.square_pos.y][ray.square_pos.x];
					block->type = blocktype_select;
					for(int i = 0;i < 6;i++)
						block->side[i].base_color = color_pallet[color_select];
					setUpdatePriority((VEC3){ray.square_pos.x,ray.square_pos.y,ray.square_pos.z});
					break;
				}
				break;
			}
			ray3Itterate(&ray);
		}
		break;*/
	case WM_MOUSEMOVE:;
		POINT cur;
		RECT offset;
		VEC2 r;
		GetCursorPos(&cur);
		GetWindowRect(window,&offset);
		float mx = (float)(cur.x-(offset.left+WND_SIZE.x/2))*0.005f;
		float my = (float)(cur.y-(offset.top+WND_SIZE.y/2))*0.005f;
		camera.dir.x += mx;
		camera.dir.y -= my;
		SetCursorPos(offset.left+WND_SIZE.x/2,offset.top+WND_SIZE.y/2);
		break;
	case WM_DESTROY: case WM_CLOSE:
		ExitProcess(0);
	}
	return DefWindowProcA(hwnd,msg,wParam,lParam);
}

VEC3 getRayAngle(int x,int y){
	VEC3 ray_ang;
	float pxY = (((float)(x + 0.5f) * 2.0f / WND_RESOLUTION.x) - 1.0f);
	float pxX = (((float)(y + 0.5f) * 2.0f / WND_RESOLUTION.y) - 1.0f);
	float pixelOffsetY = pxY / (FOV.x / WND_RESOLUTION.x * 2.0f);
	float pixelOffsetX = pxX / (FOV.y / WND_RESOLUTION.y * 2.0f);
	ray_ang.x = (camera.tri.x * camera.tri.z - camera.tri.x * camera.tri.w * pixelOffsetY) - camera.tri.y * pixelOffsetX;
	ray_ang.y = (camera.tri.y * camera.tri.z - camera.tri.y * camera.tri.w * pixelOffsetY) + camera.tri.x * pixelOffsetX;
	ray_ang.z = camera.tri.w + camera.tri.z * pixelOffsetY;
	return ray_ang;
}

typedef struct{
	VEC3 pos;
	VEC3 normal;
	int x;
	int y;
}PLANE;

PLANE rayGetPlane(RAY3 ray){
	switch(ray.square_side){
	case VEC3_X:
		return (PLANE){{ray.square_pos.x + (ray.dir.x < 0.0f),ray.square_pos.y,ray.square_pos.z},{1.0f,0.0f,0.0f},VEC3_Y,VEC3_Z};
	case VEC3_Y:
		return (PLANE){{ray.square_pos.x,ray.square_pos.y + (ray.dir.y < 0.0f),ray.square_pos.z},{0.0f,1.0f,0.0f},VEC3_X,VEC3_Z};
	case VEC3_Z:
		return (PLANE){{ray.square_pos.x,ray.square_pos.y,ray.square_pos.z + (ray.dir.z < 0.0f)},{0.0f,0.0f,1.0f},VEC3_X,VEC3_Y};
	}
}

float rayIntersectPlane(VEC3 pos,VEC3 dir,VEC3 plane){
	return -VEC3dotR(pos,plane) / VEC3dotR(dir,plane);
}
/*
void again(int x,int y){
	RAY3 ray = ray3Create(camera.pos,getRayAngle(x,y));
	ray3Itterate(&ray);
	for(;;){
		int bound_x = ray.square_pos.x < 0 || ray.square_pos.x >= MAP_SIZE;
		int bound_y = ray.square_pos.y < 0 || ray.square_pos.y >= MAP_SIZE;
		int bound_z = ray.square_pos.z < 0 || ray.square_pos.z >= MAP_SIZE;
		if(bound_x || bound_y || bound_z){
			int side;
			VEC2 uv = sampleCube(ray.dir,&side);
			if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.4f && uv.y < 0.6f){
				vram.draw[x * (int)WND_RESOLUTION.y + y] = (PIXEL){255,255,255};
				continue;
			}
			vram.draw[x * (int)WND_RESOLUTION.y + y] = (PIXEL){200,200,200};
			break;
		}
		IVEC3 block = ray.square_pos;
		if(map[block.z][block.y][block.x]){
			int side = ray.square_side * 2 + (ray.dir.a[ray.square_side] < 0.0f);
			if(map[block.z][block.y][block.x]->type){
				vram.draw[x * (int)WND_RESOLUTION.y + y] = (PIXEL){255,255,255};
				break;
			}
			VEC2 uv = ray3UV(ray);
			if(uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
				return;
			VEC3 color[4];
			VEC2 bl_filter = {tFract(uv.x * 2.0f + 0.5f),tFract(uv.y * 2.0f + 0.5f)};
			IVEC2 bl_pos = {uv.x * 2.0f + 0.5f,uv.y * 2.0f + 0.5f};

			color[0] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 0][bl_pos.y + 0];
			color[1] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 1][bl_pos.y + 0];
			color[2] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 0][bl_pos.y + 1];
			color[3] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 1][bl_pos.y + 1];

			VEC3 m_1 = VEC3mixR(color[0],color[1],bl_filter.x);
			VEC3 m_2 = VEC3mixR(color[2],color[3],bl_filter.x);
			VEC3 f_color = VEC3mixR(m_1,m_2,bl_filter.y);

			VEC3mulVEC3(&f_color,map[block.z][block.y][block.x]->side[side].base_color);

			if(((int)(uv.x * 4.0f) & 1) ^ ((int)(uv.y * 4.0f) & 1))
				VEC3mul(&f_color,0.75f);
					
			vram.draw[x * (int)WND_RESOLUTION.y + y] = (PIXEL){tMinf(f_color.r * 255.0f,255.0f),tMinf(f_color.g * 255.0f,255.0f),tMinf(f_color.b * 255.0f,255.0f)};
			break;
		}
		ray3Itterate(&ray);
	}
}
*/
VEC3 getSubCoords(RAY3 ray){
	VEC2 wall;
	switch(ray.square_side){
	case 0:
		wall.x = tFract(ray.pos.y + (ray.side.x - ray.delta.x) * ray.dir.y) * 2.0f;
		wall.y = tFract(ray.pos.z + (ray.side.x - ray.delta.x) * ray.dir.z) * 2.0f;
		return (VEC3){(ray.dir.x < 0.0f) * 1.0f,wall.x,wall.y};
	case 1:
		wall.x = tFract(ray.pos.x + (ray.side.y - ray.delta.y) * ray.dir.x) * 2.0f;
		wall.y = tFract(ray.pos.z + (ray.side.y - ray.delta.y) * ray.dir.z) * 2.0f;
		return (VEC3){wall.x,(ray.dir.y < 0.0f) * 1.0f,wall.y};
	case 2:
		wall.x = tFract(ray.pos.x + (ray.side.z - ray.delta.z) * ray.dir.x) * 2.0f;
		wall.y = tFract(ray.pos.y + (ray.side.z - ray.delta.z) * ray.dir.y) * 2.0f;
		return (VEC3){wall.x,wall.y,(ray.dir.z < 0.0f) * 1.0f};
	}
}

bool traverseTree(RAY3 ray,BLOCK_NODE* node,int x,int y,int depth){
	if(node->block){
		//PLANE plane = rayGetPlane(ray);
		for(int px = 0;px < RD_BLOCK;px++){
			for(int py = 0;py < RD_BLOCK;py++){
				vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){255,depth ? 255 : 0,0};
			}
		}
		return true;
	}
	for(;;){
		bool bound_x = ray.square_pos.x < 0 || ray.square_pos.x >= 2;
		bool bound_y = ray.square_pos.y < 0 || ray.square_pos.y >= 2;
		bool bound_z = ray.square_pos.z < 0 || ray.square_pos.z >= 2;
		if(bound_x || bound_y || bound_z){
			if(depth)
				return false;
			for(int px = 0;px < RD_BLOCK;px++){
				for(int py = 0;py < RD_BLOCK;py++){
					int side;
					VEC2 uv = sampleCube(ray.dir,&side);
					if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.4f && uv.y < 0.6f){
						vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){255,255,255};
						continue;
					}
					vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){200,200,200};
				}
			}
			return true;
		}
		IVEC3 block = ray.square_pos;
		BLOCK_NODE* node_deep = node->block_node[block.z][block.y][block.x];
		if(node_deep){
			IVEC3 pos_t = ray.square_pos;
			if(traverseTree(ray3Create(getSubCoords(ray),ray.dir),node_deep,x,y,depth + 1))
				return true;
			ray.square_pos = pos_t;
			VEC3mul(&ray.delta,2.0f);
		}
		ray3Itterate(&ray);
	}
}

void drawPixel(int x,int y,BLOCK_NODE* node){
	RAY3 ray = ray3Create(VEC3divR(camera.pos,MAP_SIZE),getRayAngle(0.5f * RD_BLOCK - 0.5f + x,0.5f * RD_BLOCK - 0.5f + y));
	traverseTree(ray,node,x,y,0);/*
	for(;;){
		int bound_x = ray.square_pos.x < 0 || ray.square_pos.x >= 2;
		int bound_y = ray.square_pos.y < 0 || ray.square_pos.y >= 2;
		int bound_z = ray.square_pos.z < 0 || ray.square_pos.z >= 2;
		if(bound_x || bound_y || bound_z){
				for(int px = 0;px < RD_BLOCK;px++){
					for(int py = 0;py < RD_BLOCK;py++){
						int side;
						VEC2 uv = sampleCube(ray.dir,&side);
						if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.4f && uv.y < 0.6f){
							vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){255,255,255};
							continue;
						}
						vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){200,200,200};
					}
				}
				break;
			node = pre_node;
		}
		IVEC3 block = ray.square_pos;
		if(node.block[block.z][block.y][block.x]){
			int side = ray.square_side * 2 + (ray.dir.a[ray.square_side] < 0.0f);
			if(map[block.z][block.y][block.x]->type){
				vram.draw[x * (int)WND_RESOLUTION.y + y] = (PIXEL){255,255,255};
				break;
			}
			PLANE plane = rayGetPlane(ray);
			for(int px = 0;px < RD_BLOCK;px++){
				for(int py = 0;py < RD_BLOCK;py++){/*
					ray.dir = getRayAngle(x + px,y + py);
					float dst = rayIntersectPlane(VEC3subVEC3R(ray.pos,plane.pos),ray.dir,plane.normal);
					VEC3 pos = VEC3addVEC3R(ray.pos,VEC3mulR(ray.dir,dst));
					VEC2 uv = {pos.a[plane.x] - ray.square_pos.a[plane.x],pos.a[plane.y] - ray.square_pos.a[plane.y]};
					if(uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f){
						again(x + px,y + py);
						continue;
					}
					VEC3 color[4];
					VEC2 bl_filter = {tFract(uv.x * 2.0f + 0.5f),tFract(uv.y * 2.0f + 0.5f)};
					IVEC2 bl_pos = {uv.x * 2.0f + 0.5f,uv.y * 2.0f + 0.5f};

					color[0] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 0][bl_pos.y + 0];
					color[1] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 1][bl_pos.y + 0];
					color[2] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 0][bl_pos.y + 1];
					color[3] = map[block.z][block.y][block.x]->side[side].color[bl_pos.x + 1][bl_pos.y + 1];

					VEC3 m_1 = VEC3mixR(color[0],color[1],bl_filter.x);
					VEC3 m_2 = VEC3mixR(color[2],color[3],bl_filter.x);
					VEC3 f_color = VEC3mixR(m_1,m_2,bl_filter.y);

					VEC3mulVEC3(&f_color,map[block.z][block.y][block.x]->side[side].base_color);

					if(((int)(uv.x * 4.0f) & 1) ^ ((int)(uv.y * 4.0f) & 1))
						VEC3mul(&f_color,0.75f);
					
					vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){tMinf(f_color.r * 255.0f,255.0f),tMinf(f_color.g * 255.0f,255.0f),tMinf(f_color.b * 255.0f,255.0f)};
					
					vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (PIXEL){255,0,0};
				}
			}
			break;
		}
		ray3Itterate(&ray);
	}*/
}

typedef struct{
	char w;
	char s;
	char d;
	char a;
}KEYS;

KEYS key;

void walkPhysics(){
	char hx = FALSE,hy = FALSE,hz = FALSE;/*
	for(float x = -0.2f;x <= 0.2f;x += 0.05f){
		for(float y = -0.2f;y <= 0.2f;y += 0.05f){
			if(map[(int)(camera.pos.z - 1.8f + camera.vel.z)][(int)(camera.pos.y + y)][(int)(camera.pos.x + x)])
				hz = TRUE;
		}
	}
	for(float x = -1.8f;x <= 0.1f;x += 0.05f){
		for(float y = -0.2f;y <= 0.2f;y += 0.05f){
			if(map[(int)(camera.pos.z + x)][(int)(camera.pos.y + y)][(int)(camera.pos.x - 0.2f + camera.vel.x)])
				hx = TRUE;
			if(map[(int)(camera.pos.z + x)][(int)(camera.pos.y + y)][(int)(camera.pos.x + 0.2f + camera.vel.x)])
				hx = TRUE;
			if(map[(int)(camera.pos.z + x)][(int)(camera.pos.y + -0.2f + camera.vel.y)][(int)(camera.pos.x + y)])
				hy = TRUE;
			if(map[(int)(camera.pos.z + x)][(int)(camera.pos.y + 0.2f + camera.vel.y)][(int)(camera.pos.x + y)])
				hy = TRUE;
		}
	}*/
	VEC3addVEC3(&camera.pos,camera.vel);
	if(hx){
		camera.pos.x -= camera.vel.x;
		camera.vel.x = 0.0f;
	}
	if(hy){
		camera.pos.y -= camera.vel.y;
		camera.vel.y = 0.0f;
	}
	if(hz){
		camera.pos.z -= camera.vel.z;
		camera.vel.z = 0.0f;
		if(key.w){
			float mod = key.d || key.a ? WALK_SPEED_DIA : WALK_SPEED;
			camera.vel.x += cosf(camera.dir.x) * mod * 2.0f;
			camera.vel.y += sinf(camera.dir.x) * mod * 2.0f;
		}
		if(key.s){
			float mod = key.d || key.a ? WALK_SPEED_DIA : WALK_SPEED;
			camera.vel.x -= cosf(camera.dir.x) * mod * 2.0f;
			camera.vel.y -= sinf(camera.dir.x) * mod * 2.0f;
		}
		if(key.d){
			float mod = key.s || key.w ? WALK_SPEED_DIA : WALK_SPEED;
			camera.vel.x += cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
			camera.vel.y += sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		}
		if(key.a){
			float mod = key.s || key.w ? WALK_SPEED_DIA : WALK_SPEED;
			camera.vel.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
			camera.vel.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod * 2.0f;
		}
		if(GetKeyState(VK_SPACE) & 0x80)
			camera.vel.z = 0.25f;
		VEC3mul(&camera.vel,GROUND_FRICTION); 
	}
	else
		VEC3mul(&camera.vel,0.99f);
	camera.vel.z -= 0.015f;
}

void physics(){
	for(;;){
		key.w = GetKeyState(VK_W) & 0x80;
		key.s = GetKeyState(VK_S) & 0x80;
		key.d = GetKeyState(VK_D) & 0x80;
		key.a = GetKeyState(VK_A) & 0x80;
		camera.tri = (VEC4){cosf(camera.dir.x),sinf(camera.dir.x),cosf(camera.dir.y),sinf(camera.dir.y)};
		if(MV_MODE){
			walkPhysics();
			Sleep(3);
			continue;
		}
		float mod = (GetKeyState(VK_LCONTROL) & 0x80) ? 3.0f : 1.0f;
		if(key.w){
			float mod_2 = key.d || key.a ? mod * 0.042f : mod * 0.06f;
			camera.pos.x += cosf(camera.dir.x) * mod_2;
			camera.pos.y += sinf(camera.dir.x) * mod_2;
		}
		if(key.s){
			float mod_2 = key.d || key.a ? mod * 0.042f : mod * 0.06f;
			camera.pos.x -= cosf(camera.dir.x) * mod_2;
			camera.pos.y -= sinf(camera.dir.x) * mod_2;
		}
		if(key.d){
			float mod_2 = key.s || key.w ? mod * 0.042f : mod * 0.06f;
			camera.pos.x += cosf(camera.dir.x + M_PI * 0.5f) * mod_2;
			camera.pos.y += sinf(camera.dir.x + M_PI * 0.5f) * mod_2;
		}
		if(key.a){
			float mod_2 = key.s || key.w ? mod * 0.042f : mod * 0.06f;
			camera.pos.x -= cosf(camera.dir.x + M_PI * 0.5f) * mod_2;
			camera.pos.y -= sinf(camera.dir.x + M_PI * 0.5f) * mod_2;
		}
		if(GetKeyState(VK_SPACE) & 0x80) 
			camera.pos.z += 0.04f * mod;
		if(GetKeyState(VK_LSHIFT) & 0x80) 
			camera.pos.z -= 0.04f * mod;
		Sleep(3);
	}
}

void draw_1(int p){
	BLOCK_NODE* node = &block_node;
	VEC3 pos = VEC3divR(camera.pos,MAP_SIZE);
	for(;;){
		BLOCK_NODE* node_zoom = node->block_node[(int)pos.z & 1][(int)pos.y & 1][(int)pos.x & 1];
		if(!node_zoom)
			break;
		node = node_zoom;
		VEC3mul(&pos,2.0f);
	}
	switch(p){
	case 0:
		for(int x = 0;x < WND_RESOLUTION.x / 2;x += RD_BLOCK)
			for(int y = 0;y < WND_RESOLUTION.y / 2;y += RD_BLOCK)
				drawPixel(x,y,node);
		break;
	case 1:
		for(int x = 0;x < WND_RESOLUTION.x / 2;x += RD_BLOCK)
			for(int y = WND_RESOLUTION.y;y >= WND_RESOLUTION.y / 2;y -= RD_BLOCK)
				drawPixel(x,y,node);
		break;
	case 2:
		for(int x = WND_RESOLUTION.x;x >= WND_RESOLUTION.x / 2;x -= RD_BLOCK)
			for(int y = 0;y < WND_RESOLUTION.y / 2;y += RD_BLOCK)
				drawPixel(x,y,node);
		break;
	case 3:
		for(int x = WND_RESOLUTION.x;x >= WND_RESOLUTION.x / 2;x -= RD_BLOCK)
			for(int y = WND_RESOLUTION.y;y >= WND_RESOLUTION.y / 2;y -= RD_BLOCK)
				drawPixel(x,y,node);
		break;
	}
}

void draw(){
	for(;;){
		unsigned long long t = __rdtsc();
		is_rendering = true;
		HANDLE draw_threads[4];
		draw_threads[0] = CreateThread(0,0,draw_1,(void*)0,0,0);
		draw_threads[1] = CreateThread(0,0,draw_1,(void*)1,0,0);
		draw_threads[2] = CreateThread(0,0,draw_1,(void*)2,0,0);
		draw_threads[3] = CreateThread(0,0,draw_1,(void*)3,0,0);
		WaitForMultipleObjects(4,draw_threads,TRUE,INFINITE);
		if(block_deleted.x != -1){

		}
		is_rendering = false;
		//printf("%i\n",__rdtsc() - t);
		int middle_pixel = WND_RESOLUTION.x * WND_RESOLUTION.y / 2 + WND_RESOLUTION.y / 2;
		vram.draw[middle_pixel] = (PIXEL){0,255,0};
		vram.draw[middle_pixel + 1] = (PIXEL){0,0,0};
		vram.draw[middle_pixel - 1] = (PIXEL){0,0,0};
		vram.draw[middle_pixel + (int)WND_RESOLUTION.y] = (PIXEL){0,0,0};
		vram.draw[middle_pixel - (int)WND_RESOLUTION.y] = (PIXEL){0,0,0};
		PIXEL* temp = vram.render;
		vram.render = vram.draw;
		vram.draw = temp;
		if(frame_ready)
			Sleep(1);
		frame_ready = TRUE;
	}	
}

void setVoxel(int x,int y,int z,int depth){
	BLOCK_NODE* node = &block_node;
	for(int i = 0;i < depth;i++){
		int shift = depth - i - 1;
		if(!node->block_node[z >> shift & 1][y >> shift & 1][x >> shift & 1])
			node->block_node[z >> shift & 1][y >> shift & 1][x >> shift & 1] = MALLOC_ZERO(sizeof(BLOCK_NODE));
		node = node->block_node[z >> shift & 1][y >> shift & 1][x >> shift & 1];
	}
	node->block = MALLOC_ZERO(sizeof(BLOCK));
}

void main(){
	bmi.bmiHeader.biWidth  = WND_RESOLUTION.y;
	bmi.bmiHeader.biHeight = WND_RESOLUTION.x;

	for(int i = 0;i < 1;i++)
		setVoxel(7,7,7,3);

	printf("%i\n",block_node.block_node_s[7]->block_node_s[7]->block_node_s[7]->block);

	vram.draw = MALLOC(sizeof(VRAM) * WND_RESOLUTION.x * WND_RESOLUTION.y);
	vram.render = MALLOC(sizeof(VRAM) * WND_RESOLUTION.x * WND_RESOLUTION.y);

	RegisterClassA(&wndclass);
	window = CreateWindowExA(0,"class","hello",WS_VISIBLE|WS_POPUP,0,0,WND_SIZE.x,WND_SIZE.y,0,0,wndclass.hInstance,0);
	context = GetDC(window);
	ShowCursor(FALSE);
	HANDLE thread_render = CreateThread(0,0,render,0,0,0);
	CreateThread(0,0,draw,0,0,0);
	CreateThread(0,0,physics,0,0,0);
	//CreateThread(0,0,lightmap,0,0,0);
	MSG msg;
	while(GetMessageA(&msg,window,0,0)){
		TranslateMessage(&msg);
 		DispatchMessageA(&msg);
	}
}