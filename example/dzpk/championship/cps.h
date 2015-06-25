/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：
**************************************************************
*/

#ifndef __CPS_H__
#define __CPS_H__

#include <vector>
#include "../standard/custom.h"
#include "../include/json/json.h"
#include "cpsRecord.h"

/*锦标赛流程*/

#define CPS_MIN_ROOM_NUMBER     4       //开始最少人数 正常为4
#define CPS_INIT_ROOM_NUMBER    6       //初始房间人数 正常为6
#define CPS_LAST_ROOM_NUMBER    9       //决赛最多几个用户 正常为9
#define CPS_ROOM_MAX_NUMBER     9       //房间最多能容纳多少用户
#define CPS_MAX_ROOM_USER       500     //房间最多可进多少用户
#define CPS_ADD_ONE_BLIND       5      //一次涨多少盲注
#define CPS_ADD_BLIND_TIME      2*60      //涨盲注间隔
#define CPS_INIT_HAND_SHIP      1000   //初始筹码
#define CPS_INIT_SMALL_BLIND    10      //初始盲注

#define CPS_TYPE_SIX_MIN_COUNT  30      //淘汰赛最少多少用户

//#define CPS_TEXT                        //测试标识

/*进入房间*/
void CpsEnterRoom( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

/*坐下,锦标赛的坐下只能由系统分配*/
void CpsSeatDown( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

/*用户离开座位*/
void  CpsLeaveSeat( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

/*播放完毕*/
void  CpsDisplayOver( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

/*加注*/
void CpsAddChip(int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead,int nType);

/*用户离开房间*/
void CpsExitRoom( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

/*获得比赛排名*/
void CpsGetRankList( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);

/*进入房间，内部调用*/
int CpsEnter(dealer *pRoom,user newuser);

/*坐下，内部调用*/
int CpsSeatDownIn(dealer *pRoom,int nUserID);

/*离开座位*/
int CpsLeaveSeatIn(dealer *pRoom,int nUserID);
/*定时器*/
int CpsTimer();

/*到时间比赛开始*/
int CpsStart(CPS_RECORD_T * pRecord);

/*比赛结束回调函数*/
int CpsNoLockEndBack(CPS_RECORD_T * pRecord,dealer *pRoom,int nType = 0);

/*锦标赛*/
int CpsUserGiveUp(int nType,user *pUser);

#endif //__CPS_H__
