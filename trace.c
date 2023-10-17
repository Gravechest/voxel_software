#include <intrin.h>

#include "source.h"
#include "tree.h"
#include "trace.h"
#include "draw.h"
#include "lighting.h"
#include "tmath.h"

void tracePixel(traverse_init_t init,vec3 dir,int x,int y,vec3 camera_pos){
	node_hit_t result;
	if(tAbsf(dir.x) < 0.02f || tAbsf(dir.y) < 0.02f || tAbsf(dir.z) < 0.02f)
		result = traverseTree(ray3Create(init.pos,dir),init.node);
	else
		result = traverseTreeI(ray3CreateI(init.pos,dir),init.node);
	if(node_root[result.node].block){
		drawSurfacePixel(node_root[result.node],dir,camera_pos,result.side,x,y);
		return;
	}
	drawSky(x,y,1,dir);
}

static bool isInBlockSide(vec3 origin,vec3 ray_direction,float distance,plane_t plane,float block_size){
	vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
	vec2 uv = {
		hit_pos.a[plane.x] - plane.pos.a[plane.x],
		hit_pos.a[plane.y] - plane.pos.a[plane.y]
	};
	if(uv.x < 0.0f || uv.y < 0.0f || uv.x > block_size || uv.y > block_size)
		return false;
	return true;
}

