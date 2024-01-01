#include <stdbool.h>

#include "opengl.h"

#include <gl/GL.h>

uint32_t triangle_count;
triangle_t* triangles;

PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR), 1,
PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,PFD_TYPE_RGBA,
24,0, 0, 0, 0, 0, 0,0,0,0,
0,0,0,0,32,0,0,PFD_MAIN_PLANE,
0,0,0,0	};

int (*glGetUniformLocation)(unsigned int program,char* name);
unsigned int (*glCreateProgram)();
unsigned int (*glCreateShader)(unsigned int shader);
unsigned int (*wglSwapIntervalEXT)(unsigned int status);

unsigned int VBO;
unsigned int texture2;
unsigned int VBO_plain_texture;
unsigned int shaderprogram;
unsigned int shader_program_ui;
unsigned int shader_program_plain_texture;

char *plain_texture_vert_source = ""
"#version 460 core\n"

"layout (location = 0) in vec2 verticles;"
"layout (location = 1) in vec2 textcoords;"

"out vec2 TextCoords;"

"void main(){"
	"TextCoords = textcoords;"
	"gl_Position = vec4(verticles,0.0,1.0);"
"}";

char *plain_texture_frag_source = ""
"#version 460 core\n"

"out vec4 FragColor;"

"in vec2 TextCoords;"

"uniform sampler2D ourTexture;"

"void main(){"
	"FragColor = texture(ourTexture,crd);"
"}";

char *VERTsource = ""
"#version 460 core\n"

"layout (location = 0) in vec3 verticles;"
"layout (location = 1) in vec2 textcoords;"
"layout (location = 2) in vec3 lighting;"
"layout (location = 3) in float distance;"
"layout (location = 4) in vec3 fog_color;"

"out vec3 Lighting;"
"out vec2 TextCoords;"
"out float Distance;"
"out vec3 Fog_color;"

"void main(){"
	"TextCoords = textcoords;"
	"Lighting = lighting;"
	"Distance = distance;"
	"Fog_color = fog_color;"
	"gl_Position = vec4(verticles.xy,0.0,verticles.z);"
"}";
char *FRAGsource = ""
"#version 460 core\n"

"out vec4 FragColor;"

"in vec2 TextCoords;"
"in vec3 Lighting;"
"in vec3 Fog_color;"
"in float Distance;"

"uniform sampler2D ourTexture;"

"void main(){"
	"vec2 crd = TextCoords;"
	"FragColor = vec4(Lighting,1.0f);"
	"vec4 texture_color = texture(ourTexture,crd);"
	"if(texture_color.a > 0.0){"
	"discard;"
	"}"
	"FragColor *= texture_color;"
	"FragColor.rgb = mix(FragColor.rgb,Fog_color,1.0f - Distance);"
"}";

char *VERTsourceUI = ""
"#version 460 core\n"

"layout (location = 0) in vec2 verticles;"
"layout (location = 1) in vec2 textcoords;"
"layout (location = 2) in vec3 lighting;"

"out vec3 Lighting;"

"void main(){"
	"Lighting = lighting;"
	"gl_Position = vec4(verticles,0.0,1.0);"
"}";

char *FRAGsourceUI = ""
"#version 460 core\n"

"out vec4 FragColor;"

"in vec3 Lighting;"

"void main(){"
	"FragColor = vec4(Lighting,1.0f);"
"}";

#define RD_VSYNC true

void drawBlockOutline(triangle_t* triangles){
	glBufferData(GL_ARRAY_BUFFER,sizeof(triangle_t) * 8,triangles,GL_DYNAMIC_DRAW);
	glDrawArrays(GL_LINES,0,8);
}
#include <stdio.h>

