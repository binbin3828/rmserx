/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标准流程通用函数
**************************************************************
*/

#ifndef XXTEA_h
#define XXTEA_h

#include "stdlib.h"
#include "stdio.h"
#include <string.h>

#ifndef XXTEA_TYPE
#define XXTEA_TYPE int         // 一种32位长的数据类型，因int在32bit和64bit系统中都是32位的，故直接用int
#endif

class XXTEA {
public:
    XXTEA();
    ~XXTEA();

public:
    /**
     *	@brief     获得输出缓冲区的长度
     *
     *	@param 	nLength 	输入缓冲区长度
     *	@param 	isEncode 	TRUE 即为加密，FALSE即为解密
     *
     *	@return	返回所需长度
     */
    static size_t XXTEAGetBufferLength(size_t nLength, bool isEncode);

    /**
     *	@brief	对数据流进行加密
     *
     *	@param 	lpDstBuffer 	目标缓冲区
     *	@param 	lpSrcBuffer 	源缓冲区
     *	@param 	nLength 	源缓冲区长度
     *	@param 	lpKey 	密钥
     *
     *	@return	加密是否成功
     */
    static bool XXTEAEncode(char* lpDstBuffer, const char* lpSrcBuffer, size_t nLength, const XXTEA_TYPE* lpKey);

    /**
     *	@brief	对数据流进行解密
     *
     *	@param 	lpDstBuffer 	目标缓冲区
     *	@param 	lpSrcBuffer 	源缓冲区
     *	@param 	nLength 	源缓冲区长度
     *	@param 	lpKey 	密钥
     *
     *	@return	解密是否成功
     */
    static bool XXTEADecode(char* lpDstBuffer, const char* lpSrcBuffer, size_t nLength, const XXTEA_TYPE* lpKey);

private:
    /**
     *	@brief	XXTEA官方算法，此类即是对其进行封装，详见http://en.wikipedia.org/wiki/XXTEA
     *
     *	@param 	v 	加解密数据流
     *	@param 	n 	加解密长度，n > 1为加密，n < -1为解密
     *	@param 	k 	密钥      注意：形式参数k需和MX中的k对应
     *
     *	@return	返回0表示加解密成功，返回1表示失败
     */
    static long btea(XXTEA_TYPE* v, long n, const XXTEA_TYPE* k);

    /**
     *	@brief	正常字符串以16进制保存
     *
     *	@param 	surBuffer 	源缓冲区
     *	@param 	length 	源缓冲区长度
     *	@param 	dstBuffer 	目标缓冲区
     */
    static void StringToHex(const char* surBuffer, int length, char* dstBuffer);

    /**
     *	@brief	16进制字符串解析成正常字符串
     *
     *	@param 	surBuffer 	源缓冲区
     *	@param 	dstBuffer 	目标缓冲区
     *
     *	@return	目标缓冲区的长度
     */
    static int HexToString(const char* surBuffer,  char* dstBuffer);
};

#endif
