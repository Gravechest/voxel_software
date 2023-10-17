#pragma once

#include <Windows.h>
#include "source.h"

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
	vec2 pos;
	vec2 texture_pos;
	vec3 lighting;
	float distance;
	vec3 fog_color;
}triangles_t;

typedef struct{
	vec2 pos;
	vec3 lighting;
}triangle_ui_t;

extern uint triangle_count;
extern triangles_t* triangles;
extern unsigned int shaderprogram;
extern unsigned int shader_program_ui;
extern unsigned int shader_program_plain_texture;
extern unsigned int VBO_plain_texture;
extern unsigned int shader_program_plain_texture;

void drawBlockOutline(triangles_t* triangles);
void initGL(HDC context);

void (*glBufferData)(unsigned int target,unsigned int size,const void *data,unsigned int usage);
void (*glCreateBuffers)(unsigned int n,unsigned int *buffers);
void (*glBindBuffer)(unsigned int target,unsigned int buffer);
void (*glEnableVertexAttribArray)(unsigned int index);
void (*glVertexAttribPointer)(unsigned int index,int size,unsigned int type,unsigned char normalized,unsigned int stride,const void *pointer);
void (*glShaderSource)(unsigned int shader,int count,const char **string,int *length);
void (*glCompileShader)(unsigned int shader);
void (*glAttachShader)(unsigned int program,unsigned int shader);
void (*glLinkProgram)(unsigned int program);
void (*glUseProgram)(unsigned int program);
void (*glGenerateMipmap)(unsigned int target);
void (*glActiveTexture)(unsigned int texture);
void (*glUniform4f)(int loc,float v1,float v2,float v3,float v4);