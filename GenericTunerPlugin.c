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
#include <time.h>
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

#define PLUGIN_VERSION "1.2"
#define PLUGIN_REVISION "1"

// used by the macro tune to hold the command of the last loaded remote
static char loadedDevName[256];

// syslog like log levels
#define LOG_NONE   -1   /* don't log anything */
#define LOG_ERROR   3   /* error conditions */
#define LOG_WARNING 4   /* warning conditions */
#define LOG_INFO    6   /* informational */
#define LOG_DEBUG   7   /* debug-level messages */

// only messages with priority of LOG_LEVEL and lower are logged
#define DEFAULT_LOG_LEVEL LOG_DEBUG

int currentLogLevel = DEFAULT_LOG_LEVEL;

// Simply non-efficient logger.
void _log(int priority, char *fmt, ...) {
    va_list ap;
    time_t now;
    struct tm *timeinfo;
    FILE *logfile;

    if ( priority > currentLogLevel ) return; 

    logfile = fopen(LOG_FILE, "a+");
    if (logfile == NULL) {
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
        printf("\n");
    	return;
    }

    time(&now);
    timeinfo = localtime(&now);
    fprintf(logfile, "%04i-%02i-%02i %02i:%02i ", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
            timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);

    switch(priority) {
        case LOG_ERROR:
            fprintf(logfile, "ERROR: ");
            break;
        case LOG_WARNING:
            fprintf(logfile, "WARNING: ");
            break;
        case LOG_INFO:
            fprintf(logfile, "INFO: ");
            break;
        case LOG_DEBUG:
            fprintf(logfile, "DEBUG: ");
            break;
        default:
            break;
    } 
  
    va_start(ap, fmt);
    vfprintf(logfile, fmt, ap);
    va_end(ap);

    fprintf(logfile, "\n");

    fflush(logfile);
    fclose(logfile);
}

// logging shortcuts
#define _log_error(a, args...) _log(LOG_ERROR, a, ## args);
#define _log_warning(a, args...) _log(LOG_WARNING, a, ## args);
#define _log_info(a, args...) _log(LOG_INFO, a, ## args);
#define _log_debug(a, args...) _log(LOG_DEBUG, a, ## args);

// simple method to allocate and copy string
char *alloc_string(char *str) {
    if ( str == NULL ) return NULL;
    char *newStr = malloc(strlen(str)+1); 
    strcpy((char *)newStr, (const char *)str);
    return newStr;
}

