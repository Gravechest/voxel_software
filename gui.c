#include "gui.h"
#include "opengl.h"

void guiRectangle(vec2_t pos,vec2_t size,vec3_t color){
	triangles[triangle_count + 0] = (triangle_t){pos.x,pos.y,1.0f,0.0f,0.0f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 1] = (triangle_t){pos.x + size.x,pos.y,1.0f,0.01f,0.0f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 2] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,0.01f,0.01f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 3] = (triangle_t){pos.x,pos.y,1.0f,0.0f,0.0f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 4] = (triangle_t){pos.x,pos.y + size.y,1.0f,0.0f,0.01f,color.r,color.g,color.b,1.0f};
	triangles[triangle_count + 5] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,0.01f,0.01f,color.r,color.g,color.b,1.0f};
	triangle_count += 6;
}

void guiFrame(vec2_t pos,vec2_t size,vec3_t color,float thickness){
	guiRectangle(pos,(vec2_t){size.x,thickness},color);
	guiRectangle(pos,(vec2_t){thickness * RD_RATIO,size.y},color);
	guiRectangle((vec2_t){pos.x,pos.y + size.y},(vec2_t){size.x + thickness * RD_RATIO,thickness},color);
	guiRectangle((vec2_t){pos.x + size.x,pos.y},(vec2_t){thickness * RD_RATIO,size.y},color);
}

void drawChar(vec2_t pos,vec2_t size,char c){
	c -= 0x21;
	int x = 9 - c / 10;
	int y = c % 10;

	float t_y = (1.0f / 2048) * (x * 8) + (1.0f / 2048.0f * 256.0f);
	float t_x = (1.0f / 2048) * y * 8;

	vec2_t texture_coord[4];
	texture_coord[0] = (vec2_t){t_x,t_y};
	texture_coord[1] = (vec2_t){t_x,t_y + (1.0f / (2048)) * 8};
	texture_coord[2] = (vec2_t){t_x + (1.0f / (2048)) * 8,t_y};
	texture_coord[3] = (vec2_t){t_x + (1.0f / (2048)) * 8,t_y + (1.0f / (2048)) * 8};

	triangles[triangle_count++] = (triangle_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x,pos.y + size.y,1.0f,texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y,1.0f,texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
}

void drawNumber(vec2_t pos,vec2_t size,uint32_t number){
	char str[8];
	if(!number){
		drawChar(pos,size,'0');
		return;
	}
	uint32_t length = 0;
	for(uint32_t i = number;i;i /= 10)
		length++;
	pos.x += size.x * length;
	for(int i = 0;i < length;i++){
		drawChar(pos,size,number % 10 + 0x30);
		number /= 10;
		pos.x -= size.x;
	}
}

void drawString(vec2_t pos,float size,char* str){
	while(*str){
		drawChar(pos,(vec2_t){size,size},*str++);
		pos.x += size;
	}
}

void guiInventoryContent(inventoryslot_t* slot,vec2_t pos,vec2_t size){
	if(slot->type == -1)
		return;
	drawNumber(vec2addvec2R(pos,(vec2_t){-0.04f,0.065f}),vec2mulR(size,0.3f),slot->ammount);
	material_t material = material_array[slot->type];
	vec2_t texture_coord[4];
	texture_coord[0] = material.texture_pos;
	texture_coord[1] = (vec2_t){material.texture_pos.x,material.texture_pos.y + material.texture_size.y};
	texture_coord[2] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y};
	texture_coord[3] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y + material.texture_size.y};

	triangles[triangle_count++] = (triangle_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x,pos.y + size.y,1.0f,texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x,pos.y,1.0f,texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y,1.0f,texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],1.0f,1.0f,1.0f,1.0f};
}