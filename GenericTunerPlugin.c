//
// Generic Tuner Plugin for SageTV
// 
// The Generic Tuner Plugin is based on MultiDCTTunerDLL which is
// Copyright (C) 2004 Sean Newman <snewman@sourceforge.net>
// and was licensed under the GPLv2 license.
// 
// This code was written and/or modified by by Jim Paris <jim@jtan.com>,
// Frey Technologies LLC, Sean Newman <snewman@sourceforge.net>,
// Sean Stuckless, and John Wittkoski.
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the
// Free Software Foundation, Inc.
// 59 Temple Place
// Suite 330
// Boston, MA 02111-1307 USA
// 

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "GenericTunerPlugin.h"

#ifndef DEVICE_NAME
#define DEVICE_NAME "Generic Tuner"
#endif

#ifndef LOG_FILE
#define LOG_FILE "/opt/sagetv/server/gentuner.log"
#endif

#ifndef CMD_PATH
#define CMD_PATH "/opt/sagetv/server/gentuner"
#endif

// used by the macro tune to hold the command of the last loaded remote
static char loadedDevName[256];

// Simply non-efficient logger.
void _log(char *pref, char *fmt, ...) {
	va_list ap;
	FILE *f1;
	f1 = fopen(LOG_FILE, "a+");
	if (f1==NULL) return;
	va_start(ap, fmt);
	fprintf(f1, "%s: ", pref);
	vfprintf(f1, fmt, ap);
	fprintf(f1,"\n");
	va_end(ap);
	fflush(f1);
	fclose(f1);
}

#define INFO(s) _log("INFO", s)
#define INFO1(f,a) _log("INFO", f,a)
#define INFO2(f,a,b) _log("INFO", f,a,b)
#define INFO3(f,a,b,c) _log("INFO", f,a,b,c)
#define ERROR(s) _log("ERROR", s)
#define ERROR1(f,a) _log("ERROR", f,a)
#define ERROR2(f,a,b) _log("ERROR", f,a,b)

// simple method to allocate and copy string
char *alloc_string(char *str) {
	if (str==NULL) return NULL;
	char *newStr = malloc(strlen(str)+1); 
	strcpy((char *)newStr, (const char *)str);
	return newStr;
}

command* CreateCommand(char *Name) {
	struct command *Com; //pointer to new command structure

	Com = (struct command*)malloc(sizeof(struct command));//allocate space for a command structure
	if (Com == NULL) {
		return Com;
	}
	Com->name = (unsigned char *)Name; //point to caller allocated name string
	Com->next = NULL;
	Com->pattern = NULL;
	return Com; //return pointer to new command structure
}

int NeedBitrate() {
	return 0;
}

int NeedCarrierFrequency() {
	return 0;
}

const char* DeviceName() {
	return DEVICE_NAME;
}

//
// Open the device
// This is a leftover from the original tuning plugin which
// assumed that a serial port was used for tuning.
// Note: The int value that is returned here is passed as 
//       "devHandle" to several other routines by Sage.
//
int OpenDevice(int ComPort) {
	INFO1("OpenDevice %d", ComPort);
	return 1;
}

void CloseDevice(int devHandle) {
	INFO1("CloseDevice %d", devHandle);
}

unsigned long FindBitRate(int devHandle) {
	return 0;
}

unsigned long FindCarrierFrequency(int devHandle) {
	return 0;
}

void AddRemote(struct remote *Remote, struct remote **head) {
	struct remote *Temp; //Local remote structure

	if (!(*head)) { //if there are no structures in the list
		*head = Remote; //then assign this one to head.
		(*head)->next = NULL;
	} else //otherwise, add to end of list
	{
		Temp = *head;
		while (Temp->next) {
			Temp = Temp->next; //find the last structure in list
		}
		Temp->next = Remote; //assign the next field to the new structure
		Remote->next = NULL; //assign the next field of the new structure to NULL
	}
}

void AddCommand(struct command *Command, struct command **Command_List) {
	struct command *Temp; //temporary command structure pointer

	if (!(*Command_List)) { //if no commands in list, assign Command_List
		(*Command_List) = Command; //to the command structure
		(*Command_List)->next = NULL;
	} else {
		Temp = (*Command_List); //ELSE add to end of list of commands
		while (Temp->next) {
			Temp = Temp->next;
		}
		Temp->next = Command;
		Command->next = NULL;
	}
}

void chomp (char* s) {
    int end = strlen(s) - 1;
    if (end >= 0 && s[end] == '\n')
        s[end] = '\0';
}

int LoadRemoteKeys(struct remote *Remote) {
	char buf[1024];
    char cmd[1024];
	
	INFO1("Loading Keys for Remote %s", Remote->name);
	
	snprintf(cmd, 1024, "%s KEYS %s", CMD_PATH, Remote->name);   
	FILE *f1 = popen(cmd, "r");
	if (f1==NULL) {
		ERROR1("Failed to execute keys command %s", cmd);
		return 0;
	}

	command *head = NULL;
	command *newCmd;
	
	while (fgets(buf, 1024, f1)) {
		if (buf==NULL) break;
		chomp(buf);
		
		newCmd = CreateCommand(alloc_string(buf));
		
		AddCommand(newCmd, &head);
	}
	
	fclose(f1);

    Remote->command = head;
	
	return 0;
}

