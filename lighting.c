#include "tmath.h"
#include "source.h"
#include "tree.h"
#include "lighting.h"
#include "fog.h"
#include "memory.h"

static vec2_t lookDirectionTable[6] = {
	{M_PI,0.0f},
	{0.0f,0.0f},
	{-M_PI * 0.5f,0.0f},
	{M_PI * 0.5f,0.0f},
	{0.0f,-M_PI * 0.5f},
	{0.0f,M_PI * 0.5f}
};

int getLightmapSize(int depth){
	return 1 << (LM_MAXDEPTH - (int)depth < 0 ? 0 : LM_MAXDEPTH - (int)depth);
}

void lightNewStackPut(uint32_t node_index,block_t* block,uint32_t side){
	node_t* node = dynamicArrayGet(node_root,node_index);
	if(light_new_stack_ptr >= LIGHT_NEW_STACK_SIZE - 1)
		return;

	light_new_stack_t* stack = &light_new_stack[light_new_stack_ptr++];
	stack->node = node;
	stack->side = side;

	uint32_t size = getLightmapSize(node->depth);

	if(node->type == BLOCK_SPHERE)
		block->luminance[side] = tMallocZero(sizeof(vec3_t) * 16 * 16);
	else
		block->luminance[side] = tMallocZero(sizeof(vec3_t) * (size + 1) * (size + 1));
}

