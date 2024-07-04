#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#include "aydes.h"
#include "cJSON.h"
#include "define.h"
#include "http_base64.h"
#include "protocol_device.h"
#include "ayentry.h"
#include "ay_sdk.h"
#include "ayutil.h"

static int json_info_decode(const char* pin_Content,int in_len,char pou_Content[],int buf_len)
{
    char *pb64_buf = NULL;
    int decode_len = 0;
    int decode_buflen = 0;

    pb64_buf = (char*)malloc(in_len+1);
    if (!pb64_buf)
    {
		ERRORF("decode json malloc %d err1\n",in_len+1);
		return -1;
    }

    decode_buflen = in_len;
    memset(pb64_buf, 0, in_len + 1);

    decode_len = ZBase64Decode((const char*)pin_Content, in_len,(unsigned char*)pb64_buf,&decode_buflen);
    if (decode_len <= 0)
    {		
		ERRORF("ZBase64Decode ret = %d\n",decode_len);
		free(pb64_buf);
		return -1;
    }
    if(DES_Decrypt((uint8*)pb64_buf, decode_len, (uint8*)_ANYAN_DEFAULT_KEY, 8,(uint8*)pou_Content, buf_len,&decode_buflen) != ANYAN_DES_OK)
    {
		ERRORF("DES_Decrypt err3\n");
		free(pb64_buf);
		return -1;
    }

    free(pb64_buf);
    return decode_buflen;
}

/*
 * return webcontent use malloc memory,so the caller must free the RESULT if it's not NULL!!
 */
char *request_webserver_content_v2(const char *purl, int *pout_len, int (*handle)(const char *in, int inlen, char out[], int outlen))
{
    int bytes = 0;
    ghttp_status req_status;
    ghttp_request *pReq = NULL;
    char *pWebContent = NULL;
    char *pcontent = NULL;

    if (purl == NULL || NULL == (pReq = ghttp_request_new())) {
        ERRORF("ghttp_request_new fail!\n");
        return NULL;
    }

    if (ghttp_set_uri(pReq, (char*)purl) < 0) {
		ERRORF("ghttp_set_uri:%s fail!\n",purl);
		ghttp_request_destroy(pReq);
		return NULL;
    }

    ghttp_set_sync(pReq, ghttp_async);
    ghttp_prepare(pReq);
    do {
        req_status = ghttp_process(pReq);
        if (req_status == ghttp_error) {
            errorf("ghttp err: %s\n", ghttp_get_error(pReq));
            ghttp_request_destroy(pReq);
            return NULL;
        }
    } while (req_status == ghttp_not_done);

    bytes = ghttp_get_body_len(pReq);
    if ((pWebContent = ghttp_get_body(pReq)))
    {
        pcontent = (char *)malloc(bytes + 1);
        if (NULL != pcontent) {
            memset(pcontent, 0, bytes + 1);
            if (handle) {
                bytes = handle(pWebContent, bytes, pcontent, bytes);
                if (pout_len) {
                    *pout_len = bytes;
                }
            } else {
                memcpy(pcontent, pWebContent, bytes);
                if (pout_len) {
                    *pout_len = bytes;
                }
            }
        }
    }

    ghttp_request_destroy(pReq);
    return pcontent;
}

#include "prebuilts/include/curl/curl.h"
typedef struct {
	int size;
	int capacity;
	char* data;
}CURL_BODY_DATA;

static size_t curl_writedata_callback(void* contents, size_t size, size_t nmemb, void* user) {
	CURL_BODY_DATA* data = (CURL_BODY_DATA*)user;
	int content_size = size * nmemb;
	if (data->size + content_size < data->capacity) {
		memcpy(data->data + data->size, contents, content_size);
		data->size += content_size;
		return content_size;
	} else {
		errorf("buffer capacity:%d < datalen\n", data->capacity);
		return 0;
	}
}

