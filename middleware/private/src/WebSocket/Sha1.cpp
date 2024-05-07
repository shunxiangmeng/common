/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Sha1.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:33:16
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "infra/include/Utils.h"
#include "Sha1.h"
#include "infra/include/Logger.h"

// 实现四字节整形按字节转序
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define BSWAP32(x) infra::ntohl(x)
#else
#define BSWAP32(x) (x)
#endif

// 循环左移
#define S1(X)  ((X << 1)  | (X >> 31))
#define S5(X)  ((X << 5)  | (X >> 27))
#define S30(X) ((X << 30) | (X >> 2))

// 按照RFC3174 第五章规范定义
// f(t;B,C,D) = (B AND C) OR ((NOT B) AND D)         ( 0 <= t <= 19)
// f(t;B,C,D) = B XOR C XOR D                        (20 <= t <= 39)
// f(t;B,C,D) = (B AND C) OR (B AND D) OR (C AND D)  (40 <= t <= 59)
// f(t;B,C,D) = B XOR C XOR D                        (60 <= t <= 79)
#define f0(B,C,D) ((B & C) | (~B & D))
#define f1(B,C,D) (B ^ C ^ D)
#define f2(B,C,D) ((B & C) | (B & D) | (C & D))
#define f3(B,C,D) (B ^ C ^ D)

#define SHA_K0	0x5A827999		// K(t) = 5A827999 		( 0 <= t <= 19)
#define SHA_K1	0x6ED9EBA1		// K(t) = 6ED9EBA1		(20 <= t <= 39)
#define SHA_K2	0x8F1BBCDC		// K(t) = 8F1BBCDC		(40 <= t <= 59)
#define SHA_K3	0xCA62C1D6		// K(t) = CA62C1D6		(60 <= t <= 79)

CSha1::CSha1() : m_length(0), m_num(0){
	for(uint32_t i = 0; i < sizeof(m_hash)/sizeof(m_hash[0]); i++){
		m_hash[i] = 0;
	}
	for(uint32_t i = 0; i < sizeof(m_message)/sizeof(m_message[0]); i++){
		m_message[i] = 0;
	}
}

CSha1::~CSha1() {
}

CSha1& CSha1::operator=(const CSha1& other){
	if(this == &other){
		return *this;
	}
	else{
		m_length = other.m_length;
		m_num = other.m_num;
		for(uint32_t i = 0; i < sizeof(m_hash)/sizeof(m_hash[0]); i++){
			m_hash[i] = other.m_hash[i];
		}
		for(uint32_t i = 0; i < sizeof(m_message)/sizeof(m_message[0]); i++){
			m_message[i] = other.m_message[i];
		}
		return *this;
	}
}

void CSha1::Init() {
	// 初始消息摘要
	m_hash[0] = 0x67452301;
	m_hash[1] = 0xefcdab89;
	m_hash[2] = 0x98badcfe;
	m_hash[3] = 0x10325476;
	m_hash[4] = 0xc3d2e1f0;
	m_length = 0;
	m_num = 0;
}

int CSha1::Update(const uint8_t* message, uint32_t length){
	uint8_t* buf = (uint8_t*)m_message;
	if(message == NULL || length == 0){
		errorf("Buffer is NULL.\n");
		return -1;
	}
	// 更新消息的比特数
	m_num += ((uint64_t)length * 8);
	// 将更新的消息全部拷贝到m_message, 如果64字节长度不够，需要对m_message做一次摘要,
	// 将结果保存在m_hash中，并将超出部分的数据从m_message起始覆盖
	while (length > 0){
		if(length + m_length >= 64){
			length -= (64 - m_length);
			for(int i = m_length; i < 64; i++){
				buf[i] = *message++;
			}
			m_length = 0;
			Core();
		}
		else{
			for(uint32_t i = m_length; i < (m_length + length); i++){
				buf[i] = *message++;
			}
			m_length += length;
			length = 0;
		}
	}
	return 0;
}

void CSha1::Final(uint32_t output[5]) {
	uint8_t* buf = (uint8_t*)m_message;
	// 数据末尾添加一个字节0x80
	buf[m_length++] = 0x80;				
	// 后面字节填充0x00
	for(uint32_t i = m_length; i < 64; i++){
		buf[i] = 0x00;
	}
	// 数据的最末尾需要添加总bit数，占64bits，因此buffer中的长度大于56字节时，
	// 需要多计算一次
	if(m_length <= 56){
		m_message[14] = BSWAP32((uint32_t)(m_num>>32));
		m_message[15] = BSWAP32((uint32_t)m_num);
		Core();
	}
	else{
		Core();
		for(uint32_t i = 0; i < 15; i++){
			m_message[i] = 0x00;
		}		
		m_message[14] = BSWAP32((uint32_t)(m_num>>32));
		m_message[15] = BSWAP32((uint32_t)m_num);
		Core();
	}
	output[0] = BSWAP32(m_hash[0]);
	output[1] = BSWAP32(m_hash[1]);
	output[2] = BSWAP32(m_hash[2]);
	output[3] = BSWAP32(m_hash[3]);
	output[4] = BSWAP32(m_hash[4]);
	m_length = 0;
	m_num = 0;
	return;
}

void CSha1::Core() {
	uint32_t A, B, C, D, E, TEMP, W[80];
	uint32_t t;
   	// Divide M(i) into 16 words W(0), W(1), ... , W(15), where W(0)is the left-most word.
	for( t = 0; t < 16; t++){
		W[t] = BSWAP32(m_message[t]);
	}
	// For t = 16 to 79 let 		
	//     W(t) = S^1(W(t-3) XOR W(t-8) XOR W(t-14) XOR W(t-16)).
	for(t = 16; t < 80; t++){
		TEMP = W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16];
		W[t] = S1(TEMP);      
	}
	// Let A = H0, B = H1, C = H2, D = H3, E = H4.
	A = m_hash[0];
	B = m_hash[1];
	C = m_hash[2];
	D = m_hash[3];
	E = m_hash[4];
	// For t = 0 to 79 do
	// 	  TEMP = S^5(A) + f(t;B,C,D) + E + W(t) + K(t);
	//    E = D;  D = C;  C = S^30(B);  B = A; A = TEMP;
	for(t = 0; t < 80; t++){
		switch(t/20){
			case 0:
				TEMP = S5(A) + f0(B, C, D) + E + W[t] + SHA_K0;
				break;
			case 1:
				TEMP = S5(A) + f1(B, C, D) + E + W[t] + SHA_K1;
				break;
			case 2:
				TEMP = S5(A) + f2(B, C, D) + E + W[t] + SHA_K2;
				break;
			case 3:
				TEMP = S5(A) + f3(B, C, D) + E + W[t] + SHA_K3;
				break;
			default:
				break;
		}
		E = D;
		D = C;
		C = S30(B);
		B = A;
		A = TEMP;
	}
	// Let H0 = H0 + A, H1 = H1 + B, H2 = H2 + C, H3 = H3 + D, H4 = H4 + E.
	m_hash[0] += A;
	m_hash[1] += B;
	m_hash[2] += C;
	m_hash[3] += D;
	m_hash[4] += E;
	return;
}
