/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克用户管理
**************************************************************
*/

#include "dzpkUser.h"
#include "standard/custom.h"
#include "mainDzpk.h"
#include "Start.h"

/*添加socket用户*/
int DzpkAddSocketMallocBack(int nfd,int nSocketID,int nPort,CNetSocketRecv *pRecv,CNetSocketSend *pSend)
{
	int nRet = -1;
	DZPK_USER_T *pSocket =(DZPK_USER_T *)NetSocketUserMalloc(sizeof(DZPK_USER_T));
	DZPK_USER_INFO_T *pUser = new DZPK_USER_INFO_T();
	if(pSocket && pUser)
	{
		pSocket->nfd = nfd;
		pSocket->nSocketID = nSocketID;
		pSocket->nPort = nPort;
		pSocket->listSend = pSend;
		pSocket->listRecv = pRecv;

		pSocket->nRoomID = -1;
		pSocket->nUserID = -1;
		pSocket->nSendClose= 0;
		pSocket->nRecFireWallMark = 0;

		pSocket->pUser = pUser;

		char szName[100];
		memset(szName,0,sizeof(szName));
		sprintf(szName,"socket:%d dzpk rec ",nSocketID);
		pRecv->SetName(szName);
		sprintf(szName,"socket:%d dzpk Send ",nSocketID);
		pSend->SetName(szName);

		if(NetAddUserToSocket(nSocketID,pSocket,sizeof(DZPK_USER_T)) == 0)
		{
			return 0;
		}
		else
		{
			NetFreeSocketUser((void*)pSocket);
		}
	}
	else
	{
		if(pSocket)
		{
			pSocket->nfd = nfd;
			pSocket->nSocketID = nSocketID;
			pSocket->nPort = nPort;
			pSocket->listSend = pSend;
			pSocket->listRecv = pRecv;

			pSocket->nRoomID = -1;
			pSocket->nUserID = -1;
			NetFreeSocketUser((void*)pSocket);
		}

		if(pUser)
		{
			delete pUser;
			pUser = NULL;
		}
	}
	return nRet;
}

/*socket 断开*/
int DzpkCloseSocketBack(int nSocket,void *pUser,int nType)
{
	return 0;
}

/*释放用户内存*/
void DzpkHashRealFreeDataBack(void *ptr)
{
	DZPK_USER_T *pSocket =(DZPK_USER_T *)ptr;
	if(pSocket)
	{
		if(pSocket->pUser)
		{
			delete pSocket->pUser;
			pSocket->pUser = NULL;
		}
	}
}
