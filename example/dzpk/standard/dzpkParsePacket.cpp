/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：网络数据封装
**************************************************************
*/

#include "dzpkParsePacket.h"
#include "../../../com/common.h"
#include "dzpkCustom.h"
#include "dzpkActivity.h"
#include "../db/dzpkDb.h"
#include "../Start.h"


/*******************************************
 * 获得进入房间数据
 * pInfo            //解释出来结构
 * msg_commond      //收到数据
 * return           //成功返回0 失败放回 -1
 ******************************************/
int dzpkGetEnterRoomPack(ENTER_ROOM_REQ_T *pInfo,Json::Value msg_commond)
{
    int nRet = -1;
    if(pInfo)
    {
        pInfo->room_number = atoi(msg_commond["room_number"].asString().c_str());       //命令码，整型
        memcpy(pInfo->act_user_id,(msg_commond["act_user_id"].asString()).c_str(),strlen((msg_commond["act_user_id"].asString()).c_str()));           //用户帐号
        pInfo->nChangeRoom = 0;
        pInfo->nChangeRoomID = 0;

        nRet = 0;
    }
    return nRet;
}

/******************************************
 * 将进入房间返回数据打包
 * pInfo            //要打进去的结构
 * pBuf             //获得的网络数据
 * nBufLen          //传进来是buf长度，成功的话传出去为实际长度
 * return           //成功返回0，失败返回-1
 *****************************************/
int dzpkPutEnterRoomPack(ENTER_ROOM_RES_T *pInfo,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    return nRet;
}

/******************************************
 * 将进入房间返回数据打包
 * pRoom            //房间指针
 * pUserAccount     //用户帐号
 * nUserID          //用户ID
 * pBuf             //获得的网络数据
 * nBufLen          //传进来是buf长度，成功的话传出去为实际长度
 * return           //成功返回0，失败返回-1 ,bufLen 长度不够 -2
 *****************************************/
