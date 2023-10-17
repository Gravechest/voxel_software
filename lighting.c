#include "tmath.h"
#include "source.h"
#include "tree.h"
#include "lighting.h"

vec2 lookDirectionTable[6] = {
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

vec3 lightmapUpX(block_node_t node,int side,int lightmap_x,int lightmap_y,int size){
	block_t* block = node.block;
	if(!block)
		return;
	vec3* lightmap_ptr = block->side[side].luminance_p;
	if(!lightmap_ptr)
		return;
	int hit_axis = side >> 1;
	int hit_side = side & 1;

	int plane_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[hit_axis];
	int plane_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[hit_axis];

	ivec3 pos = node.pos;
	pos.a[hit_axis] += hit_side ? 1 : -1;
	pos.a[plane_x]++;
	uint neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
	lightmap_y++;
	int block_edge = lightmap_y + size * (size + 2);
	if(!neighbour){
	same_axis:
		pos.a[hit_axis] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
		block_node_t neighbour_node = node_root[neighbour];
		block_t* neighbour_block = neighbour_node.block;
		if(!neighbour)
			return lightmap_ptr[block_edge];
		if(neighbour_block){
			if(material_array[neighbour_block->material].flags & MAT_LUMINANT)
				return block->side[side].luminance_p[block_edge];
			int offset = (node.pos.a[plane_y] & (1 << node.depth - neighbour_node.depth) - 1) * size;
			size <<= node.depth - neighbour_node.depth;
			if(!neighbour_block->side[side].luminance_p)
				return lightmap_ptr[block_edge];
			vec3 luminance = neighbour_block->side[side].luminance_p[(lightmap_y + offset) + (size + 2)];
			if(!luminance.r && !luminance.g && !luminance.b)
				return lightmap_ptr[block_edge];
			return luminance;
		}

		for(int d = node.depth;;d++){
			int shift = tMax(LM_MAXDEPTH - d - 1,0);
			ivec3 sub_pos;
			sub_pos.a[hit_axis] = hit_side;
			sub_pos.a[plane_x] = 1 - hit_side;
			sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;
			if(!node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x])
				return lightmap_ptr[block_edge];

			neighbour = neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x];
			neighbour_node = node_root[neighbour];
			neighbour_block = neighbour_node.block;
			if(!neighbour_block)
				continue;

			if(material_array[neighbour_node.block->material].flags & MAT_LUMINANT)
				return lightmap_ptr[block_edge];
			if(!neighbour_node.block->side[side].luminance_p)
				return lightmap_ptr[block_edge];

			int depth_difference = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			int mask = depth_difference >> neighbour_node.depth - node.depth;
			size >>= neighbour_node.depth - node.depth;
			int lightmap_location = (size + 2) + (lightmap_y - 1 & mask - 1) + 1;
			return neighbour_node.block->side[side].luminance_p[lightmap_location];
		}
	}
	block_node_t neighbour_node = node_root[neighbour];
	if(neighbour_node.block)
		return lightmap_ptr[block_edge];

	for(int d = node.depth;;d++){
		int shift = tMax(LM_MAXDEPTH - d - 1,0);
		ivec3 sub_pos;
		sub_pos.a[hit_axis] = 1 - hit_side;
		sub_pos.a[plane_x] = 1 - hit_side;
		sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;

		if(neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x]){
			neighbour = neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x];
			neighbour_node = node_root[neighbour];
			if(neighbour_node.block)
				return lightmap_ptr[block_edge];
			continue;
		}
		goto same_axis;
	}
}

