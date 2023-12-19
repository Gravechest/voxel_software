#include "tmath.h"
#include "source.h"
#include "tree.h"
#include "lighting.h"
#include "fog.h"

static vec2 lookDirectionTable[6] = {
	{M_PI,0.0f},
	{0.0f,0.0f},
	{-M_PI * 0.5f,0.0f},
	{M_PI * 0.5f,0.0f},
	{0.0f,-M_PI * 0.5f},
	{0.0f,M_PI * 0.5f}
};

vec3 normal_table[6] = {
	{1.0f,0.0f,0.0f},
	{-1.0f,0.0f,0.0f},
	{0.0f,1.0f,0.0f},
	{0.0f,-1.0f,0.0f},
	{0.0f,0.0f,1.0f},
	{0.0f,0.0f,-1.0f}
};

bool lightMapRePosition(vec3* pos,int side){
	float block_size = 1.0f;

	int p_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
	int p_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];

	pos->a[p_x] -= block_size * 0.25f;
	pos->a[p_y] -= block_size * 0.25f;

	for(int x = 0;x < 2;x++){
		for(int y = 0;y < 2;y++){
			bool inside = insideBlock(*pos);
			if(!inside)
				return true;
			pos->a[p_x] += block_size * 0.5f;
		}
		pos->a[p_x] -= block_size;
		pos->a[p_y] += block_size * 0.5f;
	}
	return false;
}

static bool isExposed(vec3 pos){
	node_t* node = node_root;
	traverse_init_t init = initTraverse(pos);
	if(node[init.node].block)
		return true;
	return false;
}
#include <stdio.h>
#include <math.h>

vec3 getLuminance(block_side_t side,vec2 uv,uint depth){
	if(!side.luminance_p)
		return VEC3_ZERO;
	uint lm_size = GETLIGHTMAPSIZE(depth);

	vec2 fract_pos = {
		tFractUnsigned(uv.x * lm_size),
		tFractUnsigned(uv.y * lm_size)
	};
	
	uint lm_offset_x = tMinf(uv.x * lm_size,lm_size - 1);
	uint lm_offset_y = tMinf(uv.y * lm_size,lm_size - 1);

	uint location = lm_offset_x * (lm_size + 1) + lm_offset_y;

	vec3 lm_1 = side.luminance_p[location];
	vec3 lm_2 = side.luminance_p[location + 1];
	vec3 lm_3 = side.luminance_p[location + (lm_size + 1)];
	vec3 lm_4 = side.luminance_p[location + (lm_size + 1) + 1];

	vec3 mix_1 = vec3mixR(lm_1,lm_3,fract_pos.x);
	vec3 mix_2 = vec3mixR(lm_2,lm_4,fract_pos.x);

	return vec3mixR(mix_1,mix_2,fract_pos.y);
}
/*
float rayCubeIntersectionInside(vec3 ray_pos,vec3 cube_pos,vec3 angle,float size) {
	vec3 rel_pos = vec3subvec3R(cube_pos,ray_pos);
    vec3 m = vec3divFR(angle,1.0f);
    vec3 n = vec3mulvec3R(m,rel_pos);   
    vec3 k = vec3mulR(vec3absR(m),size);
    vec3 t1 = vec3subvec3R(vec3negR(n),k);
    vec3 t2 = vec3addvec3R(vec3negR(n),k);
    float tN = tMaxf(tMaxf(t1.x,t1.y),t1.z);
    float tF = tMinf(tMinf(t2.x,t2.y),t2.z);
    if(tN > tF || tF < 0.0f) 
		return 0.0f;
    return tF;	
}
*/

