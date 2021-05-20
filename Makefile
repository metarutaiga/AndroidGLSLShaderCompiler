CC=clang
CFLAGS=-O3 -g
LD=clang
LDFLAGS=-lEGL -lGLESv2 /system/lib64/liblzma.so

SRCS=ShaderCompiler.c
OBJS=$(SRCS:.c=.o)
DEPS=$(OBJS:.o=.d)
ShaderCompiler: $(OBJS)

TARGETS=ShaderCompiler

.DEFAULT_GOAL=all
.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)
	$(RM) $(DEPS)