vec3 lightmapDownX(block_node_t node,int side,int lightmap_x,int lightmap_y,int size){
	block_t* block = node.block;
	if(!block)
		return;
	vec3* lightmap_ptr = block->side[side].luminance_p;
	if(!lightmap_ptr)
		return;
	uint hit_axis = side >> 1;
	uint hit_side = side & 1;

	int plane_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[hit_axis];
	int plane_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[hit_axis];

	ivec3 pos = node.pos;
	pos.a[hit_axis] += hit_side ? 1 : -1;
	pos.a[plane_x]--;
	uint neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
	lightmap_y++;
	int block_edge = lightmap_y + (size + 2);
	if(!neighbour){
	same_axis:
		pos.a[hit_axis] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
		if(!neighbour)
			return lightmap_ptr[block_edge];
		block_node_t neighbour_node = node_root[neighbour];
		block_t* neighbour_block = neighbour_node.block;
		if(neighbour_block){
			if(material_array[neighbour_block->material].flags & MAT_LUMINANT)
				return lightmap_ptr[block_edge];
			int offset = (node.pos.a[plane_y] & (1 << node.depth - neighbour_node.depth) - 1) * size;
			size <<= node.depth - neighbour_node.depth;
			if(!neighbour_block->side[side].luminance_p)
				return lightmap_ptr[block_edge];
			vec3 luminance = neighbour_block->side[side].luminance_p[size * (size + 2) + (lightmap_y + offset)];
			if(!luminance.r && !luminance.g && !luminance.b)
				return lightmap_ptr[block_edge];
			return luminance;
		}

		for(int d = node.depth;;d++){
			int shift = tMax(LM_MAXDEPTH - d - 1,0);
			ivec3 sub_pos;
			sub_pos.a[hit_axis] = hit_side;
			sub_pos.a[plane_x] = hit_side;
			sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;
			if(!neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x])
				return lightmap_ptr[block_edge];

			neighbour = neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x];
			neighbour_node = node_root[neighbour];
			neighbour_block = neighbour_node.block;
			if(!neighbour_block)
				continue;

			if(material_array[neighbour_node.block->material].flags & MAT_LUMINANT)
				return lightmap_ptr[block_edge];
			int depth_difference = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			int mask = depth_difference >> neighbour_node.depth - node.depth;
			size >>= neighbour_node.depth - node.depth;
			int lightmap_location = (size + 1) * (size + 2) + (lightmap_y - 1 & mask - 1) + 1;
			if(!neighbour_block->side[side].luminance_p)
				return lightmap_ptr[block_edge];
			return neighbour_block->side[side].luminance_p[lightmap_location];
		}
	}
	if(node_root[neighbour].block)
		return lightmap_ptr[block_edge];

	for(int d = node.depth;;d++){
		int shift = tMax(LM_MAXDEPTH - d - 1,0);
		ivec3 sub_pos;
		sub_pos.a[hit_axis] = 1 - hit_side;
		sub_pos.a[plane_x] = hit_side;
		sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;

		if(node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x]){
			neighbour = node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x];
			if(node_root[neighbour].block)
				return lightmap_ptr[block_edge];
			continue;
		}
		goto same_axis;
	}
	
}

vec3 lightmapDownY(block_node_t node,int side,int lightmap_x,int lightmap_y,int size){	
	block_t* block = node.block;
	if(!block)
		return;
	vec3* lightmap_ptr = block->side[side].luminance_p;
	if(!lightmap_ptr)
		return;
	int hit_axis = side >> 1;
	int hit_side = side & 1;

	int plane_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[hit_axis];
	int plane_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[hit_axis];
	
	ivec3 pos = node.pos;
	pos.a[hit_axis] += hit_side ? 1 : -1;
	pos.a[plane_y]--;
	uint neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
	lightmap_x++;
	int block_edge = lightmap_x * (size + 2) + 1;
	if(!neighbour){	
	same_axis:
		pos.a[hit_axis] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
		if(!neighbour)
			return lightmap_ptr[block_edge];
		block_node_t neighbour_node = node_root[neighbour];
		block_t* neighbour_block = neighbour_node.block;
		if(neighbour_block){
			if(material_array[neighbour_block->material].flags & MAT_LUMINANT)
				return lightmap_ptr[block_edge];
			int offset = (node.pos.a[plane_x] & (1 << node.depth - neighbour_node.depth) - 1) * size;
			size <<= node.depth - neighbour_node.depth;
			if(!neighbour_block->side[side].luminance_p)
				return lightmap_ptr[block_edge];
			vec3 luminance = neighbour_block->side[side].luminance_p[(lightmap_x + offset) * (size + 2) + size];
			if(!luminance.r && !luminance.g && !luminance.b)
				return lightmap_ptr[block_edge];
			return luminance;
		}

		for(int d = node.depth;;d++){
			int shift = tMax(LM_MAXDEPTH - d - 1,0);
			ivec3 sub_pos;
			sub_pos.a[hit_axis] = hit_side;
			sub_pos.a[plane_y] = hit_side;
			sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;

			if(!neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x])
				return lightmap_ptr[block_edge];

			neighbour = neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x];
			neighbour_node = node_root[neighbour];
			neighbour_block = neighbour_node.block;
			if(!neighbour_block)
				continue;

			if(material_array[neighbour_node.block->material].flags & MAT_LUMINANT)
				return lightmap_ptr[block_edge];
			int depth_difference = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			int mask = depth_difference >> neighbour_node.depth - node.depth;
			size >>= neighbour_node.depth - node.depth;
			if(!neighbour_node.block->side[side].luminance_p)
				return lightmap_ptr[block_edge];
			int lightmap_location = size + ((lightmap_x - 1 & mask - 1) + 1) * (size + 2);
			return neighbour_node.block->side[side].luminance_p[lightmap_location];		
		}
	}
	if(node_root[neighbour].block)
		return lightmap_ptr[block_edge];

	for(int d = node.depth;;d++){
		int shift = tMax(LM_MAXDEPTH - d - 1,0);
		ivec3 sub_pos;
		sub_pos.a[hit_axis] = 1 - hit_side;
		sub_pos.a[plane_y] = hit_side;
		sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;

		if(node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x]){
			neighbour = node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x];
			if(node_root[neighbour].block)
				return lightmap_ptr[block_edge];
			continue;
		}
		goto same_axis;
	}
}

