#include <math.h>

#include "draw.h" 
#include "tmath.h"
#include "lighting.h"

void drawSurfacePixel(block_t* block,vec3 dir,vec3 camera_pos,int axis,int x,int y){
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT){
		pixel_t color = {tMinf(material.luminance.r * 255.0f,255.0f),tMinf(material.luminance.g * 255.0f,255.0f),tMinf(material.luminance.b * 255.0f,255.0f)};
		vram.draw[x * (int)WND_RESOLUTION.y + y] = color;
		return;
	}

	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
	block_pos.x = block->pos.x * block_size;
	block_pos.y = block->pos.y * block_size;
	block_pos.z = block->pos.z * block_size;
	plane_t plane = getPlane(block_pos,dir,axis,block_size);
	float dst = rayIntersectPlane(vec3subvec3R(plane.pos,camera_pos),dir,plane.normal);
	vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(dir,dst));
	pixel_t pixel;
	vec3 color;
	int side = axis * 2 + (dir.a[axis] < 0.0f);
	vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
	vec3 luminance = getLuminance(block->side[side],uv,block->depth);
	if(material.flags & MAT_REFLECT){
		hit_pos.a[side >> 1] += (dir.a[axis] < 0.0f) * 0.0001f - 0.0002f;
		dir.a[axis] = -dir.a[axis];
		vec2 texture_uv = (vec2){hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};
		float texture_x = tFractUnsigned(uv.x * (128.0f / (1 << block->depth)));
		float texture_y = tFractUnsigned(uv.y * (128.0f / (1 << block->depth)));

		luminance = vec3mixR(getReflectLuminance(hit_pos,dir),luminance,0.75f);
	}
	if(material.texture){
		vec2 texture_uv = {hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};
		int texture_x = tFract(texture_uv.x * (32.0f / (1 << block->depth))) * material.texture[0].size;
		int texture_y = tFract(texture_uv.y * (32.0f / (1 << block->depth))) * material.texture[0].size;
		pixel_t pixel = material.texture[texture_x * material.texture[0].size + texture_y + 1];
		color = (vec3){pixel.r,pixel.g,pixel.b};
	}
	else
		color = (vec3){255.0f * 0.9f,255.0f * 0.9f,255.0f * 0.9f}; 
	vec3mulvec3(&color,luminance);
	pixel = (pixel_t){tMin(color.r,0xff),tMin(color.g,0xff),tMin(color.b,0xff)};
	vram.draw[x * (int)WND_RESOLUTION.y + y] = pixel;
}

void tracePixel(traverse_init_t init,vec3 dir,int x,int y,vec3 camera_pos){
	blockray_t result = traverseTree(ray3Create(init.pos,dir),init.node);
	if(result.node){
		drawSurfacePixel(result.node->block,dir,camera_pos,result.side,x,y);
		return;
	}
	int side;
	vec2 uv = sampleCube(getRayAngle(x,y),&side);
	if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f){
		vram.draw[x * (int)WND_RESOLUTION.y + y] = (pixel_t){255,255,255};
		return;
	}
	vram.draw[x * (int)WND_RESOLUTION.y + y] = (pixel_t){200,200,200};
}

