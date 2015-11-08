#include "GenericTunerPlugin.h"
#include <stdio.h>
#include <malloc.h>


int main(int argc, char *argv[]) {
	printf("Test Generic Tuner Plugin\n");

	printf("Open Device\n");
	OpenDevice(1);

	printf("Creating Test Remote\n");
	remote *rem = CreateRemote((unsigned char *)"testremote");
	SetRemoteName("testremote");

	if (CanMacroTune()) {
		printf("Test Macro Tune\n");
		MacroTune(1, 256);
	}

	printf("Playing Command\n");
	PlayCommand((unsigned int)1, rem, (unsigned char *)"ENTER", 1);

	printf("Freeing Test Remote\n");
	free(rem);

	printf("Close Device\n");
	CloseDevice(1);
	return 0;
}
