#pragma once

#define C_STD 0

void* tMalloc(unsigned long long size);
void* tMallocZero(unsigned long long size);
void* tReAlloc(void* ptr,unsigned long long size);
void tFree(void* ptr);
