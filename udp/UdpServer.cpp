#include "UdpServer.h"

CUDPServer ClientServer;

//初始化程序 0:成功 -1:失败
int8 CUDPServer::InitServer(uint16 iPort,int32 iThread)
{
	m_iSock = socket(AF_INET,SOCK_DGRAM,0);
	if(m_iSock <= 0)
	{
		printf("socket SOCK_DGRAM failed!\n");
		return -1;
	}
	//非阻塞
	if(-1 == fcntl(m_iSock,F_SETFL,O_NONBLOCK | fcntl(m_iSock,F_GETFL,0)))
	{
		printf("fcntl O_NONBLOCK failed!\n");
		return -1;
	}
	//绑定端口
	socklen_t AddrLen = sizeof(sockaddr_in);
	sockaddr_in LocalAddr;
	LocalAddr.sin_family = AF_INET;
	LocalAddr.sin_addr.s_addr = INADDR_ANY;
	LocalAddr.sin_port = iPort;
	if(-1 == bind(m_iSock,(sockaddr*)&LocalAddr,AddrLen))
	{
		printf("bind failed!\n");
		return -1;
	}
	//epoll
	m_iEpoll = epoll_create(1);
	if(m_iEpoll <= 0)
	{
		printf("epoll create failed!\n");
		return -1;
	}
	epoll_event Event;
	Event.data.fd = m_iSock;
	Event.events  = EPOLLIN;//EPOLLET | 
	if(-1 == epoll_ctl(m_iEpoll,EPOLL_CTL_ADD,m_iSock,&Event))
	{
		printf("EPOLL_CTL_ADD failed!\n");
		return -1;
	}

	m_iRead = 0;
	m_iThread = iThread;
	pthread_t th;
	if(0 != pthread_create(&th,NULL,ThreadCall,this))
	{
		printf("thread create failed!\n");
		return -1;
	}

	return 0;
}

void CUDPServer::WaitListen()
{
	epoll_event Event;
	while(1)
	{
		if(epoll_wait(m_iEpoll,&Event,1,1000) > 0)
		{
			m_ReadLock.Lock();
			if(++m_iRead > m_iThread)
				m_iRead = m_iThread;
			m_ReadLock.Unlock();
		}
	}
}

void* CUDPServer::ThreadCall(void* lp)
{
	((CUDPServer*)lp)->WaitListen();
	return lp;
}

//是否可读 0:可读 -1:不可读
int8 CUDPServer::IsRead()
{
	int8 iRet = -1;
	m_ReadLock.Lock();
	if(m_iRead)
	{
		--m_iRead;
		iRet = 0;
	}
	m_ReadLock.Unlock();
	return iRet;
}

