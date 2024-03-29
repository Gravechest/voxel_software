#include "gui.h"
#include "opengl.h"
#include "tree.h"

dynamic_array_t gui_array[] = {
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
	{.element_size = sizeof(gui_t)},
};

void copyGUI(dynamic_array_t* dst,dynamic_array_t src){
	for(int i = 0;i < src.size;i++)
		dynamicArrayAdd(dst,dynamicArrayGet(src,i));
}

void initGUI(){
	gui_t element;
	element = (gui_t){
		.offset = {0.26f - 0.31f,0.535f - 0.2f},
		.type = GUI_ELEMENT_PROGRESS,
		.size = {0.19f,0.05f},
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_INVENTORY],&element);
	element = (gui_t){
		.offset = {0.079f - 0.31f,0.505f - 0.2f},
		.type = GUI_ELEMENT_ITEM,
		.value = 0,
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_INVENTORY],&element);
	element = (gui_t){
		.offset = {0.179f - 0.31f,0.505f - 0.2f},
		.type = GUI_ELEMENT_ITEM,
		.value = 1,
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_INVENTORY],&element);
	element = (gui_t){
		.offset = {0.485f - 0.31f,0.505f - 0.2f},
		.type = GUI_ELEMENT_ITEM,
		.value = 2,
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_INVENTORY],&element);
	element = (gui_t){
		.offset = {-0.193f,0.47f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.1f * RD_RATIO,0.1f},
			.f_void = increaseCraftAmmount,
			.value = -1,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_AMMOUNT],&element);
	element = (gui_t){
		.offset =  {-0.253f,0.47f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.1f * RD_RATIO,0.1f},
			.f_void = decreaseCraftAmmount,
			.value = -1
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_AMMOUNT],&element);
	element = (gui_t){
		.offset = {0.0f * RD_RATIO,-0.1f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.35f},
			.f_int = changeCraftRecipy,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {{0.0f * RD_RATIO,-0.45f}},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.35f},
			.f_int = changeCraftRecipy,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {-0.45f * RD_RATIO,-0.1f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.35f},
			.f_int = changeCraftRecipy,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {-0.45f * RD_RATIO,-0.45f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.35f},
			.f_int = changeCraftRecipy,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {-0.05f * RD_RATIO,0.0f},
		.type = GUI_ELEMENT_ITEM,
		.value = 0,
	};
	dynamicArrayAdd(&gui_array[GUI_GENERATOR],&element);
	element = (gui_t){
		.offset = {-0.2f,-0.15f},
		.type = GUI_ELEMENT_PROGRESS,
		.size = {0.4f,0.1f},
		.value = 0,
	};
	dynamicArrayAdd(&gui_array[GUI_GENERATOR],&element);

	copyGUI(&gui_array[GUI_MINER],gui_array[GUI_GENERATOR]);

	copyGUI(&gui_array[GUI_CRAFT_MENU],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[0].button.value = RECIPY_TORCH;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[1].button.value = RECIPY_ELEKTRIC1;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[2].button.value = RECIPY_RESEARCH1;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[3].button.value = RECIPY_BASIC;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[0].button.f_void = playerCraft;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[1].button.f_void = playerCraft;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[2].button.f_void = playerCraft;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[3].button.f_void = playerCraft;
	copyGUI(&gui_array[GUI_CRAFT_MENU],gui_array[GUI_CRAFT_AMMOUNT]);
	
	copyGUI(&gui_array[GUI_CRAFT_BASIC],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[0].button.value = RECIPY_FURNACE;
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[1].button.value = 0;
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[2].button.value = 0;
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[3].button.value = 0;
	copyGUI(&gui_array[GUI_CRAFT_BASIC],gui_array[GUI_CRAFT_INVENTORY]);

	copyGUI(&gui_array[GUI_CRAFT_ELEKTRIC1],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[0].button.value = RECIPY_GENERATOR;
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[1].button.value = RECIPY_ELEKTRIC1;
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[2].button.value = RECIPY_RESEARCH1;
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[3].button.value = RECIPY_BASIC;
	copyGUI(&gui_array[GUI_CRAFT_ELEKTRIC1],gui_array[GUI_CRAFT_INVENTORY]);

	copyGUI(&gui_array[GUI_FURNACE],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_FURNACE].data)[0].button.value = RECIPY_COPPER;
	((gui_t*)gui_array[GUI_FURNACE].data)[1].button.value = RECIPY_IRON;
	((gui_t*)gui_array[GUI_FURNACE].data)[2].button.value = RECIPY_RESEARCH1;
	((gui_t*)gui_array[GUI_FURNACE].data)[3].button.value = RECIPY_BASIC;
	copyGUI(&gui_array[GUI_FURNACE],gui_array[GUI_CRAFT_INVENTORY]);
}

void blockGUIrectangle(vec3_t block_pos,ivec3 axis_table,float block_size,vec2_t pos,vec2_t size,vec3_t color){
	block_pos.a[axis_table.x] += block_size * pos.x;
	block_pos.a[axis_table.y] += block_size * pos.y;
	vec3_t vertex[4] = {block_pos,block_pos,block_pos,block_pos};
	vertex[1].a[axis_table.x] += block_size * size.x;
	vertex[2].a[axis_table.y] += block_size * size.y;
	vertex[3].a[axis_table.x] += block_size * size.x;
	vertex[3].a[axis_table.y] += block_size * size.y;
	vec3_t point[4] = {
		pointToScreenRenderer(vertex[0]),	
		pointToScreenRenderer(vertex[1]),	
		pointToScreenRenderer(vertex[2]),	
		pointToScreenRenderer(vertex[3]),	
	};
	vec3_t screen_point[4];
	for(int k = 0;k < 4;k++){
		screen_point[k].z = point[k].z;
		screen_point[k].y = point[k].x;
		screen_point[k].x = point[k].y;
	}
	triangles[triangle_count++] = (triangle_t){screen_point[0],(vec2_t){0.0f,0.0f}  ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[1],(vec2_t){0.01f,0.0f} ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],(vec2_t){0.01f,0.01f},color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[0],(vec2_t){0.0f,0.0f}  ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[2],(vec2_t){0.0f,0.01f} ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],(vec2_t){0.01f,0.01f},color,1.0f};
}

