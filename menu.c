#include <stdbool.h>
#include <Windows.h>

#include "source.h"
#include "menu.h"

enum{
	BTN_CLOSE,
	BTN_SMOOTH_LIGHTING,
	BTN_FLY
};

typedef struct{
	ivec2 pos;
	int size;
}button_t;

uint menu_select;

button_t button[] = {
	{.pos = {WND_RESOLUTION_X / 2 + 60,WND_RESOLUTION_Y / 2 + 120},.size = 40},
	{.pos = {WND_RESOLUTION_X / 2 - 60,WND_RESOLUTION_Y / 2 - 120},.size = 40},
	{.pos = {WND_RESOLUTION_X / 2 - 20,WND_RESOLUTION_Y / 2 - 120},.size = 40}
};

uint buttonCount(){
	return sizeof(button) / sizeof(button_t);
}

bool aabb_button(button_t button,ivec2 cursor){
	bool x = cursor.x >= button.pos.x && cursor.x <= button.pos.x + button.size;
	bool y = cursor.y >= button.pos.y && cursor.y <= button.pos.y + button.size;
	if(x && y)
		return true;
	return false;
}

void executeButton(ivec2 cursor){
	for(int i = 0;i < buttonCount();i++){
		if(aabb_button(button[i],(ivec2){cursor.y,cursor.x})){
			switch(i){
			case BTN_CLOSE:
				ExitProcess(0);
			case BTN_SMOOTH_LIGHTING:
				setting_smooth_lighting ^= 1;
				break;
			case BTN_FLY:
				setting_fly ^= 1;
				break;
			}
			break;
		}
	}
}

void drawSquare(int x,int y,int size,pixel_t color){
	for(int i = x;i < x + size;i++){
		for(int j = y;j < y + size;j++){
			vram.draw[i * (int)WND_RESOLUTION.y + j] = color;
		}
	}
}

void drawButton(){
	for(int i = 0;i < buttonCount();i++){
		switch(i){
		case BTN_CLOSE:
			drawLine((vec2){button[i].pos.x,button[i].pos.y},vec2addR((vec2){button[i].pos.x,button[i].pos.y},button[i].size),(pixel_t){0,0,190});
			drawLine((vec2){button[i].pos.x + button[i].size,button[i].pos.y},(vec2){button[i].pos.x,button[i].pos.y + button[i].size},(pixel_t){0,0,190});
			break;
		case BTN_SMOOTH_LIGHTING:
			if(!setting_smooth_lighting){
				drawSquare(button[i].pos.x,button[i].pos.y,button[i].size / 2,(pixel_t){30,30,30});
				drawSquare(button[i].pos.x,button[i].pos.y + button[i].size / 2,button[i].size / 2,(pixel_t){80,80,80});
				drawSquare(button[i].pos.x + button[i].size / 2,button[i].pos.y,button[i].size / 2,(pixel_t){120,120,120});
				drawSquare(button[i].pos.x + button[i].size / 2,button[i].pos.y + button[i].size / 2,button[i].size / 2,(pixel_t){220,220,220});
				break;
			}
			pixel_t pixel[] = {
				{30,30,30},
				{80,80,80},
				{120,120,120},
				{220,220,220}
			};
			for(int x = 0;x < button[i].size;x++){
				for(int y = 0;y < button[i].size;y++){
					pixel_t color;
					uint size_squared = button[i].size * button[i].size;
					color.r = pixel[0].r * (button[i].size - x) * (button[i].size - y) / size_squared;
					color.g = pixel[0].g * (button[i].size - x) * (button[i].size - y) / size_squared;
					color.b = pixel[0].b * (button[i].size - x) * (button[i].size - y) / size_squared;
					color.r += pixel[1].r * (button[i].size - x) * y / size_squared;
					color.g += pixel[1].g * (button[i].size - x) * y / size_squared;
					color.b += pixel[1].b * (button[i].size - x) * y / size_squared;
					color.r += pixel[2].r * x * (button[i].size - y) / size_squared;
					color.g += pixel[2].g * x * (button[i].size - y) / size_squared;
					color.b += pixel[2].b * x * (button[i].size - y) / size_squared;
					color.r += pixel[3].r * x * y / size_squared;
					color.g += pixel[3].g * x * y / size_squared;
					color.b += pixel[3].b * x * y / size_squared;
					vram.draw[(x + button[i].pos.x) * (int)WND_RESOLUTION.y + (y + button[i].pos.y)] = color;
				}
			}
			break;
		}
	}
}