vec3 sideGetLuminance(vec3 ray_pos,vec3 angle,node_hit_t result,uint depth){
	node_t node = node_root[result.node];
	block_t* block = node.block;
	if(!block)
		return VEC3_ZERO;
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT)
		return material.luminance;
	if(block->material == BLOCK_LAMP){
		if(block->power)
			return vec3mulR(material.luminance,block->power);
	}
	if(material.flags & MAT_LIQUID)
		return VEC3_ZERO;
	if(material.flags & MAT_REFRACT){
		float distance = rayNodeGetDistance(ray_pos,node.pos,node.depth,angle,result.side);
		vec3 pos_new = vec3addvec3R(ray_pos,vec3mulR(angle,distance + 0.001f));
		traverse_init_t init = initTraverse(pos_new);
		vec3 normal = VEC3_ZERO;
		normal.a[result.side] = angle.a[result.side] < 0.0f ? 1.0f : -1.0f;
		float refract_1;
		node_t start_node = treeTraverse(ray_pos);
		if(start_node.air)
			refract_1 = 1.0f;
		else{
			if(material_array[start_node.block->material].flags & MAT_REFRACT)
				refract_1 = 1.66f;
			else
				refract_1 = 1.0f;
		}
		angle = vec3refract(angle,normal,refract_1 / 1.66f);
		node_hit_t result_inside = treeRayOnce(ray3Create(init.pos,angle),init.node,pos_new);
		distance = rayNodeGetDistance(pos_new,node_root[result_inside.node].pos,node_root[result_inside.node].depth,angle,result_inside.side);	
		pos_new = vec3addvec3R(pos_new,vec3mulR(angle,distance - 0.001f));
		init = initTraverse(pos_new);
		normal = VEC3_ZERO;
		normal.a[result_inside.side] = angle.a[result_inside.side] < 0.0f ? 1.0f : -1.0f;
		if(node_root[result_inside.node].air)
			refract_1 = 1.0f;
		else{
			if(node_root[result_inside.node].block){
				if(material_array[node_root[result_inside.node].block->material].flags & MAT_REFRACT)
					refract_1 = 1.66f;
				else
					refract_1 = 1.0f;
			}
			else
				refract_1 = 1.0f;
		}
		vec3 angle_f = vec3refract(angle,normal,1.66f / refract_1);
		if(!angle_f.x)
			angle_f = vec3reflect(angle,normal);

		float density = 1.0f / (distance + 1.0f);
		vec3 filter = {
			powf(material.luminance.r,distance),
			powf(material.luminance.g,distance),
			powf(material.luminance.b,distance)
		};
		return vec3mulvec3R(rayGetLuminance(pos_new,init.pos,angle_f,init.node,depth + 1),filter);
	}
	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << node.depth) * 2.0f;
	block_pos.x = node.pos.x * block_size;
	block_pos.y = node.pos.y * block_size;
	block_pos.z = node.pos.z * block_size;
	plane_t plane = getPlane(block_pos,angle,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,ray_pos);
	float dst = rayIntersectPlane(plane_pos,angle,plane.normal);
	vec3 hit_pos = vec3addvec3R(ray_pos,vec3mulR(angle,dst));
	vec2 uv = {
		(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
		(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
	};
	vec2 texture_uv = {
		hit_pos.a[plane.x] / block_size,
		hit_pos.a[plane.y] / block_size
	};
	float texture_x = texture_uv.x * TEXTURE_SCALE / material.texture.size / (8 << node.depth);
	float texture_y = texture_uv.y * TEXTURE_SCALE / material.texture.size / (8 << node.depth);
			
	int side = result.side * 2 + (angle.a[result.side] < 0.0f);
	uv.x = tMaxf(tMinf(uv.x,1.0f),0.0f);
	uv.y = tMaxf(tMinf(uv.y,1.0f),0.0f);

	vec3 hit_block_luminance = getLuminance(block->side[side],uv,node.depth);

	vec3 texture;
	if(material.texture.data){
		uint texture_x_i = (uint)(tFract(texture_x) * material.texture.size);
		uint texture_y_i = (uint)(tFract(texture_y) * material.texture.size);
		pixel_t text = material.texture.data[texture_x_i * material.texture.size + texture_y_i];

		texture = (vec3){text.r / 255.0f,text.g / 255.0f,text.b / 255.0f};
	}
	else
		texture = material.luminance;

	return vec3mulvec3R(texture,hit_block_luminance);
}

vec3 rayGetLuminance(vec3 ray_pos,vec3 init_pos,vec3 angle,uint init_node,uint depth){
	if(depth > 10)
		return VEC3_ZERO;
	node_hit_t result = treeRay(ray3Create(init_pos,angle),init_node,ray_pos);
	if(!result.node)
		return LUMINANCE_SKY;
	return sideGetLuminance(ray_pos,angle,result,depth);
}

vec3 rayGetLuminanceDir(vec3 init_pos,vec2 direction,uint init_node,vec3 ray_pos){
	return rayGetLuminance(ray_pos,init_pos,getLookAngle(direction),init_node,0);
}

void setLightMap(vec3* color,vec3 pos,int side,uint quality){
	int p_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
	int p_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];
	if(!color)
		return;
	vec2 dir = lookDirectionTable[side];
	vec3 normal = normal_table[side];
	traverse_init_t init = initTraverse(pos);
	node_t begin_node = treeTraverse(pos);
	if(!begin_node.air && !(material_array[begin_node.block->material].flags & MAT_LIQUID)){
		*color = VEC3_ZERO;
		return;
	}
	dir.x = -1.0f + 1.0f / (quality + 1);
	dir.y = -1.0f + 1.0f / (quality + 1);
	vec3 s_color = VEC3_ZERO;
	float weight_total = 0.0f;
	for(int x = 0;x < quality;x++){
		for(int y = 0;y < quality;y++){
			vec3 angle;
			angle.a[p_x] = dir.x;
			angle.a[p_y] = dir.y;
			angle.a[side >> 1] = 1.0f - (tAbsf(dir.x) + tAbsf(dir.y)) * 0.5f;
			if(!(side & 1))
				angle.a[side >> 1] = -angle.a[side >> 1];
			float weight = tAbsf(vec3dotR(angle,normal));
			vec3 luminance = rayGetLuminance(pos,init.pos,angle,init.node,0);
			vec3mul(&luminance,weight);
			vec3addvec3(&s_color,luminance);
			weight_total += weight;
			dir.y += 2.0f / quality;
		}
		dir.y -= 2.0f;
		dir.x += 2.0f / quality;
	}
	vec3div(&s_color,weight_total);
	*color = s_color;
}

