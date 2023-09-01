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

vec3 normalTable[6] = {
	{1.0f,0.0f,0.0f},
	{0.0f,1.0f,0.0f},
	{0.0f,0.0f,1.0f},
	{-1.0f,0.0f,0.0f},
	{0.0f,-1.0f,0.0f},
	{0.0f,0.0f,-1.0f}
};

bool lightMapRePosition(vec3* pos,int side,int depth){
	float block_size = (float)MAP_SIZE / (1 << depth) * 2.0f;
	int x_table[] = {VEC3_Y,VEC3_X,VEC3_X}; 
	int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};

	int p_x = x_table[side >> 1];
	int p_y = y_table[side >> 1];

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

vec3 lightmapUpX(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size){
	int x_table[] = {VEC3_Y,VEC3_X,VEC3_X}; 
	int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
	
	int plane_x = x_table[side >> 1];
	int plane_y = y_table[side >> 1];

	ivec3 pos = block->pos;
	pos.a[side >> 1] += hit_side ? 1 : -1;
	pos.a[plane_x]++;
	block_node_t* neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
	lightmap_y++;
	int block_edge = lightmap_y + size * (size + 2);
	if(!neighbour){
	same_axis:
		pos.a[side >> 1] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
		if(neighbour){
			if(neighbour->block)
				return neighbour->block->side[side].luminance_p[lightmap_y + (size + 2)];

			for(int d = block->depth;d < LM_MAXDEPTH;d++){
				if(d == LM_MAXDEPTH)
					return block->side[side].luminance_p[block_edge];

				int shift = LM_MAXDEPTH - d - 1;
				ivec3 sub_pos;
				sub_pos.a[side >> 1] = hit_side;
				sub_pos.a[plane_x] = 1 - hit_side;
				sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;
				if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
					neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
					if(neighbour->block){
						if(neighbour->block->depth >= LM_MAXDEPTH)
							return neighbour->block->side[side].luminance;

						int mask = (1 << LM_MAXDEPTH - block->depth) >> neighbour->block->depth - block->depth - 1;
						size >>= (neighbour->block->depth - block->depth);
						return neighbour->block->side[side].luminance_p[(size + 1) * (size + 2) + (lightmap_y - 1 & mask) + 1];
					}
					continue;
				}
				return block->side[side].luminance_p[block_edge];
			}
		}
		return block->side[side].luminance_p[block_edge];
	}
	if(!neighbour->block){
		for(int d = block->depth;d < LM_MAXDEPTH;d++){
			if(d == LM_MAXDEPTH)
				return block->side[side].luminance_p[block_edge];

			int shift = LM_MAXDEPTH - d - 1;
			ivec3 sub_pos;
			sub_pos.a[side >> 1] = 1 - hit_side;
			sub_pos.a[plane_x] = 1 - hit_side;
			sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;

			if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
				neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
				if(neighbour->block)
					return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
				continue;
			}
			goto same_axis;
		}
	}
	return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
}

vec3 lightmapDownX(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size){
	int x_table[] = {VEC3_Y,VEC3_X,VEC3_X}; 
	int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
	
	int plane_x = x_table[side >> 1];
	int plane_y = y_table[side >> 1];

	ivec3 pos = block->pos;
	pos.a[side >> 1] += hit_side ? 1 : -1;
	pos.a[plane_x]--;
	block_node_t* neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
	lightmap_y++;
	int block_edge = lightmap_y + (size + 2);
	if(!neighbour){
	same_axis:
		pos.a[side >> 1] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
		if(neighbour){
			if(neighbour->block)
				return neighbour->block->side[side].luminance_p[size * (size + 2) + lightmap_y];

			for(int d = block->depth;d < LM_MAXDEPTH;d++){
				if(d == LM_MAXDEPTH)
					return block->side[side].luminance_p[block_edge];

				int shift = LM_MAXDEPTH - d - 1;
				ivec3 sub_pos;
				sub_pos.a[side >> 1] = hit_side;
				sub_pos.a[plane_x] = hit_side;
				sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;
				if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
					neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
					if(neighbour->block){
						if(neighbour->block->depth >= LM_MAXDEPTH)
							return neighbour->block->side[side].luminance;
						int mask = (1 << LM_MAXDEPTH - block->depth) >> neighbour->block->depth - block->depth - 1;
						return neighbour->block->side[side].luminance_p[(size + 1) + ((lightmap_y - 1) & mask) + 1];
					}
					continue;
				}
				return block->side[side].luminance_p[block_edge];
			}
		}
		return block->side[side].luminance_p[block_edge];
	}
	if(!neighbour->block){
		for(int d = block->depth;d < LM_MAXDEPTH;d++){
			if(d == LM_MAXDEPTH)
				return block->side[side].luminance_p[block_edge];

			int shift = LM_MAXDEPTH - d - 1;
			ivec3 sub_pos;
			sub_pos.a[side >> 1] = 1 - hit_side;
			sub_pos.a[plane_x] = hit_side;
			sub_pos.a[plane_y] = lightmap_y - 1 >> shift & 1;

			if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
				neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
				if(neighbour->block)
					return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
				continue;
			}
			goto same_axis;
		}
	}
	return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
}

