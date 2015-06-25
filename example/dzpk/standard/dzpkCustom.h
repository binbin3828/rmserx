#ifndef __DZPKCUSTOM_H__
#define __DZPKCUSTOM_H__
#include "custom.h"
#include "dzpkHead.h"

/*德州扑克标准流程通用函数*/

/*获得座位号，5人桌*/
int StdGetEffectiveSeatNumFive(dealer *pRoom,int nSeatNumber);

/*获得有效座位号*/
int StdGetEffectiveSeatNum(dealer *pRoom,int nSeatNumber);

/*用户坐下*/
int StdUserSeatDown(dealer *pRoom,int nUserID,int nSeatNumber,SEAT_DOWN_REQ_T *pInfo);

/*检查用户筹码状态,开局前需要检查一下用户状态*/
int StdCheckUserstatus(dealer *pRoom,int &nStatusChange,int nCheckFee = 1,int *pCurFee = NULL,int nGiveUpLeave = 0);

/*清空房间上一局所有信息*/
int StdClearRoomInfo(dealer *pRoom,int nStatus = 0);

/*检查是否能开始*/
int StdCheckGameStart(dealer *pRoom,int nCurFee);

/*洗牌*/
int StdShuffleCards(dealer *pRoom);

/*发牌*/
int StdDealCards(dealer *pRoom);

/*下大小盲注*/
int StdBigSmallBind(dealer *pRoom);

/*轮到某个用户操作*/
int StdTurnToUser(dealer *pRoom);

/*更新用户信息*/
int StdUpdateUserInfo(int nUserID,user userInfo);

/*获得跟注值*/
int StdGetFollowChip(dealer *pRoom);

/*判断用户是否在玩的状态*/
int StdIfPlayingUser(int nUserStatus);

/*判断用户是否有双倍经验卡*/
int StdIfDoubleExpCard(user *pUser);

/*判断用户是否有vip卡*/
int StdIfVipCard(user *pUser,int &nMallID,int *pLevel = NULL);

/*判断用户是否有时间类道具*/
int StdIfTimeToolCard(user *pUser,int nCardID,int *pTime = NULL);

/*获得标准时间*/
int StdGetTime();

/*获得经验*/
int StdGetAddExperience(user *pUser,int nExpAdd);

/*返回错误*/
int StdDendErrorToUser(int nCmd,int nErrorCode,int nUserID,int nRoomID);

/*清除过时禁止用户进入信息*/
int StdUpdateNoInUser(dealer *pRoom);


/*用户是否注册*/
int StdIfUserRegister(int nUserID,DZPK_USER_T *pSocketUser,int nType = 0);

/*用户是否在房间*/
int StdIfUserInRoom(dealer *pRoom,int nUserID,user *pUser = NULL);
int StdIfUserInRoom(dealer *pRoom,string user_account,user *pUser = NULL);

/*用户是否是坐下状态*/
int StdIfUserSeatDown(dealer *pRoom,int nUserID,user *pUser = NULL);
int StdIfUserSeatDown(dealer *pRoom,string user_account,user *pUser = NULL);

/*用户是否可以坐下*/
int StdUserCanSeatDownOfIP(dealer *pRoom,long lUserIP);

/*玩家在游戏中赢取金币时，如果满足条件，玩家信息及赢取信息会在公共频道上播出
  这里调用httpClient的post把消息传递给，指定接口*/
int StdHttpPostChipsWinMessage(int nUserID, user *pUserInfo, int nChip);


#endif //__DZPKCUSTOM_H__
