#ifndef __DZPKPLAY_H__
#define __DZPKPLAY_H__

#include "../../com/common.h"

/*JackFrost 2013.10.31 兼容第一版本中转函数*/

/*用户进入房间*/
int DzpkUserEnterRoom(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*用户坐下*/
int DzpkUserSeatDown(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*用户行为如加注、弃牌*/
int DzpkUserAddChip(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*聊天*/
int DzpkUserSpeekWords(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*离开位置*/
int DzpkUserLeaveSeat(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*播放结束动画*/
int DzpkUserDisplayOver(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*用户离开房间*/
int DzpkUserExitRoom(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*快速开始*/
int DzpkUserQuickStart(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*赠送礼物*/
int DzpkPresentGift(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*定时器*/
int DzpkTimer(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*socket关闭*/
int DzpkUserLeave(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*赠送筹码*/
int DzpkPresentChips(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*添加朋友*/
int DzpkAddFriends(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*心跳*/
int DzpkHeartWare(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*获取旁观玩家*/
int DzpkGetStandby(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*手动亮牌*/
int DzpkLayHandCards(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*手动补充筹码*/
int DzpkResetHandChips(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*获取玩家等级礼包信息*/
int DzpkGetLevelGifts(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*处理玩家等级礼包领取*/
int DzpkDealLevelGifts(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*处理玩家倒计时宝箱领取*/
int DzpkDealCountDown(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*获得锦标赛列表*/
int DzpkCpsGetRankList(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*发送消息*/
int DzpkSendMsgToRoomUser(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*发起投票禁止某个用户*/
int DzpkProhibitUser(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*投票*/
int DzpkProhibitUserVote(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*告诉服务器重新读背包*/
int DzpkReReadPack(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*给荷官小费*/
int dzpkGiveTipToDealer(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);

/*重新加载数据到内存*/
int reloadMemData(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen);



#endif // __DZPKPLAY_H__
