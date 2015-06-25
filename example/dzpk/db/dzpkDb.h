#ifndef __DZPKDB_H__
#define __DZPKDB_H__

#include "../standard/custom.h"

#define DB_CHAR_LEN         50
#define DB_LONG_CHAR_LEN     250


/*读数据库需要的结构*/

/******************************************* 每天活动 ***********************************/
typedef struct tagCpsRecordDb
{
    tagCpsRecordDb()
    {
        memset(this,0,sizeof(tagCpsRecordDb));
    }
    int nRecordID;
    int nRecordType;
    char szGameName[DB_CHAR_LEN];               //记录名称
    int nStartTime;                             //开始时间
    int nStartNum;                              //比赛最少开始人数
    int nGameFee;                               //报名费用
    int nSmallBlind;                            //初始小盲注
    int nHandChip;                              //初始筹码
    int nAddBlindTime;                          //涨盲时间
    int nRankID;                                //奖励ID
    tagCpsRecordDb *pNext;                      //下一个
}CPS_RECORD_DB_T;

typedef struct tagCpsRecordDbHead
{
    tagCpsRecordDbHead()
    {
        memset(this,0,sizeof(tagCpsRecordDbHead));
    }
    CPS_RECORD_DB_T *pFirst;
    CPS_RECORD_DB_T *pLast;
}CPS_RECORD_DB_HEAD_T;

/******************************************涨盲注**************************************/
typedef struct tagCpsAddBlindDb
{
    tagCpsAddBlindDb()
    {
        memset(this,0,sizeof(tagCpsAddBlindDb));
    }
    int nID;
    int nType;
    int nValue;
    char szName[DB_CHAR_LEN];               //记录名称
    tagCpsAddBlindDb *pNext;
}CPS_ADD_BLIND_DB_T;

typedef struct tagCpsAddBlindDbHead
{
    tagCpsAddBlindDbHead()
    {
        memset(this,0,sizeof(tagCpsAddBlindDbHead));
    }
    CPS_ADD_BLIND_DB_T *pFirst;
    CPS_ADD_BLIND_DB_T *pLast;
}CPS_ADD_BLIND_DB_HEAD_T;

/******************************************奖励**************************************/
typedef struct tagCpsAwardDb
{
    tagCpsAwardDb()
    {
        memset(this,0,sizeof(tagCpsAwardDb));
    }

    int nId;
    char szRankName[DB_LONG_CHAR_LEN];      //奖励名称
    int nRankID;                            //奖励标识
    int nRank;                              //排第几名
    int nType;                              //奖励类型，如筹码
    char szReward[DB_LONG_CHAR_LEN];        //奖励实际东西，如筹码为数字
    int nStatue;
    tagCpsAwardDb *pNext;
}CPS_AWARD_DB_T;

typedef struct tagCpsAwardDbHead
{
    tagCpsAwardDbHead()
    {
        memset(this,0,sizeof(tagCpsAwardDbHead));
    }
    CPS_AWARD_DB_T *pFirst;
    CPS_AWARD_DB_T *pLast;
}CPS_AWARD_DB_HEAD_T;

/******************************************活动**************************************/
typedef struct tagSystemActivity
{
    tagSystemActivity()
    {
        memset(this,0,sizeof(tagSystemActivity));
    }
    int nID;
    char szName[DB_CHAR_LEN];               //活动名称
    int nStartTime;                         //开始时间
    int nEndTime;                           //结束时间
    int nOverTime;                          //彻底结束时间
    char szTeam1[DB_CHAR_LEN];              //预留参数
    char szTeam2[DB_CHAR_LEN];              //预留参数
    char szTeam3[DB_CHAR_LEN];              //预留参数
    char szTeam4[DB_CHAR_LEN];              //预留参数
    char szTeam5[DB_CHAR_LEN];              //预留参数
    char szTeam6[DB_CHAR_LEN];              //预留参数
    char szTeam7[DB_CHAR_LEN];              //预留参数
    char szTeam8[DB_CHAR_LEN];              //预留参数
    char szTeam9[DB_CHAR_LEN];              //预留参数
    char szTeam10[DB_CHAR_LEN];             //预留参数
    tagSystemActivity *pNext;
}SYSTEM_ACTIVITY_T;

