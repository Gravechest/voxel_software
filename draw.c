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

void textureSingleLuminanceSIMD128(material_t material,vec3 luminance,vec2 fract,texture_pixel_angle_t angle,float texture_size,int x,int y,int offset,float fog){	
	__m128 mulx,muly;

	__m128 texture_size_simd = _mm_set1_ps(texture_size);
	__m128 luminance_red     = _mm_set1_ps(luminance.r);
	__m128 luminance_green   = _mm_set1_ps(luminance.g);
	__m128 luminance_blue    = _mm_set1_ps(luminance.b);

	__m128 fog_wide = _mm_broadcast_ss(&fog);
	__m128 fog_inv  = _mm_set1_ps(1.0f - fog);
	__m128 sky_red  = _mm_mul_ps(_mm_set1_ps((LUMINANCE_SKY.r * 256.0f) * camera_exposure),fog_wide);
	__m128 sky_green= _mm_mul_ps(_mm_set1_ps((LUMINANCE_SKY.g * 256.0f) * camera_exposure),fog_wide);
	__m128 sky_blue = _mm_mul_ps(_mm_set1_ps((LUMINANCE_SKY.b * 256.0f) * camera_exposure),fog_wide);

	__m128 mul  = _mm_set_ps(3.0f,2.0f,1.0f,0.0f);

	mulx = _mm_broadcast_ss(&angle.x.x);
	muly = _mm_broadcast_ss(&angle.x.y);

	mulx = _mm_mul_ps(mulx,mul);
	muly = _mm_mul_ps(muly,mul);

	for(int px = 0;px < 4;px++){
		__m128i texture_i[2];
		__m128 texture_x = _mm_broadcast_ss(&fract.x);
		__m128 texture_y = _mm_broadcast_ss(&fract.y);
		texture_x = _mm_add_ps(texture_x,mulx);
		texture_y = _mm_add_ps(texture_y,muly);
		texture_i[0] = _mm_cvtps_epi32(texture_x);
		texture_i[1] = _mm_cvtps_epi32(texture_y);

		texture_x = _mm_cvtepi32_ps(texture_i[0]);
		texture_y = _mm_cvtepi32_ps(texture_i[1]);
		texture_x = _mm_mul_ps(texture_x,texture_size_simd);
		texture_x = _mm_add_ps(texture_x,texture_y);

		texture_i[0] = _mm_cvtps_epi32(texture_x);

		int texture_red[4];
		int texture_green[4];
		int texture_blue[4];

		for(int i = 0;i < 4;i++){
			pixel_t texture_color = material.texture.data[texture_i[0].m128i_i32[i] + offset];
			texture_red[i] = texture_color.r;
			texture_green[i] = texture_color.g;
			texture_blue[i] = texture_color.b;
		}

		__m128i texture_color_red_i   = _mm_load_si128(&texture_red);
		__m128i texture_color_green_i = _mm_load_si128(&texture_green);
		__m128i texture_color_blue_i  = _mm_load_si128(&texture_blue);

		__m128 texture_color_red   = _mm_cvtepi32_ps(texture_color_red_i);
		__m128 texture_color_green = _mm_cvtepi32_ps(texture_color_green_i);
		__m128 texture_color_blue  = _mm_cvtepi32_ps(texture_color_blue_i);

		texture_color_red   = _mm_mul_ps(texture_color_red,luminance_red);
		texture_color_green = _mm_mul_ps(texture_color_green,luminance_green);
		texture_color_blue  = _mm_mul_ps(texture_color_blue,luminance_blue);

		texture_color_red = _mm_add_ps(_mm_mul_ps(texture_color_red,fog_inv),sky_red);
		texture_color_green = _mm_add_ps(_mm_mul_ps(texture_color_green,fog_inv),sky_green);
		texture_color_blue = _mm_add_ps(_mm_mul_ps(texture_color_blue,fog_inv),sky_blue);
			
		texture_color_red   = _mm_min_ps(texture_color_red,_mm_set1_ps(255.0f));
		texture_color_green = _mm_min_ps(texture_color_green,_mm_set1_ps(255.0f));
		texture_color_blue  = _mm_min_ps(texture_color_blue,_mm_set1_ps(255.0f));

		texture_color_green_i = _mm_cvtps_epi32(texture_color_green);
		texture_color_green   = _mm_cvtepi32_ps(texture_color_green_i);
		texture_color_green   = _mm_mul_ps(texture_color_green,_mm_set1_ps(256.0f));
					
		texture_color_blue_i = _mm_cvtps_epi32(texture_color_blue);
		texture_color_blue   = _mm_cvtepi32_ps(texture_color_blue_i);
		texture_color_blue   = _mm_mul_ps(texture_color_blue,_mm_set1_ps(256.0f * 256.0f));

		texture_color_red = _mm_add_ps(texture_color_red,texture_color_green);
		texture_color_red = _mm_add_ps(texture_color_red,texture_color_blue);

		texture_color_red_i = _mm_cvtps_epi32(texture_color_red);

		_mm_stream_si128(&vram.draw[(x + px) * (int)WND_RESOLUTION.y + y],texture_color_red_i);
		fract.x += angle.y.x;
		fract.y += angle.y.y;
	}
}