void blockGUIrectangleRotate(vec3_t block_pos,ivec3 axis_table,float block_size,vec2_t pos,vec2_t size,vec3_t color,float rotation){
	block_pos.a[axis_table.x] += block_size * pos.x;
	block_pos.a[axis_table.y] += block_size * pos.y;
	vec3_t vertex[4] = {VEC3_ZERO,VEC3_ZERO,VEC3_ZERO,VEC3_ZERO};
	vertex[1].a[axis_table.x] += block_size * size.x;
	vertex[2].a[axis_table.y] += block_size * size.y;
	vertex[3].a[axis_table.x] += block_size * size.x;
	vertex[3].a[axis_table.y] += block_size * size.y;
	for(int i = 0;i < 4;i++){
		vec2_t r = vec2rotR((vec2_t){vertex[i].a[axis_table.x],vertex[i].a[axis_table.y]},rotation);
		vertex[i].a[axis_table.x] = r.x;
		vertex[i].a[axis_table.y] = r.y;
		vec3addvec3(&vertex[i],block_pos);
	}
	vec3_t point[4] = {
		pointToScreenRenderer(vertex[0]),	
		pointToScreenRenderer(vertex[1]),	
		pointToScreenRenderer(vertex[2]),	
		pointToScreenRenderer(vertex[3]),	
	};
	vec3_t screen_point[4];
	for(int k = 0;k < 4;k++){
		screen_point[k].z = point[k].z;
		screen_point[k].y = point[k].x;
		screen_point[k].x = point[k].y;
	}
	triangles[triangle_count++] = (triangle_t){screen_point[0],(vec2_t){0.0f,0.0f}  ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[1],(vec2_t){0.01f,0.0f} ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],(vec2_t){0.01f,0.01f},color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[0],(vec2_t){0.0f,0.0f}  ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[2],(vec2_t){0.0f,0.01f} ,color,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],(vec2_t){0.01f,0.01f},color,1.0f};
}

void blockGUIframe(vec3_t block_pos,ivec3 axis_table,float block_size,vec2_t pos,vec2_t size,vec3_t color,float thickness){
	blockGUIrectangle(block_pos,axis_table,block_size,pos,(vec2_t){size.x,thickness},color);
	blockGUIrectangle(block_pos,axis_table,block_size,pos,(vec2_t){thickness * RD_RATIO,size.y},color);
	blockGUIrectangle(block_pos,axis_table,block_size,(vec2_t){pos.x,pos.y + size.y},(vec2_t){size.x + thickness * RD_RATIO,thickness},color);
	blockGUIrectangle(block_pos,axis_table,block_size,(vec2_t){pos.x + size.x,pos.y},(vec2_t){thickness * RD_RATIO,size.y},color);
}