typedef struct tagSystemActivityHead
{
    tagSystemActivityHead()
    {
        memset(this,0,sizeof(tagSystemActivityHead));
    }
    SYSTEM_ACTIVITY_T *pFirst;
    SYSTEM_ACTIVITY_T *pLast;
}SYSTEM_ACTIVITY_HEAD_T;

/******************************************任务**************************************/
typedef struct tagSystemTaskInfo
{
    tagSystemTaskInfo()
    {
        memset(this,0,sizeof(tagSystemTaskInfo));
    }
    int nID;                                  //id
    int nType;                                //任务类型
    char szTitle[DB_CHAR_LEN];                //任务名陈
    int nTotal;                               //总公需要多少局
    int nAward;                               //奖励ID
    tagSystemTaskInfo *pNext;
}TASK_BASE_T;

typedef struct tagSystemTaskHead
{
    tagSystemTaskHead()
    {
        memset(this,0,sizeof(tagSystemTaskHead));
    }
    TASK_BASE_T *pFirst;
    TASK_BASE_T *pLast;
}SYSTEM_TASK_HEAD_T;


/************************************************************************************/
/*德州扑克所有数据库操作*/

/*获得数据库用户信息*/
int dzpkGetDbUserInfo(user &newuser,char *pUserAccount,int nSocketID,int nRoomID);

/*获取数据库用户等级礼包信息*/
int dzpkGetDbLevelGifts(level_gifts &lgifts,string user_id);


/*更新数据库用户等级礼包信息*/
int dzpkUpdateDbLevelGifts(string user_id, long chip_account,int level_gift);

/*获取数据库用户下一个倒计时宝箱信息*/
int dzpkGetDbCountDown(string& chip_awards,string user_id,int&  next_cntid,int& next_rest_time);

/*读每天活动*/
int dzpkGetDbEveryDay(CPS_RECORD_DB_HEAD_T *pHead);

/*涨盲注数组*/
int dzpkGetDbAddBlind(CPS_ADD_BLIND_DB_HEAD_T *pHead);

/*读取奖励*/
int dzpkGetDbAward(CPS_AWARD_DB_HEAD_T *pHead);

/*更新锦标赛状态*/
int dzpkUpdateDbCpsStatus(int nID,int nStatus);

/*清除用户在房间里标识*/
int dzpkDbClearUserRoomMark();

/*更新等级礼包时间,和今天玩次数*/
int dzpkDbUpdateLevelGift(int nUserID,int nGiftLevel,int nCountTime,int nTodayPlayTimes);

/*更新用户筹码,差值*/
#define DZPK_USER_CHIP_CHANGE_PLAY_GAME         0    //普通玩牌
#define DZPK_USER_CHIP_CHANGE_PLAY_CPS          1    //锦标赛玩牌
#define DZPK_USER_CHIP_CHANGE_CPS_AWARD         2    //锦标赛奖励
#define DZPK_USER_CHIP_CHANGE_CPS_ENTRYFEE      3    //锦标赛退还报名费
#define DZPK_USER_CHIP_CHANGE_LUCK_HAND_CARD    4    //幸运手牌获得奖励
#define DZPK_USER_CHIP_CHANGE_CPS_FACE          5    //锦标赛发表情
#define DZPK_USER_CHIP_CHANGE_COUNT_DOWN        6    //倒计时间宝箱
int dzpkDbUpdateUserChip(int nUserID,long nChip,long &lUserChip,int nType,user *pUserInfo);

//更新玩家携带筹码数
void dzpkDbUpdateChipHand(int nUserID,int nChip);

//插入玩家牌局信息
void dzpkDbInsertPlayCard(user *pUser,dealer *pDealer,int nChip);

/*插如用户获得筹码*/
int dzpkDbInsertGetChip(int nUserID,long nChip,int nFromUserID,int nType);

