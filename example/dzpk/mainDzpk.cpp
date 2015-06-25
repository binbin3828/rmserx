/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克模块入口
**************************************************************
*/
#include "mainDzpk.h"
#include "dzpkPlay.h"
#include "dzpkUser.h"
#include <string.h>
#include <stdlib.h>
#include "championship/cps.h"
#include "Start.h"
#include "championship/cpsUser.h"
#include "championship/cpsCustom.h"
#include "standard/dzpkCustom.h"
#include "standard/dzpkTask.h"
#include "standard/dzpkActivity.h"

int g_ndzpkStatus = 0;			//德州扑克模块运行情况

int g_ndzpkTimerID = 0;
class CMutex g_dzpkTimerMutex;


#define DZPK_ROOM_TYPE_COUNT    20  //房间最多类型
DZPK_MODULE_T *g_arryRoomType[DZPK_ROOM_TYPE_COUNT];
int g_arryRoomTypeIndex = 0;
DZPK_MODULE_T * g_pModuleNor = NULL;

/*德州扑克接入*/
extern int dzpkStart();

/*初始化模块*/
int DzpkInitModule()
{
    g_arryRoomTypeIndex = 0;

    //普通房间
    g_pModuleNor = new DZPK_MODULE_T();
    g_arryRoomType[g_arryRoomTypeIndex] = g_pModuleNor;
    g_arryRoomTypeIndex++;

    //锦标赛
    DZPK_MODULE_T * pChampionship = new DZPK_MODULE_T();
    pChampionship->nRoomType = 10;                          //测试，type还没确定
    pChampionship->msgEnterRoom = CpsEnterRoom;             //进入房间
    pChampionship->msgSeatDown = CpsSeatDown;               //坐下
    pChampionship->msgLeaveSeat = CpsLeaveSeat;             //离开座位
    pChampionship->msgDisplayOver = CpsDisplayOver;         //播放完毕
    pChampionship->msgAddChip = CpsAddChip;                 //加注
    pChampionship->msgExitRoom = CpsExitRoom;               //离开房间
    pChampionship->msgPresentChip = NULL;                   //不能赠送筹码
    pChampionship->msgCpsGetRankList = CpsGetRankList;      //获得比赛列表
    pChampionship->msgProhibitUser = NULL;                  //不能使用踢人卡
    pChampionship->msgProhibitUserVote = NULL;

    g_arryRoomType[g_arryRoomTypeIndex] = pChampionship;
    g_arryRoomTypeIndex++;
    return 0;
}


int dzpkAdd(int nUserID,int nStart,int nChip,int nFailTime)
{
        CPS_USER_T _RecordUser;
        memset(&_RecordUser,0,sizeof(CPS_USER_T));
        _RecordUser.nCpsID = 100;
        _RecordUser.nCurHandChip = nChip;
        _RecordUser.nStart = nStart;
        _RecordUser.nType = 100;
        _RecordUser.nUserID = nUserID;
        _RecordUser.nFailTime = nFailTime;
        _RecordUser.nActiveTime = time(NULL);
        CpsInserUser(_RecordUser.nUserID,&_RecordUser,sizeof(CPS_USER_T));
    return 0;
}

