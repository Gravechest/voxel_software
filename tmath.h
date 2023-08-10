#pragma once

#define M_PI 3.14159265359f

#define TRND1 (tRnd() - 1.0f)

float tMaxf(float p,float p2);
float tMinf(float p,float p2);
float tAbsf(float p);
float tFract(float x);
float tMix(float v1,float v2,float inpol);
float tFloorf(float p);
float tCeilingf(float p);
float tRnd();

int tAbs(int x);
int tMax(int p,int p2);
int tMin(int p,int p2);
int tHash(int x);
int tFloori(float p);