void drawSurfacePixelGroup(block_t* block,plane_t plane,vec3 hit_pos,vec3 luminance,vec3 dir,vec3 camera_pos,int axis,int x,int y){
	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
	block_pos.x = block->pos.x * block_size;
	block_pos.y = block->pos.y * block_size;
	block_pos.z = block->pos.z * block_size;
	pixel_t pixel;
	vec3 color;
	int side = axis * 2 + (dir.a[axis] < 0.0f);
	vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
	material_t material = material_array[block->material];
	if(material.texture){
		vec2 texture_uv = {hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};
		int texture_x = tFract(texture_uv.x * (32.0f / (1 << block->depth))) * material.texture[0].size;
		int texture_y = tFract(texture_uv.y * (32.0f / (1 << block->depth))) * material.texture[0].size;
		pixel_t pixel = material.texture[texture_x * material.texture[0].size + texture_y + 1];
		color = (vec3){pixel.r,pixel.g,pixel.b};
	}
	else
		color = (vec3){255.0f * 0.9f,255.0f * 0.9f,255.0f * 0.9f}; 
	vec3mulvec3(&color,luminance);
	pixel = (pixel_t){tMin(color.r,0xff),tMin(color.g,0xff),tMin(color.b,0xff)};
	vram.draw[x * (int)WND_RESOLUTION.y + y] = pixel;
}
#include <stdio.h>
void drawPixelGroup(int x,int y,traverse_init_t init,vec3 camera_pos){
	vec3 ray_direction = getRayAngle(0.5f * RD_BLOCK - 0.5f + x,0.5f * RD_BLOCK - 0.5f + y);
	blockray_t result = traverseTree(ray3Create(init.pos,ray_direction),init.node);
	if(!result.node){
		int side;
		vec2 uv = sampleCube(ray_direction,&side);
		for(int px = 0;px < RD_BLOCK;px++){
			for(int py = 0;py < RD_BLOCK;py++){
				if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f){
					vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (pixel_t){255,255,255};
					continue;
				}
				vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = (pixel_t){200,200,200};
			}
		}
		return;
	}
	block_t* block = result.node->block;
	material_t material = material_array[block->material];

	if(material.flags & MAT_LUMINANT){
		pixel_t color = {tMinf(material.luminance.r * 256.0f,255.0f),tMinf(material.luminance.g * 256.0f,255.0f),tMinf(material.luminance.b * 256.0f,255.0f)};
		for(int px = 0;px < RD_BLOCK;px++)
			for(int py = 0;py < RD_BLOCK;py++)
				vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = color;

		return;
	}

	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
	block_pos.x = block->pos.x * block_size;
	block_pos.y = block->pos.y * block_size;
	block_pos.z = block->pos.z * block_size;
	plane_t plane = getPlane(block_pos,ray_direction,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,camera_pos);

	float dst = rayIntersectPlane(plane_pos,ray_direction,plane.normal);
	vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(ray_direction,dst));
	vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};

	vec2 texture_uv = (vec2){hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};
	int side = result.side * 2 + (ray_direction.a[result.side] < 0.0f);
	
	vec3 luminance;
	portal_t portal = block->side[side].portal;
	if(portal.block){
		int x_table[] = {VEC3_Y,VEC3_X,VEC3_X};
		int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
		int portal_x = x_table[portal.side >> 1];
		int portal_y = y_table[portal.side >> 1];

		vec3 pos = {portal.block->pos.x,portal.block->pos.y,portal.block->pos.z};
		vec3mul(&pos,block_size);

		pos.a[portal_x] += hit_pos.a[plane.x] - plane.pos.a[plane.x];
		pos.a[portal_y] += hit_pos.a[plane.y] - plane.pos.a[plane.y];

		if(side == portal.side)
			ray_direction.a[side >> 1] = -ray_direction.a[side >> 1];
		else if(side >> 1 != portal.side >> 1){
			float temp = ray_direction.a[portal.side >> 1] * -((portal.side & 1) * 2 - 1);
			ray_direction.a[portal.side >> 1] = ray_direction.a[side >> 1] * ((portal.side & 1) * 2 - 1);
			ray_direction.a[side >> 1] = temp;
		}

		pos.a[portal.side >> 1] += (ray_direction.a[portal.side >> 1] > 0.0f) * block_size * 1.0002f - 0.0001f; 
		luminance = getReflectLuminance(pos,ray_direction);
	}
	else
		luminance = getLuminance(block->side[side],uv,block->depth);

	if(block->depth < LM_MAXDEPTH){
		vec3 pos = {block->pos.x,block->pos.y,block->pos.z};
		int size = 1 << LM_MAXDEPTH - block->depth;

		pos.a[plane.x] -= 0.5f - 0.5f / size;
		pos.a[plane.y] -= 0.5f - 0.5f / size;

		int lightmap_x = uv.x * size;
		int lightmap_y = uv.y * size;

		pos.a[plane.x] += 1.0f / size * lightmap_x;
		pos.a[plane.y] += 1.0f / size * lightmap_y;

		if(block->side[side].luminance_update_priority_p[lightmap_x * size + lightmap_y]++ > 1 << LM_UPDATE_RATE){
			block->side[side].luminance_update_priority_p[lightmap_x * size + lightmap_y] = 0;
			setLightSide(&block->side[side].luminance_p[(lightmap_x + 1) * (size + 2) + (lightmap_y + 1)],pos,side,block->depth);
			int hit_side = ray_direction.a[result.side] < 0.0f;
			vec3* lightmap = block->side[side].luminance_p;
			if(!lightmap_x)
				lightmap[lightmap_y + 1] = lightmapDownX(block,side,hit_side,lightmap_x,lightmap_y,size);
			if(!lightmap_y)
				lightmap[(lightmap_x + 1) * (size + 2)] = lightmapDownY(block,side,hit_side,lightmap_x,lightmap_y,size);
			if(lightmap_x == size - 1)
				lightmap[(size + 1) * (size + 2) + (lightmap_y + 1)] = lightmapUpX(block,side,hit_side,lightmap_x,lightmap_y,size);
			if(lightmap_y == size - 1)
				lightmap[(lightmap_x + 1) * (size + 2) + (size + 1)] = lightmapUpY(block,side,hit_side,lightmap_x,lightmap_y,size);
			if(!lightmap_x && !lightmap_y)
				lightmap[0] = vec3avgvec3R(lightmap[1],lightmap[size + 2]);
			if(!lightmap_x && lightmap_y == size - 1)
				lightmap[size + 1] = vec3avgvec3R(lightmap[size],lightmap[size + 2 + size + 1]);
			if(lightmap_x == size - 1 && !lightmap_y)
				lightmap[(size + 1) * (size + 2)] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + 1],lightmap[(size + 1) * (size + 1)]);
			if(lightmap_x == size - 1 && lightmap_y == size - 1)
				lightmap[(size + 1) * (size + 2) + size + 1] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + size],lightmap[(size + 1) * (size + 1) + size + 1]);
		}
	}
	else if(block->side[side].luminance_update_priority++ > 1 << LM_UPDATE_RATE){
		block->side[side].luminance_update_priority = 0;
		vec3 pos = {block->pos.x,block->pos.y,block->pos.z};
		setLightSide(&block->side[side].luminance,pos,side,block->depth);
	}
	if(material.flags & MAT_REFLECT){
		hit_pos.a[side >> 1] += (ray_direction.a[result.side] < 0.0f) * 0.0001f - 0.0002f;
		ray_direction.a[result.side] = -ray_direction.a[result.side];

		float texture_x = tFractUnsigned(uv.x * (128.0f / (1 << block->depth)));
		float texture_y = tFractUnsigned(uv.y * (128.0f / (1 << block->depth)));
		if(material.flags & MAT_REFRACT){
			if(texture_x < 0.2f)
				ray_direction.a[plane.x] -= (0.2f - texture_x) * 1.5f;
			if(texture_x > 0.8f)
				ray_direction.a[plane.x] -= (0.8f - texture_x) * 1.5f;
			if(texture_y < 0.2f)
				ray_direction.a[plane.y] -= (0.2f - texture_y) * 1.5f;
			if(texture_y > 0.8f)
				ray_direction.a[plane.y] -= (0.8f - texture_y) * 1.5f;
		}
		luminance = vec3mixR(getReflectLuminance(hit_pos,ray_direction),luminance,0.75f);
	}

	vec3 pixel_dir = getRayAngle(0.5f * RD_BLOCK - 0.5f + x,0.5f * RD_BLOCK - 0.5f + y + 1);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));
	vec2 offset_uv = (vec2){(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
	
	float texture_x = uv.x * (32.0f / (1 << block->depth));
	float texture_y = uv.y * (32.0f / (1 << block->depth));

	float texture_dxx = (offset_uv.x * (32.0f / (1 << block->depth))) - texture_x;
	float texture_dyx = (offset_uv.y * (32.0f / (1 << block->depth))) - texture_y;

	pixel_dir = getRayAngle(0.5f * RD_BLOCK - 0.5f + x + 1,0.5f * RD_BLOCK - 0.5f + y);
	dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
	hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));
	offset_uv = (vec2){(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};

	float texture_dxy = (offset_uv.x * (32.0f / (1 << block->depth))) - texture_x;
	float texture_dyy = (offset_uv.y * (32.0f / (1 << block->depth))) - texture_y;

	texture_x = texture_uv.x * (32.0f / (1 << block->depth));
	texture_y = texture_uv.y * (32.0f / (1 << block->depth));
	float edge_distance;

	float max_dx = tMaxf(tAbsf(texture_dxx),tAbsf(texture_dxy));
	float max_dy = tMaxf(tAbsf(texture_dyx),tAbsf(texture_dyy));

	edge_distance = tMinf(uv.x / max_dx,(1.0f - uv.x) / max_dx);
	edge_distance = tMinf(tMinf(uv.y / max_dy,(1.0f - uv.y) / max_dy),edge_distance);

	if(edge_distance < 20.0f / block_size){
		for(int px = 0;px < RD_BLOCK;px++){
			for(int py = 0;py < RD_BLOCK;py++){
				vec3 pixel_dir = getRayAngle(x + px,y + py);
				float dst = rayIntersectPlane(plane_pos,pixel_dir,plane.normal);
				vec3 hit_pos = vec3addvec3R(camera_pos,vec3mulR(pixel_dir,dst));
				uv = (vec2){(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};
				if(uv.x < 0.0f || uv.y < 0.0f || uv.x > 1.0f || uv.y > 1.0f){
					tracePixel(init,getRayAngle(x + px,y + py),x + px,y + py,camera_pos);
					continue;
				}
				drawSurfacePixelGroup(block,plane,hit_pos,luminance,pixel_dir,camera_pos,result.side,x + px,y + py);
			}
		}
		return;
	}
	if(!material.texture){
		for(int px = 0;px < RD_BLOCK;px++){
			for(int py = 0;py < RD_BLOCK;py++){
				vec3 block_color = {255.0f * 0.9f,255.0f * 0.9f,255.0f * 0.9f};
				vec3mulvec3(&block_color,luminance);
				pixel_t block_color_i = {tMin(block_color.r,0xff),tMin(block_color.g,0xff),tMin(block_color.b,0xff)};
				vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = block_color_i;
			}
		}
		return;
	}
	for(int px = 0;px < RD_BLOCK;px++){
		for(int py = 0;py < RD_BLOCK;py++){
			int texture_x_i = tFractUnsigned(texture_x) * material.texture[0].size;
			int texture_y_i = tFractUnsigned(texture_y) * material.texture[0].size;
			pixel_t text = material.texture[texture_x_i * material.texture[0].size + texture_y_i + 1];
			vec3 text_v = {text.r,text.g,text.b};
			vec3mulvec3(&text_v,luminance);
			text.r = tMin(text_v.r,0xff);
			text.g = tMin(text_v.g,0xff);
			text.b = tMin(text_v.b,0xff);
			vram.draw[(x + px) * (int)WND_RESOLUTION.y + (y + py)] = text;
			texture_x += texture_dxx;
			texture_y += texture_dyx;
		}
		texture_x -= texture_dxx * RD_BLOCK;
		texture_y -= texture_dyx * RD_BLOCK;
		texture_x += texture_dxy;
		texture_y += texture_dyy;
	}
}