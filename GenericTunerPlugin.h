/*
 * Copyright 2001-2004 Frey Technologies, LLC. All rights reserved.
 */

struct pattern
{
	unsigned bit_length;
	unsigned length;
	char r_flag;
	unsigned char *bytes;
	struct pattern *next;
};
typedef struct pattern pattern;	

struct command 
{
	unsigned char *name;
	struct pattern *pattern;
	struct command *next;
};      	
typedef struct command command;

struct remote 
{
	unsigned char *name;
	unsigned long carrier_freq;
	unsigned bit_time;
	struct command *command;
	struct remote *next;
};
typedef struct remote remote;

#ifdef __cplusplus
extern "C" {
#endif

// External
int CanMacroTune(void);
void MacroTune(int,int);
int NeedBitrate(void);
int NeedCarrierFrequency(void);
const char* DeviceName();
int OpenDevice(int ComPort);
void CloseDevice(int);
unsigned long FindBitRate(int);
unsigned long FindCarrierFrequency(int);
struct remote *CreateRemote(unsigned char *Name);
void AddRemote(struct remote *Remote, struct remote **head);
void AddCommand(struct command *Command, struct command **Command_List);
void SaveRemotes(remote *head, const char* pszPathName);
struct remote *LoadRemotes(const char* pszPathName);
void InitDevice();
command* RecordCommand(int,unsigned char *Name);
void PlayCommand (int,remote *remote, unsigned char *name, int tx_repeats);
void FreeRemotes(remote **head);
void DumpRemotes(remote *head);

void SetRemoteName(char *name);

#ifdef __cplusplus
}
#endif
