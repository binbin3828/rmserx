/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：
**************************************************************
*/
#ifndef __CPSRECORD_H__
#define __CPSRECORD_H__
/*锦标赛具体记录*/

#include "../../../com/safeHash.h"

#define CPS_RECORD_ROOM_COUNT       200         //最多有多少个房间

typedef struct tagCpsRecord
{
    int nRecordID;
    int nType;                                  //房间类型  5
    long nStartTime;
    int nStartNum;                              //最小开赛人数
    int nStart;                                 //是否已经开始
    int nLastAddBlindTime;                      //上次涨盲时间
    int arryRoom[CPS_RECORD_ROOM_COUNT];        //本次活动使用那几个房间
    int nArryRoomIndex;                         //当前个数
	class CMutex *pMutex;                         //记录锁
    int nCurSmallBlind;                         //当前小盲注
    int nInitHandChip;                          //初始筹码
    int nAddBlind;                              //一次涨多少筹码
    int nAddBlindTime;                          //间隔多少秒涨盲注
    int nRecordUserCount;                       //总共有多少用户玩
    int nRanK;                                  //奖励
    char szGameName[50];                        //游戏名称
    int nGameFee;                               //报名费用
}CPS_RECORD_T;

/*添加记录*/
int CpsInsertRecord(int nRecordID,CPS_RECORD_T *pData,int nLen);

/*获得记录信息*/
CPS_RECORD_T * CpsGetRecord(int nRecordID);

/*释放记录信息*/
int CpsFreeRecord(CPS_RECORD_T *pData);

/*删除记录*/
int CpsRemoveRecord(int nRecordID);

//获得所有数据
SAFE_HASH_LIST_T *CpsRecordGetList();

//获得所有数据需要调用此接口释放
int CpsRecordFreeList(SAFE_HASH_LIST_T * pList);

//获得记录个数
int CpsRecordGetSize();


#endif //__CPSRECORD_H__