void blockGUIchar(vec3_t block_pos,ivec3 axis_table,float block_size,vec2_t pos,vec2_t size,char c){
	if(c == ' ')
		return;
	c -= 0x21;
	int x = 9 - c / 10;
	int y = c % 10;

	float t_y = (1.0f / 2048) * (x * 8) + (1.0f / 2048.0f * 256.0f);
	float t_x = (1.0f / 2048) * y * 8;

	vec2_t texture_coord[4];
	texture_coord[0] = (vec2_t){t_x,t_y};
	texture_coord[2] = (vec2_t){t_x,t_y + (1.0f / (2048)) * 8};
	texture_coord[1] = (vec2_t){t_x + (1.0f / (2048)) * 8,t_y};
	texture_coord[3] = (vec2_t){t_x + (1.0f / (2048)) * 8,t_y + (1.0f / (2048)) * 8};

	block_pos.a[axis_table.x] += block_size * pos.x;
	block_pos.a[axis_table.y] += block_size * pos.y;
	vec3_t vertex[4] = {block_pos,block_pos,block_pos,block_pos};
	vertex[1].a[axis_table.x] += block_size * size.x;
	vertex[2].a[axis_table.y] += block_size * size.y;
	vertex[3].a[axis_table.x] += block_size * size.x;
	vertex[3].a[axis_table.y] += block_size * size.y;
	vec3_t point[4] = {
		pointToScreenRenderer(vertex[0]),	
		pointToScreenRenderer(vertex[1]),	
		pointToScreenRenderer(vertex[2]),	
		pointToScreenRenderer(vertex[3]),	
	};
	vec3_t screen_point[4];
	for(int k = 0;k < 4;k++){
		screen_point[k].z = point[k].z;
		screen_point[k].y = point[k].x;
		screen_point[k].x = point[k].y;
	}
	triangles[triangle_count++] = (triangle_t){screen_point[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[1],texture_coord[1],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[0],texture_coord[0],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[2],texture_coord[2],1.0f,1.0f,1.0f,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],texture_coord[3],1.0f,1.0f,1.0f,1.0f};
}

void blockGUInumber(vec3_t block_pos,ivec3 axis_table,float block_size,vec2_t pos,vec2_t size,uint32_t number){
	char str[8];
	if(!number){
		blockGUIchar(block_pos,axis_table,block_size,pos,size,'0');
		return;
	}
	uint32_t length = 0;
	for(uint32_t i = number;i;i /= 10)
		length++;
	pos.x += size.x * (length - 1);
	for(int i = 0;i < length;i++){
		blockGUIchar(block_pos,axis_table,block_size,pos,size,number % 10 + 0x30);
		number /= 10;
		pos.x -= size.x;
	}
}

void blockGUIinventoryContent(vec3_t block_pos,ivec3 axis_table,float block_size,item_t* slot,vec2_t pos,vec2_t size){
	if(slot->type == ITEM_VOID)
		return;
	uint32_t ammount = slot->ammount;
	char behind = 0;
	if(ammount > 99999999){
		behind = 'M';
		ammount /= 1000000;
	}
	else if(ammount > 9999){
		behind = 'K';
		ammount /= 1000;
	}
	if(behind){
		if(ammount < 100)
			blockGUIchar(block_pos,axis_table,block_size,vec2addvec2R(pos,(vec2_t){0.03f,0.085f}),vec2mulR(size,0.3f),behind);
		else if (ammount < 1000)
			blockGUIchar(block_pos,axis_table,block_size,vec2addvec2R(pos,(vec2_t){0.048f,0.085f}),vec2mulR(size,0.3f),behind);
		else
			blockGUIchar(block_pos,axis_table,block_size,vec2addvec2R(pos,(vec2_t){0.066f,0.085f}),vec2mulR(size,0.3f),behind);
	}
	blockGUInumber(block_pos,axis_table,block_size,vec2addvec2R(pos,(vec2_t){-0.005f,0.085f}),vec2mulR(size,0.3f),ammount);
	material_t material = material_array[slot->type];
	vec2_t texture_coord[4];
	texture_coord[0] = material.texture_pos;
	texture_coord[1] = (vec2_t){material.texture_pos.x,material.texture_pos.y + material.texture_size.y};
	texture_coord[2] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y};
	texture_coord[3] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y + material.texture_size.y};

	block_pos.a[axis_table.x] += block_size * pos.x;
	block_pos.a[axis_table.y] += block_size * pos.y;
	vec3_t vertex[4] = {block_pos,block_pos,block_pos,block_pos};
	vertex[1].a[axis_table.x] += block_size * size.x;
	vertex[2].a[axis_table.y] += block_size * size.y;
	vertex[3].a[axis_table.x] += block_size * size.x;
	vertex[3].a[axis_table.y] += block_size * size.y;
	vec3_t point[4] = {
		pointToScreenRenderer(vertex[0]),	
		pointToScreenRenderer(vertex[1]),	
		pointToScreenRenderer(vertex[2]),	
		pointToScreenRenderer(vertex[3]),	
	};
	vec3_t screen_point[4];
	for(int k = 0;k < 4;k++){
		screen_point[k].z = point[k].z;
		screen_point[k].y = point[k].x;
		screen_point[k].x = point[k].y;
	}

	triangles[triangle_count++] = (triangle_t){screen_point[0],texture_coord[0],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[1],texture_coord[1],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],texture_coord[3],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[0],texture_coord[0],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[2],texture_coord[2],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){screen_point[3],texture_coord[3],material.luminance,1.0f};
}

