#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../driver/drv_ir.h"

#ifdef BK_LITTLEFS
	#include "../littlefs/our_lfs.h"
#endif

#define HASH_SIZE 128

static int generateHashValue(const char *fname) {
	int		i;
	int		hash;
	int		letter;
	unsigned char *f = (unsigned char *)fname;

	hash = 0;
	i = 0;
	while (f[i]) {
		letter = tolower(f[i]);
		hash+=(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (HASH_SIZE-1);
	return hash;
}

command_t *g_commands[HASH_SIZE] = { NULL };

static int CMD_SimonTest(const void *context, const char *cmd, const char *args, int cmdFlags){
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SimonTest: ir test routine");

#ifdef PLATFORM_BK7231T
	stackCrash(0);
	CrashMalloc();
	// anything
#endif

	
	return 1;
}

static int CMD_Restart(const void *context, const char *cmd, const char *args, int cmdFlags){
	int delaySeconds;

	if(args==0||*args==0) {
		delaySeconds = 3;
	} else {
		delaySeconds = atoi(args);
	}

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_Restart: will reboot in %i",delaySeconds);

	RESET_ScheduleModuleReset(delaySeconds);

	return 1;
}
static int CMD_ClearAll(const void *context, const char *cmd, const char *args, int cmdFlags) {

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	CHANNEL_ClearAllChannels();

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearAll: all clear");

	return 1;
}
static int CMD_ClearConfig(const void *context, const char *cmd, const char *args, int cmdFlags){

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearConfig: whole device config has been cleared, restart device to connect to clear AP");

	return 1;
}
static int CMD_Echo(const void *context, const char *cmd, const char *args, int cmdFlags){


	ADDLOG_INFO(LOG_FEATURE_CMD, args);
	// send direct to serial too.
	// does no harm, but good to see exactly when for diagnostic purposes
	bk_printf(args);

	return 1;
}


void CMD_Init_Early() {
	//cmddetail:{"name":"echo","args":"[string to echo]",
	//cmddetail:"descr":"Echo something to logs and serial",
	//cmddetail:"fn":"CMD_Echo","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":"echo hello"}
    CMD_RegisterCommand("echo", "", CMD_Echo, NULL, NULL);
	//cmddetail:{"name":"restart","args":"[delay in seconds, default 3]",
	//cmddetail:"descr":"Reboots the module",
	//cmddetail:"fn":"CMD_Restart","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":"restart 5"}
    CMD_RegisterCommand("restart", "", CMD_Restart, NULL, NULL);
	//cmddetail:{"name":"clearConfig","args":"none",
	//cmddetail:"descr":"Clears all config, and restarts in AP mode",
	//cmddetail:"fn":"CMD_ClearConfig","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("clearConfig", "", CMD_ClearConfig, NULL, NULL);
	//cmddetail:{"name":"simonirtest","args":"depends?",
	//cmddetail:"descr":"Simons Special Test",
	//cmddetail:"fn":"CMD_SimonTest","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":"don't do it"}
    CMD_RegisterCommand("simonirtest", "", CMD_SimonTest, NULL, NULL);
	//cmddetail:{"name":"if","args":"",
	//cmddetail:"descr":"",
	//cmddetail:"fn":"CMD_If","file":"cmnds/cmd_main.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("if", NULL, CMD_If, NULL, NULL);
#if (defined WINDOWS) || (defined PLATFORM_BEKEN)
	CMD_InitScripting();
#endif
}

void CMD_Init_Delayed() {
	if(CFG_HasFlag(OBK_FLAG_CMD_ENABLETCPRAWPUTTYSERVER)) {
		CMD_StartTCPCommandLine();
	}
}


void CMD_ListAllCommands(void *userData, void (*callback)(command_t *cmd, void *userData)) {
	int i;
	command_t *newCmd;

	for(i = 0; i < HASH_SIZE; i++) {
		newCmd = g_commands[i];
		while(newCmd) {
			callback(newCmd,userData);
			newCmd = newCmd->next;
		}
	}

}
void CMD_RegisterCommand(const char *name, const char *args, commandHandler_t handler, const char *userDesc, void *context) {
	int hash;
	command_t *newCmd;

	// check
	newCmd = CMD_Find(name);
	if(newCmd != 0) {
		ADDLOG_ERROR(LOG_FEATURE_CMD, "command with name %s already exists!",name);
		return;
	}
	ADDLOG_DEBUG(LOG_FEATURE_CMD, "Adding command %s",name);

	hash = generateHashValue(name);
	newCmd = (command_t*)malloc(sizeof(command_t));
	newCmd->argsFormat = args;
	newCmd->handler = handler;
	newCmd->name = name;
	newCmd->next = g_commands[hash];
	newCmd->userDesc = userDesc;
	newCmd->context = context;
	g_commands[hash] = newCmd;
}

command_t *CMD_Find(const char *name) {
	int hash;
	command_t *newCmd;

	hash = generateHashValue(name);

	newCmd = g_commands[hash];
	while(newCmd != 0) {
		if(!stricmp(newCmd->name,name)) {
			return newCmd;
		}
		newCmd = newCmd->next;
}
	return 0;
}

// get a string up to whitespace.
// if stripnum is set, stop at numbers.
int get_cmd(const char *s, char *dest, int maxlen, int stripnum){
	int i;
	int count = 0;
	for (i = 0; i < maxlen-1; i++){
		if (isWhiteSpace(*s)) {
			break;
		}
		if(*s == 0) {
			break;
		}
		if (stripnum && *s >= '0' && *s <= '9'){
			break;
		}
		*(dest++) = *(s++);
		count++;
	}
	*dest = '\0';
	return count;
}


// execute a command from cmd and args - used below and in MQTT
int CMD_ExecuteCommandArgs(const char *cmd, const char *args, int cmdFlags) {
	command_t *newCmd;
	//int len;

	// look for complete commmand
	newCmd = CMD_Find(cmd);
	if (!newCmd) {
		// not found, so...
		char nonums[32];
		// get the complete string up to numbers.
		//len =
		get_cmd(cmd, nonums, 32, 1);
		newCmd = CMD_Find(nonums);
		if (!newCmd) {
			// if still not found, then error
			ADDLOG_ERROR(LOG_FEATURE_CMD, "cmd %s NOT found (args %s)", cmd, args);
			return 0;
		}
	} else {
	}

	if (newCmd->handler){
		int res;
		res = newCmd->handler(newCmd->context, cmd, args, cmdFlags);
		return res;
	}
	return 0;
}


// execute a raw command - single string
int CMD_ExecuteCommand(const char *s, int cmdFlags) {
	const char *p;
	const char *args;

	char copy[128];
	int len;
	//const char *org;

	if(s == 0 || *s == 0) {
		return 0;
	}

	while(isWhiteSpace(*s)) {
		s++;
	}
	if(*s == 0) {
		return 0;
	}
	if((cmdFlags & COMMAND_FLAG_SOURCE_TCP)==0) {
		ADDLOG_DEBUG(LOG_FEATURE_CMD, "cmd [%s]", s);
	}
	//org = s;

	// get the complete string up to whitespace.
	len = get_cmd(s, copy, sizeof(copy), 0);
	s += len;

	p = s;

	while(*p && isWhiteSpace(*p)) {
		p++;
	}
	args = p;

	return CMD_ExecuteCommandArgs(copy, args,cmdFlags);
}


