#ifndef __DZPKHEAD_H__
#define __DZPKHEAD_H__
#include <string.h>
#include <stdio.h>

/* 命令结构*/

#define HEAD_CHAR_LEN	50			        //短字符长度
#define HEAD_LONG_CHAR_LEN	250		        //长字符长度
#define HEAD_LONG_LONG_CHAR_LEN	1024*3	    //特长字符长度
#define HEAD_DESK_POOL_COUNT	10		    //产生边池最大个数
#define HEAD_ROOM_USER_COUNT	30		    //房间最多可以进多少人

/*公共*/
typedef struct tagDzpkMessage
{
	char msg_head[HEAD_CHAR_LEN];		    //前三个为德州扑克包标识
	char game_number[HEAD_CHAR_LEN];
	char area_number[HEAD_CHAR_LEN];
	int act_commond;			            //命令号
	int room_number;			            //房间ID

	/*下面变量是为写进数据库准备的,这样可以保存牌局*/
	int nRoomID;				            //是那个房间
	int nTickTime;				            //产生时间
	int nUse;				                //是否被房间接受（比如定时器比用户先到，用户包就不被房间接收）
	int arryMsgUser[HEAD_ROOM_USER_COUNT];  //信息跟那些用户有关，接收信息为接收到那个用户信息(接收肯定只有一个)，如果是发送为发送给那些用户
	int arryMsgUserIndex;			        //当前数组个数
	char szPacket[HEAD_LONG_LONG_CHAR_LEN];	//网络格式，这样方便存储
}DZPK_MESSAGE_T;

/*************************************** 进房间请求结构************************************/
/*CMD_ENTER_ROOM       370001   */
typedef struct tagEnterRoomReq:public DZPK_MESSAGE_T
{
	tagEnterRoomReq()
	{
		memset(this,0,sizeof(tagEnterRoomReq));
	}
	char act_user_id[HEAD_CHAR_LEN];	    //用户帐号
    int nChangeRoom;                        //是否是换房间
    int nChangeRoomID;                      //要换到那个房间
}ENTER_ROOM_REQ_T;

/***************************************  进入房间返回************************************/

/*边池信息*/
typedef struct tagRoomDeskPool
{
	int pool;				                //边池里总注金
	int type;				                //池子的种类：1: 主池 0:边池
	int id;					                //池子的id：0: 主池 >1:边池
}ROOM_DESK_POOL_T;

/*房间信息*/
typedef struct tagRoomInfo
{
	int step;				                //当前房间状态 (牌局的步骤  0： 未开始  1：发底牌   2:翻三张  3:翻第四张  4：翻最后一张（比大小）)
	int desk_card[5];			            //公共牌
	int all_desk_chips;			            //桌面总筹码
	int room_type;				            //房间场次类别   2：游戏场  3：比赛场  10: 金豆房
	int room_seat_num;			            //房间内座位数     5人房  9人房
	int bet_time;				            //操作时间
	int turn;				                //当前到那个座位
	int max_buy_chip;			            //最大买入
	int min_buy_chip;			            //最小买入
	char room_name[HEAD_CHAR_LEN];		    //房间名称
	ROOM_DESK_POOL_T desk_pool[HEAD_DESK_POOL_COUNT];	//边池信息
	int nDesk_poolIndex;			        //边池下表，不用打进包 *****************/
	int min_blind;				            //小盲注位置
	int big_blind;				            //大盲注位置
	int banker;				                //庄加位置
}ROOT_INFO_T;

/*房间用户信息*/
typedef struct tagRoomUserInfo
{
	int user_id;				            //用户ID
	char user_account[HEAD_CHAR_LEN];	    //用户名
	char nick_name[HEAD_CHAR_LEN];		    //用户昵称
	int dzpk_level;				            //用户等级
	int dzpk_experience;			        //用户经验
	int vip_level;				            //用户vip等级
	int chip_account;			            //筹码账户
	int coin_account;			            //龙币账户
	char city[HEAD_CHAR_LEN];		        //用户所在城市
	int total_time;				            //玩总次数
	int win_per;				            //
	char rank_name[HEAD_CHAR_LEN];		    //头衔
	char head_photo_serial[HEAD_LONG_CHAR_LEN];	//头像序列号
	int desk_chip;				            //当前这一轮丢到桌面的筹码
	int sex;				                //性别  1男 0女
	int user_staus;				            //用户状态：   0：旁观  1：坐下(等待下一局)   2： 加注 3：跟注 4：看牌   5:弃牌  6:全下  7：等待下注
	int room_num;				            //所在的房间号
	int seat_num;				            //分配的座位号
	int hand_chips;				            //手上有多少筹码
	int giftid;				                //礼品ID
	char mall_path[HEAD_LONG_CHAR_LEN];	    //商品路径
	char level_title[HEAD_CHAR_LEN];	    //级别头衔
}ROOM_USER_INFO_T;

