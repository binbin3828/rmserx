#ifndef __DZPKACTIVITY_H__
#define __DZPKACTIVITY_H__
#include "custom.h"
#include "dzpkHead.h"

/*德州扑克标准流程活动*/

#define SYSTEM_ACTIVITY_AWARD       1       //系统抽奖活动ID
#define SYSTEM_LUCKY_CARD_AWARD     2       //幸运手牌活动
/*读取系统活动*/
int StdReadSystemActivity(int nType = 0);

/*是否抽奖活动正在进行*/
int StdActivityAwardRunning(int nActivityID,/*SYSTEM_ACTIVITY_T*/ void *pInfo = NULL);

/*看用户是否玩了一盘,系统抽奖活动*/
int StdTodayPlayTimes(dealer *pRoom);

/*获得下一次奖励次数*/
int StdGetNextPlayTimes(int nCurTimes);

/*读取幸运手牌奖励*/
int StdReadLuckCardAward();

/*开始手牌奖励*/
int StdSetHandCardAward(int nHandCard1,int nHandCard2,int nSmallBlind,int nHandChip,int nMultiple,char *pName,int nType);

/*牌型奖励*/
int StdSetHandTypeAward(int nCardTypeStart,int nCardTypeEnd,int nSmallBlind,int nHandChip,int nMultiple,char *pName,int nType);

/*手牌获得奖励*/
int StdGetHandCardAward(int nHandCard1,int nHandCard2,/*LUCK_CARD_HAND_T*/void *pArg);

/*牌型获得奖励*/
int StdGetCardTypeAward(int nCardType,/*LUCK_CARD_TYPE_T*/void *pArg);

/*幸运手牌活动,nStage = 0 开始手牌，1 结束牌型,2 结束没牌型*/
int StdLuckyCardActivity(dealer *pRoom,int nStage);

/*发送奖励*/
int StdSendLuckCardPack(dealer *pRoom,user *pInfo,int nSmallBlind,int nHandChip,int nMultiple,char *pName,int visible,int nAwardType);

/*发送包*/
int stdSendLuckCardParam(dealer *pRoom,int nUserID,char *pUserAccount,int nAwardChip,char *pTooltip,int nVisible,int nAddChip,char *pHeadPhoto);

/*获取奖励的金豆数*/
int getJinDouNum(int playTimes, int smallBind);



#endif //__DZPKACTIVITY_H__
