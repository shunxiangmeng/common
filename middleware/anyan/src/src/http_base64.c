#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "http_base64.h"

const char b64_alphabet[65] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=" };

char * http_base64_encode(const char *text) {
    /* The tricky thing about this is doing the padding at the end,
     * doing the bit manipulation requires a bit of concentration only */
    char *buffer = NULL;
    char *point = NULL;
    int inlen = 0;
    int outlen = 0;

    /* check our args */
    if (text == NULL)
	return NULL;

    /* Use 'buffer' to store the output. Work out how big it should be...
     * This must be a multiple of 4 bytes */

    inlen = strlen( text );
    /* check our arg...avoid a pesky FPE */
    if (inlen == 0)
    {
	buffer = malloc(sizeof(char));
	buffer[0] = '\0';
	return buffer;
    }
    outlen = (inlen*4)/3;
    if( (inlen % 3) > 0 ) /* got to pad */
	outlen += 4 - (inlen % 3);

    buffer = malloc( outlen + 1 ); /* +1 for the \0 */
    memset(buffer, 0, outlen + 1); /* initialize to zero */

    /* now do the main stage of conversion, 3 bytes at a time,
     * leave the trailing bytes (if there are any) for later */

    for( point=buffer; inlen>=3; inlen-=3, text+=3 ) {
	*(point++) = b64_alphabet[ *text>>2 ]; 
	*(point++) = b64_alphabet[ (*text<<4 & 0x30) | *(text+1)>>4 ]; 
	*(point++) = b64_alphabet[ (*(text+1)<<2 & 0x3c) | *(text+2)>>6 ];
	*(point++) = b64_alphabet[ *(text+2) & 0x3f ];
    }

    /* Now deal with the trailing bytes */
    if( inlen ) {
	/* We always have one trailing byte */
	*(point++) = b64_alphabet[ *text>>2 ];
	*(point++) = b64_alphabet[ (*text<<4 & 0x30) |
	    (inlen==2?*(text+1)>>4:0) ]; 
	*(point++) = (inlen==1?'=':b64_alphabet[ *(text+1)<<2 & 0x3c ] );
	*(point++) = '=';
    }

    *point = '\0';

    return buffer;
}

static const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char * ay_base64_encode( const unsigned char * bindata, char * base64, int binlength )
{
    int i, j;
    unsigned char current;

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';
    return base64;
}

int ay_base64_decode(const char * base64, unsigned char * bindata )
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
	memset( temp, 0xFF, sizeof(temp) );
	for ( k = 0 ; k < 64 ; k ++ )
	{
	    if ( base64char[k] == base64[i] )
		temp[0]= k;
	}
	for ( k = 0 ; k < 64 ; k ++ )
	{
	    if ( base64char[k] == base64[i+1] )
		temp[1]= k;
	}
	for ( k = 0 ; k < 64 ; k ++ )
	{
	    if ( base64char[k] == base64[i+2] )
		temp[2]= k;
	}
	for ( k = 0 ; k < 64 ; k ++ )
	{
	    if ( base64char[k] == base64[i+3] )
		temp[3]= k;
	}

	bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
	    ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
	if ( base64[i+2] == '=' )
	    break;

	bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
	    ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
	if ( base64[i+3] == '=' )
	    break;

	bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
	    ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}

static inline int ZBase64DecodeLength(int src_length)
{
    return src_length/4*3;
}


int ZBase64Decode(const char* Data,int DataByte,unsigned char out_data[],int *OutByte)
{
    const char DecodeTable[] =
    {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,           0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          0, 0, 0, 0,//24
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,           0, 0, 0, 0, 0, 0, 0, 0, 0,//19
	62, // '+'
	0, 0, 0,
	63, // '/'
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
	0, 0, 0, 
	64,//=
	0,
	0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
	13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
	0, 0, 0, 0, 0, 0,
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
	39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
    };
    if (Data == NULL || OutByte == NULL)
    {
	return 0;
    }
    if(*OutByte < ZBase64DecodeLength(DataByte))
    {	    
	return 0;
    }

    int out_data_pos = 0;
    *OutByte = ZBase64DecodeLength(DataByte);
    int decode_pos = 0;
    unsigned char Tmp[4]={0};
    do
    {
	if (DataByte - decode_pos>=4)
	{
	    Tmp[0] = DecodeTable[(int)(*Data++)];
	    Tmp[1] = DecodeTable[(int)(*Data++)];
	    Tmp[2] = DecodeTable[(int)(*Data++)];
	    Tmp[3] = DecodeTable[(int)(*Data++)];

	    out_data[out_data_pos++] = ((Tmp[0]<<2)&0xFC) | ((Tmp[1]>>4)&0X03);
	    if( Tmp[2] == '@' )		
	    {
		break;
	    }

	    out_data[out_data_pos++] = ((Tmp[1]<<4)&0xF0) | ((Tmp[2]>>2)&0X0f);

	    if( Tmp[3] == '@')
	    {
		break;
	    }

	    out_data[out_data_pos++] = ((Tmp[2]<<6)&0XC0) | (Tmp[3]&0X3F);
	    decode_pos += 4;
	}
	else
	{
	    break;
	}
    }while(1);
    return out_data_pos;
}

