
#ifndef __HTTP_BASE64_H__
#define __HTTP_BASE64_H__

#include "typedefine.h"

char *http_base64_encode(const char *text);

int ZBase64Decode(const char* Data,int DataByte,unsigned char out_data[],int *OutByte);
int ZBase64Encode(const char *input, int in_len, char *output, int *out_len);

int anyan_device_udp_encry_1(char * buff,int buff_len);
int anyan_device_udp_decry_1(char * buff,int buff_len);
int anyan_device_tcp_encry_1(unsigned char * buf,int buf_len,unsigned char *pkey,int key_len);
int anyan_device_tcp_decry_1(unsigned char * buf, int buf_len,unsigned char *pkey, int key_len);

int hex_encode(unsigned char * buff,int buff_len,char out_buff[],int out_len);
int hex_decode(char buff[],int buff_len,unsigned char out_buff[],int * out_buff_len);

char * ay_base64_encode( const unsigned char * bindata, char * base64, int binlength );
int ay_base64_decode(const char * base64, unsigned char * bindata );

#endif /* HTTP_BASE64_H */
