SOURCE_FILES:= $(shell find  -name "*.c")
CC:= gcc
CF:= -g -O0 -Wall -I"src" #-fsanitize=address
CL:= -lncurses -lavformat -lavcodec -lavutil -lswscale -lswresample -ltinfo -lSDL2 -lm -pthread
BIN:= a.out

all: $(SOURCE_FILES:.c=.o) $(BIN)

rebuild: clean all

clean:
	find . -name "*.o" -delete
	rm -rf $(BIN)

%.o: %.c
	$(CC) $(CF) $< -c -o $@

$(BIN):
	$(CC) $(CF) $(CL) $(shell find -name "*.o") -o $@ 
