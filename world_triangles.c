#include "source.h"
#include "world_triangles.h"
#include "tmath.h"
#include "fog.h"
#include "opengl.h"
#include "tree.h"
#include "lighting.h"
#include "entity.h"

char mask[RD_MASK_X][RD_MASK_Y];
ivec2 scanline[RD_MASK_X];

void setScanlineMask(vec2_t pos_1,vec2_t pos_2){
	int p_begin,p_end;
	float delta,delta_pos;
	if(pos_1.x > pos_2.x){
		p_end = pos_2.x;
		p_begin = pos_1.x;
		delta = (pos_2.y - pos_1.y) / (pos_2.x - pos_1.x);
		delta_pos = pos_1.y + delta * ((int)pos_1.x-pos_1.x);
	}
	else{
		p_begin = pos_2.x;
		p_end = pos_1.x;
		delta = (pos_1.y - pos_2.y) / (pos_1.x - pos_2.x);
		delta_pos = pos_2.y + delta * ((int)pos_2.x-pos_2.x);
	}
	if(p_begin < 0 || p_end >= RD_MASK_X) 
		return;
	if(p_end < 0) 
		p_end = 0;
	if(p_begin >= RD_MASK_X){
		delta_pos -= delta * (p_begin - RD_MASK_X);
		p_begin = RD_MASK_X;
	}
	while(p_begin-- > p_end){
		scanline[p_begin].x = tMin(scanline[p_begin].x,delta_pos);
		scanline[p_begin].y = tMax(scanline[p_begin].y,delta_pos);
		delta_pos -= delta;
	}
}

void insertMaskQuad(vec2_t pos[4]){
	int max_x = tMax(tMax(pos[0].x,pos[1].x),tMax(pos[2].x,pos[3].x));
	int min_x = tMin(tMin(pos[0].x,pos[1].x),tMin(pos[2].x,pos[3].x));
	max_x = tMin(max_x,RD_MASK_X + 1);
	min_x = tMax(min_x,-1);
	for(int i = min_x;i < max_x;i++){
		scanline[i].x = RD_MASK_Y;
		scanline[i].y = 0;
	}
	setScanlineMask(pos[0],pos[1]);
	setScanlineMask(pos[1],pos[2]);
	setScanlineMask(pos[2],pos[3]);
	setScanlineMask(pos[3],pos[0]);
	for(int x = min_x + 1;x < max_x - 1;x++){
		int min_y = tMax(scanline[x].x + 1,0);
		int max_y = tMin(scanline[x].y - 1,RD_MASK_Y);
		for(int y = min_y;y < max_y;y++)
			mask[x][y] = true;
	}
}

void insertMask(vec3_t point[8]){
	vec2_t screen[8];
	bool is_behind[8] = {false,false,false,false,false,false,false,false};
	for(int i = 0;i < 8;i++){
		vec3_t point_n = pointToScreenZ(point[i]);
		if(!point_n.z){
			is_behind[i] = true;
			continue;
		}
		screen[i].x = point_n.x / RD_MASK_SIZE;
		screen[i].y = point_n.y / RD_MASK_SIZE;
	}
	if(!is_behind[0] && !is_behind[1] && !is_behind[3] && !is_behind[2])
		insertMaskQuad((vec2_t[]){screen[0],screen[1],screen[3],screen[2]});
	if(!is_behind[4] && !is_behind[5] && !is_behind[7] && !is_behind[6])
		insertMaskQuad((vec2_t[]){screen[4],screen[5],screen[7],screen[6]});
	if(!is_behind[0] && !is_behind[1] && !is_behind[5] && !is_behind[4])
		insertMaskQuad((vec2_t[]){screen[0],screen[1],screen[5],screen[4]});
	if(!is_behind[2] && !is_behind[3] && !is_behind[7] && !is_behind[6])
		insertMaskQuad((vec2_t[]){screen[2],screen[3],screen[7],screen[6]});
	if(!is_behind[1] && !is_behind[2] && !is_behind[6] && !is_behind[5])
		insertMaskQuad((vec2_t[]){screen[1],screen[2],screen[6],screen[5]});
	if(!is_behind[3] && !is_behind[0] && !is_behind[4] && !is_behind[7])
		insertMaskQuad((vec2_t[]){screen[3],screen[0],screen[4],screen[7]});
}

