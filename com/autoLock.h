/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标准流程通用函数
**************************************************************
*/


#ifndef __AUTOLOCK_H__
#define __AUTOLOCK_H__

#include "mutex.h"

/* 自动解锁类，一般只在函数里实例化*/
class CAutoLock
{
public:
	CAutoLock(class CMutex * pMute);
	CAutoLock(class CRwMutex  * pMute);
	~CAutoLock();
private:
	int nType;
	void *m_pMute;
	bool m_bLock;
public:
	int m_nFileLine;		//文件行数
	char m_szFileName[100];		//文件名

	//互斥锁调用(如果是读写锁误调用，调用WriteLock函数)
	void Lock(const char *file = __FILE__, int line = __LINE__);
	int TryLock(const char *file = __FILE__, int line = __LINE__);

	//读写锁调用
	void ReadLock(const char *file = __FILE__, int line = __LINE__);
	void WriteLock(const char *file = __FILE__, int line = __LINE__);

	//互斥锁和读写锁都可以调用（也可以不用调用，在析构函数会自动调用）
	void Unlock();
};

#endif //__AUTOLOCK_H__
