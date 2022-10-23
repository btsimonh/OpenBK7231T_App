#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../obk_config.h"
#include <ctype.h>
#include "cmd_local.h"
#include "../driver/drv_ir.h"
#include "../memory/memtest.h"

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




static int callme(int calls){
#ifdef PLATFORM_BK7231T
	logStack("callme");
	rtos_delay_milliseconds(40);
	calls++;
	//memset(test, 0, 200);
	//char *test2 = malloc(100);
	ADDLOG_INFO(LOG_FEATURE_CMD, " recurse %d", calls);
	if (calls < 10){
		callme(calls);
	}
	//free(test2);
#endif	
	return 0;
}

static int CMD_SimonTest(const void *context, const char *cmd, const char *args, int cmdFlags){


#ifdef PLATFORM_BK7231T
	logStack("CMD_SimonTest");
	mallocTest();
	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SimonTest: starting stackvarsize %d", sizeof(StackType_t));

	char *test = malloc(4000);

	callme(0);

	if (test){
		memset(test, 0xaa, 4000);
		free(test);
	} else {
		ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_SimonTest: #####MALLOC FAILE####");

	}
	//rtos_delay_milliseconds(500);
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
static int CMD_ClearConfig(const void *context, const char *cmd, const char *args, int cmdFlags){

	CFG_SetDefaultConfig();
	CFG_Save_IfThereArePendingChanges();

	ADDLOG_INFO(LOG_FEATURE_CMD, "CMD_ClearConfig: whole device config has been cleared, restart device to connect to clear AP");

	return 1;
}

void CMD_Init() {
    CMD_RegisterCommand("restart", "", CMD_Restart, "Reboots the module", NULL);
    CMD_RegisterCommand("clearConfig", "", CMD_ClearConfig, "Clears all config", NULL);
    CMD_RegisterCommand("simonirtest", "", CMD_SimonTest, "Simons Special Test", NULL);
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