char *request_webserver_content(const char *url, int *pout_len, int (*handle)(const char *in, int inlen, char out[], int outlen))
{
    if (url == NULL) {
        return NULL;
    }
    tracef("http url: %s\n", url);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL *curl = curl_easy_init();
    if (curl == NULL) {
		errorf("curl_easy_init faild\n");
        return NULL;
    }
	CURL_BODY_DATA body_data = { 0 };
	body_data.capacity = 4 * 1024;
	body_data.data = (char*)malloc(body_data.capacity);
	if (body_data.data == NULL) {
		errorf("malloc size:%d faild\n", body_data.capacity);
		return NULL;
	}
	memset(body_data.data, 0x00, body_data.capacity);

    curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writedata_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body_data);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        errorf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		free(body_data.data);
        return NULL;
    }
    curl_easy_cleanup(curl);

    char *handled_data = NULL; (char*)malloc(body_data.size + 1);
    if (handle) {
        char *handled_data = (char*)malloc(body_data.size + 1);
        if (handled_data == NULL) {
            free(body_data.data);
            return NULL;
        }
        memset(handled_data, 0x00, body_data.size + 1);
        int bytes = handle(body_data.data, body_data.size, handled_data, body_data.size);
        if (pout_len) {
            *pout_len = bytes;
        }
        free(body_data.data);
        return handled_data;
    }
	return body_data.data;
}

static int get_group(unsigned int group_num)
{
    unsigned int sum = 0,i = 0;

    for (i = 0; i<sdk_get_handle(0)->devinfo.did.device_id_length; ++i)
    {
		sum += sdk_get_handle(0)->devinfo.did.device_id[i];
    }

    DEBUGF(" sum: %d\n",sum);
    return sum % group_num;
}

static int get_entryserver_byjson(const char* pContent, entry_server_list* pserver_list)
{
    ayJSON *pjson = NULL;
    ayJSON *pchild = NULL;
    ayJSON *pgroup = NULL;
    ayJSON *pvalue = NULL;
    int   group_on = 0;
    int   group_num = 0;
    int i = 0;

    DEBUGF("entry server json: %s\n", pContent);
    pjson = ayJSON_Parse(pContent);
    if (!pjson) {
        return -1;
    }

    pchild = ayJSON_GetObjectItem(pjson, "version");
    if (pchild) {
        DEBUGF("entry ver:%s\n", pchild->valuestring);
        strncpy(sdk_get_handle(0)->devinfo.entry_ver, pchild->valuestring, 63);
    }

    pchild = ayJSON_GetObjectItem(pjson, "group_num");
    if (pchild) {
        group_num = pchild->valueint;
        if (group_num < 1) {
            ayJSON_Delete(pjson);
            return -1;
        }
        group_on = get_group(group_num);
        DEBUGF("group_num: %d,use group: %d\n", group_num, group_on);
    } else {
        ayJSON_Delete(pjson);
        return -1;
    }

    pgroup = ayJSON_GetObjectItem(pjson, "groups");
    if (!pgroup) {
        ERRORF("{{DeviceCommEvent}}GetTrackerConfig_FAIL|get groups fail!\n");
        ayJSON_Delete(pjson);
        return -1;
    }

    pchild = ayJSON_GetArrayItem(pgroup, group_on);
    if (!pchild) {
        ERRORF("{{DeviceCommEvent}}GetTrackerConfig_FAIL|get server list fail!\n");
        ayJSON_Delete(pjson);
        return -1;
    }

    pgroup = ayJSON_GetObjectItem(pchild, "config");
    if (!pgroup) {
        ERRORF("{{DeviceCommEvent}}GetTrackerConfig_FAIL|get server config fail!\n");
        ayJSON_Delete(pjson);
        return -1;
    }

    memset(pserver_list, 0, sizeof(entry_server_list));
    i = 0;
    while (pgroup) {
        pchild = ayJSON_GetArrayItem(pgroup, i);
        if (!pchild) {
            break;
        }
        if (pserver_list->num >= ENTRY_SERVER_MAX_NUM) {
            break;
        }
        pserver_list->num++;
        DEBUGF("***********************************************************\n");
        pvalue = ayJSON_GetObjectItem(pchild, "ip");
        if (pvalue && pvalue->valuestring) {
            strcpy(pserver_list->list[i].IP, pvalue->valuestring);
            DEBUGF(" ip: %s\n",pvalue->valuestring);
        } else {
            pvalue = ayJSON_GetObjectItem(pchild, "domain");
            if (pvalue && pvalue->valuestring) {
                DEBUGF(" domain: %s\n",pvalue->valuestring);
                ayutil_getaddrinfo_byip(pvalue->valuestring, 9010, pserver_list->list[i].IP, sizeof(pserver_list->list[i].IP));
            }
        }

        pvalue = ayJSON_GetObjectItem(pchild, "port");
        if (pvalue != NULL) {
            pserver_list->list[i].port = pvalue->valueint;
            DEBUGF(" port: %d\n", pvalue->valueint);
        }		
        DEBUGF("***********************************************************\n"); 
        i++;
    }
    ayJSON_Delete(pjson);
    return 0;
}

