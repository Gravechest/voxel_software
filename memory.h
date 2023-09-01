#pragma once

#include <Windows.h>

#define MALLOC(AMMOUNT) HeapAlloc(GetProcessHeap(),0,AMMOUNT)
#define MALLOC_ZERO(AMMOUNT) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,AMMOUNT)
#define MFREE(POINTER) HeapFree(GetProcessHeap(),0,POINTER)