command* CreateCommand(char *Name) {
    struct command *Com; //pointer to new command structure

    Com = (struct command*)malloc(sizeof(struct command));//allocate space for a command structure
    if ( Com == NULL ) {
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
// So here we just log the current version and revision number.
// Note: The int value that is returned here is passed as 
//       "devHandle" to several other routines by Sage.
//
int OpenDevice(int ComPort) {
    int revision = 0;
    char *v_loc = strchr(PLUGIN_REVISION, ' ');
    if ( v_loc != NULL ) sscanf(v_loc+1, "%9d", &revision);
    _log_info("%s, Version %s, Revision %i", DEVICE_NAME, PLUGIN_VERSION, revision);

    _log_debug("OpenDevice: %d", ComPort);
    return 1;
}

void CloseDevice(int devHandle) {
    _log_debug("CloseDevice: %d", devHandle);
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
    if ( end >= 0 && s[end] == '\n' )
        s[end] = '\0';
}

int LoadRemoteKeys(struct remote *Remote) {
    char buf[1024];
    char cmd[1024];
    
    snprintf(cmd, sizeof(cmd), "%s KEYS %s", CMD_PATH, Remote->name);   
    _log_debug("LoadRemoteKeys: Using command: %s", cmd);

    FILE *f1 = popen(cmd, "r");
    if ( f1 == NULL ) {
        _log_error("LoadRemoteKeys: Failed to execute KEYS command: %s", cmd);
        return 0;
    }

    command *head = NULL;
    command *newCmd;
    
    while (fgets(buf, sizeof(buf), f1)) {
        if ( buf == NULL ) break;
        chomp(buf);
        
        newCmd = CreateCommand(alloc_string(buf));
        
        AddCommand(newCmd, &head);
    }
    
    pclose(f1);

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
    
    remote *head = NULL;

    if ( pszPathName == NULL ) {
        _log_debug("LoadRemotes: No Remote");
    } else {
        _log_debug("LoadRemotes: Remote: %s", pszPathName);
    }

    snprintf(cmd, sizeof(cmd), "%s REMOTES", CMD_PATH);   
    _log_debug("LoadRemotes: Using command: %s", cmd);

    FILE *f1 = popen(cmd, "r");
    if ( f1 == NULL ) {
        _log_error("LoadRemotes: Failed to execute REMOTES command: %s", cmd);
        return NULL;
    }
    
    remote *newRemote;
    while (fgets(buf, sizeof(buf), f1)) {
        if ( buf == NULL ) break;
        
        // eat the newline
        chomp(buf);
        _log_debug("LoadRemotes: Found Remote: %s", buf);
        
        // each line contains one and only one remote name
        newRemote = CreateRemote((unsigned char *)alloc_string(buf));
        
        // load the remote keys
        LoadRemoteKeys(newRemote);
        
        // if pszPathName is defined, only add that remote to the list
        if ( pszPathName ) {
            if ( 0 == strcmp((const char *)pszPathName,(const char *)newRemote->name) ) {
                // add the new remote to the list
                AddRemote(newRemote, &head);
                _log_debug("LoadRemotes: Returning Remote: %s", (const char *)newRemote->name);
                // save name in loadedDevName for MacroTune and CanMacroTune
                strncpy(loadedDevName, pszPathName, sizeof(loadedDevName));
            }
        } else {
            // add the new remote to the list
            AddRemote(newRemote, &head);
            _log_debug("LoadRemotes: Returning Remote: %s", (const char *)newRemote->name);
        }
    }
    pclose(f1);
    
    return head;
}

remote* CreateRemote(unsigned char *Name) {
    _log_debug("CreateRemote: Remote: %s", Name);
    remote *Remote;

    Remote = (struct remote*)malloc(sizeof(struct remote)); //allocate space for a remote structure
    if ( Remote == NULL ) {
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
    _log_debug("InitDevice");
}

command* RecordCommand(int devHandle, unsigned char *Name) {
    return 0;
}

void PlayCommand(int devHandle, remote *remote, unsigned char *name, int tx_repeats) {
    char cmd[1024];

    if ( ! remote ) {
        return;
    }

    snprintf(cmd, sizeof(cmd), "%s SEND %s %s", CMD_PATH, remote->name, name);   
    _log_debug("PlayCommand: Using command: %s", cmd);
    _log_info("PlayCommand: Remote: %s, Key: %s", remote->name, name);

    if ( system(cmd) != 0 ) {
        _log_error("PlayCommand: Failed to execute SEND command: %s", cmd);
    } else {
        _log_debug("PlayCommand: Executed SEND command without error: %s", cmd);
    }
}

void FreeRemotes(remote **head) {
    _log_debug("FreeRemotes");
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
    
    snprintf(cmd, sizeof(cmd), "%s CAN_TUNE %s", CMD_PATH, loadedDevName);   
    _log_debug("CanMacroTune: Using command: %s", cmd);

    FILE *f1 = popen(cmd, "r");
    if ( f1 == NULL ) {
        _log_error("CanMacroTune: Failed to execute CAN_TUNE command: %s", cmd);
        return 0;
    }
    
    int result = 0;
    while (fgets(buf, sizeof(buf), f1)) {
        if ( buf == NULL ) break;

        chomp(buf);
        if ( buf[0] == 'O' && buf[1] == 'K' ) {
            _log_debug("CanMacroTune: CAN_TUNE is OK: %s", buf);
            result = 1;
        } else {
            _log_debug("CanMacroTune: CAN_TUNE is not OK: %s", buf);
        }

        break;
        
    }
    pclose(f1);

    return result;
}

void MacroTune(int devHandle, int channel) {
    char cmd[1024];

    snprintf(cmd, sizeof(cmd), "%s TUNE %s %d", CMD_PATH, loadedDevName, channel);   
    _log_debug("MacroTune: Using command: %s", cmd);
    _log_info("MacroTune: Remote: %s, Channel: %d", loadedDevName, channel);

    if ( system(cmd) != 0 ) {
        _log_error("MacroTune: Failed to execute TUNE command: %s", cmd);
    } else {
        _log_debug("MacroTune: Executed TUNE command without error: %s", cmd);
    }
}

void SetRemoteName(char *name) {
	strncpy(loadedDevName, name, sizeof(loadedDevName));
}
