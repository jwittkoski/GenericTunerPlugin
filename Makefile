RELEASE_VERSION ?= $(shell git describe --tags)

CC = gcc
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
	rm -rf release/

release: all
	mkdir -p release/irtunerplugins/gentuner/samples
	mkdir -p release/irtunerplugins/gentuner/doc
	cp GenericTunerPlugin.so release/irtunerplugins/
	cp gentuner.CUSTOM release/irtunerplugins/gentuner/samples/
	cp gentuner.HR44 release/irtunerplugins/gentuner/samples/
	cp gentuner.IPSTB release/irtunerplugins/gentuner/samples/
	cp gentuner.LIRC release/irtunerplugins/gentuner/samples/
	cp gentuner.PANELCTL release/irtunerplugins/gentuner/samples/
	cp README.md release/irtunerplugins/gentuner/doc/
	cp README.txt release/irtunerplugins/gentuner/doc/
	cd release/ && tar cvzf gentuner-$(RELEASE_VERSION).linux-amd64.zip ./irtunerplugins/
