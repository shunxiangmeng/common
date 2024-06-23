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
#include "monitor.h"
#include "aydevice2.h"

static int aydevice2_encode_info(char* pin_Content,int in_len,char pou_Content[],int buf_len)
{
    unsigned char *plain_info = (unsigned char *)pin_Content;
    char des_info[512];
    int des_len = sizeof(des_info);

    if(DES_Encrypt((unsigned char*)plain_info, in_len, (unsigned char*)_ANYAN_DEFAULT_KEY, 8, 
		(unsigned char*)des_info ,des_len, &des_len) < 0)
    {
	ERRORF("DES_Encrypt failed\n");
	return -1;
    }
    if(hex_encode((unsigned char*)des_info, des_len, pou_Content,buf_len) < 0)
    {
	ERRORF("hex_encode failed, %d\n", des_len);
	return -1;
    }
    return 0;
}

static int aydevice2_decode_json(const char* pin_Content,int in_len,char pou_Content[],int buf_len)
{
    char *pb64_buf = NULL;
    int decode_len = 0,ret = 0;
    int decode_buflen = 0;

    pb64_buf = (char*)malloc(in_len+1);
    if (!pb64_buf)
    {
	ERRORF("decode json malloc %d err1\n",in_len+1);
	return -1;
    }

    decode_buflen = in_len;
    memset(pb64_buf, 0, in_len + 1);

    ret = hex_decode((char*)pin_Content,in_len, (unsigned char*)pb64_buf,&decode_buflen);
    if (ret < 0)
    {		
	ERRORF("hex decode ret = %d\n",ret);
	free(pb64_buf);
	return -1;
    }
    if(DES_Decrypt((uint8*)pb64_buf, decode_buflen, (uint8*)_ANYAN_DEFAULT_KEY, 8,(uint8*)pou_Content, buf_len,&decode_len) != ANYAN_DES_OK)
    {
	ERRORF("DES_Decrypt err3\n");
	free(pb64_buf);
	return -1;
    }

    free(pb64_buf);
    return decode_len;
}

