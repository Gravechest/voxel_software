#pragma once

#include <wtypes.h>

enum{
	SOUND_STEP,
	SOUND_JUMP,
	SOUND_MUSIC,
	SOUND_PUNCH
};

typedef struct{
	unsigned short *data;
	unsigned int sz;
}SOUNDFILE;

void initSound(HWND window);
void playSound(uint sound,vec3 pos);