/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  Sha1.h
 * Author      :  mengshunxiang 
 * Data        :  2024-04-13 18:33:05
 * Description :  None
 * Note        : 
 ************************************************************************/
#pragma once
#include <stddef.h>	// for size_t
#include <stdint.h>

class CSha1{
	// 不允许外部调用拷贝构造函数
	CSha1(const CSha1& other);
public:
	// 默认构造函数
	CSha1();

	//析构函数
	~CSha1();

	// 重载的赋值操作符
	CSha1& operator=(const CSha1& other);

	///\brief Sha1初始化
	void Init();

	///\brief 更新消息
	///\param [in] message-消息内容
	///\param [in] length-消息的长度
	///\return 0-ok, -1-failed	
	int Update(const uint8_t* message, uint32_t length);

	///\brief 获取摘要的结果
	///\param [out] output-获取到的摘要结果
	void Final(uint32_t output[5]);

private:

	///\brief 当m_message 数据达到512bits时，用于更新摘要值
	void Core();

private:
	uint32_t						m_hash[5];		// 存储更新的哈希值数组
	uint32_t						m_message[16];	// 存储512 bits的消息长度，超过部分循环覆盖
	uint32_t						m_length;		// m_message有效消息的长度，不会超过64 bytes
	uint64_t						m_num;			// 消息的总长度，最长不超过2的64次方bits
};