int dzpkPutEnterRoom(dealer *pRoom,char *pUserAccount,int nUserID,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    if(pRoom && pBuf && pUserAccount)
    {
        //根据房间号获取该房间所有玩家的信息
        dealer &onedealer = *pRoom;

        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  dealer_property;
        Json::Value  arry_deskcard;
        Json::Value  edge_desk_pool;
        Json::Value  pool_st;

        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_user_id"]=pUserAccount;
        msg_body["act_commond"]=CMD_ENTER_ROOM;
        msg_body["before_chip"]=pRoom->before_chip;
        msg_body["result"]=0;

        dealer_property["step"]=onedealer.step;
        unsigned int show_deskcard = 0;
        switch(onedealer.step){
            case 2:show_deskcard = 3;
                break;
            case 3: show_deskcard = 4;break;
            case 4: show_deskcard = 5;break;
        }

        for(unsigned int i= 0;i<5;i++){
            if(i<show_deskcard){
                arry_deskcard.append(onedealer.desk_card[i]);
            }else{
                arry_deskcard.append(0);
            }
        }

        dealer_property["desk_card"]=arry_deskcard;
        dealer_property["all_desk_chips"]=onedealer.all_desk_chips;
        dealer_property["room_type"]=onedealer.room_type;//房间场次类别   2：游戏场  3：比赛场
        dealer_property["room_seat_num"]=onedealer.room_seat_num;  //房间内座位数     5人房  9人房
        dealer_property["bet_time"]=onedealer.bet_time;
        dealer_property["turn"]=onedealer.turn;
        dealer_property["max_buy_chip"]=onedealer.max_buy_chip;
        dealer_property["min_buy_chip"]=onedealer.min_buy_chip;
        dealer_property["room_name"]=onedealer.room_name;

        for(unsigned int i=0;i<onedealer.edge_pool_list.size();i++){
            int pool=onedealer.edge_pool_list.at(i).edge_height *  onedealer.edge_pool_list.at(i).share_seat.size();
            for(unsigned int j=0;j<onedealer.edge_pool_list.at(i).share_seat.size();j++){
                if(  onedealer.edge_pool_list.at(i).share_seat.at(j) <=0 ){
                    pool+= onedealer.edge_pool_list.at(i).share_seat.at(j);
                }
            }
            pool_st["pool"]=pool;//池子里总的注金数
            pool_st["type"]=(onedealer.edge_pool_list.at(i)).pool_type;//池子的种类：1: 主池 0:边池
            pool_st["id"]=(onedealer.edge_pool_list.at(i)).pool_id;//池子的id：0: 主池 >1:边池
            edge_desk_pool.append(pool_st);

        }
        dealer_property["desk_pool"]=edge_desk_pool;
        dealer_property["min_blind"]=onedealer.small_blind;
        dealer_property["small_blind"]=onedealer.seat_small_blind;
        dealer_property["big_blind"]=onedealer.seat_big_blind;
        dealer_property["banker"]= onedealer.seat_bank; //庄家位置
        msg_body["dealer_property"]=dealer_property;
        //反馈该玩家是否在座位上
         for(unsigned int p=0;p< pRoom->players_list.size();p++)
         {
            if(  (pRoom->players_list).at(p).nUserID == nUserID )
            {
                if(  (pRoom->players_list).at(p).user_staus >= 1 )
                {
                    msg_body["at_game"]="1"; //正在 游戏中
                }
                else
                {
                    msg_body["at_game"]="0";//旁观
                }
                 msg_body["countdown_no"]= (pRoom->players_list).at(p).countdown_no;
                 msg_body["cd_rest_time"]= (pRoom->players_list).at(p).cd_rest_time;
                 msg_body["today_play_times"]= (pRoom->players_list).at(p).nTodayPlayTimes;
                 msg_body["countdown_max_level"]= g_nSystemCountDownMaxLevel;

                int nCountTimes = -1;
                nCountTimes = StdGetNextPlayTimes((pRoom->players_list).at(p).nTodayPlayTimes);
                msg_body["count_times"]=nCountTimes;

                //免费表情卡
                int nOverTime = -1;
                if(StdIfTimeToolCard(&pRoom->players_list.at(p),DZPK_USER_FREE_FACE_SHOW,&nOverTime) == 0)
                {
                    msg_body["faces_card_times"]=nOverTime;
                }
                else
                {
                    msg_body["faces_card_times"]=-1;
                }
            }
         }

        Json::Value  player_seatdown;
        Json::Value  player_standby;
        Json::Value  player_list;
        Json::Value  watcher_list;
        user  oneplayer;

        for(unsigned int i=0; i< onedealer.players_list.size();i++)
        {
                oneplayer = onedealer.players_list.at(i);

                if(oneplayer.nUserID == nUserID)
                {
                    Json::Value  hand_cards;
                    if(oneplayer.user_staus == 2 ||oneplayer.user_staus == 3 ||oneplayer.user_staus == 4 ||oneplayer.user_staus == 5 ||oneplayer.user_staus == 6)
                    {
                        if(oneplayer.hand_cards.size() >= 2)
                        {
                            hand_cards.append(oneplayer.hand_cards[0]);
                            hand_cards.append(oneplayer.hand_cards[1]);
                        }
                    }
                    msg_body["hand_cards"]=hand_cards;

                }

                if(oneplayer.user_staus>=1 && oneplayer.seat_num >0)
                {
                    player_seatdown["user_id"]=oneplayer.user_id;
                    player_seatdown["user_account"]=oneplayer.user_account;
                    player_seatdown["nick_name"]=oneplayer.nick_name;
                    player_seatdown["dzpk_level"]=oneplayer.dzpk_level;
                    player_seatdown["dzpk_experience"]=itoa(oneplayer.dzpk_experience,10);
                    //player_seatdown["vip_level"]=oneplayer.vip_level;
                    if( oneplayer.dzpk_level >=  oneplayer.level_gift ){ //可以领取等级礼包
                        player_seatdown["level_gift"]="0";
                    }else{
                        player_seatdown["level_gift"]="1";
                    }
                    player_seatdown["chip_account"]=itoa(oneplayer.chip_account,10);
                    //player_seatdown["chip_account"]=GetlongString(oneplayer.chip_account);
                    player_seatdown["coin_account"]=itoa(oneplayer.coin_account,10);
                    player_seatdown["city"]=oneplayer.city;
                    player_seatdown["total_time"]=itoa(oneplayer.total_time,10);
                    player_seatdown["win_per"]=itoa(oneplayer.win_per,10);
                    player_seatdown["rank_name"]=oneplayer.rank_name;
                    player_seatdown["head_photo_serial"]=oneplayer.head_photo_serial;
                    player_seatdown["desk_chip"]=oneplayer.desk_chip;
                    player_seatdown["sex"]=oneplayer.sex;
                    player_seatdown["user_staus"]=oneplayer.user_staus;
                    player_seatdown["room_num"]=oneplayer.room_num;
                    player_seatdown["seat_num"]=oneplayer.seat_num;
                    player_seatdown["hand_chips"]=oneplayer.hand_chips;
                    player_seatdown["giftid"]=oneplayer.giftid;
                    player_seatdown["mall_path"]=oneplayer.mall_path;
                    player_seatdown["level_title"]=oneplayer.level_title;
                    player_seatdown["vip_level"]=oneplayer.nVipLevel;

                    player_list.append(player_seatdown);
                 }else if( oneplayer.user_staus ==0 && oneplayer.seat_num ==0 ){
                    player_standby["user_id"]=oneplayer.user_id;
                    player_standby["user_account"]=oneplayer.user_account;
                    player_standby["nick_name"]=oneplayer.nick_name;
                    player_standby["dzpk_level"]=oneplayer.dzpk_level;
                    player_standby["dzpk_experience"]=itoa(oneplayer.dzpk_experience,10);
                    //player_standby["vip_level"]=oneplayer.vip_level;
                    if( oneplayer.dzpk_level >=  oneplayer.level_gift ){ //可以领取等级礼包
                        player_standby["level_gift"]="0";
                    }else{
                        player_standby["level_gift"]="1";
                    }
                    player_standby["chip_account"]=itoa(oneplayer.chip_account,10);
                    //player_standby["chip_account"]=GetlongString(oneplayer.chip_account);
                    player_standby["coin_account"]=itoa(oneplayer.coin_account,10);
                    player_standby["city"]=oneplayer.city;
                    player_standby["total_time"]=itoa(oneplayer.total_time,10);
                    player_standby["win_per"]=itoa(oneplayer.win_per,10);
                    player_standby["rank_name"]=oneplayer.rank_name;
                    player_standby["head_photo_serial"]=oneplayer.head_photo_serial;
                    player_standby["desk_chip"]=oneplayer.desk_chip;
                    player_standby["sex"]=oneplayer.sex;
                    player_standby["user_staus"]=oneplayer.user_staus;
                    player_standby["room_num"]=oneplayer.room_num;
                    player_standby["seat_num"]=oneplayer.seat_num;
                    player_standby["hand_chips"]=oneplayer.hand_chips;
                    player_standby["giftid"]=oneplayer.giftid;
                    player_standby["mall_path"]=oneplayer.mall_path;
                    player_standby["level_title"]=oneplayer.level_title;
                    player_standby["vip_level"]=oneplayer.nVipLevel;


                    watcher_list.append(player_standby);


            }
        }

        msg_body["player_list"]=player_list;
        msg_body["watcher_list"]=watcher_list;

        msg_body["room_type"]=pRoom->room_type;

        string   str_msg =   writer.write(msg_body);
        if(nBufLen >= (int)strlen(str_msg.c_str()))
        {
            nBufLen = strlen(str_msg.c_str());
            memcpy(pBuf,str_msg.c_str(),strlen(str_msg.c_str()));
            nRet = 0;
        }
        else
        {
            nRet = -2;
        }
    }
    return nRet;
}