/*开启德州扑克模块*/
int ServiceDzpkStart()
{
	int nRet = -1;
	if(g_ndzpkStatus == 0)
	{
        //模块初始化
        DzpkInitModule();

        /*开始游戏*/
		dzpkStart();

        //读锦标赛数据
        CpsReadDb();

        /*读取系统活动*/
        StdReadSystemActivity();

        /*系统任务*/
        dzpkReadSystemTask(0);

		/*注册函数*/
		NetRegisterAddSocketBack(DzpkAddSocketMallocBack,8006);
		NetRegisterCloseSocketBack(DzpkCloseSocketBack,8006);
		NetResgisterFreeSocketBack(DzpkHashRealFreeDataBack,8006);

		/*用户进入房间*/
		NetRegisterNormalMsg(CMD_ENTER_ROOM,DzpkUserEnterRoom);
		/*用户坐下*/
		NetRegisterNormalMsg(CMD_SEAT_DOWN,DzpkUserSeatDown);
		/*处理加注消息**/
		NetRegisterNormalMsg(CMD_ADD_CHIP,DzpkUserAddChip);
		/*处理说话消息*/
		NetRegisterNormalMsg(CMD_SPEEK_WORDS,DzpkUserSpeekWords);
		/*处理离座消息**/
		NetRegisterNormalMsg(CMD_LEAVE_SEAT,DzpkUserLeaveSeat);
		/*处理展示完毕消息*/
		NetRegisterNormalMsg(CMD_DISPALY_OVER,DzpkUserDisplayOver);
		/*处理离开房间消息*/
		NetRegisterNormalMsg(CMD_EXIT_ROOM,DzpkUserExitRoom);
		/*处理快速开始消息*/
		NetRegisterNormalMsg(CMD_QUICK_START,DzpkUserQuickStart);
		/*赠送礼物*/
		NetRegisterNormalMsg(CMD_PRESENT_GIFT,DzpkPresentGift);
		/*定时器*/
		NetRegisterNormalMsg(CMD_DZPK_TIMER,DzpkTimer);
        //socket关闭
		NetRegisterNormalMsg(CMD_USER_CLOSE,DzpkUserLeave);
		/*赠送筹码*/
		NetRegisterNormalMsg(CMD_PRESENT_CHIPS,DzpkPresentChips);
		/*添加好友*/
		NetRegisterNormalMsg(CMD_ADD_FRIEND,DzpkAddFriends);
		/*心跳*/
		NetRegisterNormalMsg(CMD_HEART_WAVE ,DzpkHeartWare);
		/*获取旁观玩家*/
		NetRegisterNormalMsg(CMD_GET_STANDBY ,DzpkGetStandby);
		/*手动亮牌*/
		NetRegisterNormalMsg(CMD_LAY_CARDS ,DzpkLayHandCards);
		/*手动补充筹码*/
		NetRegisterNormalMsg(CMD_RESET_CHIPS ,DzpkResetHandChips);
		/*获取用户等级礼包信息*/
		NetRegisterNormalMsg(CMD_LEVEL_GIFTS ,DzpkGetLevelGifts);
		/*处理用户等级礼包领取*/
		NetRegisterNormalMsg(CMD_DEAL_GIFTS ,DzpkDealLevelGifts);
		/*处理用户倒计时宝箱领取*/
		NetRegisterNormalMsg(CMD_CNTD_GIFTS ,DzpkDealCountDown);
		/*获得锦标赛用户列表*/
		NetRegisterNormalMsg(CMD_CPS_RANK_LIST,DzpkCpsGetRankList);
		/*发送消息给房间用户*/
		NetRegisterNormalMsg(CMD_SEND_MSG_TO_ROOM,DzpkSendMsgToRoomUser);
		/*发起投票禁止某个用户*/
		NetRegisterNormalMsg(CMD_PROHIBIT_USER,DzpkProhibitUser);
		/*投票*/
		NetRegisterNormalMsg(CMD_PROHIBIT_USER_VOTE,DzpkProhibitUserVote);
		/*告诉服务器重新读背包*/
		NetRegisterNormalMsg(CMD_RE_READ_USER_PACK,DzpkReReadPack);
        /*给荷官小费*/
        NetRegisterNormalMsg(CMD_GIVE_TIP_TO_DEALER,dzpkGiveTipToDealer);
        /*重新加载数据到内存*/
        NetRegisterNormalMsg(CMD_READ_MEM_DATA, reloadMemData);


		//添加定时器用户
		DzpkAddSocketMallocBack(0,0,0,new CNetSocketRecv(),new CNetSocketSend());

		nRet = 0;
	}
	else if(g_ndzpkStatus == 1)
	{
		//模块已经在运行
		nRet = 0;
	}

	return nRet;
}

/*关闭德州扑克模块*/
int ServiceDzpkStop()
{
	int nRet = -1;

	return nRet;
}

/*获得模块运行状态，是否有运行*/
int GetDzpkStatus()
{
	return g_ndzpkStatus;
}

/*********************
 *是否是锦标赛
 ********************/
int DzpkIfCpsOfType(int nRoomType)
{
    int nRet = -1;

    if(nRoomType == ROOM_TYPE_CPS || nRoomType == ROOM_TYPE_CPS+1)
    {
        nRet = 0;
    }

    return nRet;
}

/*********************
 *取德州扑克模块
 ********************/
DZPK_MODULE_T *DzpkGetModule(int nRoomID)
{
    DZPK_MODULE_T * pRes = NULL;

    dealer *pRoom = DzpkGetRoomInfo(nRoomID);
    if(pRoom)
    {
        if(DzpkIfCpsOfType(pRoom->room_type) == 0)
        {
            pRes =  g_arryRoomType[1];
        }
    }

    //如果找不到，就是标准房间
    if(pRes == NULL)
    {
        pRes = g_pModuleNor;
    }

    return pRes;
}

