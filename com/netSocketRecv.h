#ifndef __NETSOCKETRECV_H__
#define __NETSOCKETRECV_H__

//#include "../../../com/safeList.h"
#include "safeList.h"

/*JackFrost 2013.11.1 socket 接收数据类*/

typedef struct tagNetSocketRecv:public tagSafeList
{
	int nSocketID;
	int nMsgID;
	char *pData;
	int nLen;
	int nTagLen;
}NET_SOCKET_RECV_T;

class CNetSocketRecv:public CSafeList
{
public:
	CNetSocketRecv(int nNodeMax = 500);
	~CNetSocketRecv();
	virtual int FreeData(SAFE_LIST_T *pSafeList);
public:
	/*添加消息*/
	int PutData(int nSocketID,int nMsgID,char *pData,int nLen,int nReserve = 0);

	/*取消息*/
	int GetData(NET_SOCKET_RECV_T *pRecv,int nLen);
};

#endif //__NETSOCKETRECV_H__
