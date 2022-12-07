#include "../new_pins.h"
#include "../new_cfg.h"
#include "../logging/logging.h"
#include "../httpclient/http_client.h"
#include "cmd_local.h"

// SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle
// addRepeatingEvent 5 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle
// addEventHandler OnClick 8 SendGet http://192.168.0.112/cm?cmnd=Power0%20Toggle
static commandResult_t CMD_SendGET(const void *context, const char *cmd, const char *args, int cmdFlags){
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendGET received with args %s",args);

#if defined(PLATFORM_BEKEN) || defined(WINDOWS)
	HTTPClient_Async_SendGet(args);
#else
	ADDLOG_INFO(LOG_FEATURE_CMD, " CMD_SendGET not supported!");

#endif
	return CMD_RES_OK;
}


int CMD_InitSendCommands(){
	//cmddetail:{"name":"sendGet","args":"",
	//cmddetail:"descr":"qq",
	//cmddetail:"fn":"CMD_SendGET","file":"cmnds/cmd_send.c","requires":"",
	//cmddetail:"examples":""}
    CMD_RegisterCommand("sendGet", "", CMD_SendGET, NULL, NULL);

    return 0;
}
