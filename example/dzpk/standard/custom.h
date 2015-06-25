#ifndef CUSTOM_H_INCLUDED
#define CUSTOM_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>
#include <vector>
#include "../db/db_connect_pool.h"
#include "../../../com/mainNet.h"
#include "../include/json/json.h"

using namespace  std;



#define      CASE_ADD_CHIPS       1          //加注逻辑
#define      CASE_GIVE_UP         0          //弃牌逻辑

//#define      MAXBUF               1024     //缓存大小
//#define      MAXEPOLLSIZE         1000     //epoll支持最大连接数

//#define      HALL_SERVER_PORT     80      //大厅服务器端口
//#define      ROOM_SERVER_PORT     8006    //房间服务器端口

#define DZPK_ROOM_MAX_USER      9           //房间最多支持多少用户玩牌

typedef struct tagUserBackpack
{
    tagUserBackpack()
    {
        memset(this,0,sizeof(tagUserBackpack));
    }
    int nRecordID;                          //记录ID
    int nMallID;                            //商品ID
    int nEndTime;                           //到什么时候结束
}USER_BACK_PACK_T;

typedef struct tagUserTaskInfo
{
    tagUserTaskInfo()
    {
        memset(this,0,sizeof(tagUserTaskInfo));
    }
    int nID;                                //记录ID
    int nTaskID;                            //任务ID
    int nCurrent;                           //当前完成多少
    int nTotal;                             //总共需要完成多少
    int nCreateTime;                        //记录时间
    int nStatus;                            //任务的状态
}USER_TASK_INFO_T;

/*
*   用户结构体
*/
typedef struct  user
{
	user()
	{
        nReadBpFromDb = 0;
        nTodayPlayTimes = 0;
        nVipLevel = 0;
        nReadTaskFromDb = 0;
        lUserIP = 0;
		admin_kick_flag = 0;
	}

	int       socket_fd;  //连接句柄
	int      giftid;  //礼品ID
	string   mall_path;//商品路径
	string   level_title; //级别头衔
	string   user_id;  //用户ID
	string   user_account;  //用户帐号
	string   nick_name; //用户昵称
	int       dzpk_level;//级别
	long     dzpk_experience;//经验值
	int       vip_level;//会员等级
	int       level_gift;//当前可领取等级礼物的级别
	long    chip_account;//筹码账户
	long    coin_account;//筹码账户
	string   city;
	int      total_time;
	int      win_per;
	int      sex;      // 1: male  0:female
	string   rank_name; //头衔
	string   terminal_code;//渠道号
	string   last_ip;//最近登录IP
	string   head_photo_serial;  //头像序列号
	int       desk_chip;  //桌面筹码
	int       user_staus;  // 玩家状态：   0：旁观  1：坐下(等待下一局)   2： 加注 3：跟注 4：看牌   5:弃牌  6:全下  7：等待下注
	int       room_num; // 所在的房间号
	int       seat_num; // 分配的座位号
	vector<int> win_cards;//赢牌的牌型，第六位是虚拟值[0][1][2][3][4][虚拟值]

	vector<int> hand_cards;  // 手牌
	int        hand_chips; //带入的筹码数
	int        last_chips;//上一局手上的筹码数
	int        auto_buy;//游戏中自动买入  0: 不自动买入  1:自动买入
	int        max_buy;//自动按最大买入  0:自动按房间最小买入 1:自动按房间最大买入
	int        give_up_count;//系统帮助弃牌计数，超过2次自动被踢出座位
	int        disconnect_flg;//断线标记   1：连接正常  0：连接断开
	int        first_turn_blind;//大盲注首次手动操作标记
	int        reset_hand_chips; //重置手上筹码的数额
	int        nUserID;			//用户ID
	int       countdown_no;  //倒计时宝箱编号 1~7
	int       cd_rest_time; //当前倒计时宝箱领取剩余时间
	int       start_countdown; //开始倒计时的时间点
    vector<USER_BACK_PACK_T>  arryBackpack;     //用户背包
    int       nReadBpFromDb;                    //是否读取过背包数据
    int       nTodayPlayTimes;                  //今天玩多少盘
    int       nEnterTime;                       //用户进入房间时间
	int       nEnterRing;   //用户进入房间时的轮数
	int       nChipUpdate;  //用户在房间中游戏的筹码变更(包含扣除的台费)
    int       nVipLevel;                        //vip等级
    vector<USER_TASK_INFO_T>  arryTask;         //用户任务
    int       nReadTaskFromDb;                  //是否已经读取过
    long      lUserIP;                          //用户IP
    int       admin_kick_flag;                  //管理员踢人标志 1表示有效
}DZPK_USER_INFO_T;

