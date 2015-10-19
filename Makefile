BIN = ./emu
BUILD_DIR = obj

CFLAGS = $(shell $(SDL2_CONFIG) --cflags) -g -Wall -Iinc
LDFLAGS = $(shell $(SDL2_CONFIG) --libs) -lSDL2main -framework OpenGL -Llib -lfmod

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:$(BUILD_DIR)/%.o=$(BUILD_DIR)/%.d)

SDL2_CONFIG = /usr/local/bin/sdl2-config

all: $(BIN)

$(BIN): $(OBJS)
	gcc -o $@ $(LDFLAGS) $^
	install_name_tool -change @rpath/libfmod.dylib lib/libfmod.dylib $@

-include $(DEPS)

$(BUILD_DIR)/%.o: %.c
	gcc -o $@ -c $(CFLAGS) -MMD $<

clean:
	-rm $(BIN) $(OBJS) $(DEPS)
