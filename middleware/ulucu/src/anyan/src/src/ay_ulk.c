#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "define.h"
#include "ayqueue.h"
#include "ayutil.h"
#include "cJSON.h"

#define AY_OEMID_ULK	1
#define AY_OEMID_LG	100

void ay_parse_oem_cmd(CMD_PARAM_STRUCT *args)
{
    if(args == NULL)
	return ;

    if(args->cmd_id == USER_DATA)
    {
		char json_cmd[1024] = {0};
		int _cmd = 0;
		memcpy(json_cmd,args->cmd_args,args->cmd_args_len); /* we would rewrite cmd_args content */
		// Parse json protocol
		DEBUGF("ulk json cmd:%s\n",json_cmd);
		ayJSON *pjson = ayJSON_Parse(json_cmd);
		if(pjson==NULL) 
			return;
		ayJSON *pitem = ayJSON_GetObjectItem(pjson,"oem_id");
		if(pitem == NULL) 
			return;

		if((AY_OEMID_ULK == pitem->valueint) || (AY_OEMID_LG== pitem->valueint))
		{
			pitem = ayJSON_GetObjectItem(pjson,"control");
			if(pitem==NULL)
			return;

			memset(args->cmd_args,0,sizeof(args->cmd_args));
			args->cmd_args_len = 0;

			_cmd = atoi(pitem->valuestring);
			switch(_cmd)
			{
			case 10001:
				args->cmd_id = EXT_MDSENSE_QUERY;
				args->cmd_args_len = 0;
				break;
			case 10002:
				args->cmd_id = EXT_MDSENSE_SET;
				pitem = ayJSON_GetObjectItem(pjson,"switch");
				if(pitem) 
					args->cmd_args[0] = atoi(pitem->valuestring);
				pitem = ayJSON_GetObjectItem(pjson,"value");
				if(pitem) args->cmd_args[1] = atoi(pitem->valuestring);
				args->cmd_args_len = 2;
				break;
			case 10011:
				args->cmd_id = EXT_ADSENSE_QUERY;
				args->cmd_args_len = 0;
				break;
			case 10012:
				args->cmd_id = EXT_ADSENSE_SET;
				pitem = ayJSON_GetObjectItem(pjson,"switch");
				if(pitem) args->cmd_args[0] = atoi(pitem->valuestring);
				pitem = ayJSON_GetObjectItem(pjson,"value");
				if(pitem) args->cmd_args[1] = atoi(pitem->valuestring);
				args->cmd_args_len = 2;
				break;
			case 10031:
				args->cmd_id = EXT_DETECT_RECORD_QUERY;
				pitem = ayJSON_GetObjectItem(pjson,"start_time");
				if(pitem) /* 2015-10-13 14:30:00 */
				{
					struct tm start;
					time_t ts;
					memset(&start,0,sizeof(start));
					//strptime(pitem->valuestring,"%Y-%m-%d %H:%M:%S",&start);
					sscanf(pitem->valuestring,"%d-%d-%d %d:%d:%d",&start.tm_year,&start.tm_mon,&start.tm_mday,&start.tm_hour,&start.tm_min,&start.tm_sec);
					start.tm_year -= 1900;
					start.tm_mon -= 1;
					ts = mktime(&start);
					args->cmd_args[0] = ts&0xff;
					args->cmd_args[1] = (ts>>8)&0xff;
					args->cmd_args[2] = (ts>>16)&0xff;
					args->cmd_args[3] = (ts>>24)&0xff;
				}
				pitem = ayJSON_GetObjectItem(pjson,"end_time");
				{
				struct tm end;
				time_t ts;
				memset(&end,0,sizeof(end));
				//strptime(pitem->valuestring,"%Y-%m-%d %H:%M:%S",&end);
				sscanf(pitem->valuestring,"%d-%d-%d %d:%d:%d",&end.tm_year,&end.tm_mon,&end.tm_mday,&end.tm_hour,&end.tm_min,&end.tm_sec);
				end.tm_year -= 1900;
				end.tm_mon -= 1;
				ts = mktime(&end);
				args->cmd_args[4] = ts&0xff;
				args->cmd_args[5] = (ts>>8)&0xff;
				args->cmd_args[6] = (ts>>16)&0xff;
				args->cmd_args[7] = (ts>>24)&0xff;
				}
				args->cmd_args_len = 8;
				break;
			case 10041:
				args->cmd_id = EXT_SDCARD_QUERY;
				args->cmd_args_len = 0;
				break;
			case 10042:
				args->cmd_id = EXT_SDCARD_CONTROL;
				pitem = ayJSON_GetObjectItem(pjson,"operate");
				if(pitem) 
				{
					unsigned short op = atoi(pitem->valuestring);
					args->cmd_args[0] = op&0xff;
					args->cmd_args[1] = (op>>8)&0xff;
					args->cmd_args_len += 2;
				}
				pitem = ayJSON_GetObjectItem(pjson,"switch_record");
				if(pitem) 
				{
					args->cmd_args[2] = atoi(pitem->valuestring);
					args->cmd_args_len += 1;
				}
				break;
			case 10051:
				args->cmd_id = EXT_PRESET_QUERY;
				args->cmd_args_len = 0;
				break;
			case 10052:
				args->cmd_id = EXT_PRESET_SET;
				pitem = ayJSON_GetObjectItem(pjson,"value");
				if(pitem) args->cmd_args[0] = atoi(pitem->valuestring);
				pitem = ayJSON_GetObjectItem(pjson,"index");
				if(pitem) args->cmd_args[1] = atoi(pitem->valuestring);
				pitem = ayJSON_GetObjectItem(pjson,"x");
				if(pitem)
				{
					int x = atoi(pitem->valuestring);
					args->cmd_args[2] = x&0xff;
					args->cmd_args[3] = (x>>8)&0xff;
					args->cmd_args[4] = (x>>16)&0xff;
					args->cmd_args[5] = (x>>24)&0xff;
				}
				pitem = ayJSON_GetObjectItem(pjson,"y");
				if(pitem)
				{
					int y = atoi(pitem->valuestring);
					args->cmd_args[6] = y&0xff;
					args->cmd_args[7] = (y>>8)&0xff;
					args->cmd_args[8] = (y>>16)&0xff;
					args->cmd_args[9] = (y>>24)&0xff;
				}
				pitem = ayJSON_GetObjectItem(pjson,"z");
				if(pitem)
				{
					int z = atoi(pitem->valuestring);
					args->cmd_args[10] = z&0xff;
					args->cmd_args[11] = (z>>8)&0xff;
				}
				args->cmd_args_len = 12;
				break;

			case 50001:
				args->cmd_id = EXT_IMAGE_QUERY_TURN;
				args->cmd_args_len = 0;
				break;
			case 50002:
				args->cmd_id = EXT_IMAGE_SET_TURN;
				pitem = ayJSON_GetObjectItem(pjson,"turn_h");
				if(pitem) args->cmd_args[0] = atoi(pitem->valuestring);
				pitem = ayJSON_GetObjectItem(pjson,"turn_v");
				if(pitem) args->cmd_args[1] = atoi(pitem->valuestring);
				args->cmd_args_len = 2;
				break;
			case 50011:
				args->cmd_id = EXT_REBOOT_DEVICE;
				pitem = ayJSON_GetObjectItem(pjson,"switch");
				if(pitem) args->cmd_args[0] = atoi(pitem->valuestring);
				args->cmd_args_len = 1;
				break;
			case 50021:
				args->cmd_id = EXT_RESET_DEVICE;
				pitem = ayJSON_GetObjectItem(pjson,"switch");
				if(pitem) args->cmd_args[0] = atoi(pitem->valuestring);
				args->cmd_args_len = 1;
				break;

			case 99990:
				args->cmd_id = EXT_MANAGE_DEVICE;
				pitem = ayJSON_GetObjectItem(pjson,"service");
				if(pitem) args->cmd_args[0] = atoi(pitem->valuestring);
				args->cmd_args_len = 1;
				break;
			default: /* note: we resotre original user data when we can't handle */
				DEBUGF("_cmd = %d unsupported !\n",_cmd);
				memcpy(args->cmd_args,json_cmd,strlen(json_cmd));
				args->cmd_args_len = strlen(json_cmd);
				break;
			}
			ayJSON_Delete(pjson);
		}
    }
}

