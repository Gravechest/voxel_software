#include "fog.h"
#include "tree.h"
#include "tmath.h"
#include "lighting.h"

#define FOG_STEP 15

static vec2_t lookDirectionTable[6] = {
	{M_PI,0.0f},
	{0.0f,0.0f},
	{-M_PI * 0.5f,0.0f},
	{M_PI * 0.5f,0.0f},
	{0.0f,-M_PI * 0.5f},
	{0.0f,M_PI * 0.5f}
};

void calculateNodeLuminance(node_t* node,uint32_t quality){
	air_t* air = dynamicArrayGet(air_array,node->index);

	if(air->luminance.r && TRND1 > 0.05f)
		return;

	vec3_t s_color = VEC3_ZERO;
	float block_size = getBlockSize(node->depth);
	vec3_t node_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&node_pos,0.5f);
	vec3mul(&node_pos,block_size);
	traverse_init_t init = initTraverse(node_pos);
	for(int i = 0;i < 6;i++){
		vec2_t dir = lookDirectionTable[i];
		dir.x = -1.0f + 1.0f / (quality + 1);
		dir.y = -1.0f + 1.0f / (quality + 1);
		int p_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[i >> 1];
		int p_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[i >> 1];
		for(int x = 0;x < quality;x++){
			for(int y = 0;y < quality;y++){
				vec3_t angle;
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
	vec3div(&s_color,quality * quality * 6);
	
	if(!s_color.b)
		s_color.b = 0.001f;
	air->luminance = (vec3_t){s_color.b,s_color.g,s_color.r};
}

void calculateNodeLuminanceTree(uint32_t node_ptr,uint32_t quality){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 8;i++)
			calculateNodeLuminanceTree(node->child_s[i],quality);
		return;
	}
	if(node->type == BLOCK_AIR){
		calculateNodeLuminance(node,quality);
		return;
	}
}

fog_t calculateFogColor(vec3_t pos){
	vec3_t ray_angle = vec3normalizeR(vec3subvec3R(camera.pos,pos));
	vec3_t ray_pos = pos; 
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