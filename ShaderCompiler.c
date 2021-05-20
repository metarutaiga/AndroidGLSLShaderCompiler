#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

int main(int argc, char** argv)
{
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (display == EGL_NO_DISPLAY)
  {
    printf("%s is failed\n", "eglGetDisplay");
    return 0;
  }

  EGLint majorVersion = 0;
  EGLint minorVersion = 0;
  if (eglInitialize(display, &majorVersion, &minorVersion) == EGL_FALSE)
  {
    printf("%s is failed\n", "eglInitialize");
    return 0;
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
    return 0;
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
    return 0;
  }
 
  eglMakeCurrent(display, surface, surface, context);
  printf("%-12s : %s\n", "GL_VENDOR", glGetString(GL_VENDOR));
  printf("%-12s : %s\n", "GL_RENDERER", glGetString(GL_RENDERER));
  printf("%-12s : %s\n", "GL_VERSION", glGetString(GL_VERSION));

  int shaderType = 0;
  int shaderSize = 0;
  GLchar* shaderCode = NULL;
  if (argc >= 2 && (strstr(argv[1], ".vert") || strstr(argv[1], ".frag")))
  {
    FILE* file = fopen(argv[1], "rb");
    if (file == NULL)
    {
      printf("%s is failed", argv[1]);
      return 0;
    }
    fseek(file, 0, SEEK_END);
    shaderSize = ftell(file);
    shaderCode = calloc(shaderSize + 1, 1);
    fseek(file, 0, SEEK_SET);
    fread(shaderCode, shaderSize, 1, file);
    fclose(file);

    if (strstr(argv[1], ".vert"))
      shaderType = GL_VERTEX_SHADER;
    if (strstr(argv[1], ".frag"))
      shaderType = GL_FRAGMENT_SHADER;
  }

  if (shaderType != 0 && shaderCode != NULL)
  {
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
	PFNGLGETPROGRAMBINARYOESPROC glGetProgramBinaryOES = NULL;
	if (glGetProgramBinaryOES == NULL)
	  glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinary");
	if (glGetProgramBinaryOES == NULL)
	  glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOESPROC)eglGetProcAddress("glGetProgramBinaryOES");
	if (glGetProgramBinaryOES == NULL)
	{
	  printf("%s is failed\n", "eglGetProcAddress");
	  return 0;
	}

	GLint length = 0;
	glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH_OES, &length); 
	printf("%s : %d\n", "glGetProgramiv", length);

	GLsizei size = 0;
	GLenum binaryFormat = 0;
	void* binary = malloc(length);
	glGetProgramBinaryOES(program, length, &size, &binaryFormat, binary);
	printf("%s : %d\n", "glGetProgramBinaryOES", size);
	printf("%s : %d\n", "glGetProgramBinaryOES", binaryFormat);

	if (argc >= 3)
	{
	  FILE* file = fopen(argv[2], "wb");
	  if (file == NULL)
	  {
	    printf("%s is failed\n", argv[2]);
	    return 0;
	  }
	  fwrite(binary, 1, size, file);
	  fclose(file);
	}

	free(binary);
      }

      if (vertex)
	glDeleteShader(vertex);
      if (fragment)
	glDeleteShader(fragment);
      glDeleteProgram(program);
    }

    glDeleteShader(shader);
    free(shaderCode);
  }
  else
  {
    printf("Usage : %s input[.vert][.frag] [output]\n", argv[0]);
    return 0;
  }
  
  return 0;
}
