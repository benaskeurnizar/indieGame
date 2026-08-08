#include <stdlib.h>
#include <stdio.h>


char* LoadShaderText(const char* address,int* size){
  int SIZE = 1024;

  char* result = (char*)malloc(SIZE);
  FILE* ptr;
  char chr;

  // Opening file in reading mode
  ptr = fopen(address, "r");
  if(ptr==NULL){
    printf("file can't be opened : %s \n",address);
    return NULL;
  }
  printf("file opened succsessfully! \n");
  *size = 0;
  chr = fgetc(ptr);
  while(chr != EOF){
    if(*size < SIZE){
      result[(*size)++] = chr;
    }else{
      SIZE *=2;
      result = (char*)realloc(result,SIZE);
      result[(*size)++] = chr;
    }
    chr = fgetc(ptr);
  }
  result[*size] = '\0';
  fclose(ptr);
  return result;
}

void LinkVShader(unsigned int*VertexShader,const char* VertexCodeAdr){
  int size = 0;
  char* VertexCode = LoadShaderText(VertexCodeAdr,&size);
  int success;
  char infolog[512];
  // vertex shader
  *VertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(*VertexShader,1,&VertexCode,NULL);
  glCompileShader(*VertexShader);
  glGetShaderiv(*VertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(*VertexShader, 512, NULL, infolog);
    printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n");
  }
}

void LinkFrShader(unsigned int*FragmentShader,const char* FragmentCodeAdr){
  int size = 0;
  char* FragmentCode = LoadShaderText(FragmentCodeAdr,&size);
  int success;
  char infolog[512];
  // fragment shader
  *FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(*FragmentShader,1,&FragmentCode,NULL);
  glCompileShader(*FragmentShader);
  glGetShaderiv(*FragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(*FragmentShader, 512, NULL, infolog);
    //glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    printf("FRAGMENT SHADER COMPILATION FAILED:\n%s\n", infolog);
    //printf("ERROR::SHADER::Fragment::COMPILATION_FAILED\n");
  }
}

void LinkPShader(unsigned int* VertexShader,unsigned int* FragmentShader,unsigned int* ProgramShader){
  int success;
  char infolog[512];
  // Program shader
  *ProgramShader = glCreateProgram();
  glAttachShader(*ProgramShader,*VertexShader );
  glAttachShader(*ProgramShader,*FragmentShader );
  glLinkProgram(*ProgramShader);
  glGetProgramiv(*ProgramShader,GL_LINK_STATUS,&success);
  if (!success) {
    glGetShaderInfoLog(*ProgramShader, 512, NULL, infolog);
    printf("ERROR::SHADER::Program::COMPILATION_FAILED\n");
  }
	
}
