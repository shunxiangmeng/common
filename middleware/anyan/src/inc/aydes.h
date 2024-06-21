#ifndef __AY_DES_H__
#define __AY_DES_H__

#define ANYAN_DES_CIPHER_ERROR -1 
#define ANYAN_DES_OK 0  

#define _ANYAN_DEFAULT_KEY "Ulucu888"
typedef char ElemType;


int DES_Encrypt(unsigned char *pPlainData, int iPlainDataLength, 
	unsigned char *pKeyStr, int iKeyLength, 
	unsigned char *pCipherData, int iCipherMaxSize,
	int*   piCipherSize);  
int DES_Decrypt(unsigned char *pCipherData, int iCipherDataLength, 
	unsigned char *pKeyStr, int iKeyLength, 
	unsigned char *pPlainData, int iPlainMaxSize,
	int*   piPlainSize); 

#endif
