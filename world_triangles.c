#include "source.h"
#include "world_triangles.h"
#include "tmath.h"
#include "fog.h"
#include "opengl.h"
#include "tree.h"
#include "lighting.h"

vec3 fog_color_cache[512 * 512];

char mask[RD_MASK_X][RD_MASK_Y];
ivec2 scanline[RD_MASK_X];

void setScanlineMask(vec2 pos_1,vec2 pos_2){
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

void insertMaskQuad(vec2 pos[4]){
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
	for(int x = min_x + 1;x < max_x - 1;x++){
		int min_y = tMax(scanline[x].x,-1) + 1;
		int max_y = tMin(scanline[x].y,RD_MASK_Y + 1) - 1;
		for(int y = min_y;y < max_y;y++)
			mask[x][y] = true;
	}
}

void insertMask(vec3 point[8]){
	vec2 screen[8];
	bool is_behind[8] = {false,false,false,false,false,false,false,false};
	for(int i = 0;i < 8;i++){
		vec3 point_n = pointToScreenZ(point[i]);
		if(!point_n.z){
			is_behind[i] = true;
			continue;
		}
		screen[i].x = point_n.x / RD_MASK_SIZE;
		screen[i].y = point_n.y / RD_MASK_SIZE;
	}
	if(!is_behind[0] && !is_behind[1] && !is_behind[3] && !is_behind[2])
		insertMaskQuad((vec2[]){screen[0],screen[1],screen[3],screen[2]});
	if(!is_behind[4] && !is_behind[5] && !is_behind[7] && !is_behind[6])
		insertMaskQuad((vec2[]){screen[4],screen[5],screen[7],screen[6]});
	if(!is_behind[0] && !is_behind[1] && !is_behind[5] && !is_behind[4])
		insertMaskQuad((vec2[]){screen[0],screen[1],screen[5],screen[4]});
	if(!is_behind[2] && !is_behind[3] && !is_behind[7] && !is_behind[6])
		insertMaskQuad((vec2[]){screen[2],screen[3],screen[7],screen[6]});
	if(!is_behind[1] && !is_behind[2] && !is_behind[6] && !is_behind[5])
		insertMaskQuad((vec2[]){screen[1],screen[2],screen[6],screen[5]});
	if(!is_behind[3] && !is_behind[0] && !is_behind[4] && !is_behind[7])
		insertMaskQuad((vec2[]){screen[3],screen[0],screen[4],screen[7]});
}

bool readMaskQuad(vec2 pos[4]){
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

bool readMask(vec3 point[8]){
	vec2 screen[8];
	for(int i = 0;i < 8;i++){
		vec3 point_n = pointToScreenZ(point[i]);
		if(!point_n.z)
			return true;
		screen[i].x = point_n.x / RD_MASK_SIZE;
		screen[i].y = point_n.y / RD_MASK_SIZE;
	}
	if(readMaskQuad((vec2[]){screen[0],screen[1],screen[3],screen[2]}))
		return true;
	if(readMaskQuad((vec2[]){screen[4],screen[5],screen[7],screen[6]}))
		return true;
	if(readMaskQuad((vec2[]){screen[0],screen[1],screen[5],screen[4]}))
		return true;
	if(readMaskQuad((vec2[]){screen[2],screen[3],screen[7],screen[6]}))
		return true;
	if(readMaskQuad((vec2[]){screen[1],screen[2],screen[6],screen[5]}))
		return true;
	if(readMaskQuad((vec2[]){screen[3],screen[0],screen[4],screen[7]}))
		return true;
	return false;
}

void addSquareEdge(vec3 block_pos,ivec3 axis,block_node_t node,float block_size,int side){
	material_t material = material_array[node.block->material];
	float texture_offset = (1.0f / TEXTURE_AMMOUNT) * material.texture_id;
	float x = block_pos.a[axis.x];
	float y = block_pos.a[axis.y];
	uint square_size = tMax((GETLIGHTMAPSIZE(node.depth)),1);
	float lm_size = GETLIGHTMAPSIZE(node.depth);
	float block_size_sub = block_size / square_size;
	for(int i = 0;i < square_size + 1;i++){
		vec3 pos;
		pos.a[axis.y] = block_pos.a[axis.y] + i * block_size_sub;
		pos.a[axis.x] = block_pos.a[axis.x];
		pos.a[axis.z] = block_pos.a[axis.z];
		for(int j = 0;j < square_size + 1;j++){
			fog_color_cache[i * (square_size + 1) + j] = calculateFogColor(pos);
			pos.a[axis.x] += block_size_sub;
		}
	}
	for(int i = 0;i < square_size;i++){
		for(int j = 0;j < square_size;j++){
			vec3 point[4];	
			vec3 fog_color[4];
			vec2 screen_point[4];
			float distance[4];
			material_t material = material_array[node.block->material];
			vec3 pos[4];
			pos[0].a[axis.x] = x;
			pos[0].a[axis.y] = y;
			pos[0].a[axis.z] = block_pos.a[axis.z];
			pos[1] = pos[0];
			pos[2] = pos[1];
			pos[3] = pos[2];
			pos[1].a[axis.y] += block_size_sub;
			pos[2].a[axis.x] += block_size_sub;
			pos[3].a[axis.x] += block_size_sub;
			pos[3].a[axis.y] += block_size_sub;
			point[0] = pointToScreenZ(pos[0]);
			point[1] = pointToScreenZ(pos[1]);
			point[2] = pointToScreenZ(pos[2]);
			point[3] = pointToScreenZ(pos[3]);
			distance[0] = 1.0f / (vec3distance(pos[0],camera_rd.pos) * FOG_DENSITY + 1.0f);
			distance[1] = 1.0f / (vec3distance(pos[1],camera_rd.pos) * FOG_DENSITY + 1.0f);
			distance[2] = 1.0f / (vec3distance(pos[2],camera_rd.pos) * FOG_DENSITY + 1.0f);
			distance[3] = 1.0f / (vec3distance(pos[3],camera_rd.pos) * FOG_DENSITY + 1.0f);
			fog_color[0] = fog_color_cache[i * (square_size + 1) + j];
			fog_color[1] = fog_color_cache[(i + 1) * (square_size + 1) + j];
			fog_color[2] = fog_color_cache[i * (square_size + 1) + j + 1];
			fog_color[3] = fog_color_cache[(i + 1) * (square_size + 1) + j + 1];

			for(int k = 0;k < 4;k++){
				screen_point[k].y = point[k].x / WND_RESOLUTION.x * 2.0f - 1.0f;
				screen_point[k].x = point[k].y / WND_RESOLUTION.y * 2.0f - 1.0f;
			}
			vec3 lighting[4];
			vec2 lm = {(1.0f / square_size * j),(1.0f / square_size * i)};	
			if(material.flags & MAT_LUMINANT){
				lighting[0] = material.luminance;
				lighting[1] = material.luminance;
				lighting[2] = material.luminance;
				lighting[3] = material.luminance;
				lighting[0] = (vec3){lighting[0].b,lighting[0].g,lighting[0].r};
				lighting[1] = (vec3){lighting[1].b,lighting[1].g,lighting[1].r};
				lighting[2] = (vec3){lighting[2].b,lighting[2].g,lighting[2].r};
				lighting[3] = (vec3){lighting[3].b,lighting[3].g,lighting[3].r};
			}
			else{
				lighting[0] = getLuminance(node.block->side[side],lm,node.depth);
				lighting[1] = getLuminance(node.block->side[side],vec2addvec2R(lm,(vec2){0.0f,1.0f / square_size}),node.depth);
				lighting[2] = getLuminance(node.block->side[side],vec2addvec2R(lm,(vec2){1.0f / square_size,0.0f}),node.depth);
				lighting[3] = getLuminance(node.block->side[side],vec2addvec2R(lm,(vec2){1.0f / square_size,1.0f / square_size}),node.depth);
				vec3mul(&lighting[0],camera_exposure);
				vec3mul(&lighting[1],camera_exposure);
				vec3mul(&lighting[2],camera_exposure);
				vec3mul(&lighting[3],camera_exposure);
				lighting[0] = (vec3){lighting[0].b,lighting[0].g,lighting[0].r};
				lighting[1] = (vec3){lighting[1].b,lighting[1].g,lighting[1].r};
				lighting[2] = (vec3){lighting[2].b,lighting[2].g,lighting[2].r};
				lighting[3] = (vec3){lighting[3].b,lighting[3].g,lighting[3].r};
			}
			if(point[0].z && point[1].z && point[2].z && point[3].z){
				vec2 text_crd = {tFractUnsigned(x * 0.25f) / TEXTURE_AMMOUNT + texture_offset,tFractUnsigned(y * 0.25f)};
				vec2 texture_scale = {block_size_sub * 0.25f / TEXTURE_AMMOUNT,block_size_sub * 0.25f};
				triangles[triangle_count + 0] = (triangles_t){screen_point[0],{text_crd.x,text_crd.y},lighting[0],distance[0],fog_color[0]};
				triangles[triangle_count + 1] = (triangles_t){screen_point[1],{text_crd.x,text_crd.y + texture_scale.y},lighting[1],distance[1],fog_color[1]};
				triangles[triangle_count + 2] = (triangles_t){screen_point[3],{tMinf(text_crd.x + texture_scale.x,0.2f + texture_offset),text_crd.y + texture_scale.y},lighting[3],distance[3],fog_color[3]};
				triangle_count += 3;
				triangles[triangle_count + 0] = (triangles_t){screen_point[0],{text_crd.x,text_crd.y},lighting[0],distance[0],fog_color[0]};
				triangles[triangle_count + 1] = (triangles_t){screen_point[2],{tMinf(text_crd.x + texture_scale.x,0.2f + texture_offset),text_crd.y},lighting[2],distance[2],fog_color[2]};
				triangles[triangle_count + 2] = (triangles_t){screen_point[3],{tMinf(text_crd.x + texture_scale.x,0.2f + texture_offset),text_crd.y + texture_scale.y},lighting[3],distance[3],fog_color[3]};
				triangle_count += 3;
				x += block_size_sub;
				continue;
			}
			for(int i2 = 0;i2 < 8;i2++){
				for(int j2 = 0;j2 < 8;j2++){
					pos[0].a[axis.x] = x + block_size_sub / 8.0f * i2;
					pos[0].a[axis.y] = y + block_size_sub / 8.0f * j2;
					pos[0].a[axis.z] = block_pos.a[axis.z];
					pos[1] = pos[0];
					pos[2] = pos[1];
					pos[3] = pos[2];
					pos[1].a[axis.y] += block_size_sub / 8.0f;
					pos[2].a[axis.x] += block_size_sub / 8.0f;
					pos[3].a[axis.x] += block_size_sub / 8.0f;
					pos[3].a[axis.y] += block_size_sub / 8.0f;
					point[0] = pointToScreenZ(pos[0]);
					point[1] = pointToScreenZ(pos[1]);
					point[2] = pointToScreenZ(pos[2]);
					point[3] = pointToScreenZ(pos[3]);
					distance[0] = 1.0f / (vec3distance(pos[0],camera_rd.pos) * FOG_DENSITY + 1.0f);
					distance[1] = 1.0f / (vec3distance(pos[1],camera_rd.pos) * FOG_DENSITY + 1.0f);
					distance[2] = 1.0f / (vec3distance(pos[2],camera_rd.pos) * FOG_DENSITY + 1.0f);
					distance[3] = 1.0f / (vec3distance(pos[3],camera_rd.pos) * FOG_DENSITY + 1.0f);
					if(!point[0].z || !point[1].z || !point[2].z || !point[3].z)
						continue;
					for(int k = 0;k < 4;k++){
						screen_point[k].y = point[k].x / WND_RESOLUTION.x * 2.0f - 1.0f;
						screen_point[k].x = point[k].y / WND_RESOLUTION.y * 2.0f - 1.0f;
					}
					vec2 lm = {(1.0f / square_size * j),(1.0f / square_size * i)};	
					vec3 lighting_sub[4];
					lighting_sub[0] = vec3mulR(lighting[0],(1.0f / 7.0f) * i2 * (1.0f / 7.0f) * j2);
					vec3addvec3(&lighting_sub[0],vec3mulR(lighting[1],(1.0f / 7.0f) * i2 * (1.0f - (1.0f / 7.0f) * j2)));
					vec3addvec3(&lighting_sub[0],vec3mulR(lighting[2],(1.0f - (1.0f / 7.0f) * i2) * (1.0f / 7.0f) * j2));
					vec3addvec3(&lighting_sub[0],vec3mulR(lighting[3],(1.0f - (1.0f / 7.0f) * i2) * (1.0f - (1.0f / 7.0f) * j2)));
					lighting_sub[1] = lighting_sub[0];
					lighting_sub[2] = lighting_sub[1];
					lighting_sub[3] = lighting_sub[2];
					vec2 text_crd = {
						tFractUnsigned(x * 0.25f) / TEXTURE_AMMOUNT + texture_offset,
						tFractUnsigned(y * 0.25f)
					};
					vec2 texture_scale = {block_size_sub * 0.25f / TEXTURE_AMMOUNT,block_size_sub * 0.25f};
					triangles[triangle_count + 0] = (triangles_t){screen_point[0],{text_crd.x,text_crd.y},lighting_sub[0],distance[0]};
					triangles[triangle_count + 1] = (triangles_t){screen_point[1],{text_crd.x,text_crd.y + texture_scale.y},lighting_sub[1],distance[1]};
					triangles[triangle_count + 2] = (triangles_t){screen_point[3],{tMinf(text_crd.x + texture_scale.x,0.2f + texture_offset),text_crd.y + texture_scale.y},lighting_sub[3],distance[3]};
					triangle_count += 3;
					triangles[triangle_count + 0] = (triangles_t){screen_point[0],{text_crd.x,text_crd.y},lighting_sub[0],distance[0]};
					triangles[triangle_count + 1] = (triangles_t){screen_point[2],{tMinf(text_crd.x + texture_scale.x,0.2f + texture_offset),text_crd.y},lighting_sub[2],distance[2]};
					triangles[triangle_count + 2] = (triangles_t){screen_point[3],{tMinf(text_crd.x + texture_scale.x,0.2f + texture_offset),text_crd.y + texture_scale.y},lighting_sub[3],distance[3]};
					triangle_count += 3;
				}
			}
			x += block_size_sub;
		}
		x -= block_size;
		y += block_size_sub;
	}
}

void addSquare(vec3 block_pos,ivec3 axis,block_node_t node,float block_size,int side,float detail){
	material_t material = material_array[node.block->material];
	vec3 pos_quad = block_pos;
	vec3 screen_quad[4];
	screen_quad[0] = pointToScreenZ(pos_quad);
	pos_quad.a[axis.x] += block_size;
	screen_quad[1] = pointToScreenZ(pos_quad);
	pos_quad.a[axis.x] -= block_size;
	pos_quad.a[axis.y] += block_size;
	screen_quad[2] = pointToScreenZ(pos_quad);
	pos_quad.a[axis.x] += block_size;
	screen_quad[3] = pointToScreenZ(pos_quad);
	vec2 screen_quad_2[4] = {
		screen_quad[0].x / RD_MASK_SIZE,screen_quad[0].y / RD_MASK_SIZE,
		screen_quad[1].x / RD_MASK_SIZE,screen_quad[1].y / RD_MASK_SIZE,
		screen_quad[2].x / RD_MASK_SIZE,screen_quad[2].y / RD_MASK_SIZE,
		screen_quad[3].x / RD_MASK_SIZE,screen_quad[3].y / RD_MASK_SIZE
	};
	
	if(!readMaskQuad((vec2[]){screen_quad_2[0],screen_quad_2[1],screen_quad_2[3],screen_quad_2[2]}))
		return;
	
	uint square_size = tMax(detail * GETLIGHTMAPSIZE(node.depth),1);
	float block_size_sub = block_size / square_size;
	for(int i = 0;i < square_size + 1;i++){
		vec3 pos;
		pos.a[axis.y] = block_pos.a[axis.y] + i * block_size_sub;
		pos.a[axis.x] = block_pos.a[axis.x];
		pos.a[axis.z] = block_pos.a[axis.z];
		for(int j = 0;j < square_size + 1;j++){
			fog_color_cache[i * (square_size + 1) + j] = calculateFogColor(pos);
			pos.a[axis.x] += block_size_sub;
		}
	}
	float texture_offset = (1.0f / TEXTURE_AMMOUNT) * material.texture_id;
	float x = block_pos.a[axis.x];
	float y = block_pos.a[axis.y];

	float lm_size = GETLIGHTMAPSIZE(node.depth);

	for(int i = 0;i < square_size;i++){
		for(int j = 0;j < square_size;j++){
			vec3 point[4];	
			vec2 screen_point[4];
			float distance[4];
			vec3 fog_color[4];
			vec3 pos[4];
			pos[0].a[axis.x] = x;
			pos[0].a[axis.y] = y;
			pos[0].a[axis.z] = block_pos.a[axis.z];
			pos[1] = pos[0];
			pos[2] = pos[1];
			pos[3] = pos[2];
			pos[1].a[axis.y] += block_size_sub;
			pos[2].a[axis.x] += block_size_sub;
			pos[3].a[axis.x] += block_size_sub;
			pos[3].a[axis.y] += block_size_sub;

			point[0] = pointToScreenZ(pos[0]);
			point[1] = pointToScreenZ(pos[1]);
			point[2] = pointToScreenZ(pos[2]);
			point[3] = pointToScreenZ(pos[3]);
			distance[0] = 1.0f / (vec3distance(pos[0],camera_rd.pos) * FOG_DENSITY + 1.0f);
			distance[1] = 1.0f / (vec3distance(pos[1],camera_rd.pos) * FOG_DENSITY + 1.0f);
			distance[2] = 1.0f / (vec3distance(pos[2],camera_rd.pos) * FOG_DENSITY + 1.0f);
			distance[3] = 1.0f / (vec3distance(pos[3],camera_rd.pos) * FOG_DENSITY + 1.0f);

			fog_color[0] = fog_color_cache[i * (square_size + 1) + j];
			fog_color[1] = fog_color_cache[(i + 1) * (square_size + 1) + j];
			fog_color[2] = fog_color_cache[i * (square_size + 1) + j + 1];
			fog_color[3] = fog_color_cache[(i + 1) * (square_size + 1) + j + 1];
			
			for(int k = 0;k < 4;k++){
				screen_point[k].y = point[k].x / WND_RESOLUTION.x * 2.0f - 1.0f;
				screen_point[k].x = point[k].y / WND_RESOLUTION.y * 2.0f - 1.0f;
			}
			
			vec2 lm = {(1.0f / square_size * j),(1.0f / square_size * i)};	
			vec3 lighting[4];
			material_t material = material_array[node.block->material];
			if(material.flags & MAT_LUMINANT){
				lighting[0] = material.luminance;
				lighting[1] = material.luminance;
				lighting[2] = material.luminance;
				lighting[3] = material.luminance;
				lighting[0] = (vec3){lighting[0].b,lighting[0].g,lighting[0].r};
				lighting[1] = (vec3){lighting[1].b,lighting[1].g,lighting[1].r};
				lighting[2] = (vec3){lighting[2].b,lighting[2].g,lighting[2].r};
				lighting[3] = (vec3){lighting[3].b,lighting[3].g,lighting[3].r};
			}
			else{
				lighting[0] = getLuminance(node.block->side[side],lm,node.depth);
				lighting[1] = getLuminance(node.block->side[side],vec2addvec2R(lm,(vec2){0.0f,1.0f / square_size}),node.depth);
				lighting[2] = getLuminance(node.block->side[side],vec2addvec2R(lm,(vec2){1.0f / square_size,0.0f}),node.depth);
				lighting[3] = getLuminance(node.block->side[side],vec2addvec2R(lm,(vec2){1.0f / square_size,1.0f / square_size}),node.depth);
				vec3mul(&lighting[0],camera_exposure);
				vec3mul(&lighting[1],camera_exposure);
				vec3mul(&lighting[2],camera_exposure);
				vec3mul(&lighting[3],camera_exposure);
				lighting[0] = (vec3){lighting[0].b,lighting[0].g,lighting[0].r};
				lighting[1] = (vec3){lighting[1].b,lighting[1].g,lighting[1].r};
				lighting[2] = (vec3){lighting[2].b,lighting[2].g,lighting[2].r};
				lighting[3] = (vec3){lighting[3].b,lighting[3].g,lighting[3].r};
			}
			vec2 text_crd = {
				tFractUnsigned(x * 0.25f) / TEXTURE_AMMOUNT + texture_offset,
				tFractUnsigned(y * 0.25f)
			};
			vec2 texture_scale = {block_size_sub * 0.25f / TEXTURE_AMMOUNT,block_size_sub * 0.25f};
			triangles[triangle_count + 0] = (triangles_t){screen_point[0],{text_crd.x,text_crd.y},lighting[0],distance[0],fog_color[0]};
			triangles[triangle_count + 1] = (triangles_t){screen_point[1],{text_crd.x,text_crd.y + texture_scale.y},lighting[1],distance[1],fog_color[1]};
			triangles[triangle_count + 2] = (triangles_t){screen_point[3],{tMinf(text_crd.x + texture_scale.x,(1.0f / TEXTURE_AMMOUNT) + texture_offset),text_crd.y + texture_scale.y},lighting[3],distance[3],fog_color[3]};
			triangle_count += 3;
			triangles[triangle_count + 0] = (triangles_t){screen_point[0],{text_crd.x,text_crd.y},lighting[0],distance[0],fog_color[0]};
			triangles[triangle_count + 1] = (triangles_t){screen_point[2],{tMinf(text_crd.x + texture_scale.x,(1.0f / TEXTURE_AMMOUNT) + texture_offset),text_crd.y},lighting[2],distance[2],fog_color[2]};
			triangles[triangle_count + 2] = (triangles_t){screen_point[3],{tMinf(text_crd.x + texture_scale.x,(1.0f / TEXTURE_AMMOUNT) + texture_offset),text_crd.y + texture_scale.y},lighting[3],distance[3],fog_color[3]};
			triangle_count += 3;
			x += block_size_sub;
		}
		x -= block_size;
		y += block_size_sub;
	}
}
bool blockInScreenSpace(vec3 point[8]){
	bool outside[8] = {false,false,false,false,false,false,false,false};
	uint z_count = 0;
	for(int i = 0;i < 8;i++){
		vec3 point2 = pointToScreenZ(point[i]);
		if(!point2.z)
			continue;
		vec2 screen;
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

bool cubeInScreenSpace(vec3 point[8]){
	bool outside[4] = {false,false,false,false};
	uint z_count = 0;
	for(int i = 0;i < 8;i++){
		vec3 point2 = pointToScreenZ(point[i]);
		if(!point2.z){
			z_count++;
			continue;
		}
		vec2 screen;
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

void sceneGatherTriangles(uint node_ptr){
	block_node_t node = node_root[node_ptr];
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
	vec3 block_pos = {node.pos.x,node.pos.y,node.pos.z};
	vec3mul(&block_pos,block_size);

	vec2 screen_point[8];
	vec3 point[8];

	point[0] = (vec3){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[1] = (vec3){block_pos.x + 0.0f,block_pos.y + 0.0f,block_pos.z + block_size};
	point[2] = (vec3){block_pos.x + 0.0f,block_pos.y + block_size,block_pos.z + 0.0f};
	point[3] = (vec3){block_pos.x + 0.0f,block_pos.y + block_size,block_pos.z + block_size};
	point[4] = (vec3){block_pos.x + block_size,block_pos.y + 0.0f,block_pos.z + 0.0f};
	point[5] = (vec3){block_pos.x + block_size,block_pos.y + 0.0f,block_pos.z + block_size};
	point[6] = (vec3){block_pos.x + block_size,block_pos.y + block_size,block_pos.z + 0.0f};
	point[7] = (vec3){block_pos.x + block_size,block_pos.y + block_size,block_pos.z + block_size};
	if(!node.block){
		if(!cubeInScreenSpace(point))
			return;
		if(!readMask(point))
			return;
		static const uint order[8][8] = {
			{0,1,2,4,3,5,6,7},
			{1,0,3,5,2,7,4,6},
			{2,0,3,6,1,4,7,5},
			{3,1,2,7,0,5,6,4},
			{4,5,6,0,7,1,2,3},
			{5,7,4,1,6,3,0,2},
			{6,4,7,2,5,0,3,1},
			{7,5,6,3,4,1,2,0},
		};
		vec3 pos = {node.pos.x * block_size,node.pos.y * block_size,node.pos.z * block_size};
		vec3 rel_pos = vec3subvec3R(camera_rd.pos,pos);
		ivec3 i_pos = {
			rel_pos.x / block_size * 2.0f,
			rel_pos.y / block_size * 2.0f,
			rel_pos.z / block_size * 2.0f
		};
		
		bool o_x = i_pos.x < -RD_DISTANCE || i_pos.x > RD_DISTANCE + 1;
		bool o_y = i_pos.y < -RD_DISTANCE || i_pos.y > RD_DISTANCE + 1;
		bool o_z = i_pos.z < -RD_DISTANCE || i_pos.z > RD_DISTANCE + 1;
		if(o_x || o_y || o_z)
			return;
		
		i_pos.x = tMax(tMin(i_pos.x,1),0);
		i_pos.y = tMax(tMin(i_pos.y,1),0);
		i_pos.z = tMax(tMin(i_pos.z,1),0);
		uint order_id = i_pos.x + i_pos.y * 2 + i_pos.z * 4;
		for(int i = 0;i < 8;i++){
			if(!node.child_s[order[order_id][i]])
				continue;
			sceneGatherTriangles(node.child_s[order[order_id][i]]);
		}
		return;
	}

	if(!blockInScreenSpace(point))
		return;

	vec3 rel_pos = vec3subvec3R(block_pos,camera_rd.pos);

	float block_size_sub = block_size / GETLIGHTMAPSIZE(node.depth);

	float max_distance = 0.0f;
	float min_distance = 999999.0f;
	float min_x = WND_RESOLUTION.x;
	float max_x = 0;
	float min_y = WND_RESOLUTION.y;
	float max_y = 0;

	for(int i = 0;i < 8;i++){
		vec3 pos = pointToScreenZ(point[i]);
		if(pos.z > max_distance)
			max_distance = pos.z;
		if(pos.z < min_distance)
			min_distance = pos.z;
		if(!pos.z)
			continue;
		if(pos.x < min_x)
			min_x = pos.x;
		if(pos.x > max_x)
			max_x = pos.x;
		if(pos.y < min_y)
			min_y = pos.y;
		if(pos.y > max_y)
			max_y = pos.y;
	}

	uint distance_level;               
	float detail_multiplier;
	if(rel_pos.z > 0.0f && node.block->side[4].luminance_p){
		vec3 dir = vec3normalizeR(vec3addvec3R(rel_pos,(vec3){block_size * 0.5f,block_size * 0.5f,0.0f}));
		_BitScanReverse(&distance_level,tMax((uint)(tMaxf(min_distance,0.01f) / tAbsf(dir.z) * RD_LOD),1));
		if(!min_distance)
			addSquareEdge(block_pos,(ivec3){VEC3_X,VEC3_Y,VEC3_Z},node,block_size,4);
		else{
			detail_multiplier = 1.0f / (float)(1 << distance_level); 
			addSquare(block_pos,(ivec3){VEC3_X,VEC3_Y,VEC3_Z},node,block_size,4,detail_multiplier);
		}
		
	}
	if(rel_pos.z + block_size < 0.0f && node.block->side[5].luminance_p){
		vec3 pos = block_pos;
		pos.z += block_size;
		vec3 dir = vec3normalizeR(vec3addvec3R(rel_pos,(vec3){block_size * 0.5f,block_size * 0.5f,block_size}));
		_BitScanReverse(&distance_level,tMax((uint)(tMaxf(min_distance,0.01f) / tAbsf(dir.z) * RD_LOD),1));
		if(!min_distance)
			addSquareEdge(pos,(ivec3){VEC3_X,VEC3_Y,VEC3_Z},node,block_size,5);
		else{
			detail_multiplier = 1.0f / (float)(1 << distance_level);
			addSquare(pos,(ivec3){VEC3_X,VEC3_Y,VEC3_Z},node,block_size,5,detail_multiplier);
		}
		
	}
	if(rel_pos.x > 0.0f && node.block->side[0].luminance_p){
		vec3 dir = vec3normalizeR(vec3addvec3R(rel_pos,(vec3){0.0f,block_size * 0.5f,block_size * 0.5f}));
		_BitScanReverse(&distance_level,tMax((uint)(tMaxf(min_distance,0.01f) / tAbsf(dir.x) * RD_LOD),1));
		if(!min_distance)
			addSquareEdge(block_pos,(ivec3){VEC3_Y,VEC3_Z,VEC3_X},node,block_size,0);
		else{
			detail_multiplier = 1.0f / (float)(1 << distance_level);
			addSquare(block_pos,(ivec3){VEC3_Y,VEC3_Z,VEC3_X},node,block_size,0,detail_multiplier);
		}

	}
	if(rel_pos.x + block_size < 0.0f && node.block->side[1].luminance_p){
		vec3 pos = block_pos;
		pos.x += block_size;
		vec3 dir = vec3normalizeR(vec3addvec3R(rel_pos,(vec3){block_size,block_size * 0.5f,block_size * 0.5f}));
		_BitScanReverse(&distance_level,tMax((uint)(tMaxf(min_distance,0.01f) / tAbsf(dir.x) * RD_LOD),1));
		if(!min_distance)
			addSquareEdge(pos,(ivec3){VEC3_Y,VEC3_Z,VEC3_X},node,block_size,1);
		else{
			detail_multiplier = 1.0f / (float)(1 << distance_level);
			addSquare(pos,(ivec3){VEC3_Y,VEC3_Z,VEC3_X},node,block_size,1,detail_multiplier);
		}

	}
	if(rel_pos.y > 0.0f && node.block->side[2].luminance_p){
		vec3 dir = vec3normalizeR(vec3addvec3R(rel_pos,(vec3){block_size * 0.5f,0.0f,block_size * 0.5f}));
		_BitScanReverse(&distance_level,tMax((uint)(tMaxf(min_distance,0.01f) / tAbsf(dir.y) * RD_LOD),1));
		if(!min_distance)
			addSquareEdge(block_pos,(ivec3){VEC3_X,VEC3_Z,VEC3_Y},node,block_size,2);
		else{
			detail_multiplier = 1.0f / (float)(1 << distance_level);
			addSquare(block_pos,(ivec3){VEC3_X,VEC3_Z,VEC3_Y},node,block_size,2,detail_multiplier);
		}
	}
	if(rel_pos.y + block_size < 0.0f && node.block->side[3].luminance_p){
		vec3 pos = block_pos;
		pos.y += block_size;
		vec3 dir = vec3normalizeR(vec3addvec3R(rel_pos,(vec3){0.0f,block_size,0.0f}));
		_BitScanReverse(&distance_level,tMax((uint)(tMaxf(min_distance,0.01f) / tAbsf(dir.y) * RD_LOD),1));
		if(!min_distance)
			addSquareEdge(pos,(ivec3){VEC3_X,VEC3_Z,VEC3_Y},node,block_size,3);
		else{
			detail_multiplier = 1.0f / (float)(1 << distance_level);
			addSquare(pos,(ivec3){VEC3_X,VEC3_Z,VEC3_Y},node,block_size,3,detail_multiplier);
		}
	}

	if(max_x - min_x > RD_MASK_SIZE * 1.5f && max_y - min_y > RD_MASK_SIZE * 1.5f)
		insertMask(point);
}