static int get_entryserver_list_bywed(const char* pIn_url,entry_server_list* pserver_list)
{
    int jsonlen = 0;
    int ret = -1;
    char *pJsonContent = NULL;
    pJsonContent = request_webserver_content(pIn_url, &jsonlen, json_info_decode);
    if (pJsonContent) {
        ret = get_entryserver_byjson(pJsonContent, pserver_list);
        free(pJsonContent);
        return ret;
    }
    return -1;
}

int ayentry_get_entryserver(entry_server_list* pserver_list)
{
    char url[256];
    snprintf(url, sizeof(url), "http://%s/entry.dat", AYENTRY_SERVER_CONFIG);
    if (get_entryserver_list_bywed(url, pserver_list) < 0) {
        ERRORF("{{DeviceCommEvent}}GetTrackerConfig_FAIL|try read entrylist file!\n");
        ayutil_del_host(AYENTRY_SERVER_CONFIG);
        return ayutil_read_file(sdk_get_handle(0)->devinfo.rw_path, "entrylist", (char*)pserver_list, sizeof(entry_server_list));
    }
    ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path, "entrylist", (const char *)pserver_list, sizeof(entry_server_list));
    return 0;
}

/************************* Get Entry Server Login info ***********************/
int ayentry_get_alarm_param(ayJSON *pjson,ULK_Alarm_Struct *palarm)
{
    ayJSON *pchild = NULL;
    ayJSON *pdata = NULL;
    ayJSON *pdata_tmp = NULL;

    pdata = pjson;
    memset(palarm, 0, sizeof(ULK_Alarm_Struct));
    do
    {
		pchild = ayJSON_GetObjectItem(pdata, "alarm");
		if (!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get alarm fail!\n");							
			break;
		}
		else
		{
			palarm->alarm_flag = pchild->valueint;
		}

		pchild = ayJSON_GetObjectItem(pdata, "interval");
		if (!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get interval fail!\n"); 						
			break;
		}
		else
		{
			palarm->alarm_interval_mix = pchild->valueint;
		}
		pchild = ayJSON_GetObjectItem(pdata, "detail");
		if (!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get detail fail!\n");						
			break;
		}
		else
		{
			int tmp , i;
			tmp = ayJSON_GetArraySize(pchild);
			tmp = tmp < sizeof(palarm->alarm_time_table)?tmp:sizeof(palarm->alarm_time_table);

			palarm->alarm_time_table_num = tmp;
			for ( i = 0 ; i < tmp; i++ )
			{
				pdata = ayJSON_GetArrayItem(pchild, i);
				if ( pdata != NULL )
				{
					pdata_tmp = ayJSON_GetObjectItem(pdata, "start");
					if ( pdata_tmp != NULL )
					{
						palarm->alarm_time_table[i].start = pdata_tmp->valueint;
					}
					pdata_tmp = ayJSON_GetObjectItem(pdata, "end");
					if ( pdata_tmp != NULL )
					{
						palarm->alarm_time_table[i].end= pdata_tmp->valueint;
					}
				}
			}
			notify_alarm_config_update(palarm,1);
		}
		return 1;
    }while(1);
    return -1;
}
static int get_entry_token_byjson(const char* in_content,int in_content_len,entry_token* ou_token)
{
    ayJSON *pjson = NULL;
    ayJSON *pchild = NULL;
    ayJSON *pdata = NULL;

    pjson=ayJSON_Parse(in_content);
    if (!pjson)
    {
		return -1;
    }

    do 
    {
		pchild = ayJSON_GetObjectItem(pjson,"code");
		if(!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token code fail!\n");
			break;
		}
		ou_token->icode = pchild->valueint;
		sdk_get_handle(0)->devinfo.resp_json_code = pchild->valueint;

		pchild = ayJSON_GetObjectItem(pjson,"message");
		if(!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token message fail!\n");
			break;
		}

		if(pchild->valuestring) 
			strcpy(ou_token->szmessage,pchild->valuestring);
		pdata = ayJSON_GetObjectItem(pjson,"data");
		if(!pdata)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token data fail!\n");
			break;
		}

		pchild = ayJSON_GetObjectItem(pdata,"device_id");
		if (!pchild || !pchild->valuestring)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token did fail!\n");
			break;
		}

		//DEBUGF(" device_id: %s\n",pchild->valuestring);
		if(pchild->valuestring)
			strcpy(ou_token->data.device_id,pchild->valuestring);

		pchild = ayJSON_GetObjectItem(pdata,"status");
		if (!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token status fail!\n");
			break;
		}
		ou_token->data.status = pchild->valueint;

		pchild = ayJSON_GetObjectItem(pdata,"token");
		if (!pchild || !pchild->valuestring)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token token fail!\n");
			break;
		}
		if(pchild->valuestring)
			strcpy(ou_token->data.token,pchild->valuestring);

		pchild = ayJSON_GetObjectItem(pdata,"user_type");
		if (!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token user_type fail!\n");
			break;
		}
		if(pchild->valuestring)ou_token->data.iuser_type = atoi(pchild->valuestring);

		pchild = ayJSON_GetObjectItem(pdata,"oem");
		if (!pchild)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token oem fail!\n");
			break;
		}

		if(pchild->valuestring) 
			ou_token->data.oem = atoi(pchild->valuestring);

		pchild = ayJSON_GetObjectItem(pdata,"channel_count");
		if (!pchild || pchild->valueint <= 0)
		{
			ERRORF("{{DeviceCommEvent}}GetToken_FAIL|get token channel_count fail!\n");
			ou_token->data.ichancel_count = 0;
		}
		else
		{
			ou_token->data.ichancel_count = pchild->valueint;
		}

		pchild = ayJSON_GetObjectItem(pdata,"device_flag");
		if (pchild)
		{
			sdk_get_handle(0)->devinfo.vip_flag = pchild->valueint;
			DEBUGF("*** vip flag = %#x ***\n",sdk_get_handle(0)->devinfo.vip_flag);
		}
		ayentry_get_alarm_param(pdata,&sdk_get_handle(0)->alarm_ctrl);

		DEBUGF("get token success!\n");
		ayJSON_Delete(pjson);
		return 0;
    } while (0);
    ayJSON_Delete(pjson);
    return -1;
}

