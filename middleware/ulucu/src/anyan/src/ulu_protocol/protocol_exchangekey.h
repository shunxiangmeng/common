#ifndef __PROTOCOL_EXCHANGEKEY_H__
#define __PROTOCOL_EXCHANGEKEY_H__

#include "protocol_device.h"

enum {
	EExchangekey_Rijndael = 101,
	EExchangekey_Blowfish = 102,
};

#pragma pack (1)

typedef struct
{
	DWORD	Key[16];
	WORD	key_size;
}ExchangeKeyValue;

//ExchangeKeyRequest 0x01
typedef struct  
{
	BYTE	key_P_length;
	BYTE	key_P[64];
	BYTE	key_A_length;
	BYTE	key_A[64];
}ExchangeKeyRequest_01;

struct ExchangeKeyRequest
{
	DWORD	mask;
	//0X01
	ExchangeKeyRequest_01 keyPA_01;
	//0X02
	WORD	except_algorithm;
};

//ExchangeKeyResponse 0x01
typedef struct
{
	BYTE	key_B_length;
	BYTE	key_B[64];
	WORD	key_size;
}ExchangeKeyResponse_01;

typedef struct
{
	c3_error_t error;
	WORD	encry_algorithm;
}ExchangeKeyResponse_02;

//ExchangeKeyResponse 0x02
struct ExchangeKeyResponse
{
	DWORD	mask;
	//0x01
	ExchangeKeyResponse_01 keyB_01;
	//0x02
	ExchangeKeyResponse_02 c3_error_no_02;
};
#pragma pack ()


int Write_MsgExchangeKeyRequest(char *pMsg_buf,uint16 msg_len,DWORD socket_fd);
int Unpack_MsgExchangeKeyResponse(char *buf, uint16 len,struct ExchangeKeyResponse *pKey_Resp,DWORD socket_fd);
int GetExchengeKey(struct ExchangeKeyResponse *pKey_Resp,ExchangeKeyValue *pKey,DWORD socket_fd);



#endif //__PROTOCOL_EXCHANGEKEY_H__

