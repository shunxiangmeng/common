#include <string.h>
#include <stdio.h>

#include "protocol_exchangekey.h"
#include "cdh_crypt_lib.h"
#include "cdiffie_hell_man.h"
#include "protocol_device.h"
#include "define.h"

int write_dword(char *pbuf,int buf_len,DWORD mask)
{
    char *pPos = NULL;
    int mask_len = 0;
    do
    {
        pPos = pbuf;
        if (buf_len <= 0 || pPos == NULL || buf_len < sizeof(DWORD))
        {
            break;
        }

        *pPos++ =(char) (mask);
        *pPos++ =(char) (mask>>8);
        *pPos++ =(char) (mask>>16);
        *pPos++ =(char) (mask>>24);
        mask_len = sizeof(DWORD);

        return mask_len;
    }
    while(0);
    return -1;
}

static int read_dword(char *pbuf,uint16 buf_len)
{
    DWORD dw_value = 0;
    if (buf_len < sizeof(DWORD))
        return -1;

    dw_value = *(pbuf) + (*(pbuf + 1) << 8) + (*(pbuf + 2) << 16) + (*(pbuf + 3) << 24);
    return dw_value;
}

int  Pack_MsgExchangeKeyRequest(char *pbuf,int buf_len,struct ExchangeKeyRequest *_msgdata)
{
    int pack_data_len = 0;
    char *pPos = NULL;

    do
    {
        pPos = pbuf;
        if (buf_len <= 0 || pPos == NULL)
        {
            break;
        }
        pack_data_len += write_dword(pPos,buf_len,_msgdata->mask);
        if(pack_data_len > buf_len)
        {
            break;
        }
        pPos = pbuf + pack_data_len;

        //0x01
        if( 0x01 & _msgdata->mask )
        {
            memcpy(pPos, &_msgdata->keyPA_01.key_P_length, sizeof(_msgdata->keyPA_01.key_P_length));
            pack_data_len += sizeof(_msgdata->keyPA_01.key_P_length);
            pPos = pbuf + pack_data_len;
            if(pack_data_len > buf_len)
            {
                break;
            }
            if( _msgdata->keyPA_01.key_P_length && _msgdata->keyPA_01.key_P_length <= sizeof(_msgdata->keyPA_01.key_P) )
            {
                memcpy(pPos, &_msgdata->keyPA_01.key_P[0], sizeof(BYTE)*_msgdata->keyPA_01.key_P_length);
                pack_data_len += sizeof(BYTE)*_msgdata->keyPA_01.key_P_length;
                pPos = pbuf + pack_data_len;
                if(pack_data_len > buf_len)
                {
                    break;
                }
            }
            else
            {
                break;
            }

            memcpy(pPos, &_msgdata->keyPA_01.key_A_length, sizeof(_msgdata->keyPA_01.key_A_length));
            pack_data_len += sizeof(_msgdata->keyPA_01.key_A_length);
            pPos = pbuf + pack_data_len;
            if(pack_data_len > buf_len)
            {
                break;
            }
            if( _msgdata->keyPA_01.key_A_length && _msgdata->keyPA_01.key_A_length <= sizeof(_msgdata->keyPA_01.key_A) )
            {
                memcpy(pPos, &_msgdata->keyPA_01.key_A[0], sizeof(BYTE)*_msgdata->keyPA_01.key_A_length);
                pack_data_len += sizeof(BYTE)*_msgdata->keyPA_01.key_A_length;
                pPos = pbuf + pack_data_len;
                if(pack_data_len > buf_len)
                {
                    break;
                }
            }
            else
            {

                break;
            }
        }

        if( 0x02 & _msgdata->mask )
        {

            pack_data_len += sizeof(WORD);
            if(pack_data_len > buf_len)
            {
                break;
            }

            //			*(WORD*)pPos = _msgdata->except_algorithm;
            *pPos = (char)_msgdata->except_algorithm;
            pPos++;
            *pPos = (char)(_msgdata->except_algorithm >> 8);
            pPos++;

            //pack_data_len += write_dword(pPos,buf_len,_msgdata->except_algorithm);
            if (pack_data_len == -1)
            {
                break;
            }
        }

        return pack_data_len;
    }
    while (0);
    return -1;
}


int Make_Pack(uint32 cmdID,char *pPack_buf,int buf_len,char *pMsgbody,uint16 body_len)
{
    int head_len = 0;
    uint16 pack_len = 0;

    do
    {
        if(pPack_buf == NULL || pMsgbody == NULL)
        {
            break;
        }
        head_len = sizeof(uint16) + sizeof(uint32);
        if(buf_len < head_len || buf_len <= 0)
        {
            break;
        }

        pack_len = (uint16)(body_len + head_len);		//填充长度

        if(pack_len > buf_len)
        {
            break;
        }

        *(pPack_buf + 2)=(char)cmdID;//消息类型
        *(pPack_buf + 3)=(char)(cmdID >> 8);
        *(pPack_buf + 4)=(char)(cmdID >> 16);
        *(pPack_buf + 5)=(char)(cmdID >> 24);

        memcpy(pPack_buf + head_len, pMsgbody, body_len);
        //		*(uint16*)pPack_buf = pack_len;
        *pPack_buf = (char)pack_len;
        *(pPack_buf + 1) = (char)(pack_len >>8);

        return pack_len;//返回长度
    }
    while (0);

    return -1;
}

