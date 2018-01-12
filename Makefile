all: fswc.c
	gcc -o bin/fswc fswc.c -lSDL2main -lSDL2 -mwindows
