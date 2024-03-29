#pragma once

#include <stdint.h>

#define MV_GRAVITY 0.0007f
#define MV_JUMPHEIGHT 0.07f
#define MV_WALKSPEED 0.0015f
#define MV_FLYSPEED 0.1f

#define MV_DIAGONAL 0.5625f

#define MV_BHOP_BONUS 0.003f

#define GROUND_FRICTION 0.92f
#define AIR_FRICTION 0.997f

void physics(uint32_t tick_ammount);

extern float physic_mouse_x;