/*德州扑克的用户结构 */
typedef struct tagDzpkUser:public SOCKET_INFO_T
{
	tagDzpkUser()
	{
		pUser = NULL;
	}
	DZPK_USER_INFO_T *pUser;
}DZPK_USER_T;

/*
*   边池结构体
*/
struct  edge_pool
{
    edge_pool()
    {
        nCanCompose = 0;
    }
    int       edge_height;  //阶高
    vector<int> share_seat; //共享座位号
    int       pool_type;    //1：主池 0：边池
    int       pool_id;
    int       nCanCompose;  //是否能和并
};


/*
*   DZTIMER结构体
*/
#define DZPK_TIMER_BUF_LEN	1024*4
struct  dztimer
{
	dztimer()
	{
		nUserID = -1;
		nTimerID = -1;
		memset(szUserAccount,0,sizeof(szUserAccount));
		nMsgLen = 0;
		memset(szMsg,0,DZPK_TIMER_BUF_LEN);
	}

	int       socket_fd;    //目标socket句柄

	char szUserAccount[50];
	int       room_num;  //目标房间号
	int       seat_num;   //目标座位号
	long      start_time;  //定时开始时间
	int       time_out;   // 超时时间
	int       timer_type; //定时器类型            1: 弃牌定时器          2: display over 定时器
	int nUserID;		//用户ID
	int nTimerID;		//定时器ID
	char szMsg[DZPK_TIMER_BUF_LEN];	//消息存起来
	int nMsgLen;			//消息长度
};




/*
*   赢家结构体
*/
struct  winer_pool
{

    int       seat_number;  //座位号
    vector<int> pool_id_list; //赢得边池号
    vector<int> persent_list;//赢得边池所占比例
};


/*
*   等级经验系统结构体
*/
struct  level_system
{
    int       ex_id;  //编号
    int       level;    //等级
    string   level_title; //称号
    long      exp_upgrade; //升级所需经验
    long      exp_total;  // 等级总经验
};

/*
*   等级礼包结构体
*/
struct  level_gifts
{
    int         level;  //等级
    string      chip_award;    //筹码奖励
    string      coin_award; //金币奖励
    string      gift_award; //互动道具奖励
    string      face_award;  //
    string      vip_award;  // VIP卡奖励
    string      stone_award;  // 宝石奖励
};

/*
*房间结构体
*
*/
struct room
{
    int  room_id; //房间ID
    string  room_name;    //房间名称
    int  current_seat_person;//当前在座位上的人数
    int  current_sideline_person;//当前在旁观的人数
    int  small_blind; //小盲注额
	int  charge_per; //台费百分比 台费=小盲注*台费百分比*2/100
    string  minimum_bring; //最小带入筹码
    string  maximum_bring; //最大带入筹码
    string  before_chip;  //必下房的前注筹码
    int  room_seat_number;//房间总的座位数
    int  room_type;    //房间场   1:保留值 2:游戏场  3:比赛场 4:贵宾房
    int  room_level_st;    //房间进入限制等级最小值
    int  room_level_nd;    //房间进入限制等级最大值
    int  bet_time;    //下注间隙   10秒   20秒
    string  host_ip;    //分配房间服务器主机IP
    string  port;   //端口
};

#define ROOM_TYPE_CPS   5       //锦标赛类型,大于5都为进标赛类型   6为淘汰赛

typedef struct tagNoInUser
{
    tagNoInUser()
    {
        memset(this,0,sizeof(tagNoInUser));
    }
    int nUserID;            //用户ID
    int nEndTime;           //结束时间
    char szActionName[50];  //发起的用户名
    char szTargetName[50];  //被踢的用户名
}NO_IN_USER_T;

typedef struct tagRoomVote
{
    int nUserID;
    int nResult;
}ROOM_VOTE_T;

