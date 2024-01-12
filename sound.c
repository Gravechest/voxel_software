#include <windows.h>
#include <dsound.h>

#include "source.h"
#include "sound.h"
#include "tmath.h"
#include "tree.h"
#include "memory.h"

#pragma comment(lib,"DSound.lib")

WAVEFORMATEX waveformat  = {WAVE_FORMAT_PCM,2,44100*2,44100*8,4,16,0};
WAVEFORMATEX waveformatS = {WAVE_FORMAT_PCM,2,44100,44100*4,4,16,0};

int outSelect;
int outSelectS;
HWAVEOUT soundOut;

soundfile_t sound_file[10];

LPDIRECTSOUND       directSoundH;
LPDIRECTSOUNDBUFFER directSoundBP[16];
LPDIRECTSOUNDBUFFER directSoundBS[16];
LPDIRECTSOUNDBUFFER directSoundBPS[16];
LPDIRECTSOUNDBUFFER directSoundBSS[16];

LPDIRECTSOUNDBUFFER	directSoundBP_long;
LPDIRECTSOUNDBUFFER	directSoundBS_long;

DSBUFFERDESC        bufferDescP;
DSBUFFERDESC        bufferDescS;
DSBUFFERDESC        bufferDescSS;

unsigned int sbufp1Sz;
unsigned int sbufp2Sz;
unsigned int sCursor;
unsigned int wCursor;

unsigned short *sbufp1;
unsigned short *sbufp2;

#define BUFFER_SIZE 200000