bool readMaskQuad(vec2_t pos[4]){
	int max_x = tMax(tMax(pos[0].x,pos[1].x),tMax(pos[2].x,pos[3].x));
	int min_x = tMin(tMin(pos[0].x,pos[1].x),tMin(pos[2].x,pos[3].x));
	max_x = tMin(max_x,RD_MASK_X);
	min_x = tMax(min_x,0);
	for(int i = min_x;i < max_x;i++){
		scanline[i].x = RD_MASK_Y;
		scanline[i].y = 0;
	}
	setScanlineMask(pos[0],pos[1]);
	setScanlineMask(pos[1],pos[2]);
	setScanlineMask(pos[2],pos[3]);
	setScanlineMask(pos[3],pos[0]);
	bool looped = false;
	for(int x = min_x;x < max_x;x++){
		int min_y = tMax(scanline[x].x,0);
		int max_y = tMin(scanline[x].y,RD_MASK_Y);
		for(int y = min_y;y < max_y;y++){
				if(!mask[x][y])
				return true;
			looped = true;
		}
	}
	if(looped)
		return false;
	return true;
}

bool readMask(vec3_t point[8]){
	vec2_t screen[8];
	for(int i = 0;i < 8;i++){
		vec3_t point_n = pointToScreenZ(point[i]);
		if(!point_n.z)
			return true;
		screen[i].x = point_n.x / RD_MASK_SIZE;
		screen[i].y = point_n.y / RD_MASK_SIZE;
	}
	if(readMaskQuad((vec2_t[]){screen[0],screen[1],screen[3],screen[2]}))
		return true;
	if(readMaskQuad((vec2_t[]){screen[4],screen[5],screen[7],screen[6]}))
		return true;
	if(readMaskQuad((vec2_t[]){screen[0],screen[1],screen[5],screen[4]}))
		return true;
	if(readMaskQuad((vec2_t[]){screen[2],screen[3],screen[7],screen[6]}))
		return true;
	if(readMaskQuad((vec2_t[]){screen[1],screen[2],screen[6],screen[5]}))
		return true;
	if(readMaskQuad((vec2_t[]){screen[3],screen[0],screen[4],screen[7]}))
		return true;
	return false;
}