#define ROOM_SEAT_DOWN_MAX_NUMBER       9
#define ROOM_PROHIBIT_USER_TIMEOUT      10 //秒
#define ROOM_PROHIBIT_USER_TIME         10*60
typedef struct tagProhibitInfo
{
    tagProhibitInfo()
    {
        memset(this,0,sizeof(tagProhibitInfo));
    }
    int nVoteID;                    //此次活动ID
    int nSendUserID;                //发起人ID
    int nVoteUserID;                //被投票ID
    ROOM_VOTE_T arryInfo[ROOM_SEAT_DOWN_MAX_NUMBER];
    int nArryInfoIndex;             //下标
    char szTooltip[100];            //成功后提示
    char szFirstTooltip[100];       //一开始提示
    int nStartTime;                 //开始时间
    char szActionName[50];  //发起的用户名
    char szTargetName[50];  //被踢的用户名
}PROHIBIT_INFO_T;

typedef struct tagForceLeaveUser
{
    tagForceLeaveUser()
    {
        memset(this,0,sizeof(tagForceLeaveUser));
    }
    int nUserID;            //用户ID
    char szActionName[50];  //发起的用户名
    char szTargetName[50];  //被踢的用户名
}FORCE_LEAVE_USER_T;

typedef struct tagLuckCardParam
{
    tagLuckCardParam()
    {
        memset(this,0,sizeof(tagLuckCardParam));
    }
    char szUserAccount[50]; //用户帐号
    int nAwardChip;         //奖励筹码
    int nUserID;            //用户ID
    char szTooltip[200];    //提示
    char szHeadPhoto[200];    //玩家头像
}LUCK_CARD_PARAM_T;

/*
*  荷官结构体
*
*/
struct  dealer
{
	dealer()
	{
		nActionUserID = -1;
        nTimeDelete = 0;
        before_chip = 0;
	}
	int  dealer_id; //荷官ID，即房间编号
	int  step;        //牌局的步骤  0： 未开始  1：发底牌   2:翻三张  3:翻第四张  4：翻最后一张（比大小）
	int  card_dun[52]; //荷官墩上的牌  每张牌由三位整数构成 最左位代表花色 例如 103（方片3） 312（红桃Q)
	int  desk_card[5]; //桌面公共牌
	int  all_desk_chips;    //桌面总筹码数
	vector<edge_pool> edge_pool_list;//边池列表
	int  room_seat_num;   //房间类型： 5：五人房   9:九人房
	int  turn;          // 轮到谁动作  按照座位号
	int  look_flag;     //本轮是否能看牌   0:  能看牌   1: 不能看牌
	int  add_chip_manual_flg;//手动加注标记  0：默认加注  1：玩家手动加注
	int  follow_chip;    //当前最小跟注
	int  max_follow_chip;//该房间最大跟注
	int  max_buy_chip;  //该房间最大可购买筹码数量
	int  min_buy_chip;  //该房间最小可购买的筹码数量
	int  before_chip;  //必下房的前注筹码
	string   room_name;  //房间名称
	int  small_blind;// 盲注的最小注金
	int  charge_per; //台费百分比 台费=小盲注*台费百分比*2/100
	int  seat_bank;    //庄家的座位号
	int  seat_small_blind;    //小盲注的座位号
	int  seat_big_blind;      //大盲注的座位号
	int room_type;//房间场     2：普通房  3：比赛房   4：VIP 房 5:锦标赛 9:宝箱房  10: 金豆房
	int room_level_st;//房间进入限制最小级别
	int room_level_nd;//房间进入限制最大级别
	int bet_time;  //每次加注等待时长     普通房 20秒  快速房 10秒
	int current_player_num;//当前在玩人数
	vector<user>  players_list; //玩家队列
	vector<user>  leave_players_list; //离开或站起玩家队列
	class CMutex Mutex;
	int nRing;			//第几轮
	int nActionUserID;		//上一次操作玩家
	int nVecIndex;			//在数组中的下标
    int nCpsRecordID;       //锦标赛记录ID,被锦标赛那个记录所使用
    int nTurnMinChip;       //当前加注最小值
    int nTurnMaxChip;       //当前加注最大值
    vector<NO_IN_USER_T>    arryNoInUser;       //不能进用户
    vector<FORCE_LEAVE_USER_T>             arryForceLeaveUser; //被迫离开玩家
    vector<PROHIBIT_INFO_T> arryVote;           //用户投票情况
    vector<LUCK_CARD_PARAM_T> arryLuckCard;     //幸运手牌提示
    int nTimeDelete;        //当前是否显示  0显示
    int nStepOver;  //牌局结束状态STEP =4时 标记  0：还未进入比牌结算,任然可以加注  1:已经跟满注，不能进行加注
};

