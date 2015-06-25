#ifndef _BUSINESS_H_
#define _BUSINESS_H_

#include <arpa/inet.h>
#include "UdpServer.h"
#include "../com/mainNet.h"
#include "../example/dzpk/include/json/json.h"
#include "../example/dzpk/standard/custom.h"
#include "../example/dzpk/mainDzpk.h"


#define UDP_MANAGE_CMD_KICKING_PLAYER_OUT  1


extern int32 iUDPMax;

//client业务处理
void* ClientBusiness(void* lp);

//发送数据
void SendToAddr(sockaddr_in* lpAddr,int8* lpBuf,uint16 iLen);
//处理后台踢人指令
void handler_manage_kicking_player_out(Json::Value json_message);



#endif
