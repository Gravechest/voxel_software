#pragma once

#include "source.h"
#include "inventory.h"
#include "dynamic_array.h"

enum{
	GUI_ELEMENT_BUTTON,
	GUI_ELEMENT_GUI,
	GUI_ELEMENT_ITEM
};

enum{
	GUI_CRAFT_AMMOUNT,
	GUI_CRAFT_TEMPLATE,
	GUI_CRAFT_MENU,
	GUI_CRAFT_ELEKTRIC1,
	GUI_CRAFT_BASIC,
	GUI_FURNACE
};

typedef struct{
	vec2_t size;
	union{
		void (*f_int)(int);
		void (*f_void)(void);
	};
	int value;
}button_t;

typedef struct{
	int type;
	vec2_t offset;
	union{
		button_t button;
		int gui_index;
		item_t* item;
	};
}gui_t;

extern dynamic_array_t gui_array[];

void guiInventoryContent(item_t* slot,vec2_t pos,vec2_t size);
void guiRectangle(vec2_t pos,vec2_t size,vec3_t color);
void guiFrame(vec2_t pos,vec2_t size,vec3_t color,float thickness);
void drawChar(vec2_t pos,vec2_t size,char c);
void drawString(vec2_t pos,float size,char* str);
void drawNumber(vec2_t pos,vec2_t size,uint32_t number);
void drawGUI(dynamic_array_t gui_array);
void initGUI();