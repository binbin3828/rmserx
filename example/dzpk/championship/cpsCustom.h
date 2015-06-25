/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：
**************************************************************
*/
#ifndef __CPSCUSTOM_H__
#define __CPSCUSTOM_H__
#include "cpsRecord.h"
#include "../db/dzpkDb.h"

/*锦标赛通用函数*/

/*算名排序*/
typedef struct tagCpsLastResult
{
    tagCpsLastResult()
    {
        pUser = NULL;
        nPreHandChip = 0;
    }
    user *pUser;                    //用户信息
    int nPreHandChip;               //上依稀筹码
}CPS_LAST_RESULT_T;

/*检查房间状态，检查是否开始*/
int CpsCheckRecord();

/*检查用户长时间没有操作,只检查已经开始的用户*/
int CpsCheckUserOut();

/*检查用户迁移,是否到终极赛*/
int CpsNoLockCheckFinalTable(CPS_RECORD_T *pRecord,int nRoomId);

/*所有桌开始,这是第一次开始，如果发现有的房间少于可开始用户，要拼桌*/
int CpsNoLockStartAllTables(CPS_RECORD_T *pRecord);

/*开始一局*/
int CpsNoLockTableStart(CPS_RECORD_T *pRecord,int nRoomId);

/*获得真实人数*/
int CpsNoLockGetSeatDownUser(CPS_RECORD_T *pRecord);

/*用户合并成终极桌*/
int CpsNoLockComposeFinalTables(CPS_RECORD_T *pRecord,int nRoomId);

/*拼桌*/
int CpsNoLockComposeTables(CPS_RECORD_T *pRecord,int nRoomID,int nStart);

/*拼桌获得一个人数少于标准的桌*/
dealer *CpsNoLockGetLessTables(CPS_RECORD_T *pRecord,int nView);

/*获得一个房间用户数量，里面要自己加锁*/
int CpsGetRoomNumber(dealer *pRoom,int nAllUser = 0);

/*检查用户筹码状态,开局前需要检查一下用户状态*/
int CpsCheckUserstatus(dealer *pRoom,int nGiveUpLeave,int nCurTime);

/*预检查要删除多少人*/
int CpsPreCheckUserstatus(dealer *pRoom,int nGiveUpLeave,int nCurTime);

/*检查在游戏中用户两次棋牌*/
int CpsCheckGameingUserStatus(dealer *pRoom,int nUserID);

/*用户找一个房间进入比赛*/
dealer *CpsGetUseRoom(CPS_RECORD_T *pRecord);

/*增加一个房间*/
dealer *CpsAddRoom(dealer *pRoomCopy,CPS_RECORD_T *pRecord);

/*更新用户所在房间*/
int CpsUpdateUserInRoom(int nUserID,int nRoomID);

/*用户失败*/
int CpsUserFail(user *pUser,dealer *pRoom,int nRecordID,int nFinal);

/*用户获奖，房间顺便解散*/
int CpsUserSuccess(CPS_RECORD_T * pRecord,dealer *pRoom,int nSucUserID);

/*比赛异常结束*/
int CpsForceStop(CPS_RECORD_T * pRecord,int nType);

/*强制用户离开比赛房间*/
int CpsForceUserLeave(CPS_RECORD_T * pRecord,dealer *pRoom,int nType);

/*更新锦标赛用户筹码*/
int CpsUpdateRecordUserChip(int nUserId,int nHandChip);

/*更新锦标赛用户最后活动时间*/
int CpsUpdateRecordUserActive(int nUserID);

/*奖励用户并返回提示*/
int CpsRewardUser(int nUserID,int nRank,CPS_RECORD_T * pRecord,char *pTooltip,user *pUser,CPS_AWARD_DB_HEAD_T *pHead);

/*从数据库读每天锦标赛活动*/
int CpsReadDbEveryDay();

/*从数据库读所有奖励*/
int cpsReadDbReward();

/*读取涨盲注数组*/
int CpsReadDbAddBlind();

/*启动读所有数据*/
int CpsReadDb();

/*获得当前时间*/
int CpsGetTime();

/*获得下一次盲注*/
int CpsGetNextBlind(int nType,int nCurBlind);

/*获得第几名提示,里面调用奖励*/
int CpsUserCpsResult(int nUserID,int nRankID,int nRank,char *pTooltip,user *pUser,CPS_AWARD_DB_HEAD_T *pHead);

/*用户获得奖励*/
int CpsUserGetAward(int nUserID,CPS_AWARD_DB_T *p,user *pUser);

/*涨盲注提示*/
int CpsAddSmallblindTooltip(CPS_RECORD_T * pRecord,int nLastBlind,int nCurBlind);

/*发送排名给用户*/
int CpsSendRankToUser(int nRecordID,int nUserID = -1);

/*获得用户筹码*/
int CpsGetRecordUserChip(int nUserId);

/*排名排序*/
int CpsSortResultUser(CPS_LAST_RESULT_T  *pArry,int nCount);

/*获得用户是否走开标识*/
int CpsGetUserLeaveMark(int nUserID);

#endif //__CPSCUSTOM_H__
