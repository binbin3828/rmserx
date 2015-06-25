#ifndef __LINUXTHREAD_H__
#define __LINUXTHREAD_H__

#include <pthread.h>

/* 对线程的封装，方便调用*/
typedef int (*THREAD_BACK)(void *args);

//创建线程
//pThread 线程ID
//callBackFun 回调函数
//args 返回指针
int CreateThread(pthread_t *pThread, THREAD_BACK callBackFun, void *args);

//获得当前线程ID
unsigned int GetCurrentThreadID();

//等待线程退出
void JoinThread(pthread_t *pThread);

//释放线程句柄
void CloseThread(pthread_t *pThread);

#endif //__LINUXTHREAD_H__