vec3 posNew(vec3 pos,float size,int v_x,int v_y){
	pos.a[v_x] -= 1.0f / size * 0.5f;
	pos.a[v_y] -= 1.0f / size * 0.5f;
	for(int i = 0;i < 4;i++){
		for(int j = 0;j < 4;j++){
			if(!insideBlock(pos))
				return pos;
			pos.a[v_x] += 1.0f / size * 0.25f;
		}
		pos.a[v_x] -= 1.0f / size;
		pos.a[v_y] += 1.0f / size * 0.25f;
	}
	return (vec3){-1.0f,0.0f,0.0f};
}

void setLightSideBig(vec3* color,vec3 pos,int side,int depth,uint quality){
	if(!color)
		return;
	vec3add(&pos,0.5f);
	pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.02f;

	uint v_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
	uint v_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];

	uint size = GETLIGHTMAPSIZE(depth);

	pos.a[v_x] -= 0.5f - 0.5f / size;
	pos.a[v_y] -= 0.5f - 0.5f / size;

	for(int x = 1;x < size + 1;x++){
		for(int y = 1;y < size + 1;y++){
			vec3 l_pos = vec3divR(pos,(float)(1 << depth) * 0.5f);
			vec3mul(&l_pos,MAP_SIZE);
			if(insideBlock(l_pos)){
				color[x * (size + 2) + y] = (vec3){1.0f,1.0f,0.0f};
				continue;
				l_pos = posNew(pos,size,v_x,v_y);
				if(l_pos.x == -1.0f)
					continue;
			}
			setLightMap(&color[x * (size + 2) + y],l_pos,side,quality);
			vec3 result = color[x * (size + 2) + y];
			if(!result.r && !result.g && !result.b){
				vec3 avg_color = VEC3_ZERO;
				uint lm_size = 1 << tMax(LM_MAXDEPTH - depth,0);
				int count = 0;
				for(int i = 0;i < 4;i++){
					int dx = (int[]){0,0,-1,1}[i] + x;
					int dy = (int[]){-1,1,0,0}[i] + y;
					vec3 neighbour_pos = pos;
					neighbour_pos.a[v_x] += 1.0f / size * dx;
					neighbour_pos.a[v_y] += 1.0f / size * dy;
					if(!isExposed(neighbour_pos))
						continue;
					count++;
					vec3addvec3(&avg_color,color[dx * (lm_size + 2) + dy]);
				}
				color[x * (lm_size + 2) + y] = vec3mulR(avg_color,1.0f / count);
			}
			pos.a[v_y] += 1.0f / size;
		}
		pos.a[v_y] -= 1.0f;
		pos.a[v_x] += 1.0f / size;
	}
}

