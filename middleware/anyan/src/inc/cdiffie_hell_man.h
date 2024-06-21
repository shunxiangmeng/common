#ifndef __CDIFFIE_HELL_MAN_H__
#define __CDIFFIE_HELL_MAN_H__

#include "cdh_crypt_lib.h"
#include "protocol_exchangekey.h"


#define 	MAX_SOCET_FD       4

#pragma pack(1)
typedef struct
{
	UINT	size_;
	UINT	key_size_;
	DWORD	p_[16];
	DWORD	g_[16];
	DWORD	A_[16];
	DWORD	B_[16];
	DWORD	a_[16];
	DWORD	b_[16];
	DWORD	S1_[16];
	DWORD	S2_[16];
	DHCryptData dh_crypt_data;
	int   	socket_fd;
	UINT    use_key_size_;
	BOOL    connecting;
}DiffieHellManData;

typedef struct
{
	DWORD connect_num;
	DiffieHellManData _data[MAX_SOCET_FD];
}DiffieHellManDataList;

#pragma pack()

int InitDiffieHellman(UINT nSize, int socket_fd);
void InitDHDataList();
int MakePrime(DWORD socket_fd);
int ComputesA(DWORD socket_fd); 
int ComputesB(DWORD socket_fd); 
int ComputesS1(DWORD socket_fd); 
int ComputesS2(DWORD socket_fd); 

int Printf_a_p(struct ExchangeKeyRequest *pMsg,DWORD socket_fd);
void Set_B(struct ExchangeKeyResponse *pMsg,DWORD socket_fd);
int Printf_S1(DWORD socket_fd,DWORD *pdw_Buf,int buf_len);
int Get_size_(DWORD socket_fd);
int Get_exchangekey_(int socket_fd,char *pS_);
int Set_Connect_status(DWORD socket_fd,int status);

#endif //__DIFFIEHELLMAN_H__
