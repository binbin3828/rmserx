/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：
**************************************************************
*/
#ifndef __CPSUSER_H__
#define __CPSUSER_H__

#include "../../../com/safeHash.h"

/*锦标赛用户*/

//#define

typedef struct tagCpsUser
{
    int nUserID;                //用户ID
    int nType;                  //类型
    int nCpsID;                 //当前活动ID
    int nCurHandChip;           //筹码
    int nStart;                 //是否已经开始
    int nFailTime;              //失败时的时间
    int nActiveTime;            //最后一次活动时间
    int nEnterTime;             //进入时间
    int nUserLeaveGame;         //用户走开标识
}CPS_USER_T;

/*添加用户*/
int CpsInserUser(int nUserID,CPS_USER_T *pData,int nLen);

/*获得用户信息*/
CPS_USER_T * CpsGetUser(int nUserID);

/*释放用户信息*/
int CpsFreeNetUser(CPS_USER_T *pData);

/*删除用户*/
int CpsRemoveUser(int nUserID);

/*获得活动人数*/
int CpsGetUserCount(int nType,int nCpsID);

//获得所有数据
SAFE_HASH_LIST_T *CpsUserGetList(int nType,int nCpsID,int nSortChip = 0);

/*获得在玩的用户个数*/
int CpsUserGetOnline(int nType,int nCpsID);

//获得所有数据需要调用此接口释放
int CpsUserFreeList(SAFE_HASH_LIST_T * pList);

//获得所有记录个数
int CpsUserGetCount();

/*复制*/
int CpsUserCopy(CPS_USER_T *p,CPS_USER_T *p1);

#endif //__CPSUSER_H__
