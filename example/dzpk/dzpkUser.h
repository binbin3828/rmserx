
#ifndef __DZPKUSER_H__
#define __DZPKUSER_H__

#include "mainDzpk.h"

/*JackFrost 2013.11.13  德州扑克用户管理*/

/*添加socket用户*/
int DzpkAddSocketMallocBack(int nfd,int nSocketID,int nPort,CNetSocketRecv *pRecv,CNetSocketSend *pSend);

/*关闭socket用户*/
int DzpkCloseSocketBack(int nSocket,void *pUser,int nType);

/*释放用户内存*/
void DzpkHashRealFreeDataBack(void *ptr);


#endif //__DZPKUSER_H__
