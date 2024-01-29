#pragma once

#include <Windows.h>

#include "source.h"

#define TEXTURE_ATLAS_SIZE 2048

#define GL_ARRAY_BUFFER 0x8892
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31

#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6

typedef struct{
	vec3_t pos;
	vec2_t texture_pos;
	vec3_t lighting;
	float distance;
	vec3_t fog_color;
}triangle_t;

extern uint32_t triangle_count;
extern triangle_t* triangles;
extern uint32_t shaderprogram;
extern uint32_t VBO_plain_texture;
extern uint32_t shader_program_plain_texture;

void drawBlockOutline(triangle_t* triangles);
void initGL(void* context);

void (*glBufferData)(uint32_t target,uint32_t size,const void *data,uint32_t usage);
void (*glCreateBuffers)(uint32_t n,uint32_t *buffers);
void (*glBindBuffer)(uint32_t target,uint32_t buffer);
void (*glEnableVertexAttribArray)(uint32_t index);
void (*glVertexAttribPointer)(uint32_t index,int size,uint32_t type,unsigned char normalized,uint32_t stride,const void *pointer);
void (*glShaderSource)(uint32_t shader,int count,const char **string,int *length);
void (*glCompileShader)(uint32_t shader);
void (*glAttachShader)(uint32_t program,uint32_t shader);
void (*glLinkProgram)(uint32_t program);
void (*glUseProgram)(uint32_t program);
void (*glGenerateMipmap)(uint32_t target);
void (*glActiveTexture)(uint32_t texture);
void (*glUniform4f)(int loc,float v1,float v2,float v3,float v4);