void setLightSideBigSingle(vec3* color,vec3 pos,uint side,uint depth,uint x,uint y,uint quality){
	vec3add(&pos,0.5f);
	pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.002f;

	int v_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
	int v_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];
	uint size = GETLIGHTMAPSIZE(depth);

	pos.a[v_x] -= 0.5f - 0.5f / size;
	pos.a[v_y] -= 0.5f - 0.5f / size;

	pos.a[v_x] += 1.0f / size * x;
	pos.a[v_y] += 1.0f / size * y;
	vec3div(&pos,(float)(1 << depth) * 0.5f);
	vec3mul(&pos,MAP_SIZE);
	setLightMap(color,pos,side,quality);
}

void setLightSide(vec3* color,vec3 pos,int side,int depth){
	vec3add(&pos,0.5f);
	pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.02f;
	vec3div(&pos,(float)(1 << depth) * 0.5f);
	vec3mul(&pos,MAP_SIZE);
	setLightMap(color,pos,side,LM_SIZE);
}

#include <Windows.h>

void updateLightMapSide(node_t node,vec3* lightmap,material_t material,uint side,uint quality){
	if(!lightmap)
		return;
	vec3 lm_pos = {node.pos.x,node.pos.y,node.pos.z};

	vec3add(&lm_pos,0.5f);
	lm_pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.002f;

	int v_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
	int v_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];

	uint size = GETLIGHTMAPSIZE(node.depth);

	float lm_size_sub = (1.0f / size);

	lm_pos.a[v_x] -= lm_size_sub * (size / 2.0f) - 0.001f;
	lm_pos.a[v_y] -= lm_size_sub * (size / 2.0f) - 0.001f;

	for(int sub_x = 0;sub_x < size + 1;sub_x++){
		for(int sub_y = 0;sub_y < size + 1;sub_y++){
			uint x = sub_x;
			uint y = sub_y;
			vec3 lm_pos_b = lm_pos;

			lm_pos_b.a[v_x] += lm_size_sub * x * 0.998f;
			lm_pos_b.a[v_y] += lm_size_sub * y * 0.998f;
			vec3 pos_c = lm_pos_b;
			lm_pos_b = vec3divR(lm_pos_b,(float)(1 << node.depth) * 0.5f);
			vec3mul(&lm_pos_b,MAP_SIZE);
			uint lightmap_location = x * (size + 1) + y;
			vec3 pre = lightmap[lightmap_location];
			if(insideBlock(lm_pos_b)){
				lm_pos_b = posNew(lm_pos_b,size,v_x,v_y);
				if(lm_pos_b.x == -1.0f)
					continue;
			}
			setLightMap(&lightmap[lightmap_location],lm_pos_b,side,quality);
			if(main_thread_status){
				main_thread_status = 2;
				while(main_thread_status == 2)
					Sleep(1);
			}
			vec3mulvec3(&lightmap[lightmap_location],material.luminance);
			vec3 post = lightmap[lightmap_location];
		}
	}
}

void updateLightMap(uint node_ptr,vec3 pos,uint depth,float radius){
	node_t node = node_root[node_ptr];
	float block_size = (float)((1 << 20) >> depth) * MAP_SIZE_INV * 0.5f;
	if(!node.block && !node.air){
		for(int i = 0;i < 8;i++){
			if(!node.child_s[i])
				continue;
			vec3 pos_2 = {
				node_root[node.child_s[i]].pos.x,
				node_root[node.child_s[i]].pos.y,
				node_root[node.child_s[i]].pos.z
			};
			vec3add(&pos_2,0.5f);
			vec3mul(&pos_2,block_size);
			float distance = sdCube(pos,pos_2,block_size);
			if(radius > distance)
				updateLightMap(node.child_s[i],pos,depth + 1,radius);
		}
		return;
	}
	if(node.air){
		calculateNodeLuminance(&node_root[node_ptr],32);
		return;
	}
	block_t* block = node.block;
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT)
		return;

	if(material.flags & MAT_LIQUID)
		return;

	for(int side = 0;side < 6;side++)
		updateLightMapSide(node,node.block->side[side].luminance_p,material,side,LM_QUALITY);
}

