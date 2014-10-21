CC = gcc
CCFLAGS += -Os -g -std=gnu99 -Wall
CINCS = -I../include

OBJ_HOSTWARE_CONSOLE = user_hostware.o

all:	hostware_console

hostware_console:	$(OBJ_HOSTWARE_CONSOLE)
	$(CC) -o hostware_console $^

user_hostware.o : user_hostware.c
	$(CC) $(CCFLAGS) $(CINCS) $< -c -o $@

clean:
	$(RM) hostware_console user_hostware.o