/*******************************************
 * 获得坐下数据
 * pInfo            //解释出来结构
 * msg_commond      //收到数据
 * return           //成功返回0 失败放回 -1
 ******************************************/
int dzpkGetSeatDownPack(SEAT_DOWN_REQ_T *pInfo,Json::Value msg_commond)
{
    int nRet = -1;
    if(pInfo)
    {
        pInfo->room_number = atoi(msg_commond["room_number"].asString().c_str());       //命令码，整型
        memcpy(pInfo->act_user_id,(msg_commond["act_user_id"].asString()).c_str(),strlen((msg_commond["act_user_id"].asString()).c_str()));           //用户帐号
        pInfo->seat_number = atoi(msg_commond["seat_number"].asString().c_str());       //坐下时选定的座位号 0：表示快速坐下，由服务器找到座位并返回
        pInfo->hand_chips = atoi(msg_commond["hand_chips"].asString().c_str());         //命令码，整型
        pInfo->auto_buy = atoi(msg_commond["auto_buy"].asString().c_str());             //自动买入    1：开启     0：关闭
        pInfo->max_buy = atoi(msg_commond["max_buy"].asString().c_str());               //自动按最大带入买入  1：按最大买入   0：按最小买入
        nRet = 0;
    }
    return nRet;
}
/******************************************
 * 坐下失败，用户钱不够或者找不到用户
 * pRoom            //房间指针
 * pInfo            //用户请求结构
 * nErrorCode       //错误码
 * pBuf             //获得的网络数据
 * nBufLen          //传进来是buf长度，成功的话传出去为实际长度
 * return           //成功返回0，失败返回-1 ,bufLen 长度不够 -2
 ******************************************/