int Write_MsgExchangeKeyRequest(char *pMsg_buf,uint16 msg_len,DWORD socket_fd)
{
    struct ExchangeKeyRequest KeyReq;
    char *pKeyReq_body = NULL;
    int  pPack_len = 0;
    int  buf_len = 0;
    int body_len = 0;

    uint16 head_len = sizeof(uint16) + sizeof(uint32);
    do
    {
        buf_len = msg_len;
        if(buf_len < head_len || pMsg_buf == NULL)
        {
            ERRORF("exchpack req 5\n");
            break;
        }
        pKeyReq_body = pMsg_buf + 6;

        memset(&KeyReq,0,sizeof(struct ExchangeKeyRequest));
        KeyReq.mask = 0x01;
        KeyReq.mask |= 0x02;

        if(InitDiffieHellman(8, socket_fd) == -1)
        {
            ERRORF("exchpack req 4\n");
            break;
        }

        if(MakePrime(socket_fd) == -1)
        {
            ERRORF("exchpack req 3\n");
            break;
        }
        if(ComputesA(socket_fd) == -1)
        {
            break;
        }

        Printf_a_p(&KeyReq, socket_fd);
        KeyReq.except_algorithm = 1001;

        body_len = Pack_MsgExchangeKeyRequest(pKeyReq_body, buf_len - head_len, &KeyReq);
        if(body_len == -1)
        {
            ERRORF("exchpack req 1\n");
            break;
        }

        pPack_len = Make_Pack(CID_ExchangeKeyRequest,pMsg_buf,buf_len,pKeyReq_body,body_len);
        if(pPack_len == -1)
        {
            ERRORF("exchpack req 2\n");
            break;
        }
        return pPack_len;
    }
    while (0);
    return -1;
}

int Unpack_MsgExchangeKeyResponse(char *buf, uint16 len, struct ExchangeKeyResponse *pKey_Resp,DWORD socket_fd)
{
    char    *pos = buf;
    uint32   mask, err;
    uint32   cmdID=0;
    uint16   pack_len = 0;
    uint16   read_len = 0;

    do
    {
    	//aydebug_printf("ExchangeKeyR", buf, len);
        if(buf == NULL || len < 8 || pKey_Resp == NULL)
        {
            ERRORF("exch  err7\n");
            break;
        }
        pack_len = (*(buf + 1) << 8) + *(buf);
        pos += sizeof(uint16);

	if (pack_len > len )//如果长度大于数据长度 .则不能进行解析.否则会异常--bcg 20141229
        {
            ERRORF("exch  err6\n");
            break;
        }

        read_len += sizeof(uint16);
        cmdID = read_dword(pos, len);
        pos += sizeof(uint32);
        read_len += sizeof(uint32);

        mask = read_dword(pos, len);
        pos += sizeof(uint32);
        read_len += sizeof(uint32);

        if(mask & 0x01)
        {
            read_len += sizeof(BYTE);
            if (read_len > len)
            {
		ERRORF("exch  err5,cmdid:%u\n",cmdID);
                break;
            }
            pKey_Resp->keyB_01.key_B_length = *(BYTE*)pos;
            pos +=sizeof(BYTE);

            read_len += sizeof(BYTE) * pKey_Resp->keyB_01.key_B_length;
            if (read_len > len)
            {
            	ERRORF("exch  err4\n");
                break;
            }

            if( pKey_Resp->keyB_01.key_B_length && pKey_Resp->keyB_01.key_B_length <= sizeof(pKey_Resp->keyB_01.key_B) )
            {
                memcpy(&(pKey_Resp->keyB_01.key_B[0]),pos, pKey_Resp->keyB_01.key_B_length);
                pos +=sizeof(BYTE) * pKey_Resp->keyB_01.key_B_length;
            }
            read_len += sizeof(WORD);
            if (read_len > len)
            {
            	ERRORF("exch  err3\n");
                break;
            }
            pKey_Resp->keyB_01.key_size =  (*(pos + 1) << 8) + *(pos);
            pos +=sizeof(WORD);
        }

        if(mask & 0x02)//错误信息
        {
            read_len += sizeof(DWORD);
            if (read_len > len)
            {
            	ERRORF("exch  err2\n");
                break;
            }
            pos = (char *)Check_ID_Error((uint8*)pos,len - (pos - buf), &err);
            if(pos == NULL)
            {
            	ERRORF("exch  err1\n");
                break;
            }
            pKey_Resp->c3_error_no_02.encry_algorithm = (*(pos + 1) << 8) + *(pos);
        }
        return 0;
    }
    while (0);

    return -1;

}

int GetExchengeKey(struct ExchangeKeyResponse *pKey_Resp,ExchangeKeyValue *pKey,DWORD socket_fd)
{
    int buf_len = 0;
    do
    {
        if (pKey_Resp == NULL || pKey == NULL || socket_fd < 0)
        {
            break;
        }

        buf_len = sizeof(DWORD)*16;
        if(pKey_Resp->keyB_01.key_size > buf_len)
        {
            break;
        }

        Set_B(pKey_Resp,socket_fd);
        if(ComputesS1(socket_fd) == -1)
        {
            break;
        }

        pKey->key_size = pKey_Resp->keyB_01.key_size;

        Printf_S1(socket_fd,&pKey->Key[0],buf_len);

        return 0;
    }
    while (0);
    return -1;
}