vec3 lightmapUpY(block_node_t node,int side,int lightmap_x,int lightmap_y,int size){
	block_t* block = node.block;
	if(!block)
		return;
	vec3* lightmap_ptr = block->side[side].luminance_p;
	if(!lightmap_ptr)
		return;
	int hit_axis = side >> 1;
	int hit_side = side & 1;

	int plane_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[hit_axis];
	int plane_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[hit_axis];

	ivec3 pos = node.pos;
	pos.a[hit_axis] += hit_side ? 1 : -1;
	pos.a[plane_y]++;
	uint neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
	lightmap_x++;
	int block_edge = lightmap_x * (size + 2) + size;
	if(!neighbour){
	same_axis:
		pos.a[hit_axis] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,node.depth);
		if(!neighbour)
			return lightmap_ptr[block_edge];
		block_node_t neighbour_node = node_root[neighbour];
		block_t* neighbour_block = neighbour_node.block;
		if(neighbour_block){
			if(material_array[neighbour_block->material].flags & MAT_LUMINANT)
				return block->side[side].luminance_p[block_edge];
			int offset = (node.pos.a[plane_x] & (1 << node.depth - neighbour_node.depth) - 1) * size;
			size <<= node.depth - neighbour_node.depth;
			if(!neighbour_block->side[side].luminance_p)
				return block->side[side].luminance_p[block_edge];
			vec3 luminance = neighbour_block->side[side].luminance_p[(lightmap_x + offset) * (size + 2) + 1];
			if(!luminance.r && !luminance.g && !luminance.b)
				return lightmap_ptr[block_edge];
			return luminance;
		}

		for(int d = node.depth;;d++){

			int shift = LM_MAXDEPTH - d - 1;
			ivec3 sub_pos;
			sub_pos.a[hit_axis] = hit_side;
			sub_pos.a[plane_y] = 1 - hit_side;
			sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;

			if(!neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x])
				return block->side[side].luminance_p[block_edge];

			neighbour = neighbour_node.child[sub_pos.z][sub_pos.y][sub_pos.x];
			neighbour_node = node_root[neighbour];
			neighbour_block = neighbour_node.block;
			if(!neighbour_block)
				continue;

			if(material_array[neighbour_node.block->material].flags & MAT_LUMINANT)
				return lightmap_ptr[block_edge];
			if(!neighbour_node.block->side[side].luminance_p)
				return lightmap_ptr[block_edge];
			int depth_difference = 1 << tMax(LM_MAXDEPTH - node.depth,0);
			int mask = depth_difference >> neighbour_node.depth - node.depth;
			size >>= neighbour_node.depth - node.depth;
			int lightmap_location = 1 + ((lightmap_x - 1 & mask - 1) + 1) * (size + 2);
			return neighbour_node.block->side[side].luminance_p[lightmap_location];
		}
		
	}
	if(!node_root[neighbour].block)
		return lightmap_ptr[block_edge];

	for(int d = node.depth;;d++){
		int shift = tMax(LM_MAXDEPTH - d - 1,0);
		ivec3 sub_pos;
		sub_pos.a[hit_axis] = 1 - hit_side;
		sub_pos.a[plane_y] = 1 - hit_side;
		sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;
			
		if(node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x]){
			neighbour = node_root[neighbour].child[sub_pos.z][sub_pos.y][sub_pos.x];
			if(node_root[neighbour].block)
				return lightmap_ptr[block_edge];
			continue;
		}
		goto same_axis;
	}
}

static bool isExposed(vec3 pos){
	block_node_t* node = node_root;
	traverse_init_t init = initTraverse(pos);
	if(node[init.node].block)
		return true;
	return false;
}