static int get_token_byjson(const char* in_content,int in_content_len,entry_token* ou_token)
{
    ayJSON *pjson = NULL;
    ayJSON *pchild = NULL;
    ayJSON *pdata = NULL;

    pjson=ayJSON_Parse(in_content);
    if (!pjson) { return -1; }

    DEBUGF("token msg:%s\n",in_content);
    do {
	pchild = ayJSON_GetObjectItem(pjson,"code");
	if(!pchild)
	{
	    ERRORF("get token code fail!\n");
	    break;
	}
	ou_token->icode = pchild->valueint;
	sdk_get_handle(0)->devinfo.resp_json_code = pchild->valueint;

	pchild = ayJSON_GetObjectItem(pjson,"message");
	if(!pchild)
	{
	    ERRORF("get token message fail!\n");
	    break;
	}

	if(pchild->valuestring) strcpy(ou_token->szmessage,pchild->valuestring);
	pdata = ayJSON_GetObjectItem(pjson,"data");
	if(!pdata)
	{
	    ERRORF("get token data fail!\n");
	    break;
	}

	pchild = ayJSON_GetObjectItem(pdata,"device_id");
	if (!pchild || !pchild->valuestring)
	{
	    ERRORF("get token did fail!\n");
	    break;
	}

	//DEBUGF(" device_id: %s\n",pchild->valuestring);
	if(pchild->valuestring)strcpy(ou_token->data.device_id,pchild->valuestring);

	ou_token->data.status = pchild->valueint;

	pchild = ayJSON_GetObjectItem(pdata,"token");
	if (!pchild || !pchild->valuestring)
	{
	    ERRORF("get token token fail!\n");
	    break;
	}
	if(pchild->valuestring)strcpy(ou_token->data.token,pchild->valuestring);

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

/* {“code”:0,“message”:“success”,“data”:“Ys000000E0F820F411ON”} */
static int get_data_byjson(const char* in_content,int in_content_len,char* pcontent)
{
    ayJSON *pjson = NULL;
    ayJSON *pchild = NULL;
    ayJSON *pdata = NULL;

    if(in_content == NULL || pcontent == NULL)
    {
	return -1;
    }
    DEBUGF("json: %s\n",in_content);
    pjson=ayJSON_Parse(in_content);
    if (!pjson)
    {
	return -1;
    }

    do {
	pchild = ayJSON_GetObjectItem(pjson,"code");
	if(!pchild)
	{
	    ERRORF("get json code fail!\n");
	    break;
	}
	sdk_get_handle(0)->devinfo.resp_json_code = pchild->valueint;
	if(pchild->valueint != 0)
	{
	    break;
	}

	pchild = ayJSON_GetObjectItem(pjson,"message");
	if(!pchild)
	{
	    ERRORF("get json message fail!\n");
	    break;
	}

	pdata = ayJSON_GetObjectItem(pjson,"data");
	if(!pdata)
	{
	    ERRORF("get json data fail!\n");
	    break;
	}

	if(pdata->valuestring) strcpy(pcontent,pdata->valuestring);

	ayJSON_Delete(pjson);
	return 0;
    } while (0);
    ayJSON_Delete(pjson);
    return -1;
}

int aydevice2_get_20sn(Dev_SN_Info *param,char *pou_deviceid)
{
    char *pJonsContent = NULL;
    int json_len = 0;
    char url[2048] = {0};
    char plain_info[256];
    char cipher_info[1024];

    if (param == NULL || pou_deviceid == NULL)
    {
	return -1;
    }

    snprintf(plain_info, sizeof(plain_info),"oem_sn=%s&sn_head=%s", param->SN, param->OEM_name);
    DEBUGF("info: %s\n",plain_info);

    if(aydevice2_encode_info(plain_info,strlen(plain_info),cipher_info,sizeof(cipher_info))<0)
	return -1;

    snprintf(url, sizeof(url), "http://%s/sn/get?info=%s", AYDEVICE2_SERVER_SN,cipher_info);

    pJonsContent = request_webserver_content(url, &json_len,aydevice2_decode_json);
    if (pJonsContent == NULL)
    {
	ERRORF("request_webserver_content:%s fail\n",url);
	return -1;
    }

    if(get_data_byjson(pJonsContent, json_len, pou_deviceid) == -1)
    {
	free(pJonsContent);
	ERRORF("get_data_byjson fail!\n");
	return -1;
    }
    free(pJonsContent);
    return 0;
}

int aydevice2_login(const char *sn20,int min_rate,int max_rate)
{
    char *pJonsContent = NULL;
    int json_len = 0;
    char plain_info[256];
    char url[2048] = {0};
    char cipher_info[1024];

    if (sn20 == NULL) return -1;

    snprintf(plain_info, sizeof(plain_info),"sn=%s&rate=%d,%d", sn20, min_rate,max_rate);
    DEBUGF("login info: %s\n",plain_info);

    if(aydevice2_encode_info(plain_info,strlen(plain_info),cipher_info,sizeof(cipher_info))<0)
	return -1;

    snprintf(url, sizeof(url), "http://%s/device/login?info=%s",AYDEVICE2_SERVER_PUBLIC, cipher_info);

    pJonsContent = request_webserver_content(url, &json_len,aydevice2_decode_json);
    if (pJonsContent == NULL)
    {
	ERRORF("request_webserver_content:%s fail\n",url);
	return -1;
    }

    char deviceid[32]={0};
    if(get_data_byjson(pJonsContent, json_len, deviceid) == -1)
    {
	free(pJonsContent);
	ERRORF("get_data_byjson fail!\n");
	return -1;
    }
    free(pJonsContent);
    return 0;
}

int aydevice2_get_token(const char *sn20,entry_token *token)
{
    char *pJonsContent = NULL;
    int json_len = 0;
    char plain_info[256];
    char url[2048] = {0};
    char cipher_info[1024];

    if (sn20 == NULL) return -1;

    snprintf(plain_info, sizeof(plain_info),"sn=%s", sn20);
    DEBUGF("login info: %s\n",plain_info);

    if(aydevice2_encode_info(plain_info,strlen(plain_info),cipher_info,sizeof(cipher_info))<0)
	return -1;

    snprintf(url, sizeof(url), "http://%s/device/token?info=%s", AYDEVICE2_SERVER_PUBLIC,cipher_info);

    pJonsContent = request_webserver_content(url, &json_len,aydevice2_decode_json);
    if (pJonsContent == NULL)
    {
	ERRORF("request_webserver_content:%s fail\n",url);
	return -1;
    }
#if 1
    memset(token,0,sizeof(entry_token));
    if(get_token_byjson(pJonsContent, json_len, token) == -1)
    {
	free(pJonsContent);
	ERRORF("get device token fail!\n");
	return -1;
    }
#else
    char data[256]={0};
    if(get_data_byjson(pJonsContent, json_len, data) == -1)
    {
	free(pJonsContent);
	DEBUGF("get device token fail!\n");
	return -1;
    }
    DEBUGF("token data:%s\n",data);
#endif
    
    free(pJonsContent);
    return 0;
}

static int post_webserver_content(const char *purl,char *body,int body_len)
{
    int bytes = 0;
    ghttp_status req_status;
    ghttp_request *pReq = NULL;
    char *pWebContent = NULL;

    if(purl == NULL ||body==NULL || NULL==(pReq=ghttp_request_new()))
    {
	ERRORF("ghttp_request_new fail!\n");
	return -1;
    }

    if (ghttp_set_uri(pReq,(char*)purl) < 0)
    {
	ERRORF("ghttp_set_uri:%s fail!\n",purl);
	ghttp_request_destroy(pReq);
	return -1;
    }

    ghttp_set_sync(pReq, ghttp_async);
    ghttp_set_type(pReq, ghttp_type_post);
    ghttp_set_body(pReq, body,body_len);
    ghttp_prepare(pReq);

    do {
	req_status = ghttp_process(pReq);
	if (req_status == ghttp_error)
	{
	    ERRORF("ghttp err: %s\n", ghttp_get_error(pReq));
	    ghttp_request_destroy(pReq);
	    return -1;
	}
    }while (req_status == ghttp_not_done);

    bytes = ghttp_get_body_len(pReq);
    if(bytes>0 && (pWebContent=ghttp_get_body(pReq)))
    {
	DEBUGF("post reslt:%s\n",pWebContent);
    }

    ghttp_request_destroy(pReq);
    return bytes;
}

int aydevice2_upload_log(const char *log)
{
    int ret = -1;

    if(log==NULL) return -1;

    char url[128]={0};
    snprintf(url,sizeof(url),"http://%s/stat.html",AYDEVICE2_SERVER_LOG);
    tracef("~~~~~~body:%s\n",log);
    ret = post_webserver_content(url,(char*)log,strlen(log));

    if(ret >= 0) 
    {
	ret = 0;
    }
    else 
    {
	ret = -1;
    }

    return ret;
}

/* {“code”:0,“message”:“success”,“data”:“base64encode data”} */
static int get_dnslist_byjson(const char* in_content,int in_content_len,st_ay_net_addr ips[],int max)
{
    ayJSON *pjson = NULL;
    ayJSON *pchild = NULL;
    ayJSON *pdata = NULL;
    if (in_content == NULL) {
        return -1;
    }
    DEBUGF("dnslist json: %s\n",in_content);
    pjson=ayJSON_Parse(in_content);
    if (!pjson) {
        return -1;
    }

    do {
        pchild = ayJSON_GetObjectItem(pjson,"code");
        if (!pchild) {
            ERRORF("get dnslist json code fail!\n");
            break;
        }
        if (pchild->valueint != 0) {
            ERRORF("dnslist json code = %d!\n", pchild->valueint);
            break;
        }

        pchild = ayJSON_GetObjectItem(pjson, "msg");
        if (!pchild) {
            ERRORF("get dnslist json message fail!\n");
            break;
        }

        pdata = ayJSON_GetObjectItem(pjson, "data");
        if (!pdata) {
            ERRORF("get json data fail!\n");
            break;
        }

        if (pdata->valuestring) {
            int len;
            char data[1024] = {0};
            len = ay_base64_decode(pdata->valuestring, (uint8*)data);
            if (len > 0) {
                tracef("data:%s\n", data);
                ayJSON *pitem = ayJSON_Parse(data);
                int num = 0;
                if (pitem && (num = ayJSON_GetArraySize(pitem)) > 0) {
                    for (int i = 0; i < num && i < max; i++) {
                        pchild = ayJSON_GetArrayItem(pitem,i);
                        if (pchild && pchild->valuestring) {
                            tracef("ip[%d]:%s\n", i,pchild->valuestring);
                            if (strstr(pchild->valuestring,":"))  {
                                sscanf(pchild->valuestring, "%[0-9.]:%hu", ips[i].ipstr, &ips[i].port);
                            } else  {
                                strcpy(ips[i].ipstr,pchild->valuestring);
                                ips[i].port = 80;
                            }
                        }
                    }
                    ayJSON_Delete(pitem);
                } else {
                    ERRORF("dnslist json parse:%s fail!\n",data);
                    break;
                }
            } else {
                ERRORF("dnslist base64decode fail!\n");
                break;
            }
	    }
        ayJSON_Delete(pjson);
        return 0;
    } while (0);
    ayJSON_Delete(pjson);
    return -1;
}

void aydevice2_query_dnslit(int arg)
{
    int json_len = 0, fail_flag = 0;
    char url[1024]={0};
    char fname[256]={0};
    char *pJonsContent = NULL;
    int i = 0,start = rand()%AY_MAX_DNSIP_NUM;
    st_ay_net_addr ip;
    st_ay_dns_ctrl *pdnsctrl = &sdk_get_handle(0)->dnsctrl;

    snprintf(fname, sizeof(fname), "%s/ay_dns", sdk_get_handle(0)->devinfo.rw_path);
    snprintf(url, sizeof(url), "http://%s:%hu/dnslist?sdk_version=%d", AYDEVICE2_SERVER_DNSLIST, 80, Ulu_SDK_Get_Version());
    DEBUGF("dns url:%s\n",url);
    pJonsContent = request_webserver_content(url, &json_len, NULL);
    if (pJonsContent == NULL) {
        ERRORF("GetDNSList_FAIL|request_webserver_content:%s fail\n",url);
        fail_flag = 1;
    } else if (get_dnslist_byjson(pJonsContent, json_len, pdnsctrl->dnsips, AY_MAX_DNSIP_NUM) == -1) {
        free(pJonsContent);
        ERRORF("GetDNSList_FAIL|get dnslist from json fail!\n");
        fail_flag = 1;
    } else {
        free(pJonsContent);
        memcpy(&pdnsctrl->dnsok, &pdnsctrl->dnsips[0], sizeof(st_ay_net_addr));
        ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path, "ay_dns", (const char *)pdnsctrl, sizeof(st_ay_dns_ctrl));
    }

    if (fail_flag)
    {
        if (ayutil_read_file(sdk_get_handle(0)->devinfo.rw_path, "ay_dns", (char*)pdnsctrl, sizeof(st_ay_dns_ctrl)) < 0)
        {
            strcpy(pdnsctrl->dnsips[0].ipstr,"139.224.20.59");  // Ali shanghai
            pdnsctrl->dnsips[0].port = 80;
            strcpy(pdnsctrl->dnsips[1].ipstr,"101.201.61.249"); // Ali beijing
            pdnsctrl->dnsips[1].port = 80;
            strcpy(pdnsctrl->dnsips[2].ipstr,"203.195.145.109");// QQ guangzhou
            pdnsctrl->dnsips[2].port = 80;
        }
        for (i = 0; i < AY_MAX_DNSIP_NUM; i++)
        {
            start = (start+i)%AY_MAX_DNSIP_NUM;
            if (strlen(pdnsctrl->dnsips[start].ipstr) > 0)
            {
                snprintf(url, sizeof(url), "http://%s:%hu/dnslist?sdk_version=%d", 
                    pdnsctrl->dnsips[start].ipstr, pdnsctrl->dnsips[start].port, Ulu_SDK_Get_Version());
                pJonsContent = request_webserver_content(url, &json_len,NULL);
                if (pJonsContent == NULL)
                {
                    ERRORF("GetDNSList_FAIL|request_webserver_content:%s fail\n", url);
                    continue;
                }
                if (get_dnslist_byjson(pJonsContent, json_len, pdnsctrl->dnsips, AY_MAX_DNSIP_NUM) == -1)
                {
                    free(pJonsContent);
                    ERRORF("GetDNSList_FAIL|get dnslist from json fail!\n");
                    continue;
                }
                free(pJonsContent);
                memcpy(&pdnsctrl->dnsok, &pdnsctrl->dnsips[start], sizeof(st_ay_net_addr));
                ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path, "ay_dns", (const char *)pdnsctrl, sizeof(st_ay_dns_ctrl));
                fail_flag = 0;
                break;
            }
        }
    }
    if (fail_flag) {
        return ;
    }
#if 0
    printf("====== ulu dns ok ip:%s,%hu ======\n",pdnsctrl->dnsok.ipstr,pdnsctrl->dnsok.port);
    if(aydevice2_nslookup_host(AYENTRY_SERVER_CONFIG,&ip)==0)
    {
	strcpy(pdnsctrl->host[0].name,AYENTRY_SERVER_CONFIG);
	memcpy(&pdnsctrl->host[0].lastip,&ip,sizeof(ip));
	printf("--- name:%s,ip:%s ---\n",pdnsctrl->host[0].name,pdnsctrl->host[0].lastip.ipstr);
    }
    if(aydevice2_nslookup_host(AYENTRY_SERVER_PUBLIC,&ip)==0)
    {
	strcpy(pdnsctrl->host[1].name,AYENTRY_SERVER_PUBLIC);
	memcpy(&pdnsctrl->host[1].lastip,&ip,sizeof(ip));
	printf("--- name:%s,ip:%s ---\n",pdnsctrl->host[1].name,pdnsctrl->host[1].lastip.ipstr);
    }
    if(aydevice2_nslookup_host(AYDEVICE2_SERVER_LOG,&ip)==0)
    {
	strcpy(pdnsctrl->host[2].name,AYDEVICE2_SERVER_LOG);
	memcpy(&pdnsctrl->host[2].lastip,&ip,sizeof(ip));
	printf("--- name:%s,ip:%s ---\n",pdnsctrl->host[2].name,pdnsctrl->host[2].lastip.ipstr);
    }
    if(aydevice2_nslookup_host("trackers.anyan.com",&ip)==0)
    {
	strcpy(pdnsctrl->host[3].name,"trackers.anyan.com");
	memcpy(&pdnsctrl->host[3].lastip,&ip,sizeof(ip));
	printf("--- name:%s,ip:%s ---\n",pdnsctrl->host[3].name,pdnsctrl->host[3].lastip.ipstr);
    }
    if(aydevice2_nslookup_host(AYDEVICE2_SERVER_PICYUN,&ip)==0)
    {
	char content[256]={0};
	snprintf(content,sizeof(content),"%s:%hu",ip.ipstr,ip.port);
	ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path,"PicYunIP",(const char *)content,strlen(content));
    }
    if(aydevice2_nslookup_host(AYDEVICE2_SERVER_UPYUN,&ip)==0)
    {
	char content[256]={0};
	snprintf(content,sizeof(content),"%s:%hu",ip.ipstr,ip.port);
	ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path,"UPYunIP",(const char *)content,strlen(content));
    }
