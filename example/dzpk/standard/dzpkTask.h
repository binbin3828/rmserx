#ifndef __DZPKTASK_H__
#define __DZPKTASK_H__
#include "custom.h"
/*JackFrost 2014.2.18 任务系统*/

#define DZPK_SYSTEM_TASK_SEND_FACE_TYPE          1       //发送表情
#define DZPK_SYSTEM_TASK_SEND_GIFT_TYPE          2       //赠送礼物
#define DZPK_SYSTEM_TASK_PLAY_GAME_TYPE          3       //玩一局
#define DZPK_SYSTEM_TASK_WIN_GAME_TYPE           10      //赢一局
#define DZPK_SYSTEM_TASK_WIN_CHIP_TYPE           15      //赢3000

/*读取系统任务*/
int dzpkReadSystemTask(int nType);

/*任务完成一*/
int dzpkTaskTypePlusOne(int nTaskType,user *pUserInfo);

/*具体任务加一*/
int dzpkTaskSpecific(int nTaskID,user *pUserInfo,int nTaskTotal);

/*检查是否玩了一局*/
int dzpkCheckTaskPlayGame(dealer *pRoom);

/*检查是否赢牌和赢的大*/
int dzpkCheckTaskWin(int nChip,user *pUserInfo);

/*发完成消息给用户*/
int dzpkCheckTaskComplete(user *pUserInfo,int nTaskID);

/*进房间检查是否需要提示任务完成*/
int dzpkEnterCheckTaskComplete(user *pUserInfo);

/*发送有任务完成提示*/
int dzpkSendTaskComplete(user *pUserInfo);

#endif //__DZPKTASK_H__
