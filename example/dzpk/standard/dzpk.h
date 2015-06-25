#ifndef READYTOPLAY_H_INCLUDED
#define READYTOPLAY_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>
#include <vector>
#include "../include/json/json.h"
#include "custom.h"

/*标准版本*/

using namespace  std;

#define DEAL_ADD_CHIP_TYPE_USER		1		//用户调用
#define DEAL_ADD_CHIP_TYPE_TIMER	2		//定时器调用
#define DEAL_ADD_CHIP_TYPE_EXITROOM	3		//退出房间调用
#define DEAL_ADD_CHIP_TYPE_LEAVESEAT	4		//离座调用
#define DEAL_ADD_CHIP_TYPE_FORCELEAVE	5		//异常弃牌
#define DEAL_RESULT_TYPE_FORCELEAVE 	6		//只处理结果

void deal_enter_room( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_seat_down( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_add_chip( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead,int nType);

void deal_speek_words( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_leave_seat( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_display_over( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_exit_room( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_quick_start( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_present_gift( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_present_chips( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_add_friend( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_heart_wave( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_get_standby( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  lay_hand_cards( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  reset_hand_chips( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  get_level_gift( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_level_gift( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void  deal_count_down( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_prohibit_user( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_prohibit_user_vote( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

void deal_re_read_pack( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);
/*发送消息给房间用户*/
int deal_send_to_room(int nUserID,int nRoomdID,char *pBuf,int nBufLen,int nType);

/*玩牌前强迫用户退出房间*/
int CheckForceUserExitRoom(dealer *pRoom,int nUserID,int nType,char *pTemp1 = NULL,char *pTemp2 = NULL);

/*投票*/
int ProhibitUserVote(dealer *pRoom,int nUserID,int nResult,int nRecordID,int nTimer = 0);

/*进入获得投票详细信息*/
int ProhibitUserGetInfo(dealer *pRoom,int nUserID);

/*强迫用户退出房间*/
int ForceUserExitRoom(int nRoomNumber,int nUserID,int nType);

/* 管理员强迫用户退出房间 */
int CheckAdminForceUserExitRoom(dealer *pRoom, int nUserID);


/*给荷官小费*/
int dealGiveTipToDealer(int nRoomdID, DZPK_USER_T *pUser, string seat_num);


#endif // READYTOPLAY_H_INCLUDED