void drawSIMD(vec3 color,vec3 luminance[4],int x,int y,float fog){	
	__m256 luminance_red[4];
	__m256 luminance_green[4];
	__m256 luminance_blue[4];

	for(int j = 0;j < 4;j++){
		luminance_red[j]     = _mm256_set1_ps(luminance[j].r);
		luminance_green[j]   = _mm256_set1_ps(luminance[j].g);
		luminance_blue[j]    = _mm256_set1_ps(luminance[j].b);
	}

	__m256 red     = _mm256_set1_ps(color.r);
	__m256 green   = _mm256_set1_ps(color.g);
	__m256 blue    = _mm256_set1_ps(color.b);

	__m256 add_y[4] = {
		_mm256_setr_ps(1.0f,0.9333f,0.8666f,0.8f,0.7333f,0.6666f,0.6f,0.5333f),
		_mm256_setr_ps(0.4666f,0.4f,0.3333f,0.2666f,0.2f,0.1333f,0.0666f,0.0f),
		_mm256_setr_ps(0.0f,0.0666f,0.1333f,0.2f,0.2666f,0.3333f,0.4f,0.4666f),
		_mm256_setr_ps(0.5333f,0.6f,0.6666f,0.7333f,0.8f,0.8666f,0.9333f,1.0f)
	};

	__m256 mul  = _mm256_set_ps(7.0f,6.0f,5.0f,4.0f,3.0f,2.0f,1.0f,0.0f);
	__m256 mul2 = _mm256_set_ps(15.0f,14.0f,13.0f,12.0f,11.0f,10.0f,9.0f,8.0f);

	__m256 fog_wide = _mm256_broadcast_ss(&fog);
	__m256 fog_inv  = _mm256_set1_ps(1.0f - fog);
	__m256 sky_red  = _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.r * 256.0f) * camera_exposure),fog_wide);
	__m256 sky_green= _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.g * 256.0f) * camera_exposure),fog_wide);
	__m256 sky_blue = _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.b * 256.0f) * camera_exposure),fog_wide);

	for(int px = 0;px < 16;px++){
		__m256 interpolate = _mm256_set1_ps(1.0f - (float)px / (16 - 1));
		__m256 interpolate_inv = _mm256_set1_ps((float)px / (16 - 1));
		for(int j = 0;j < 2;j++){
			__m256i texture_color_red_i; 
			__m256i texture_color_green_i;
			__m256i texture_color_blue_i;

			__m256 luminance_1 = _mm256_mul_ps(interpolate,add_y[j]);
			__m256 luminance_2 = _mm256_mul_ps(interpolate_inv,add_y[j]);
			__m256 luminance_3 = _mm256_mul_ps(interpolate,add_y[j + 2]);
			__m256 luminance_4 = _mm256_mul_ps(interpolate_inv,add_y[j + 2]);

			__m256 luminance_combined_red = _mm256_mul_ps(luminance_red[0],luminance_1);
			__m256 luminance_combined_green = _mm256_mul_ps(luminance_green[0],luminance_1);
			__m256 luminance_combined_blue = _mm256_mul_ps(luminance_blue[0],luminance_1);

			luminance_combined_red = _mm256_add_ps(luminance_combined_red ,_mm256_mul_ps(luminance_red[1],luminance_2));
			luminance_combined_green = _mm256_add_ps(luminance_combined_green ,_mm256_mul_ps(luminance_green[1],luminance_2));
			luminance_combined_blue = _mm256_add_ps(luminance_combined_blue ,_mm256_mul_ps(luminance_blue[1],luminance_2));

			luminance_combined_red = _mm256_add_ps(luminance_combined_red ,_mm256_mul_ps(luminance_red[2],luminance_3));
			luminance_combined_green = _mm256_add_ps(luminance_combined_green ,_mm256_mul_ps(luminance_green[2],luminance_3));
			luminance_combined_blue = _mm256_add_ps(luminance_combined_blue ,_mm256_mul_ps(luminance_blue[2],luminance_3));

			luminance_combined_red = _mm256_add_ps(luminance_combined_red ,_mm256_mul_ps(luminance_red[3],luminance_4));
			luminance_combined_green = _mm256_add_ps(luminance_combined_green ,_mm256_mul_ps(luminance_green[3],luminance_4));
			luminance_combined_blue = _mm256_add_ps(luminance_combined_blue ,_mm256_mul_ps(luminance_blue[3],luminance_4));

			__m256 texture_color_red   = _mm256_mul_ps(red,luminance_combined_red);
			__m256 texture_color_green = _mm256_mul_ps(green,luminance_combined_green);
			__m256 texture_color_blue  = _mm256_mul_ps(blue,luminance_combined_blue);

			texture_color_red = _mm256_add_ps(_mm256_mul_ps(texture_color_red,fog_inv),sky_red);
			texture_color_green = _mm256_add_ps(_mm256_mul_ps(texture_color_green,fog_inv),sky_green);
			texture_color_blue = _mm256_add_ps(_mm256_mul_ps(texture_color_blue,fog_inv),sky_blue);
			
			texture_color_red   = _mm256_min_ps(texture_color_red,_mm256_set1_ps(255.0f));
			texture_color_green = _mm256_min_ps(texture_color_green,_mm256_set1_ps(255.0f));
			texture_color_blue  = _mm256_min_ps(texture_color_blue,_mm256_set1_ps(255.0f));

			texture_color_green_i = _mm256_cvtps_epi32(texture_color_green);
			texture_color_green   = _mm256_cvtepi32_ps(texture_color_green_i);
			texture_color_green   = _mm256_mul_ps(texture_color_green,_mm256_set1_ps(256.0f));
					
			texture_color_blue_i = _mm256_cvtps_epi32(texture_color_blue);
			texture_color_blue   = _mm256_cvtepi32_ps(texture_color_blue_i);
			texture_color_blue   = _mm256_mul_ps(texture_color_blue,_mm256_set1_ps(256.0f * 256.0f));

			texture_color_red = _mm256_add_ps(texture_color_red,texture_color_green);
			texture_color_red = _mm256_add_ps(texture_color_red,texture_color_blue);

			texture_color_red_i = _mm256_cvtps_epi32(texture_color_red);

			_mm256_stream_si256(&vram.draw[(x + px) * (int)WND_RESOLUTION.y + y + j * 8],texture_color_red_i);
		}
	}
}

void textureDrawSIMD8(material_t material,vec3 luminance,vec2 fract,texture_pixel_angle_t angle,float texture_size,int x,int y,int offset,float fog){	
	__m256 mulx,muly;

	__m256 texture_size_simd = _mm256_set1_ps(texture_size);
	__m256 luminance_red = _mm256_set1_ps(luminance.r);
	__m256 luminance_green = _mm256_set1_ps(luminance.g);
	__m256 luminance_blue = _mm256_set1_ps(luminance.b);

	__m256 mul  = _mm256_set_ps(7.0f,6.0f,5.0f,4.0f,3.0f,2.0f,1.0f,0.0f);

	mulx = _mm256_broadcast_ss(&angle.x.x);
	muly = _mm256_broadcast_ss(&angle.x.y);

	mulx = _mm256_mul_ps(mulx,mul);
	muly = _mm256_mul_ps(muly,mul);

	__m256 fog_wide = _mm256_broadcast_ss(&fog);
	__m256 fog_inv  = _mm256_set1_ps(1.0f - fog);
	__m256 sky_red  = _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.r * 256.0f) * camera_exposure),fog_wide);
	__m256 sky_green= _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.g * 256.0f) * camera_exposure),fog_wide);
	__m256 sky_blue = _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.b * 256.0f) * camera_exposure),fog_wide);

	for(int px = 0;px < 8;px++){
		__m256i texture_i[2];
		__m256 texture_x = _mm256_broadcast_ss(&fract.x);
		__m256 texture_y = _mm256_broadcast_ss(&fract.y);
		texture_x = _mm256_add_ps(texture_x,mulx);
		texture_y = _mm256_add_ps(texture_y,muly);
		texture_i[0] = _mm256_cvtps_epi32(texture_x);
		texture_i[1] = _mm256_cvtps_epi32(texture_y);

		texture_x = _mm256_cvtepi32_ps(texture_i[0]);
		texture_y = _mm256_cvtepi32_ps(texture_i[1]);
		texture_x = _mm256_mul_ps(texture_x,texture_size_simd);
		texture_x = _mm256_add_ps(texture_x,texture_y);

		texture_i[0] = _mm256_cvtps_epi32(texture_x);

		__m256i texture_color_red_i; 
		__m256i texture_color_green_i;
		__m256i texture_color_blue_i;

		for(int i = 0;i < 8;i++){
			texture_color_red_i.m256i_i32[i] = material.texture.data[texture_i[0].m256i_i32[i] + offset].r;
			texture_color_green_i.m256i_i32[i] = material.texture.data[texture_i[0].m256i_i32[i] + offset].g;
			texture_color_blue_i.m256i_i32[i]  = material.texture.data[texture_i[0].m256i_i32[i] + offset].b;
		};

		__m256 texture_color_red   = _mm256_mul_ps(_mm256_cvtepi32_ps(texture_color_red_i),luminance_red);
		__m256 texture_color_green = _mm256_mul_ps(_mm256_cvtepi32_ps(texture_color_green_i),luminance_green);
		__m256 texture_color_blue  = _mm256_mul_ps(_mm256_cvtepi32_ps(texture_color_blue_i),luminance_blue);

		texture_color_red = _mm256_add_ps(_mm256_mul_ps(texture_color_red,fog_inv),sky_red);
		texture_color_green = _mm256_add_ps(_mm256_mul_ps(texture_color_green,fog_inv),sky_green);
		texture_color_blue = _mm256_add_ps(_mm256_mul_ps(texture_color_blue,fog_inv),sky_blue);
			
		texture_color_red   = _mm256_min_ps(texture_color_red,_mm256_set1_ps(255.0f));
		texture_color_green = _mm256_min_ps(texture_color_green,_mm256_set1_ps(255.0f));
		texture_color_blue  = _mm256_min_ps(texture_color_blue,_mm256_set1_ps(255.0f));

		texture_color_green_i = _mm256_cvtps_epi32(texture_color_green);
		texture_color_green   = _mm256_cvtepi32_ps(texture_color_green_i);
		texture_color_green   = _mm256_mul_ps(texture_color_green,_mm256_set1_ps(256.0f));
					
		texture_color_blue_i = _mm256_cvtps_epi32(texture_color_blue);
		texture_color_blue   = _mm256_cvtepi32_ps(texture_color_blue_i);
		texture_color_blue   = _mm256_mul_ps(texture_color_blue,_mm256_set1_ps(256.0f * 256.0f));

		texture_color_red = _mm256_add_ps(texture_color_red,texture_color_green);
		texture_color_red = _mm256_add_ps(texture_color_red,texture_color_blue);

		texture_color_red_i = _mm256_cvtps_epi32(texture_color_red);

		_mm256_stream_si256(&vram.draw[(x + px) * (int)WND_RESOLUTION.y + y],texture_color_red_i);
		fract.x += angle.y.x;
		fract.y += angle.y.y;
	}
}

