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
      GLuint program = glCreateProgram();
      glAttachShader(program, shader);
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
	
      }

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
