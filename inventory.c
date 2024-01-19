#include "inventory.h"
#include "source.h"

item_t inventory_slot[] = {
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1},
	{.type = -1}
};

void changeInventorySelect(int select){
	if(inventory_slot[select].type != ITEM_VOID){
		if(material_array[inventory_slot[select].type].fixed_size)
			edit_depth = material_array[inventory_slot[select].type].fixed_size;
	}
	inventory_select = select;
}

void itemChange(item_t* item,int ammount){
	if(-ammount >= item->ammount){
		item->ammount = 0;
		item->type = ITEM_VOID;
		return;
	}
	item->ammount += ammount;
}

void inventoryAdd(item_t item){
	for(int i = 0;i < 9;i++){
		if(inventory_slot[i].type == item.type){
			inventory_slot[i].type = item.type;
			inventory_slot[i].ammount += item.ammount;
			if(inventory_select == i && material_array[item.type].fixed_size)
				edit_depth = material_array[item.type].fixed_size;
			return;
		}
	}
	for(int i = 0;i < 9;i++){
		if(inventory_slot[i].type == ITEM_VOID){
			inventory_slot[i].type = item.type;
			inventory_slot[i].ammount += item.ammount;
			if(inventory_select == i && material_array[item.type].fixed_size)
				edit_depth = material_array[item.type].fixed_size;
			break;
		}
	}
}