typedef struct tagDealer
{
	dealer *pDealer;
}DEALER_T;

/*数据库结构*/
typedef struct tagDzpkDbInfo
{
    tagDzpkDbInfo()
    {
        memset(this,0,sizeof(tagDzpkDbInfo));
    }
    char szIP[50];
    int nPort;
    char szLoginName[50];
    char szLoginPwd[50];
    char szDbName[50];
}DZPK_DB_INFO_T;

void  strrever(char *s);
char* convert(int value, char* buffer);
char* itoa(int i, int radix);
void split(const string& src, const string& separator, vector<string>& dest);
int setnonblocking(int sockfd);
long  get_current_time();
int  next_player(int current_seat, vector<dealer *>&dealers_vector,int room_number,int nSeatCount = 9);
void  edge_pool_computer(vector<dealer *>&  dealers_vector,int room_number,vector<edge_pool>& edge_pool_temp );
void  edge_pool_special(vector<dealer *>&  dealers_vector,int room_number,vector<edge_pool>& edge_pool_temp );
void  edge_pool_normal(vector<dealer *>&  dealers_vector,int room_number,vector<edge_pool>& edge_pool_temp );
void  computer_card_value(vector<dealer *>&  dealers_vector,int room_number);
int   max_card_seat( vector<int>& share_seat,vector<dealer*>&  dealers_vector ,int room_number);
void  virtual_card_value( int desk_card[5], vector<int>& hand_cards,vector<int>& win_cards);
void  sort_cards(vector<int>& card_temp );

void add_chip_branch( int nUserID,vector<dealer *>& dealers_vector,int room_number,string user_id,int chips_value,int seat_number,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,int nType);
void give_up_branch( int nUserID,vector<dealer *>& dealers_vector,int room_number,string user_id,int chips_value,int seat_number,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,int nType);
void do_next_work( int nUserID,vector<dealer *>& dealers_vector,int room_number,string user_id,int chips_value,int seat_number,ConnPool *connpool,vector<level_system>  level_list ,DZPK_USER_T *pSocketUser,int nType);
int judge_flush( vector<int>& card_temp,vector<int>& win_cards );
int judge_straight( vector<int>& card_temp,vector<int>& win_cards );
int judge_4ofking( vector<int>& card_temp,vector<int>& win_cards );
int judge_3ofking( vector<int>& card_temp,vector<int>& win_cards );
int judge_2ofdouble( vector<int>& card_temp,vector<int>& win_cards );
int judge_1ofdouble( vector<int>& card_temp,vector<int>& win_cards );
void computer_winer_card(vector<dealer *>&  dealers_vector,int room_number);
void computer_winer_card(vector<dealer *>&  dealers_vector,int room_number,vector<winer_pool>&  winder_list);
long computer_exp_level( long& experience, int& level,vector<level_system>  level_list);
long get_total_exp_level( int& level,vector<level_system>  level_list);
void update_user_exp_level(string user_account,int dzpk_level,long dzpk_experience,ConnPool *connpool);
//void update_user_chip_account(string user_account,long chip_account,ConnPool *connpool);
void update_user_persition(string user_account,int room_num,ConnPool *connpool);
void update_session_time(string user_account,ConnPool *connpool);
void update_user_best_win_card(vector<dealer *>&  dealers_vector,int room_number,ConnPool *connpool);
string GetString(int n);
int GetInt(string s);
/*获得随机数*/
int DzpkGetRandom(int nMax);
/*更新房间人数*/
void DzpkUpdateRoomPerson(int nRoomID,ConnPool *connpool);
/*需要显示手牌玩家*/
int StdPutResultShowHandCards(dealer *pRoom,Json::Value  &hand_cards_all,vector<winer_pool>  winder_list,vector<edge_pool> edge_pool_temp);
/*隐藏光圈*/
int StdStopHideLightRing(dealer *pRoom);
string GetlongString(long n);

#endif
