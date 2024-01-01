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
}soundfile_t;

void initSound(HWND window);
void playSound(uint32_t sound,vec3_t pos);