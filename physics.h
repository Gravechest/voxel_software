#pragma once

#define MV_JUMPHEIGHT 0.1f
#define MV_WALKSPEED 0.0015f
#define MV_FLYSPEED 0.1f

#define MV_DIAGONAL 0.5625f

#define MV_BHOP_BONUS 0.003f

#define GROUND_FRICTION 0.92f
#define AIR_FRICTION 0.997f

void physics();

extern float physic_mouse_x;