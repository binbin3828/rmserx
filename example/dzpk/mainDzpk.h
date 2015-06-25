#ifndef __MAINDZPK_H__
#define __MAINDZPK_H__

/* 德州扑克对外接口,一个模块对外只有一个头文件(而且最好是格式统一)*/

#include "../../com/mainNet.h"
#include "../../com/mainLoger.h"
#include "../../com/autoLock.h"
#include "../../core/loger/log.h"
#include "standard/dzpk.h"

#define USER_PROHIBIT_CAR_LOG_LEVEL           0             //踢人卡日志
#define USER_VIP_CARD_LOG_LEVEL               0             //vip日志、免费表情卡
#define USER_DOUBLE_CARD_LOG_LEVEL            0             //双倍经验卡日志
#define USER_ACTIVITY_AWARD_LEVEL            0              //系统活动,抽奖
#define USER_ACTIVITY_CARD_LEVEL              0          //系统活动,幸运手牌
#define USER_SYSTEM_TASK_LEVEL                0             //每日任务
#define USER_SYSTEM_COUNTDOWN_LEVEL           0             //倒计时宝箱


/*开启德州扑克模块*/
int ServiceDzpkStart();

/*关闭德州扑克模块*/
int ServiceDzpkStop();

/*获得模块运行状态，是否有运行（以后增加支持暂停等状态）*/
/*0 为没有运行  1 为正常运行*/
int GetDzpkStatus();

/*获得定时器ID*/
int DzpkGetTimerID();

/*设置数据库*/
int DzpkSetDbInfo(char *pIP,int nPort,char *pLoginName,char *pLoginPwd,char *pDbName);

/***************** 下面为模块内调用函数 *******************/
typedef void (*DZPKNORMALMSGBACK)( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead);
typedef void (*DZPKNORMALMSGADDCHIPBACK)( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead,int nType);
typedef int  (*DZPKSOCKETCLOSE)(int nSocket,DZPK_USER_T *pUser);
/*德州扑克模块*/
typedef struct tagDzpkModule
{
    tagDzpkModule()
    {
        msgEnterRoom = deal_enter_room;
        msgSeatDown = deal_seat_down;
        msgAddChip = deal_add_chip;
        msgSpeekWorks = deal_speek_words;
        msgLeaveSeat = deal_leave_seat;
        msgDisplayOver = deal_display_over;
        msgExitRoom = deal_exit_room;
        msgQuickStart = deal_quick_start;
        msgPresentGift = deal_present_gift;
        msgPresentChip = deal_present_chips;
        msgAddFriend = deal_add_friend;
        msgHeartWave = deal_heart_wave;
        msgCpsGetRankList = NULL;
        msgProhibitUser = deal_prohibit_user;
        msgProhibitUserVote = deal_prohibit_user_vote;
        msgReReadPack = deal_re_read_pack;
        nRoomType = 0;
    }
    DZPKNORMALMSGBACK msgEnterRoom;             //进入房间
    DZPKNORMALMSGBACK msgSeatDown;              //坐下
    DZPKNORMALMSGADDCHIPBACK msgAddChip;        //加注
    DZPKNORMALMSGBACK msgSpeekWorks;            //聊天
    DZPKNORMALMSGBACK msgLeaveSeat;             //站起
    DZPKNORMALMSGBACK msgDisplayOver;           //播放完毕
    DZPKNORMALMSGBACK msgExitRoom;              //离开房间
    DZPKNORMALMSGBACK msgQuickStart;            //快速开始
    DZPKNORMALMSGBACK msgPresentGift;           //赠送礼物
    DZPKNORMALMSGBACK msgPresentChip;           //赠送筹码
    DZPKNORMALMSGBACK msgAddFriend;             //添加好友
    DZPKNORMALMSGBACK msgHeartWave;             //心跳
    DZPKNORMALMSGBACK msgCpsGetRankList;        //获得锦标赛类型
    DZPKNORMALMSGBACK msgProhibitUser;          //发起投票禁止某个用户
    DZPKNORMALMSGBACK msgProhibitUserVote;       //投票
    DZPKNORMALMSGBACK msgReReadPack;             //告诉服务器重新读背包
    int nRoomType;                               //房间类型
}DZPK_MODULE_T;

/*通过房间号获取模块*/
DZPK_MODULE_T *DzpkGetModule(int nRoomID);

/*通过房间号获取模块*/
DZPK_MODULE_T *DzpkGetModuleFromRomType(int nRoomType);
/*是否是锦标赛*/
int DzpkIfCpsOfType(int nRoomType);

/*发送数据*/
int DzpkSendToUser(int nUserID,int nMsgID,char *pBuf, int nBufLen,int nRoomID,short nAck = 0,unsigned int nOrder = 0,unsigned int nSourceID = 0,unsigned int nAimID = 0);

/*发送数据*/
int DzpkSend(int nSocketID,int nMsgID,char *pBuf, int nBufLen,int nRoomID,int nUserID,short nAck = 0,unsigned int nOrder = 0,unsigned int nSourceID = 0,unsigned int nAimID = 0);

/*发送数据给未注册用户*/
int DzpkSendP(void *pSocketUser,int nMsgID,char *pBuf, int nBufLen,int nRoomID,int nUserID,short nAck = 0,unsigned int nOrder = 0,unsigned int nSourceID = 0,unsigned int nAimID = 0);

//服务器关闭最后调用
int DzpkUninit();

/*初始化*/
int DzpkInit();

/*服务器人数日志*/
int DzpkServerStatus(int nServerID,int nServerType,int nSocketCount,int nUserCount);

#endif //__MAINDZPK_H__