static int get_entryserver_logininfo_byweb(const char* pin_url, entry_token* ou_token, entry_request_param *param)
{
    char *pJonsContent = NULL;
    int json_len = 0,i = 0, len = 0;
    char entry_token_url[1024] = {0};
    char szrate[64] = {0};

    if (pin_url == NULL || param == NULL || ou_token == NULL) {
        return -1;
    }

    if (param->rate_count > 0) {
        len = snprintf(szrate,sizeof(szrate), "%u", param->ratelist[0]);
        for (i = 0; i < param->rate_count - 1; i++) {
            len += snprintf(szrate + len, sizeof(szrate) - len, ",%u", param->ratelist[i+1]);
        }
    }

    snprintf(entry_token_url, sizeof(entry_token_url), "%s?device_id=%s&rate=%s&ver=%d.%d.%d.%d&audio=%u&recv_audio=%u&tilt_spin=%u&spin=%u&zoom=%u&disk=%u&position=%u"
        ,pin_url, param->device_id, szrate, param->ver[0], param->ver[1], param->ver[2], param->ver[3], param->audio, param->recv_audio
        ,param->tilt_spin, param->spin, param->zoom,param->disk, param->preset);
    DEBUGF("entry_token_url: %s\n", entry_token_url);

    pJonsContent = request_webserver_content(entry_token_url, &json_len, json_info_decode);
    if (pJonsContent == NULL) {
        return -1;
    }
    pJonsContent[json_len] = '\0';
    DEBUGF("tokenjs decode:%s\n", pJonsContent);		
    if (get_entry_token_byjson(pJonsContent, json_len, ou_token) == -1) {		
        free(pJonsContent);
        return -1;
    }

    free(pJonsContent);
    return 0;
}