vec3 lightmapDownY(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size){
	int x_table[] = {VEC3_Y,VEC3_X,VEC3_X}; 
	int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
	
	int plane_x = x_table[side >> 1];
	int plane_y = y_table[side >> 1];
	
	ivec3 pos = block->pos;
	pos.a[side >> 1] += hit_side ? 1 : -1;
	pos.a[plane_y]--;
	block_node_t* neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
	lightmap_x++;
	int block_edge = lightmap_x * (size + 2) + 1;
	if(!neighbour){	
	same_axis:
		pos.a[side >> 1] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
		if(neighbour){
			if(neighbour->block)
				return neighbour->block->side[side].luminance_p[lightmap_x * (size + 2) + size];

			for(int d = block->depth;d < LM_MAXDEPTH;d++){
				if(d == LM_MAXDEPTH)
					return block->side[side].luminance_p[block_edge];

				int shift = LM_MAXDEPTH - d - 1;
				ivec3 sub_pos;
				sub_pos.a[side >> 1] = hit_side;
				sub_pos.a[plane_y] = hit_side;
				sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;
				if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
					neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
					if(neighbour->block){
						if(neighbour->block->depth >= LM_MAXDEPTH)
							return neighbour->block->side[side].luminance;
						int mask = (1 << LM_MAXDEPTH - block->depth) >> neighbour->block->depth - block->depth - 1;
						size >>= (neighbour->block->depth - block->depth);
						return neighbour->block->side[side].luminance_p[((lightmap_x - 1 & mask) + 1) * (size + 2) + size];
					}
					continue;
				}
				return block->side[side].luminance_p[block_edge];
			}
		}
		return block->side[side].luminance_p[block_edge];
	}
	if(!neighbour->block){
		for(int d = block->depth;d < LM_MAXDEPTH;d++){
			if(d == LM_MAXDEPTH)
				return block->side[side].luminance_p[block_edge];

			int shift = LM_MAXDEPTH - d - 1;
			ivec3 sub_pos;
			sub_pos.a[side >> 1] = 1 - hit_side;
			sub_pos.a[plane_y] = hit_side;
			sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;

			if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
				neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
				if(neighbour->block)
					return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
				continue;
			}
			goto same_axis;
		}
	}
	return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
}

vec3 lightmapUpY(block_t* block,int side,int hit_side,int lightmap_x,int lightmap_y,int size){
	int x_table[] = {VEC3_Y,VEC3_X,VEC3_X}; 
	int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
	
	int plane_x = x_table[side >> 1];
	int plane_y = y_table[side >> 1];

	ivec3 pos = block->pos;
	pos.a[side >> 1] += hit_side ? 1 : -1;
	pos.a[plane_y]++;
	block_node_t* neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
	lightmap_x++;
	int block_edge = lightmap_x * (size + 2) + size;
	if(!neighbour){
	same_axis:
		pos.a[side >> 1] -= hit_side ? 1 : -1;
		neighbour = getNode(pos.x,pos.y,pos.z,block->depth);
		if(neighbour){
			if(neighbour->block)
				return neighbour->block->side[side].luminance_p[lightmap_x * (size + 2) + 1];

			for(int d = block->depth;d < LM_MAXDEPTH;d++){
				if(d == LM_MAXDEPTH)
					return block->side[side].luminance_p[block_edge];

				int shift = LM_MAXDEPTH - d - 1;
				ivec3 sub_pos;
				sub_pos.a[side >> 1] = hit_side;
				sub_pos.a[plane_y] = 1 - hit_side;
				sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;
				if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
					neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
					if(neighbour->block){
						if(neighbour->block->depth >= LM_MAXDEPTH)
							return neighbour->block->side[side].luminance;
						int mask = (1 << LM_MAXDEPTH - block->depth) >> neighbour->block->depth - block->depth - 1;
						return neighbour->block->side[side].luminance_p[((lightmap_x - 1 & mask) + 1) * (size + 2) + 1];
					}
					continue;
				}
				return block->side[side].luminance_p[block_edge];
			}
		}
		return block->side[side].luminance_p[block_edge];
	}
	if(!neighbour->block){
		for(int d = block->depth;d < LM_MAXDEPTH;d++){
			if(d == LM_MAXDEPTH)
				return block->side[side].luminance_p[block_edge];

			int shift = LM_MAXDEPTH - d - 1;
			ivec3 sub_pos;
			sub_pos.a[side >> 1] = 1 - hit_side;
			sub_pos.a[plane_y] = 1 - hit_side;
			sub_pos.a[plane_x] = lightmap_x - 1 >> shift & 1;

			if(neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x]){
				neighbour = neighbour->child[sub_pos.z][sub_pos.y][sub_pos.x];
				if(neighbour->block)
					return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
				continue;
			}
			goto same_axis;
		}
	}
	return vec3mulR(block->side[side].luminance_p[block_edge],0.5f);
}