void addSubSquare(vec3_t block_pos,ivec3 axis,node_t node,float block_size,int side,float detail,float sqf_x,float sqf_y,float sub_size){
	block_t* block = dynamicArrayGet(block_array,node.index);
	if(material_array[node.type].flags & (MAT_REFLECT | MAT_LIQUID | MAT_REFRACT))
		detail *= 8.0f;
	material_t material = material_array[node.type];
	uint32_t lm_size = GETLIGHTMAPSIZE(node.depth);
	uint32_t square_size = tMax(tMin(detail,lm_size) * sub_size,1);
	uint32_t count = 0;

	while(square_size > 1){
		square_size >>= 1;
		count++;
	}
	while(count--)
		square_size <<= 1;
	
	float square_size_lm = sub_size / square_size;
	float block_size_sub = block_size / square_size;
	
	vec3_t pos;
	pos.a[axis.y] = block_pos.a[axis.y];
	pos.a[axis.x] = block_pos.a[axis.x];
	pos.a[axis.z] = block_pos.a[axis.z];

	static vec3_t point_grid[4096];
	static vec3_t lighting_grid[4096];
	static fog_t fog_color_cache[4096];

	for(int i = 0;i < square_size + 1;i++){
		for(int j = 0;j < square_size + 1;j++){
			vec2_t lm = {
				square_size_lm * j + sqf_x,
				square_size_lm * i + sqf_y
			};
			uint32_t location = i * (square_size + 1) + j;
			fog_color_cache[location] = calculateFogColor(pos);
			point_grid[location] = pos;
			if(material.flags & MAT_LUMINANT){
				lighting_grid[location] = material.luminance;
			}
			
			else if(material.flags & MAT_POWER){
				lighting_grid[location].r = tFract((tHash(block->power_grid[0]) & 255) * 0.1f);
				lighting_grid[location].g = tFract((tHash(block->power_grid[0] + 124) & 255) * 0.1f);
				lighting_grid[location].b = tFract((tHash(block->power_grid[0] + 1354) & 255) * 0.1f);
			}
			
			else if(block->power){
				lighting_grid[location] = vec3mulR(material.luminance,block->power);
			}
			else if(material.flags & MAT_REFLECT){
				vec3_t normal = VEC3_ZERO;
				normal.a[axis.z] = 1.0f;
				normal.a[axis.x] = 0.0f;
				normal.a[axis.y] = 0.0f;
				normal = vec3normalizeR(normal);
				vec3_t ray_angle = vec3reflect(vec3normalizeR(vec3subvec3R(pos,camera.pos)),normal);

				vec3_t luminance = VEC3_ZERO;

				vec3addvec3(&luminance,rayGetLuminance(pos,initTraverse(pos).pos,ray_angle,initTraverse(pos).node,0));
				lighting_grid[location] = luminance;
				luminance = getLuminance(block->luminance[side],lm,node.depth);
				lighting_grid[location] = vec3avgvec3R(luminance,lighting_grid[location]);
				vec3mulvec3(&lighting_grid[location],material.luminance);
			}
			else if(material.flags & MAT_LIQUID){
				vec2_t wave_pos = {
					pos.a[axis.x] + global_tick * 0.05f,
					pos.a[axis.y] + global_tick * 0.05f
				};
				vec2_t wave_pos_n = {
					pos.a[axis.x] - global_tick * 0.05f,
					pos.a[axis.y] - global_tick * 0.05f
				};
				vec2_t wave = {
					cosf(wave_pos.x * 0.56f) + cosf(wave_pos.x * 1.67f) + cosf(wave_pos_n.y * 1.25f) + cosf(wave_pos_n.y * 0.94f),
					cosf(wave_pos.y * 1.38f) + cosf(wave_pos.y * 2.53f) + cosf(wave_pos_n.x * 0.84f) + cosf(wave_pos_n.x * 1.89f),
				};
				wave.x *= block->disturbance;
				wave.y *= block->disturbance;
				vec3_t normal = vec3normalizeR((vec3_t){wave.x,wave.y,1.0f});
				vec3_t ray_angle = vec3normalizeR(vec3subvec3R(pos,camera.pos));
				vec3_t luminance = VEC3_ZERO;

				vec3_t reflect = vec3reflect(ray_angle,normal);
				vec3_t refract = vec3refract(ray_angle,normal,0.751879f);

				vec3addvec3(&luminance,rayGetLuminance(pos,initTraverse(pos).pos,reflect,initTraverse(pos).node,0));
				vec3addvec3(&luminance,rayGetLuminance(pos,initTraverse(pos).pos,refract,initTraverse(pos).node,0));
				lighting_grid[location] = vec3mulR(luminance,0.5f);
				vec3mulvec3(&lighting_grid[location],material.luminance);
			}
			else if(material.flags & MAT_REFRACT){
				vec3_t luminance = VEC3_ZERO;
				vec3_t ray_angle = vec3normalizeR(vec3subvec3R(pos,camera.pos));
				vec3_t normal = VEC3_ZERO;
				normal.a[axis.z] = 1.0f;
				vec3_t ray_pos = pos;
				ray_pos.a[axis.z] += ray_angle.a[axis.z] < 0.0f ? 0.001f : -0.001f; 
				vec3addvec3(&luminance,rayGetLuminance(ray_pos,initTraverse(ray_pos).pos,ray_angle,initTraverse(ray_pos).node,0));
				lighting_grid[location] = vec3mulR(luminance,1.0f);
				vec3mulvec3(&lighting_grid[location],material.luminance);
			}
			else{
				int lm_location = (int)(lm.x * (lm_size)) * (int)(lm_size + 1) + lm.y * (lm_size);
				lighting_grid[location] = block->luminance[side][lm_location];
				vec3mul(&lighting_grid[location],camera.exposure);
			}
			lighting_grid[location] = (vec3_t){lighting_grid[location].b,lighting_grid[location].g,lighting_grid[location].r};
			pos.a[axis.x] += block_size_sub;
		}
		pos.a[axis.x] -= block_size_sub * (square_size + 1);
		pos.a[axis.y] += block_size_sub;
	}
	
	if(tool_select == 0 && material.flags & MAT_LUMINANT){
		for(int i = 0;i < square_size + 1;i++){
			for(int j = 0;j < square_size + 1;j++){
				uint32_t location = i * (square_size + 1) + j;
				vec3addvec3(&lighting_grid[location],material.luminance);
			}
		}
	}

	float x = block_pos.a[axis.x];
	float y = block_pos.a[axis.y];

	for(int i = 0;i < square_size;i++){
		for(int j = 0;j < square_size;j++){
			vec3_t point[4];	
			vec3_t screen_point[4];
			float distance[4];
			fog_t fog_color[4];
			vec3_t pos[4];

			pos[0] = point_grid[i * (square_size + 1) + j];
			pos[1] = point_grid[i * (square_size + 1) + j + 1];
			pos[2] = point_grid[(i + 1) * (square_size + 1) + j];
			pos[3] = point_grid[(i + 1) * (square_size + 1) + j + 1];

			point[0] = pointToScreenRenderer(pos[0]);
			point[1] = pointToScreenRenderer(pos[1]);
			point[2] = pointToScreenRenderer(pos[2]);
			point[3] = pointToScreenRenderer(pos[3]);

			fog_color[0] = fog_color_cache[i * (square_size + 1) + j];
			fog_color[1] = fog_color_cache[i * (square_size + 1) + j + 1];
			fog_color[2] = fog_color_cache[(i + 1) * (square_size + 1) + j];
			fog_color[3] = fog_color_cache[(i + 1) * (square_size + 1) + j + 1];

			for(int k = 0;k < 4;k++){
				screen_point[k].z = point[k].z;
				screen_point[k].y = point[k].x;
				screen_point[k].x = point[k].y;
			}
		
			vec3_t lighting[4];

			lighting[0] = lighting_grid[i * (square_size + 1) + j];
			lighting[1] = lighting_grid[i * (square_size + 1) + j + 1];
			lighting[2] = lighting_grid[(i + 1) * (square_size + 1) + j];
			lighting[3] = lighting_grid[(i + 1) * (square_size + 1) + j + 1];

			vec2_t text_crd = {
				tFractUnsigned(x * 0.25f) * material.texture_size.x + material.texture_pos.x,
				tFractUnsigned(y * 0.25f) * material.texture_size.y + material.texture_pos.y
			};
			vec2_t texture_scale = {
				block_size_sub * 0.25f * material.texture_size.x,
				block_size_sub * 0.25f * material.texture_size.y
			};
			vec2_t text_crd_end = {
				tMinf(text_crd.x + texture_scale.x,material.texture_size.x + material.texture_pos.x),
				tMinf(text_crd.y + texture_scale.y,material.texture_size.y + material.texture_pos.y)
			};
			triangles[triangle_count++] = (triangle_t){screen_point[0],text_crd,                   lighting[0],fog_color[0].density,fog_color[0].luminance};
			triangles[triangle_count++] = (triangle_t){screen_point[1],{text_crd_end.x,text_crd.y},lighting[1],fog_color[1].density,fog_color[1].luminance};
			triangles[triangle_count++] = (triangle_t){screen_point[3],text_crd_end,               lighting[3],fog_color[3].density,fog_color[3].luminance};
			triangles[triangle_count++] = (triangle_t){screen_point[0],text_crd,                   lighting[0],fog_color[0].density,fog_color[0].luminance};
			triangles[triangle_count++] = (triangle_t){screen_point[2],{text_crd.x,text_crd_end.y},lighting[2],fog_color[2].density,fog_color[2].luminance};
			triangles[triangle_count++] = (triangle_t){screen_point[3],text_crd_end,               lighting[3],fog_color[3].density,fog_color[3].luminance};
			x += block_size_sub;
		}
		x -= block_size;
		y += block_size_sub;
	}
}