struct ext_cmd_dict
{
    char name[32];
    char type;/* 0 - int, 1 - string */
    int value;
    char string[64];
};

int ay_ext_response(int cmd, struct ext_cmd_dict values[],int num)
{
    ayJSON *pjson = ayJSON_CreateObject();
    ayJSON *item = NULL;
    char buf[32]={0};

    if(pjson == NULL)
	return -1;
    item = ayJSON_CreateNumber(AY_OEMID_ULK);
    ayJSON_AddItemToObject(pjson,"oem_id",item); 
    item = ayJSON_CreateString("1.0.0.0");
    ayJSON_AddItemToObject(pjson,"version",item); 

    snprintf(buf,sizeof(buf),"%d",cmd);
    item = ayJSON_CreateString(buf);
    ayJSON_AddItemToObject(pjson,"control",item); 

    int i = 0;
    for(i=0;i<num;i++)
    {
		if(values[i].type==1)
		{
			item = ayJSON_CreateString(values[i].string);
			ayJSON_AddItemToObject(pjson,values[i].name,item); 
		}
		else
		{
			item = ayJSON_CreateNumber(values[i].value);
			ayJSON_AddItemToObject(pjson,values[i].name,item); 
		}
    }
    
    char *json_str = ayJSON_PrintUnformatted(pjson);
    DEBUGF("response json:%s\n",json_str);
    Ulu_SDK_Send_Userdata((uint8*)json_str, (uint16)strlen(json_str));

    free(json_str);
    ayJSON_Delete(pjson);
    return 0;
}