int ayentry_get_token(entry_token* ou_token, entry_request_param *param)
{
    char url[256];
    snprintf(url, sizeof(url), "http://%s/device/tokencode", AYENTRY_SERVER_PUBLIC);
    int ret = get_entryserver_logininfo_byweb(url, ou_token, param);
    if(ret < 0)
    {
        ayutil_del_host(AYENTRY_SERVER_PUBLIC);
    }
    return ret;
}

/********************************** Device Register to Entry Server *****************/
static int get_deviceid_byjson(const char* in_content, int in_content_len, char* pou_deviceid)
{
    ayJSON *pjson = NULL;
    ayJSON *pchild = NULL;
    ayJSON *pdata = NULL;
    if (in_content == NULL || pou_deviceid == NULL) {
        return -1;
    }
    DEBUGF("tokenjson: %s\n", in_content);
    pjson = ayJSON_Parse(in_content);
    if (!pjson) {
        return -1;
    }

    do 
    {
        pchild = ayJSON_GetObjectItem(pjson, "code");
        if(!pchild) {
            ERRORF("Register_FAIL|get code fail!\n");
            break;
        }
        sdk_get_handle(0)->devinfo.resp_json_code = pchild->valueint;

        pchild = ayJSON_GetObjectItem(pjson, "message");
        if(!pchild) {
            ERRORF("Register_FAIL|get message fail!\n");
            break;
        }

		pdata = ayJSON_GetObjectItem(pjson, "data");
		if(!pdata) {
			ERRORF("Register_FAIL|get data fail!\n");
			break;
		}

		pchild = ayJSON_GetObjectItem(pdata, "device_id");
		if (!pchild || !pchild->valuestring) {
			ERRORF("Register_FAIL|get did fail!\n");
			break;
		}
		if (pchild->valuestring) {
            strcpy(pou_deviceid, pchild->valuestring);
        }

		pchild = ayJSON_GetObjectItem(pdata, "dev_flag");
		if (!pchild || !pchild->valuestring) {
			ERRORF("Register_FAIL|get devflag fail!\n");
			break;
		}
		int devflag = atoi(pchild->valuestring);
		DEBUGF("device flag = %#x\n", devflag);

		ayJSON_Delete(pjson);
		return 0;
    } while (0);
    ayJSON_Delete(pjson);
    return -1;
}

