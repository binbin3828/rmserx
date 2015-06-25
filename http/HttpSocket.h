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

typedef list<TASKINFO*> TASKLIST;//TASKINFO����

class CHttpServer;
extern CHttpServer HttpServer;

//http���������
class CHttpServer
{
public:
	//��ʼhttp�������������� 0:�ɹ� -1:ʧ��
	int8 InitServer(int8 *lpHost, int8 *lpWebPathFile, int32 iIP,uint16 iPort,int16 iTh);

	//�������
	void AddTaskInfo(TASKINFO *lpTask);
	//��ȡ����
	TASKINFO* GetTaskInfo();
	//��ӿ������񻺴��
	void AddFreeTask(TASKINFO *lpTask);
	//��ȡ�������񻺴��
	TASKINFO* GetFreeTask();

	//http��������ص�
	static void* ThreadCall(void* lp);
	//http��������
	void PostHttpTask();

private:
	string     m_strHost;        //http post host��ַ
	string     m_strWebPathFile; //http post ���ļ�Ŀ¼
	uint32     m_iIP;            //http������������IP
	uint16     m_iPort;          //http������������Port
	uint32     m_iTask;          //��������
	TASKLIST   m_TaskList;       //�����б�
	CMutexLock m_TaskLock;       //�����б�����
	uint32     m_iFree;          //��������
	TASKLIST   m_FreeList;       //�����б�
	CMutexLock m_FreeLock;       //������𻥳���
};


#endif