int dzpkPutSeatDownError(dealer *pRoom,SEAT_DOWN_REQ_T *pInfo,int nErrorCode,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    if(pRoom && pBuf && pInfo)
    {
        Json::FastWriter  writer;
        Json::Value  msg_body;

        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_user_id"]=pInfo->act_user_id;
        msg_body["seat_number"]=pInfo->seat_number;
        msg_body["act_commond"]=CMD_SEAT_DOWN;
        msg_body["err_code"]=nErrorCode;

        string   str_msg =   writer.write(msg_body);
        if(nBufLen >= (int)strlen(str_msg.c_str()))
        {
            nBufLen = strlen(str_msg.c_str());
            memcpy(pBuf,str_msg.c_str(),strlen(str_msg.c_str()));
            nRet = 0;
        }
        else
        {
            nRet = -2;
        }
    }
    return nRet;
}

/******************************************
 * 坐下信息发给房间用户
 * pRoom            //房间指针
 * pInfo            //用户请求结构
 * nUserID          //坐下用户ID
 * pBuf             //获得的网络数据
 * nBufLen          //传进来是buf长度，成功的话传出去为实际长度
 * return           //成功返回0，失败返回-1 ,bufLen 长度不够 -2
 ******************************************/
int dzpkPutSeatDownPack(dealer *pRoom,SEAT_DOWN_REQ_T *pInfo,int nUserID,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    if(pRoom && pBuf && pInfo)
    {
        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  player_property;

        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_user_id"]=pInfo->act_user_id;
        msg_body["act_commond"]=CMD_SEAT_DOWN;
        msg_body["seat_number"]=pInfo->seat_number;


        //向房间内的所有玩家发送消息周知
        for(unsigned int i=0; i< (pRoom->players_list).size();i++)
        {
            if( (pRoom->players_list).at(i).nUserID == nUserID)
            {
                player_property["user_id"]=(pRoom->players_list).at(i).user_id;
                player_property["user_account"]=(pRoom->players_list).at(i).user_account;
                player_property["nick_name"]=(pRoom->players_list).at(i).nick_name;
                player_property["dzpk_level"]=(pRoom->players_list).at(i).dzpk_level;
                player_property["dzpk_experience"]=itoa((pRoom->players_list).at(i).dzpk_experience,10);
                //player_property["vip_level"]=(pRoom->players_list).at(i).vip_level;
                player_property["chip_account"]=itoa((pRoom->players_list).at(i).chip_account,10);
                player_property["coin_account"]=itoa((pRoom->players_list).at(i).coin_account,10);
                player_property["city"]=(pRoom->players_list).at(i).city;
                player_property["total_time"]=itoa((pRoom->players_list).at(i).total_time,10);
                player_property["win_per"]=itoa((pRoom->players_list).at(i).win_per,10);
                player_property["rank_name"]=(pRoom->players_list).at(i).rank_name;
                player_property["head_photo_serial"]=(pRoom->players_list).at(i).head_photo_serial;
                player_property["desk_chip"]=(pRoom->players_list).at(i).desk_chip;
                player_property["user_staus"]=(pRoom->players_list).at(i).user_staus;
                player_property["room_num"]=(pRoom->players_list).at(i).room_num;
                player_property["seat_num"]=(pRoom->players_list).at(i).seat_num;
                player_property["hand_chips"]=(pRoom->players_list).at(i).hand_chips;
                player_property["giftid"]=(pRoom->players_list).at(i).giftid;
                player_property["mall_path"]=(pRoom->players_list).at(i).mall_path;
                player_property["level_title"]=(pRoom->players_list).at(i).level_title;
                player_property["countdown_no"]=(pRoom->players_list).at(i).countdown_no;//倒计时宝箱编号
                player_property["cd_rest_time"]=(pRoom->players_list).at(i).cd_rest_time;//宝箱领取剩余时间
                player_property["sex"]=(pRoom->players_list).at(i).sex;//性别
                player_property["vip_level"]=(pRoom->players_list).at(i).nVipLevel;
                break;
            }
        }

        msg_body["player_property"]=player_property;
        string   str_msg =   writer.write(msg_body);
        if(nBufLen >= (int)strlen(str_msg.c_str()))
        {
            nBufLen = strlen(str_msg.c_str());
            memcpy(pBuf,str_msg.c_str(),strlen(str_msg.c_str()));
            nRet = 0;
        }
        else
        {
            nRet = -2;
        }
    }
    return nRet;
}