vec3 rayGetLuminance(vec3 init_pos,vec2 direction,uint init_node){
	vec3 angle = getLookAngle(direction);
	node_hit_t result;
	result = traverseTree(ray3Create(init_pos,angle),init_node);
			 
	if(!result.node){
		int side;
		vec2 uv = sampleCube(angle,&side);
		if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f)
			return LUMINANCE_SUN;
		else
			return LUMINANCE_SKY;
	}
	block_node_t node = node_root[result.node];
	block_t* block = node.block;
	if(!block)
		return VEC3_ZERO;
	material_t material = material_array[block->material];
	if(material.flags & MAT_LUMINANT)
		return material.luminance;

	vec3 block_pos;
	float block_size = (float)MAP_SIZE / (1 << node.depth) * 2.0f;
	block_pos.x = node.pos.x * block_size;
	block_pos.y = node.pos.y * block_size;
	block_pos.z = node.pos.z * block_size;
	plane_t plane = getPlane(block_pos,angle,result.side,block_size);
	vec3 plane_pos = vec3subvec3R(plane.pos,camera_rd.pos);
	float dst = rayIntersectPlane(plane_pos,angle,plane.normal);
	vec3 hit_pos = vec3addvec3R(camera_rd.pos,vec3mulR(angle,dst));
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

void setLightMap(vec3* color,vec3 pos,int side,int depth,uint quality){
	if(!color)
		return;
	vec2 dir = lookDirectionTable[side];
	vec3 normal = normal_table[side];	
	traverse_init_t init = initTraverse(pos);
	block_node_t node = node_root[init.node];
	if(node.block){
		if(!lightMapRePosition(&pos,side)){
			*color = VEC3_ZERO;
			return;
		}
		init = initTraverse(pos);
	}
	dir.x -= M_PI * 0.5f - M_PI * 0.5f / (quality);
	dir.y -= M_PI * 0.5f - M_PI * 0.5f / (quality);
	vec3 s_color = VEC3_ZERO;
	float weight_total = 0.0f;
	for(int x = 0;x < quality;x++){
		for(int y = 0;y < quality;y++){
			vec3 angle = getLookAngle(dir);
			float weight = tAbsf(vec3dotR(angle,normal));
			vec3 luminance = rayGetLuminance(init.pos,dir,init.node);
			vec3mul(&luminance,weight);
			vec3addvec3(&s_color,luminance);
			weight_total += weight;
			dir.y += M_PI / quality;
		}
		dir.y -= M_PI;
		dir.x += M_PI / quality;
	}
	vec3div(&s_color,weight_total);
	*color = s_color;
}

void updateEdgeLightMapArea(uint node_ptr,vec3 pos,float radius,int depth){
	block_node_t node = node_root[node_ptr];
	float block_size = (float)MAP_SIZE / (1 << depth);

	if(!node.block){
		for(int i = 0;i < 8;i++){
			if(!node.child_s[i])
				continue;
			vec3 pos_2 = {node_root[node.child_s[i]].pos.x,node_root[node.child_s[i]].pos.y,node_root[node.child_s[i]].pos.z};
			vec3add(&pos_2,0.5f);
			vec3mul(&pos_2,block_size);
			float distance = vec3distance(pos,pos_2);
			if(block_size + radius > distance)
				updateEdgeLightMapArea(node.child_s[i],pos,radius,depth + 1);
		}
		return;
	}
	block_t* block = node.block;
	if(depth >= LM_MAXDEPTH || material_array[block->material].flags & MAT_LUMINANT)
		return;
	float block_size_2 = (float)MAP_SIZE / (1 << depth) * 2.0f;
	for(int side = 0;side < 6;side++){
		vec3* lightmap = block->side[side].luminance_p;
		int size = 1 << LM_MAXDEPTH - depth;
		for(int i = 1;i < size + 1;i++){
			lightmap[i]                           = lightmapDownX(node,side,0,i - 1,size);
			lightmap[i * (size + 2)]              = lightmapDownY(node,side,i - 1,0,size);
			lightmap[(size + 1) * (size + 2) + i] = lightmapUpX(node,side,size - 1,i - 1,size);
			lightmap[i * (size + 2) + (size + 1)] = lightmapUpY(node,side,i - 1,size - 1,size);
		}
		lightmap[0] = vec3avgvec3R(lightmap[1],lightmap[size + 2]);
		lightmap[size + 1] = vec3avgvec3R(lightmap[size],lightmap[size + 2 + size + 1]);
		lightmap[(size + 1) * (size + 2)] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + 1],lightmap[(size + 1) * (size + 1)]);
		lightmap[(size + 1) * (size + 2) + size + 1] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + size],lightmap[(size + 1) * (size + 1) + size + 1]);
	}
}

