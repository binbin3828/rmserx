#ifndef __DZPKPARSEPACKET_H__
#define __DZPKPARSEPACKET_H__
#include "dzpkHead.h"
#include "../include/json/json.h"
#include "custom.h"

/*解释和打包网络数据*/
#define SEND_PACK_MAX_LEN       1024*1024*4

/*获得进入房间数据*/
int dzpkGetEnterRoomPack(ENTER_ROOM_REQ_T *pInfo,Json::Value msg_commond);

/*将进入房间返回数据打包*/
int dzpkPutEnterRoomPack(ENTER_ROOM_RES_T *pInfo,char *pBuf,int &nBufLen);

/*将进入房间返回数据打包*/
int dzpkPutEnterRoom(dealer *pRoom,char *pUserAccount,int nUserID,char *pBuf,int &nBufLen);

/*获得坐下数据*/
int dzpkGetSeatDownPack(SEAT_DOWN_REQ_T *pInfo,Json::Value msg_commond);

/*坐下失败，用户钱不够或者找不到用户*/
int dzpkPutSeatDownError(dealer *pRoom,SEAT_DOWN_REQ_T *pInfo,int nErrorCode,char *pBuf,int &nBufLen);

/*坐下信息发给房间用户*/
int dzpkPutSeatDownPack(dealer *pRoom,SEAT_DOWN_REQ_T *pInfo,int nUserID,char *pBuf,int &nBufLen);

/*自动买入*/
int dzpkPutAutoBuy(dealer *pRoom,DZPK_USER_INFO_T *pUser,int nBuyChip,char *pBuf,int &nBufLen);

/*筹码不足强制站起*/
int dzpkPutForceLeaveSeat(dealer *pRoom,DZPK_USER_INFO_T *pUser,char *pBuf,int &nBufLen);

/*锦标赛返回倒计时*/
int dzpkPutCpsCountDown(dealer *pRoom,int nSecond,char *pBuf,int &nBufLen);

#endif //__DZPKPARSEPACKET_H__
