#ifndef __START_H__
#define __START_H__

#include "standard/custom.h"
#include <vector>
#include "mainDzpk.h"
#include "db/dzpkDb.h"
#include "db/MemData.h"


using namespace std;

extern vector<dealer *>  dealers_vector; //所有房间荷官存储队列
extern vector<level_system>  level_list;
extern ConnPool *connpool;
extern CSafeHash g_dzpkTimer;
extern DZPK_DB_INFO_T *g_pDzpkDbInfo;
extern SYSTEM_ACTIVITY_HEAD_T g_SystemActivity;                    //系统活动
extern int g_nSystemCountDownMaxLevel;                             //总公七个宝箱
extern MemData *g_pMemData;


/*用户断线*/
int dzpkUserExitAction(int nUserID,int nRoomID);

/*获取房间信息*/
dealer * DzpkGetRoomInfo(int nRoomNum);

/*定时器*/
int dzpkTimerCheck();

int dzpkStartUninit();

/*开数据库*/
int dzpkStartDb();

#endif //__START_H__