void updateLightMapArea(unsigned int node_ptr,vec3 pos,float radius,int depth){
	block_node_t node = node_root[node_ptr];
	float block_size = (float)MAP_SIZE / (1 << depth);

	if(!node.block){
		for(int i = 0;i < 8;i++){
			if(!node.child_s[i])
				continue;
			vec3 pos_2 = {node_root[node.child_s[i]].pos.x,node_root[node.child_s[i]].pos.y,node_root[node.child_s[i]].pos.z};
			vec3add(&pos_2,0.5f);	
			vec3mul(&pos_2,block_size);
			float distance = vec3distance(pos,pos_2);
			if(block_size + radius > distance)
				updateLightMapArea(node.child_s[i],pos,radius,depth + 1);
		}
		return;
	}
	block_t* block = node.block;
	if(material_array[block->material].flags & MAT_LUMINANT)
		return;
	float block_size_2 = (float)MAP_SIZE / (1 << depth) * 2.0f;
	for(int side = 0;side < 6;side++){
		vec3 lm_pos = {node.pos.x,node.pos.y,node.pos.z};
		vec3add(&lm_pos,0.5f);
		lm_pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.02f;
			
		int v_x = (int[]){VEC3_Y,VEC3_X,VEC3_X}[side >> 1];
		int v_y = (int[]){VEC3_Z,VEC3_Z,VEC3_Y}[side >> 1];

		int size = 1 << tMax(LM_MAXDEPTH - depth,0);

		lm_pos.a[v_x] -= 0.5f - 0.5f / size;
		lm_pos.a[v_y] -= 0.5f - 0.5f / size;
		if(-vec3dotR(vec3normalizeR(vec3subvec3R(pos,vec3mulR(lm_pos,block_size_2))),normal_table[side]) < 0.0f)
			continue;
		for(int x = 1;x < size + 1;x++){
			for(int y = 1;y < size + 1;y++){
				vec3 l_pos = vec3mulR(lm_pos,block_size_2);
				vec3 l_dir = vec3normalizeR(vec3subvec3R(pos,l_pos));
				if(vec3distance(l_pos,pos) / -vec3dotR(normal_table[side],l_dir) > radius + block_size * 0.25f){
					lm_pos.a[v_y] += 1.0f / size;
					continue;
				}
				vec3 lm_pos_b = vec3divR(lm_pos,(float)(1 << depth) * 0.5f);
				vec3mul(&lm_pos_b,MAP_SIZE);
				setLightMap(&block->side[side].luminance_p[x * (size + 2) + y],lm_pos_b,side,depth,LM_SIZE);
				lm_pos.a[v_y] += 1.0f / size;
			}
			lm_pos.a[v_y] -= 1.0f;
			lm_pos.a[v_x] += 1.0f / size;
		}
	}
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
			setLightMap(&color[x * (size + 2) + y],l_pos,side,depth,quality);
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
	setLightMap(color,pos,side,depth,quality);
}

void setLightSide(vec3* color,vec3 pos,int side,int depth){
	vec3add(&pos,0.5f);
	pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.02f;
	vec3div(&pos,(float)(1 << depth) * 0.5f);
	vec3mul(&pos,MAP_SIZE);
	setLightMap(color,pos,side,depth,LM_SIZE);
}

void setEdgeLightSide(block_node_t node,uint side){
	if(node.depth >= LM_MAXDEPTH)
		return;
	uint size = GETLIGHTMAPSIZE(node.depth);
	block_t* block = node.block;
	if(!block)
		return;
	vec3* lightmap = block->side[side].luminance_p;
	if(!lightmap)
		return;
	
	for(int j = 1;j < size + 1;j++){
		lightmap[j]                           = lightmapDownX(node,side,0,j - 1,size);
		lightmap[j * (size + 2)]              = lightmapDownY(node,side,j - 1,0,size);
		lightmap[(size + 1) * (size + 2) + j] = lightmapUpX(node,side,size - 1,j - 1,size);
		lightmap[j * (size + 2) + (size + 1)] = lightmapUpY(node,side,j - 1,size - 1,size);
	}
	lightmap[0] = vec3avgvec3R(lightmap[1],lightmap[size + 2]);
	lightmap[size + 1] = vec3avgvec3R(lightmap[size],lightmap[size + 2 + size + 1]);
	lightmap[(size + 1) * (size + 2)] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + 1],lightmap[(size + 1) * (size + 1)]);
	lightmap[(size + 1) * (size + 2) + size + 1] = vec3avgvec3R(lightmap[(size + 1) * (size + 2) + size],lightmap[(size + 1) * (size + 1) + size + 1]);
}