void setLightMap(vec3* color,vec3 pos,int side,int depth){
	vec2 dir = lookDirectionTable[side];
	vec3 normal = normalTable[side];	
	traverse_init_t init = initTraverse(pos);
	bool half = false;
	if(init.node->block){
		if(init.node->block->depth <= depth && !lightMapRePosition(&pos,side,tMin(depth,LM_MAXDEPTH - 1))){
			*color = VEC3_ZERO;
			return;
		}
		init = initTraverse(pos);
	}
	dir.x -= M_PI * 0.5f - M_PI * 0.5f / (LM_SIZE);
	dir.y -= M_PI * 0.5f - M_PI * 0.5f / (LM_SIZE);
	vec3 s_color = VEC3_ZERO;
	float weight_total = 0.0f;
	
	for(int x = 0;x < LM_SIZE;x++){
		for(int y = 0;y < LM_SIZE;y++){
			vec3 angle = getLookAngle(dir);
			blockray_t result = traverseTree(ray3Create(init.pos,angle),init.node);
			float weight = tAbsf(vec3dotR(angle,normal));
			if(!result.node){
				int side;
				vec2 uv = sampleCube(angle,&side);
				if(side == 1 && uv.x > 0.6f && uv.x < 0.8f && uv.y > 0.6f && uv.y < 0.8f)
					vec3addvec3(&s_color,vec3mulR((vec3){25.0f,40.0f,70.0f},weight));
				else
					vec3addvec3(&s_color,vec3mulR((vec3){1.0f,0.75f,0.5f},weight));
				weight_total += weight;
				dir.y += M_PI / LM_SIZE;
				continue;
			}
			block_t* block = result.node->block;
			material_t material = material_array[block->material];

			if(material.flags & MAT_LUMINANT){
				vec3addvec3(&s_color,vec3mulR(material.luminance,weight));
				weight_total += weight;
				dir.y += M_PI / LM_SIZE;
				continue;
			}

			vec3 block_pos;
			float block_size = (float)MAP_SIZE / (1 << block->depth) * 2.0f;
			block_pos.x = block->pos.x * block_size;
			block_pos.y = block->pos.y * block_size;
			block_pos.z = block->pos.z * block_size;
			plane_t plane = getPlane(block_pos,angle,result.side,block_size);
			vec3 plane_pos = vec3subvec3R(plane.pos,pos);
			float dst = rayIntersectPlane(plane_pos,angle,plane.normal);
			vec3 hit_pos = vec3addvec3R(pos,vec3mulR(angle,dst));
			vec2 uv = {(hit_pos.a[plane.x] - plane.pos.a[plane.x]) / block_size,(hit_pos.a[plane.y] - plane.pos.a[plane.y]) / block_size};

			float texture_x = uv.x * (32.0f / (1 << block->depth));
			float texture_y = uv.y * (32.0f / (1 << block->depth));
			
			int side = result.side * 2 + (angle.a[result.side] < 0.0f);
			uv.x = tMaxf(tMinf(uv.x,1.0f),0.0f);
			uv.y = tMaxf(tMinf(uv.y,1.0f),0.0f);

			vec3 hit_block_luminance;
			portal_t portal = block->side[side].portal;


			if(portal.block){
				int x_table[] = {VEC3_Y,VEC3_X,VEC3_X};
				int y_table[] = {VEC3_Z,VEC3_Z,VEC3_Y};
				int portal_x = x_table[portal.side >> 1];
				int portal_y = y_table[portal.side >> 1];

				vec3 portal_pos = {portal.block->pos.x,portal.block->pos.y,portal.block->pos.z};
				vec3mul(&portal_pos,block_size);

				portal_pos.a[portal_x] += hit_pos.a[plane.x] - plane.pos.a[plane.x];
				portal_pos.a[portal_y] += hit_pos.a[plane.y] - plane.pos.a[plane.y];

				if(side == portal.side)
					angle.a[side >> 1] = -angle.a[side >> 1];

				else if(side >> 1 != portal.side >> 1){
					float temp = angle.a[portal.side >> 1] * -((portal.side & 1) * 2 - 1);
					angle.a[portal.side >> 1] = angle.a[side >> 1] * ((portal.side & 1) * 2 - 1);
					angle.a[side >> 1] = temp;
				}

				portal_pos.a[portal.side >> 1] += (angle.a[portal.side >> 1] > 0.0f) * block_size * 1.0002f - 0.0001f; 
				hit_block_luminance = getReflectLuminance(portal_pos,angle);
			}
			else
				hit_block_luminance = getLuminance(block->side[side],uv,block->depth);

			vec3 texture;
			if(material.texture){
				pixel_t text = material.texture[(int)(tFract(texture_x) * material.texture[0].size) * material.texture[0].size + (int)(tFract(texture_y) * material.texture[0].size)];

				texture = (vec3){text.r / 255.0f,text.g / 255.0f,text.b / 255.0f};
			}
			else
				texture = (vec3){0.9f,0.9f,0.9f};

			vec3addvec3(&s_color,vec3mulR(vec3mulvec3R(texture,hit_block_luminance),weight));
			weight_total += weight;
			dir.y += M_PI / LM_SIZE;
		}
		dir.y -= M_PI;
		dir.x += M_PI / LM_SIZE;
	}
	vec3div(&s_color,weight_total);
	*color = s_color;
	if(half)
		vec3mul(color,0.5f);
}

