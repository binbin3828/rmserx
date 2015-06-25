#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <netinet/in.h> //sockaddr_in
#include <netinet/tcp.h> //TCP_NODELAY
#include <stdio.h>

#include <pthread.h>
#include "../com/common1.h"


class CUDPServer;
extern CUDPServer ClientServer;

class CUDPServer
{
public:
	//初始化程序 0:成功 -1:失败
	int8 InitServer(uint16 iPort,int32 iThread);
	void WaitListen();
	static void* ThreadCall(void* lp);
	
	//是否可读 0:可读 -1:不可读
	int8 IsRead();
	int32 Sock() { return m_iSock; }

private:
	int32  m_iSock;
	int32  m_iEpoll;
	int32  m_iRead;//可读次数
	int32  m_iThread;//处理线程数
	CMutexLock m_ReadLock;//可读互斥锁
};

#endif