void textureDrawSIMD(material_t material,vec3 luminance[4],vec2 fract,texture_pixel_angle_t angle,float texture_size,int x,int y,int offset,float fog){	
	__m256 mulx[2],muly[2];

	__m256 texture_size_simd = _mm256_set1_ps(texture_size);
	__m256 luminance_red[4];
	__m256 luminance_green[4];
	__m256 luminance_blue[4];

	for(int j = 0;j < 4;j++){
		luminance_red[j]     = _mm256_set1_ps(luminance[j].r);
		luminance_green[j]   = _mm256_set1_ps(luminance[j].g);
		luminance_blue[j]    = _mm256_set1_ps(luminance[j].b);
	}

	__m256 add_y[4] = {
		_mm256_setr_ps(1.0f,0.9333f,0.8666f,0.8f,0.7333f,0.6666f,0.6f,0.5333f),
		_mm256_setr_ps(0.4666f,0.4f,0.3333f,0.2666f,0.2f,0.1333f,0.0666f,0.0f),
		_mm256_setr_ps(0.0f,0.0666f,0.1333f,0.2f,0.2666f,0.3333f,0.4f,0.4666f),
		_mm256_setr_ps(0.5333f,0.6f,0.6666f,0.7333f,0.8f,0.8666f,0.9333f,1.0f)
	};

	__m256 mul  = _mm256_set_ps(7.0f,6.0f,5.0f,4.0f,3.0f,2.0f,1.0f,0.0f);
	__m256 mul2 = _mm256_set_ps(15.0f,14.0f,13.0f,12.0f,11.0f,10.0f,9.0f,8.0f);

	mulx[0] = _mm256_broadcast_ss(&angle.x.x);
	muly[0] = _mm256_broadcast_ss(&angle.x.y);
	mulx[1] = _mm256_broadcast_ss(&angle.x.x);
	muly[1] = _mm256_broadcast_ss(&angle.x.y);

	mulx[0] = _mm256_mul_ps(mulx[0],mul);
	muly[0] = _mm256_mul_ps(muly[0],mul);
	mulx[1] = _mm256_mul_ps(mulx[1],mul2);
	muly[1] = _mm256_mul_ps(muly[1],mul2);

	__m256 fog_wide = _mm256_broadcast_ss(&fog);
	__m256 fog_inv  = _mm256_set1_ps(1.0f - fog);
	__m256 sky_red  = _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.r * 256.0f) * camera_exposure),fog_wide);
	__m256 sky_green= _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.g * 256.0f) * camera_exposure),fog_wide);
	__m256 sky_blue = _mm256_mul_ps(_mm256_set1_ps((LUMINANCE_SKY.b * 256.0f) * camera_exposure),fog_wide);

	for(int px = 0;px < 16;px++){
		for(int j = 0;j < 2;j++){
			__m256i texture_i[2];
			__m256 texture_x = _mm256_broadcast_ss(&fract.x);
			__m256 texture_y = _mm256_broadcast_ss(&fract.y);
			texture_x = _mm256_add_ps(texture_x,mulx[j]);
			texture_y = _mm256_add_ps(texture_y,muly[j]);
			texture_i[0] = _mm256_cvtps_epi32(texture_x);
			texture_i[1] = _mm256_cvtps_epi32(texture_y);

			texture_x = _mm256_cvtepi32_ps(texture_i[0]);
			texture_y = _mm256_cvtepi32_ps(texture_i[1]);
			texture_x = _mm256_mul_ps(texture_x,texture_size_simd);
			texture_x = _mm256_add_ps(texture_x,texture_y);

			texture_i[0] = _mm256_cvtps_epi32(texture_x);

			__m256i texture_color_red_i; 
			__m256i texture_color_green_i;
			__m256i texture_color_blue_i;

			for(int i = 0;i < 8;i++){
				texture_color_red_i.m256i_i32[i] = material.texture.data[texture_i[0].m256i_i32[i] + offset].r;
				texture_color_green_i.m256i_i32[i] = material.texture.data[texture_i[0].m256i_i32[i] + offset].g;
				texture_color_blue_i.m256i_i32[i]  = material.texture.data[texture_i[0].m256i_i32[i] + offset].b;
			};

			__m256 texture_color_red   = _mm256_cvtepi32_ps(texture_color_red_i);
			__m256 texture_color_green = _mm256_cvtepi32_ps(texture_color_green_i);
			__m256 texture_color_blue  = _mm256_cvtepi32_ps(texture_color_blue_i);

			__m256 luminance_1 = _mm256_mul_ps(_mm256_set1_ps(1.0f - (float)px / (16 - 1)),add_y[j]);
			__m256 luminance_2 = _mm256_mul_ps(_mm256_set1_ps((float)px / (16 - 1)),add_y[j]);
			__m256 luminance_3 = _mm256_mul_ps(_mm256_set1_ps(1.0f - (float)px / (16 - 1)),add_y[j + 2]);
			__m256 luminance_4 = _mm256_mul_ps(_mm256_set1_ps((float)px / (16 - 1)),add_y[j + 2]);

			__m256 luminance_combined_red = _mm256_mul_ps(luminance_red[0],luminance_1);
			__m256 luminance_combined_green = _mm256_mul_ps(luminance_green[0],luminance_1);
			__m256 luminance_combined_blue = _mm256_mul_ps(luminance_blue[0],luminance_1);

			luminance_combined_red = _mm256_add_ps(luminance_combined_red ,_mm256_mul_ps(luminance_red[1],luminance_2));
			luminance_combined_green = _mm256_add_ps(luminance_combined_green ,_mm256_mul_ps(luminance_green[1],luminance_2));
			luminance_combined_blue = _mm256_add_ps(luminance_combined_blue ,_mm256_mul_ps(luminance_blue[1],luminance_2));

			luminance_combined_red = _mm256_add_ps(luminance_combined_red ,_mm256_mul_ps(luminance_red[2],luminance_3));
			luminance_combined_green = _mm256_add_ps(luminance_combined_green ,_mm256_mul_ps(luminance_green[2],luminance_3));
			luminance_combined_blue = _mm256_add_ps(luminance_combined_blue ,_mm256_mul_ps(luminance_blue[2],luminance_3));

			luminance_combined_red = _mm256_add_ps(luminance_combined_red ,_mm256_mul_ps(luminance_red[3],luminance_4));
			luminance_combined_green = _mm256_add_ps(luminance_combined_green ,_mm256_mul_ps(luminance_green[3],luminance_4));
			luminance_combined_blue = _mm256_add_ps(luminance_combined_blue ,_mm256_mul_ps(luminance_blue[3],luminance_4));

			texture_color_red   = _mm256_mul_ps(texture_color_red,luminance_combined_red);
			texture_color_green = _mm256_mul_ps(texture_color_green,luminance_combined_green);
			texture_color_blue  = _mm256_mul_ps(texture_color_blue,luminance_combined_blue);

			texture_color_red = _mm256_add_ps(_mm256_mul_ps(texture_color_red,fog_inv),sky_red);
			texture_color_green = _mm256_add_ps(_mm256_mul_ps(texture_color_green,fog_inv),sky_green);
			texture_color_blue = _mm256_add_ps(_mm256_mul_ps(texture_color_blue,fog_inv),sky_blue);
			
			texture_color_red   = _mm256_min_ps(texture_color_red,_mm256_set1_ps(255.0f));
			texture_color_green = _mm256_min_ps(texture_color_green,_mm256_set1_ps(255.0f));
			texture_color_blue  = _mm256_min_ps(texture_color_blue,_mm256_set1_ps(255.0f));

			texture_color_green_i = _mm256_cvtps_epi32(texture_color_green);
			texture_color_green   = _mm256_cvtepi32_ps(texture_color_green_i);
			texture_color_green   = _mm256_mul_ps(texture_color_green,_mm256_set1_ps(256.0f));
					
			texture_color_blue_i = _mm256_cvtps_epi32(texture_color_blue);
			texture_color_blue   = _mm256_cvtepi32_ps(texture_color_blue_i);
			texture_color_blue   = _mm256_mul_ps(texture_color_blue,_mm256_set1_ps(256.0f * 256.0f));

			texture_color_red = _mm256_add_ps(texture_color_red,texture_color_green);
			texture_color_red = _mm256_add_ps(texture_color_red,texture_color_blue);

			texture_color_red_i = _mm256_cvtps_epi32(texture_color_red);

			_mm256_stream_si256(&vram.draw[(x + px) * (int)WND_RESOLUTION.y + y + j * 8],texture_color_red_i);
		}
		fract.x += angle.y.x;
		fract.y += angle.y.y;
	}
}

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

