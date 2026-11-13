#include "ChangedBlockTrackingSDKJsonSDK.h"
#include <string.h>
int main(void){uint64_t session=0;if(changed_block_tracking_sdk_json_open(&session))return 1;char *reply=changed_block_tracking_sdk_json_call(session,"{",1);if(!reply){changed_block_tracking_sdk_json_close(session);return 2;}int failed=strstr(reply,"\"ok\":false")==0;changed_block_tracking_sdk_json_free(reply);if(changed_block_tracking_sdk_json_close(session))return 3;return failed;}