void setLightSideBig(vec3* color,vec3 pos,int side,int depth){
	vec3add(&pos,0.5f);
	pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.02f;
	int x_table[3] = {VEC3_Y,VEC3_X,VEC3_X};
	int y_table[3] = {VEC3_Z,VEC3_Z,VEC3_Y};

	int v_x = x_table[side >> 1];
	int v_y = y_table[side >> 1];

	int size = 1 << LM_MAXDEPTH - depth;

	pos.a[v_x] -= 0.5f - 0.5f / size;
	pos.a[v_y] -= 0.5f - 0.5f / size;

	for(int x = 1;x < size + 1;x++){
		for(int y = 1;y < size + 1;y++){
			vec3 l_pos = vec3divR(pos,(float)(1 << depth) * 0.5f);
			setLightMap(&color[x * (size + 2) + y],l_pos,side,depth);
			pos.a[v_y] += 1.0f / size;
		}
		pos.a[v_y] -= 1.0f;
		pos.a[v_x] += 1.0f / size;
	}
	for(int i = 1;i < size + 1;i++){
		color[i * (size + 2) + 0] = color[i * (size + 2) + 1];
		color[i * (size + 2) + size + 1] = color[i * (size + 2) + size];
		
		color[i + 0 * (size + 2)] = color[i + 1 * (size + 2)];
		color[i + (size + 1) * (size + 2)] = color[i + size * (size + 2)];
	}
	color[0] = vec3avgvec3R(color[1],color[size + 2]);
	color[size + 1] = vec3avgvec3R(color[size],color[size + 2 + size + 1]);
	color[(size + 2) * (size + 1)] = vec3avgvec3R(color[(size + 2) * size],color[(size + 2) * (size + 1) + 1]);
	color[(size + 2) * (size + 1) + size + 1] = vec3avgvec3R(color[(size + 2) * (size) + size + 1],color[(size + 2) * (size + 1) + size]);
}

void setLightSide(vec3* color,vec3 pos,int side,int depth){
	vec3add(&pos,0.5f);
	pos.a[side >> 1] -= 0.501f - (float)(side & 1) * 1.02f;
	vec3div(&pos,(float)(1 << depth) * 0.5f);
	setLightMap(color,pos,side,depth);
}