void drawSurfacePixel(block_node_t node,vec3 ray_direction,vec3 camera_pos,int axis,int x,int y){
	block_t* block = node.block;
	int side = axis * 2 + (ray_direction.a[axis] < 0.0f);
	if(!block)
		return;
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT){
		vec3 luminance = vec3mulR(material.luminance,camera_exposure);
		pixel_t color = {
			tMin(luminance.r * 255.0f,255),
			tMin(luminance.g * 255.0f,255),
			tMin(luminance.b * 255.0f,255)
		};
		vram.draw[x * (int)WND_RESOLUTION.y + y] = color;
		return;
	}
	vec3 block_pos = {node.pos.x,node.pos.y,node.pos.z};
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;

	vec3mul(&block_pos,block_size);

	plane_t plane = getPlane(block_pos,ray_direction,axis,block_size);
	float distance = ((plane.pos.a[plane.z] - camera_pos.a[plane.z])) / ray_direction.a[plane.z];
	float fog = distance;
	fog *= FOG_DISTANCE;
	if(fog > 1.0f){
		drawSky(x,y,1,ray_direction);
		return;
	}

	vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(ray_direction,distance));
	vec3 color;
	vec2 uv = {
		(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
		(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
	};
	uv.x = tMaxf(tMinf(uv.x,1.0f),0.0f);
	uv.y = tMaxf(tMinf(uv.y,1.0f),0.0f);

	vec3 luminance;
	if(setting_smooth_lighting)
		luminance = getLuminance(block->side[side],uv,node.depth);
	else{
		if(block->side[side].luminance_p){
			uint lm_size = tMax(1 << LM_MAXDEPTH - node.depth,1);
			uint location = (int)(uv.x * lm_size + 1) * (lm_size + 2) + (int)(uv.y * lm_size + 1);
			luminance = block->side[side].luminance_p[location];
		}
	}

	vec3mul(&luminance,camera_exposure);

	if(!material.texture.data){
		color = vec3mulR(material.luminance,255.0f);
		vec3mulvec3(&color,luminance);
		color = vec3mixR(color,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
		pixel_t pixel = {tMin(color.r,0xff),tMin(color.g,0xff),tMin(color.b,0xff)};
		vram.draw[x * (int)WND_RESOLUTION.y + y] = pixel;
		return;
	}

	vec2 texture_uv = {
		hit_pos.a[plane.x] / block_size,
		hit_pos.a[plane.y] / block_size
	};
	uint mipmap_level = distance / (tAbsf(ray_direction.a[axis]) * MIPMAP_AGGRESION);
	_BitScanReverse(&mipmap_level,mipmap_level + 1);

	uint texture_size = material.texture.size >> mipmap_level;
	uint texture_offset = 0;
	for(int i = 0;i < mipmap_level;i++)
		texture_offset += (material.texture.size >> i) * (material.texture.size >> i);

	float texture_x = texture_uv.x * TEXTURE_SCALE / material.texture.size / (8 << node.depth);
	float texture_y = texture_uv.y * TEXTURE_SCALE / material.texture.size / (8 << node.depth);

	texture_x = tFractUnsigned(texture_x);
	texture_y = tFractUnsigned(texture_y);
	texture_x *= texture_size;
	texture_y *= texture_size;

	uint location = (int)texture_x * texture_size + (int)texture_y + texture_offset;

	pixel_t text = material.texture.data[location];
	vec3 text_v = {text.r,text.g,text.b};
	vec3mulvec3(&text_v,luminance);
	text_v = vec3mixR(text_v,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
	text.r = tMin(text_v.r,0xff);
	text.g = tMin(text_v.g,0xff);
	text.b = tMin(text_v.b,0xff);
	vram.draw[x * (int)WND_RESOLUTION.y + y] = text;
}

void drawPixelGroup4(int x,int y,node_hit_t result,vec3 camera_pos,vec3 ray_direction){
	block_node_t node = node_root[result.node];
	if(result.node == -1)
		return;
	block_t* block = node.block;
	if(!block)
		return;
	material_t material = material_array[block->material];

	if(material.flags & MAT_LUMINANT){
		vec3 luminance = vec3mulR(material.luminance,camera_exposure);
		pixel_t color = {
			tMinf(luminance.r * 256.0f,255.0f),
			tMinf(luminance.g * 256.0f,255.0f),
			tMinf(luminance.b * 256.0f,255.0f)
		};
		for(int px = 0;px < 4;px++)
			for(int py = 0;py < 4;py++)
				vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = color;
		return;
	}
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;

	vec3 block_pos = vec3mulR((vec3){node.pos.x,node.pos.y,node.pos.z},block_size);

	plane_t plane = getPlane(block_pos,ray_direction,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,camera_pos);

	int side = result.side * 2 + (ray_direction.a[result.side] < 0.0f);

	float distance = rayIntersectPlane(plane_pos,ray_direction,plane.normal);

	float fog = distance;
	fog *= FOG_DISTANCE;
	if(fog > 1.0f){
		drawSky(x,y,4,ray_direction);
		return;
	}

	vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(ray_direction,distance));
	vec2 uv = {hit_pos.a[plane.x] - plane.pos.a[plane.x],hit_pos.a[plane.y] - plane.pos.a[plane.y]};

	vec2div(&uv,block_size);	

	float dst = rayIntersectPlane(plane_pos,getRayAngle(x,y),plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(getRayAngle(x,y),dst));
	vec2 begin_uv = {hit_pos.a[plane.x] - plane.pos.a[plane.x],hit_pos.a[plane.y] - plane.pos.a[plane.y]};
	vec2div(&begin_uv,block_size);

	dst = rayIntersectPlane(plane_pos,getRayAngle(x,y),plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(getRayAngle(x,y),dst));
	vec2 texture_uv = {hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};

	vec3 pixel_dir = getRayAngle(x + 4 * 0.5f,y + 4 * 0.5f + 1);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));

	vec2 offset_uv_x = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	vec2div(&offset_uv_x,block_size);

	pixel_dir = getRayAngle(x + 4 * 0.5f + 1,y + 4 * 0.5f);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));

	vec2 offset_uv_y = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	vec2div(&offset_uv_y,block_size);
	
	vec3 luminance;

	vec2 offset_x = vec2mulR(vec2subvec2R(offset_uv_y,uv),4);
	vec2 offset_y = vec2mulR(vec2subvec2R(offset_uv_x,uv),4);

	uv.x = tMinf(tMaxf(uv.x,0.0f),1.0f);
	uv.y = tMinf(tMaxf(uv.y,0.0f),1.0f);

	if(setting_smooth_lighting){
		if(!block->side[side].luminance_p){
			lightNewStackPut(&node_root[result.node],block,side);
			luminance = VEC3_ZERO;
		}
		else
			luminance = getLuminance(block->side[side],uv,node.depth);
	}
	else{
		if(block->side[side].luminance_p){
			int lm_size = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			uint location = (int)(uv.x * lm_size + 1) * (lm_size + 2) + (int)(uv.y * lm_size + 1);
			luminance = block->side[side].luminance_p[location];
		}
		else{
			lightNewStackPut(&node_root[result.node],block,side);
			luminance = VEC3_ZERO;
		}
	}
	if(material.flags & MAT_REFLECT){
		hit_pos.a[result.side] += (ray_direction.a[result.side] < 0.0f) * 0.0001f - 0.0002f;
		ray_direction.a[result.side] = -ray_direction.a[result.side];

		float texture_x = tFractUnsigned(uv.x * (128.0f / (1 << node.depth)));
		float texture_y = tFractUnsigned(uv.y * (128.0f / (1 << node.depth)));

		luminance = vec3mixR(getReflectLuminance(hit_pos,ray_direction),luminance,0.5f);
	}

	if(!material.texture.data){
		vec3mul(&luminance,camera_exposure);
		static float avg;
		vec3 block_color = vec3mulR(material.luminance,255.0f);
		vec3mulvec3(&block_color,luminance);
		block_color = vec3mixR(block_color,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
		pixel_t block_color_i = {
			tMin(block_color.r,0xff),
			tMin(block_color.g,0xff),
			tMin(block_color.b,0xff)
		};
		for(int px = 0;px < 4;px++){
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 0)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 1)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 2)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 3)] = block_color_i;
		}
		return;
	}
	
	float scale = TEXTURE_SCALE / material.texture.size / (8 << node.depth);

	float texture_x = uv.x * scale;
	float texture_y = uv.y * scale;

	texture_pixel_angle_t texture_angle;

	texture_angle.x.x = offset_uv_x.x * scale - texture_x;
	texture_angle.x.y = offset_uv_x.y * scale - texture_y;

	texture_angle.y.x = offset_uv_y.x * scale - texture_x;
	texture_angle.y.y = offset_uv_y.y * scale - texture_y;

	texture_x = texture_uv.x * scale;
	texture_y = texture_uv.y * scale;
	
	uint mipmap_level = distance / (tAbsf(ray_direction.a[result.side]) * MIPMAP_AGGRESION);
	_BitScanReverse(&mipmap_level,mipmap_level + 1);
	uint texture_size = material.texture.size >> mipmap_level;
	uint texture_offset = 0;
	for(int i = 0;i < mipmap_level;i++)
		texture_offset += (material.texture.size >> i) * (material.texture.size >> i);

	vec2 fract_texture = {
		tFractUnsigned(texture_x) * texture_size,
		tFractUnsigned(texture_y) * texture_size
	};

	texture_pixel_angle_t texture_angle_row;
	texture_angle_row.x = vec2mulR(texture_angle.x,texture_size * 4);
	texture_angle_row.y = vec2mulR(texture_angle.y,texture_size * 4);

	uint bound_x_1 = tFloorf(fract_texture.x + texture_angle_row.x.x);
	uint bound_x_2 = tFloorf(fract_texture.x + texture_angle_row.y.x);
	uint bound_x_3 = tFloorf(fract_texture.x + texture_angle_row.x.x + texture_angle_row.y.x);
	uint bound_y_1 = tFloorf(fract_texture.y + texture_angle_row.x.y);
	uint bound_y_2 = tFloorf(fract_texture.y + texture_angle_row.y.y);
	uint bound_y_3 = tFloorf(fract_texture.y + texture_angle_row.x.y + texture_angle_row.y.y);

	uint bound_x = bound_x_1 | bound_x_2 | bound_x_3;
	uint bound_y = bound_y_1 | bound_y_2 | bound_y_3;
	vec3mul(&luminance,camera_exposure);
	if(bound_x < texture_size && bound_y < texture_size){
		vec2mul(&texture_angle.x,texture_size);
		vec2mul(&texture_angle.y,texture_size);
		textureSingleLuminanceSIMD128(material,luminance,fract_texture,texture_angle,texture_size,x,y,texture_offset,fog);
		return;
	}
	for(int px = 0;px < 4;px++){
		for(int py = 0;py < 4;py++){
			int texture_x_i = tFractUnsigned(texture_x) * texture_size;
			int texture_y_i = tFractUnsigned(texture_y) * texture_size;
			pixel_t text = material.texture.data[texture_x_i * texture_size + texture_y_i + texture_offset];
			vec3 text_v = {text.r,text.g,text.b};

			vec3mulvec3(&text_v,luminance);
			text_v = vec3mixR(text_v,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
			text.r = tMin(text_v.r,0xff);
			text.g = tMin(text_v.g,0xff);
			text.b = tMin(text_v.b,0xff);
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = text;
			texture_x += texture_angle.x.x;
			texture_y += texture_angle.x.y;
		}
		texture_x -= texture_angle.x.x * 4;
		texture_y -= texture_angle.x.y * 4;
		texture_x += texture_angle.y.x;
		texture_y += texture_angle.y.y;
	}
}

