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

//clientҵ����
void* ClientBusiness(void* lp);

//��������
void SendToAddr(sockaddr_in* lpAddr,int8* lpBuf,uint16 iLen);
//�����̨����ָ��
void handler_manage_kicking_player_out(Json::Value json_message);



#endif
