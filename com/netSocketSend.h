#ifndef __NETSOCKETSEND_H__
#define __NETSOCKETSEND_H__

/*JackFrost 2013.11.1 socket 发送数据类*/

//#include "../../../com/safeList.h"
#include "safeList.h"

typedef struct tagNetSocketSend:public tagSafeList
{
	int nSocketID;
	char *pData;
	int nLen;
	int nTagLen;
}NET_SOCKET_SEND_T;

class CNetSocketSend:public CSafeList
{
public:
	CNetSocketSend(int nNodeMax = 500);
	~CNetSocketSend();
	virtual int FreeData(SAFE_LIST_T *pSafeList);
private:
	int DelFrontNode();		//删除第一个
public:
	/*添加消息*/
	int PutData(int nfd,int nSocketID,char *pData,int nLen,int nReserve = 0);

	/*删除最后一个数据*/
	int DelLastNode();

	/*发送消息*/
	int Send(int nfd,int nSocketID,int &nSendClose);
};

#endif //__NETSOCKETSEND_H__