void initGL(HDC context){
	SetPixelFormat(context, ChoosePixelFormat(context, &pfd), &pfd);
	wglMakeCurrent(context, wglCreateContext(context));

	glBufferData           	  = wglGetProcAddress("glBufferData");
	glCreateBuffers           = wglGetProcAddress("glCreateBuffers");
	glBindBuffer              = wglGetProcAddress("glBindBuffer");
	glEnableVertexAttribArray = wglGetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer     = wglGetProcAddress("glVertexAttribPointer");
	glGenerateMipmap          = wglGetProcAddress("glGenerateMipmap");
	glActiveTexture           = wglGetProcAddress("glActiveTexture");
					
	glUniform4f           = wglGetProcAddress("glUniform4f");
	glGetUniformLocation  = wglGetProcAddress("glGetUniformLocation");
	glCreateProgram		  = wglGetProcAddress("glCreateProgram");
	glCreateShader		  = wglGetProcAddress("glCreateShader");
	glShaderSource		  = wglGetProcAddress("glShaderSource");
	glCompileShader		  = wglGetProcAddress("glCompileShader");
	glAttachShader		  = wglGetProcAddress("glAttachShader");
	glLinkProgram		  = wglGetProcAddress("glLinkProgram");
	glUseProgram		  = wglGetProcAddress("glUseProgram");

	wglSwapIntervalEXT = wglGetProcAddress("wglSwapIntervalEXT");

	wglSwapIntervalEXT(RD_VSYNC);

	glCreateBuffers(1,&VBO);
	glBindBuffer(GL_ARRAY_BUFFER,VBO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,0,12 * sizeof(float),(void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1,2,GL_FLOAT,0,12 * sizeof(float),(void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2,3,GL_FLOAT,0,12 * sizeof(float),(void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3,1,GL_FLOAT,0,12 * sizeof(float),(void*)(8 * sizeof(float)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4,3,GL_FLOAT,0,12 * sizeof(float),(void*)(9 * sizeof(float)));

	context_is_gl = true;
	
	
	shader_program_ui = glCreateProgram();
	shaderprogram   = glCreateProgram();
	shader_program_plain_texture = glCreateProgram();

	unsigned int vertexshader    = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentshader  = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexshader,1,(const char**)&VERTsource,0);
	glShaderSource(fragmentshader,1,(const char**)&FRAGsource,0);

	glCompileShader(vertexshader);
	glCompileShader(fragmentshader);
	glAttachShader(shaderprogram,vertexshader);
	glAttachShader(shaderprogram,fragmentshader);
	glLinkProgram(shaderprogram);
	glUseProgram(shaderprogram);

	unsigned int vertexshader_ui   = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentshader_ui = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexshader_ui,1,(const char**)&VERTsourceUI,0);
	glShaderSource(fragmentshader_ui,1,(const char**)&FRAGsourceUI,0);
				
	glCompileShader(vertexshader_ui);
	glCompileShader(fragmentshader_ui);
		
	glAttachShader(shader_program_ui,vertexshader_ui);
	glAttachShader(shader_program_ui,fragmentshader_ui);
	
	glLinkProgram(shader_program_ui);

	unsigned int vertexshader_plain_texture   = glCreateShader(GL_VERTEX_SHADER);
	unsigned int fragmentshader_plain_texture = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexshader_plain_texture,1,(const char**)&plain_texture_vert_source,0);
	glShaderSource(fragmentshader_plain_texture,1,(const char**)&plain_texture_frag_source,0);
				
	glCompileShader(vertexshader_plain_texture);
	glCompileShader(fragmentshader_plain_texture);
		
	glAttachShader(shader_program_plain_texture,vertexshader_plain_texture);
	glAttachShader(shader_program_plain_texture,fragmentshader_plain_texture);
	
	glLinkProgram(shader_program_plain_texture);

	glEnable(GL_DEPTH_TEST);
	
	glGenTextures(1, &texture2);  
	glBindTexture(GL_TEXTURE_2D, texture2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				  
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,TEXTURE_ATLAS_SIZE,TEXTURE_ATLAS_SIZE,0,GL_RGBA,GL_UNSIGNED_BYTE,texture_atlas);
	glGenerateMipmap(GL_TEXTURE_2D);
}