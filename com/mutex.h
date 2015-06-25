/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标准流程互斥锁
**************************************************************
*/

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <pthread.h>

/*互斥锁*/
class CMutex
{
public:
	CMutex();
	~CMutex();
	void Lock();
	int Trylock();
	void Unlock();
	pthread_mutex_t *GetMutex();

private:
	pthread_mutex_t m_mutex;
};

/* 读写锁*/
class CRwMutex
{
public:
    CRwMutex();
    ~CRwMutex();
    void ReadLock();
    void WriteLock();
    void Unlock();

private:
    pthread_rwlock_t m_rwl;
};

#endif  // __MUTEX_H__

