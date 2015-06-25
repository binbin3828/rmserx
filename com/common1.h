#ifndef _COMMON1_H_
#define _COMMON1_H_

#include <pthread.h>

//互斥锁类
class CMutexLock
{
public:
	CMutexLock()    { pthread_mutex_init(&m_MutexLock,NULL); }
	~CMutexLock()   { pthread_mutex_destroy(&m_MutexLock); }
	int   Lock()    { return pthread_mutex_lock(&m_MutexLock); }
	int   Unlock()  { return pthread_mutex_unlock(&m_MutexLock); }
	int   Trylock() { return pthread_mutex_trylock(&m_MutexLock); }
		
private:
	pthread_mutex_t   m_MutexLock;
};

struct MUTEXLOCK
{
private:
	CMutexLock *m_lpLock;
public:
	MUTEXLOCK(CMutexLock *lpLock):m_lpLock(lpLock)
	{
		m_lpLock->Lock();
	}

	~MUTEXLOCK()
	{
		m_lpLock->Unlock();
	}
};

typedef char           int8;
typedef unsigned char  uint8;
typedef short          int16;
typedef unsigned short uint16;
typedef int            int32;
typedef unsigned int   uint32;
typedef long long      int64;
typedef unsigned long long uint64;
typedef void *funcall (void *lpPa); //funcall是返回类型为void指针的函数指针
//#define ntohll(x)	__bswap_64 (x)
//#define htonll(x)	__bswap_64 (x)


#endif

