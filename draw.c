#include <math.h>
#include <intrin.h>

#include "draw.h" 
#include "tmath.h"
#include "lighting.h"
#include "vec2.h"
#include "memory.h"

typedef struct{
	vec2 x;
	vec2 y;
}texture_pixel_angle_t;

void lightNewStackPut(block_node_t* node,block_t* block,uint side){
	if(light_new_stack_ptr >= LIGHT_NEW_STACK_SIZE - 1)
		return;

	light_new_stack_t* stack = &light_new_stack[light_new_stack_ptr++];
	stack->node = node;
	stack->side = side;

	uint size = GETLIGHTMAPSIZE(node->depth);
	block->side[side].luminance_p = MALLOC_ZERO(sizeof(vec3) * (size + 2) * (size + 2));
}

void drawSky16(int x,int y,vec3 ray_direction){
	int side;
	vec2 uv = sampleCube(ray_direction,&side);
	vec3 color;
	if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f)
		color = vec3mulR(LUMINANCE_SUN,256.0f);
	else
		color = vec3mulR(LUMINANCE_SKY,256.0f);
	vec3mul(&color,camera_exposure);
	pixel_t color_i = {
		color_i.r = tMin(color.r,0xff),
		color_i.g = tMin(color.g,0xff),
		color_i.b = tMin(color.b,0xff)
	};
	__m256i pixel = _mm256_set1_epi32(color_i.size);
	uint offset = x * (int)WND_RESOLUTION.y + y;
	for(int i = 0;i < 16;i++){
		_mm256_stream_si256(vram.draw + offset,pixel);
		_mm256_stream_si256(vram.draw + offset + 8,pixel);
		offset += (int)WND_RESOLUTION.y;
	}
}

void drawSky(int x,int y,int pixelblock_size,vec3 ray_direction){
	int side;
	vec2 uv = sampleCube(ray_direction,&side);
	vec3 color;
	if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f)
		color = vec3mulR(LUMINANCE_SUN,256.0f);
	else
		color = vec3mulR(LUMINANCE_SKY,256.0f);
	vec3mul(&color,camera_exposure);
	pixel_t color_i = {
		color_i.r = tMin(color.r,0xff),
		color_i.g = tMin(color.g,0xff),
		color_i.b = tMin(color.b,0xff)
	};
	for(int px = 0;px < pixelblock_size;px++){
		for(int py = 0;py < pixelblock_size;py++){
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = color_i;
		}
	}
}