void drawPixelGroup8(int x,int y,node_hit_t result,vec3 camera_pos,vec3 ray_direction){
	block_node_t node = node_root[result.node];
	if(result.node == -1)
		return;
	block_t* block = node.block;
	if(!block)
		return;
	material_t material = material_array[block->material];

	if(material.flags & MAT_LUMINANT){
		vec3 luminance = vec3mulR(material.luminance,camera_exposure);
		pixel_t color = {
			tMinf(luminance.r * 256.0f,255.0f),
			tMinf(luminance.g * 256.0f,255.0f),
			tMinf(luminance.b * 256.0f,255.0f)
		};
		for(int px = 0;px < 8;px++)
			for(int py = 0;py < 8;py++)
				vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = color;
		return;
	}
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;

	vec3 block_pos = vec3mulR((vec3){node.pos.x,node.pos.y,node.pos.z},block_size);

	plane_t plane = getPlane(block_pos,ray_direction,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,camera_pos);

	int side = result.side * 2 + (ray_direction.a[result.side] < 0.0f);

	float distance = rayIntersectPlane(plane_pos,ray_direction,plane.normal);

	float fog = distance;
	fog *= FOG_DISTANCE;
	if(fog > 1.0f){
		drawSky(x,y,8,ray_direction);
		return;
	}

	vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(ray_direction,distance));
	vec2 uv = {hit_pos.a[plane.x] - plane.pos.a[plane.x],hit_pos.a[plane.y] - plane.pos.a[plane.y]};

	vec2div(&uv,block_size);	

	float dst = rayIntersectPlane(plane_pos,getRayAngle(x,y),plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(getRayAngle(x,y),dst));
	vec2 begin_uv = {hit_pos.a[plane.x] - plane.pos.a[plane.x],hit_pos.a[plane.y] - plane.pos.a[plane.y]};
	vec2div(&begin_uv,block_size);

	dst = rayIntersectPlane(plane_pos,getRayAngle(x,y),plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(getRayAngle(x,y),dst));
	vec2 texture_uv = {hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};

	vec3 pixel_dir = getRayAngle(x + 8 * 0.5f,y + 8 * 0.5f + 1);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));

	vec2 offset_uv_x = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	vec2div(&offset_uv_x,block_size);

	pixel_dir = getRayAngle(x + 8 * 0.5f + 1,y + 8 * 0.5f);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));

	vec2 offset_uv_y = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	vec2div(&offset_uv_y,block_size);
	
	vec3 luminance;

	vec2 offset_x = vec2mulR(vec2subvec2R(offset_uv_y,uv),7);
	vec2 offset_y = vec2mulR(vec2subvec2R(offset_uv_x,uv),7);

	uv.x = tMinf(tMaxf(uv.x,0.0f),1.0f);
	uv.y = tMinf(tMaxf(uv.y,0.0f),1.0f);

	if(setting_smooth_lighting){
		if(!block->side[side].luminance_p){
			lightNewStackPut(&node_root[result.node],block,side);
			luminance = VEC3_ZERO;
		}
		else
			luminance = getLuminance(block->side[side],uv,node.depth);
	}
	else{
		if(block->side[side].luminance_p){
			int lm_size = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			uint location = (int)(uv.x * lm_size + 1) * (lm_size + 2) + (int)(uv.y * lm_size + 1);
			luminance = block->side[side].luminance_p[location];
		}
		else{
			lightNewStackPut(&node_root[result.node],block,side);
			luminance = VEC3_ZERO;
		}
	}
	if(material.flags & MAT_REFLECT){
		hit_pos.a[result.side] += (ray_direction.a[result.side] < 0.0f) * 0.0001f - 0.0002f;
		ray_direction.a[result.side] = -ray_direction.a[result.side];

		float texture_x = tFractUnsigned(uv.x * (128.0f / (1 << node.depth)));
		float texture_y = tFractUnsigned(uv.y * (128.0f / (1 << node.depth)));

		luminance = vec3mixR(getReflectLuminance(hit_pos,ray_direction),luminance,0.5f);
	}

	if(!material.texture.data){
		vec3mul(&luminance,camera_exposure);
		static float avg;
		vec3 block_color = vec3mulR(material.luminance,255.0f);
		vec3mulvec3(&block_color,luminance);
		block_color = vec3mixR(block_color,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
		pixel_t block_color_i = {
			tMin(block_color.r,0xff),
			tMin(block_color.g,0xff),
			tMin(block_color.b,0xff)
		};
		for(int px = 0;px < 8;px++){
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 0)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 1)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 2)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 3)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 4)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 5)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 6)] = block_color_i;
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + 7)] = block_color_i;
		}
		return;
	}
	
	float scale = TEXTURE_SCALE / material.texture.size / (8 << node.depth);

	float texture_x = uv.x * scale;
	float texture_y = uv.y * scale;

	texture_pixel_angle_t texture_angle;

	texture_angle.x.x = offset_uv_x.x * scale - texture_x;
	texture_angle.x.y = offset_uv_x.y * scale - texture_y;

	texture_angle.y.x = offset_uv_y.x * scale - texture_x;
	texture_angle.y.y = offset_uv_y.y * scale - texture_y;

	texture_x = texture_uv.x * scale;
	texture_y = texture_uv.y * scale;
	
	uint mipmap_level = distance / (tAbsf(ray_direction.a[result.side]) * MIPMAP_AGGRESION);
	_BitScanReverse(&mipmap_level,mipmap_level + 1);
	uint texture_size = material.texture.size >> mipmap_level;
	uint texture_offset = 0;
	for(int i = 0;i < mipmap_level;i++)
		texture_offset += (material.texture.size >> i) * (material.texture.size >> i);

	vec2 fract_texture = {
		tFractUnsigned(texture_x) * texture_size,
		tFractUnsigned(texture_y) * texture_size
	};

	texture_pixel_angle_t texture_angle_row;
	texture_angle_row.x = vec2mulR(texture_angle.x,texture_size * 7);
	texture_angle_row.y = vec2mulR(texture_angle.y,texture_size * 7);

	uint bound_x_1 = tFloorf(fract_texture.x + texture_angle_row.x.x);
	uint bound_x_2 = tFloorf(fract_texture.x + texture_angle_row.y.x);
	uint bound_x_3 = tFloorf(fract_texture.x + texture_angle_row.x.x + texture_angle_row.y.x);
	uint bound_y_1 = tFloorf(fract_texture.y + texture_angle_row.x.y);
	uint bound_y_2 = tFloorf(fract_texture.y + texture_angle_row.y.y);
	uint bound_y_3 = tFloorf(fract_texture.y + texture_angle_row.x.y + texture_angle_row.y.y);

	uint bound_x = bound_x_1 | bound_x_2 | bound_x_3;
	uint bound_y = bound_y_1 | bound_y_2 | bound_y_3;
	vec3mul(&luminance,camera_exposure);
	if(bound_x < texture_size && bound_y < texture_size){
		vec2mul(&texture_angle.x,texture_size * 0.5f);
		vec2mul(&texture_angle.y,texture_size * 0.5f);
		textureDrawSIMD8(material,luminance,fract_texture,texture_angle,texture_size,x,y,texture_offset,fog);
		return;
	}
	for(int px = 0;px < 8;px++){
		for(int py = 0;py < 8;py++){
			int texture_x_i = tFractUnsigned(texture_x) * texture_size;
			int texture_y_i = tFractUnsigned(texture_y) * texture_size;
			pixel_t text = material.texture.data[texture_x_i * texture_size + texture_y_i + texture_offset];
			vec3 text_v = {text.r,text.g,text.b};

			vec3mulvec3(&text_v,luminance);
			text_v = vec3mixR(text_v,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
			text.r = tMin(text_v.r,0xff);
			text.g = tMin(text_v.g,0xff);
			text.b = tMin(text_v.b,0xff);
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = text;
			texture_x += texture_angle.x.x;
			texture_y += texture_angle.x.y;
		}
		texture_x -= texture_angle.x.x * 8;
		texture_y -= texture_angle.x.y * 8;
		texture_x += texture_angle.y.x;
		texture_y += texture_angle.y.y;
	}
}

