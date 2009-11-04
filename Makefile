CC=gcc
CFLAGS = -c -fPIC -D_FILE_OFFSET_BITS=64

all: GenericTunerPlugin.so

GenericTunerPlugin.so: GenericTunerPlugin.o
	$(CC) -o GenericTunerPlugin.so -shared GenericTunerPlugin.o

GenericTunerPlugin.o: GenericTunerPlugin.c
	$(CC) -Wall -O2 -c GenericTunerPlugin.c

clean:
	rm -f *.o *.so *.c~ *.h~