/******************************************
 * 自动买入
 * pRoom            //房间指针
 * pUser            //用户指针
 * nBuyChip         //购买多少筹码
 * pBuf             //获得的网络数据
 * nBufLen          //传进来是buf长度，成功的话传出去为实际长度
 * return           //成功返回0，失败返回-1 ,bufLen 长度不够 -2
 ******************************************/
int dzpkPutAutoBuy(dealer *pRoom,DZPK_USER_INFO_T *pUser,int nBuyChip,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    if(pRoom && pUser && nBuyChip > 0)
    {
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_user_id"]=pUser->user_account;
        msg_body["nick_name"]=pUser->nick_name;
        msg_body["act_commond"]=CMD_AUTO_BUY;
        msg_body["chip_value"]=nBuyChip;
        msg_body["seat_number"]=pUser->seat_num;
        string   str_msg =   writer.write(msg_body);
        if(nBufLen >= (int)strlen(str_msg.c_str()))
        {
            nBufLen = strlen(str_msg.c_str());
            memcpy(pBuf,str_msg.c_str(),strlen(str_msg.c_str()));
            nRet = 0;
        }
        else
        {
            nRet = -2;
        }
    }
    return nRet;
}

/******************************************
 * 筹码不足强制站起
 * pRoom            //房间指针
 * pUser            //用户指针
 * nBuyChip         //购买多少筹码
 * pBuf             //获得的网络数据
 * nBufLen          //传进来是buf长度，成功的话传出去为实际长度
 * return           //成功返回0，失败返回-1 ,bufLen 长度不够 -2
 ******************************************/
int dzpkPutForceLeaveSeat(dealer *pRoom,DZPK_USER_INFO_T *pUser,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    if(pRoom && pBuf && pUser)
    {
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_LEAVE_SEAT;
        msg_body["seat_number"]=pUser->seat_num;
        msg_body["act_user_id"]=pUser->user_account;
        msg_body["nick_name"]=pUser->nick_name;
        msg_body["chip_account"]=itoa(pUser->chip_account,10);
        msg_body["force_exit"]=2;
        string   str_msg =   writer.write(msg_body);
        if(nBufLen >= (int)strlen(str_msg.c_str()))
        {
            nBufLen = strlen(str_msg.c_str());
            memcpy(pBuf,str_msg.c_str(),strlen(str_msg.c_str()));
            nRet = 0;
        }
        else
        {
            nRet = -2;
        }
    }
    return nRet;
}
int dzpkPutCpsCountDown(dealer *pRoom,int nSecond,char *pBuf,int &nBufLen)
{
    int nRet = -1;
    if(pRoom && pBuf)
    {
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_CPS_COUNTDOWN;
        msg_body["cps_count_down"]=nSecond;
        string   str_msg =   writer.write(msg_body);
        if(nBufLen >= (int)strlen(str_msg.c_str()))
        {
            nBufLen = strlen(str_msg.c_str());
            memcpy(pBuf,str_msg.c_str(),strlen(str_msg.c_str()));
            nRet = 0;
        }
        else
        {
            nRet = -2;
        }
    }
    return nRet;
}
