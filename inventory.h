#pragma once

#include <stdint.h>

#define ITEM_VOID 0xffffffff

typedef struct{
	uint32_t type;
	uint32_t ammount;
}item_t;

extern item_t inventory_slot[];

void inventoryAdd(item_t item);
void itemChange(item_t* item,int ammount);
void changeInventorySelect(int select);