DZPK_MODULE_T *DzpkGetModuleFromRomType(int nRoomType)
{

    DZPK_MODULE_T * pRes = NULL;

    if(DzpkIfCpsOfType(nRoomType) == 0)
    {
        pRes =  g_arryRoomType[1];
    }

    //如果找不到，就是标准房间
    if(pRes == NULL)
    {
        pRes = g_pModuleNor;
    }

    return pRes;
}
/**********************
 * 发送消息给用户
 * nUserID   用户ID
 * nMsgID    消息ID
 * pBuf      消息内容
 * nBufLen   消息长度
 * nRoomID   用户所在房间
 * 其他字段目前没有使用
**********************/
int DzpkSendToUser(int nUserID,int nMsgID,char *pBuf, int nBufLen,int nRoomID,short nAck,unsigned int nOrder,unsigned int nSourceID,unsigned int nAimID)
{
    return DzpkSend(0,nMsgID,pBuf,nBufLen,nRoomID,nUserID,nAck,nOrder,nSourceID,nAimID);
}

int DzpkSend(int nSocketID,int nMsgID,char *pBuf, int nBufLen,int nRoomID,int nUserID,short nAck,unsigned int nOrder,unsigned int nSourceID,unsigned int nAimID)
{
	int nRet = -1;
	int nSendLen = 	nBufLen;
	char *pSend = (char *)malloc(nSendLen);
	if(pSend)
	{
		memset(pSend,0,nSendLen);
		if(pBuf && nBufLen)
		{
			memcpy(pSend,pBuf,nBufLen);
		}
		void *pUserInfo = NULL;
		pUserInfo = NetGetNetUser(nUserID);
		if( NULL != pUserInfo)
		{
			nRet = NetSendMsgToSocketP(pUserInfo,pSend,nSendLen);
			NetFreeNetUser(pUserInfo);
		}
		if(nRet != 0)
		{
			WriteRoomLog(4000,nRoomID,nUserID,"send fail content:%s",pBuf);
			free(pSend);
			pSend = NULL;
		}
		else
		{
			WriteRoomLog(4000,nRoomID,nUserID,"send success content:%s",pBuf);
		}
	}


	return nRet;
}
int DzpkSendP(void *pSocketUser,int nMsgID,char *pBuf, int nBufLen,int nRoomID,int nUserID,short nAck ,unsigned int nOrder ,unsigned int nSourceID ,unsigned int nAimID)
{
	int nRet = 0;
	int nSendLen = 	nBufLen;
	char *pSend = (char *)malloc(nSendLen);
	if(pSend)
	{
		memset(pSend,0,nSendLen);
		if(pBuf && nBufLen)
		{
			memcpy(pSend,pBuf,nBufLen);
		}
		nRet = NetSendMsgToSocketP(pSocketUser,pSend,nSendLen);
		WriteRoomLog(4000,nRoomID,nUserID,"send  success content:%s",pBuf);
		if(nRet != 0)
		{
			free(pSend);
			pSend = NULL;
		}
	}

	return nRet;
}



int DzpkGetTimerID()
{
	int nRet = -1;
	g_dzpkTimerMutex.Lock();
	g_ndzpkTimerID++;
	nRet = g_ndzpkTimerID;
	g_dzpkTimerMutex.Unlock();
	return nRet;
}

int DzpkUninit()
{
    return dzpkStartUninit();
}
/*设置数据库*/
int DzpkSetDbInfo(char *pIP,int nPort,char *pLoginName,char *pLoginPwd,char *pDbName)
{
    int nRet = 0;

    if(pIP && nPort > 0 && pLoginName && pLoginPwd && pDbName)
    {
        memcpy(g_pDzpkDbInfo->szIP,pIP,strlen(pIP));
        g_pDzpkDbInfo->nPort = nPort;
        memcpy(g_pDzpkDbInfo->szLoginName,pLoginName,strlen(pLoginName));
        memcpy(g_pDzpkDbInfo->szLoginPwd,pLoginPwd,strlen(pLoginPwd));
        memcpy(g_pDzpkDbInfo->szDbName,pDbName,strlen(pDbName));
    }

    return nRet;
}
/*初始化*/
int DzpkInit()
{
cout << "---------------------------dzpkStartDb---b----------------" << endl;
    dzpkStartDb();
cout << "---------------------------dzpkStartDb---e----------------" << endl;
    return 0;
}
/*服务器人数日志*/
int DzpkServerStatus(int nServerID,int nServerType,int nSocketCount,int nUserCount)
{
    dzpkDbWriteOnlineNumLog(nServerID,nServerType,nSocketCount,nUserCount);
    return 0;
}
