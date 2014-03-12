CC = clang
CXX = clang++ -std=c++11 --stdlib=libc++

PAYLOAD_SRCS = glstatPayload.cc

MACH_OVERRIDE_SRCS = $(shell ls mach_override/mach_override.c mach_override/libudis86/*.c)
MACH_OVERRIDE_OBJS = $(subst .c,.o,$(MACH_OVERRIDE_SRCS))

all: glstat.dylib

glstat.dylib: $(MACH_OVERRIDE_OBJS) $(PAYLOAD_SRCS)
	$(CXX) -arch i386 -arch x86_64 -g -framework OpenGL -framework CoreServices -dynamiclib $(PAYLOAD_SRCS) $(MACH_OVERRIDE_OBJS) -o $@

%.o: %.c
	$(CC) -c -arch i386 -arch x86_64 $< -o $@

clean:
	rm -f $(MACH_OVERRIDE_OBJS)
	rm -f glstat.dylib
	rm -f glstat_fmt
	rm -rf glstat.dylib.dSYM glstat_fmt.dSYM