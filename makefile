CC:= gcc
OPTS:= -fsanitize=address -fsanitize=undefined -g -I./include/
LINK:= -lm -lasound




OBJS_CORE:= \
	src/array.o \
	src/gate.o \
	src/gensyn.o \
	src/string.o \
	src/table.o \
	src/extern/srgs.o \
	src/extern/duktape.o \
	src/system/system_linux.o


%.o : %.c
	$(CC) $(OPTS) -c -o $@ $<

all: $(OBJS_CORE)
	$(CC) $(OBJS_CORE) ./build/cli/cli.c -o ./build/cli/gensyn-cli $(LINK) $(OPTS)
	$(CC) $(OBJS_CORE) ./build/midi-test/midi-test.c -o ./build/midi-test/midi-test $(LINK) $(OPTS)

clean:
	rm `find ./ -iname '*.o'`