soundfile_t loadSoundFile(char *name){
	soundfile_t file;
	HANDLE h = CreateFileA(name,GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if(h!=-1){
		file.sz = GetFileSize(h,0);
		file.data = HeapAlloc(GetProcessHeap(),8,file.sz+1);
		ReadFile(h,file.data,file.sz+1,0,0);
	}
	CloseHandle(h);
	return file;
}

void initSound(HWND window){
	sound_file[SOUND_STEP] = loadSoundFile("sfx/concrete_step.wav");
	sound_file[SOUND_JUMP] = loadSoundFile("sfx/jump.wav");
	//sound_file[2] = loadSoundFile("sfx/gamemusic2.wav");
	sound_file[SOUND_PUNCH] = loadSoundFile("sfx/punch.wav");
	DirectSoundCreate(0,&directSoundH,0);
	directSoundH->lpVtbl->SetCooperativeLevel(directSoundH,window,DSSCL_PRIORITY);
	bufferDescP.dwFlags = DSBCAPS_PRIMARYBUFFER;
	bufferDescP.dwSize  = sizeof(DSBUFFERDESC);

	for(int i = 0;i < 16;i++){
		directSoundH->lpVtbl->CreateSoundBuffer(directSoundH,&bufferDescP,&directSoundBPS[i],0);
		directSoundBPS[i]->lpVtbl->SetFormat(directSoundBPS[i],&waveformat);
	}

	for(int i = 0;i < 16;i++){
		directSoundH->lpVtbl->CreateSoundBuffer(directSoundH,&bufferDescP,&directSoundBP[i],0);
		directSoundBP[i]->lpVtbl->SetFormat(directSoundBP[i],&waveformatS);
	}

	directSoundH->lpVtbl->CreateSoundBuffer(directSoundH,&bufferDescP,&directSoundBP_long,0);

	bufferDescS.dwFlags = DSBCAPS_CTRLVOLUME;
	bufferDescS.dwSize  = sizeof(DSBUFFERDESC);
	bufferDescS.dwBufferBytes = BUFFER_SIZE;
	bufferDescS.lpwfxFormat = &waveformat;

	bufferDescSS.dwFlags = DSBCAPS_CTRLVOLUME;
	bufferDescSS.dwSize  = sizeof(DSBUFFERDESC);
	bufferDescSS.dwBufferBytes = BUFFER_SIZE;
	bufferDescSS.lpwfxFormat = &waveformatS;

	for(int i = 0;i < 16;i++)
		directSoundH->lpVtbl->CreateSoundBuffer(directSoundH,&bufferDescSS,&directSoundBSS[i],0);

	for(int i = 0;i < 16;i++)
		directSoundH->lpVtbl->CreateSoundBuffer(directSoundH,&bufferDescS,&directSoundBS[i],0);

	directSoundH->lpVtbl->CreateSoundBuffer(directSoundH,&bufferDescSS,&directSoundBS_long,0);
}

#define BOUNCE_AMMOUNT 4
#define BOUNCE_QUALITY 16

uint32_t sound_length;

typedef struct{
	float dst;
	ivec3 normal;
	float direction;
}prop_t;

prop_t getRayInfo(traverse_init_t init,vec3_t angle,vec3_t pos){
	node_hit_t hit = treeRay(ray3Create(init.pos,angle),init.node,pos);
	if(!hit.node)
		return (prop_t){-1.0f};
	node_t* node = dynamicArrayGet(node_root,hit.node);
	vec3_t block_pos;
	float block_size = (float)MAP_SIZE / (1 << node->depth) * 2.0f;
	block_pos.x = node->pos.x * block_size;
	block_pos.y = node->pos.y * block_size;
	block_pos.z = node->pos.z * block_size;
	plane_ray_t plane = getPlane(block_pos,angle,hit.side,block_size);
	vec3_t plane_pos = vec3subvec3R(plane.pos,pos);
	return (prop_t){tAbsf(rayIntersectPlane(plane_pos,angle,plane.normal)),(ivec3){plane.x,plane.y,plane.z},plane.normal.a[plane.z]};
}
#include <math.h>
void soundRay(soundfile_t sound,float* f_buffer,vec3_t pos,ivec3 normal,float direction,uint32_t depth,float volume,int delay){
	if(depth > 2)
		return;
	if(delay + sound.sz / 2 - 1000 >= BUFFER_SIZE / 2)
		return;
	traverse_init_t init = initTraverse(pos);
	vec3_t to_player_direction = vec3normalizeR(vec3subvec3R(camera.pos,pos));
	float to_player = getRayInfo(init,to_player_direction,pos).dst;
	float sound_direction = atan2f(to_player_direction.x,to_player_direction.y) / M_PI;
	sound_direction += camera.dir.x / M_PI * 2.0f + 1.0f;
	sound_direction = tFract((sound_direction + 1.0f) * 0.5f) * 2.0f - 1.0f;
	sound_direction *= 1.0f - to_player_direction.z;
	vec3_t perpendicular = getLookAngle((vec2_t){camera.dir.x + M_PI * 0.25f,camera.dir.y});
	vec3mul(&perpendicular,5.0f);
	float left_distance = vec3distance(pos,vec3subvec3R(camera.pos,perpendicular));
	float right_distance = vec3distance(pos,vec3addvec3R(camera.pos,perpendicular));

	float left_volume = 1.0f / sqrtf((left_distance + 1.0f + delay / 250.0f)) * volume;
	float right_volume = 1.0f / sqrtf((right_distance + 1.0f + delay / 250.0f)) * volume;

	int left = (int)(-sound_direction * 2000.0f) << 1;
	int right = (int)(sound_direction * 2000.0f) << 1;

	left  = tMax(left,0);
	right = tMax(right,0);

	if(to_player == -1.0f || to_player > vec3distance(camera.pos,pos)){
		int sample_delay = delay + vec3distance(camera.pos,pos) * 250.0f;
		sample_delay &= ~1;

		for(int i = 0;i < sound.sz / 2 - 1000;i += 2)
			f_buffer[i + sample_delay + left] += (short)sound.data[i + 30] * left_volume;

		for(int i = 0;i < sound.sz / 2 - 1000;i += 2)
			f_buffer[i + sample_delay + right + 1] += (short)sound.data[i + 30] * right_volume;
	}
	sound_length = tMax(delay + sound.sz / 2 - 1000,sound_length);
	if(depth != 0){
		for(float i = -1.5f;i <= 1.5f;i += 1.0f){
			for(float j = -1.5f;j <= 1.5f;j += 1.0f){
				vec3_t angle;
				angle.a[normal.z] = direction;
				angle.a[normal.x] = i;
				angle.a[normal.y] = j;
				prop_t prop = getRayInfo(init,angle,pos);
				if(prop.dst <= 0.0f)
					continue;
				int sample_delay = delay + prop.dst * 250.0f;
				vec3_t pos_n = vec3addvec3R(pos,vec3mulR(vec3normalizeR(angle),prop.dst - 0.001f));
				soundRay(sound,f_buffer,pos_n,prop.normal,prop.direction,depth + 1,volume / BOUNCE_AMMOUNT * 1.0f,sample_delay);
			}
		}
		return;
	}
	for(int i = 0;i < 6;i++){
		normal = (ivec3[]){{VEC3_Y,VEC3_Z,VEC3_X},{VEC3_X,VEC3_Z,VEC3_Y},{VEC3_X,VEC3_Y,VEC3_Z}}[i >> 1];
		direction = i & 1 ? -1.0f : 1.0f;
		for(float i = -1.0f;i <= 1.0f;i += 2.0f){
			for(float j = -1.0f;j <= 1.0f;j += 2.0f){
				vec3_t angle;
				angle.a[normal.z] = direction;
				angle.a[normal.x] = i;
				angle.a[normal.y] = j;
				prop_t prop = getRayInfo(init,angle,pos);
				if(prop.dst <= 0.0f)
					continue;
				int sample_delay = delay + prop.dst * 250.0f;
				vec3_t pos_n = vec3addvec3R(pos,vec3mulR(vec3normalizeR(angle),prop.dst - 0.001f));
				soundRay(sound,f_buffer,pos_n,prop.normal,prop.direction,depth + 1,volume / BOUNCE_AMMOUNT * 1.0f,sample_delay);
			}
		}
	}
}

void playSound(uint32_t sound,vec3_t pos){
	return;
	soundfile_t file = sound_file[sound];
	if(!file.sz)
		return;
	float* f_buffer = tMallocZero(BUFFER_SIZE * sizeof(float));
	soundRay(file,f_buffer,pos,(ivec3){VEC3_X,VEC3_Y,VEC3_Z},0.0f,0,1.0f / BOUNCE_AMMOUNT,0);
	directSoundBS_long->lpVtbl->Lock(directSoundBS_long,0,0,&sbufp1,&sbufp1Sz,&sbufp2,&sbufp2Sz,DSBLOCK_ENTIREBUFFER);
	for(int i = 0;i < BUFFER_SIZE / 4;i++)
		sbufp1[i] = f_buffer[i] + sbufp1[i + file.sz];
	sound_length = 0;
	tFree(f_buffer);
	directSoundBS_long->lpVtbl->Unlock(directSoundBS_long,sbufp1,sbufp1Sz,sbufp2,sbufp2Sz);
	directSoundBS_long->lpVtbl->SetCurrentPosition(directSoundBS_long,0);
	directSoundBS_long->lpVtbl->Play(directSoundBS_long,0,0,0);
}