void lighting(){
	for(;;){
		while(light_new_stack_ptr){
			light_new_stack_t stack = light_new_stack[--light_new_stack_ptr];
			if(!stack.node)
				continue;
			uint size = GETLIGHTMAPSIZE(stack.node->depth);
			block_t* block = stack.node->block;
			if(!block)
				continue;
			uint side = stack.side;
			ivec3 pos_i = stack.node->pos;
			vec3 pos = {pos_i.x,pos_i.y,pos_i.z};
			vec3* lightmap = block->side[side].luminance_p;
			if(!lightmap)
				continue;
			updateLightMapSide(*stack.node,block->side[side].luminance_p,material_array[block->material],side,LM_SIZE);
		}
		updateLightMap(0,camera.pos,0,16.0f);
		if(main_thread_status){
			main_thread_status = 2;
			while(main_thread_status == 2)
				Sleep(1);
		}
	}
}

void calculateDynamicLight(vec3 pos,uint node_ptr,float radius,vec3 color){
	return;
	node_t node = node_root[node_ptr];
	if(node.air)
		return;
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
	vec3 block_pos = {node.pos.x,node.pos.y,node.pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	if(!node.block){
		float distance = sdCube(pos,block_pos,block_size);
		if(radius < distance)
			return;
		for(int i = 0;i < 8;i++)
			calculateDynamicLight(pos,node.child_s[i],radius,color);
		return;
	}
	if(material_array[node.block->material].flags & MAT_LIQUID)
		return;
	uint lm_size = GETLIGHTMAPSIZE(node.depth);

	static const vec3 normal_table[6] = {
		{1.0f,0.0f,0.0f},
		{-1.0f,0.0f,0.0f},
		{0.0f,1.0f,0.0f},
		{0.0f,-1.0f,0.0f},
		{0.0f,0.0f,1.0f},
		{0.0f,0.0f,-1.0f}
	};
	for(int i = 0;i < 6;i++){
		if(!node.block->side[i].luminance_p)
			continue;
		vec3 normal = normal_table[i];
		vec3 t_pos = block_pos;

		uint v_x = (uint[]){VEC3_Y,VEC3_X,VEC3_X}[i >> 1];
		uint v_y = (uint[]){VEC3_Z,VEC3_Z,VEC3_Y}[i >> 1];

		t_pos.a[i >> 1] += block_size * (float)(i & 1 ? 1.0f : -1.0f) * 0.5f;
		t_pos.a[v_x] -= block_size * 0.5f + block_size / (lm_size + 1);
		t_pos.a[v_y] -= block_size * 0.5f + block_size / (lm_size + 1);
		if(vec3dotR(vec3normalizeR(vec3subvec3R(t_pos,pos)),normal) < 0.0f)
			continue;
		for(int x = 0;x < lm_size + 1;x++){
			for(int y = 0;y < lm_size + 1;y++){
				float intensity = 1.0f / (vec3distance(t_pos,pos) * vec3distance(t_pos,pos));
				if(intensity < 0.0f){
					t_pos.a[v_x] += block_size / (lm_size + 1);
					continue;
				}
				intensity *= vec3dotR(vec3normalizeR(vec3subvec3R(t_pos,pos)),normal);
				vec3 color_f = color;
				vec3mul(&color_f,intensity);
				vec3addvec3(&node.block->side[i].luminance_p[x * (lm_size + 1) + y],color_f);
				t_pos.a[v_y] += block_size / (lm_size + 1);
			}
			t_pos.a[v_y] -= block_size;
			t_pos.a[v_x] += block_size / (lm_size + 1);
		}
	}
}