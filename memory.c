#if C_STD
#include <stdlib.h>
#else
#include <Windows.h>
#endif

#include "memory.h"

void* tMalloc(unsigned long long size){
#if C_STD
	return malloc(size);
#else
	return HeapAlloc(GetProcessHeap(),0,size);
#endif
}

void* tMallocZero(unsigned long long size){
#if C_STD
	return calloc(1,size);
#else
	return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);
#endif
}

void tFree(void* ptr){
#if C_STD
	free(ptr);
#else
	HeapFree(GetProcessHeap(),0,ptr);
#endif
}

void* tReAlloc(void* ptr,unsigned long long size){
#if C_STD
	return realloc(ptr,size);
#else
	return HeapReAlloc(GetProcessHeap(),0,ptr,size);
#endif
}