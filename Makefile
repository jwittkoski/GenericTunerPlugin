CC=gcc
CFLAGS = -m64 -fPIC -D_FILE_OFFSET_BITS=64

all: GenericTunerPlugin.so TestTuner

TestTuner: GenericTunerPlugin.o TestTuner.o
	$(CC) -o TestTuner GenericTunerPlugin.o TestTuner.o

TestTuner.o: TestTuner.c
	$(CC) $(CFLAGS) -Wall -O2 -c TestTuner.c

GenericTunerPlugin.so: GenericTunerPlugin.o
	$(CC) $(CFLAGS) -o GenericTunerPlugin.so -shared GenericTunerPlugin.o

GenericTunerPlugin.o: GenericTunerPlugin.c
	$(CC) $(CFLAGS) -Wall -O2 -c GenericTunerPlugin.c

clean:
	rm -f *.o *.so *.c~ *.h~

realclean: clean
	rm -f TestTuner