static void base256to64(char c1, char c2, char c3, int padding, char *output)
{
    *output++ = b64_alphabet[c1>>2];
    *output++ = b64_alphabet[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)];
    switch (padding) 
    {
	case 0:
	    *output++ = b64_alphabet[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
	    *output = b64_alphabet[c3 & 0x3F];
	    break;
	case 1:
	    *output++ = b64_alphabet[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
	    *output = '=';
	    break;
	case 2:
	default:
	    *output++ = '=';
	    *output = '=';
	    break;
    }
}

#define BASE256_TO_BASE64_LEN(len)	(len * 4 / 3 + 3)
#define BASE64_TO_BASE256_LEN(len)	(len * 3 / 4)

int ZBase64Encode(const char *input, int in_len, char *output, int *out_len)
{
    const char *pi = input;
    char c1, c2, c3;
    int i = 0;
    char *po = output;

    if(!input || !output || !out_len)
    {
	return -1;
    }

    if(*out_len < BASE256_TO_BASE64_LEN(in_len))
    {
	return -1;
    }

    while (i < in_len) 
    {
	c1 = *pi++;
	++i;

	if (i == in_len) 
	{
	    base256to64(c1, 0, 0, 2, po);
	    po += 4;
	    break;
	} 
	else 
	{
	    c2 = *pi++;
	    ++i;

	    if (i == in_len) 
	    {
		base256to64(c1, c2, 0, 1, po);
		po += 4;
		break;
	    } 
	    else 
	    {
		c3 = *pi++;
		++i;
		base256to64(c1, c2, c3, 0, po);
	    }
	}

	po += 4;
    }

    *out_len = (int)(po - output);
    return 0;
}

int hex_encode(unsigned char * buff,int buff_len,char out_buff[],int out_len)
{
    int i = 0;

    if(buff_len<=0 || buff_len > 1024*16 || out_len < buff_len*2+1)
    {
	printf("hex encode:inlen = %d,out size = %d\n",buff_len,out_len);
	return -1;
    }
    out_buff[buff_len*2] = 0;
    for(; i<buff_len; ++i)
    {
	sprintf(out_buff+2*i,"%02x",buff[i]);
    }
    return 0;
}

static unsigned char get_hex_value(char c)
{
    unsigned char ret = 255;
    if ( c >='0'&& c <='9')
    {
	ret = c - '0';
    }
    else if (c>='A'&& c<='F')
    {
	ret = c - 'A' + 10;
    }
    else if (c>='a'&& c<='f')
    {
	ret = c - 'a' + 10;
    }

    return ret;
}

int hex_decode(char buff[],int buff_len, unsigned char out_buff[],int *out_buff_len)
{
    int i = 0;
    if (buff_len<=0 || buff_len%2 || buff_len > 1024*16 || *out_buff_len<buff_len/2)
    {
	printf("hex_decode inlen = %d,out size = %d\n",buff_len,*out_buff_len);
	return -1;
    }

    *out_buff_len = buff_len/2;
    for (i = 0; i<*out_buff_len; ++i)
    {
	unsigned char tmp1 = get_hex_value(buff[2*i]);
	unsigned char tmp2 = get_hex_value(buff[2*i+1]);

	if (tmp1 == 255 || tmp2 == 255)
	{
	    printf("hex_decode buff[%d] = %x,buff[%d] = %x\n",2*i,buff[2*i],2*i+1,buff[2*i+1]);
	    return -1;
	}

	out_buff[i] = tmp1*16+tmp2;
    }
    return 0;
}

/*
 * C=A^B
 * A=B^C
 */
/* 算法1的加密实现 */
int anyan_device_udp_encry_1(char * buff, int buff_len)
{
    int i = 0;
    int start_key_pos = 0;
    unsigned char   key = 0;
    unsigned char   algo = 1;

    do 
    {
	//产生随机key
	//srand((int)time(NULL));
	key = rand()%256;
	start_key_pos = 2+4+8-1;

	if(buff_len < start_key_pos)
	{
	    break;
	}
	/* 保存在flag的最高字节 */
	buff[start_key_pos] = key;
	key = key|0x01;

	for(i = 0; i<buff_len; ++i)
	{
	    if( i != 1 && i != start_key_pos)
	    {
		buff[i] = buff[i]^key;
		key = buff[i];
	    }
	}

	/* 加密成功后再置算法位 */
	algo <<= 3;
	buff[1] |= algo;

	return 0;
    } while (0);
    return -1;
}

/* 算法1的解密实现 */
int anyan_device_udp_decry_1(char * buff,int buff_len)
{
    unsigned char testkey;
    unsigned char get_algo;
    //	int device_id_len_pos = 0;
    int start_key_pos = 0;
    unsigned char key;
    int i = 0;
    do 
    {
	get_algo = buff[1];
	get_algo >>= 3;

	if (get_algo == 1)
	{
	    /* 设备ID长度所在的位置 */
	    start_key_pos = 2+4+8-1;

	    if(buff_len < start_key_pos)
	    {
		break;
	    }

	    /* 选定设备ID的最后一个字节为KEY */
	    key = buff[start_key_pos];
	    key = key|0x01;

	    for( i = 0; i<buff_len; ++i)
	    {
		if( i != 1 && i != start_key_pos)
		{
		    testkey = buff[i];
		    buff[i] = buff[i]^key;
		    key = testkey;
		}
	    }

	    buff[1] &= 0x07;//((~get_algo)|0x07);//0x7F;
	    buff[start_key_pos] &=0x0;
	}
	return 0;
    } while (0);
    return -1;
}

int anyan_device_tcp_encry_1(unsigned char * buf,int buf_len, unsigned char *pkey,int key_len)
{
    int i = 0;
    int start_key_pos = 0;
    int encry_len = 0;
    unsigned char key = 0;

    do 
    {
	if(buf == NULL || pkey == NULL)
	{
	    break;
	}
	if(buf_len > 128)
	    encry_len = 128;
	else
	    encry_len = buf_len;

	key = rand()%256;
	start_key_pos = 2+4+8-1;

	if(buf_len < start_key_pos)
	{
	    break;
	}

	//保存在flag的最高字节
	buf[start_key_pos] = key;
	key = key|0x01;

	for(i = 0; i < key_len; i++)
	{
	    pkey[i] = pkey[i]^key;
	    key = pkey[i];
	}
	for(i = 2; i < encry_len; ++i)
	{
	    if(i != start_key_pos)
	    {
		buf[i] = buf[i]^pkey[i%key_len];
	    }
	}
	return 0;
    } while (0);

    return -1;
}

int anyan_device_tcp_decry_1(unsigned char * buf,int buf_len,unsigned char * pkey,int key_len)
{
    int start_key_pos = 0;
    unsigned char key;
    int encry_len = 0;
    int i = 0;

    do 
    {
	if (buf == NULL || pkey == NULL)
	{
	    break;
	}
	//设备ID长度所在的位置
	start_key_pos = 2+4+8-1;

	if(buf_len < start_key_pos)
	{
	    break;
	}
	if (buf_len > 128)
	{
	    encry_len = 128;
	}
	else
	{
	    encry_len = buf_len;
	}

	//选定设备ID的最后一个字节为KEY
	key = buf[start_key_pos];
	key = key|0x01;

	for(i = 0; i<key_len; i++)
	{
	    pkey[i] = pkey[i]^key;
	    key = pkey[i];
	}

	for(i = 2; i < encry_len; ++i)
	{
	    if(i != start_key_pos)
	    {
		buf[i] = buf[i]^pkey[i%key_len];
	    }	 
	}

	buf[start_key_pos] &=0x0;

	return 0;
    } while (0);

    return -1;
}

