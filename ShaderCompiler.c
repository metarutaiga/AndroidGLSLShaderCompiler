#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

bool eglCreate()
{
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (display == EGL_NO_DISPLAY)
  {
    printf("%s is failed\n", "eglGetDisplay");
    return false;
  }

  EGLint majorVersion = 0;
  EGLint minorVersion = 0;
  if (eglInitialize(display, &majorVersion, &minorVersion) == EGL_FALSE)
  {
    printf("%s is failed\n", "eglInitialize");
    return false;
  }
  printf("%-12s : %s\n", "EGL_VENDOR", eglQueryString(display, EGL_VENDOR));
  printf("%-12s : %s\n", "EGL_VERSION", eglQueryString(display, EGL_VERSION));

  EGLint configs = 0;
  EGLint configAttributes[] =
  {
    EGL_SURFACE_TYPE,       EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
    EGL_NONE
  };
  EGLConfig config = NULL;
  if (eglChooseConfig(display, configAttributes, &config, 1, &configs) == EGL_FALSE)
  {
    printf("%s is failed\n", "eglChooseConfig");
    return 0;
  }

  EGLint surfaceAttributes[] =
  {
    EGL_WIDTH,  1,
    EGL_HEIGHT, 1,
    EGL_NONE
  };
  EGLSurface surface = eglCreatePbufferSurface(display, config, surfaceAttributes);
  if (surface == EGL_NO_SURFACE)
  {
    printf("%s is failed\n", "eglCreatePbufferSurface");
    return false;
  }

  EGLint contextAttibutes[] =
  {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };
  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttibutes);
  if (context == EGL_NO_CONTEXT)
  {
    printf("%s is failed\n", "eglCreateContext");
    return false;
  }
 
  eglMakeCurrent(display, surface, surface, context);
  printf("%-12s : %s\n", "GL_VENDOR", glGetString(GL_VENDOR));
  printf("%-12s : %s\n", "GL_RENDERER", glGetString(GL_RENDERER));
  printf("%-12s : %s\n", "GL_VERSION", glGetString(GL_VERSION));

  return true;
}

void eglShutdown()
{
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLSurface surface = eglGetCurrentSurface(EGL_DRAW);
  EGLContext context = eglGetCurrentContext();

  eglMakeCurrent(display, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);
  eglDestroyContext(display, context);
  eglDestroySurface(display, surface);
  eglTerminate(display);
}

bool shaderCompile(int shaderVersion, GLenum shaderType, const char* shaderCode, int shaderSize, void** binaryCode, int* binarySize)
{
  PFNGLGETPROGRAMBINARYOESPROC glGetProgramBinaryOES = NULL;
  if (glGetProgramBinaryOES == NULL)
    glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinary");
  if (glGetProgramBinaryOES == NULL)
    glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinaryOES");
  if (glGetProgramBinaryOES == NULL)
  {
    printf("%s is failed\n", "eglGetProcAddress");
    return false;
  }

  bool succeed = false;
  
  const GLchar* code[] = { shaderCode };
  GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, code, NULL);
  glCompileShader(shader);

  GLint status = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
  {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    char* log = calloc(length + 1, 1);
    if (log)
    {
      glGetShaderInfoLog(shader, length, NULL, log);
      printf("%s : %s\n", "Shader", log);
      free(log);
    }
  }
  else
  {
    GLuint vertex = 0;
    GLuint fragment = 0;
    if (shaderType != GL_VERTEX_SHADER)
    {
      const GLchar* code[] = { "#version 100\nvoid main() { gl_Position = vec4(0); }" };
      vertex = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(vertex, 1, code, NULL);
      glCompileShader(vertex);
    }
    if (shaderType != GL_FRAGMENT_SHADER)
    {
      const GLchar* code[] = { "#version 100\nvoid main() { gl_FragColor = vec4(0); }" };
      fragment = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(fragment, 1, code, NULL);
      glCompileShader(fragment);
    }
      
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    if (vertex)
      glAttachShader(program, vertex);
    if (fragment)
      glAttachShader(program, fragment);
    glLinkProgram(program);
    
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
      GLint length = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
      char* log = calloc(length + 1, 1);
      if (log)
      {
	glGetProgramInfoLog(program, length, NULL, log);
	printf("%s : %s\n", "Program", log);
	free(log);
      }
    }
    else
    {
      GLint length = 0;
      glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH_OES, &length); 
      printf("%s : %d\n", "glGetProgramiv", length);

      GLsizei size = 0;
      GLenum binaryFormat = 0;
      void* binary = malloc(length);
      glGetProgramBinaryOES(program, length, &size, &binaryFormat, binary);
      printf("%s : %d\n", "glGetProgramBinaryOES", binaryFormat);

      if (binaryCode)
	*binaryCode = binary;
      else
	free(binary);

      if (binarySize)
	*binarySize = size;
      
      succeed = true;
    }

    if (vertex)
      glDeleteShader(vertex);
    if (fragment)
      glDeleteShader(fragment);
    glDeleteProgram(program);
  }

  glDeleteShader(shader);
  return succeed;
}

int main(int argc, char** argv)
{
  if (eglCreate() == false)
  {
    return 0;
  }

  if (argc >= 2 && (strstr(argv[1], ".vert") || strstr(argv[1], ".frag")))
  {
    FILE* file = fopen(argv[1], "rb");
    if (file == NULL)
    {
      printf("%s is failed", argv[1]);
      return 0;
    }
    fseek(file, 0, SEEK_END);
    int shaderSize = ftell(file);
    char* shaderCode = calloc(shaderSize + 1, 1);
    fseek(file, 0, SEEK_SET);
    fread(shaderCode, shaderSize, 1, file);
    fclose(file);

    int shaderType = 0;
    if (strstr(argv[1], ".vert"))
      shaderType = GL_VERTEX_SHADER;
    if (strstr(argv[1], ".frag"))
      shaderType = GL_FRAGMENT_SHADER;

    void* binaryCode = NULL;
    int binarySize = 0;
    shaderCompile(100, shaderType, shaderCode, strlen(shaderCode), &binaryCode, &binarySize);
    
    if (argc >= 3 && binaryCode)
    {
      FILE* file = fopen(argv[2], "wb");
      if (file == NULL)
      {
	printf("%s is failed\n", argv[2]);
	return 0;
      }
      fwrite(binaryCode, 1, binarySize, file);
      fclose(file);
    }

    free(binaryCode);
    free(shaderCode);
  }
  else
  {
    printf("Usage : %s input[.vert][.frag] [output]\n", argv[0]);
  }

  eglShutdown();
  return 0;
}