void drawPixelGroup16(int x,int y,node_hit_t result,vec3 camera_pos,vec3 ray_direction){
	block_node_t node = node_root[result.node];
	block_t* block = node.block;
	if(!block)
		return;
	material_t material = material_array[block->material];

	if(material.flags & MAT_LUMINANT){
		vec3 luminance = vec3mulR(material.luminance,camera_exposure);
		pixel_t color = {
			tMinf(luminance.r * 256.0f,255.0f),
			tMinf(luminance.g * 256.0f,255.0f),
			tMinf(luminance.b * 256.0f,255.0f)
		};
		__m256i pixel = _mm256_set1_epi32(color.size);
		uint offset = x * (int)WND_RESOLUTION.y + y;
		for(int i = 0;i < 16;i++){
			_mm256_stream_si256(vram.draw + offset,pixel);
			_mm256_stream_si256(vram.draw + offset + 8,pixel);
			offset += (int)WND_RESOLUTION.y;
		}
		return;
	}
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;

	vec3 block_pos = vec3mulR((vec3){node.pos.x,node.pos.y,node.pos.z},block_size);

	plane_t plane = getPlane(block_pos,ray_direction,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,camera_pos);

	int side = result.side * 2 + (ray_direction.a[result.side] < 0.0f);

	float distance = ((plane.pos.a[plane.z] - camera_pos.a[plane.z])) / ray_direction.a[plane.z];
	
	float fog = distance;
	fog *= FOG_DISTANCE;
	if(fog > 1.0f){
		drawSky(x,y,16,ray_direction);
		return;
	}
	
	vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(ray_direction,distance));
	vec2 uv = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};

	float dst = rayIntersectPlane(plane_pos,getRayAngle(x,y),plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(getRayAngle(x,y),dst));
	vec2 begin_uv = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	
	begin_uv.x = tMaxf(begin_uv.x,0.0f);
	begin_uv.y = tMaxf(begin_uv.y,0.0f);

	dst = tAbsf(rayIntersectPlane(plane_pos,getRayAngle(x,y),plane.normal));
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(getRayAngle(x,y),dst));
	vec2 texture_uv = {
		hit_pos.a[plane.x] / block_size,
		hit_pos.a[plane.y] / block_size
	};

	vec3 pixel_dir = getRayAngle(x + 16 * 0.5f,y + 16 * 0.5f + 1);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));

	vec2 offset_uv_x = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	
	pixel_dir = getRayAngle(x + 16 * 0.5f + 1,y + 16 * 0.5f);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));

	vec2 offset_uv_y = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};

	texture_uv.x = tMaxf(texture_uv.x,0.0f);
	texture_uv.y = tMaxf(texture_uv.y,0.0f);

	vec2div(&uv,block_size);	
	vec2div(&begin_uv,block_size);
	vec2div(&offset_uv_x,block_size);
	vec2div(&offset_uv_y,block_size);
	
	vec3 luminance[4];

	vec2 offset_x = vec2mulR(vec2subvec2R(offset_uv_y,uv),15);
	vec2 offset_y = vec2mulR(vec2subvec2R(offset_uv_x,uv),15);
	if(setting_smooth_lighting){
		if(!block->side[side].luminance_p){
			lightNewStackPut(&node_root[result.node],block,side);
			luminance[0] = VEC3_ZERO;
			luminance[1] = VEC3_ZERO;
			luminance[2] = VEC3_ZERO;
			luminance[3] = VEC3_ZERO;
		}
		else{
			luminance[0] = getLuminance(block->side[side],begin_uv,node.depth);
			luminance[1] = getLuminance(block->side[side],vec2addvec2R(begin_uv,offset_x),node.depth);
			luminance[2] = getLuminance(block->side[side],vec2addvec2R(begin_uv,offset_y),node.depth);
			luminance[3] = getLuminance(block->side[side],vec2addvec2R(begin_uv,vec2addvec2R(offset_x,offset_y)),node.depth);
		}
	}
	else{
		if(block->side[side].luminance_p){
			uint lm_size = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			uint location = (int)(uv.x * lm_size + 1) * (lm_size + 2) + (int)(uv.y * lm_size + 1);
			luminance[0] = block->side[side].luminance_p[location];
			luminance[1] = luminance[0];
			luminance[2] = luminance[0];
			luminance[3] = luminance[0];
		}
		else{
			lightNewStackPut(&node_root[result.node],block,side);
			luminance[0] = VEC3_ZERO;
			luminance[1] = VEC3_ZERO;
			luminance[2] = VEC3_ZERO;
			luminance[3] = VEC3_ZERO;
		}
	}
	if(material.flags & MAT_REFLECT){
		hit_pos.a[result.side] += (ray_direction.a[result.side] < 0.0f) * 0.0001f - 0.0002f;
		ray_direction.a[result.side] = -ray_direction.a[result.side];

		vec3 luminance_reflect2[4];
		vec3 ray_angles[4];

		ray_angles[0] = getRayAngle(x,y);
		ray_angles[1] = getRayAngle(x,y + 15);
		ray_angles[2] = getRayAngle(x + 15,y);
		ray_angles[3] = getRayAngle(x + 15,y + 15);

		ray_angles[0].a[result.side] = -ray_angles[0].a[result.side];
		ray_angles[1].a[result.side] = -ray_angles[1].a[result.side];
		ray_angles[2].a[result.side] = -ray_angles[2].a[result.side];
		ray_angles[3].a[result.side] = -ray_angles[3].a[result.side];

		luminance_reflect2[0] = getReflectLuminance(hit_pos,ray_angles[0]);
		luminance_reflect2[1] = getReflectLuminance(hit_pos,ray_angles[1]);
		luminance_reflect2[2] = getReflectLuminance(hit_pos,ray_angles[2]);
		luminance_reflect2[3] = getReflectLuminance(hit_pos,ray_angles[3]);

		luminance[0] = vec3mixR(luminance_reflect2[0],luminance[0],0.5f);
		luminance[1] = vec3mixR(luminance_reflect2[2],luminance[1],0.5f);
		luminance[2] = vec3mixR(luminance_reflect2[1],luminance[2],0.5f);
		luminance[3] = vec3mixR(luminance_reflect2[3],luminance[3],0.5f);
	}

	if(!material.texture.data){
		vec3mul(&luminance[0],camera_exposure);
		vec3mul(&luminance[1],camera_exposure);
		vec3mul(&luminance[2],camera_exposure);
		vec3mul(&luminance[3],camera_exposure);
		drawSIMD(vec3mulR(material.luminance,255.0f),luminance,x,y,fog);
		return;
		vec3 luminance_combined = vec3addvec3R(vec3addvec3R(luminance[0],luminance[1]),vec3addvec3R(luminance[2],luminance[3]));
		vec3mul(&luminance_combined,0.25f);
		vec3mul(&luminance_combined,camera_exposure);
		static float avg;
		vec3 block_color = vec3mulR(material.luminance,255.0f);
		vec3mulvec3(&block_color,luminance_combined);
		block_color = vec3mixR(block_color,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
		pixel_t block_color_i = {
			tMin(block_color.r,0xff),
			tMin(block_color.g,0xff),
			tMin(block_color.b,0xff)
		};
		__m256i pixel = _mm256_set1_epi32(block_color_i.size);
		uint offset = x * (int)WND_RESOLUTION.y + y;
		for(int i = 0;i < 16;i++){
			_mm256_stream_si256(vram.draw + offset,pixel);
			_mm256_stream_si256(vram.draw + offset + 8,pixel);
			offset += (int)WND_RESOLUTION.y;
		}
		return;
	}
	
	float scale = TEXTURE_SCALE / material.texture.size / (8 << node.depth);

	float texture_x = uv.x * scale;
	float texture_y = uv.y * scale;

	texture_pixel_angle_t texture_angle;

	texture_angle.x.x = offset_uv_x.x * scale - texture_x;
	texture_angle.x.y = offset_uv_x.y * scale - texture_y;

	texture_angle.y.x = offset_uv_y.x * scale - texture_x;
	texture_angle.y.y = offset_uv_y.y * scale - texture_y;

	texture_x = texture_uv.x * scale;
	texture_y = texture_uv.y * scale;
	
	uint mipmap_level = distance / (tAbsf(ray_direction.a[result.side]) * MIPMAP_AGGRESION);
	_BitScanReverse(&mipmap_level,mipmap_level + 1);
	uint texture_size = material.texture.size >> mipmap_level;
	uint texture_offset = 0;

	vec2 fract_texture = {
		tFractUnsigned(texture_x) * texture_size,
		tFractUnsigned(texture_y) * texture_size
	};

	texture_pixel_angle_t texture_angle_row;
	texture_angle_row.x = vec2mulR(texture_angle.x,texture_size * 16);
	texture_angle_row.y = vec2mulR(texture_angle.y,texture_size * 16);

	uint bound_x_1 = tFloorf(fract_texture.x + texture_angle_row.x.x);
	uint bound_x_2 = tFloorf(fract_texture.x + texture_angle_row.y.x);
	uint bound_x_3 = tFloorf(fract_texture.x + texture_angle_row.x.x + texture_angle_row.y.x);
	uint bound_y_1 = tFloorf(fract_texture.y + texture_angle_row.x.y);
	uint bound_y_2 = tFloorf(fract_texture.y + texture_angle_row.y.y);
	uint bound_y_3 = tFloorf(fract_texture.y + texture_angle_row.x.y + texture_angle_row.y.y);

	uint bound_x = bound_x_1 | bound_x_2 | bound_x_3;
	uint bound_y = bound_y_1 | bound_y_2 | bound_y_3;

	for(int i = 0;i < mipmap_level;i++)
		texture_offset += (material.texture.size >> i) * (material.texture.size >> i);

	vec3mul(&luminance[0],camera_exposure);
	vec3mul(&luminance[1],camera_exposure);
	vec3mul(&luminance[2],camera_exposure);
	vec3mul(&luminance[3],camera_exposure);

	if(bound_x < texture_size && bound_y < texture_size){
		vec2mul(&texture_angle.x,texture_size);
		vec2mul(&texture_angle.y,texture_size);
		textureDrawSIMD(material,luminance,fract_texture,texture_angle,texture_size,x,y,texture_offset,fog);
		return;
	}
	for(int px = 0;px < 16;px++){
		float m_x = (float)px / (16 - 1);
		for(int py = 0;py < 16;py++){
			uint texture_x_i = tFractUnsigned(texture_x) * texture_size;
			uint texture_y_i = tFractUnsigned(texture_y) * texture_size;
			uint location = texture_x_i * texture_size + texture_y_i + texture_offset;
			location = tMax(location,0);
			pixel_t text = material.texture.data[location];
			vec3 text_v = {text.r,text.g,text.b};
					
			float m_y = (float)py / (16 - 1);

			vec3 lum_1 = vec3mulR(luminance[0],(1.0f - m_x) * (1.0f - m_y));
			vec3 lum_2 = vec3mulR(luminance[1],m_x * (1.0f - m_y));
			vec3 lum_3 = vec3mulR(luminance[2],m_y * (1.0f - m_x));
			vec3 lum_4 = vec3mulR(luminance[3],m_y * m_x);

			vec3mulvec3(&text_v,vec3addvec3R(vec3addvec3R(lum_1,lum_2),vec3addvec3R(lum_3,lum_4)));
			text_v = vec3mixR(text_v,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
			text.r = tMin(text_v.r,0xff);
			text.g = tMin(text_v.g,0xff);
			text.b = tMin(text_v.b,0xff);
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = text;
			texture_x += texture_angle.x.x;
			texture_y += texture_angle.x.y;
		}
		texture_x -= texture_angle.x.x * 16;
		texture_y -= texture_angle.x.y * 16;
		texture_x += texture_angle.y.x;
		texture_y += texture_angle.y.y;
	}
	return;
}