void blockGUIinventory(vec3_t block_pos,ivec3 axis_table,float block_size,item_t* item,vec2_t pos,vec3_t frame_color){
	blockGUIinventoryContent(block_pos,axis_table,block_size,item,pos,(vec2_t){0.1f,0.1f});
	blockGUIframe(block_pos,axis_table,block_size,vec2subvec2R(pos,(vec2_t){0.02f,0.025f}),(vec2_t){0.15f,0.15f},VEC3_ZERO,0.01f);
	blockGUIframe(block_pos,axis_table,block_size,vec2subvec2R(pos,(vec2_t){0.022f,0.03f}),(vec2_t){0.15f,0.15f},frame_color,0.02f);
};

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
	if(c == ' ')
		return;
	c -= 0x21;
	int x = c / 10;
	int y = c % 10;

	float t_y = (1.0f / 2048) * (x * 8) + (1.0f / 2048.0f * 256.0f);
	float t_x = (1.0f / 2048) * y * 8;

	vec2_t texture_coord[4];
	texture_coord[1] = (vec2_t){t_x,t_y};
	texture_coord[0] = (vec2_t){t_x,t_y + (1.0f / (2048)) * 8};
	texture_coord[3] = (vec2_t){t_x + (1.0f / (2048)) * 8,t_y};
	texture_coord[2] = (vec2_t){t_x + (1.0f / (2048)) * 8,t_y + (1.0f / (2048)) * 8};

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
	pos.x += size.x * (length - 1);
	for(int i = 0;i < length;i++){
		drawChar(pos,size,number % 10 + 0x30);
		number /= 10;
		pos.x -= size.x;
	}
}