int Ulu_SDK_EXT_Response_All_Preset(AY_PRESET_POS preset[],int num)
{
    int ret = 0,i;
    char json_str[4096]={0};

    ret = snprintf(json_str,sizeof(json_str),"{\"oem_id\":%d,\"version\":\"1.0.0.0\",\"control\":\"20051\",\"value\":[",AY_OEMID_ULK);

    for(i=0;i<num;i++)
    {
	if(i==0)
	{
	    ret += snprintf(json_str+ret,sizeof(json_str)-ret,"{\"index\":\"%d\",\"x\":\"%d\",\"y\":\"%d\",\"z\":\"%d\"}",
		    preset[i].index,preset[i].x,preset[i].y,preset[i].z);
	}
	else
	{
	    ret += snprintf(json_str+ret,sizeof(json_str)-ret,",{\"index\":\"%d\",\"x\":\"%d\",\"y\":\"%d\",\"z\":\"%d\"}",
		    preset[i].index,preset[i].x,preset[i].y,preset[i].z);
	}
    }

    ret += snprintf(json_str+ret,sizeof(json_str)-ret,"]}");
    DEBUGF("json_str:%s\n",json_str);
    return Ulu_SDK_Send_Userdata((uint8*)json_str, strlen(json_str));
}

int Ulu_SDK_EXT_Response_Preset_Result(int index,int result)
{
    struct ext_cmd_dict values[4];

    values[0].type = 1;
    strcpy(values[0].name,"index");
    snprintf(values[0].string,sizeof(values[0].string),"%d",index);

    values[1].type = 1;
    strcpy(values[1].name,"value");
    snprintf(values[1].string,sizeof(values[1].string),"%d",result);

    return ay_ext_response(20052,values,2);
}

