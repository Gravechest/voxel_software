#include "gui.h"
#include "opengl.h"

dynamic_array_t gui_array[] = {
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
		.offset = {0.05f * RD_RATIO,0.07f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.45f},
			.f_int = playerCraft,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {{0.05f * RD_RATIO,-0.38f}},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.45f},
			.f_int = playerCraft,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {-0.4f * RD_RATIO,0.07f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.45f},
			.f_int = playerCraft,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	element = (gui_t){
		.offset = {-0.4f * RD_RATIO,-0.38f},
		.type = GUI_ELEMENT_BUTTON,
		.button = {
			.size = {0.45f * RD_RATIO,0.45f},
			.f_int = playerCraft,
		}
	};
	dynamicArrayAdd(&gui_array[GUI_CRAFT_TEMPLATE],&element);
	copyGUI(&gui_array[GUI_CRAFT_MENU],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[0].button.value = RECIPY_TORCH;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[1].button.value = RECIPY_ELEKTRIC1;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[2].button.value = RECIPY_RESEARCH1;
	((gui_t*)gui_array[GUI_CRAFT_MENU].data)[3].button.value = RECIPY_BASIC;
	copyGUI(&gui_array[GUI_CRAFT_MENU],gui_array[GUI_CRAFT_AMMOUNT]);
	
	copyGUI(&gui_array[GUI_CRAFT_BASIC],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[0].button.value = RECIPY_FURNACE;
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[1].button.value = RECIPY_FURNACE;
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[2].button.value = RECIPY_FURNACE;
	((gui_t*)gui_array[GUI_CRAFT_BASIC].data)[3].button.value = RECIPY_FURNACE;
	copyGUI(&gui_array[GUI_CRAFT_BASIC],gui_array[GUI_CRAFT_AMMOUNT]);

	copyGUI(&gui_array[GUI_CRAFT_ELEKTRIC1],gui_array[GUI_CRAFT_TEMPLATE]);
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[0].button.value = RECIPY_TORCH;
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[1].button.value = RECIPY_ELEKTRIC1;
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[2].button.value = RECIPY_RESEARCH1;
	((gui_t*)gui_array[GUI_CRAFT_ELEKTRIC1].data)[3].button.value = RECIPY_BASIC;
	copyGUI(&gui_array[GUI_CRAFT_ELEKTRIC1],gui_array[GUI_CRAFT_AMMOUNT]);
	element = (gui_t){
		.offset = {-0.05f * RD_RATIO,-0.05f},
		.type = GUI_ELEMENT_ITEM
	};
	dynamicArrayAdd(&gui_array[GUI_FURNACE],&element);
}

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
		drawChar(pos,RD_SQUARE(size),*str++);
		pos.x += size * 0.6f;
	}
}

void drawGUI(dynamic_array_t gui_array){
	for(int i = 0;i < gui_array.size;i++){
		gui_t* element = dynamicArrayGet(gui_array,i);
		switch(element->type){
		case GUI_ELEMENT_ITEM:
			guiInventoryContent(element->item,element->offset,RD_SQUARE(0.1f));
			break;
		case GUI_ELEMENT_BUTTON:
			button_t button = element->button;
			if(button.f_void == playerCraft){
				vec2_t offset = element->offset;
				craftrecipy_t recipy = craft_recipy[button.value];
				drawString(vec2addvec2R(offset,(vec2_t){-0.02f,0.3f}),0.02f,recipy.name);
				item_t cost_1 = {.type = recipy.cost[0].type,.ammount = recipy.cost[0].ammount * craft_ammount};
				item_t cost_2 = {.type = recipy.cost[1].type,.ammount = recipy.cost[1].ammount * craft_ammount};
				item_t result = {.type = recipy.result,.ammount = craft_ammount};
				guiInventoryContent(&cost_2,offset,RD_SQUARE(0.1f));
				guiInventoryContent(&cost_1,vec2addvec2R(offset,(vec2_t){0.0f,0.15f}),RD_SQUARE(0.1f));	
				guiInventoryContent(&result,vec2addvec2R(offset,(vec2_t){0.1f,0.075f}),RD_SQUARE(0.1f));
				break;
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
	drawNumber(vec2addvec2R(pos,(vec2_t){-0.02f,0.085f}),vec2mulR(size,0.3f),ammount);
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