bool inScreenSpace(vec3_t* point,uint32_t ammount){
	bool outside[8] = {false,false,false,false,false,false,false,false};
	uint32_t z_count = 0;
	for(int i = 0;i < ammount;i++){
		vec3_t point2 = pointToScreenZ(point[i]);
		if(!point2.z)
			continue;
		vec2_t screen;
		screen.x = point2.x / WND_RESOLUTION.x * 2.0f - 1.0f;
		screen.y = point2.y / WND_RESOLUTION.y * 2.0f - 1.0f;
		if(screen.x > -1.0f && screen.x < 1.0f && screen.y > -1.0f && screen.y < 1.0f)
			return true;
		if(screen.x < -1.0f){
			if(screen.y < -1.0f)
				outside[0] = true;
			else if(screen.y > 1.0f)
				outside[1] = true;
			else
				outside[2] = true;
		}
		else if(screen.x > 1.0f){
			if(screen.y < -1.0f)
				outside[3] = true;
			else if(screen.y > 1.0f)
				outside[4] = true;
			else
				outside[5] = true;
		}
		else if(screen.y < 1.0f)
			outside[6] = true;
		else 
			outside[7] = true;
	}
	if(outside[0] && (outside[4] || outside[5] || outside[7]))
		return true;
	if(outside[1] && (outside[3] || outside[5] || outside[6]))
		return true;
	if(outside[2] && (outside[3] || outside[4] || outside[5] || outside[6] || outside[7]))
		return true;
	if(outside[3] && (outside[1] || outside[2] || outside[7]))
		return true;
	if(outside[4] && (outside[0] || outside[2] || outside[6]))
		return true;
	if(outside[5] && (outside[0] || outside[1] || outside[2] || outside[6] || outside[7]))
		return true;
	return false;
}
#include <math.h> 

