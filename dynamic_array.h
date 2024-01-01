#pragma once

#include <stdint.h>

typedef struct{
	uint32_t free_ptr;
	uint32_t* free;
	void* data;
	int element_size;
	uint64_t size;
}dynamic_array_t;

#define DYNAMIC_ARRAY(NAME,TYPE) dynamic_array_t NAME = {.element_size = sizeof(TYPE)}

void dynamicArrayRemove(dynamic_array_t* dynamic_array,uint64_t element);
void dynamicArrayAdd(dynamic_array_t* dynamic_array,void* data);
void* dynamicArrayGet(dynamic_array_t dynamic_array,uint64_t element);
void dynamicArrayReset(dynamic_array_t* dynamic_array);
uint32_t dynamicArrayTop(dynamic_array_t dynamic_array);