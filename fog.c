#include "fog.h"
#include "tree.h"
#include "tmath.h"
#include "lighting.h"

#define FOG_STEP 15

void calculateNodeLuminance(block_node_t* node,uint quality){
	vec3 s_color = VEC3_ZERO;
	float block_size = (float)((1 << 20) >> node->depth) * MAP_SIZE_INV;
	vec3 node_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&node_pos,0.5f);
	vec3mul(&node_pos,block_size);
	traverse_init_t init = initTraverse(node_pos);
	float increment = (M_PI * 2.0f) / quality; 
	for(float x = 0.01f;x < M_PI * 2.0f;x += increment){
		for(float y = 0.01f;y < M_PI * 2.0f;y += increment){
			vec3 luminance = rayGetLuminance(init.pos,(vec2){x,y},init.node);
			vec3addvec3(&s_color,luminance);
		}
	}
	vec3div(&s_color,quality * quality);
	node->luminance = (vec3){s_color.b,s_color.g,s_color.r};
}

void calculateNodeLuminanceTree(uint node_ptr,uint quality){
	block_node_t* node = &node_root[node_ptr];
	if(node->block)
		return;
	calculateNodeLuminance(node,quality);
	for(int i = 0;i < 8;i++){
		if(!node->child_s[i])
			continue;
		calculateNodeLuminanceTree(node->child_s[i],quality);
	}
}

vec3 calculateFogColor(vec3 pos){
	vec3 ray_angle = vec3normalizeR(vec3subvec3R(camera_rd.pos,pos));
	vec3 ray_pos = pos; 
	vec3 color = VEC3_ZERO;
	float total_distance = vec3distance(ray_pos,camera_rd.pos);
	uint step_count = tMin(total_distance,FOG_STEP);
	ray_angle = vec3divR(vec3mulR(ray_angle,total_distance),(float)step_count);
	vec3addvec3(&ray_pos,ray_angle);
	float div = 0.0f;
	for(int i = 0;i < step_count;i++){
		traverse_init_t init = initTraverse(ray_pos);
		if(node_root[init.node].block){
			vec3addvec3(&ray_pos,ray_angle);
			continue;
		}
		float block_size = (float)MAP_SIZE / (1 << node_root[init.node].depth) * 2.0f;
		vec3 f_pos = {
			tFractUnsigned(ray_pos.x / block_size),
			tFractUnsigned(ray_pos.y / block_size),
			tFractUnsigned(ray_pos.z / block_size)
		};
		vec3addvec3(&color,node_root[init.node].luminance);
		vec3addvec3(&ray_pos,ray_angle);
		div += 1.0f;
	}
	vec3div(&color,tMaxf((float)div,1.0f));	
	return color;
}