#define LOD_NORMAL (1800.0f * 2.0f)

void test(node_t node,vec3_t block_pos,ivec3 axis,float block_size,int side,float x,float y,float size,uint32_t depth){
	if(block_size < 0.05f)
		return;
	vec3_t point[4];
	point[0] = block_pos;
	point[1] = block_pos;
	point[2] = block_pos;
	point[3] = block_pos;
	point[1].a[axis.y] += block_size;
	point[2].a[axis.x] += block_size;
	point[3].a[axis.x] += block_size;
	point[3].a[axis.y] += block_size;

	vec3_t screen_quad[4];
	screen_quad[0] = pointToScreenZ(point[0]);
	screen_quad[1] = pointToScreenZ(point[1]);
	screen_quad[2] = pointToScreenZ(point[2]);
	screen_quad[3] = pointToScreenZ(point[3]);
	vec2_t screen_quad_2[4] = {
		screen_quad[0].x / RD_MASK_SIZE,screen_quad[0].y / RD_MASK_SIZE,
		screen_quad[1].x / RD_MASK_SIZE,screen_quad[1].y / RD_MASK_SIZE,
		screen_quad[2].x / RD_MASK_SIZE,screen_quad[2].y / RD_MASK_SIZE,
		screen_quad[3].x / RD_MASK_SIZE,screen_quad[3].y / RD_MASK_SIZE
	};
	if(!screen_quad[0].z && !screen_quad[1].z && !screen_quad[2].z && !screen_quad[3].z)
		return;
	if(!screen_quad[0].z || !screen_quad[1].z || !screen_quad[2].z || !screen_quad[3].z){
		block_size *= 0.5f;
		size *= 0.5f;
		for(int i = 0;i < 2;i++){
			for(int j = 0;j < 2;j++){
				test(node,block_pos,axis,block_size,side,x,y,size,depth + 1);
				block_pos.a[axis.x] += block_size;
				x += size;
			}
			block_pos.a[axis.x] -= block_size * 2.0f;
			block_pos.a[axis.y] += block_size;
			x -= size * 2.0f;
			y += size;
		}
		return;
	}
	
	if(!readMaskQuad((vec2_t[]){screen_quad_2[0],screen_quad_2[1],screen_quad_2[3],screen_quad_2[2]}))
		return;
	if(!inScreenSpace(point,4))
		return;
		
	float min_distance = tMinf(tMinf(screen_quad[0].z,screen_quad[1].z),tMinf(screen_quad[2].z,screen_quad[3].z));
	uint32_t distance_level;               
	float detail_multiplier;
	vec3_t offset;
	offset.a[axis.x] = block_size * 0.5f;
	offset.a[axis.y] = block_size * 0.5f;
	offset.a[axis.z] = 0.0f;
	vec3_t rel_pos = vec3subvec3R(vec3addvec3R(block_pos,offset),camera.pos);
	vec3_t dir = vec3normalizeR(rel_pos);
	float detail = tMaxf(min_distance,1.0f);
	
	detail = LOD_NORMAL / sqrtf(detail);
	detail *= sqrtf(tAbsf(dir.a[axis.z]));
	detail /= (float)(1 << node.depth);
	addSubSquare(block_pos,axis,node,block_size,side,detail,x,y,size);
}

