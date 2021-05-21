#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

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

struct ClientData
{
  int socket;
  int shaderType;
  int shaderSize;
  int shaderRead;
  char* shaderCode;
  int binarySize;
  int binaryWrite;
  char* binaryCode;
};

bool serve(const char* ip, unsigned short port)
{
  int epoll = epoll_create(1024);
  if (epoll < 0)
  {
    printf("%s is failed\n", "epoll_create");
    return false;
  }

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  inet_pton(AF_INET, ip, &sa.sin_addr);
  int server = socket(sa.sin_family, SOCK_STREAM, IPPROTO_TCP);
  if (server < 0)
  {
    printf("%s is failed\n", "socket");
    return false;
  }

  if (bind(server, (struct sockaddr*)&sa, sizeof(sa)) < 0)
  {
    printf("%s is failed\n", "bind");
    return false;
  }

  if (listen(server, 512) < 0)
  {
    printf("%s is failed\n", "listen");
    return false;
  }
  
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = NULL;
  epoll_ctl(epoll, EPOLL_CTL_ADD, server, &ev);

  struct epoll_event events[1024];
  for (;;)
  {
    int count = epoll_wait(epoll, events, 1024, -1);
    if (count <= 0)
      break;
    printf("%s : %d\n", "epoll_wait", count);

    for (int i = 0; i < count; ++i)
    {
      struct ClientData* data = events[i].data.ptr;
      if (data == NULL)
      {
	struct sockaddr_storage addr;
        socklen_t addrlen = sizeof(addr);
	int client = accept(server, (struct sockaddr*)&addr, &addrlen);
	if (client < 0)
	  continue;
	printf("%s : %d\n", "accept", client);

	struct ClientData* data = calloc(sizeof(struct ClientData), 1);
	if (data == NULL)
	  continue;
	data->socket = client;
	
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLET;
	ev.data.ptr = data;
	epoll_ctl(epoll, EPOLL_CTL_ADD, client, &ev);
	continue;
      }
      if (events[i].events & EPOLLIN)
      {
	if (data->shaderSize == 0)
	{
	  int header[3];
	  if (recv(data->socket, header, sizeof(header), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0)
	    continue;
	  if (header[0] != 0xDEADBEEF || header[2] > 1048576)
	  {
	    events[i].events |= EPOLLERR;
	  }
	  else
	  {
	    data->shaderType = header[1];
	    data->shaderSize = header[2];
	    data->shaderCode = calloc(data->shaderSize, 1);
	  }
	}
	if (data->shaderRead != data->shaderSize)
	{
	  int read = recv(data->socket, data->shaderCode + data->shaderRead, data->shaderSize - data->shaderRead, MSG_DONTWAIT | MSG_NOSIGNAL);
	  printf("%s : %d\n", "recv", read);
	  if (read <= 0)
	    continue;
	  data->shaderRead += read;
	  if (data->shaderRead != data->shaderSize)
	    continue;
	  
	  void* binaryCode = NULL;
	  int binarySize = 0;
	  shaderCompile(100, data->shaderType, data->shaderCode, data->shaderSize, &binaryCode, &binarySize);
	  if (binaryCode == NULL)
	  {
	    events[i].events |= EPOLLERR;
	  }
	  else
	  {
	    data->binarySize = binarySize;
	    data->binaryCode = binaryCode;
	    if (send(data->socket, &binarySize, sizeof(binarySize), MSG_DONTWAIT | MSG_NOSIGNAL) <= 0)
	    {
	      events[i].events |= EPOLLERR;
	    }
	    else
	    {
	      events[i].events |= EPOLLOUT;
	    }
	  }
	}
      }
      if (events[i].events & EPOLLOUT)
      {
	if (data->binaryCode && data->binaryWrite != data->binarySize)
        {
	  int write = send(data->socket, data->binaryCode + data->binaryWrite, data->binarySize - data->binaryWrite, MSG_DONTWAIT | MSG_NOSIGNAL);
	  printf("%s : %d\n", "send", write);
	  if (write <= 0)
	    continue;
	  data->binaryWrite += write;
	  if (data->binaryWrite != data->binarySize)
	    continue;
	  events[i].events |= EPOLLERR;
	}
      }      
      if (events[i].events & EPOLLERR)
      {
	close(data->socket);
	printf("%s : %d\n", "close", data->socket);

	epoll_ctl(epoll, EPOLL_CTL_DEL, data->socket, NULL);
      }
    }
  }
  
  return true;
}

int main(int argc, char** argv)
{
  if (eglCreate() == false)
  {
    return 0;
  }

  if (argc >= 4 && strcmp(argv[1], "-s") == 0)
  {
    serve(argv[2], atoi(argv[3]));
  }
  else if (argc >= 2 && (strstr(argv[1], ".vert") || strstr(argv[1], ".frag")))
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
