#ifndef _HTTPSOCKET_H_
#define _HTTPSOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h> //sockaddr_in
#include <netinet/tcp.h> //TCP_NODELAY
#include <string.h>
#include <stdio.h>
#include <list>
#include <string>

#include "../com/common1.h"
#include "../com/base64.h"

using namespace std;

struct TASKINFO
{
	string strAcct;
	string strID;
	uint32 addChip;
	string nickName;
	string roomName;
};

typedef list<TASKINFO*> TASKLIST;//TASKINFO队列

class CHttpServer;
extern CHttpServer HttpServer;

//http任务服务器
class CHttpServer
{
public:
	//初始http发送任务服务程序 0:成功 -1:失败
	int8 InitServer(int8 *lpHost, int8 *lpWebPathFile, int32 iIP,uint16 iPort,int16 iTh);

	//添加任务
	void AddTaskInfo(TASKINFO *lpTask);
	//获取任务
	TASKINFO* GetTaskInfo();
	//添加空闲任务缓存块
	void AddFreeTask(TASKINFO *lpTask);
	//获取空闲任务缓存块
	TASKINFO* GetFreeTask();

	//http发送任务回调
	static void* ThreadCall(void* lp);
	//http发送任务
	void PostHttpTask();

private:
	string     m_strHost;        //http post host地址
	string     m_strWebPathFile; //http post 的文件目录
	uint32     m_iIP;            //http服务器网络序IP
	uint16     m_iPort;          //http服务器网络序Port
	uint32     m_iTask;          //任务数量
	TASKLIST   m_TaskList;       //任务列表
	CMutexLock m_TaskLock;       //任务列表互斥锁
	uint32     m_iFree;          //空闲数量
	TASKLIST   m_FreeList;       //空闲列表
	CMutexLock m_FreeLock;       //空闲类别互斥锁
};


#endif