bool cubeInScreenSpace(vec3_t point[8]){
	bool outside[4] = {false,false,false,false};
	uint32_t z_count = 0;
	for(int i = 0;i < 8;i++){
		vec3_t point2 = pointToScreenZ(point[i]);
		if(!point2.z){
			z_count++;
			continue;
		}
		vec2_t screen;
		screen.x = point2.x / WND_RESOLUTION.x * 2.0f - 1.0f;
		screen.y = point2.y / WND_RESOLUTION.y * 2.0f - 1.0f;
		if(screen.x > -1.0f && screen.x < 1.0f && screen.y > -1.0f && screen.y < 1.0f)
			return true;
		if(screen.x < -1.0f)
			outside[0] = true;
		if(screen.x > 1.0f)
			outside[1] = true;
		if(screen.y < -1.0f)
			outside[2] = true;
		if(screen.y > 1.0f)
			outside[3] = true;
	}
	if(z_count)
		return z_count != 8;
	if(outside[0] && outside[1])
		return true;
	if(outside[2] && outside[3])
		return true;
	return false;
}

void gatherEntityTriangles(entity_t* entity,air_t* air){
	if(!entity->alive)
		return;
	vec3_t point = pointToScreenZ(entity->pos);
	if(!point.z)
		return;
	float size = 1.0f / point.z * entity->size;
	point.x = point.x / WND_RESOLUTION.x * 2.0f - 1.0f;
	point.y = point.y / WND_RESOLUTION.y * 2.0f - 1.0f;
	vec3_t screen_point[4];
	screen_point[0] = (vec3_t){point.y - size,point.x - size,1.0f};
	screen_point[1] = (vec3_t){point.y + size,point.x - size,1.0f};
	screen_point[2] = (vec3_t){point.y - size,point.x + size,1.0f};
	screen_point[3] = (vec3_t){point.y + size,point.x + size,1.0f};

	fog_t fog = calculateFogColor(entity->pos);
	float distance = 1.0f / (point.z * FOG_DENSITY + 1.0f);

	vec2_t texture_coords[4];
	texture_coords[0] = entity->texture_pos;
	texture_coords[1] = (vec2_t){entity->texture_pos.x + entity->texture_size.x,entity->texture_pos.y};
	texture_coords[2] = (vec2_t){entity->texture_pos.x,entity->texture_pos.y + entity->texture_size.y};
	texture_coords[3] = (vec2_t){entity->texture_pos.x + entity->texture_size.x,entity->texture_pos.y + entity->texture_size.y};

	vec3_t luminance = vec3mulvec3R(entity->color,air->luminance);
	triangles[triangle_count++] = (triangle_t){screen_point[0],texture_coords[0],luminance,fog.density,fog.luminance};
	triangles[triangle_count++] = (triangle_t){screen_point[1],texture_coords[1],luminance,fog.density,fog.luminance};
	triangles[triangle_count++] = (triangle_t){screen_point[3],texture_coords[3],luminance,fog.density,fog.luminance};

	triangles[triangle_count++] = (triangle_t){screen_point[0],texture_coords[0],luminance,fog.density,fog.luminance};
	triangles[triangle_count++] = (triangle_t){screen_point[2],texture_coords[2],luminance,fog.density,fog.luminance};
	triangles[triangle_count++] = (triangle_t){screen_point[3],texture_coords[3],luminance,fog.density,fog.luminance};
}