void beamTrace4(traverse_init_t init[4],vec3 origin,int x,int y){
	block_node_t* node = node_root;
	beam_result_t beam_result;

	beam_result = traverseTreeBeamI(init,x,y,4);

	if(beam_result.side >= 0){
		vec3 ray_direction = getRayAngle(x + 2.5f,y + 2.5f);
		node_hit_t result;
		result.node = beam_result.node;
		result.side = beam_result.side;
		drawPixelGroup4(x,y,result,origin,ray_direction);
		return;
	}
	if(beam_result.side == -2){
		drawSky(x,y,4,getRayAngle(x,y));
		return;
	}

	uint duplicate[4] = {0,0,0,0};
	for(int i = 1;i < 4;i++){
		for(int j = 0;j < i;j++){
			bool p_x = beam_result.plane[i].pos.x == beam_result.plane[j].pos.x;
			bool p_y = beam_result.plane[i].pos.y == beam_result.plane[j].pos.y;
			bool p_z = beam_result.plane[i].pos.z == beam_result.plane[j].pos.z;
			bool p_d = beam_result.plane[i].z == beam_result.plane[j].z;
			if(p_x && p_y && p_z && p_d){
				duplicate[i] = 1;
				break;
			}
		}
	}
	pixel_t pixel_cache;
	uint cached_node = 0;
	uint cached_side = 0;
	/*
	if(test_bool){
		for(int j = x;j < x + 4;j++){
			for(int k = y;k < y + 4;k++){
				vram.draw[j * (int)WND_RESOLUTION.y + k] = (pixel_t){0,0,255};
			}
		}
		return;
	}
	*/
	for(int i = x;i < x + 4;i++){
		for(int j = y;j < y + 4;j++){
			float nearest_distance = 999999.0f;
			int nearest = -1;
			uint nearest_node = 0;
			vec3 ray_direction = getRayAngle(i,j);
			vec2 nearest_uv;
			for(int p = 0;p < 4;p++){
				if(duplicate[p])
					continue;
				plane_t plane = beam_result.plane[p];
				float block_size = (float)((1 << 20) >> node[beam_result.node_a[p]].depth) * MAP_SIZE_INV;
				vec3 plane_pos = vec3subvec3R(plane.pos,origin);
				float distance = ((plane.pos.a[plane.z] - origin.a[plane.z])) / ray_direction.a[plane.z];
				if(distance < 0.0f || nearest_distance < distance)
					continue;
				if(nearest_node == beam_result.node_a[p]){
					if(!isInBlockSide(origin,ray_direction,distance,plane,block_size))
						continue;

					if(nearest_distance > distance){
						nearest = p;
						nearest_distance = distance;
					}
					continue;
				}
				if(nearest_distance > distance){
					nearest_distance = distance;
					nearest = -1;
					nearest_node = beam_result.node_a[p];
				}
				vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
				vec2 uv = {
					hit_pos.a[plane.x] - plane.pos.a[plane.x],
					hit_pos.a[plane.y] - plane.pos.a[plane.y]
				};
				if(!node[beam_result.node_a[p]].block)
					continue;
				if(uv.x < 0.0f || uv.y < 0.0f || uv.x > block_size || uv.y > block_size)
					continue;
				nearest_uv = uv;
				nearest = p;
			}
			if(nearest == -1){
				vec3 pos_new = vec3addvec3R(origin,vec3mulR(ray_direction ,nearest_distance - 0.001f));
				tracePixel(initTraverse(pos_new),ray_direction ,i,j,origin);
				continue;
			}
			uint node_ptr = beam_result.node_a[nearest];
			uint node_side = beam_result.side_a[nearest];
			if(node_ptr == -1){
				drawSky(i,j,1,ray_direction);
				continue;
			}
			block_node_t node = node_root[node_ptr];
			block_t* block = node.block;
			if(!block)
				continue;
			float fog = nearest_distance * FOG_DISTANCE;
			if(fog > 1.0f){
				drawSky(i,j,1,ray_direction);
				continue;
			}
			material_t material = material_array[block->material];
			if(material.flags & MAT_LUMINANT){
				vec3 luminance = vec3mulR(material.luminance,camera_exposure);
				pixel_t color = {
					tMin(luminance.r * 255.0f,255),
					tMin(luminance.g * 255.0f,255),
					tMin(luminance.b * 255.0f,255)
				};
				vram.draw[i * (int)WND_RESOLUTION.y + j] = color;
				continue;
			}
			vec3 color;
			int side = node_side * 2 + (ray_direction.a[node_side] < 0.0f);
			if(cached_node == node_ptr && cached_side == side){
				vram.draw[i * (int)WND_RESOLUTION.y + j] = pixel_cache;
				continue;
			}
			cached_node = node_ptr;
			cached_side = side;
			float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
			vec2 uv = vec2divR(nearest_uv,block_size);
			vec3 luminance;
			plane_t plane = beam_result.plane[nearest];
			float distance = nearest_distance;
			vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
			if(setting_smooth_lighting)
				luminance = getLuminance(block->side[side],uv,node.depth);
			else{
				if(block->side[side].luminance_p){
					uint lm_size = 1 << tMax(LM_MAXDEPTH - node.depth,1);
					uint location = (int)(uv.x * lm_size + 1) * (lm_size + 2) + (int)(uv.y * lm_size + 1);
					luminance = block->side[side].luminance_p[location];
				}
			}
			vec3mul(&luminance,camera_exposure);

			if(!material.texture.data){
				color = vec3mulR(material.luminance,255.0f);
				vec3mulvec3(&color,luminance);
				color = vec3mixR(color,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
				pixel_t pixel = {tMin(color.r,0xff),tMin(color.g,0xff),tMin(color.b,0xff)};
				vram.draw[i * (int)WND_RESOLUTION.y + j] = pixel;
				pixel_cache = pixel;
				continue;
			}

			vec2 texture_uv = {hit_pos.a[plane.x] / block_size,hit_pos.a[plane.y] / block_size};
			uint mipmap_level = distance / (tAbsf(ray_direction.a[node_side]) * MIPMAP_AGGRESION);
			_BitScanReverse(&mipmap_level,mipmap_level + 1);

			uint texture_size = material.texture.size >> mipmap_level;
			uint texture_offset = 0;
			for(int i = 0;i < mipmap_level;i++)
				texture_offset += (material.texture.size >> i) * (material.texture.size >> i);

			float texture_x = texture_uv.x * TEXTURE_SCALE / material.texture.size / (8 << node.depth);
			float texture_y = texture_uv.y * TEXTURE_SCALE / material.texture.size / (8 << node.depth);

			texture_x = tMaxf(texture_x,0.0f);
			texture_y = tMaxf(texture_y,0.0f);
			texture_x = tFractUnsigned(texture_x);
			texture_y = tFractUnsigned(texture_y);
			texture_x *= texture_size;
			texture_y *= texture_size;

			uint location = (int)texture_x * texture_size + (int)texture_y + texture_offset;

			pixel_t text = material.texture.data[location];
			vec3 text_v = {text.r,text.g,text.b};
			vec3mulvec3(&text_v,luminance);
			text_v = vec3mixR(text_v,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
			text.r = tMin(text_v.r,0xff);
			text.g = tMin(text_v.g,0xff);
			text.b = tMin(text_v.b,0xff);
			vram.draw[i * (int)WND_RESOLUTION.y + j] = text;
			pixel_cache = text;
		}
	}
}

typedef struct{
	int node;
	plane_t plane;
	uint side;
}node_plane_t;

void beamTraceDiverse(int x,int y,node_plane_t* beam,uint count,vec3 origin){
	block_node_t* node = node_root;

	int nearest[4] = {-1,-1,-1,-1};
	float nearest_distance[4] = {999999.0f,999999.0f,999999.0f,999999.0f};
	float block_size[4];
	for(int i = 0;i < count;i++){
		int node_ptr = beam[i].node;
		if(node_ptr == -1)
			continue;
		if(!node[node_ptr].block)
			return;
		block_size[i] = (float)((1 << 20) >> node[beam[i].node].depth) * MAP_SIZE_INV;
	}
	for(int k = 0;k < 4;k++){
		uint r_x = x + (k / 2) * 3;
		uint r_y = y + (k & 1) * 3;
		vec3 ray_direction = getRayAngle(r_x,r_y);
		uint nearest_node = 0;
		for(int p = 0;p < count;p++){
			plane_t plane = beam[p].plane;
			float distance = (plane.pos.a[plane.z] - origin.a[plane.z]) / ray_direction.a[plane.z];
			if(distance < 0.0f)
				continue;

			if(nearest_node == beam[p].node){
				if(!isInBlockSide(origin,ray_direction,distance,plane,block_size[p]))
					continue;

				if(nearest_distance[k] > distance)
					nearest_distance[k] = distance;
				nearest[k] = p;
				continue;
			}
			if(nearest_distance[k] < distance)
				continue;
			if(nearest_distance[k] > distance){
				nearest_node = beam[p].node;
				nearest_distance[k] = distance;
				nearest[k] = -1;
			}
			if(!isInBlockSide(origin,ray_direction,distance,plane,block_size[p]))
					continue;
			
			nearest[k] = p;
		}
	}
	if(nearest[0] == -1 && nearest[1] == -1 && nearest[2] == -1 && nearest[3] == -1){
		traverse_init_t reinit[4];
		for(int k = 0;k < 4;k++){
			uint r_x = x + (k / 2) * 3;
			uint r_y = y + (k & 1) * 3;
			vec3 ray_angle = getRayAngle(r_x,r_y);
			vec3 pos_new = vec3addvec3R(origin,vec3mulR(ray_angle,nearest_distance[k] - 0.001f));
			reinit[k] = initTraverse(pos_new);
		}
		beamTrace4(reinit,origin,x,y);
		return;
	}
	if(nearest[0] == nearest[1] && nearest[0] == nearest[2] && nearest[0] == nearest[3]){
		vec3 ray_direction = getRayAngle(x + 2.5f,y + 2.5f);
		node_hit_t result;
		result.node = beam[nearest[0]].node;
		result.side = beam[nearest[0]].side;
		drawPixelGroup4(x,y,result,origin,ray_direction);
		return;
	}
	/*
	if(test_bool){
		for(int j = x;j < x + 4;j++){
			for(int k = y;k < y + 4;k++){
				vram.draw[j * (int)WND_RESOLUTION.y + k] = (pixel_t){0,0,255};
			}
		}
		return;
	}
	*/
	pixel_t pixel_cache;
	uint cached_node = 0;
	uint cached_side = 0;
	for(int j = x;j < x + 4;j++){
		for(int k = y;k < y + 4;k++){
			float nearest_distance = 999999.0f;
			int nearest2 = -1;
			vec2 nearest_uv;
			uint nearest_node = 0;
			vec3 ray_direction = getRayAngle(j,k);
			for(int p = 0;p < count;p++){
				float block_size = (float)((1 << 20) >> node[beam[p].node].depth) * MAP_SIZE_INV;
				plane_t plane = beam[p].plane;

				float distance = ((plane.pos.a[plane.z] - origin.a[plane.z])) / ray_direction.a[plane.z];
				if(distance < 0.0f)
					continue;
				if(nearest_node == beam[p].node){
					vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
					vec2 uv = {
						hit_pos.a[plane.x] - plane.pos.a[plane.x],
						hit_pos.a[plane.y] - plane.pos.a[plane.y]
					};
					if(uv.x < 0.0f || uv.y < 0.0f || uv.x > block_size || uv.y > block_size)
						continue;

					if(nearest_distance > distance){
						nearest2 = p;
						nearest_distance = distance;
					}
					continue;
				}
				if(nearest_distance < distance)
					continue;
				if(nearest_distance > distance){
					if(!beam[p].node){			
						nearest2 = p;
						continue;
					}
					nearest_distance = distance;
					nearest2 = -1;
					nearest_node = p;
				}
				
				vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,distance));
				vec2 uv = {
					hit_pos.a[plane.x] - plane.pos.a[plane.x],
					hit_pos.a[plane.y] - plane.pos.a[plane.y]
				};
				if(beam[p].node == -1 || !node[beam[p].node].block)
					continue;
				if(uv.x < 0.0f || uv.y < 0.0f || uv.x > block_size || uv.y > block_size)
					continue;
				
				nearest_uv = uv;
				nearest2 = p;
			}
			if(nearest2 == -1){
				vec3 pos_new = vec3addvec3R(origin,vec3mulR(ray_direction,nearest_distance - 0.001f));
				tracePixel(initTraverse(pos_new),ray_direction,j,k,origin);
				continue;
			}
			if(nearest_distance == 999999.0f){
				drawSky(j,k,block_size,ray_direction);
				continue;
			}
			float fog = nearest_distance * FOG_DISTANCE;
			if(fog > 1.0f){
				drawSky(j,k,1,ray_direction);
				continue;
			}
			node_hit_t result;
			result.node = beam[nearest2].node;
			result.side = beam[nearest2].side;
			int side = result.side * 2 + (ray_direction.a[result.side] < 0.0f);
			if(test_bool && cached_node == result.node && cached_side == side){
				vram.draw[j * (int)WND_RESOLUTION.y + k] = pixel_cache;
				continue;
			}
			if(result.node == -1)
				continue;
			cached_node = result.node;
			cached_side = side;
			block_node_t node = node_root[result.node];
			block_t* block = node.block;
			if(!block)
				continue;
			material_t material = material_array[block->material];
			if(material.flags & MAT_LUMINANT){
				vec3 luminance = vec3mulR(material.luminance,camera_exposure);
				pixel_t color = {
					tMin(luminance.r * 255.0f,255),
					tMin(luminance.g * 255.0f,255),
					tMin(luminance.b * 255.0f,255)
				};
				vram.draw[j * (int)WND_RESOLUTION.y + k] = color;
				pixel_cache = color;
				continue;
			}
			float block_size = (float)((1 << 20) >> node.depth) * MAP_SIZE_INV;
			plane_t plane = beam[nearest2].plane;
			vec3 hit_pos = vec3addvec3R(origin,vec3mulR(ray_direction,nearest_distance));
			vec3 color;
			vec2 uv = vec2divR(nearest_uv,block_size);
			vec3 luminance;
			if(setting_smooth_lighting)
				luminance = getLuminance(block->side[side],uv,node.depth);
			else{
				if(block->side[side].luminance_p){
					uint lm_size = 1 << tMax(LM_MAXDEPTH - node.depth,0);
					uint location = (int)(uv.x * lm_size + 1) * (lm_size + 2) + (int)(uv.y * lm_size + 1);
					luminance = block->side[side].luminance_p[location];
				}
			}

			vec3mul(&luminance,camera_exposure);

			if(!material.texture.data){
				color = vec3mulR(material.luminance,255.0f); 
				vec3mulvec3(&color,luminance);
				color = vec3mixR(color,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
				pixel_t pixel = {tMin(color.r,0xff),tMin(color.g,0xff),tMin(color.b,0xff)};
				vram.draw[j * (int)WND_RESOLUTION.y + k] = pixel;
				pixel_cache = pixel;
				continue;
			}

			vec2 texture_uv = {
				hit_pos.a[plane.x] / block_size,
				hit_pos.a[plane.y] / block_size
			};
			uint mipmap_level = nearest_distance / (tAbsf(ray_direction.a[result.side]) * MIPMAP_AGGRESION);
			_BitScanReverse(&mipmap_level,mipmap_level + 1);

			uint texture_size = material.texture.size >> mipmap_level;
			uint texture_offset = 0;
			for(int i = 0;i < mipmap_level;i++)
				texture_offset += (material.texture.size >> i) * (material.texture.size >> i);

			float texture_x = texture_uv.x * TEXTURE_SCALE / material.texture.size / (8 << node.depth);
			float texture_y = texture_uv.y * TEXTURE_SCALE / material.texture.size / (8 << node.depth);

			texture_x = tMaxf(texture_x,0.0f);
			texture_y = tMaxf(texture_y,0.0f);
			texture_x = tFractUnsigned(texture_x);
			texture_y = tFractUnsigned(texture_y);
			texture_x *= texture_size;
			texture_y *= texture_size;

			uint location = (int)texture_x * texture_size + (int)texture_y + texture_offset;

			pixel_t text = material.texture.data[location];
			vec3 text_v = {text.r,text.g,text.b};
			vec3mulvec3(&text_v,luminance);
			text_v = vec3mixR(text_v,vec3mulR(vec3mulR(LUMINANCE_SKY,256.0f),camera_exposure),fog);
			text.r = tMin(text_v.r,0xff);
			text.g = tMin(text_v.g,0xff);
			text.b = tMin(text_v.b,0xff);
			vram.draw[j * (int)WND_RESOLUTION.y + k] = text;
			pixel_cache = text;
		}
	}
}

void beamTrace8(traverse_init_t init[4],vec3 origin,int x,int y){
	block_node_t* node = node_root;
	beam_result_t beam_result;

	beam_result = traverseTreeBeamI(init,x,y,8);

	if(beam_result.side >= 0){
		vec3 ray_direction = getRayAngle(x + 4.5f,y + 4.5f);
		node_hit_t result;
		result.node = beam_result.node;
		result.side = beam_result.side;
		drawPixelGroup8(x,y,result,origin,ray_direction);
		return;
	}
	if(beam_result.side == -2){
		drawSky(x,y,8,getRayAngle(x,y));
		return;
	}
	uint duplicate[4] = {0,0,0,0};
	for(int i = 1;i < 4;i++){
		for(int j = 0;j < i;j++){
			bool p_x = beam_result.plane[i].pos.x == beam_result.plane[j].pos.x;
			bool p_y = beam_result.plane[i].pos.y == beam_result.plane[j].pos.y;
			bool p_z = beam_result.plane[i].pos.z == beam_result.plane[j].pos.z;
			bool p_d = beam_result.plane[i].z == beam_result.plane[j].z;
			if(p_x && p_y && p_z && p_d){
				duplicate[i] = 1;
				break;
			}
		}
	}
	uint count = 0;
	node_plane_t node_plane[4];
	for(int i = 0;i < 4;i++){
		if(duplicate[i])
			continue;
		node_plane[count].plane = beam_result.plane[i];
		node_plane[count].side = beam_result.side_a[i];
		node_plane[count].node = beam_result.node_a[i];
		count++;
	}
	for(int i = x;i < x + 8;i += 4)
		for(int j = y;j < y + 8;j += 4)
			beamTraceDiverse(i,j,node_plane,count,origin);
}

void beamTraceDiverse8(int x,int y,node_plane_t* beam,uint count,vec3 origin,traverse_init_t init){
	block_node_t* node = node_root;

	int nearest[4] = {-1,-1,-1,-1};
	float nearest_distance[4] = {999999.0f,999999.0f,999999.0f,999999.0f};
	float block_size[4];
	for(int i = 0;i < count;i++){
		int node_ptr = beam[i].node;
		if(node_ptr == -1)
			continue;
		if(!node[node_ptr].block)
			return;
		block_size[i] = (float)((1 << 20) >> node[beam[i].node].depth) * MAP_SIZE_INV;
	}
	for(int k = 0;k < 4;k++){
		uint r_x = x + (k / 2) * 7;
		uint r_y = y + (k & 1) * 7;
		vec3 ray_direction = getRayAngle(r_x,r_y);
		uint nearest_node = 0;
		for(int p = 0;p < count;p++){
			plane_t plane = beam[p].plane;
			float distance = (plane.pos.a[plane.z] - origin.a[plane.z]) / ray_direction.a[plane.z];
			if(distance < 0.0f)
				continue;

			if(nearest_node == beam[p].node){
				if(!isInBlockSide(origin,ray_direction,distance,plane,block_size[p]))
					continue;

				if(nearest_distance[k] > distance)
					nearest_distance[k] = distance;
				nearest[k] = p;
				continue;
			}
			if(nearest_distance[k] < distance)
				continue;
			if(nearest_distance[k] > distance){
				nearest_node = beam[p].node;
				nearest_distance[k] = distance;
				nearest[k] = -1;
			}
			if(!isInBlockSide(origin,ray_direction,distance,plane,block_size[p]))
				continue;
			
			nearest[k] = p;
		}
	}
	if(nearest[0] == -1 && nearest[1] == -1 && nearest[2] == -1 && nearest[3] == -1){
		traverse_init_t reinit[4];
		for(int k = 0;k < 4;k++){
			uint r_x2 = x + (k / 2) * 7;
			uint r_y2 = y + (k & 1) * 7;
			vec3 ray_angle = getRayAngle(r_x2,r_y2);
			vec3 pos_new = vec3addvec3R(origin,vec3mulR(ray_angle,nearest_distance[k] - 0.001f));
			reinit[k] = initTraverse(pos_new);
		}
		beamTrace8(reinit,origin,x,y);
		return;
	}
	if(nearest[0] == nearest[1] && nearest[0] == nearest[2] && nearest[0] == nearest[3]){
		vec3 ray_direction = getRayAngle(x + 4.5f,y + 4.5f);
		node_hit_t result;
		result.node = beam[nearest[0]].node;
		result.side = beam[nearest[0]].side;
		drawPixelGroup8(x,y,result,origin,ray_direction);
		return;
	}
	beamTraceDiverse(x,y,beam,count,origin,init);
	beamTraceDiverse(x,y + 4,beam,count,origin,init);
	beamTraceDiverse(x + 4,y,beam,count,origin,init);
	beamTraceDiverse(x + 4,y + 4,beam,count,origin,init);
}

void beamTrace16(traverse_init_t init,vec3 origin,uint x,uint y,uint thread){
	traverse_init_t init_a[4] = {init,init,init,init};
	block_node_t* node = node_root;
	
	beam_result_t beam_result = traverseTreeBeamI(init_a,x,y,16);

	if(beam_result.side == -3)
		return;

	if(beam_result.side >= 0){
		vec3 ray_direction = getRayAngle(x + 8.5f,y + 8.5f);
		node_hit_t result;
		result.node = beam_result.node;
		result.side = beam_result.side;
		drawPixelGroup16(x,y,result,origin,ray_direction);
		return;
	}
	if(beam_result.side == -2){
		drawSky16(x,y,getRayAngle(x,y));
		return;
	}
	uint duplicate[4] = {0,0,0,0};
	for(int i = 1;i < 4;i++){
		for(int j = 0;j < i;j++){
			bool p_x = beam_result.plane[i].pos.x == beam_result.plane[j].pos.x;
			bool p_y = beam_result.plane[i].pos.y == beam_result.plane[j].pos.y;
			bool p_z = beam_result.plane[i].pos.z == beam_result.plane[j].pos.z;
			bool p_d = beam_result.plane[i].z == beam_result.plane[j].z;
			if(p_x && p_y && p_z && p_d){
				duplicate[i] = 1;
				break;
			}
		}
	}
	uint count = 0;
	node_plane_t node_plane[4];
	for(int i = 0;i < 4;i++){
		if(duplicate[i])
			continue;
		node_plane[count].plane = beam_result.plane[i];
		node_plane[count].side = beam_result.side_a[i];
		node_plane[count].node = beam_result.node_a[i];
		count++;
	}
	/*
	uint material = node_root[node_plane[0].node].block->material;
	uint side = node_plane[0].side;
	uint distance = node_plane[0].plane.pos.a[side];
	if(test_bool){
	for(int i = 1;i < count;i++){
		uint side2 = node_plane[i].side;
		uint distance2 = node_plane[i].plane.pos.a[side2];
		if(node_root[node_plane[i].node].block->material != material || side2 != side || distance2 != distance){
			beamTraceDiverse8(x,y,node_plane,count,origin,init);
			beamTraceDiverse8(x,y + 8,node_plane,count,origin,init);
			beamTraceDiverse8(x + 8,y,node_plane,count,origin,init);
			beamTraceDiverse8(x + 8,y + 8,node_plane,count,origin,init);
			return;
		}
	}
	for(int i = 0;i < 16;i++){
		for(int j = 0;j < 16;j++){
			vram.draw[(x + i) * (int)WND_RESOLUTION.y + (y + j)] = (pixel_t){0,0,255};
		}
	}
	return;
	}
	*/
	beamTraceDiverse8(x,y,node_plane,count,origin,init);
	beamTraceDiverse8(x,y + 8,node_plane,count,origin,init);
	beamTraceDiverse8(x + 8,y,node_plane,count,origin,init);
	beamTraceDiverse8(x + 8,y + 8,node_plane,count,origin,init);
}