int Ulu_SDK_EXT_Response_Image_Turn(int hturn, int vturn)
{
    struct ext_cmd_dict values[2];

    values[0].type = 1;
    strcpy(values[0].name,"turn_h");
    snprintf(values[0].string,sizeof(values[0].string),"%d",hturn);

    values[1].type = 1;
    strcpy(values[1].name,"turn_v");
    snprintf(values[1].string,sizeof(values[1].string),"%d",vturn);

    return ay_ext_response(60001,values,2);
}

int Ulu_SDK_EXT_Response_Mdsense(int value,int enable)
{
    struct ext_cmd_dict values[2];

    values[0].type = 1;
    strcpy(values[0].name,"value");
    snprintf(values[0].string,32,"%d",value);

    values[1].type = 1;
    strcpy(values[1].name,"switch");
    snprintf(values[1].string,32,"%d",enable);

    return ay_ext_response(20001,values,2);
}

int Ulu_SDK_EXT_Response_Adsense(int value,int enable)
{
    struct ext_cmd_dict values[2];

    values[0].type = 1;
    strcpy(values[0].name,"value");
    snprintf(values[0].string,32,"%d",value);

    values[1].type = 1;
    strcpy(values[1].name,"switch");
    snprintf(values[1].string,32,"%d",enable);

    return ay_ext_response(20011,values,2);
}

int Ulu_SDK_EXT_Response_Private_Alarm(const char *msg)
{
    struct ext_cmd_dict values;

    values.type = 1;
    strcpy(values.name,"value");
    snprintf(values.string,sizeof(values.string),"%s",msg);

    return ay_ext_response(20021,&values,1);
}

int Ulu_SDK_EXT_Response_Detect_Record(uint32 start,uint32 end,int bitmap)
{
    struct tm mytm;
    struct ext_cmd_dict values[3];

    localtime_r((time_t*)&start,&mytm);
    values[0].type = 1;
    strcpy(values[0].name,"start_time");
    strftime(values[0].string,sizeof(values[0].string),"%Y-%m-%d %H:%M:%S",&mytm);
    localtime_r((time_t*)&end,&mytm);
    values[1].type = 1;
    strcpy(values[1].name,"end_time");
    strftime(values[1].string,sizeof(values[1].string),"%Y-%m-%d %H:%M:%S",&mytm);

    values[2].type = 1;
    strcpy(values[2].name,"value");
    //snprintf(values[2].string,sizeof(values[2].string),"%d",bitmap);
    int i = 0,ret = 0,val = 0;
    for(i=0;i<sizeof(int)*8;i++)
    {
		val = (bitmap>>i) & 0x01;
		ret += snprintf(values[2].string+ret,sizeof(values[2].string)-ret,"%d",val);
    }

    return ay_ext_response(20031,values,3);
}

int Ulu_SDK_EXT_Response_Sdcard_Result(SDcard_status status,int record,int total_v,int free_v)
{
    int val = 0;

    switch(status)
    {
	case SDCARD_NONE:
	    val = 30;
	    break;
	case SDCARD_NORMAL:
	    val = 20;
	    break;
	case SDCARD_FORMATTING:
	    val = 10;
	    break;
	case SDCARD_BAD:
	    val = 5;
	    break;
	default:
	    return -1;
    }
    struct ext_cmd_dict values[4];

    values[0].type = 1;
    strcpy(values[0].name,"value");
    snprintf(values[0].string,sizeof(values[0].string),"%d",val);

    values[1].type = 1;
    strcpy(values[1].name,"switch_record");
    snprintf(values[1].string,sizeof(values[1].string),"%d",record);

    values[2].type = 1;
    strcpy(values[2].name,"total_v");
    snprintf(values[2].string,sizeof(values[2].string),"%dM",total_v);

    values[3].type = 1;
    strcpy(values[3].name,"free_v");
    snprintf(values[3].string,sizeof(values[3].string),"%dM",free_v);

    return ay_ext_response(20041,values,4);
}