/*进入房间返回*/
/*CMD_ENTER_ROOM       370001   */
typedef struct tagEnterRoomRes:public DZPK_MESSAGE_T
{
	tagEnterRoomRes()
	{
		memset(this,0,sizeof(tagEnterRoomRes));
		sprintf(msg_head,"dzpk");
		sprintf(game_number,"dzpk");
		sprintf(area_number,"normal");
	}
	char act_user_id[HEAD_CHAR_LEN];	    //用户帐号
	ROOT_INFO_T dealer_property;		    //房间信息，结构在上面
	int at_game;				            //该玩家是否在游戏中   1是 0否
	ROOM_USER_INFO_T player_list[HEAD_ROOM_USER_COUNT];	//房间用户信息
	int nPlayer_ListIndex;			        //房间用户数量，不用打进包  **********************/
	ROOM_USER_INFO_T watcher_list[HEAD_ROOM_USER_COUNT];	//房间旁观用户信息
	int nWatcher_ListIndex;			        //房间旁观用户数量，不用打进包  **********************/
}ENTER_ROOM_RES_T;

/***************************************  坐下请求************************************/
/*CMD_SEAT_DOWN */
typedef struct tagSeatDownReq:public DZPK_MESSAGE_T
{
	tagSeatDownReq()
	{
		memset(this,0,sizeof(tagSeatDownReq));
	}
	char act_user_id[HEAD_CHAR_LEN];	    //玩家账号
	int seat_number;			            //坐下时选定的座位号 0：表示快速坐下，由服务器找到座位并返回
	int hand_chips;				            //命令码，整型
	int auto_buy;				            //自动买入    1：开启     0：关闭
	int max_buy;				            //自动按最大带入买入  1：按最大买入   0：按最小买入
}SEAT_DOWN_REQ_T;

/***************************************  坐下返回************************************/
/*CMD_SEAT_DOWN 错误返回 */
typedef struct tagSeatDownResError:public DZPK_MESSAGE_T
{
	tagSeatDownResError()
	{
		memset(this,0,sizeof(tagSeatDownResError));
		sprintf(msg_head,"dzpk");
		sprintf(game_number,"dzpk");
		sprintf(area_number,"normal");
	}
	char act_user_id[HEAD_CHAR_LEN];	    //用户帐号
	int err_code;				            //错误码
}SEAT_DOWN_RES_ERROR_T;

/*CMD_SEAT_DOWN 正确返回*/
typedef struct tagSeatDownRes:public DZPK_MESSAGE_T
{
	tagSeatDownRes()
	{
		memset(this,0,sizeof(tagSeatDownRes));
		sprintf(msg_head,"dzpk");
		sprintf(game_number,"dzpk");
		sprintf(area_number,"normal");
	}
	char act_user_id[HEAD_CHAR_LEN];	    //用户帐号
	int seat_number;			            //座位号	1--9
	ROOM_USER_INFO_T player_property;	    //坐下用户信息
}SEAT_DOWN_RES_T;


/***************************************  开局前命令：自动买入、强制站起  ************************************/
/*自动买入*/
/*CMD_AUTO_BUY*/
typedef struct tagAutoBuy:public DZPK_MESSAGE_T
{
	tagAutoBuy()
	{
		memset(this,0,sizeof(tagAutoBuy));
		sprintf(msg_head,"dzpk");
		sprintf(game_number,"dzpk");
		sprintf(area_number,"normal");
	}
	char act_user_id[HEAD_CHAR_LEN];	    //用户帐号
	char nick_name[HEAD_CHAR_LEN];		    //用户昵称
	int chip_value;				            //购买筹码
	int seat_number;			            //座位号
}AUTO_BUY_T;

/*强制站起*/
/*CMD_LEAVE_SEAT*/
typedef struct tagLeaveSeat:public DZPK_MESSAGE_T
{
	tagLeaveSeat()
	{
		memset(this,0,sizeof(tagLeaveSeat));
		sprintf(msg_head,"dzpk");
		sprintf(game_number,"dzpk");
		sprintf(area_number,"normal");
	}
	int seat_number;			            //座位号
	char act_user_id[HEAD_CHAR_LEN];	    //用户帐号
	char nick_name[HEAD_CHAR_LEN];		    //用户昵称
	int chip_account;			            //筹码账户
	int force_exit;				            //离开类型
}LEAVE_SEAT_T;


/*注释*/

/***************************************  发牌  ************************************/

#endif //__DZPKHEAD_H__
