#ifndef __COMMON_H__
#define __COMMON_H__
/*
**************************************************************
** 公司：北京中清龙图网络技术有限公司
** 创建时间：2013.10.30
** 版本：1.0
** 说明：核心部分公用包含头文件
**************************************************************
*/

//#include "../core/network/mainNet.h"
#include "mainNet.h"
#include "mainLoger.h"

/*游戏一些参数define*/
#define _DEBUG				//是否为debug版本

/*****************************德州扑克错误码***************************/

#define      CMD_ERR_CODE                   999999    //错误
#define      ERR_TIP_CHIPLESS               211001    //筹码不足
#define      ERR_NO_SEAT                    211002    //没有座位可坐
#define      ERR_LEVEL_LIMITED              211003    //进入房间等级限制
#define      ERR_CPS_HAD_START              211004    //进标赛已经开始
#define      ERR_ENTER_ROOM_ERROR           211005    //进入房间错误
#define      ERR_PROHIBIT_ENTER_ROOM        211006    //玩家禁止进当前房间
#define      ERR_SEND_TOOL_NO_CARD          211007    //玩家发送互动道具  没有互动道具卡

/*****************************德州扑克命令***************************/

#define      CMD_DZPK_TIMER       370000   //定时器
#define      CMD_ENTER_ROOM       370001   //进入房间
#define      CMD_SEAT_DOWN        370002   //坐下
#define      CMD_ADD_CHIP         370003   //加注
#define      CMD_DRAW_CARD        370004   //翻牌
#define      CMD_LEAVE_SEAT       370005   //离座
#define      CMD_DEAL_CARD        370006   //发牌
#define      CMD_BLIND_SEAT       370007   //盲注座位
#define      CMD_NEXT_TURN        370008   //轮到下一个动作的玩家
#define      CMD_WIN_LIST         370009   //赢牌列表
#define      CMD_QUICK_SEAT       370010   //快速坐下
#define      CMD_QUICK_START      370011   //快速开始
#define      CMD_AUTO_BUY         370012   //买入筹码

#define      CMD_SPEEK_WORDS      370020   //说话
#define      CMD_DISPALY_OVER     370021   //牌局展示完毕
#define      CMD_EXIT_ROOM        370022   //离开房间
#define      CMD_PRESENT_GIFT     370023   //批量赠送礼物
#define      CMD_PRESENT_CHIPS    370024   //赠送筹码
#define      CMD_ADD_FRIEND       370025   //添加牌友
#define      CMD_HEART_WAVE       370026   //心跳
#define      CMD_USER_CLOSE       370027    //用户关闭
#define      CMD_SEND_MSG_TO_ROOM 370028    //发送消息给房间用户
#define      CMD_SEND_MSG_USER    370029    //发送消息给房间特定用户

#define      CMD_GET_STANDBY      370030   //获取房间内旁观的玩家
#define      CMD_LAY_CARDS        370031   //手动亮牌
#define      CMD_RESET_CHIPS      370032   //手动补齐手上的筹码
#define      CMD_LEVEL_GIFTS      370033   //获取玩家等级礼包信息
#define      CMD_DEAL_GIFTS       370034   //赠送玩家等级礼包
#define      CMD_CNTD_GIFTS       370035   //领取倒计时宝箱

#define      CMD_CPS_COUNTDOWN    370040   //锦标赛倒计时
#define      CMD_CPS_RESULT       370041   //锦标赛结果
#define      CMD_CPS_VIEW         370042   //锦标赛是否转为旁观
#define      CMD_CPS_RANK_LIST    370043   //锦标赛获得列表
#define      CMD_CPS_ADD_BLIND    370044   //锦标赛涨盲注

#define      CMD_HIDE_LIGHT_RING  370060   //游戏结束隐藏光圈
#define      CMD_PROHIBIT_MID_RESULT    370061   //投票的中间结果
#define      CMD_RE_ENTER_TURN_ME       370062   //进入到我操作信息
#define      CMD_TODAY_PLAY_TIMES       370063   //今天玩多少盘
#define      CMD_AWARD_CHANGE_ONE       370064   //获得一张抽奖劵
#define      CMD_RE_READ_USER_PACK      370065   //告诉服务器重新读背包
#define      CMD_PROHIBIT_USER          370066   //发起投票禁止某个用户
#define      CMD_PROHIBIT_USER_VOTE     370067   //投票

#define      CMD_DZPK_NORMAL_ERROR      370068   //返回错误码
#define      CMD_ENTER_OTHER_ROOM       370069   //转移房间



/******************************* 一些共用结构 ************************/

#endif //__COMMON_H__