static int device_register_byweb(const char* pin_url, char* pou_deviceid, Dev_SN_Info *param)
{
    char *pJonsContent = NULL;
    int json_len = 0,des_len = 0;
    char entry_token_url[2048] = {0};
    char plain_reginfo[512];
    char des_reginfo[512];
    char cipher_reginfo[1024];

    if (pin_url == NULL || param == NULL || pou_deviceid == NULL) {
        errorf("device_register_byweb error\n");
        return -1;
    }

    st_ay_dev_info *pdevinfo = &sdk_get_handle(0)->devinfo;
    memset(plain_reginfo, 0x0, sizeof(plain_reginfo));
    snprintf(plain_reginfo, sizeof(plain_reginfo), 
	    "sn=%s&oem_id=%u&oem_name=%s&factory=%s&mac=%s&model=%s&dev_type=%d&ch_num=%d&use_type=%d&dev_flag=%u",
	    param->SN, param->OEMID, param->OEM_name, param->Factory, param->MAC, param->Model,
	    (int)pdevinfo->dev_type, (int)pdevinfo->channelnum, (int)pdevinfo->use_type, get_device_flag(pdevinfo));
    tracef("reginfo: %s\n", plain_reginfo);

    des_len = sizeof(des_reginfo);
    if (DES_Encrypt((unsigned char*)plain_reginfo, strlen(plain_reginfo),
		(unsigned char*)_ANYAN_DEFAULT_KEY, 8, (unsigned char*)des_reginfo ,des_len, &des_len) < 0)
    {
		ERRORF("Register_FAIL|DES_Encrypt failed\n");
		return -1;
    }
    if (hex_encode((unsigned char*)des_reginfo, des_len, cipher_reginfo, sizeof(cipher_reginfo)) < 0)
    {
		ERRORF("Register_FAIL|hex_encode failed, %d\n", des_len);
		return -1;
    }

    memset(entry_token_url, 0x0, sizeof(entry_token_url));
    snprintf(entry_token_url, sizeof(entry_token_url), "%s?reginfo=%s",pin_url, cipher_reginfo);

    pJonsContent = request_webserver_content(entry_token_url, &json_len, json_info_decode);
    if (pJonsContent == NULL) {
		ERRORF("Register_FAIL|request_webserver_content:%s fail\n",entry_token_url);
		return -1;
    }

    if (get_deviceid_byjson(pJonsContent, json_len, pou_deviceid) == -1) {
		free(pJonsContent);
		ERRORF("Register_FAIL|get_deviceid_byjson fail!\n");
		return -1;
    }
    free(pJonsContent);
    return 0;
}

int ayentry_register_device(char* pou_deviceid,Dev_SN_Info *param)
{
    int ret = 0;
    char url[256];
    snprintf(url,sizeof(url), "http://%s/certify/reg", AYENTRY_SERVER_PUBLIC);
    ret = device_register_byweb(url, pou_deviceid, param);
    if (ret < 0) {
        ayutil_del_host(AYENTRY_SERVER_PUBLIC);
    }
    return ret;
}

int ay_request_reset_device(const char *devid,const char *dev_token)
{
    if (devid==NULL || dev_token==NULL)
	return -1;

    char request[1024]={0};
    char *pJonsContent = NULL;
    int json_len = 0;
    ayJSON *pjson,*pitem = NULL;

    snprintf(request,sizeof(request), "http://%s/device/reset?device_id=%s&device_token=%s",AYENTRY_SERVER_PUBLIC,devid,dev_token);
    DEBUGF("reset request: %s\n", request);

    pJonsContent = request_webserver_content(request, &json_len,json_info_decode);
    if (pJonsContent == NULL)
    {
		return -1;
    }
    DEBUGF("web content: %s\n", pJonsContent);

    pjson = ayJSON_Parse(pJonsContent);
    free(pJonsContent);pJonsContent = NULL;

    if (pjson == NULL)
    {
		return -4;
    }
    else
    {
		pitem = ayJSON_GetObjectItem(pjson,"code");
		if(pitem==NULL ||pitem->valueint != 0)
		{
			ayJSON_Delete(pjson);
			return -5;
		}
    }

    ayJSON_Delete(pjson);
    return 0;
}