//
// Return a list of one or more known remotes
// If pszPathName is null, return a list of all available remotes
// If pszPathName is not null, return a list containing only the specified remote
//
// TO DO: According to an email from SageTV staff, SageTV does not ever free the
//        memory allocated by LoadRemotes, so it's up to this plugin to do that
//        at some point. Currently it doesn't so there is a small memory leak
//        each time LoadRemotes is called.
//
remote* LoadRemotes(const char* pszPathName) {
	char buf[1024];
    char cmd[1024];
	
	if (pszPathName==NULL) {
		INFO("LoadRemotes Called");
	} else {
		INFO1("LoadRemotes(%s) Called", pszPathName);
	}

	remote *head = NULL;
	
	snprintf(cmd, 1024, "%s REMOTES", CMD_PATH);   
	INFO1("Loading Remotes using command: %s", cmd);

	FILE *f1 = popen(cmd, "r");
	if (f1==NULL) {
		ERROR1("Failed to execute REMOTES command %s", cmd);
		return NULL;
	}
	
	remote *newRemote;
	while (fgets(buf, 1024, f1)) {
		if (buf==NULL) break;
		
		// eat the newline
		chomp(buf);
	    INFO1("Found Remote: %s", buf);
		
        // each line contains one and only one remote name
		newRemote = CreateRemote((unsigned char *)alloc_string(buf));
		
		// load the remote keys
		LoadRemoteKeys(newRemote);
		
        // if pszPathName is defined, only add that remote to the list
        if ( pszPathName ) {
            if ( 0 == strcmp((const char *)pszPathName,(const char *)newRemote->name) ) {
                // add the new remote to the list
		        AddRemote(newRemote, &head);
	            INFO1("Returning Remote: %s", (const char *)newRemote->name);
                // save name in loadedDevName for MacroTune
                strncpy(loadedDevName, pszPathName, 256);
            }
        } else {
            // add the new remote to the list
		    AddRemote(newRemote, &head);
	        INFO1("Returning Remote: %s", (const char *)newRemote->name);
        }
	}
	fclose(f1);
	
	return head;
}

remote* CreateRemote(unsigned char *Name) {
	INFO1("Creating/Adding Remote: %s", Name);
	remote *Remote;

	Remote = (struct remote*)malloc(sizeof(struct remote)); //allocate space for a remote structure
	if (Remote == NULL) {
		return Remote;
	}
	Remote->name = Name; //point to caller allocated name string
	Remote->carrier_freq = 0;
	Remote->bit_time = 0;
	Remote->command = NULL;
	Remote->next = NULL;
	return Remote; //return pointer to remote structure
}

void InitDevice() {
	INFO("InitDevice Called");
}

command* RecordCommand(int devHandle, unsigned char *Name) {
	return 0;
}

void PlayCommand(int devHandle, remote *remote, unsigned char *name, int tx_repeats) {
    char cmd[1024];

    if ( ! remote ) {
        return;
    }

	INFO3("PlayCommand Called: Remote: %s; Command: %s; Repeats: %d", remote->name, name, tx_repeats);
   	INFO1("devHandle is: %i", devHandle);

	snprintf(cmd, 1024, "%s SEND %s %s", CMD_PATH, remote->name, name);   
    if (system(cmd)!=0) {
    	ERROR1("Failed to execute Tuner Command: %s", cmd);
    } else {
    	INFO1("Executed Tuner Command without Error: %s", cmd);
    }
}

void FreeRemotes(remote **head) {
	INFO("FreeRemotes Called.");
	command *Temp_Com; //temporary command pointer
	remote *Temp_Rem; //temporary remote pointer
	pattern *temp_pat; //temporary pattern pointer

	while (*head) {
		Temp_Rem = *head;
		Temp_Com = (*head)->command;
		while (Temp_Com) {
			(*head)->command = (*head)->command->next;
			temp_pat = Temp_Com->pattern;
			while (temp_pat) {
				Temp_Com->pattern = Temp_Com->pattern->next;
				free(temp_pat->bytes);
				free(temp_pat);
				temp_pat = Temp_Com->pattern;
			}
			free(Temp_Com->name); //free command list
			free(Temp_Com);
			Temp_Com = (*head)->command;
		}
		(*head) = (*head)->next;
		free(Temp_Rem->name); //free remote data
		free(Temp_Rem);
	}
	*head = NULL;
}

void DumpRemotes(remote *head) {
	command *chead = NULL;
	while (head) {
		printf("RemoteName: %s\n", head->name);
	    chead = head->command;
		while (chead) {
			printf("CmdKey: %s\n", chead->name);
			chead=chead->next;
		}
		printf("\n");
		head = head->next;
	}
}

int CanMacroTune(void) {
	char buf[1024];
    char cmd[1024];
	
	snprintf(cmd, 1024, "%s CAN_TUNE %s", CMD_PATH, loadedDevName);   
	INFO1("CanMacroTune command: %s", cmd);

	FILE *f1 = popen(cmd, "r");
	if (f1==NULL) {
		ERROR1("Failed to execute CAN_TUNE command %s", cmd);
		return 0;
	}
	
    int result = 0;
	while (fgets(buf, 1024, f1)) {
		if (buf==NULL) break;

		chomp(buf);
        if ( buf[0] == 'O' && buf[1] == 'K' ) {
	        INFO1("CAN_TUNE is OK: %s", buf);
            result = 1;
        } else {
	        INFO1("CAN_TUNE is not OK: %s", buf);
        }

        break;
		
	}
	fclose(f1);

	return result;
}

void MacroTune(int devHandle, int channel) {
	char cmd[1024];

	INFO2("MacroTune: devHandle: %d, Channel %d", devHandle, channel);

	snprintf(cmd, 1024, "%s TUNE %s %d", CMD_PATH, loadedDevName, channel);   
    if (system(cmd)!=0) {
    	ERROR1("Failed to execute MacroTune Command: %s", cmd);
    } else {
    	INFO1("Executed MacroTune Command without Error: %s", cmd);
    }
}