void drawString(vec2_t pos,float size,char* str){
	while(*str){
		drawChar(pos,RD_SQUARE(size),*str++);
		pos.x += size * 0.6f;
	}
}
#include <stdio.h>
void drawGUI(dynamic_array_t gui_array){
	for(int i = 0;i < gui_array.size;i++){
		gui_t* element = dynamicArrayGet(gui_array,i);
		switch(element->type){
		case GUI_ELEMENT_PROGRESS:
			guiProgressBar(element->offset,element->size,*element->progress);
			break;
		case GUI_ELEMENT_ITEM:
			guiInventory(element->item,element->offset,(vec3_t){1.0f,1.0f,1.0f});
			break;
		case GUI_ELEMENT_BUTTON:;
			button_t button = element->button;
			if(button.f_void == changeCraftRecipy || button.f_void == playerCraft){
				vec2_t offset = vec2addvec2R(element->offset,(vec2_t){0.05f,0.03f});
				craftrecipy_t recipy = craft_recipy[button.value];
				drawString(vec2addvec2R(offset,(vec2_t){0.07f,0.25f}),0.02f,recipy.name);
				item_t cost_1 = {.type = recipy.cost[0].type,.ammount = recipy.cost[0].ammount * craft_ammount};
				item_t cost_2 = {.type = recipy.cost[1].type,.ammount = recipy.cost[1].ammount * craft_ammount};
				item_t result = {.type = recipy.result,.ammount = craft_ammount};
				guiInventoryContent(&cost_2,offset,RD_SQUARE(0.1f));
				guiInventoryContent(&cost_1,vec2addvec2R(offset,(vec2_t){0.0f,0.15f}),RD_SQUARE(0.1f));	
				guiInventoryContent(&result,vec2addvec2R(offset,(vec2_t){0.1f,0.075f}),RD_SQUARE(0.1f));
			}
			if(button.f_void == changeCraftRecipy){
				node_t* node = dynamicArrayGet(node_root,block_menu_block);
				block_t* block = dynamicArrayGet(block_array,node->index);
				if(block->recipy_select == button.value)
					guiFrame(element->offset,button.size,(vec3_t){0.0f,0.0f,0.0f},0.01f);
			}
			if(button.f_void == increaseCraftAmmount){
				drawChar((vec2_t){-0.17f,0.515f},RD_SQUARE(0.03f),'+');
				guiFrame(element->offset,button.size,(vec3_t){0.0f,0.0f,0.0f},0.01f);
				guiRectangle(element->offset,button.size,(vec3_t){0.2f,0.7f,0.2f});
			}
			if(button.f_void == decreaseCraftAmmount){
				drawChar((vec2_t){-0.23f,0.515f},RD_SQUARE(0.03f),'-');
				guiFrame(element->offset,button.size,(vec3_t){0.0f,0.0f,0.0f},0.01f);
				guiRectangle(element->offset,button.size,(vec3_t){0.7f,0.2f,0.2f});
			}
			break;
		}
	}
}

void guiProgressBar(vec2_t pos,vec2_t size,float progress){
	guiFrame(pos,size,(vec3_t){0.2f,0.2f,0.2f},0.005f);
	guiRectangle(pos,(vec2_t){progress * size.x,size.y},(vec3_t){0.2f,1.0f,0.2f});
	guiRectangle((vec2_t){pos.x + progress * size.x,pos.y},(vec2_t){(1.0f - progress) * size.x,size.y},VEC3_ZERO);
}

void guiInventoryContent(item_t* slot,vec2_t pos,vec2_t size){
	if(slot->type == ITEM_VOID)
		return;
	uint32_t ammount = slot->ammount;
	char behind = 0;
	if(ammount > 99999999){
		behind = 'M';
		ammount /= 1000000;
	}
	else if(ammount > 9999){
		behind = 'K';
		ammount /= 1000;
	}
	if(behind){
		if(ammount < 100)
			drawChar(vec2addvec2R(pos,(vec2_t){0.03f,0.085f}),vec2mulR(size,0.3f),behind);
		else if (ammount < 1000)
			drawChar(vec2addvec2R(pos,(vec2_t){0.048f,0.085f}),vec2mulR(size,0.3f),behind);
		else
			drawChar(vec2addvec2R(pos,(vec2_t){0.066f,0.085f}),vec2mulR(size,0.3f),behind);
	}
	drawNumber(vec2addvec2R(pos,(vec2_t){-0.005f,0.085f}),vec2mulR(size,0.3f),ammount);
	material_t material = material_array[slot->type];
	vec2_t texture_coord[4];
	texture_coord[0] = material.texture_pos;
	texture_coord[1] = (vec2_t){material.texture_pos.x,material.texture_pos.y + material.texture_size.y};
	texture_coord[2] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y};
	texture_coord[3] = (vec2_t){material.texture_pos.x + material.texture_size.x,material.texture_pos.y + material.texture_size.y};

	triangles[triangle_count++] = (triangle_t){pos.x,pos.y,1.0f,texture_coord[0],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x,pos.y + size.y,1.0f,texture_coord[1],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x,pos.y,1.0f,texture_coord[0],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y,1.0f,texture_coord[2],material.luminance,1.0f};
	triangles[triangle_count++] = (triangle_t){pos.x + size.x,pos.y + size.y,1.0f,texture_coord[3],material.luminance,1.0f};
}

void guiInventory(item_t* item,vec2_t pos,vec3_t frame_color){
	guiInventoryContent(item,pos,RD_SQUARE(0.1f));
	guiFrame(vec2subvec2R(pos,(vec2_t){0.02f,0.025f}),RD_SQUARE(0.15f),VEC3_ZERO,0.01f);
	guiFrame(vec2subvec2R(pos,(vec2_t){0.022f,0.03f}),RD_SQUARE(0.15f),frame_color,0.02f);
};