#include "fog.h"
#include "tree.h"
#include "tmath.h"
#include "lighting.h"

#define FOG_STEP 15

static vec2 lookDirectionTable[6] = {
	{M_PI,0.0f},
	{0.0f,0.0f},
	{-M_PI * 0.5f,0.0f},
	{M_PI * 0.5f,0.0f},
	{0.0f,-M_PI * 0.5f},
	{0.0f,M_PI * 0.5f}
};

void calculateNodeLuminance(node_t* node,uint quality){
	vec3 s_color = VEC3_ZERO;
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3 node_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&node_pos,0.5f);
	vec3mul(&node_pos,block_size);
	traverse_init_t init = initTraverse(node_pos);
	float increment = (M_PI * 2.0f) / quality;
	for(int i = 0;i < 6;i++){
		vec2 dir = lookDirectionTable[i];
		dir.x = -1.0f + 1.0f / (quality + 1);
		dir.y = -1.0f + 1.0f / (quality + 1);
		int p_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[i >> 1];
		int p_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[i >> 1];
		for(int x = 0;x < quality;x++){
			for(int y = 0;y < quality;y++){
				vec3 angle;
				angle.a[p_x] = dir.x;
				angle.a[p_y] = dir.y;
				angle.a[i >> 1] = 1.0f - (tAbsf(dir.x) + tAbsf(dir.y)) * 0.5f;
				vec3addvec3(&s_color,rayGetLuminance(node_pos,init.pos,angle,init.node,0));
				dir.y += 2.0f / quality;
			}
			dir.y -= 2.0f;
			dir.x += 2.0f / quality;
		}
	}
	/*
	for(int i = 0;i < quality * quality;i++){
		vec3 luminance = rayGetLuminance(node_pos,init.pos,vec3rnd(),init.node,0);
		if(luminance.r < 0.0f || luminance.g < 0.0f || luminance.b < 0.0f)
			continue;
		if(luminance.r > 999.0f || luminance.g > 999.0f || luminance.b > 999.0f)
			continue;
		vec3addvec3(&s_color,luminance);
	}
	vec3div(&s_color,quality * quality);
	*/
	vec3div(&s_color,quality * quality * 6);
	node->air->luminance = (vec3){s_color.b,s_color.g,s_color.r};
}

void calculateNodeLuminanceTree(uint node_ptr,uint quality){
	node_t* node = &node_root[node_ptr];
	if(node->block)
		return;
	if(node->air){
		calculateNodeLuminance(node,quality);
		return;
	}
	for(int i = 0;i < 8;i++)
		calculateNodeLuminanceTree(node->child_s[i],quality);
}

fog_t calculateFogColor(vec3 pos){
	vec3 ray_angle = vec3normalizeR(vec3subvec3R(camera.pos,pos));
	vec3 ray_pos = pos; 
	vec3addvec3(&ray_pos,vec3mulR(ray_angle,0.01f));
	float total_distance = vec3distance(ray_pos,camera.pos);
	traverse_init_t init = initTraverse(ray_pos);
	fog_t fog = treeRayFog(ray3Create(init.pos,ray_angle),init.node,ray_pos,total_distance);
	if(!fog.density){
		fog.density = 1.0f;
		return fog;
	}	
	fog.density = 1.0f / (fog.density + 1.0f);
	fog.luminance = vec3divR(fog.luminance,1.0f - fog.density);
	return fog;
}