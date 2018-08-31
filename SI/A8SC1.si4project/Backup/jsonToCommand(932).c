#include <stdlib.h>
#include <string.h>
#include "DebugLog.h"
#include "cJSON.h"
#include "smartStructureDefine.h"
#include <stdio.h>
#define JSON_COMMAND_TAG "JSON_COMMAND"

#define  GET_STR_FORM_JSON(Content,strName,jsonContent )  do 					\
		{																		\
			bzero((Content)->strName,sizeof((Content)->strName));				\
			if(cJSON_GetObjectItem(jsonContent,#strName) != NULL)  				\
				strcpy((Content)->strName,										\
				cJSON_GetObjectItem(jsonContent, #strName)->valuestring);		\
}while(0)

#define  GET_INTVALUE_FORM_JSON(Content,intValue,jsonContent )  do 				\
		{																		\
			(Content)->intValue = -1;											\
			if(cJSON_GetObjectItem(jsonContent,#intValue) != NULL)				\
			{																	\
				(Content)->intValue = 											\
					cJSON_GetObjectItem(jsonContent,#intValue)->valueint;		\
			}																	\
																				\
}while(0)
static int getCmdFromStr(const char *s_str[],char *d_str,int max_len);
static int getContent(cJSON *jsonContent,pS_SMARTHOME_CMD * Content);
static const char *settingsStr[WELBELL_SETTINGS_MAX] = {
		"welbell_operate_target_device",
		"welbell_operate_target_floor_device",
		"welbell_operate_target_floor_room_device",
		"welbell_search_all_devices",
		"welbell_linking_state",
		"welbell_status_refresh",
		"welbell_modify_scene",
		"welbell_create_scene",
		"welbell_modify_room",
		"welbell_modify_linkage_task",
		"welbell_delete_linkage_task",
		"welbell_create_floor",
};
static const char *resultStr[RESULT_MAX] = {
		"succeed",
		"flase"
};
static const char *switchState[ACTION_MAX] = {
		"off",
		"on"
};

		
//必须事先给jsonStr分配足量的空间　（>）
//将cmd结构体转换成字符串
int getJsonFromCmd(S_CONTROL_CMD       cmd,char * jsonStr)
{
	
	cJSON* pRoot = cJSON_CreateObject();  
	cJSON* content = cJSON_CreateObject();
    cJSON* devices = cJSON_CreateArray();  
	cJSON* devicespItem ;
	char* szJSON = NULL;
	
	if(!(pRoot&&content&&devices))
	{
		goto err1;
	}
	cJSON_AddStringToObject(pRoot, "setting", settingsStr[cmd.controlCmd]); 
	cJSON_AddStringToObject(pRoot, "result", resultStr[cmd.resultCode]); 
	cJSON_AddItemToObject(pRoot, "content", content);
	cJSON_AddItemToObject(content, "devices", devices);
	/*填充数组*/
	pS_SMARTHOME_CMD temp_content = cmd.content;
	while(temp_content != NULL)
	{
		devicespItem = cJSON_CreateObject();
		cJSON_AddStringToObject(devicespItem, "manufacturer", temp_content->manufacturer);
		cJSON_AddStringToObject(devicespItem, "device_name", temp_content->device_name);
		cJSON_AddStringToObject(devicespItem, "device_id", temp_content->device_id);
    	cJSON_AddStringToObject(devicespItem, "switch_state",switchState[temp_content->action]);  
    	cJSON_AddNumberToObject(devicespItem, "device_type", temp_content->dev_type);
		cJSON_AddNumberToObject(devicespItem, "values", temp_content->values);
    	cJSON_AddItemToArray(devices, devicespItem);
		temp_content = temp_content->next;
	}
	cJSON_AddStringToObject(pRoot, "success", "true"); 
	szJSON = cJSON_Print(pRoot);
	if(szJSON == NULL ||jsonStr == NULL )
		goto err0;
	strcpy(jsonStr,szJSON);	
	
	jsonStr[strlen(szJSON)] = 0;
	cJSON_Delete(pRoot); 
	free(szJSON);
	return 0;

err0:
	  cJSON_Delete(pRoot); 
	
err1:
		return -1;
	
	
}
int getCmdFromJson(char * jsonStr,pS_CONTROL_CMD         cmd) 
{
	char getStr[64] = {0};
	int getCmd;
	pS_CONTROL_CMD tempCmd = cmd;

	bzero(tempCmd,sizeof(S_CONTROL_CMD ));
	cJSON *jsonRoot = cJSON_Parse(jsonStr);  
	if(jsonRoot == NULL )
		return -1;
	cJSON *jsonContent = cJSON_GetObjectItem(jsonRoot,"content"); 
	if(cJSON_GetObjectItem(jsonRoot, "setting") == NULL)
	{
		LOGE(JSON_COMMAND_TAG,"get setting str fail!\n");
		return -1;
	}
	strcpy(getStr,cJSON_GetObjectItem(jsonRoot, "setting")->valuestring);

	getCmd = getCmdFromStr(settingsStr,getStr,WELBELL_SETTINGS_MAX);
	if(getCmd < 0) return -1;
	tempCmd->controlCmd = getCmd;
	if(cJSON_GetObjectItem(jsonRoot, "result") != NULL)
	{
		strcpy(getStr,cJSON_GetObjectItem(jsonRoot, "result")->valuestring);
		tempCmd->resultCode = getCmdFromStr(resultStr,getStr,RESULT_MAX);
	}
	 
	
	switch(getCmd)
	{
		case WELBELL_OPERATE_TARGET_DEVICE:
		case WELBELL_OPERATE_TARGET_FLOOR_DEVICE:
		case WELBELL_OPERATE_TARGET_FLOOR_ROOM_DEVICE:
			
			getContent(jsonContent,&(tempCmd->content));
			break;
		case WELBELL_SEARCH_ALL_DEVICES:
			//只赋值controlCmd即可
			break;
			
		case WELBELL_LINKING_STATE:
			break;
			
		case WELBELL_STATUS_REFRESH:
			break;
			
		case WELBELL_MODIFY_SCENE:
			break;
			
		case WELBELL_CREATE_SCENE:
			break;
			
		case WELBELL_MODIFY_ROOM:
			break;
			
		case WELBELL_MODIFY_LINKAGE_TASK:
			break;
			
		case WELBELL_DELETE_LINKAGE_TASK:
			break;

			
		case WELBELL_CREATE_FLOOR:
			break;
		default :
			break;
		
	}
	cJSON_Delete(jsonRoot);
	
}
static int getCmdFromStr(const char *s_str[],char *d_str,int max_len)
{
	int index;
	for(index= 0;index < max_len;index++)
	{
	
		if(strcmp(d_str,s_str[index]) == 0)
		{
			return index;
		}				
	}
	return -1;
}
static int getContent(cJSON *jsonContent,pS_SMARTHOME_CMD * Content)
{	
	cJSON * devices = NULL;
	pS_SMARTHOME_CMD tempContent = NULL;
	pS_SMARTHOME_CMD fristNode = NULL;
	char tempStr[32] = {0};
	uint mallocCount = 0;
	int temoCmd;

	if((devices = cJSON_GetObjectItem(jsonContent,"devices")) == NULL )
	{
		
		tempContent = (pS_SMARTHOME_CMD)malloc(sizeof(S_SMARTHOME_CMD));
		if(Content == NULL)
			return -1;
		*Content = tempContent;
		GET_STR_FORM_JSON(tempContent,device_name,jsonContent);
		GET_STR_FORM_JSON(tempContent,device_id,jsonContent);
		GET_STR_FORM_JSON(tempContent,manufacturer,jsonContent);
		GET_STR_FORM_JSON(tempContent,floor_name,jsonContent);
		GET_STR_FORM_JSON(tempContent,room_name,jsonContent);
		GET_INTVALUE_FORM_JSON(tempContent,dev_type,jsonContent);
		GET_INTVALUE_FORM_JSON(tempContent,values,jsonContent);
		GET_INTVALUE_FORM_JSON(tempContent,floor_id,jsonContent);
		GET_INTVALUE_FORM_JSON(tempContent,room_id,jsonContent);
		bzero(tempStr,sizeof(tempStr));
		if(cJSON_GetObjectItem(jsonContent, "action") != NULL){
			strcpy(tempStr,cJSON_GetObjectItem(jsonContent, "action")->valuestring);
			temoCmd = getCmdFromStr(switchState,tempStr,ACTION_MAX);
			if (temoCmd >= 0)
				tempContent->action = temoCmd;
		}
		(tempContent)->next = NULL;
		*Content = tempContent;
	}else{
			int devicesSize = cJSON_GetArraySize(devices);
			if(devicesSize > 0)
			{
					
				  cJSON *tasklist=devices->child;
				  while(tasklist)
				  {	
					
					tempContent = (pS_SMARTHOME_CMD)malloc(sizeof(S_SMARTHOME_CMD));
					if(tempContent == NULL)
						return -1;
					bzero(tempContent,sizeof(S_SMARTHOME_CMD ));
					if(mallocCount == 0 )
					{
						*Content = tempContent;
						fristNode = tempContent;
					}else {
						
						fristNode->next = tempContent;
						fristNode = fristNode->next;	
					}
					mallocCount ++;
					GET_STR_FORM_JSON(tempContent,manufacturer,tasklist);	
					GET_STR_FORM_JSON(tempContent,device_name,tasklist);
					GET_STR_FORM_JSON(tempContent,device_id,tasklist);	
					GET_STR_FORM_JSON(tempContent,room_name,jsonContent);
					GET_INTVALUE_FORM_JSON(tempContent,room_id,jsonContent);

					if( cJSON_GetObjectItem(tasklist, "action") != NULL ){
						bzero(tempStr,sizeof(tempStr));
						strcpy(tempStr,cJSON_GetObjectItem(tasklist, "action")->valuestring);
						temoCmd = getCmdFromStr(switchState,tempStr,ACTION_MAX);
						if (temoCmd >= 0)
					 		tempContent->action = temoCmd;
					}
					GET_INTVALUE_FORM_JSON(tempContent,floor_id,jsonContent);
					GET_STR_FORM_JSON(tempContent,floor_name,jsonContent);
					GET_INTVALUE_FORM_JSON(tempContent,values,tasklist);
					
					(tempContent) = (tempContent)->next;
					tasklist=tasklist->next;
					
				  }
			
			}
	}
}

int cleanCmd(pS_CONTROL_CMD cmd)
{
	pS_SMARTHOME_CMD temp_free = NULL; 
	pS_SMARTHOME_CMD frist_node = NULL;
	
	cmd->controlCmd = -1;
	cmd->resultCode = 1;
	frist_node = cmd->content;
	if(frist_node == NULL )
		return 0;
	
	while(frist_node->next != NULL)
	{
		  temp_free = frist_node;
		  frist_node = frist_node->next;
		  free(temp_free);
	}
	if(frist_node != NULL)
		free(frist_node);
	frist_node = NULL;

	return 0;
}






