/*判断用户是否有踢人卡*/
int dzpkDbUseProhibitCar(int nUserID,int *pToolCount);

/*更新用户背包时间*/
int dzpkDbUpdateBackpackTime(int nUserID);

/*查询用户vip*/
int dzpkDbGetUserMaxVIP(int nUserID,int &nVip);

#define DZPK_USER_BACKPACK_VIPI_1              66         //vip
#define DZPK_USER_BACKPACK_VIPI_2              65         //vip
#define DZPK_USER_BACKPACK_VIPI_3              64         //vip
#define DZPK_USER_BACKPACK_VIPI_4              63         //vip
#define DZPK_USER_BACKPACK_VIPI_5              62         //vip
#define DZPK_USER_BACKPACK_VIPI_6              59         //vip

#define DZPK_USER_DOUBLE_EXPERIENCE_CARD       61         //双倍经验卡
#define DZPK_USER_PROHIBIT_USER_CARD           60         //踢人卡
#define DZPK_USER_SEND_TOOL_CARD               93         //互动道具
#define DZPK_USER_FREE_FACE_SHOW               95         //免费表情使用权

/*读用户背包东西*/
int dzpkDbReadUserBackpack(user *pUser);

/*给荷官小费写日志和更新用户筹码数*/
int dzpkGiveTipUpdateData(int roomType, int userId);


/*判断用户是否有互动道具*/
int dzpkDbUseSendToolCard(int nUserID,int *pCurCount);

/*获得时间秒数*/
int dzpkDbGetTimeToIn(string sTime);

/*获得时间秒数*/
int dzpkDbGetTimeDayToInt(string sTime);

/*增加一张抽奖券*/
int dzpkDbAddAwardChangeCard(int nUserID,int nAddCards);

/*重新读用户信息*/
int dzpkDbReadUserInfo(user *pUser,int nType);

/*记录用户玩牌时间*/
int dzpkDbUserPlayTime(int nUserID,int nRoomID,int nEnterTime,int nGames,int nChips,int nBlind,int nPercent);

/*读取活动参数*/
int dzpkReadActivityInfo(SYSTEM_ACTIVITY_HEAD_T *pHead);

/*时间类道具奖励*/
int dzpkDbAwardTimeTool(int nUserID,int nMallID,int nSecond);

/*读取每日任务*/
int dzpkReadEveryDayTaskInfo(SYSTEM_TASK_HEAD_T *pHead);

/*更新一个任务的个数*/
int dzpkUpdateTaskStage(int nRecordID,int nCurrent);

/*增加任务*/
int dzpkAddTask(int nUserID,int nTaskID,int nCurrent,int nTotal);

/*读用户任务*/
int dzpkDbReadUserTask(user *pUser);

/*幸运手牌日志*/
int dzpkDbWriteLuckCardLog(int nType,int nRoomID,int nUserID,int nChip,char *pTooltip,long nCurChipCount);

/*更新房间显示标识*/
int dzpkDbReadRoomVisible();

/*背包日志*/
int dzpkDbWritePacketLog(int nType,int nRoomID,int nUserID,int nCount,char *pTooltip,long nCurToolCount);

/*服务器人数日志*/
int dzpkDbWriteOnlineNumLog(int nServerID,int nServerType,int nSocketCount,int nUserCount);

/*查询玩牌次数可以奖励的金豆数*/
bool queryAwardJinDou(int awardId, int &award, int &num);

/*记录用户获取的金豆*/
int addUserJinDou(int userId, int jinDouNum, int roomId, int smallBind, int playtime);

/*查询玩牌次数可以奖励的金豆数*/
bool addGetChestsLog(int userid, int roomid, int smallBlind, int playtime);

/*记录用户比赛结果*/
bool insertGameRank(int recoredId, int userId, int rank, int awardType);

/*管理员禁用玩家，修改数据库玩家禁用标志*/
bool dzpkDbLoginDisable(int user_id);







#endif //__DZPKDB_H__
