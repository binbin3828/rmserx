#include "HttpSocket.h"

CHttpServer HttpServer;

int8 lpPostHead[] = "POST %s HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Shockwave Flash\r\nConnection: Close\r\nContent-type:  application/octet-stream\r\nHost: %s\r\nContent-Length: %d\r\n\r\n%s";
int8 lpPostBody[] = "{\"account\":\"%s\",\"user_id\":\"%s\",\"add_chip\":\"%u\",\"nick_name\":\"%s\",\"room_name\":\"%s\"}\r\n\r\n";

//int8 lpPostBody[] = "{\"req\":{\"t\":\"kuwoapi_chiptocoinauto\"},\"ctx\":{\"chan\":\"0\",\"account\":\"%s\",\"user_id\":\"%s\",\"sessionid\":\"d5a5ae3571a3a473cb931fa05fd9dc78\",\"ter\":\"WEB\",\"mver\":\"1\",\"serial\":\"47588\"}}\r\n\r\n";
//{\"req\":{\"t\":\"kuwoapi_chiptocoinauto\"},\"ctx\":{\"chan\":\"0\",\"account\":\"%s\",\"user_id\":\"%s\",\"sessionid\":\"d5a5ae3571a3a473cb931fa05fd9dc78\",\"ter\":\"WEB\",\"mver\":\"1\",\"serial\":\"47588\"}}
//"http://203.195.138.89:80/hallroom/jsonServlet"
//http://192.168.1.120/douniu/1.jsp
//http://192.168.1.120/hallroom/jsonServlet
//int8 lpGetHead[] = "GET /douniu/1.jsp HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-cn\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\nHost: %s\r\nConnection: Close\r\n\r\n";
//int8 lpPostHead[] = "POST /hallroom/jsonServlet HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Shockwave Flash\r\nConnection: Close\r\nContent-type:  application/octet-stream\r\nHost: %s\r\nContent-Length: %d\r\n\r\n%s";
//int8 lpPostHead[] = "POST /httpRevDemo/rev.php HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Shockwave Flash\r\nConnection: Close\r\nContent-type:  application/octet-stream\r\nHost: %s\r\nContent-Length: %d\r\n\r\n%s";

////////////////////////////////////////////////
//初始http发送任务服务程序 0:成功 -1:失败
int8 CHttpServer::InitServer(int8 *lpHost, int8 *lpWebPathFile, int32 iIP,uint16 iPort,int16 iTh)
{
	m_strHost = lpHost;
	m_strWebPathFile = lpWebPathFile;
	m_iIP = iIP;
	m_iPort = iPort;
	m_iTask = 0;
	m_iFree = 0;	
	
	pthread_t th;
	for(int16 i = 0;i < iTh;++i)
	{
		if(0 != pthread_create(&th,NULL,ThreadCall,this))
			return -1;
	}

	return 0;
}


//添加任务
void CHttpServer::AddTaskInfo(TASKINFO *lpTask)
{
	m_TaskLock.Lock();
	m_TaskList.push_back(lpTask);
	++m_iTask;
	m_TaskLock.Unlock();
}

//获取任务
TASKINFO* CHttpServer::GetTaskInfo()
{
	TASKINFO* lpTask = NULL;
	if(m_iTask)
	{
		TASKLIST::iterator it;
		m_TaskLock.Lock();
		it = m_TaskList.begin();
		if(it != m_TaskList.end())
		{
			lpTask = *it;
			m_TaskList.erase(it);
			--m_iTask;
		}
		m_TaskLock.Unlock();
	}
	return lpTask;
}

//添加空闲任务缓存块
void CHttpServer::AddFreeTask(TASKINFO *lpTask)
{
	m_FreeLock.Lock();
	m_FreeList.push_back(lpTask);
	++m_iFree;
	m_FreeLock.Unlock();
}

//获取空闲任务缓存块
TASKINFO* CHttpServer::GetFreeTask()
{
	TASKINFO* lpTask = NULL;
	if(m_iFree)
	{
		TASKLIST::iterator it;
		m_FreeLock.Lock();
		it = m_FreeList.begin();
		if(it != m_FreeList.end())
		{
			lpTask = *it;
			m_FreeList.erase(it);
			--m_iFree;
		}
		m_FreeLock.Unlock();
	}

	if(NULL == lpTask)
		lpTask = new TASKINFO();

	return lpTask;
}

//http发送任务回调
void* CHttpServer::ThreadCall(void* lp)
{
	((CHttpServer*)lp)->PostHttpTask();
	return lp;
}

//http发送任务
void CHttpServer::PostHttpTask()
{
	sockaddr_in RemoteAddr;
	socklen_t RemoteAddrLen = sizeof(sockaddr_in);
	RemoteAddr.sin_family = AF_INET;
	RemoteAddr.sin_addr.s_addr = m_iIP;
	RemoteAddr.sin_port = m_iPort;
		
	TASKINFO *lpTask = NULL;
	int32 iSock = -1,iSize = 0,iBody = 0;
	int8 lpBuf[4096],lpBody[2048];
	//string strBody = "";
	while(1)
	{
		lpTask = GetTaskInfo();
		if(NULL == lpTask)
		{
			usleep(1000);//微妙
			continue;
		}
		iSock = socket(AF_INET,SOCK_STREAM,0);
		if(iSock > 0)
		{
			
			iBody = sprintf(lpBody,lpPostBody,lpTask->strAcct.c_str(),lpTask->strID.c_str(),lpTask->addChip,lpTask->nickName.c_str(),lpTask->roomName.c_str());			
			//strBody = base64_encode((uint8 *)lpBody,iBody);
			iSize = sprintf(lpBuf,lpPostHead,m_strWebPathFile.c_str(),m_strHost.c_str(),iBody,lpBody);
			if(0 == connect(iSock,(sockaddr*)&RemoteAddr,RemoteAddrLen))
			{
				printf("connect HttpServer ok\r\n");
				unsigned int i_Size = -2;
				i_Size = send(iSock,lpBuf,iSize,0);
				printf("lpBuf:\r\n%s\r\n",lpBuf);
				printf("send ok PostData ok  sendSize = %u, buffSize=%u\r\n",i_Size,iSize);				
			}
			close(iSock);
		}
		AddFreeTask(lpTask);
	}
}

///////////////////////////////////////////////