#endif
    if (aydevice2_nslookup_host(AYDEVICE2_SERVER_DEVENTRY, &ip) == 0)
    {
        char content[256]={0};
        strcpy(pdnsctrl->host[4].name, AYDEVICE2_SERVER_DEVENTRY);
        memcpy(&pdnsctrl->host[4].lastip, &ip, sizeof(ip));
        tracef("--- name:%s,ip:%s ---\n",pdnsctrl->host[4].name,pdnsctrl->host[4].lastip.ipstr);
        snprintf(content, sizeof(content), "%s:%hu", pdnsctrl->host[4].lastip.ipstr,pdnsctrl->host[4].lastip.port);
        ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path, "EDServIP", (const char *)content, strlen(content));
    }
    return;
}

int aydevice2_nslookup_host(const char *host,st_ay_net_addr *ip)
{
    if(host==NULL || ip==NULL) return -1;
    if(strlen(sdk_get_handle(0)->dnsctrl.dnsok.ipstr)==0) return -1;

    int json_len = 0;
    char url[1024]={0};
    char *pJonsContent = NULL;
    st_ay_net_addr addr[10];

    snprintf(url, sizeof(url), "http://%s:%hu/nslookup?business_channel=%s&sdk_version=%d", 
	    sdk_get_handle(0)->dnsctrl.dnsok.ipstr,sdk_get_handle(0)->dnsctrl.dnsok.port,host,Ulu_SDK_Get_Version());
    pJonsContent = request_webserver_content(url, &json_len,NULL);
    if (pJonsContent == NULL)
    {
	ERRORF("{{DeviceCommEvent}}GetDNSList_FAIL|request_webserver_content:%s fail\n",url);
	return -1;
    }
    memset(addr,0,sizeof(addr));
    if(get_dnslist_byjson(pJonsContent, json_len, addr,10) == -1)
    {
	free(pJonsContent);
	ERRORF("{{DeviceCommEvent}}GetDNSList_FAIL|get host %s from json fail!\n",host);
	return -1;
    }
    free(pJonsContent);
    memcpy(ip,&addr[0],sizeof(addr[0]));
    return 0;
}

int aydevice2_update_domainip(const char *domain,const char *file)
{
    char fname[256];
    st_ay_net_addr ip;

    if(domain==NULL || file==NULL)
	return -1;

    snprintf(fname,sizeof(fname),"%s/%s",sdk_get_handle(0)->devinfo.rw_path,file);
    if(access(fname,0) == 0)
	return 0;
    else if(aydevice2_nslookup_host(domain,&ip)==0)
    {
	char content[256]={0};
	snprintf(content,sizeof(content),"%s:%hu",ip.ipstr,ip.port);
	ayutil_save_file(sdk_get_handle(0)->devinfo.rw_path,(char*)file,(const char *)content,strlen(content));
	if(access(fname,0)) 
	{
	    return -2;
	}
	return 0;
    }
    return -1;
}