bool lightMapRePosition(vec3_t* pos,int side){
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

static bool isExposed(vec3_t pos){
	node_t* node = dynamicArrayGet(node_root,0);
	traverse_init_t init = initTraverse(pos);
	if(node[init.node].type < BLOCK_PARENT)
		return true;
	return false;
}
#include <stdio.h>
#include <math.h>

vec3_t getLuminance(vec3_t* luminance,vec2_t uv,uint32_t depth){
	if(!luminance)
		return VEC3_ZERO;
	uint32_t lm_size = getLightmapSize(depth);

	vec2_t fract_pos = {
		tFractUnsigned(uv.x * lm_size),
		tFractUnsigned(uv.y * lm_size)
	};
	
	uint32_t lm_offset_x = tMinf(uv.x * lm_size,lm_size - 1);
	uint32_t lm_offset_y = tMinf(uv.y * lm_size,lm_size - 1);

	uint32_t location = lm_offset_x * (lm_size + 1) + lm_offset_y;

	vec3_t lm_1 = luminance[location];
	vec3_t lm_2 = luminance[location + 1];
	vec3_t lm_3 = luminance[location + (lm_size + 1)];
	vec3_t lm_4 = luminance[location + (lm_size + 1) + 1];

	vec3_t mix_1 = vec3mixR(lm_1,lm_3,fract_pos.x);
	vec3_t mix_2 = vec3mixR(lm_2,lm_4,fract_pos.x);

	return vec3mixR(mix_1,mix_2,fract_pos.y);
}
/*
float rayCubeIntersectionInside(vec3_t ray_pos,vec3_t cube_pos,vec3_t angle,float size) {
	vec3_t rel_pos = vec3subvec3R(cube_pos,ray_pos);
    vec3_t m = vec3divFR(angle,1.0f);
    vec3_t n = vec3mulvec3R(m,rel_pos);   
    vec3_t k = vec3mulR(vec3absR(m),size);
    vec3_t t1 = vec3subvec3R(vec3negR(n),k);
    vec3_t t2 = vec3addvec3R(vec3negR(n),k);
    float tN = tMaxf(tMaxf(t1.x,t1.y),t1.z);
    float tF = tMinf(tMinf(t2.x,t2.y),t2.z);
    if(tN > tF || tF < 0.0f) 
		return 0.0f;
    return tF;	
}
*/

vec3_t sideGetLuminance(vec3_t ray_pos,vec3_t angle,node_hit_t result,uint32_t depth){
	node_t* node = dynamicArrayGet(node_root,result.node);
	block_t* block = dynamicArrayGet(block_array,node->index);
	if(!block)
		return VEC3_ZERO;
	material_t material = material_array[node->type];
	if(material.flags & MAT_LUMINANT)
		return material.luminance;
	if(node->type == BLOCK_LAMP && block->power)
		return vec3mulR(material.luminance,block->power);
	if(material.flags & MAT_LIQUID)
		return VEC3_ZERO;
	if(material.flags & MAT_REFRACT){
		float distance = rayNodeGetDistance(ray_pos,node->pos,node->depth,angle,result.side);
		vec3_t pos_new = vec3addvec3R(ray_pos,vec3mulR(angle,distance + 0.001f));
		traverse_init_t init = initTraverse(pos_new);
		vec3_t normal = VEC3_ZERO;
		normal.a[result.side] = angle.a[result.side] < 0.0f ? 1.0f : -1.0f;
		float refract_1;
		node_t start_node = treeTraverse(ray_pos);
		if(start_node.type == BLOCK_AIR)
			refract_1 = 1.0f;
		else{
			if(material_array[start_node.type].flags & MAT_REFRACT)
				refract_1 = 1.66f;
			else
				refract_1 = 1.0f;
		}
		angle = vec3refract(angle,normal,refract_1 / 1.66f);
		node_hit_t result_inside = treeRayOnce(ray3Create(init.pos,angle),init.node,pos_new);
		node_t* hit_node = dynamicArrayGet(node_root,result_inside.node);
		distance = rayNodeGetDistance(pos_new,hit_node->pos,hit_node->depth,angle,result_inside.side);	
		pos_new = vec3addvec3R(pos_new,vec3mulR(angle,distance - 0.001f));
		init = initTraverse(pos_new);
		normal = VEC3_ZERO;
		normal.a[result_inside.side] = angle.a[result_inside.side] < 0.0f ? 1.0f : -1.0f;
		if(hit_node->type == BLOCK_AIR)
			refract_1 = 1.0f;
		else{
			if(hit_node->type != BLOCK_PARENT){
				if(material_array[hit_node->type].flags & MAT_REFRACT)
					refract_1 = 1.66f;
				else
					refract_1 = 1.0f;
			}
			else
				refract_1 = 1.0f;
		}
		vec3_t angle_f = vec3refract(angle,normal,1.66f / refract_1);
		if(!angle_f.x)
			angle_f = vec3reflect(angle,normal);

		float density = 1.0f / (distance + 1.0f);
		vec3_t filter = {
			powf(material.luminance.r,distance),
			powf(material.luminance.g,distance),
			powf(material.luminance.b,distance)
		};
		return vec3mulvec3R(rayGetLuminance(pos_new,init.pos,angle_f,init.node,depth + 1),filter);
	}
	if(node->type == BLOCK_SPHERE){
		return VEC3_ZERO;    
	}
	vec3_t block_pos;
	float block_size = getBlockSize(node->depth);
	block_pos.x = node->pos.x * block_size;
	block_pos.y = node->pos.y * block_size;
	block_pos.z = node->pos.z * block_size;
	plane_ray_t plane = getPlane(block_pos,angle,result.side,block_size);
	vec3_t plane_pos = vec3subvec3R(plane.pos,ray_pos);
	float dst = rayIntersectPlane(plane_pos,angle,plane.normal);
	vec3_t hit_pos = vec3addvec3R(ray_pos,vec3mulR(angle,dst));
	vec2_t uv = {
		(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,
		(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size
	};
	vec2_t texture_uv = {
		tFract(hit_pos.a[plane.x] * 0.25f),
		tFract(hit_pos.a[plane.y] * 0.25f)
	};
			
	int side = result.side * 2 + (angle.a[result.side] < 0.0f);
	uv.x = tMaxf(tMinf(uv.x,1.0f),0.0f);
	uv.y = tMaxf(tMinf(uv.y,1.0f),0.0f);

	vec3_t hit_block_luminance = getLuminance(block->luminance[side],uv,node->depth);

	int ty = (texture_uv.x * material.texture_size.x + material.texture_pos.x) * 2047.0f;
	int tx = (texture_uv.y * material.texture_size.y + material.texture_pos.y) * 2047.0f;
	
	pixel_t text = texture_atlas[tx * 2048 + ty];
	vec3_t texture = (vec3_t){text.r / 255.0f,text.g / 255.0f,text.b / 255.0f};

	return vec3mulvec3R(texture,hit_block_luminance);
}

vec3_t rayGetLuminance(vec3_t ray_pos,vec3_t init_pos,vec3_t angle,uint32_t init_node,uint32_t depth){
	if(depth > 10)
		return VEC3_ZERO;
	node_hit_t result = treeRay(ray3Create(init_pos,angle),init_node,ray_pos);
	if(!result.node)
		return LUMINANCE_SKY;
	return sideGetLuminance(ray_pos,angle,result,depth);
}

vec3_t rayGetLuminanceDir(vec3_t init_pos,vec2_t direction,uint32_t init_node,vec3_t ray_pos){
	return rayGetLuminance(ray_pos,init_pos,getLookAngle(direction),init_node,0);
}

void setLightMap(vec3_t* color,vec3_t pos,vec3_t normal,uint32_t quality){
	if(!color)
		return;
	traverse_init_t init = initTraverse(pos);
	node_t begin_node = treeTraverse(pos);
	if(begin_node.type != BLOCK_AIR && !(material_array[begin_node.type].flags & MAT_LIQUID) && begin_node.type != BLOCK_SPHERE){
		*color = VEC3_ZERO;
		return;
	}
	vec3_t s_color = VEC3_ZERO;
	float weight_total = 0.0f;
	for(int x = 0;x < quality;x++){
		for(int y = 0;y < quality;y++){
			vec3_t angle = getRayAnglePlane(normal,(float)x / quality * 2.0f - 1.0f,(float)y / quality * 2.0f - 1.0f);
			float weight = vec3dotR(angle,normal);
			vec3_t luminance = rayGetLuminance(pos,init.pos,angle,init.node,0);
			vec3mul(&luminance,weight);
			vec3addvec3(&s_color,luminance);
			weight_total += weight;
		}
	}
	vec3div(&s_color,weight_total);
	*color = s_color;
}

vec3_t posNew(vec3_t pos,float size,int v_x,int v_y){
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
	return (vec3_t){-1.0f,0.0f,0.0f};
}

#include <Windows.h>

static void cylinder(node_t* node,vec3_t block_pos,float block_size,int axis,int quality){
	block_t* block = dynamicArrayGet(block_array,node->index);
	material_t material = material_array[node->type];
	int axis_table[][3] = {{VEC3_X,VEC3_Y,VEC3_Z},{VEC3_Y,VEC3_Z,VEC3_X},{VEC3_Z,VEC3_X,VEC3_Y}};
	int axis_v[] = {axis_table[axis][0],axis_table[axis][1],axis_table[axis][2]};
	vec3_t middle = block_pos;
	vec3add(&middle,block_size * 0.5f);
	int triangle_ammount = 16;
	for(int i = 0;i < triangle_ammount;i++){
		vec3_t middle_line;
		middle_line.a[axis_v[1]] = middle.a[axis_v[1]];
		middle_line.a[axis_v[2]] = middle.a[axis_v[2]];
		middle_line.a[axis_v[0]] = block_pos.a[axis_v[0]] + (block_size * (i / (triangle_ammount - 1.0f)));
		for(int j = 0;j < triangle_ammount;j++){
			vec3_t pos = middle_line;
			vec2_t offset = vec2rotR((vec2_t){1.0f,0.0f},(j / (triangle_ammount - 1.0f)) * M_PI * 2.0f);
			pos.a[axis_v[1]] += offset.x * block_size * 0.25f;
			pos.a[axis_v[2]] += offset.y * block_size * 0.25f;
			vec3_t normal;
			normal.a[axis_v[1]] = offset.x;
			normal.a[axis_v[2]] = offset.y;
			normal.a[axis_v[0]] = 0.0f;
			setLightMap(&block->luminance[0][i * 16 + j],pos,normal,quality);
			if(main_thread_status){
				main_thread_status = 2;
				while(main_thread_status == 2)
					Sleep(1);
			}
			vec3mulvec3(&block->luminance[0][i * 16 + j],material.luminance);
		}
	}
}

static void torus(node_t* node,vec3_t block_pos,float block_size,int axis,int rotation,int quality){
	block_t* block = dynamicArrayGet(block_array,node->index);
	material_t material = material_array[node->type];
	int axis_table[][3] = {{VEC3_X,VEC3_Y,VEC3_Z},{VEC3_Y,VEC3_Z,VEC3_X},{VEC3_Z,VEC3_X,VEC3_Y}};
	int axis_v[] = {axis_table[axis][0],axis_table[axis][1],axis_table[axis][2]};
	vec3_t middle = block_pos;
	middle.a[axis_v[0]] += block_size * 0.5f;
	if(rotation == 2 || rotation == 1)
		middle.a[axis_v[1]] += block_size;
	if(rotation & 2)
		middle.a[axis_v[2]] += block_size;
	int triangle_ammount = 16;
	for(int i = 0;i < triangle_ammount;i++){
		vec2_t offset = vec2rotR((vec2_t){1.0f,0.0f},(i / (triangle_ammount - 1.0f) + rotation) * M_PI * 0.5f);
		vec3_t middle_line;
		middle_line.a[axis_v[1]] = middle.a[axis_v[1]] + offset.x * block_size * 0.5f;
		middle_line.a[axis_v[2]] = middle.a[axis_v[2]] + offset.y * block_size * 0.5f;
		middle_line.a[axis_v[0]] = middle.a[axis_v[0]];
		for(int j = 0;j < triangle_ammount;j++){
			vec3_t pos = middle_line;
			vec2_t offset2 = vec2rotR((vec2_t){1.0f,0.0f},(j / (triangle_ammount - 1.0f)) * M_PI * 2.0f);
			pos.a[axis_v[1]] += offset.x * offset2.y * block_size * 0.25f;
			pos.a[axis_v[2]] += offset.y * offset2.y * block_size * 0.25f;
			pos.a[axis_v[0]] += offset2.x * block_size * 0.25f;
			vec3_t normal = vec3subvec3R(pos,middle_line);
			setLightMap(&block->luminance[0][i * 16 + j],pos,normal,quality);
			//lightmap[i * 16 + j] = normal;
			if(main_thread_status){
				main_thread_status = 2;
				while(main_thread_status == 2)
					Sleep(1);
			}
			vec3mulvec3(&block->luminance[0][i * 16 + j],material.luminance);
		}
	}
}

void updateLightMapSide(node_t node,vec3_t* lightmap,material_t material,uint32_t side,uint32_t quality){
	if(!lightmap)
		return;
	vec3_t lm_pos = {node.pos.x,node.pos.y,node.pos.z};

	if(node.type == BLOCK_SPHERE){
		float block_size = getBlockSize(node.depth);
		vec3mul(&lm_pos,block_size);
		vec3add(&lm_pos,block_size * 0.5f); 
		block_t* block = dynamicArrayGet(block_array,node.index);
		switch(block->neighbour){
		default: 		
			for(int i = 0;i < SPHERE_TRIANGLE_COUNT;i++){
				setLightMap(&lightmap[i],lm_pos,sphere_vertex[i],quality);
				if(main_thread_status){
					main_thread_status = 2;
					while(main_thread_status == 2)
						Sleep(1);
				}
				vec3mulvec3(&lightmap[i],material.luminance);
			}	 
			break;
		case 0b000011: cylinder(&node,lm_pos,block_size,VEC3_X,quality); break;
		case 0b001100: cylinder(&node,lm_pos,block_size,VEC3_Y,quality); break;
		case 0b110000: cylinder(&node,lm_pos,block_size,VEC3_Z,quality); break;
		case 0b001010: torus(&node,lm_pos,block_size,VEC3_Z,0,quality); break;
		case 0b000110: torus(&node,lm_pos,block_size,VEC3_Z,3,quality); break;
		case 0b001001: torus(&node,lm_pos,block_size,VEC3_Z,1,quality); break;
		case 0b000101: torus(&node,lm_pos,block_size,VEC3_Z,2,quality); break;
		case 0b100010: torus(&node,lm_pos,block_size,VEC3_Y,0,quality); break;
		case 0b100001: torus(&node,lm_pos,block_size,VEC3_Y,1,quality); break;
		case 0b010010: torus(&node,lm_pos,block_size,VEC3_Y,2,quality); break;
		case 0b010001: torus(&node,lm_pos,block_size,VEC3_Y,3,quality); break;
		case 0b101000: torus(&node,lm_pos,block_size,VEC3_X,0,quality); break;
		case 0b100100: torus(&node,lm_pos,block_size,VEC3_X,1,quality); break;
		case 0b011000: torus(&node,lm_pos,block_size,VEC3_X,2,quality); break;
		case 0b010100: torus(&node,lm_pos,block_size,VEC3_X,3,quality); break;
		}

		return;
	}
	
	vec3add(&lm_pos,0.5f);
	lm_pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.002f;

	int v_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
	int v_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];

	uint32_t size = GETLIGHTMAPSIZE(node.depth);

	float lm_size_sub = 1.0f / size;

	lm_pos.a[v_x] -= lm_size_sub * (size / 2.0f) - 0.001f;
	lm_pos.a[v_y] -= lm_size_sub * (size / 2.0f) - 0.001f;

	for(int sub_x = 0;sub_x < size + 1;sub_x++){
		for(int sub_y = 0;sub_y < size + 1;sub_y++){
			uint32_t x = sub_x;
			uint32_t y = sub_y;
			vec3_t lm_pos_b = lm_pos;

			lm_pos_b.a[v_x] += lm_size_sub * x * 0.998f;
			lm_pos_b.a[v_y] += lm_size_sub * y * 0.998f;
			vec3_t pos_c = lm_pos_b;
			lm_pos_b = vec3divR(lm_pos_b,(float)(1 << node.depth) * 0.5f);
			vec3mul(&lm_pos_b,MAP_SIZE);
			uint32_t lightmap_location = x * (size + 1) + y;
			vec3_t pre = lightmap[lightmap_location];
			if(insideBlock(lm_pos_b)){
				lm_pos_b = posNew(lm_pos_b,size,v_x,v_y);
				if(lm_pos_b.x == -1.0f)
					continue;
			}
			setLightMap(&lightmap[lightmap_location],lm_pos_b,normal_table[side],quality);
			if(main_thread_status){
				main_thread_status = 2;
				while(main_thread_status == 2)
					Sleep(1);
			}
			if(node.type == BLOCK_SWITCH){
				block_t* block = dynamicArrayGet(block_array,node.index);
				if(block->on)
					vec3mulvec3(&lightmap[lightmap_location],material.luminance_secundair);
				else
					vec3mulvec3(&lightmap[lightmap_location],material.luminance);
			}
			else
				vec3mulvec3(&lightmap[lightmap_location],material.luminance);
			vec3_t post = lightmap[lightmap_location];
		}
	}
}

void updateLightMap(vec3_t pos,uint32_t node_ptr,float radius){
	node_t* node = dynamicArrayGet(node_root,node_ptr);
	float block_size = getBlockSize(node->depth);
	vec3_t block_pos = {node->pos.x,node->pos.y,node->pos.z};
	vec3add(&block_pos,0.5f);
	vec3mul(&block_pos,block_size);
	float distance = sdCube(pos,block_pos,block_size);
	if(radius < distance)
		return;	
	if(node->type == BLOCK_PARENT){
		for(int i = 0;i < 8;i++)
			updateLightMap(pos,node->child_s[i],radius);
		return;
	}
	if(node->type == BLOCK_AIR){
		calculateNodeLuminance(node,32);
		return;
	}
	block_t* block = dynamicArrayGet(block_array,node->index);
	material_t material = material_array[node->type];
	if(material.flags & MAT_LUMINANT)
		return;
	if(material.flags & MAT_LIQUID)
		return;
	for(int side = 0;side < 6;side++){
		if(block->luminance[side])
			updateLightMapSide(*node,block->luminance[side],material,side,LM_QUALITY);
	}
}

void lighting(){
	for(;;){
		while(light_new_stack_ptr){
			light_new_stack_t stack = light_new_stack[--light_new_stack_ptr];
			if(!stack.node)
				continue;
			uint32_t size = GETLIGHTMAPSIZE(stack.node->depth);
			block_t* block = dynamicArrayGet(block_array,stack.node->index);
			if(!block)
				continue;
			uint32_t side = stack.side;
			ivec3 pos_i = stack.node->pos;
			vec3_t pos = {pos_i.x,pos_i.y,pos_i.z};
			vec3_t* lightmap = block->luminance[side];
			if(!lightmap)
				continue;
			updateLightMapSide(*stack.node,block->luminance[side],material_array[stack.node->type],side,LM_SIZE);
		}
		updateLightMap(camera.pos,0,16.0f);
		if(main_thread_status){
			main_thread_status = 2;
			while(main_thread_status == 2)
				Sleep(1);
		}
	}
}
/*
void calculateDynamicLight(vec3_t pos,uint32_t node_ptr,float radius,vec3_t color){
	return;
	node_t node = node_root[node_ptr];
	if(node.air)
		return;
	float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
	vec3_t block_pos = {node.pos.x,node.pos.y,node.pos.z};
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
	uint32_t lm_size = GETLIGHTMAPSIZE(node.depth);

	static const vec3_t normal_table[6] = {
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
		vec3_t normal = normal_table[i];
		vec3_t t_pos = block_pos;

		uint32_t v_x = (uint32_t[]){VEC3_Y,VEC3_X,VEC3_X}[i >> 1];
		uint32_t v_y = (uint32_t[]){VEC3_Z,VEC3_Z,VEC3_Y}[i >> 1];

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
				vec3_t color_f = color;
				vec3mul(&color_f,intensity);
				vec3addvec3(&node.block->side[i].luminance_p[x * (lm_size + 1) + y],color_f);
				t_pos.a[v_y] += block_size / (lm_size + 1);
			}
			t_pos.a[v_y] -= block_size;
			t_pos.a[v_x] += block_size / (lm_size + 1);
		}
	}
}
*/