#include "UdpBusiness.h"

#include "../com/common1.h"
#include "../com/autoLock.h"


extern vector<dealer *>  dealers_vector; //所有房间荷官存储队列
int32 iUDPMax = 1024;

//client业务处理
void* ClientBusiness(void* lp)
{
	int8* lpBuf = new int8[iUDPMax];
	if(NULL == lpBuf)
		return lp;
	CUDPServer* lpServer = (CUDPServer*)lp;
	int32 iSock = lpServer->Sock();
	int32 iRecv = 0,iCmd = 0;
	sockaddr_in RecvAddr;
	socklen_t  AddrLen;
	while(1)
	{
		//不可读
		if(0 != lpServer->IsRead())
		{
			usleep(1000);//微妙
			continue;
		}
		//recvfrom socket
		AddrLen = sizeof(sockaddr_in);
		iRecv = recvfrom(iSock,lpBuf,iUDPMax,0,(sockaddr*)&RecvAddr,&AddrLen);
		if(iRecv == iUDPMax)//超出最大长度
			continue;
		else if(iRecv <= 0)//无可接收的数据
			continue;
		cout<< "guobin: lpBuf" << lpBuf << endl;
		
		//解析数据		
		Json::Value json_msg;
		Json::Reader reader;
		if(!reader.parse(lpBuf,json_msg))
			continue;
		
		printf("recv %d data:%s\n",iRecv,lpBuf);

        try
		{
            iCmd = json_msg["cmd"].asInt();
        }
		catch(...)
        {
             continue;
        }

		printf("cmd:%d \n",iCmd);

		switch (iCmd)
		{
			case UDP_MANAGE_CMD_KICKING_PLAYER_OUT:
				handler_manage_kicking_player_out(json_msg);
				break;
			default:
				break;
		}
	}

	delete lpBuf;
	lpBuf = NULL;
	return lp;
}

//////////////////////////////////////////////////

//发送数据
void SendToAddr(sockaddr_in* lpAddr,int8* lpBuf,uint16 iLen)
{
	static socklen_t AddrLen = sizeof(sockaddr_in);
	sendto(ClientServer.Sock(),lpBuf,iLen,0,(sockaddr*)lpAddr,AddrLen);

}

void handler_manage_kicking_player_out(Json::Value json_msg)
{
	int uid;
	try{
		uid = atoi( (json_msg["uid"].asString()).c_str());
	}
	catch(...)
	{
		printf("handler_manage_kicking_player_out, rev message uid error.\n");
		return;
	}	
	//在线玩家
	if (NetGetNetUserCount() <= 0)
	{
		printf("handler_manage_kicking_player_out, no online user.");
		return;
	}
	SAFE_HASH_LIST_T *pList = NetGetListUser();
	if (!pList)
	{
		printf("handler_manage_kicking_player_out, pList is NULL.");
		return;
	}
	DZPK_USER_T *pUser = NULL;
	DZPK_USER_T *pPlayer = NULL;
	for (int32 i=0; i<pList->nSize; i++)
	{
		pUser = (DZPK_USER_T *)pList->pTable[i];
		if (pUser && uid == pUser->nUserID)
		{
			printf("handler_manage_kicking_player_out, find the userid:%d .\n", pUser->nUserID);
			pPlayer = pUser;
			break;
		}
	}

	//找不到玩家
	if (pPlayer == NULL)
	{
		printf("handler_manage_kicking_player_out, can not find uid:%d\n",uid);
		return;
	}

	//找到玩家,处理踢人
	int room_num = pUser->pUser->room_num;
	if (room_num == 0) 
	{
		printf("handler_manage_kicking_player_out, player not in table.\n");
		return;
	}
	printf("room_num:%d, nick_name:%s, admin_kick_flag:%d\n", room_num, pUser->pUser->nick_name.c_str(),pUser->pUser->admin_kick_flag);

	
	//服务端踢人标志置 1
	
	for (unsigned int i=0; i< dealers_vector.size(); i++ )
	{
		if (dealers_vector[i]->dealer_id == room_num)
		{
			
			class CAutoLock Lock(&dealers_vector[i]->Mutex);
			Lock.Lock();
			for (unsigned int j=0; j<dealers_vector[i]->players_list.size(); j++)
			{
				if (dealers_vector[i]->players_list[j].nUserID == uid)
				{
					printf("handler_manage_kicking_player_out, find it.\n");
					dealers_vector[i]->players_list[j].admin_kick_flag = 1;
				}
			}
			break;
		}
	}
	
	printf("handler_manage_kicking_player_out, admin_kick_flag = %d.\n", pPlayer->pUser->admin_kick_flag);
	
	NetFreeListUser(pList);		
}