void sceneGatherTriangles(uint32_t node_ptr){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3mul(&block_pos,block_size);

	vec3_t point[8];

	point[0] = (vec3_t){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[1] = (vec3_t){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + block_size};
	point[2] = (vec3_t){block_pos.x + 0.0f,block_pos.y + block_size,block_pos.z + 0.0f};
	point[3] = (vec3_t){block_pos.x + 0.0f,block_pos.y + block_size,block_pos.z + block_size};
	point[4] = (vec3_t){block_pos.x + block_size,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[5] = (vec3_t){block_pos.x + block_size,block_pos.y + 0.0f,block_pos.z + block_size};
	point[6] = (vec3_t){block_pos.x + block_size,block_pos.y + block_size,block_pos.z + 0.0f};
	point[7] = (vec3_t){block_pos.x + block_size,block_pos.y + block_size,block_pos.z + block_size};
	if(node->type == BLOCK_PARENT){
		if(!cubeInScreenSpace(point,8) || !readMask(point))
			return;
		static const uint32_t order[8][8] = {
			{0,1,2,4,3,5,6,7},
			{1,0,3,5,2,7,4,6},
			{2,0,3,6,1,4,7,5},
			{3,1,2,7,0,5,6,4},
			{4,5,6,0,7,1,2,3},
			{5,7,4,1,6,3,0,2},
			{6,4,7,2,5,0,3,1},
			{7,5,6,3,4,1,2,0},
		};
		vec3_t pos = {
			node->pos.x * block_size,
			node->pos.y * block_size,
			node->pos.z * block_size
		};
		vec3_t rel_pos = vec3subvec3R(camera.pos,pos);
		ivec3 i_pos = {
			rel_pos.x / block_size * 2.0f,
			rel_pos.y / block_size * 2.0f,
			rel_pos.z / block_size * 2.0f
		};
		i_pos.x = tMax(tMin(i_pos.x,1),0);
		i_pos.y = tMax(tMin(i_pos.y,1),0);
		i_pos.z = tMax(tMin(i_pos.z,1),0);
		uint32_t order_id = i_pos.x + i_pos.y * 2 + i_pos.z * 4;
		for(int i = 0;i < 8;i++){
			uint32_t index = order[order_id][i];
			/*
			if(node_root[node.child_s[index]].air){
				int entity_index = node_root[node.child_s[index]].air->entity;
				while(entity_index != -1){
					entity_t* entity = &entity_array[node_root[node.child_s[index]].air->entity];
					gatherEntityTriangles(entity,node_root[node.child_s[index]].air);
					entity_index = entity->next_entity;
				}
				node_root[node.child_s[index]].air->entity = -1;
			}
			*/
			sceneGatherTriangles(node->child_s[index]);
		}
		return;
	}
	if(node->type == BLOCK_AIR || !inScreenSpace(point,8))
		return;

	vec3_t rel_pos = vec3subvec3R(block_pos,camera.pos);

	float block_size_sub = block_size / GETLIGHTMAPSIZE(node->depth);

	float max_distance = 0.0f;
	float min_distance = 999999.0f;
	float min_x = WND_RESOLUTION.x;
	float max_x = 0;
	float min_y = WND_RESOLUTION.y;
	float max_y = 0;

	vec3_t screen_point[8];

	for(int i = 0;i < 8;i++){
		screen_point[i] = pointToScreenZ(point[i]);
		if(screen_point[i].z > max_distance)
			max_distance = screen_point[i].z;
		if(screen_point[i].z < min_distance)
			min_distance = screen_point[i].z;
		if(!screen_point[i].z)
			continue;
		if(screen_point[i].x < min_x)
			min_x = screen_point[i].x;
		if(screen_point[i].x > max_x)
			max_x = screen_point[i].x;
		if(screen_point[i].y < min_y)
			min_y = screen_point[i].y;
		if(screen_point[i].y > max_y)
			max_y = screen_point[i].y;
	}
	material_t material = material_array[node->type];
	block_t* block = dynamicArrayGet(block_array,node->index);

	uint32_t distance_level;               
	float detail_multiplier;
	if(material.flags & MAT_LIQUID){
		if(rel_pos.z + block_size * block->ammount < 0.0f){
			vec3_t pos = block_pos;
			pos.z += block_size * tMinf(block->ammount,1.0f);
			test(*node,pos,(ivec3){VEC3_X,VEC3_Y,VEC3_Z},block_size,5,0.0f,0.0f,1.0f,0);
		}
		return;
	}
	ivec3 axis_table[] = {
		{VEC3_Y,VEC3_Z,VEC3_X},
		{VEC3_X,VEC3_Z,VEC3_Y},
		{VEC3_X,VEC3_Y,VEC3_Z}
	};
	for(int i = 0;i < 3;i++){
		
		if(rel_pos.a[i] > 0.0f && block->luminance[i * 2]){
			test(*node,block_pos,axis_table[i],block_size,i * 2,0.0f,0.0f,1.0f,0);
		}
		if(rel_pos.a[i] + block_size < 0.0f && block->luminance[i * 2 + 1]){
			vec3_t pos = block_pos;
			pos.a[i] += block_size;
			test(*node,pos,axis_table[i],block_size,i * 2 + 1,0.0f,0.0f,1.0f,0);
		}
	}
	if(max_x - min_x > RD_MASK_SIZE && max_y - min_y > RD_MASK_SIZE)
		insertMask(point);
}