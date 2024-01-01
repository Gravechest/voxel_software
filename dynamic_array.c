#include <stdbool.h>

#include "dynamic_array.h"
#include "memory.h"

#include <stdio.h>
#include <Windows.h>

void dynamicArrayAdd(dynamic_array_t* dynamic_array,void* data){
	if(dynamic_array->free_ptr){
		dynamic_array->free_ptr--;
		uint32_t index = dynamic_array->free[dynamic_array->free_ptr];
		for(int i = 0;i < dynamic_array->element_size;i++)
			((char*)dynamic_array->data)[index * dynamic_array->element_size + i] = ((char*)data)[i];
		return;
	}
	bool power_of_2 = !(dynamic_array->size & dynamic_array->size - 1);
	if(power_of_2){
		uint64_t size = dynamic_array->element_size * (dynamic_array->size * 2 + 2);
		uint64_t free_size = (dynamic_array->size * 2 + 2) * sizeof(uint32_t);
		dynamic_array->data = dynamic_array->data ? tReAlloc(dynamic_array->data,size) : tMalloc(size);
		dynamic_array->free = dynamic_array->free ? tReAlloc(dynamic_array->free,free_size) : tMalloc(free_size);
	}
	for(int i = 0;i < dynamic_array->element_size;i++)
		((char*)dynamic_array->data)[dynamic_array->size * dynamic_array->element_size + i] = ((char*)data)[i];
	dynamic_array->size++;
}
	
void dynamicArrayRemove(dynamic_array_t* dynamic_array,uint64_t element){
	dynamic_array->free[dynamic_array->free_ptr++] = element;
}

void* dynamicArrayGet(dynamic_array_t dynamic_array,uint64_t element){
	return &((char*)dynamic_array.data)[dynamic_array.element_size * element];
}

uint32_t dynamicArrayTop(dynamic_array_t dynamic_array){
	return dynamic_array.free_ptr ? dynamic_array.free[dynamic_array.free_ptr - 1] : dynamic_array.size;
}

void dynamicArrayReset(dynamic_array_t* dynamic_array){
	tFree(dynamic_array->data);
	dynamic_array->data = 0;
	dynamic_array->size = 0;
}