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
#include "custom.h"
#include "../include/json/json.h"
#include "dzpk.h"
#include <openssl/md5.h>
#include "../../../core/loger/log.h"
#include "../Start.h"
#include "dzpkHead.h"
#include "../../../com/common.h"
#include "dzpkParsePacket.h"
#include "../db/dzpkDb.h"
#include "dzpkCustom.h"


#include <algorithm>

/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2015.5.2
** 版本：1.0
** 说明：德州扑克标准流程活动
**************************************************************
*/
#include "../championship/cps.h"
#include "../championship/cpsCustom.h"
#include "dzpkTask.h"

#define ENTER_BIG_ROOM_MIN_LEVEL    3           //多少级别才可以进入中级场

using namespace  std;

/**
*      处理进入房间消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*/
void deal_enter_room(int socketfd, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //printf("enter deal_enter_room\n");
    //特别说明：改为新格式，老格式暂时不要删除
    int nRet = 0;
    user  newuser;
    int room_number = 0;
    int nHadInRoom = 0;     //是否已经在房间中

    //获取用户的房间号
    ENTER_ROOM_REQ_T recInfo;
    if (dzpkGetEnterRoomPack(&recInfo, msg_commond) != 0)
    {
        //解释网络包出错
        WriteLog(400000, "dzpkGetEnterRoomPack error  \n");
        return;
    }

    /*取得房间指针*/
    dealer *pRoom = DzpkGetRoomInfo(recInfo.room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        room_number = 0;
        pRoom = dealers_vector[0];
        WriteLog(400000, "enter room error can't find roomid[%d] \n", recInfo.room_number);
    }

    if (nRet == 0)
    {
        //获得用户数据库信息
        nRet = dzpkGetDbUserInfo(newuser, recInfo.act_user_id, socketfd, pRoom->dealer_id);
        if (nRet == 0)
        {}
    }

	//玩家被禁号，踢出玩家
	if (nRet == 2) 
	{
		
        WriteLog(4000, "guobin enter room nRet== 2 \n");
		//进入房间失败
		Json::FastWriter  writer;
		Json::Value  msg_body;		
		msg_body["msg_head"] = "dzpk";
		msg_body["game_number"] = "dzpk";
		msg_body["area_number"] = "normal";
		msg_body["room_number"] = newuser.room_num;
		msg_body["act_user_id"] = newuser.user_account;
		msg_body["act_commond"] = CMD_ENTER_ROOM;
		msg_body["error_code"] = ERR_LEVEL_LIMITED;
		msg_body["result"] = -1;
		msg_body["tooltip"] = "进入房间失败";
		string	 str_msg =	 writer.write(msg_body);
		DzpkSendP((void *)pSocketUser, CMD_ENTER_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);
		NetCloseSocketUser(pSocketUser->nSocketID);
		return;
	}
	
	
    //网络库用户注册
    if (nRet == 0)
    {
        pSocketUser->lUserIp = inet_addr(newuser.last_ip.c_str());
        newuser.lUserIP = pSocketUser->lUserIp;
        nRet = -1;
        int nUserID = newuser.nUserID;
        if (nUserID > 0)
        {
            // 更新用户资料
            pSocketUser->nRoomID = dealers_vector[room_number]->dealer_id;
            pSocketUser->nUserID = nUserID;

            //用户注册
            void *pUserInfo = NULL;
            pUserInfo = NetGetNetUser(nUserID);
            if (NULL != pUserInfo)
            {
                if (pSocketUser->nRoomID != ((DZPK_USER_T *)pUserInfo)->nRoomID)
                {
                    //退出房间
                    ForceUserExitRoom(((DZPK_USER_T *)pUserInfo)->nRoomID, nUserID, 0);
                    NetFreeNetUser(pUserInfo);
                } else
                {
                    WriteRoomLog(40030, dealers_vector[room_number]->dealer_id, nUserID, "user has in a room previous room [%d] \n", ((DZPK_USER_T *)pUserInfo)->nRoomID);
                    NetFreeNetUser(pUserInfo);
                    NetRemoveNetUser(nUserID);
                }
            }

            pUserInfo = NetGetNetUser(nUserID);
            if (NULL == pUserInfo)
            {
                if (NetAddUserToNet(nUserID, (void *)pSocketUser, sizeof(DZPK_USER_T)) == 0)
                {
                    nRet = 0;
                } else
                {
                    WriteLog(400000, "enter room regester error 3 roomid[%d] nuserid:[%d] \n", recInfo.room_number, nUserID);
                }
            } else
            {
                WriteLog(400000, "enter room regester error 2 roomid[%d] nuserid:[%d] \n", recInfo.room_number, nUserID);
            }
        } else
        {
            WriteLog(400000, "enter room regester error roomid[%d] nuserid:[%d] \n", recInfo.room_number, nUserID);
        }
    }

    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //用户是否被禁止进当前房间
    if (nRet == 0)
    {
        //清除过时记录
        StdUpdateNoInUser(pRoom);

        int nCurTime = time(NULL);
        for (int i = 0; i < (int)pRoom->arryNoInUser.size(); i++)
        {
            if (pRoom->arryNoInUser[i].nUserID == newuser.nUserID)
            {
                //被禁止，不能进入
                WriteLog(USER_PROHIBIT_CAR_LOG_LEVEL, "can't enter room is prohibited.  user account(%s) time(%d) \n", newuser.user_account.c_str(), nCurTime - pRoom->arryNoInUser[i].nEndTime);

                Json::FastWriter  writer;
                Json::Value  msg_body;

                int nNowTime = pRoom->arryNoInUser[i].nEndTime - nCurTime;
                if (nNowTime < 1)
                {
                    nNowTime = 1;
                }

                char szTooltip[50];
                memset(szTooltip, 0, 50);
                sprintf(szTooltip, "你被 %s 请出该房间，还有%2d:%2d才能进入", pRoom->arryNoInUser[i].szActionName, nNowTime / 60, nNowTime % 60);
                msg_body["msg_head"] = "dzpk";
                msg_body["game_number"] = "dzpk";
                msg_body["area_number"] = "normal";
                msg_body["room_number"] = newuser.room_num;
                msg_body["act_user_id"] = newuser.user_account;
                msg_body["act_commond"] = CMD_ENTER_ROOM;
                msg_body["error_code"] = ERR_PROHIBIT_ENTER_ROOM;
                msg_body["result"] = -1;
                msg_body["tooltip"] = szTooltip;
                string   str_msg =   writer.write(msg_body);
                DzpkSend(newuser.socket_fd, CMD_ENTER_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);
                NetRemoveNetUser(pSocketUser->nUserID);
                WriteLog(400000, "enter room level error  roomid[%d] nuserid:[%d] \n", recInfo.room_number, newuser.nUserID);
                return;

                nRet = -1;
                break;
            }
        }
    }

    //如果等级小于6级，只能进新手场
    /* if(nRet == 0)
    {
        if( newuser.dzpk_level < ENTER_BIG_ROOM_MIN_LEVEL &&  (dealers_vector.at(room_number))->small_blind >= 50 )
        {
            int nMallID =0;
            if(StdIfVipCard(&newuser,nMallID) == 0)
            {
                //^_^我有vip卡，我最大
            }
            else
            {
                char szTooltip[100];
                memset(szTooltip,0,100);
                Json::FastWriter  writer;
                Json::Value  msg_body;

                msg_body["msg_head"]="dzpk";
                msg_body["game_number"]="dzpk";
                msg_body["area_number"]="normal";
                msg_body["room_number"]=newuser.room_num;
                msg_body["act_user_id"]=newuser.user_account;
                msg_body["act_commond"]=CMD_ENTER_ROOM;
                msg_body["error_code"]=ERR_LEVEL_LIMITED;
                msg_body["result"]=-1;
                sprintf(szTooltip,"对不起，等级到达%d级才可以进入中级场以上房间，成为VIP可立即进入！",ENTER_BIG_ROOM_MIN_LEVEL);

                msg_body["tooltip"]=szTooltip;
                string   str_msg =   writer.write(msg_body);
                DzpkSend(newuser.socket_fd,CMD_ENTER_ROOM,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,pSocketUser->nUserID);
                NetRemoveNetUser(pSocketUser->nUserID);
                WriteLog(400000,"enter room level error  roomid[%d] nuserid:[%d] \n",recInfo.room_number,newuser.nUserID);
                return;
            }
        }
    }*/



    //将新人加入到该房间的玩家队列，如果发现已经在该房间有该玩家说明是重新连接，更新socket_fd
    if (nRet == 0)
    {
        int count_sameplayer = 0;
        for (unsigned int p = 0; p < (dealers_vector.at(room_number))->players_list.size(); p++)
        {
            if (((dealers_vector.at(room_number))->players_list).at(p).user_id == newuser.user_id)
            {
                ((dealers_vector.at(room_number))->players_list).at(p).lUserIP = pSocketUser->lUserIp;
                if (((dealers_vector.at(room_number))->players_list).at(p).seat_num > 0)
                {
                    WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** enter room has room \n", ((dealers_vector.at(room_number))->players_list).at(p).nUserID,
                             ((dealers_vector.at(room_number))->players_list).at(p).start_countdown,
                             time(0), time(0) - ((dealers_vector.at(room_number))->players_list).at(p).start_countdown,
                             ((dealers_vector.at(room_number))->players_list).at(p).cd_rest_time);

                    if (((dealers_vector.at(room_number))->players_list).at(p).start_countdown > 0)
                    {
                        int cd_time =  time(0) - ((dealers_vector.at(room_number))->players_list).at(p).start_countdown; //倒计时已经记录的时间
                        if (cd_time <  ((dealers_vector.at(room_number))->players_list).at(p).cd_rest_time)
                        {
                            ((dealers_vector.at(room_number))->players_list).at(p).cd_rest_time -= cd_time;
                        } else
                        {
                            ((dealers_vector.at(room_number))->players_list).at(p).cd_rest_time = 0;
                        }
                        ((dealers_vector.at(room_number))->players_list).at(p).start_countdown = time(0);

                        WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** enter room has room sitdown \n", ((dealers_vector.at(room_number))->players_list).at(p).nUserID,
                                 ((dealers_vector.at(room_number))->players_list).at(p).start_countdown,
                                 time(0), time(0) - ((dealers_vector.at(room_number))->players_list).at(p).start_countdown,
                                 ((dealers_vector.at(room_number))->players_list).at(p).cd_rest_time);

                    } else
                    {
                        WriteLog(4008, "nick[%s] startcountdown[%d]  error \n", ((dealers_vector.at(room_number))->players_list).at(p).nick_name.c_str(),
                                 ((dealers_vector.at(room_number))->players_list).at(p).start_countdown);
                    }
                }

                nHadInRoom = 1;
                ((dealers_vector.at(room_number))->players_list).at(p).give_up_count = 0;

                if (((dealers_vector.at(room_number))->players_list).at(p).socket_fd  !=  socketfd)
                {
                    if (((dealers_vector.at(room_number))->players_list).at(p).disconnect_flg  == 1)
                    {
                        //
                    }

                    ((dealers_vector.at(room_number))->players_list).at(p).socket_fd = socketfd;
                    ((dealers_vector.at(room_number))->players_list).at(p).disconnect_flg  = 1; //重新激活掉线的玩家
                    count_sameplayer++;
                } else
                {
                    ((dealers_vector.at(room_number))->players_list).at(p).disconnect_flg  = 1; //重新激活掉线的玩家
                    count_sameplayer++;
                }
                break;
            }
        }

        if (count_sameplayer == 0)
        {
            //将用户加进数组
            ((dealers_vector.at(room_number))->players_list).push_back(newuser);
            //更新房间内人数
            DzpkUpdateRoomPerson(newuser.room_num, connpool);
        }
    }

    //根据房间号获取该房间所有玩家的信息,并返回信息给用户
    if (nRet == 0)
    {
        if (nHadInRoom == 0)
        {
            WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** enter room normal \n", newuser.nUserID,
                     newuser.start_countdown,
                     time(0), time(0) - newuser.start_countdown,
                     newuser.cd_rest_time);
        }


        char *pSend = (char *)malloc(SEND_PACK_MAX_LEN);
        int nSendLen = SEND_PACK_MAX_LEN;
        if (pSend)
        {
            memset(pSend, 0, nSendLen);
            nRet = dzpkPutEnterRoom(pRoom, recInfo.act_user_id, newuser.nUserID, pSend, nSendLen);

            if (nRet == 0)
            {
                DzpkSendP((void *)pSocketUser, CMD_ENTER_ROOM, pSend, nSendLen, dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);
            } else
            {
                WriteLog(4000000, "error len(%d) %s  \n", nSendLen, pSend);
            }
            free(pSend);
            pSend = NULL;
        } else
        {
            nRet = -1;
        }

        //更新玩家session中的位置：由大厅进入房间
        update_user_persition(newuser.user_account, newuser.room_num, connpool);
    }

    //房间已经开始，并且我在里面
    if (nRet == 0)
    {
        if (nHadInRoom && (dealers_vector.at(room_number))->step != 0)
        {
            SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
            if (pList)
            {
                dztimer *pTimer = NULL;
                for (int i = 0; i < pList->nSize; i++)
                {
                    pTimer = (dztimer *)pList->pTable[i];
                    if (pTimer &&  pTimer->room_num == room_number && pTimer->nUserID == pSocketUser->nUserID && pTimer->timer_type == 1)
                    {
                        //有弃牌定时器
                        //判断下一个玩家是不是我
                        for (unsigned int p = 0; p < (dealers_vector.at(room_number))->players_list.size(); p++)
                        {
                            if (((dealers_vector.at(room_number))->players_list).at(p).user_id == newuser.user_id)
                            {
                                if (dealers_vector.at(room_number)->turn > 0 && dealers_vector.at(room_number)->turn == ((dealers_vector.at(room_number))->players_list).at(p).seat_num)
                                {
                                    //发送操作
                                    Json::FastWriter  writer;
                                    Json::Value  msg_body;

                                    msg_body["msg_head"] = "dzpk";
                                    msg_body["game_number"] = "dzpk";
                                    msg_body["area_number"] = "normal";
                                    msg_body["act_commond"] = CMD_RE_ENTER_TURN_ME;
                                    msg_body["min_chip"] = dealers_vector.at(room_number)->nTurnMinChip;
                                    msg_body["max_chip"] = dealers_vector.at(room_number)->nTurnMaxChip;
                                    msg_body["timer_id"] = pTimer->nTimerID;

                                    Json::Value  hand_cards;
                                    if (((dealers_vector.at(room_number))->players_list).at(p).hand_cards.size() >= 2)
                                    {
                                        hand_cards.append(((dealers_vector.at(room_number))->players_list).at(p).hand_cards[0]);
                                        hand_cards.append(((dealers_vector.at(room_number))->players_list).at(p).hand_cards[1]);
                                    }
                                    msg_body["hand_cards"] = hand_cards;

                                    string   str_msg =   writer.write(msg_body);

                                    DzpkSendP((void *)pSocketUser, CMD_RE_ENTER_TURN_ME, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);
                                } else
                                {
                                    WriteLog(4005, "turn error  turn[%d] seat num[%d]\n", dealers_vector.at(room_number)->turn, ((dealers_vector.at(room_number))->players_list).at(p).seat_num);
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
                g_dzpkTimer.FreeList(pList);
            }
        }
    }

    if (nRet == 0)
    {
        //用户进入房间成功

        for (int i = 0; i < (int)(dealers_vector.at(room_number))->players_list.size(); i++)
        {
            if ((dealers_vector.at(room_number))->players_list[i].nUserID == newuser.nUserID)
            {
                //获得投票信息
                ProhibitUserGetInfo(pRoom, newuser.nUserID);

                //更新用户信息
                StdUpdateUserInfo(newuser.nUserID, (dealers_vector.at(room_number))->players_list[i]);

                //检查是否有任务完成
                dzpkEnterCheckTaskComplete(&(dealers_vector.at(room_number))->players_list[i]);
                break;
            }
        }

    } else
    {
        //进入房间失败

        Json::FastWriter  writer;
        Json::Value  msg_body;

        msg_body["msg_head"] = "dzpk";
        msg_body["game_number"] = "dzpk";
        msg_body["area_number"] = "normal";
        msg_body["room_number"] = newuser.room_num;
        msg_body["act_user_id"] = newuser.user_account;
        msg_body["act_commond"] = CMD_ENTER_ROOM;
        msg_body["error_code"] = ERR_LEVEL_LIMITED;
        msg_body["result"] = -1;
        msg_body["tooltip"] = "进入房间失败";
        string   str_msg =   writer.write(msg_body);
        DzpkSendP((void *)pSocketUser, CMD_ENTER_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);
        NetCloseSocketUser(pSocketUser->nSocketID);
    }


}


/**
*      处理坐下消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*/
void deal_seat_down(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //特别说明：改为新格式，老格式暂时不要删除

    int nRet = 0, room_number = 0, ncurFee = 0;
    long lUserChipCount = 0;

    SEAT_DOWN_REQ_T SeatDownInfo;
    if (dzpkGetSeatDownPack(&SeatDownInfo, msg_commond) != 0)
    {
        //读数据出错
        return;
    }

    dealer *pRoom = DzpkGetRoomInfo(SeatDownInfo.room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }


    /*用户不能重复坐下*/
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
        {
            lUserChipCount =  ((dealers_vector.at(room_number))->players_list).at(i).chip_account;
            if (((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0)
            {
                WriteRoomLog(40001, dealers_vector[room_number]->dealer_id, nUserID, "user seat down again [%d] \n", nUserID);
                return;
            }
            break;
        }
    }
    /*
    if(nRet == 0)
    {
        printf("seatdown Userid:%d code:%s ip:%ld sock_ip:%ld\n",pSocketUser->pUser->nUserID,pSocketUser->pUser->terminal_code.c_str(),pSocketUser->pUser->lUserIP,pSocketUser->lUserIp);
        //渠道为机器人的不进行IP限制
        if((dealers_vector.at(room_number))->small_blind >= 2 && !(strcmp(pSocketUser->pUser->terminal_code.c_str(),"ROOT") == 0 || strcmp(pSocketUser->pUser->terminal_code.c_str(),"root") == 0))
        {
            nRet = StdUserCanSeatDownOfIP(pRoom,pSocketUser->lUserIp);  //相同IP检测
            if(nRet != 0)
            {
                //同ip不能坐下
                char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                int nSendLen = SEND_PACK_MAX_LEN;
                if(pSend)
                {
                    memset(pSend,0,nSendLen);
                    if(dzpkPutSeatDownError(pRoom,&SeatDownInfo,ERR_SEAT_DOWN_IP_SAME,pSend,nSendLen) == 0)
                    {
                        DzpkSend(0,CMD_SEAT_DOWN,pSend,nSendLen,dealers_vector[room_number]->dealer_id,nUserID);
                    }
                    free(pSend);
                    pSend = NULL;
                }
                else
                {
                    nRet = -1;
                }
            }
        }
    }
    */
#if 0   //去掉此限制
    if(nRet == 0)
    {
        if((dealers_vector.at(room_number))->small_blind <= 1)
        {
            if(pSocketUser->pUser->chip_account > 1000)
            nRet = -1;
            if(nRet != 0)
            {
                //筹码大于1000 不能坐下
                char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                int nSendLen = SEND_PACK_MAX_LEN;
                if(pSend)
                {
                    memset(pSend,0,nSendLen);
                    if(dzpkPutSeatDownError(pRoom,&SeatDownInfo,ERR_ENTER_ROOM_MAX_CHIP,pSend,nSendLen) == 0)
                    {
                        DzpkSend(0,CMD_SEAT_DOWN,pSend,nSendLen,dealers_vector[room_number]->dealer_id,nUserID);
                    }
                    free(pSend);
                    pSend = NULL;
                } else
                {
                    nRet = -1;
                }
            }
        }
    }
#endif

    //房间内【快速坐下】时，由服务器分配座位号，房间内满座时，不能点击【快速坐下】按钮
    if (nRet == 0)
    {
        //获得用户有效位置
        nRet = StdGetEffectiveSeatNum(pRoom, SeatDownInfo.seat_number);
        if (nRet > 0)
        {
            SeatDownInfo.seat_number = nRet;
            nRet = 0;
        } else
        {
            //找不到位置,返回错误
            nRet = -1;

            char *pSend = (char *)malloc(SEND_PACK_MAX_LEN);
            int nSendLen = SEND_PACK_MAX_LEN;
            if (pSend)
            {
                memset(pSend, 0, nSendLen);
                if (dzpkPutSeatDownError(pRoom, &SeatDownInfo, ERR_NO_SEAT, pSend, nSendLen) == 0)
                {
                    DzpkSend(0, CMD_SEAT_DOWN, pSend, nSendLen, dealers_vector[room_number]->dealer_id, nUserID);
                }

                free(pSend);
                pSend = NULL;
            } else
            {
                nRet = -1;
            }
            return;
        }
    }


    // 将座位号分配给该玩家，并带入筹码数
    if (nRet == 0)
    {
        nRet = StdUserSeatDown(pRoom, nUserID, SeatDownInfo.seat_number, &SeatDownInfo);
        if (nRet == 0)
        {} else
        {
            //用户筹码不足
            char *pSend = (char *)malloc(SEND_PACK_MAX_LEN);
            int nSendLen = SEND_PACK_MAX_LEN;
            if (pSend)
            {
                memset(pSend, 0, nSendLen);
                if (dzpkPutSeatDownError(pRoom, &SeatDownInfo, ERR_TIP_CHIPLESS, pSend, nSendLen) == 0)
                {
                    DzpkSend(0, CMD_SEAT_DOWN, pSend, nSendLen, dealers_vector[room_number]->dealer_id, nUserID);
                }
                free(pSend);
                pSend = NULL;
            } else
            {
                nRet = -1;
            }
        }
    }


    //向房间内的所有玩家发送消息周知
    if (nRet == 0)
    {
        char *pSend = (char *)malloc(SEND_PACK_MAX_LEN);
        int nSendLen = SEND_PACK_MAX_LEN;
        if (pSend)
        {
            memset(pSend, 0, nSendLen);
            nRet = dzpkPutSeatDownPack(pRoom, &SeatDownInfo, nUserID, pSend, nSendLen);

            if (nRet == 0)
            {
                //发送给房间内所有玩家
                for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
                {

                    if ((dealers_vector.at(room_number))->players_list[i].nUserID == nUserID)
                    {
                        (dealers_vector.at(room_number))->players_list[i].nEnterTime = StdGetTime();        //用户玩牌时间
                        (dealers_vector.at(room_number))->players_list[i].nEnterRing = (dealers_vector.at(room_number))->nRing; //用户进入房间时的轮数
                        (dealers_vector.at(room_number))->players_list[i].nChipUpdate = 0; //用户在房间中游戏的筹码变更
                    }
                    DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_SEAT_DOWN, pSend, nSendLen, dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
                }
            }

            free(pSend);
            pSend = NULL;
        } else
        {
            nRet = -1;
        }
        //这一步保证成功
        nRet = 0;
    }


    //开局前进行玩家手上筹码数自动买入判断
    if (nRet == 0)
    {
        int nStatusChange = 0;
		//printf("guobin_in_----StdCheckUserstatus.\n");
        nRet = StdCheckUserstatus(pRoom, nStatusChange, 1, &ncurFee);

        if (nStatusChange != 0)
        {
            //DzpkUpdateRoomPerson(dealer_id,connpool); 		//更新房间人数
        }
    }

    //更新房间人数
    if (nRet == 0)
    {
        DzpkUpdateRoomPerson(pRoom->dealer_id, connpool);        //更新房间人数

    }


    //判断开局  牌局状态为未开始且坐下人数超过两人就开始
    if (nRet == 0)
    {
        StdCheckGameStart(pRoom, ncurFee);
    }

    if (nRet == 0)
    {
        //用户进入房间成功
    } else
    {
        //进入房间失败
    }

    DZPK_USER_T *pUserInfo = (DZPK_USER_T *)NetGetNetUser(nUserID);
    if (pUserInfo)
    {
        NetFreeNetUser((void *)pUserInfo);
    }
}




/**
*      处理离开房间消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_exit_room(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //获取参数
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int force_exit   = atoi((msg_commond["force_exit"].asString()).c_str()); //强制离开房间标记  0：正常站起  1： 在牌局中强制站起
    force_exit = 0;

    int dealer_id = 0;

    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    dealer_id = pRoom->dealer_id;

    //发送消息
    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  player_property;

    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["room_number"] = dealer_id;
    msg_body["act_user_id"] = user_account;
    msg_body["act_commond"] = CMD_EXIT_ROOM;
    msg_body["result"] = "accepted";
    msg_body["chip_account_add"] = 0;
    msg_body["force_exit"] = force_exit;

    vector<user>::iterator its;

    if (force_exit != 1)
    {
        /*客户端标识有时错误*/
        for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
        {
            if (atoi((*its).user_id.c_str()) == nUserID &&  (dealers_vector.at(room_number))->step > 0)
            {
                if ((*its).user_staus == 2 || (*its).user_staus == 3 || (*its).user_staus == 4 || (*its).user_staus == 6)
                {
                    force_exit = 1;
                    (dealers_vector.at(room_number))->current_player_num=(dealers_vector.at(room_number))->current_player_num-1;
                }
                break;
            }
        }
    }

    if (force_exit != 1)
    {
        //更新玩家携带筹码数
        dzpkDbUpdateChipHand(nUserID, 0);
    }

    if (force_exit == 1) //强制退出房间
    {
        for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
        {
            if ((*its).nUserID == nUserID &&  (dealers_vector.at(room_number))->step > 0)
            {
                //如果该玩家是小盲注，移交小盲注指定
                if ((*its).seat_num == (dealers_vector.at(room_number))->seat_small_blind)
                {
                    (dealers_vector.at(room_number))->seat_small_blind  = next_player((dealers_vector.at(room_number))->seat_small_blind, dealers_vector, room_number);
                }

                if ((*its).seat_num ==  (dealers_vector.at(room_number))->turn &&  (dealers_vector.at(room_number))->turn > 0)
                {
                    //当前轮到正欲离开房间的人动作，先做弃牌操作
                    Json::Value   qp_commond;
                    qp_commond["msg_head"] = "dzpk";
                    qp_commond["game_number"] = "dzpk";
                    qp_commond["area_number"] = "normal";
                    qp_commond["room_number"] = itoa(dealer_id, 10);
                    qp_commond["nick_name"] = (*its).nick_name;
                    qp_commond["act_user_id"] = user_account;
                    qp_commond["act_commond"] = CMD_ADD_CHIP;
                    qp_commond["betMoney"] = "0";
                    qp_commond["seat_number"] = itoa((*its).seat_num, 10);
                    qp_commond["ctl_type"] = "0";

                    SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
                    if (pList)
                    {
                        dztimer *pTimer = NULL;
                        for (int i = 0; i < pList->nSize; i++)
                        {
                            pTimer = (dztimer *)pList->pTable[i];
                            if (pTimer &&  pTimer->nUserID == nUserID && pTimer->timer_type == 1)
                            {
                                qp_commond["timer_id"] = GetString(pTimer->nTimerID);
                                break;
                            }
                        }
                        g_dzpkTimer.FreeList(pList);
                    }

                    deal_add_chip(nUserID, dealers_vector, qp_commond, connpool, level_list, pSocketUser, pHead, DEAL_ADD_CHIP_TYPE_EXITROOM);
                } else
                {

                    dztimer timer;
                    //账号和当前时间一起用MD5加密
                    MD5_CTX ctx;
                    char *buf = (char *)((*its).user_account + itoa(get_current_time(), 10)).c_str();
                    unsigned char md[10];
                    MD5_Init(&ctx);
                    MD5_Update(&ctx, buf, strlen(buf));
                    MD5_Final(md, &ctx);
                    char aaa[10];
                    for (unsigned int x = 0; x < 10; x++)
                    {
                        sprintf(aaa + x, "%x", md[x]);
                    }

                    timer.nTimerID = DzpkGetTimerID();
                    timer.nUserID =  (*its).nUserID;

                    timer.socket_fd =  (*its).socket_fd;
                    timer.seat_num =  (*its).seat_num;
                    memcpy(timer.szUserAccount, (*its).user_account.c_str(), strlen((*its).user_account.c_str()));
                    timer.room_num = room_number;
                    timer.start_time = time(NULL);
                    timer.timer_type = 1;
                    timer.time_out = 17;
                    g_dzpkTimer.Insert(timer.nTimerID, (void *)&timer, sizeof(dztimer));
                    WriteRoomLog(500, dealers_vector[room_number]->dealer_id, timer.nUserID, "exit room set timer (%d)  \n", timer.nTimerID);

                    //弃牌操作
                    Json::Value   qp_commond;
                    qp_commond["msg_head"] = "dzpk";
                    qp_commond["game_number"] = "dzpk";
                    qp_commond["area_number"] = "normal";
                    qp_commond["room_number"] = itoa(dealer_id, 10);
                    qp_commond["act_user_id"] = user_account;
                    qp_commond["nick_name"] = (*its).nick_name;
                    qp_commond["act_commond"] = CMD_ADD_CHIP;
                    qp_commond["betMoney"] = "0";
                    qp_commond["seat_number"] = itoa((*its).seat_num, 10);
                    qp_commond["ctl_type"] = "0";

                    SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
                    if (pList)
                    {
                        dztimer *pTimer = NULL;
                        for (int i = 0; i < pList->nSize; i++)
                        {
                            pTimer = (dztimer *)pList->pTable[i];
                            if (pTimer &&  pTimer->nUserID == nUserID && pTimer->timer_type == 1)
                            {
                                qp_commond["timer_id"] = GetString(pTimer->nTimerID);
                            }
                        }
                        g_dzpkTimer.FreeList(pList);
                    }

                    /*异常弃牌*/
                    deal_add_chip(nUserID, dealers_vector, qp_commond, connpool, level_list, pSocketUser, pHead, DEAL_ADD_CHIP_TYPE_FORCELEAVE);
                }

                //弃牌完了之后计算筹码并通知大厅服务器扣除
                /*
                msg_body["chip_account_add"]= ((*its).hand_chips - (*its).last_chips);
                msg_body["seat_num"]= (*its).seat_num;

                (*its).chip_account+=((*its).hand_chips - (*its).last_chips);
                //update_user_chip_account((*its).user_account,(*its).chip_account,connpool);//向大厅服务器发送消息 更新玩家筹码账户

                if(DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                {
                }
                else
                {
                    dzpkDbUpdateUserChip((*its).nUserID,((*its).hand_chips - (*its).last_chips),
                            (*its).chip_account,0,&(*its));
                }
                */
            }
        }
    } else
    {
        for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
        {
            if ((*its).nUserID == nUserID &&  (dealers_vector.at(room_number))->step > 0) //先找到该玩家
            {
                //如果该玩家是小盲注，移交小盲注指定
                if ((*its).seat_num == (dealers_vector.at(room_number))->seat_small_blind)
                {
                    (dealers_vector.at(room_number))->seat_small_blind  = next_player((dealers_vector.at(room_number))->seat_small_blind, dealers_vector, room_number);
                }
            }
        }
    }

    //更新玩家session中的位置：由房间回到大厅
    update_user_persition(user_account, 0, connpool);

    //向房间内的所有玩家发送消息周知
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
        {
            if (DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
            {} else
            {
                if (((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0)
                {
                    //计算筹码并通知大厅服务器扣除
                    if (((dealers_vector.at(room_number))->players_list).at(i).hand_chips != ((dealers_vector.at(room_number))->players_list).at(i).last_chips)
                    {
                        msg_body["chip_account_add"] = (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                        msg_body["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                        ((dealers_vector.at(room_number))->players_list).at(i).chip_account += (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);

                        if (DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                        {} else
                        {
                            dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(i).nUserID, ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips,
                                                 ((dealers_vector.at(room_number))->players_list).at(i).chip_account, 0, &((dealers_vector.at(room_number))->players_list).at(i));
                            //插入玩家牌局信息
                            dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(i), dealers_vector.at(room_number), ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                        }
                    }
                }
            }

            player_property["user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_id;
            player_property["user_account"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account;
            player_property["nick_name"] = ((dealers_vector.at(room_number))->players_list).at(i).nick_name;
            player_property["dzpk_level"] = ((dealers_vector.at(room_number))->players_list).at(i).dzpk_level;
            player_property["dzpk_experience"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).dzpk_experience, 10);
            player_property["vip_level"] = ((dealers_vector.at(room_number))->players_list).at(i).vip_level;
            player_property["chip_account"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).chip_account, 10);
            player_property["rank_name"] = ((dealers_vector.at(room_number))->players_list).at(i).rank_name;
            player_property["head_photo_serial"] = ((dealers_vector.at(room_number))->players_list).at(i).head_photo_serial;
            player_property["desk_chip"] = ((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
            player_property["user_staus"] = ((dealers_vector.at(room_number))->players_list).at(i).user_staus;
            player_property["room_num"] = ((dealers_vector.at(room_number))->players_list).at(i).room_num;
            player_property["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
            player_property["hand_chips"] = ((dealers_vector.at(room_number))->players_list).at(i).hand_chips;
            player_property["giftid"] = ((dealers_vector.at(room_number))->players_list).at(i).giftid;
            player_property["mall_path"] = ((dealers_vector.at(room_number))->players_list).at(i).mall_path;
            player_property["level_title"] = ((dealers_vector.at(room_number))->players_list).at(i).level_title;
        }
    }

    msg_body["player_property"] = player_property;


    string   str_msg =   writer.write(msg_body);
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_EXIT_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
    }

    for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
    {
        if ((*its).nUserID == nUserID)
        {
            if ((*its).seat_num > 0)
            {
                WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** exit room 0 \n", (*its).nUserID,
                         (*its).start_countdown,
                         time(0), time(0) - (*its).start_countdown,
                         (*its).cd_rest_time);

                if ((*its).start_countdown > 0)
                {
                    int cd_time =  time(0) - (*its).start_countdown; //倒计时已经记录的时间
                    if (cd_time <  (*its).cd_rest_time)
                    {
                        (*its).cd_rest_time -= cd_time;
                    } else
                    {
                        (*its).cd_rest_time = 0;
                    }
                } else
                {
                    WriteLog(4008, "exit room  nick[%s] startcountdown[%d]  error \n", (*its).nick_name.c_str(),
                             (*its).start_countdown);
                }
                WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** exit room 1 \n", (*its).nUserID,
                         (*its).start_countdown,
                         time(0), time(0) - (*its).start_countdown,
                         (*its).cd_rest_time);
            }
            //更新用户的等级礼包时间
            dzpkDbUpdateLevelGift((*its).nUserID, (*its).countdown_no, (*its).cd_rest_time, (*its).nTodayPlayTimes);

            //更新玩家在房间时间
            dzpkDbUserPlayTime((*its).nUserID, (*its).room_num, (*its).nEnterTime, (dealers_vector.at(room_number))->nRing - (*its).nEnterRing, (*its).nChipUpdate, (dealers_vector.at(room_number))->small_blind, (dealers_vector.at(room_number))->charge_per);

            (dealers_vector.at(room_number))->leave_players_list.push_back(*its); //离开或站起的玩家 放入另一个队列保存
            ((dealers_vector.at(room_number))->players_list).erase(its); //删除该玩家在房间内的信息
            its--;
        }
    }
    DzpkUpdateRoomPerson(dealer_id, connpool);       /*更新房间人数*/

    NetRemoveNetUser(nUserID);
}




/**
*      处理离座消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_leave_seat(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{

    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //命令码，整型
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int seat_number = atoi((msg_commond["seat_number"].asString()).c_str()); //座位号
    int force_exit   = atoi((msg_commond["force_exit"].asString()).c_str()); //强制离座标记

    int dealer_id = 0;
    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    dealer_id = pRoom->dealer_id;

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    //快速坐下的玩家，发送离座消息不带座位号，需要找到该玩家的座位号
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if (atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str()) == nUserID)
        {
            int seatN =  ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
            if (seatN != seat_number)
            {
                seat_number = seatN;
            }
        }
    }


    //如果是在玩牌中的玩家正好轮到他操作，自动弃牌
    vector<user>::iterator its;

    force_exit = 0;
    if (force_exit != 1)
    {
        /*客户端标识有时错误，对正常离开的玩家做一次检查，更正force_exit的值*/
        for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
        {
            if (atoi((*its).user_id.c_str()) == nUserID &&  (dealers_vector.at(room_number))->step > 0)
            {
                if ((*its).user_staus == 2 || (*its).user_staus == 3 || (*its).user_staus == 4 || (*its).user_staus == 6)
                {
                    force_exit = 1;
                }
                break;
            }
        }
    }

    if (force_exit != 1)
    {
        //更新玩家携带筹码数
        dzpkDbUpdateChipHand(nUserID, 0);
    }

    if (force_exit == 1)
    {
        for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
        {
            if (atoi((*its).user_id.c_str()) == nUserID &&  (dealers_vector.at(room_number))->step > 0)
            {
                //如果该玩家是小盲注，移交小盲注指定给他的下一家
                if ((*its).seat_num == (dealers_vector.at(room_number))->seat_small_blind)
                {
                    (dealers_vector.at(room_number))->seat_small_blind  = next_player((dealers_vector.at(room_number))->seat_small_blind, dealers_vector, room_number);
                }

                if ((*its).seat_num ==  (dealers_vector.at(room_number))->turn &&  (dealers_vector.at(room_number))->turn > 0)
                { //当前轮到正欲离座的人动作，先做弃牌操作
                    Json::Value   qp_commond;
                    qp_commond["msg_head"] = "dzpk";
                    qp_commond["game_number"] = "dzpk";
                    qp_commond["area_number"] = "normal";
                    qp_commond["room_number"] = itoa(dealer_id, 10);
                    qp_commond["act_user_id"] = user_account;
                    qp_commond["nick_name"] = (*its).nick_name;
                    qp_commond["act_commond"] = CMD_ADD_CHIP;
                    qp_commond["betMoney"] = "0";
                    qp_commond["seat_number"] = itoa((*its).seat_num, 10);
                    qp_commond["ctl_type"] = "0";

                    SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
                    if (pList)
                    {
                        dztimer *pTimer = NULL;
                        for (int i = 0; i < pList->nSize; i++)
                        {
                            pTimer = (dztimer *)pList->pTable[i];
                            if (pTimer &&  pTimer->nUserID == nUserID && pTimer->timer_type == 1)
                            {
                                qp_commond["timer_id"] = GetString(pTimer->nTimerID);
                                break;
                            }
                        }
                        g_dzpkTimer.FreeList(pList);
                    }

                    deal_add_chip(nUserID, dealers_vector, qp_commond, connpool, level_list, pSocketUser, pHead, DEAL_ADD_CHIP_TYPE_LEAVESEAT);

                } else
                {


                    //如果最后一轮已经跟满注，牌局正在播放赢牌动画，玩家站起，不发送弃牌消息
                    if ((dealers_vector.at(room_number))->nStepOver == 0)
                    {

                        dztimer timer;
                        //账号和当前时间一起用MD5加密
                        MD5_CTX ctx;
                        char *buf = (char *)((*its).user_account + itoa(get_current_time(), 10)).c_str();
                        unsigned char md[10];
                        MD5_Init(&ctx);
                        MD5_Update(&ctx, buf, strlen(buf));
                        MD5_Final(md, &ctx);
                        char aaa[10];
                        for (unsigned int x = 0; x < 10; x++)
                        {
                            sprintf(aaa + x, "%x", md[x]);
                        }

                        timer.nTimerID = DzpkGetTimerID();
                        timer.nUserID =  (*its).nUserID;

                        timer.socket_fd =  (*its).socket_fd;
                        timer.seat_num =  (*its).seat_num;
                        memcpy(timer.szUserAccount, (*its).user_account.c_str(), strlen((*its).user_account.c_str()));
                        timer.room_num = room_number;
                        timer.start_time = time(NULL);
                        timer.timer_type = 1;
                        timer.time_out = 17;
                        g_dzpkTimer.Insert(timer.nTimerID, (void *)&timer, sizeof(dztimer));
                        WriteRoomLog(500, dealers_vector[room_number]->dealer_id, timer.nUserID, "leave seat set timer (%d)  \n", timer.nTimerID);

                        //弃牌操作
                        Json::Value   qp_commond;
                        qp_commond["msg_head"] = "dzpk";
                        qp_commond["game_number"] = "dzpk";
                        qp_commond["area_number"] = "normal";
                        qp_commond["room_number"] = itoa(dealer_id, 10);
                        qp_commond["act_user_id"] = user_account;
                        qp_commond["nick_name"] = (*its).nick_name;
                        qp_commond["act_commond"] = CMD_ADD_CHIP;
                        qp_commond["betMoney"] = "0";
                        qp_commond["seat_number"] = itoa((*its).seat_num, 10);
                        qp_commond["ctl_type"] = "0";

                        SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
                        if (pList)
                        {
                            dztimer *pTimer = NULL;
                            for (int i = 0; i < pList->nSize; i++)
                            {
                                pTimer = (dztimer *)pList->pTable[i];
                                if (pTimer &&  pTimer->nUserID == nUserID && pTimer->timer_type == 1)
                                {
                                    qp_commond["timer_id"] = GetString(pTimer->nTimerID);
                                }
                            }
                            g_dzpkTimer.FreeList(pList);
                        }

                        /*异常棋牌   0：正常弃牌流程 1：异常弃牌流程*/
                        deal_add_chip(nUserID, dealers_vector, qp_commond, connpool, level_list, pSocketUser, pHead, DEAL_ADD_CHIP_TYPE_FORCELEAVE);

                    }
                }
            }
        }
    } else
    {
        for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
        {
            if ((*its).nUserID == nUserID  &&  (dealers_vector.at(room_number))->step > 0) //先找到该玩家
            {
                //如果该玩家是小盲注，移交小盲注指定给他的下一家
                if ((*its).seat_num == (dealers_vector.at(room_number))->seat_small_blind)
                {
                    (dealers_vector.at(room_number))->seat_small_blind  = next_player((dealers_vector.at(room_number))->seat_small_blind, dealers_vector, room_number);
                }


            }
        }
    }

    //修改该玩家的属性，状态改为 旁观
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
        {

            if (((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0)
            {
                WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** leave sit 0 \n", ((dealers_vector.at(room_number))->players_list).at(i).nUserID,
                         ((dealers_vector.at(room_number))->players_list).at(i).start_countdown,
                         time(0), time(0) - ((dealers_vector.at(room_number))->players_list).at(i).start_countdown,
                         ((dealers_vector.at(room_number))->players_list).at(i).cd_rest_time);

                if (((dealers_vector.at(room_number))->players_list).at(i).start_countdown > 0)
                {
                    int cd_time =  time(0) - ((dealers_vector.at(room_number))->players_list).at(i).start_countdown; //倒计时已经记录的时间
                    if (cd_time <  ((dealers_vector.at(room_number))->players_list).at(i).cd_rest_time)
                    {
                        ((dealers_vector.at(room_number))->players_list).at(i).cd_rest_time -= cd_time;
                    } else
                    {
                        ((dealers_vector.at(room_number))->players_list).at(i).cd_rest_time = 0;
                    }
                } else
                {
                    WriteLog(4008, "sit up  nick[%s] startcountdown[%d]  error \n", ((dealers_vector.at(room_number))->players_list).at(i).nick_name.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(i).start_countdown);
                }
                WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** leave sit 1 \n", ((dealers_vector.at(room_number))->players_list).at(i).nUserID,
                         ((dealers_vector.at(room_number))->players_list).at(i).start_countdown,
                         time(0), time(0) - ((dealers_vector.at(room_number))->players_list).at(i).start_countdown,
                         ((dealers_vector.at(room_number))->players_list).at(i).cd_rest_time);
                //计算筹码并通知大厅服务器扣除
                if (((dealers_vector.at(room_number))->players_list).at(i).hand_chips != ((dealers_vector.at(room_number))->players_list).at(i).last_chips)
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).chip_account += (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);

                    if (DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                    {} else
                    {

                        dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(i).nUserID, ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips,
                                             ((dealers_vector.at(room_number))->players_list).at(i).chip_account, 0, &((dealers_vector.at(room_number))->players_list).at(i));
                        //插入玩家牌局信息
                        dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(i), dealers_vector.at(room_number), ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                    }
                }

            }

            (dealers_vector.at(room_number))->leave_players_list.push_back(((dealers_vector.at(room_number))->players_list).at(i)); //离开或站起的玩家 放入另一个队列保存
            ((dealers_vector.at(room_number))->players_list).at(i).seat_num = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).desk_chip = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).hand_chips = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).last_chips = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.clear();
            ((dealers_vector.at(room_number))->players_list).at(i).win_cards.clear();
            ((dealers_vector.at(room_number))->players_list).at(i).first_turn_blind = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).auto_buy = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).max_buy = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).give_up_count = 0;
            ((dealers_vector.at(room_number))->players_list).at(i).start_countdown = 0;

            (dealers_vector.at(room_number))->current_player_num = (dealers_vector.at(room_number))->current_player_num - 1;
        }

    }

    DzpkUpdateRoomPerson(dealer_id, connpool);       /*更新房间人数*/



    //向房间内的所有玩家发送消息周知
    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  player_property;

    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["room_number"] = dealer_id;
    msg_body["act_user_id"] = user_account;
    msg_body["act_commond"] = CMD_LEAVE_SEAT;
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
        {
            msg_body["chip_account"] = itoa((((dealers_vector.at(room_number))->players_list).at(i)).chip_account, 10);
        }
    }

    msg_body["seat_number"] = seat_number;
    msg_body["force_exit"] = force_exit;

    //向房间内的所有玩家发送消息周知
    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
        {

            //更新玩家在房间时间
            dzpkDbUserPlayTime(nUserID, ((dealers_vector.at(room_number))->players_list).at(i).room_num, ((dealers_vector.at(room_number))->players_list).at(i).nEnterTime, (dealers_vector.at(room_number))->nRing - ((dealers_vector.at(room_number))->players_list).at(i).nEnterRing, ((dealers_vector.at(room_number))->players_list).at(i).nChipUpdate, (dealers_vector.at(room_number))->small_blind, (dealers_vector.at(room_number))->charge_per);
            ((dealers_vector.at(room_number))->players_list).at(i).nEnterTime = -1;

            player_property["user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_id;
            player_property["user_account"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account;
            player_property["nick_name"] = ((dealers_vector.at(room_number))->players_list).at(i).nick_name;
            player_property["dzpk_level"] = ((dealers_vector.at(room_number))->players_list).at(i).dzpk_level;
            player_property["dzpk_experience"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).dzpk_experience, 10);
            player_property["vip_level"] = ((dealers_vector.at(room_number))->players_list).at(i).vip_level;
            player_property["chip_account"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).chip_account, 10);
            player_property["rank_name"] = ((dealers_vector.at(room_number))->players_list).at(i).rank_name;
            player_property["head_photo_serial"] = ((dealers_vector.at(room_number))->players_list).at(i).head_photo_serial;
            player_property["desk_chip"] = ((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
            player_property["user_staus"] = ((dealers_vector.at(room_number))->players_list).at(i).user_staus;
            player_property["room_num"] = ((dealers_vector.at(room_number))->players_list).at(i).room_num;
            player_property["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
            player_property["hand_chips"] = ((dealers_vector.at(room_number))->players_list).at(i).hand_chips;
            player_property["giftid"] = ((dealers_vector.at(room_number))->players_list).at(i).giftid;
            player_property["mall_path"] = ((dealers_vector.at(room_number))->players_list).at(i).mall_path;
            player_property["level_title"] = ((dealers_vector.at(room_number))->players_list).at(i).level_title;
        }
    }

    msg_body["player_property"] = player_property;
    string   str_msg =   writer.write(msg_body);

    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_LEAVE_SEAT, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
    }


}

/**
*      处理加注消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*      chips_value    加(跟)注的注金    看牌：等于零   加注：大于follow_chips  跟注：等于follow_chips
*	
*/
// 
void deal_add_chip(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead, int nType)
{
	
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int chips_value = atoi((msg_commond["betMoney"].asString()).c_str()); //注金数额    0:看牌   大于零：加注
    int seat_number = atoi((msg_commond["seat_number"].asString()).c_str()); //座位号
    int ctl_type = atoi((msg_commond["ctl_type"].asString()).c_str()); // 0：弃牌  1：加注
    string timer_id = (msg_commond["timer_id"].asString()).c_str(); //timer ID
    int nCurTimerID = GetInt(timer_id);
    int dealer_id = 0;
    int nContinue = 0;

    printf("ctl_tkype: %d\n", ctl_type);

	
	//WriteLog(40000,"--guobin1-- deal_add_chip -- room_num = %d\n", room_number);
    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;		
		//WriteLog(40000,"--guobin2-- deal_add_chip -- nVecIndex = %d\n", room_number);
		//WriteLog(40000,"--guobin3-- deal_add_chip -- dealer_id = %d\n", pRoom->dealer_id);
    } 
	else
    {
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);

    if (nType == DEAL_ADD_CHIP_TYPE_USER || nType == DEAL_ADD_CHIP_TYPE_TIMER)
    {
        Lock.Lock();
    }
    if (ctl_type != CASE_ADD_CHIPS && ctl_type != CASE_GIVE_UP)
    {
        printf("非法消息\n");
        return;
    }

    dealer_id = pRoom->dealer_id;


    /*帮助客户端找回丢失的timer_id*/
    if (timer_id == "")
    {
        /*只有是客户发上来的才找回*/
        if (nType == DEAL_ADD_CHIP_TYPE_USER)
        {
            dztimer *pTimer = NULL;
            SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
            if (pList)
            {
                for (int i = 0; i < pList->nSize; i++)
                {
                    pTimer = (dztimer *)pList->pTable[i];
                    if (pTimer && pTimer->nUserID == nUserID && pTimer->timer_type == 1 && pTimer->room_num == room_number)
                    {
                        //timer_id = GetString(pTimer->nTimerID);
                        nCurTimerID = pTimer->nTimerID;
                        WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "lost timer ID:[%d]  \n", pTimer->nTimerID);
                        break;
                    }
                }
                g_dzpkTimer.FreeList(pList);
            }
        }
    }

    //只有轮到的用户可以加注
    if (ctl_type == CASE_ADD_CHIPS)
    {
        for (int i = 0; i < (int)pRoom->players_list.size(); i++)
        {
            if (pRoom->players_list[i].nUserID == nUserID)
            {
                if (pRoom->turn == pRoom->players_list[i].seat_num)
                {
                    if (pRoom->players_list[i].seat_num == seat_number)
                    {
                        if (chips_value >= pRoom->nTurnMinChip && chips_value <= pRoom->nTurnMaxChip)
                        {
                            nContinue = 1;
                        }
                    }
                }

                if (nContinue == 1)
                {} else
                {
                    WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, "add ship error turn[%d] seat num[%d] client num[%d] chip value[%d] min chip[%d] max chip[%d] type[%d] \n", pRoom->turn, pRoom->players_list[i].seat_num, seat_number, chips_value, pRoom->nTurnMinChip, pRoom->nTurnMaxChip, nType);
                }
                break;
            }
        }

        if (nContinue == 1)
        {
        }
		else
        {
            WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, "add ship error can't find user[%d] type[%d] \n", nUserID, nType);
        }
    } else
    {
        nContinue = 1;
    }

    if (nContinue == 1)
    {
        //删除预设的定时器，每次只删除一个同类型的定时器
        dztimer *pTimer = (dztimer *)g_dzpkTimer.GetVal(nCurTimerID);
        if (pTimer)
        {
            if (pTimer->nTimerID == nCurTimerID &&  pTimer->nUserID == nUserID &&  pTimer->timer_type  == 1 &&  pTimer->seat_num == seat_number && pTimer->room_num == room_number)
            {
                //记录系统帮助弃牌次数
                for (unsigned int k = 0; k < ((dealers_vector.at(room_number))->players_list).size(); k++)
                {
                    if (((dealers_vector.at(room_number))->players_list).at(k).nUserID == nUserID)
                    {
                        if (((dealers_vector.at(room_number))->players_list).at(k).give_up_count > 0)
                        {
                            /*
                            if(DzpkIfCpsOfType(pRoom->room_type) == 0)
                            {
                                CpsUserGiveUp(nType,&((dealers_vector.at(room_number))->players_list).at(k));
                            }
                            else
                            {
                                ((dealers_vector.at(room_number))->players_list).at(k).give_up_count = ((dealers_vector.at(room_number))->players_list).at(k).give_up_count-1;
                            }
                            */
                            ((dealers_vector.at(room_number))->players_list).at(k).give_up_count = ((dealers_vector.at(room_number))->players_list).at(k).give_up_count - 1;
                        }
                        break;
                    }
                }

                WriteRoomLog(600, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "add ship kill timer (%d)  type:%d  \n", pTimer->nTimerID, nType);

                switch (ctl_type)
                {
                    /*加注处理分支**/
                case  CASE_ADD_CHIPS:
                    add_chip_branch(pTimer->nUserID, dealers_vector, room_number, user_account, chips_value, seat_number, connpool, level_list, pSocketUser, nType);
                    break;
                    /*弃牌处理分支**/
                case  CASE_GIVE_UP:
                    give_up_branch(pTimer->nUserID, dealers_vector, room_number, user_account, chips_value, seat_number, connpool, level_list, pSocketUser, nType);
                    break;

                default:
                    break;
                }

                if (DzpkIfCpsOfType(pRoom->room_type) == 0)
                {
                    if (nType == DEAL_ADD_CHIP_TYPE_TIMER && ctl_type == CASE_GIVE_UP)
                    {
                        //锦标赛检查是否要强制退出
                        CpsCheckGameingUserStatus(pRoom, nUserID);
                    }
                }

                //if (ctl_type == CASE_ADD_CHIPS || ctl_type == CASE_GIVE_UP || nType != DEAL_ADD_CHIP_TYPE_LEAVESEAT)
                //{
                    g_dzpkTimer.Remove(nCurTimerID);
                    printf("delete timer success\n");
                //}
            } else
            {
                WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, "add ship timer content error timerID [%d][%d] user[%d][%d] timetype[%d][%d] seat num[%d][%d] room num[%d][%d] type[%d] \n",
                             pTimer->nTimerID, nCurTimerID,
                             pTimer->nUserID, nUserID,
                             pTimer->timer_type, 1,
                             pTimer->seat_num, seat_number,
                             pTimer->room_num, room_number,
                             nType);
            }
            g_dzpkTimer.FreeVal((void *)pTimer);
        } else
        {
            WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, "add ship error can't find timer [%d] user[%d] type[%d] \n", nCurTimerID, nUserID, nType);
        }
    }
}


bool sortCompare(dealer *der1, dealer *der2)
{
    return der1->min_buy_chip > der2->min_buy_chip;
}

/**
*      处理快速开始消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*
*/
void deal_quick_start(int socketfd, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    user  newuser;
    string sUserAccount = (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int nRet = 0;


    nRet = dzpkGetDbUserInfo(newuser, (char *)sUserAccount.c_str(), socketfd, 0);
    if (nRet == 0)
    {
        if (dzpkDbReadRoomVisible() == -1)
        {
            printf("获取房间可见标示失败\n");
        }

        //int room_number=-1 ;
        int dealer_id = 0;

        vector<dealer *> tmpDealer(dealers_vector); //所有房间荷官存储队列
        sort(tmpDealer.begin(), tmpDealer.end(), sortCompare);

        vector<dealer *>::iterator ite;
        vector<dealer *>::iterator tmpIte;
#if 0
        for (ite = tmpDealer.begin(); ite != tmpDealer.end(); ite++)
        {
            printf("min_buy_chip: %d, current_player_num: %d\n, mtimedelete: %d room_type: %d\n",\
                       (*ite)->min_buy_chip, (*ite)->current_player_num, (*ite)->nTimeDelete, (*ite)->room_type);
        }
#endif

        int countSeatdown = 0;
        vector<user>::iterator userIte;

        int tmpDealerId = 0;

        for (ite = tmpDealer.begin(); ite != tmpDealer.end(); ite++)
        {
            //printf("****dealer_id : %d****\n", (*ite)->dealer_id);
            if (DzpkIfCpsOfType((*ite)->room_type) == 0
                || (*ite)->nTimeDelete == 1
                || (*ite)->dealer_id < 1000)
            {
                continue;
            }

            if ((newuser.chip_account / 3 - (*ite)->min_buy_chip >= 0)
                && ((*ite)->room_type == 2))
            {
                countSeatdown = 0;

                class CAutoLock lock(&(*ite)->Mutex);
                lock.Lock();

                for (userIte = (*ite)->players_list.begin(); userIte != (*ite)->players_list.end(); userIte++)
                {
                    if (userIte->user_staus >= 1)
                    {
                        countSeatdown++;
                    }
                }

                if (((*ite)->room_seat_num == 5 && countSeatdown > 0 && countSeatdown < 5)
                    || ((*ite)->room_seat_num == 9 && countSeatdown > 0 && countSeatdown < 9))
                {

                    dealer_id = (*ite)->dealer_id;
                    //printf("dealeerId: %d:%s:%d\n", dealer_id, __FILE__, __LINE__);
                    tmpIte = ite;
                    break;
                }
                else if (countSeatdown == 0)
                {
                    if (tmpDealerId == 0)
                    {
                        tmpDealerId = (*ite)->dealer_id;
                        //printf("tmpDealerId: %d:%s:%d\n", tmpDealerId, __FILE__, __LINE__);
                        tmpIte = ite;
                    }
                }
            }
        }

        //newuser.chip_account/3 不满足条件，则直接拿用户筹码和房间携带筹码进行比较
        if (dealer_id == 0)
        {
            for (ite = tmpDealer.begin(); ite != tmpDealer.end(); ite++)
            {
                if (DzpkIfCpsOfType((*ite)->room_type) == 0
                    || (*ite)->nTimeDelete == 1
                    || (*ite)->dealer_id < 1000)
                {
                    continue;
                }

                if (newuser.chip_account - (*ite)->min_buy_chip >= 0
                    && ((*ite)->room_type == 2))
                {
                    countSeatdown = 0;

                    class CAutoLock lock(&(*ite)->Mutex);
                    lock.Lock();

                    for (userIte = (*ite)->players_list.begin(); userIte != (*ite)->players_list.end(); userIte++)
                    {
                        if (userIte->user_staus >= 1)
                        {
                            countSeatdown++;
                        }
                    }

                    if (((*ite)->room_seat_num == 5 && countSeatdown > 0 && countSeatdown < 5)
                        || ((*ite)->room_seat_num == 9 && countSeatdown > 0 && countSeatdown < 9))
                    {

                        dealer_id = (*ite)->dealer_id;
                        //printf("dealeerId: %d:%s:%d\n", dealer_id, __FILE__, __LINE__);
                        tmpIte = ite;
                        break;
                    }
                    else if (countSeatdown == 0)
                    {
                        if (tmpDealerId == 0)
                        {
                            tmpDealerId = (*ite)->dealer_id;
                            //printf("tmpDealerId: %d:%s:%d\n", tmpDealerId, __FILE__, __LINE__);
                            tmpIte = ite;
                        }
                    }
                }
            }
        }

        if (dealer_id == 0 && tmpDealerId >= 1000)
        {
            dealer_id = tmpDealerId;
        }

        //printf("***********dealeerId**********: %d:%s:%d\n", dealer_id, __FILE__, __LINE__);

        if (dealer_id > 0)
        {
            //系统自动帮助玩家进入房间
            Json::Value   qp_commond;
            qp_commond["msg_head"] = "dzpk";
            qp_commond["game_number"] = "dzpk";
            qp_commond["area_number"] = "normal";
            qp_commond["room_number"] = itoa(dealer_id, 10);
            qp_commond["act_user_id"] = newuser.user_account;
            qp_commond["act_commond"] = CMD_ENTER_ROOM;
            deal_enter_room(socketfd, dealers_vector, qp_commond, connpool, level_list, pSocketUser, pHead);

            //大于最小携带量才可以坐下
            if (newuser.chip_account >= (*tmpIte)->min_buy_chip)
            {
                //系统自动帮助玩家快速坐下
                qp_commond["msg_head"] = "dzpk";
                qp_commond["game_number"] = "dzpk";
                qp_commond["area_number"] = "normal";
                qp_commond["room_number"] = itoa(dealer_id, 10);
                qp_commond["act_user_id"] = newuser.user_account;
                qp_commond["act_commond"] = CMD_SEAT_DOWN;
                qp_commond["seat_number"] = "0";
                qp_commond["hand_chips"] = itoa((*tmpIte)->min_buy_chip, 10);   //itoa(dealers_vector.at(room_number)->min_buy_chip,10);
                qp_commond["auto_buy"] = "1";
                qp_commond["max_buy"] = "0";

                deal_seat_down(pSocketUser->nUserID, dealers_vector, qp_commond, connpool, level_list, pSocketUser, pHead);
            }
        } else
        {
            Json::Value   qp_commond;
            qp_commond["msg_head"] = "dzpk";
            qp_commond["game_number"] = "dzpk";
            qp_commond["area_number"] = "normal";
            qp_commond["room_number"] = "0";
            qp_commond["act_user_id"] = newuser.user_account;
            qp_commond["act_commond"] = CMD_ENTER_ROOM;
            qp_commond["err_code"] = ERR_TIP_CHIPLESS;

            Json::FastWriter  writer;

            string   str_msg =   writer.write(qp_commond);
            DzpkSendP((void *)pSocketUser, CMD_ENTER_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), 0, pSocketUser->nUserID);
            NetCloseSocketUser(pSocketUser->nSocketID);
        }
    }
}


/**
*      处理快速开始消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*
*/
#if 0
void deal_quick_start( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{
    user  newuser;
    string sUserAccount = (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int nRet = 0;


    nRet= dzpkGetDbUserInfo(newuser,(char*)sUserAccount.c_str(),socketfd,0);
    if(nRet == 0)
    {
        //更新房间是否可见标识
        dzpkDbReadRoomVisible();

        int room_number=-1;
        int dealer_id = 0;

        vector<dealer *> dealers_Temp; //所有房间荷官存储队列
        for(int i=1;i<(int)dealers_vector.size();i++)
        {
            if(DzpkIfCpsOfType((dealers_vector.at(i))->room_type) == 0)
            {
                continue;
            }
            int nMallID = 0;
            if(StdIfVipCard(&newuser,nMallID) == 0 || newuser.dzpk_level >= ENTER_BIG_ROOM_MIN_LEVEL)
            {
                //三级以上
                if(newuser.chip_account >= 5000)
                {
                    if(newuser.chip_account/5 >= (dealers_vector.at(i))->max_buy_chip && (dealers_vector.at(i))->small_blind > 1)
                    {
                        dealers_Temp.push_back(dealers_vector.at(i));
                    }
                } else
                {
                    if((dealers_vector.at(i))->small_blind <= 1)
                    {
                        dealers_Temp.push_back(dealers_vector.at(i));
                    }
                }
            } else
            {
                if(newuser.chip_account >= 10000)
                {
                    if((dealers_vector.at(i))->small_blind > 1 && (dealers_vector.at(i))->small_blind < 50)
                    {
                        dealers_Temp.push_back(dealers_vector.at(i));
                    }
                } else
                {
                    if((dealers_vector.at(i))->small_blind <= 1)
                    {
                        dealers_Temp.push_back(dealers_vector.at(i));
                    }
                }
            }
        }

        //排序
        if(dealers_Temp.size() >= 2)
        {
            dealer *pRoomTemp = NULL;
            for(int i=0;i<(int)dealers_Temp.size();i++)
            {
                for(int j=i+1;j<(int)dealers_Temp.size();j++)
                {
                    if(dealers_Temp[i]->max_buy_chip < dealers_Temp[j]->max_buy_chip)
                    {
                        pRoomTemp = dealers_Temp[i];
                        dealers_Temp[i] = dealers_Temp[j];
                        dealers_Temp[j] = pRoomTemp;
                    }
                }
            }
        }

        for(int k=0;k<2;k++)
        {
            if(room_number == -1 && dealer_id == 0 )
            {} else
            {
                //已经找到就可以走了
                break;
            }


            for(int i=0;i<(int)dealers_Temp.size();i++)
            {
                int nCanEnter = 1;
                //if(k <= 2)
                {
                    if(dealers_Temp[i]->nTimeDelete != 0)
                    {
                        continue;
                    }
                }
                if(nCanEnter == 1)
                {
                    if(pSocketUser)
                    {
                        //渠道为机器人的不进行IP限制
                        if((dealers_Temp.at(i))->small_blind >= 2 && !(strcmp(pSocketUser->pUser->terminal_code.c_str(),"ROOT") == 0 || strcmp(pSocketUser->pUser->terminal_code.c_str(),"root") == 0))
                        {
                            if(StdUserCanSeatDownOfIP((dealers_Temp.at(i)),pSocketUser->lUserIp) != 0)
                            {
                                //相同ip不能进
                                nCanEnter = 0;
                            }
                        }
                    }
                }

                if(nCanEnter == 1)
                {
                    class CAutoLock Lock(&dealers_Temp.at(i)->Mutex);
                    Lock.Lock();

                    //int nCurTime = time(NULL);

                    for(int l=0;l<(int)dealers_Temp.at(i)->arryNoInUser.size();l++)
                    {
                        if(dealers_Temp.at(i)->arryNoInUser[l].nUserID == newuser.nUserID)
                        {
                            nCanEnter = 0;
                            break;
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        int count_seatdown=0;
                        for(unsigned int j=0;j<  dealers_Temp.at(i)->players_list.size();j++)
                        {
                            if(  dealers_Temp.at(i)->players_list.at(j).user_staus >= 1)
                            {
                                count_seatdown++;
                            }
                        }

                        if(newuser.chip_account < 20)
                        {
                            if(k==0)
                            {
                                if(  dealers_Temp.at(i)->room_seat_num==9  &&  count_seatdown <= 9 && count_seatdown >= 1)
                                {
                                    room_number = dealers_Temp.at(i)->nVecIndex;
                                    dealer_id = dealers_Temp.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    break;
                                } else if ( dealers_Temp.at(i)->room_seat_num==5 &&  count_seatdown <= 5  && count_seatdown >= 1)
                                {
                                    room_number = dealers_Temp.at(i)->nVecIndex;
                                    dealer_id = dealers_Temp.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;
                                    //跳出循环
                                    break;
                                }
                            } else
                            {
                                room_number = dealers_Temp.at(i)->nVecIndex;
                                dealer_id = dealers_Temp.at(i)->dealer_id;
                                newuser.room_num = dealer_id;

                                //跳出循环
                                break;
                            }
                        } else
                        {
                            if(k==0)
                            {
                                if(  dealers_Temp.at(i)->room_seat_num==9  &&  count_seatdown < 7 && count_seatdown >= 1)
                                {
                                    room_number = dealers_Temp.at(i)->nVecIndex;
                                    dealer_id = dealers_Temp.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    break;
                                } else if ( dealers_Temp.at(i)->room_seat_num==5 &&  count_seatdown < 4  && count_seatdown >= 1)
                                {
                                    room_number = dealers_Temp.at(i)->nVecIndex;
                                    dealer_id = dealers_Temp.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;
                                    //跳出循环
                                    break;
                                }
                            } else
                            {
                                if(  dealers_Temp.at(i)->room_seat_num==9  &&  count_seatdown < 7   )
                                {
                                    room_number = dealers_Temp.at(i)->nVecIndex;
                                    dealer_id = dealers_Temp.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    break;
                                } else if ( dealers_Temp.at(i)->room_seat_num==5 &&  count_seatdown < 4  )
                                {
                                    room_number = dealers_Temp.at(i)->nVecIndex;
                                    dealer_id = dealers_Temp.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        WriteLog(4009,"size(%d) roomnum:%d  \n",dealers_Temp.size(),room_number );

        if(room_number>0 && room_number < (int)dealers_vector.size())
        {} else
        {
            int k=0;
            for(k=0;k<4;k++)
            {
                if(room_number == -1 && dealer_id == 0 )
                {} else
                {
                    //已经找到就可以走了
                    break;
                }


                //根据玩家的等级、筹码账户选择空房及座位（筹码账户大于最小买入，小于最大买入）
                //for(unsigned int i=1;i<dealers_vector.size();i++)
                for(unsigned int i=dealers_vector.size() -2;i>=1;i--)
                {
                    /*
                    if(k == 1)
                    {
                        if(i >=  dealers_vector.size() -1)
                        {
                            //不要匹配最大的
                            continue;
                        }
                    }
                    */

                    if(k <= 2)
                    {
                        if(dealers_vector[i]->nTimeDelete != 0)
                        {
                            continue;
                        }
                    }

                    int nCanEnter = 1;

                    if(nCanEnter == 1)
                    {
                        if(DzpkIfCpsOfType((dealers_vector.at(i))->room_type) == 0)
                        {
                            //锦标赛房间不能进
                            nCanEnter = 0;
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        if(pSocketUser)
                        {
                            //渠道为机器人的不进行IP限制
                            if((dealers_vector.at(i))->small_blind >= 2 && !(strcmp(pSocketUser->pUser->terminal_code.c_str(),"ROOT") == 0 || strcmp(pSocketUser->pUser->terminal_code.c_str(),"root") == 0))
                            {
                                if(StdUserCanSeatDownOfIP((dealers_vector.at(i)),pSocketUser->lUserIp) != 0)
                                {
                                    //相同ip不能进
                                    nCanEnter = 0;
                                }
                            }
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        if((dealers_vector.at(i))->small_blind <= 1 && newuser.chip_account > 1000)
                        {
                            //筹码太多，不能进新手场
                            nCanEnter = 0;
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        //如果等级小于6级，只能进新手场
                        if( newuser.dzpk_level < ENTER_BIG_ROOM_MIN_LEVEL &&  (dealers_vector.at(i))->small_blind >= 50 )
                        {
                            nCanEnter = 0;
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        //筹码满足
                        if(newuser.chip_account/2 >=  (dealers_vector.at(i))->min_buy_chip)
                        {} else
                        {
                            nCanEnter = 0;
                        }

                        if(nCanEnter == 0 && k==3 && (dealers_vector.at(i))->min_buy_chip <=40)
                        {
                            nCanEnter = 1;
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        if(newuser.dzpk_level < ENTER_BIG_ROOM_MIN_LEVEL || k == 1 || k >= 3)
                        {
                            //小于六级就不要判断筹码,还有，找一次没找到第二次就需要限制条件了
                            if(k == 1)
                            {}
                        } else
                        {
                            if(dealers_vector.at(i)->min_buy_chip <=  (newuser.chip_account/4)  &&  dealers_vector.at(i)->max_buy_chip >=  (newuser.chip_account/4) )
                            {
                                //需要匹配最小错码
                            } else
                            {
                                nCanEnter = 0;
                            }
                        }
                    }

                    if(nCanEnter == 1)
                    {
                        class CAutoLock Lock(&dealers_vector.at(i)->Mutex);
                        Lock.Lock();

                        //int nCurTime = time(NULL);

                        for(int l=0;l<(int)dealers_vector.at(i)->arryNoInUser.size();l++)
                        {
                            if(dealers_vector.at(i)->arryNoInUser[l].nUserID == newuser.nUserID)
                            {
                                nCanEnter = 0;
                                break;
                            }
                        }

                        if(nCanEnter == 1)
                        {
                            int count_seatdown=0;
                            for(unsigned int j=0;j<  dealers_vector.at(i)->players_list.size();j++)
                            {
                                if(  dealers_vector.at(i)->players_list.at(j).user_staus >= 1)
                                {
                                    count_seatdown++;
                                }
                            }

                            if(k==0 || k==1)
                            {
                                if(  dealers_vector.at(i)->room_seat_num==9  &&  count_seatdown < 7 && count_seatdown >= 1)
                                {
                                    room_number = i;
                                    dealer_id = dealers_vector.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    //i= dealers_vector.size();
                                    break;
                                } else if ( dealers_vector.at(i)->room_seat_num==5 &&  count_seatdown < 4  && count_seatdown >= 1)
                                {
                                    room_number = i;
                                    dealer_id = dealers_vector.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    //i= dealers_vector.size();
                                    break;
                                }
                            } else
                            {
                                if(  dealers_vector.at(i)->room_seat_num==9  &&  count_seatdown < 7   )
                                {
                                    room_number = i;
                                    dealer_id = dealers_vector.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    //i= dealers_vector.size();
                                    break;
                                } else if ( dealers_vector.at(i)->room_seat_num==5 &&  count_seatdown < 4  )
                                {
                                    room_number = i;
                                    dealer_id = dealers_vector.at(i)->dealer_id;
                                    newuser.room_num = dealer_id;

                                    //跳出循环
                                    //i= dealers_vector.size();
                                    break;
                                }
                            }
                        }
                    }
                }
            }

        }



        if(room_number>0 && room_number < (int)dealers_vector.size())
        {
            //系统自动帮助玩家进入房间
            Json::Value   qp_commond;
            qp_commond["msg_head"]="dzpk";
            qp_commond["game_number"]="dzpk";
            qp_commond["area_number"]="normal";
            qp_commond["room_number"]=itoa(dealer_id,10);
            qp_commond["act_user_id"]=newuser.user_account;
            qp_commond["act_commond"]=CMD_ENTER_ROOM;
            deal_enter_room( socketfd,dealers_vector,qp_commond ,connpool,level_list,pSocketUser,pHead);

            //大于最小携带量才可以坐下
            if(newuser.chip_account >= dealers_vector.at(room_number)->min_buy_chip)
            {
                //系统自动帮助玩家快速坐下
                qp_commond["msg_head"]="dzpk";
                qp_commond["game_number"]="dzpk";
                qp_commond["area_number"]="normal";
                qp_commond["room_number"]=itoa(dealer_id,10);
                qp_commond["act_user_id"]=newuser.user_account;
                qp_commond["act_commond"]=CMD_SEAT_DOWN;
                qp_commond["seat_number"]="0";
                qp_commond["hand_chips"]=itoa(dealers_vector.at(room_number)->min_buy_chip,10);
                qp_commond["auto_buy"]="1";
                qp_commond["max_buy"]="0";

                deal_seat_down( pSocketUser->nUserID,dealers_vector,qp_commond,connpool,level_list ,pSocketUser,pHead);
            }
        } else
        {
            Json::Value   qp_commond;
            qp_commond["msg_head"]="dzpk";
            qp_commond["game_number"]="dzpk";
            qp_commond["area_number"]="normal";
            qp_commond["room_number"]="0";
            qp_commond["act_user_id"]=newuser.user_account;
            qp_commond["act_commond"]=CMD_ENTER_ROOM;
            qp_commond["err_code"]=ERR_TIP_CHIPLESS;

            Json::FastWriter  writer;

            string   str_msg =   writer.write(qp_commond);
            DzpkSendP((void*)pSocketUser,CMD_ENTER_ROOM,(char *)str_msg.c_str(),strlen(str_msg.c_str()),0,pSocketUser->nUserID);
            NetCloseSocketUser(pSocketUser->nSocketID);
        }
    }
}
#endif

/**
*      处理展示完毕消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_display_over(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //特别说明：改为新格式，老格式暂时不要删除

    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int seat_number = atoi((msg_commond["seat_number"].asString()).c_str()); //座位号
    string timer_id = (msg_commond["timer_id"].asString()).c_str(); //timer ID
    int nCurTimerID = GetInt(timer_id);

    int dealer_id = 0;
    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        WriteRoomLog(4000, room_number, 0, "room pointer null\n");
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    dealer_id = pRoom->dealer_id;
    WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, " **************** stop play :ring:%d **************\n", dealers_vector[room_number]->nRing);

    //帮助客户端找回丢失的timer_id
    if (timer_id == "")
    {
        dztimer *pTimer = NULL;
        SAFE_HASH_LIST_T *pList = g_dzpkTimer.GetList();
        if (pList)
        {
            for (int i = 0; i < pList->nSize; i++)
            {
                pTimer = (dztimer *)pList->pTable[i];
                if (pTimer && pTimer->nUserID == nUserID && pTimer->timer_type == 2 && pTimer->room_num == room_number)
                {
                    nCurTimerID = pTimer->nTimerID;
                    WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "lost timer ID:[%d]\n", pTimer->nTimerID);
                    break;
                }
            }
            g_dzpkTimer.FreeList(pList);
        }
    }

    //删除预设的定时器，每次只删除一个同类型的定时器
    dztimer *pTimer = (dztimer *)g_dzpkTimer.GetVal(nCurTimerID);
    if (pTimer)
    {
        if (pTimer->nTimerID == nCurTimerID && pTimer->nUserID == nUserID &&  pTimer->timer_type == 2 && pTimer->room_num == room_number)
        {
            WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "display over kill timer (%d) \n", pTimer->nTimerID);
            g_dzpkTimer.Remove(nCurTimerID);

            //一局结束，重新更新玩家的状态为坐下
            for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
            {
                if (/*((dealers_vector.at(room_number))->players_list).at(i).seat_num == seat_number*/
                    ((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID
                    && ((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0 &&
                    ((dealers_vector.at(room_number))->players_list).at(i).seat_num < 10)
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 1;
                    break;
                }
                else
                {
                    WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "server user seat number: %d; recv user seat number: %d\n",
                    ((dealers_vector.at(room_number))->players_list).at(i).seat_num, seat_number);
                }
            }

            //当前在座用户列表属性
            for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
            {
                WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "seat-num: %d : user status: %d\n",
                    ((dealers_vector.at(room_number))->players_list).at(i).seat_num,
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus);

                if (((dealers_vector.at(room_number))->players_list).at(i).seat_num == 0)
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 0; //桌面筹码清零
                    ((dealers_vector.at(room_number))->players_list).at(i).desk_chip = 0; //桌面筹码清零
                    ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).win_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).first_turn_blind = 0;
                }
            }

            //检查当前参与玩牌的玩家数量
            int player_num = 0;
            for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
            {
                if (((dealers_vector.at(room_number))->players_list).at(i).user_staus >= 2)
                {
                    //任然有在玩牌中的玩家
                    player_num++;
                }
            }

            WriteRoomLog(4000,dealers_vector[room_number]->dealer_id,nUserID, "current player number (%d) \n",(dealers_vector.at(room_number))->current_player_num);

            WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, "The current number of players %d\n", player_num);

            if (player_num == 0)
            {

                //开局前检查
                int nRet = 0;

                //全部都已经结束牌局，清除上局的所有数据，计算赢得筹码
                //清空上一局所有消息
                nRet = StdClearRoomInfo(pRoom);
                int ncurFee = 0;
                if (nRet == 0)
                {
                    int nStatusChange = 0;
                    nRet = StdCheckUserstatus(pRoom, nStatusChange, 1, &ncurFee, 1);

                    if (nStatusChange != 0)
                    {
                        DzpkUpdateRoomPerson(dealer_id, connpool);       //更新房间人数
                    }
                }


                //判断开局  牌局状态为未开始且坐下人数超过两人就开始
                if (nRet == 0)
                {
                    StdCheckGameStart(pRoom, ncurFee);
                }
                else
                {
                    WriteRoomLog(4000, pRoom->dealer_id, 0, "nRet is not equal 0\n");
                }

            }
        } else
        {
            WriteRoomLog(600, dealers_vector[room_number]->dealer_id, pTimer->nUserID, "display over find timer but info is wrong timer id[%d][%d] user id[%d][%d] timer type[%d][%d] room number[%d][%d]\n",
                         pTimer->nTimerID, nCurTimerID,
                         pTimer->nUserID, nUserID,
                         pTimer->timer_type, 2,
                         pTimer->room_num, room_number);
        }

        g_dzpkTimer.FreeVal((void *)pTimer);
    } else
    {
        WriteRoomLog(4000, dealers_vector[room_number]->dealer_id, nUserID, "display over can't find timer[%d] user id[%d]  \n", nCurTimerID, nUserID);
    }

}


/**
*      处理房间内说话消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_speek_words(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //命令码，整型
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //命令码，整型
    string chat_msg = (msg_commond["chat_msg"].asString()).c_str(); //说话的文字
    string seatId = (msg_commond["seatId"].asString()).c_str(); //座位号
    int chat_type = msg_commond["chat_type"].asInt(); //类型： 2：动画  1:文字

    int dealer_id = 0;
    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } 
	else
    {
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();
    dealer_id = pRoom->dealer_id;
    int nContinue = 0;

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    Json::FastWriter  writer;
    Json::Value  msg_body;

    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["room_number"] = dealer_id;
    msg_body["act_user_id"] = user_account;
	
    for (unsigned int k = 0; k < ((dealers_vector.at(room_number))->players_list).size(); k++)
    {
        if ((((dealers_vector.at(room_number))->players_list).at(k)).user_account == user_account)
        {
            msg_body["nick_name"] = (((dealers_vector.at(room_number))->players_list).at(k)).nick_name;
            if (chat_type == 2)
            {
                int nMallID = 0;
                if (StdIfVipCard(&(((dealers_vector.at(room_number))->players_list).at(k)), nMallID) == 0)
                {
                    //有vip卡，可以免费使用
                    nContinue = 1;
                    msg_body["chips_value"] = 0; //消费的筹码数
                }
				else if (StdIfTimeToolCard(&(((dealers_vector.at(room_number))->players_list).at(k)), DZPK_USER_FREE_FACE_SHOW) == 0)
                {
                    //有免费表情卡，可以免费使用
                    nContinue = 1;
                    msg_body["chips_value"] = 0; //消费的筹码数
                } else
                {
                    if (DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                    {
                        int nCpsFaceCost = 5;
                        if ((((dealers_vector.at(room_number))->players_list).at(k)).chip_account >= nCpsFaceCost)
                        {
                            nContinue = 1;
                            msg_body["chips_value"] = nCpsFaceCost; //消费的筹码数

                            (((dealers_vector.at(room_number))->players_list).at(k)).chip_account -= nCpsFaceCost;
                            dzpkDbUpdateUserChip(nUserID, -nCpsFaceCost, (((dealers_vector.at(room_number))->players_list).at(k)).chip_account, DZPK_USER_CHIP_CHANGE_CPS_FACE, &(((dealers_vector.at(room_number))->players_list).at(k)));
                        }
                    } else
                    {
                        //使用动画表情会消费筹码 一个小盲注
                        if ((((dealers_vector.at(room_number))->players_list).at(k)).hand_chips >= (dealers_vector.at(room_number))->small_blind)
                        {
                            nContinue = 1;

                            WriteRoomLog(4000, (dealers_vector.at(room_number))->dealer_id, nUserID, "chouma face chip[%d] chips_value[%d]  nick[%s]  account[%s] seat_number[%d]  \n", ((dealers_vector.at(room_number))->players_list).at(k).hand_chips,
                                         (dealers_vector.at(room_number))->small_blind,
                                         ((dealers_vector.at(room_number))->players_list).at(k).nick_name.c_str(),
                                         ((dealers_vector.at(room_number))->players_list).at(k).user_account.c_str(),
                                         ((dealers_vector.at(room_number))->players_list).at(k).seat_num);

                            (((dealers_vector.at(room_number))->players_list).at(k)).hand_chips -= (dealers_vector.at(room_number))->small_blind;
                            //(((dealers_vector.at(room_number))->players_list).at(k)).chip_account -= (dealers_vector.at(room_number))->small_blind;
                            msg_body["chips_value"] = (dealers_vector.at(room_number))->small_blind; //消费的筹码数
                        } else
                        {
                            WriteRoomLog(40030, pRoom->dealer_id, nUserID, "send msg but handchip no enough user id[%d] handchip[%d] need chip[%d] \n", nUserID, (((dealers_vector.at(room_number))->players_list).at(k)).hand_chips, (dealers_vector.at(room_number))->small_blind * 2);
                        }
                    }
                }

                if (nContinue == 1)
                {
                    dzpkTaskTypePlusOne(DZPK_SYSTEM_TASK_SEND_FACE_TYPE, &((dealers_vector.at(room_number))->players_list).at(k));
                }
            } 
			else if (chat_type == 3)
            {
                //猫咪表情，只有vip才能发送
                int nMallID = 0;
                if (StdIfVipCard(&(((dealers_vector.at(room_number))->players_list).at(k)), nMallID) == 0)
                {
                    //有vip卡，可以免费使用
                    nContinue = 1;
                    msg_body["chips_value"] = 0; //消费的筹码数
                } else
                {
                    //不是vip不能发送
                    StdDendErrorToUser(CMD_DZPK_NORMAL_ERROR, ERR_SEND_FACE_NO_VIP, nUserID, pRoom->dealer_id);
                }
            } 
			else
            {
                nContinue = 1;
                msg_body["chips_value"] = 0; //消费的筹码数
            }
            break;
        }
    }

    if (nContinue == 1)
    {
        msg_body["act_commond"] = CMD_SPEEK_WORDS;
        msg_body["chat_msg"] = chat_msg;
        msg_body["chat_type"] = chat_type;
        msg_body["seatId"] = seatId;
        string   str_msg =   writer.write(msg_body);

        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_SPEEK_WORDS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
        }
    }
}




/**
*      通知房间更新用户信息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_present_gift(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str();
    string giftid  =  (msg_commond["giftid"].asString()).c_str();
    string to_user_id_list  =  (msg_commond["to_user_id_list"].asString()).c_str();
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //命令码，整型


    if (act_user_id == "")
    {
        return;
    }

    int dealer_id = 0;
    int seat_num = 0;

    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        dealer_id = pRoom->dealer_id;
        room_number = pRoom->nVecIndex;

    } else
    {
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    for (unsigned int m = 0; m <  pRoom->players_list.size(); m++)
    {
        if (act_user_id ==  pRoom->players_list.at(m).user_id)
        {
            seat_num = pRoom->players_list.at(m).seat_num;
            break;
        }
    }

    if (dealer_id > 0 && seat_num > 0)
    {
        user usr;
        //从连接池获取一个连接
        Connection *con = connpool->GetConnection();
        if (con)
        {
            string sShow;
            try
            {
                Statement *state = con->createStatement(); //获取连接状态
                state->execute("use dzpk"); //使用指定的数据库
                char szSql[200];
                sprintf(szSql, "SELECT chip_account,coin_account,mall_id,mall_path FROM tbl_user_mst LEFT JOIN tbl_mallbase_mst ON mall_id = %s WHERE user_id = '%s' ", giftid.c_str(), act_user_id.c_str());
                string  sql = szSql;
                sShow = sql;
                // 查询
                ResultSet *result = state->executeQuery(sql);
                // 输出查询
                while (result->next())
                {
                    string schip_account = (result->getString("chip_account"));
                    usr.chip_account = atol(schip_account.c_str());
                    string scoin_account = (result->getString("coin_account"));
                    usr.coin_account = atol(scoin_account.c_str());

                    usr.giftid = result->getInt("mall_id");
                    usr.mall_path = result->getString("mall_path");
                }
                if (result)
                {
                    delete result;
                    result = NULL;
                }
                delete state; //删除连接状态
            } catch (sql::SQLException &e)
            {
                WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
                connpool->ReleaseConnection(con); //释放当前连接
                return;
            } catch (...)
            {
                WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
                connpool->ReleaseConnection(con); //释放当前连接
                return;
            }
            connpool->ReleaseConnection(con); //释放当前连接
        } else
        {
            WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
            return;
        }

        //重新读自己的龙币
        for (unsigned int m = 0; m <  dealers_vector.at(room_number)->players_list.size(); m++)
        {
            if (act_user_id ==  dealers_vector.at(room_number)->players_list.at(m).user_id)
            {
                dealers_vector.at(room_number)->players_list.at(m).chip_account = usr.chip_account;
                dealers_vector.at(room_number)->players_list.at(m).coin_account = usr.coin_account;
                dealers_vector.at(room_number)->players_list.at(m).giftid = usr.giftid;

                //赠送任务
                dzpkTaskTypePlusOne(DZPK_SYSTEM_TASK_SEND_GIFT_TYPE, &dealers_vector.at(room_number)->players_list.at(m));
            }
        }


        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  to_user_list;
        Json::Value  to_user;

        msg_body["msg_head"] = "dzpk";
        msg_body["game_number"] = "dzpk";
        msg_body["area_number"] = "normal";
        msg_body["room_number"] = dealer_id;
        msg_body["act_user_id"] = act_user_id;
        msg_body["seat_num"] = seat_num;
        msg_body["act_commond"] = CMD_PRESENT_GIFT;
        msg_body["giftid"] = giftid;
        msg_body["mall_path"] = usr.mall_path;
        vector<string> t_user_id;
        split(to_user_id_list, ",", t_user_id);

        WriteLog(400, "to user id list:(%s)  \n", to_user_id_list.c_str());

        for (unsigned int x = 0; x < t_user_id.size(); x++)
        {
            for (unsigned int y = 0; y < ((dealers_vector.at(room_number))->players_list).size(); y++)
            {
                if (t_user_id.at(x) == ((dealers_vector.at(room_number))->players_list).at(y).user_id)
                {
                    if (((dealers_vector.at(room_number))->players_list).at(y).seat_num > 0)
                    {
                        //重新读取礼物
                        dzpkDbReadUserInfo(&pRoom->players_list[y], 1);

                        to_user["to_user_id"] = ((dealers_vector.at(room_number))->players_list).at(y).user_id;
                        to_user["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(y).seat_num;
                        to_user_list.append(to_user);
                    }
                }
            }
        }

        msg_body["to_user_list"] = to_user_list;
        string   str_msg =   writer.write(msg_body);

        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_PRESENT_GIFT, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
        }
    }

}



/**
*      赠送筹码
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_present_chips(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //赠送动作方玩家账号
    string to_user_account  =  (msg_commond["to_user_id"].asString()).c_str(); //赠送接收方玩家账号
    string chips_value  =  (msg_commond["chips_value"].asString()).c_str(); //赠送筹码数量

    if (act_user_account == "" || to_user_account == "" || chips_value == "0") //赠送消息不合法
    {
        return;
    }

    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int room_number = 0;
    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (act_user_account ==  dealers_vector.at(d)->players_list.at(m).user_id)
            {
                dealer_id = dealers_vector.at(d)->dealer_id;
                room_number = d;
            }
        }
    }

    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    long act_chip_account = 0;
    long to_chip_account = 0;
    int act_hand_chips = 0;
    int to_hand_chips = 0;
    int act_seat = 0;
    int to_seat = 0;

    if (dealer_id > 0 &&  room_number > 0)
    {
        for (unsigned int m = 0; m <  dealers_vector.at(room_number)->players_list.size(); m++)
        {
            if (act_user_account ==  dealers_vector.at(room_number)->players_list.at(m).user_id)
            {
                if (dealers_vector.at(room_number)->players_list.at(m).hand_chips >= atoi(chips_value.c_str()) + 100)
                {
                    WriteRoomLog(4000, (dealers_vector.at(room_number))->dealer_id, nUserID, "chouma send chip chip[%d] chips_value[%d]  nick[%s]  account[%s] seat_number[%d]  \n", ((dealers_vector.at(room_number))->players_list).at(m).hand_chips,
                                 atoi(chips_value.c_str()),
                                 ((dealers_vector.at(room_number))->players_list).at(m).nick_name.c_str(),
                                 ((dealers_vector.at(room_number))->players_list).at(m).user_account.c_str(),
                                 ((dealers_vector.at(room_number))->players_list).at(m).seat_num);

                    //自己的筹码数必须大于赠送数
                    //dealers_vector.at(room_number)->players_list.at(m).chip_account  =  dealers_vector.at(room_number)->players_list.at(m).chip_account  - atol(chips_value.c_str());
                    dealers_vector.at(room_number)->players_list.at(m).hand_chips  =  dealers_vector.at(room_number)->players_list.at(m).hand_chips  - atoi(chips_value.c_str());
                    act_chip_account  = dealers_vector.at(room_number)->players_list.at(m).chip_account;
                    act_hand_chips = dealers_vector.at(room_number)->players_list.at(m).hand_chips;
                    act_seat = dealers_vector.at(room_number)->players_list.at(m).seat_num;
                } else
                {
                    Json::FastWriter  writer;
                    Json::Value  msg_body;

                    msg_body["msg_head"] = "dzpk";
                    msg_body["game_number"] = "dzpk";
                    msg_body["area_number"] = "normal";
                    msg_body["room_number"] = dealer_id;
                    msg_body["result"] = -1;
                    msg_body["act_user_id"] = act_user_account;
                    msg_body["act_chip_account"] = itoa(act_chip_account, 10);
                    msg_body["act_hand_chips"] = act_hand_chips;
                    msg_body["act_seat"] = act_seat;
                    msg_body["act_commond"] = CMD_PRESENT_CHIPS;
                    msg_body["chips_value"] = chips_value;
                    msg_body["to_user_id"] = to_user_account;
                    msg_body["to_chip_account"] = itoa(to_chip_account, 10);
                    msg_body["to_hand_chips"] = to_hand_chips;
                    msg_body["to_seat"] = to_seat;

                    string   str_msg =   writer.write(msg_body);
                    DzpkSend(0, CMD_PRESENT_CHIPS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, nUserID);
                    return;
                }
            }
        }

        for (unsigned int m = 0; m <  dealers_vector.at(room_number)->players_list.size(); m++)
        {
            if (to_user_account ==  dealers_vector.at(room_number)->players_list.at(m).user_id)
            {
                //写日志
                dzpkDbInsertGetChip(nUserID, -atol(chips_value.c_str()), dealers_vector.at(room_number)->players_list.at(m).nUserID, 5);
                dzpkDbInsertGetChip(dealers_vector.at(room_number)->players_list.at(m).nUserID, atol(chips_value.c_str()), nUserID, 4);

                WriteRoomLog(4000, (dealers_vector.at(room_number))->dealer_id, nUserID, "chouma receive chip chip[%d] chips_value[%d]  nick[%s]  account[%s] seat_number[%d]  \n", ((dealers_vector.at(room_number))->players_list).at(m).hand_chips,
                             atoi(chips_value.c_str()),
                             ((dealers_vector.at(room_number))->players_list).at(m).nick_name.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(m).user_account.c_str(),
                             ((dealers_vector.at(room_number))->players_list).at(m).seat_num);
                //筹码增加
                //dealers_vector.at(room_number)->players_list.at(m).chip_account  =  dealers_vector.at(room_number)->players_list.at(m).chip_account  + atol(chips_value.c_str());
                dealers_vector.at(room_number)->players_list.at(m).hand_chips  =  dealers_vector.at(room_number)->players_list.at(m).hand_chips  + atoi(chips_value.c_str());
                to_chip_account  = dealers_vector.at(room_number)->players_list.at(m).chip_account;
                to_hand_chips = dealers_vector.at(room_number)->players_list.at(m).hand_chips;
                to_seat = dealers_vector.at(room_number)->players_list.at(m).seat_num;
            }
        }
    }


    if (dealer_id > 0 &&  room_number > 0)
    {
        //从连接池获取一个连接
        Connection *con = connpool->GetConnection();
        if (con)
        {
            string sShow;
            try
            {
                Statement *state = con->createStatement(); //获取连接状态
                state->execute("use dzpk"); //使用指定的数据库
                if (act_chip_account != 0)
                {
                    string  sact_chip_account =  itoa(act_chip_account, 10);
                    string  sql = "update tbl_user_mst set chip_account = " + sact_chip_account;
                    sql = sql + " where user_id = " + act_user_account;
                    sShow = sql;
                    // UPDATE
                    state->execute(sql);

                }

                if (act_chip_account != 0)
                {
                    string  sto_chip_account =  itoa(to_chip_account, 10);
                    string sql = "update tbl_user_mst set chip_account = " + sto_chip_account;
                    sql = sql + " where user_id = " + to_user_account;
                    sShow = sql;
                    // UPDATE
                    state->execute(sql);
                }

                delete state; //删除连接状态
            } catch (sql::SQLException &e)
            {
                WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
                connpool->ReleaseConnection(con); //释放当前连接
                return;
            } catch (...)
            {
                WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
                connpool->ReleaseConnection(con); //释放当前连接
                return;
            }
            connpool->ReleaseConnection(con); //释放当前连接
        }

        Json::FastWriter  writer;
        Json::Value  msg_body;

        msg_body["msg_head"] = "dzpk";
        msg_body["game_number"] = "dzpk";
        msg_body["area_number"] = "normal";
        msg_body["room_number"] = dealer_id;
        msg_body["result"] = 0;
        msg_body["act_user_id"] = act_user_account;
        msg_body["act_chip_account"] = itoa(act_chip_account, 10);
        msg_body["act_hand_chips"] = act_hand_chips;
        msg_body["act_seat"] = act_seat;
        msg_body["act_commond"] = CMD_PRESENT_CHIPS;
        msg_body["chips_value"] = chips_value;
        msg_body["to_user_id"] = to_user_account;
        msg_body["to_chip_account"] = itoa(to_chip_account, 10);
        msg_body["to_hand_chips"] = to_hand_chips;
        msg_body["to_seat"] = to_seat;

        string   str_msg =   writer.write(msg_body);

        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_PRESENT_CHIPS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
        }
    }
}




/**
*      添加牌友
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_add_friend(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str(); //动作方user_id
    string friend_user_id  =  (msg_commond["friend_user_id"].asString()).c_str(); //被添加方user_id

    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int room_number = 0;
    int act_seat_num = 0;
    int friend_seat_num = 0;

    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (act_user_id ==  dealers_vector.at(d)->players_list.at(m).user_id)
            {
                dealer_id = dealers_vector.at(d)->dealer_id;
                room_number = d;
                act_seat_num = dealers_vector.at(d)->players_list.at(m).seat_num;
            }
        }
    }

    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    Json::FastWriter  writer;
    Json::Value  msg_body;
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["room_number"] = dealer_id;
    msg_body["act_user_id"] = act_user_id;
    msg_body["act_commond"] = CMD_ADD_FRIEND;
    msg_body["act_seat_num"] = act_seat_num;


    for (unsigned int m = 0; m <  dealers_vector.at(room_number)->players_list.size(); m++)
    {
        if (friend_user_id ==  dealers_vector.at(room_number)->players_list.at(m).user_id)
        {
            friend_seat_num = dealers_vector.at(room_number)->players_list.at(m).seat_num;
        }
    }

    msg_body["friend_user_id"] = friend_user_id;
    msg_body["friend_seat_num"] = friend_seat_num;

    string   str_msg =   writer.write(msg_body);

    for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
    {
        DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_PRESENT_CHIPS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
    }

}



/**
*      处理客户端心跳
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_heart_wave(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{

    Json::FastWriter  writer;
    Json::Value  msg_body;
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["act_commond"] = CMD_HEART_WAVE;
    msg_body["result"] = "ok";

    string   str_msg =   writer.write(msg_body);
    DzpkSend(0, CMD_PRESENT_CHIPS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), pSocketUser->nRoomID, nUserID);
}




/**
*      获取房间内旁观的玩家信息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_get_standby(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{

    string sroom_number  =  (msg_commond["room_number"].asString()).c_str(); //房间号

    //先找到赠送动作方的玩家所在房间
    int dealer_id = GetInt(sroom_number);
    int room_number = 0;

    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (dealer_id ==  dealers_vector.at(d)->dealer_id)
            {
                room_number = d;
            }
        }
    }

    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  player_standby;
    Json::Value  player_list;

    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["room_number"] = dealer_id;
    msg_body["act_commond"] = CMD_GET_STANDBY;

    user  oneplayer;

    for (unsigned int i = 0; i < dealers_vector.at(room_number)->players_list.size(); i++)
    {
        oneplayer = dealers_vector.at(room_number)->players_list.at(i);
        if (oneplayer.user_staus == 0 && oneplayer.seat_num == 0)
        {
            player_standby["user_id"] = oneplayer.user_id;
            player_standby["user_account"] = oneplayer.user_account;
            player_standby["nick_name"] = oneplayer.nick_name;
            player_standby["dzpk_level"] = oneplayer.dzpk_level;
            player_standby["dzpk_experience"] = itoa(oneplayer.dzpk_experience, 10);
            player_standby["vip_level"] = oneplayer.vip_level;
            player_standby["chip_account"] = itoa(oneplayer.chip_account, 10);
            player_standby["coin_account"] = itoa(oneplayer.coin_account, 10);
            player_standby["city"] = oneplayer.city;
            player_standby["total_time"] = itoa(oneplayer.total_time, 10);
            player_standby["win_per"] = itoa(oneplayer.win_per, 10);
            player_standby["rank_name"] = oneplayer.rank_name;
            player_standby["head_photo_serial"] = oneplayer.head_photo_serial;
            player_standby["desk_chip"] = oneplayer.desk_chip;
            player_standby["sex"] = oneplayer.sex;
            player_standby["user_staus"] = oneplayer.user_staus;
            player_standby["room_num"] = oneplayer.room_num;
            player_standby["seat_num"] = oneplayer.seat_num;
            player_standby["hand_chips"] = oneplayer.hand_chips;
            player_standby["giftid"] = oneplayer.giftid;
            player_standby["mall_path"] = oneplayer.mall_path;
            player_standby["level_title"] = oneplayer.level_title;

            player_list.append(player_standby);
        }
    }

    msg_body["watcher_list"] = player_list;

    string   str_msg =   writer.write(msg_body);
    DzpkSendP((void *)pSocketUser, CMD_GET_STANDBY, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);

}



/**
*      手动亮牌
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  lay_hand_cards(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int act_seat_num = 0;
    int nContinue = 0;

    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str(); //动作方user_id
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号

    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
        dealer_id = pRoom->dealer_id;
    } else
    {
        WriteLog(40005, " lay hand cards can't find room (%d) &&&&  \n", room_number);
        return;
    }

    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  hand_cards;
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["act_commond"] = CMD_LAY_CARDS;

    for (unsigned int m = 0; m <  pRoom->players_list.size(); m++)
    {
        if (act_user_id ==  pRoom->players_list.at(m).user_id)
        {
            act_seat_num = pRoom->players_list.at(m).seat_num;
            if (act_seat_num > 0 || act_seat_num  <= DZPK_ROOM_MAX_USER)
            {
                if (pRoom->players_list.at(m).hand_cards.size() >= 2)
                {
                    hand_cards.append(pRoom->players_list.at(m).hand_cards.at(0));
                    hand_cards.append(pRoom->players_list.at(m).hand_cards.at(1));
                    nContinue = 1;
                }
            }
            break;
        }
    }

    if (nContinue == 1)
    {
        msg_body["room_number"] = dealer_id;
        msg_body["act_user_id"] = act_user_id;
        msg_body["act_seat_num"] = act_seat_num;
        msg_body["hand_cards"] = hand_cards;

        string   str_msg =   writer.write(msg_body);

        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_LAY_CARDS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
        }
    }
}



/**
*      手动补齐手上的筹码
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  reset_hand_chips(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str(); //动作方user_id
    string set_chips  =  (msg_commond["set_chips"].asString()).c_str(); //设定的补齐到的数额

    Json::FastWriter  writer;
    Json::Value  msg_body;
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["act_commond"] = CMD_RESET_CHIPS;

    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int room_number = 0;
    int act_seat_num = 0;
    int user_status = 0;

    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (act_user_id ==  dealers_vector.at(d)->players_list.at(m).user_id)
            {
                dealer_id = dealers_vector.at(d)->dealer_id;
                room_number = d;
                act_seat_num = dealers_vector.at(d)->players_list.at(m).seat_num;
                user_status =  dealers_vector.at(d)->players_list.at(m).user_staus;
                if (dealers_vector.at(d)->players_list.at(m).user_staus == 5)  //如果玩家不在当前牌局中，可以立刻重置手上的筹码
                {
                    dealers_vector.at(d)->players_list.at(m).hand_chips = GetInt(set_chips);
                } else
                {
                    dealers_vector.at(d)->players_list.at(m).reset_hand_chips = GetInt(set_chips);
                }

            }
        }
    }

    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    if (user_status == 5  || user_status == 1)
    {
        msg_body["room_number"] = dealer_id;
        msg_body["act_user_id"] = act_user_id;
        msg_body["act_seat_num"] = act_seat_num;
        msg_body["hand_chips"] = set_chips;
        string   str_msg =   writer.write(msg_body);

        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_RESET_CHIPS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
        }
    }



}



/**
*      获取玩家等级礼包信息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  get_level_gift(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str(); //动作方user_id


    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  level_gifts;
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["act_commond"] = CMD_LEVEL_GIFTS;

    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int room_number = 0;
    int user_status = 0;

    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (act_user_id ==  dealers_vector.at(d)->players_list.at(m).user_id)
            {
                dealer_id = dealers_vector.at(d)->dealer_id;
                room_number = d;
                user_status =  dealers_vector.at(d)->players_list.at(m).user_staus;
            }
        }
    }

    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    struct level_gifts  lgifts;
    int nRet = dzpkGetDbLevelGifts(lgifts, act_user_id);
    if (nRet == -1)
    {
        return;
    } else if (nRet == 0)
    {
        level_gifts["level"] = lgifts.level;
        level_gifts["chip_award"] = lgifts.chip_award;
        level_gifts["coin_award"] = lgifts.coin_award;
        level_gifts["face_award"] = lgifts.face_award;
        level_gifts["gift_award"] = lgifts.gift_award;
        level_gifts["vip_award"] = lgifts.vip_award;
        level_gifts["stone_award"] = lgifts.stone_award;
        msg_body["level_gifts"] = level_gifts;
    }

    msg_body["act_user_id"] = act_user_id;
    string   str_msg =   writer.write(msg_body);

    DzpkSend(0, CMD_LEVEL_GIFTS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);

}







/**
*      赠送玩家等级礼包信息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_level_gift(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str(); //动作方user_id


    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  level_gifts;
    string result = "1";
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["act_commond"] = CMD_DEAL_GIFTS;
    msg_body["result"] = result; //领取结果    1： OK   0：NG

    struct level_gifts  lgifts;
    int nRet = dzpkGetDbLevelGifts(lgifts, act_user_id);  //获取用户等级礼包信息
    if (nRet == -1)
    {
        return;
    }
    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int room_number = 0;
    int user_status = 0;
    int chip_account = 0;

    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (act_user_id ==  dealers_vector.at(d)->players_list.at(m).user_id)
            {
                dealer_id = dealers_vector.at(d)->dealer_id;
                room_number = d;
                user_status =  dealers_vector.at(d)->players_list.at(m).user_staus;
                if (dealers_vector.at(d)->players_list.at(m).dzpk_level  >=  lgifts.level)
                {
                    dealers_vector.at(d)->players_list.at(m).chip_account += GetInt(lgifts.chip_award); // 奖励筹码增加到用户筹码账户
                    chip_account =   dealers_vector.at(d)->players_list.at(m).chip_account;

                    //其他奖励处理
                    //免费表情
                    int nFaceDay = GetInt(lgifts.face_award);
                    if (nFaceDay  > 0)
                    {
                        dzpkDbAwardTimeTool(dealers_vector.at(d)->players_list.at(m).nUserID, DZPK_USER_FREE_FACE_SHOW, nFaceDay * 24 * 60 * 60);
                        dealers_vector.at(d)->players_list.at(m).nReadBpFromDb = 0;
                    }
                } else //等级不够
                {
                    result = "0";
                }
            }
        }
    }
    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    if (result == "1")
    {
        nRet = -1;
        nRet = dzpkUpdateDbLevelGifts(act_user_id, chip_account, (lgifts.level + 1));  //获取用户等级礼包信息
        if (nRet == -1)
        {
            result = "0";
        }
    }

    nRet = dzpkGetDbLevelGifts(lgifts, act_user_id);  //重新获取用户下一等级礼包信息
    if (nRet == -1)
    {
        return;
    }
    msg_body["chip_account"] = chip_account;
    msg_body["act_user_id"] = act_user_id;
    msg_body["next_level"] = lgifts.level;
    msg_body["next_chip_award"] = lgifts.chip_award;
    msg_body["next_face_award"] = lgifts.face_award;
    msg_body["result"] = result; //领取结果    1： OK   0：NG
    string   str_msg =   writer.write(msg_body);
    DzpkSend(0, CMD_DEAL_GIFTS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);

}






/**
*      领取倒计时宝箱
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  deal_count_down(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    string act_user_id  =  (msg_commond["act_user_id"].asString()).c_str(); //动作方user_id


    Json::FastWriter  writer;
    Json::Value  msg_body;
    Json::Value  level_gifts;
    msg_body["msg_head"] = "dzpk";
    msg_body["game_number"] = "dzpk";
    msg_body["area_number"] = "normal";
    msg_body["act_commond"] = CMD_CNTD_GIFTS;
    msg_body["result"] = "1"; //领取结果    1： OK   0：NG

    string chip_awards;
    int next_cntid, next_rest_time;
    //先找到赠送动作方的玩家所在房间
    int dealer_id = 0;
    int room_number = 0;
    int user_status = 0;
    int chip_account = 0;
    int nAddChip = 0;
    int nContinue = 0;

    for (unsigned int d = 0; d < dealers_vector.size(); d++)
    {
        class CAutoLock Lock(&dealers_vector[d]->Mutex);
        Lock.Lock();
        for (unsigned int m = 0; m <  dealers_vector.at(d)->players_list.size(); m++)
        {
            if (act_user_id ==  dealers_vector.at(d)->players_list.at(m).user_id)
            {
                dealer_id = dealers_vector.at(d)->dealer_id;
                room_number = d;
                user_status =  dealers_vector.at(d)->players_list.at(m).user_staus;

                if (dealers_vector.at(d)->players_list.at(m).seat_num > 0)
                {
                    if ((time(0) - dealers_vector.at(d)->players_list.at(m).start_countdown) <  dealers_vector.at(d)->players_list.at(m).cd_rest_time)
                    {
                        //倒计时没有到期，点击领取无效
                        WriteLog(40008, "enter count down user id [%s]  time error passtime[%d]  resttime[%d]  \n", act_user_id.c_str(), time(0) - dealers_vector.at(d)->players_list.at(m).start_countdown, dealers_vector.at(d)->players_list.at(m).cd_rest_time);
                        return;
                    } else
                    {
                        int nRet = dzpkGetDbCountDown(chip_awards, act_user_id, next_cntid, next_rest_time);  //获取用户等级礼包信息
                        if (nRet == -1)
                        {
                            WriteLog(40008, "enter count down user id [%s]  sql error  \n", act_user_id.c_str());
                            return;
                        }
                    }
                } else
                {
                    if (dealers_vector.at(d)->players_list.at(m).cd_rest_time <= 0)
                    {
                        int nRet = dzpkGetDbCountDown(chip_awards, act_user_id, next_cntid, next_rest_time);  //获取用户等级礼包信息
                        if (nRet == -1)
                        {
                            WriteLog(40008, "enter count down user id [%s]  sql error  \n", act_user_id.c_str());
                            return;
                        }
                    } else
                    {
                        WriteLog(40008, "enter count down user id [%s]  time[%d] error   \n", act_user_id.c_str(), dealers_vector.at(d)->players_list.at(m).cd_rest_time);
                        return;
                    }
                }

                nAddChip =   GetInt(chip_awards);

                int nMallID = 0;
                //if(StdIfVipCard(&dealers_vector.at(d)->players_list.at(m),nMallID) == 0)
                if (0)
                {
                    //加成
                    switch (nMallID)
                    {
                    case DZPK_USER_BACKPACK_VIPI_1:
                        {
                            nAddChip = nAddChip * 4;
                        }
                        break;
                    case DZPK_USER_BACKPACK_VIPI_2:
                        {
                            nAddChip = nAddChip * 3.5;
                        }
                        break;
                    case DZPK_USER_BACKPACK_VIPI_3:
                        {
                            nAddChip = nAddChip * 3;
                        }
                        break;
                    case DZPK_USER_BACKPACK_VIPI_4:
                        {
                            nAddChip = nAddChip * 2.5;
                        }
                        break;
                    case DZPK_USER_BACKPACK_VIPI_5:
                        {
                            nAddChip = nAddChip * 2;
                        }
                        break;
                    case DZPK_USER_BACKPACK_VIPI_6:
                        {
                            nAddChip = nAddChip * 1.5;
                        }
                        break;
                    }
                }

                if (nAddChip > 0)
                {
                    dealers_vector.at(d)->players_list.at(m).chip_account += nAddChip; // 奖励筹码增加到用户筹码账户
                                                                                       //dzpkDbInsertGetChip(dealers_vector.at(d)->players_list.at(m).nUserID,GetInt(chip_awards),0,3);
                    dzpkDbUpdateUserChip(dealers_vector.at(d)->players_list.at(m).nUserID, nAddChip, dealers_vector.at(d)->players_list.at(m).chip_account, DZPK_USER_CHIP_CHANGE_COUNT_DOWN, &dealers_vector.at(d)->players_list.at(m));
                }
                chip_account =   dealers_vector.at(d)->players_list.at(m).chip_account;
                dealers_vector.at(d)->players_list.at(m).countdown_no = next_cntid;
                dealers_vector.at(d)->players_list.at(m).cd_rest_time = next_rest_time;
                dealers_vector.at(d)->players_list.at(m).start_countdown = time(0);
                //其他奖励处理

                nContinue = 1;
                break;
            }
        }
    }
    dealer *pRoom = dealers_vector[room_number];
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    if (nContinue == 1)
    {
        msg_body["chip_account"] = chip_account;
        msg_body["add_chip"] = nAddChip;
        msg_body["act_user_id"] = act_user_id;
        msg_body["next_cntid"] = next_cntid;
        msg_body["next_rest_time"] = next_rest_time;
        string   str_msg =   writer.write(msg_body);

        DzpkSend(0, CMD_CNTD_GIFTS, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, pSocketUser->nUserID);
        WriteLog(41011, " deal_count_down :%s  \n", str_msg.c_str());
    } else
    {
        WriteLog(4005, "count down error  user_id:%s    \n", act_user_id.c_str());
    }
}

/*发送消息给房间用户*/
int deal_send_to_room(int nUserID, int nRoomdID, char *pBuf, int nBufLen, int nType)
{
    int nRet = 0;

    dealer *pRoom = DzpkGetRoomInfo(nRoomdID);
    if (pRoom)
    {
        class CAutoLock Lock(&pRoom->Mutex);
        Lock.Lock();

        //判断用户是否是房间里的用户
        if (StdIfUserInRoom(pRoom, nUserID) != 0)
        {
            return -1;
        }

        int nContinue = 0;

        for (unsigned int i = 0; i <  pRoom->players_list.size(); i++)
        {
            if (pRoom->players_list[i].nUserID == nUserID)
            {
                switch (nType)
                {
                case 1:
                    {
                        int nCurCount = 0;
                        int nSuc = dzpkDbUseSendToolCard(nUserID, &nCurCount);
                        if (nSuc == 0)
                        {
                            char szTooltip[100];
                            sprintf(szTooltip, "%s 使用了一个互动道具", pRoom->players_list[i].nick_name.c_str());
                            dzpkDbWritePacketLog(1, pRoom->players_list[i].room_num, pRoom->players_list[i].nUserID, -1, szTooltip, nCurCount + 1);

                            WriteLog(5006, " no send tool   (%d) count(%d) \n", nUserID, nCurCount);
                            nContinue = 1;
                        } else
                        {
                            nContinue = -2;
                            WriteLog(5006, " no send tool  no engh (%d) \n", nUserID);
                            //返回错误
                            StdDendErrorToUser(CMD_DZPK_NORMAL_ERROR, ERR_SEND_TOOL_NO_CARD, nUserID, nRoomdID);
                        }
                    }
                    break;
                default:
                    {
                        nContinue = 1;
                    }
                    break;
                }
                break;
            }
        }

        if (nContinue == 1)
        {
            for (unsigned int i = 0; i <  pRoom->players_list.size(); i++)
            {
                DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_SEND_MSG_TO_ROOM, pBuf, nBufLen, pRoom->dealer_id);
            }
        } else
        {
            if (nContinue != -2)
            {
                WriteRoomLog(40030, pRoom->dealer_id, nUserID, "send msg to room user but I am not in this room. user id [%d] \n", nUserID);
            }
        }
    } else
    {
        WriteLog(40005, "deal send msg to room cant't find room [%d] user [%d] \n", nRoomdID, nUserID);
        return -1;
    }

    return nRet;
}

/*处理管理员强制踢人*/
int CheckAdminForceUserExitRoom(dealer *pRoom, int nUserID)
{
	
    int nRet = 0;

    WriteLog(4005, " exit step:%d   \n", pRoom->step);
    if (pRoom && pRoom->step == 0)
    {
        int room_number = pRoom->nVecIndex;
        vector<user>::iterator its;
        int nContinue = 0;

        //发送消息
        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  player_property;

        msg_body["msg_head"] = "dzpk";
        msg_body["game_number"] = "dzpk";
        msg_body["area_number"] = "normal";
        msg_body["room_number"] = pRoom->dealer_id;
        msg_body["act_commond"] = CMD_EXIT_ROOM;
        msg_body["result"] = "accepted";
        msg_body["chip_account_add"] = 0;
        msg_body["force_exit"] = 0;

        //向房间内的所有玩家发送消息周知
        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
            {
                if (((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0)
                {
                    msg_body["chip_account_add"] = (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                    msg_body["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                    //计算筹码并通知大厅服务器扣除
                    if (((dealers_vector.at(room_number))->players_list).at(i).hand_chips != ((dealers_vector.at(room_number))->players_list).at(i).last_chips)
                    {
                        ((dealers_vector.at(room_number))->players_list).at(i).chip_account += (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);

                        if (DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                        {} else
                        {
                            dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(i).nUserID, ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips,
                                                 ((dealers_vector.at(room_number))->players_list).at(i).chip_account, 0, &((dealers_vector.at(room_number))->players_list).at(i));
                            //插入玩家牌局信息
                            dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(i), dealers_vector.at(room_number), ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                        }
                    }
                }
                msg_body["act_user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account;

                //更新玩家session中的位置：由房间回到大厅
                update_user_persition(((dealers_vector.at(room_number))->players_list).at(i).user_account, 0, connpool);
				
                player_property["user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_id;
                player_property["user_account"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account;
                player_property["nick_name"] = ((dealers_vector.at(room_number))->players_list).at(i).nick_name;
                player_property["dzpk_level"] = ((dealers_vector.at(room_number))->players_list).at(i).dzpk_level;
                player_property["dzpk_experience"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).dzpk_experience, 10);
                player_property["vip_level"] = ((dealers_vector.at(room_number))->players_list).at(i).vip_level;
                player_property["chip_account"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).chip_account, 10);
                player_property["rank_name"] = ((dealers_vector.at(room_number))->players_list).at(i).rank_name;
                player_property["head_photo_serial"] = ((dealers_vector.at(room_number))->players_list).at(i).head_photo_serial;
                player_property["desk_chip"] = ((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
                player_property["user_staus"] = ((dealers_vector.at(room_number))->players_list).at(i).user_staus;
                player_property["room_num"] = ((dealers_vector.at(room_number))->players_list).at(i).room_num;
                player_property["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                player_property["hand_chips"] = ((dealers_vector.at(room_number))->players_list).at(i).hand_chips;
                player_property["giftid"] = ((dealers_vector.at(room_number))->players_list).at(i).giftid;
                player_property["mall_path"] = ((dealers_vector.at(room_number))->players_list).at(i).mall_path;
                player_property["level_title"] = ((dealers_vector.at(room_number))->players_list).at(i).level_title;

                nContinue = 1;
                break;
            }
        }

        if (nContinue == 1)
        {
            msg_body["player_property"] = player_property;

            string   str_msg =   writer.write(msg_body);
            for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
            {
                DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_EXIT_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
            }

            for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
            {
                if ((*its).nUserID == nUserID)
                {
                    if ((*its).seat_num > 0)
                    {
                        WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** check force use exit room 0 \n", (*its).nUserID,
                                 (*its).start_countdown,
                                 time(0), time(0) - (*its).start_countdown,
                                 (*its).cd_rest_time);
                        if ((*its).start_countdown > 0)
                        {
                            int cd_time =  time(0) - (*its).start_countdown; //倒计时已经记录的时间
                            if (cd_time <  (*its).cd_rest_time)
                            {
                                (*its).cd_rest_time -= cd_time;
                            } else
                            {
                                (*its).cd_rest_time = 0;
                            }
                        } else
                        {
                            WriteLog(4008, "check user exit room  nick[%s] startcountdown[%d]  error \n", (*its).nick_name.c_str(),
                                     (*its).start_countdown);
                        }
                        WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** check force use exit room 1 \n", (*its).nUserID,
                                 (*its).start_countdown,
                                 time(0), time(0) - (*its).start_countdown,
                                 (*its).cd_rest_time);
                    }
                    //更新用户的等级礼包时间
                    dzpkDbUpdateLevelGift((*its).nUserID, (*its).countdown_no, (*its).cd_rest_time, (*its).nTodayPlayTimes);

                    //更新玩家在房间时间
                    dzpkDbUserPlayTime((*its).nUserID, (*its).room_num, (*its).nEnterTime, (dealers_vector.at(room_number))->nRing - (*its).nEnterRing, (*its).nChipUpdate, (dealers_vector.at(room_number))->small_blind, (dealers_vector.at(room_number))->charge_per);

                    (dealers_vector.at(room_number))->leave_players_list.push_back(*its); //离开或站起的玩家 放入另一个队列保存
                    ((dealers_vector.at(room_number))->players_list).erase(its); //删除该玩家在房间内的信息
                    its--;
                }
            }
			
            DzpkUpdateRoomPerson(pRoom->dealer_id, connpool);        /*更新房间人数*/
			NetRemoveNetUser(nUserID);
        }
    }

	//修改数据库标志
	

    return nRet;

}

/*玩牌前强迫用户退出房间*/
int CheckForceUserExitRoom(dealer *pRoom, int nUserID, int nType, char *pTemp1, char *pTemp2)
{
    int nRet = 0;

    WriteLog(4005, " exit step:%d   \n", pRoom->step);
    if (pRoom && pRoom->step == 0)
    {
        int room_number = pRoom->nVecIndex;
        vector<user>::iterator its;
        int nContinue = 0;

        //发送消息
        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  player_property;

        msg_body["msg_head"] = "dzpk";
        msg_body["game_number"] = "dzpk";
        msg_body["area_number"] = "normal";
        msg_body["room_number"] = pRoom->dealer_id;
        msg_body["act_commond"] = CMD_EXIT_ROOM;
        msg_body["result"] = "accepted";
        msg_body["chip_account_add"] = 0;
        msg_body["force_exit"] = 0;

        //向房间内的所有玩家发送消息周知
        for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
        {
            if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
            {
                if (((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0)
                {
                    msg_body["chip_account_add"] = (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                    msg_body["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                    //计算筹码并通知大厅服务器扣除
                    if (((dealers_vector.at(room_number))->players_list).at(i).hand_chips != ((dealers_vector.at(room_number))->players_list).at(i).last_chips)
                    {
                        ((dealers_vector.at(room_number))->players_list).at(i).chip_account += (((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);

                        if (DzpkIfCpsOfType(dealers_vector[room_number]->room_type) == 0)
                        {} else
                        {
                            dzpkDbUpdateUserChip(((dealers_vector.at(room_number))->players_list).at(i).nUserID, ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips,
                                                 ((dealers_vector.at(room_number))->players_list).at(i).chip_account, 0, &((dealers_vector.at(room_number))->players_list).at(i));
                            //插入玩家牌局信息
                            dzpkDbInsertPlayCard(&((dealers_vector.at(room_number))->players_list).at(i), dealers_vector.at(room_number), ((dealers_vector.at(room_number))->players_list).at(i).hand_chips - ((dealers_vector.at(room_number))->players_list).at(i).last_chips);
                        }
                    }
                }
                msg_body["act_user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account;

                //更新玩家session中的位置：由房间回到大厅
                update_user_persition(((dealers_vector.at(room_number))->players_list).at(i).user_account, 0, connpool);
				
                player_property["user_id"] = ((dealers_vector.at(room_number))->players_list).at(i).user_id;
                player_property["user_account"] = ((dealers_vector.at(room_number))->players_list).at(i).user_account;
                player_property["nick_name"] = ((dealers_vector.at(room_number))->players_list).at(i).nick_name;
                player_property["dzpk_level"] = ((dealers_vector.at(room_number))->players_list).at(i).dzpk_level;
                player_property["dzpk_experience"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).dzpk_experience, 10);
                player_property["vip_level"] = ((dealers_vector.at(room_number))->players_list).at(i).vip_level;
                player_property["chip_account"] = itoa(((dealers_vector.at(room_number))->players_list).at(i).chip_account, 10);
                player_property["rank_name"] = ((dealers_vector.at(room_number))->players_list).at(i).rank_name;
                player_property["head_photo_serial"] = ((dealers_vector.at(room_number))->players_list).at(i).head_photo_serial;
                player_property["desk_chip"] = ((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
                player_property["user_staus"] = ((dealers_vector.at(room_number))->players_list).at(i).user_staus;
                player_property["room_num"] = ((dealers_vector.at(room_number))->players_list).at(i).room_num;
                player_property["seat_num"] = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                player_property["hand_chips"] = ((dealers_vector.at(room_number))->players_list).at(i).hand_chips;
                player_property["giftid"] = ((dealers_vector.at(room_number))->players_list).at(i).giftid;
                player_property["mall_path"] = ((dealers_vector.at(room_number))->players_list).at(i).mall_path;
                player_property["level_title"] = ((dealers_vector.at(room_number))->players_list).at(i).level_title;

                nContinue = 1;
                break;
            }
        }

        if (nContinue == 1)
        {
            msg_body["player_property"] = player_property;

            string   str_msg =   writer.write(msg_body);
            for (unsigned int i = 0; i < ((dealers_vector.at(room_number))->players_list).size(); i++)
            {
                if ((((dealers_vector.at(room_number))->players_list).at(i)).nUserID == nUserID && nType == 2)
                {
                    //转移房间自己不需要收到退出房间包
                } else
                {
                    DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd, CMD_EXIT_ROOM, (char *)str_msg.c_str(), strlen(str_msg.c_str()), dealers_vector[room_number]->dealer_id, atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
                }
            }

            for (its = ((dealers_vector.at(room_number))->players_list).begin(); its != ((dealers_vector.at(room_number))->players_list).end(); its++)
            {
                if ((*its).nUserID == nUserID)
                {
                    if ((*its).seat_num > 0)
                    {
                        WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** check force use exit room 0 \n", (*its).nUserID,
                                 (*its).start_countdown,
                                 time(0), time(0) - (*its).start_countdown,
                                 (*its).cd_rest_time);
                        if ((*its).start_countdown > 0)
                        {
                            int cd_time =  time(0) - (*its).start_countdown; //倒计时已经记录的时间
                            if (cd_time <  (*its).cd_rest_time)
                            {
                                (*its).cd_rest_time -= cd_time;
                            } else
                            {
                                (*its).cd_rest_time = 0;
                            }
                        } else
                        {
                            WriteLog(4008, "check user exit room  nick[%s] startcountdown[%d]  error \n", (*its).nick_name.c_str(),
                                     (*its).start_countdown);
                        }
                        WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL, "userid[%d]  time[%d][%d][%d] resttime[%d]  *********** check force use exit room 1 \n", (*its).nUserID,
                                 (*its).start_countdown,
                                 time(0), time(0) - (*its).start_countdown,
                                 (*its).cd_rest_time);
                    }
                    //更新用户的等级礼包时间
                    dzpkDbUpdateLevelGift((*its).nUserID, (*its).countdown_no, (*its).cd_rest_time, (*its).nTodayPlayTimes);

                    //更新玩家在房间时间
                    dzpkDbUserPlayTime((*its).nUserID, (*its).room_num, (*its).nEnterTime, (dealers_vector.at(room_number))->nRing - (*its).nEnterRing, (*its).nChipUpdate, (dealers_vector.at(room_number))->small_blind, (dealers_vector.at(room_number))->charge_per);

                    (dealers_vector.at(room_number))->leave_players_list.push_back(*its); //离开或站起的玩家 放入另一个队列保存
                    ((dealers_vector.at(room_number))->players_list).erase(its); //删除该玩家在房间内的信息
                    its--;
                }
            }
			
            DzpkUpdateRoomPerson(pRoom->dealer_id, connpool);        /*更新房间人数*/

            if (nType == 2)
            {
                //转移房间不用关闭socket
            } else
            {
                NetRemoveNetUser(nUserID);
            }
        }

        //将用户加入禁止进入列表
        if (nType == 1)
        {
            //清除过时记录
            StdUpdateNoInUser(pRoom);

            int nEndTime = time(NULL) + ROOM_PROHIBIT_USER_TIME;
            int nFind = 0;
            for (int i = 0; i < (int)pRoom->arryNoInUser.size(); i++)
            {
                if (pRoom->arryNoInUser[i].nUserID == nUserID)
                {
                    if (pRoom->arryNoInUser[i].nEndTime < nEndTime)
                    {
                        pRoom->arryNoInUser[i].nEndTime = nEndTime;
                    }
                    nFind = 1;
                    break;
                }
            }

            if (nFind == 0)
            {
                NO_IN_USER_T _sigle;
                _sigle.nUserID = nUserID;
                _sigle.nEndTime = nEndTime;
                if (pTemp1)
                {
                    sprintf(_sigle.szActionName, "%s", pTemp1);
                }
                if (pTemp2)
                {
                    sprintf(_sigle.szTargetName, "%s", pTemp2);
                }
                pRoom->arryNoInUser.push_back(_sigle);
            }
        }
    }

    return nRet;
}

void deal_prohibit_user(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //获取参数
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    string tar_account  =  (msg_commond["tar_user_id"].asString()).c_str(); //目标用户

    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        WriteLog(40077, "deal_prohibit_user can't find room (%d) &&&&  \n", room_number);
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    int nActUserID = 0,         //发起用户ID
        nTarUserID = 0;         //被投票用户ID
    char szTooltip[100];        //提示
    memset(szTooltip, 0, 100);
    sprintf(szTooltip, "发起踢人投票失败");

    char szActionName[50];      //发起者的昵称
    char szTargetName[50];
    memset(szActionName, 0, 50);
    memset(szTargetName, 0, 50);

    WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "enter prohibit user vote user_account(%s)  tar_account(%s)  \n", user_account.c_str(), tar_account.c_str());

    for (int i = 0; i < (int)pRoom->players_list.size(); i++)
    {
        if (pRoom->players_list[i].user_account == user_account)
        {
            if (pRoom->players_list[i].seat_num > 0)
            {
                if (pRoom->players_list[i].user_staus == 2 || pRoom->players_list[i].user_staus == 3 || pRoom->players_list[i].user_staus == 4 || pRoom->players_list[i].user_staus == 6)
                {
                    nActUserID = pRoom->players_list[i].nUserID;
                    memcpy(szActionName, pRoom->players_list[i].nick_name.c_str(), strlen(pRoom->players_list[i].nick_name.c_str()));
                    if (nUserID == nActUserID)
                    {} else
                    {
                        //用户信息不对应
                        nActUserID = 0;
                        WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote user error  . user_account(%s)  tar_account(%s) nUserID(%d)  nActUserID(%d)\n", user_account.c_str(), tar_account.c_str(), nUserID, nActUserID);
                    }
                } else
                {
                    sprintf(szTooltip, "不在牌局中用户不能发起踢人投票");
                    WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote user error  user status error . user_account(%s)  tar_account(%s) nUserID(%d)  nActUserID(%d) status(%d) \n", user_account.c_str(), tar_account.c_str(), nUserID, nActUserID, pRoom->players_list[i].user_staus);
                }
            } else
            {
                sprintf(szTooltip, "旁观用户不能发起踢人投票");
                WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote user error view user can't action . user_account(%s)  tar_account(%s) nUserID(%d)  nActUserID(%d)\n", user_account.c_str(), tar_account.c_str(), nUserID, nActUserID);
            }
        } else if (pRoom->players_list[i].user_account == tar_account)
        {
            if (pRoom->players_list[i].seat_num > 0)
            {
                memcpy(szTargetName, pRoom->players_list[i].nick_name.c_str(), strlen(pRoom->players_list[i].nick_name.c_str()));
                nTarUserID = pRoom->players_list[i].nUserID;
            } else
            {
                WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote user error target user view  . user_account(%s)  tar_account(%s) nUserID(%d)  nActUserID(%d)\n", user_account.c_str(), tar_account.c_str(), nUserID, nActUserID);
                sprintf(szTooltip, "不能对旁观用户发起踢人投票");
            }
        }
    }

    if (nActUserID  > 0 && nTarUserID > 0)
    {
        //判断当前是否已经在投票中
        if (pRoom->arryVote.size() > 0)
        {
            int nCurTime = time(NULL);
            //判断当前投票是否超时
            for (int i = 0; i < (int)pRoom->arryVote.size(); i++)
            {
                if (nCurTime - pRoom->arryVote[i].nStartTime  > ROOM_PROHIBIT_USER_TIMEOUT * 3)
                {
                    WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote error   record id[%d] info[%s] pass time[%d] \n", pRoom->arryVote[i].nVoteID, pRoom->arryVote[i].szFirstTooltip, nCurTime - pRoom->arryVote[i].nStartTime);
                    WriteRoomLog(40030, pRoom->dealer_id, nUserID, "prohibit user vote error   record id[%d] info[%s] pass time[%d] \n", pRoom->arryVote[i].nVoteID, pRoom->arryVote[i].szFirstTooltip, nCurTime - pRoom->arryVote[i].nStartTime);
                    pRoom->arryVote.erase(pRoom->arryVote.begin() + i);
                    i--;
                }
            }
        }
        if (pRoom->arryVote.size() > 0)
        {
            nActUserID = -1;
            nTarUserID = -1;
            sprintf(szTooltip, "正在踢人投票中，不能发起投票");
            WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote fail,a prohibit is voting.  action user [%s] target user[%s] \n", user_account.c_str(), tar_account.c_str());
            WriteRoomLog(4003, pRoom->dealer_id, nUserID, "prohibit user vote fail,a prohibit is voting.  action user [%s] target user[%s] \n", user_account.c_str(), tar_account.c_str());
        }
    }

    if (nActUserID  > 0 && nTarUserID > 0)
    {
        //判断是否已经对当前用户投票成功过
        for (int i = 0; i < (int)pRoom->arryForceLeaveUser.size(); i++)
        {
            if (pRoom->arryForceLeaveUser[i].nUserID == nTarUserID)
            {
                nActUserID = -1;
                nTarUserID = -1;
                sprintf(szTooltip, "%s 已经在被踢列表，不能再次发起投票", szTargetName);
                WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote fail,user had prohibit action user [%s] target user[%s] \n", user_account.c_str(), tar_account.c_str());
                WriteRoomLog(4003, pRoom->dealer_id, nUserID, "prohibit user vote fail,user had prohibit action user [%s] target user[%s] \n", user_account.c_str(), tar_account.c_str());
                break;
            }
        }
    }

    if (nActUserID  > 0 && nTarUserID > 0)
    {
        //判断用户是否有踢人卡,有的话消耗一张
        int nProhibitCardCount = 0;
        if (dzpkDbUseProhibitCar(nActUserID, &nProhibitCardCount) == 0)
        {
            char szTooltip[100];
            sprintf(szTooltip, "%s 使用了一个踢人卡", szActionName);
            dzpkDbWritePacketLog(4, pRoom->dealer_id, nUserID, -1, szTooltip, nProhibitCardCount + 1);
        } else
        {
            nActUserID = -1;
            nTarUserID = -1;
            sprintf(szTooltip, "对不起，您的踢人卡数量为零，请移步到商城购买");
            WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote fail,user had not prohibit card. action user [%s] target user[%s] szTooltip:(%s)\n", user_account.c_str(), tar_account.c_str(), szTooltip);
            WriteRoomLog(4003, pRoom->dealer_id, nUserID, "prohibit user vote fail,user had not prohibit card. action user [%s] target user[%s] \n", user_account.c_str(), tar_account.c_str());
        }
    }

    if (nActUserID  > 0 && nTarUserID > 0)
    {
        WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "enter prohibit user vote user_account(%s)  tar_account(%s) success \n", user_account.c_str(), tar_account.c_str());
        dztimer timer;

        timer.nTimerID = DzpkGetTimerID();
        timer.room_num = room_number;
        timer.start_time = time(NULL);
        timer.timer_type = 4;
        timer.time_out = ROOM_PROHIBIT_USER_TIMEOUT;
        g_dzpkTimer.Insert(timer.nTimerID, (void *)&timer, sizeof(dztimer));


        PROHIBIT_INFO_T _Sigle;
        _Sigle.nVoteID = timer.nTimerID;
        _Sigle.nSendUserID = nActUserID;
        _Sigle.nVoteUserID = nTarUserID;
        _Sigle.nVoteUserID = nTarUserID;
        _Sigle.nStartTime = timer.start_time;
        sprintf(_Sigle.szActionName, "%s", szActionName);
        sprintf(_Sigle.szTargetName, "%s", szTargetName);
        sprintf(_Sigle.szTooltip, "踢走 %s 投票", szTargetName);

        sprintf(_Sigle.szFirstTooltip, "用户 %s 想踢走用户 %s，现在询问你的意见", szActionName, szTargetName);

        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body["msg_head"] = "dzpk";
        msg_body["game_number"] = "dzpk";
        msg_body["area_number"] = "normal";
        msg_body["room_number"] = pRoom->dealer_id;
        msg_body["act_commond"] = CMD_PROHIBIT_USER_VOTE;
        msg_body["tooltip"] = _Sigle.szFirstTooltip;
        msg_body["prohibit_record_id"] = _Sigle.nVoteID;          //记录ID
        msg_body["time_out"] = timer.time_out;                    //超时
        msg_body["view"] = 0;                                      //是否是旁观
        string   str_msg =   writer.write(msg_body);

        Json::FastWriter  writer1;
        Json::Value  msg_body1;
        msg_body1["msg_head"] = "dzpk";
        msg_body1["game_number"] = "dzpk";
        msg_body1["area_number"] = "normal";
        msg_body1["room_number"] = pRoom->dealer_id;
        msg_body1["act_commond"] = CMD_PROHIBIT_USER_VOTE;
        msg_body1["tooltip"] = _Sigle.szFirstTooltip;
        msg_body1["prohibit_record_id"] = _Sigle.nVoteID;          //记录ID
        msg_body1["time_out"] = timer.time_out;                    //超时
        msg_body["view"] = 1;                                      //是否是旁观
        string   str_msg1 =   writer1.write(msg_body1);

        for (int i = 0; i < (int)pRoom->players_list.size(); i++)
        {
            if (pRoom->players_list[i].seat_num > 0)
            {
                if (pRoom->players_list[i].nUserID == nActUserID || pRoom->players_list[i].nUserID == nTarUserID)
                {
                    //发送显示投票信息
                    DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_PROHIBIT_USER_VOTE, (char *)str_msg1.c_str(), strlen(str_msg1.c_str()), pRoom->dealer_id);
                } else
                {
                    if (_Sigle.nArryInfoIndex >= 0 && _Sigle.nArryInfoIndex < ROOM_SEAT_DOWN_MAX_NUMBER)
                    {
                        _Sigle.arryInfo[_Sigle.nArryInfoIndex].nUserID = pRoom->players_list[i].nUserID;
                        _Sigle.arryInfo[_Sigle.nArryInfoIndex].nResult = -100;
                        _Sigle.nArryInfoIndex++;

                        //发送投票信息
                        DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_PROHIBIT_USER_VOTE, (char *)str_msg.c_str(), strlen(str_msg.c_str()), pRoom->dealer_id);
                    }
                }
            } else
            {
                //发送显示投票信息
                DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_PROHIBIT_USER_VOTE, (char *)str_msg1.c_str(), strlen(str_msg1.c_str()), pRoom->dealer_id);
            }
        }

        pRoom->arryVote.push_back(_Sigle);

        ProhibitUserVote(pRoom, -1, -100, _Sigle.nVoteID, 0);
    } else
    {
        Json::FastWriter  writer1;
        Json::Value  msg_body1;

        msg_body1["msg_head"] = "dzpk";
        msg_body1["game_number"] = "dzpk";
        msg_body1["area_number"] = "normal";
        msg_body1["room_number"] = pRoom->dealer_id;
        msg_body1["act_commond"] = CMD_PROHIBIT_USER;
        msg_body1["tooltip"] = szTooltip;
        msg_body1["result"] = -1;

        string   str_msg1 =   writer1.write(msg_body1);
        DzpkSendToUser(nUserID, CMD_PROHIBIT_USER, (char *)str_msg1.c_str(), strlen(str_msg1.c_str()), pRoom->dealer_id);
    }

}

void deal_prohibit_user_vote(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //获取参数
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
    int nRecord   = atoi((msg_commond["record_vote_id"].asString()).c_str()); //记录ID
    int nResult   = atoi((msg_commond["record_vote_result"].asString()).c_str()); //记录结果

    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        WriteLog(40077, "deal_prohibit_user can't find room (%d) &&&&  \n", room_number);
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    for (int i = 0; i < (int)pRoom->players_list.size(); i++)
    {
        if (pRoom->players_list[i].user_account == user_account)
        {
            WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prohibit user vote user count(%s) recordID(%d)   vote(%d)  \n", user_account.c_str(), nRecord, nResult);
            ProhibitUserVote(pRoom, pRoom->players_list[i].nUserID, nResult, nRecord, 0);
            break;
        }
    }
}

void deal_re_read_pack(int nUserID, vector<dealer *> &dealers_vector, Json::Value msg_commond, ConnPool *connpool, vector<level_system>  level_list, DZPK_USER_T *pSocketUser, PACKET_HEAD_T *pHead)
{
    //获取参数
    int room_number   = atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    int user_id  =  atoi((msg_commond["act_user_id"].asString()).c_str()); //用户ID
    int nType  =  atoi((msg_commond["user_info_update_type"].asString()).c_str()); //更新类型  1 更新玩家礼物  2 重读玩家时间类道具 3 充值任务加一 4、重新读取用户vip信息 5、更新用户筹码

    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if (pRoom)
    {
        room_number = pRoom->nVecIndex;
    } else
    {
        WriteLog(40077, "deal_prohibit_user can't find room (%d) &&&&  \n", room_number);
        return;
    }
    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    //判断是否是否注册
    if (StdIfUserRegister(nUserID, pSocketUser, 0) != 0)
    {
        return;
    }

    //判断用户是否是房间里的用户
    if (StdIfUserInRoom(pRoom, nUserID) != 0)
    {
        return;
    }

    for (int i = 0; i < (int)pRoom->players_list.size(); i++)
    {
        if (pRoom->players_list[i].nUserID == user_id)
        {
            switch (nType)
            {
            case 1:     //重新读礼物
                {
                    //重新读取礼物
                    dzpkDbReadUserInfo(&pRoom->players_list[i], 1);

                    //发送中间结果
                    Json::FastWriter  writer;
                    Json::Value  msg_body;
                    msg_body["msg_head"] = "dzpk";
                    msg_body["game_number"] = "dzpk";
                    msg_body["area_number"] = "normal";
                    msg_body["room_number"] = pRoom->dealer_id;
                    msg_body["act_commond"] = CMD_RE_READ_USER_PACK;
                    msg_body["giftid"] = pRoom->players_list[i].giftid;               //礼物背包ID
                    msg_body["mall_path"] = pRoom->players_list[i].mall_path;         //礼物路径
                    msg_body["room_num"] = pRoom->players_list[i].room_num;           //房间ID
                    msg_body["seat_num"] = pRoom->players_list[i].seat_num;           //座位号
                    msg_body["user_id"] = pRoom->players_list[i].user_id;             //用户ID
                    msg_body["user_account"] = pRoom->players_list[i].user_account;   //用户帐号
                    msg_body["user_info_update_type"] = 1;                            //更新用户信息类型  1 为更新礼物
                    string   str_msg =   writer.write(msg_body);

                    //通知房间所有用户
                    for (int l = 0; l < (int)pRoom->players_list.size(); l++)
                    {
                        DzpkSendToUser(pRoom->players_list[l].nUserID, CMD_RE_READ_USER_PACK, (char *)str_msg.c_str(), strlen(str_msg.c_str()), pRoom->dealer_id);
                    }
                }
                break;
            case 2:         //重新读背包
                {
                    WriteLog(USER_VIP_CARD_LOG_LEVEL, " deal_re_read_pack  user account(%s) \n", pRoom->players_list[i].user_account.c_str());
                    pRoom->players_list[i].nReadBpFromDb = 0;
                }
                break;
            case 3:         //充值任务完成，检查是否可领取
                {
                    pRoom->players_list[i].nReadTaskFromDb = 0;
                    dzpkCheckTaskComplete(&pRoom->players_list[i], 4);
                }
                break;
            case 4:        //重新读取vip信息
                {
                    pRoom->players_list[i].nReadBpFromDb = 0;
                    //获得用户的vip等级
                    int nMallID = 0;
                    StdIfVipCard(&pRoom->players_list[i], nMallID, &pRoom->players_list[i].nVipLevel);
                }
                break;
            case 5:        //重新读取用户筹码
                {
                    user  newuser;
                    if (dzpkGetDbUserInfo(newuser, (char *)pRoom->players_list[i].user_account.c_str(), pRoom->players_list[i].socket_fd, pRoom->players_list[i].room_num) == 0)
                    {
                        pRoom->players_list[i].chip_account = newuser.chip_account;
                        if (pRoom->players_list[i].chip_account < pRoom->players_list[i].hand_chips)
                        {
                            //更新玩家携带筹码数
                            pRoom->players_list[i].hand_chips = pRoom->players_list[i].chip_account;
                        }
                    }
                }
                break;
            default:
                {}
                break;
            }


            break;
        }
    }
}

/*投票*/
int ProhibitUserVote(dealer *pRoom, int nUserID, int nResult, int nRecordID, int nTimer)
{
    if (pRoom)
    {
        class CAutoLock Lock(&pRoom->Mutex);
        if (nTimer > 0)
        {
            Lock.Lock();
        }

        WriteRoomLog(4003, pRoom->dealer_id, nUserID, "vote sigle info   vote record id[%d] result[%d] \n", nRecordID, nResult);

        PROHIBIT_INFO_T *pRecord = NULL;

        for (int i = 0; i < (int)pRoom->arryVote.size(); i++)
        {
            if (pRoom->arryVote[i].nVoteID == nRecordID)
            {
                pRecord = &pRoom->arryVote[i];
                break;
            }
        }

        if (pRecord)
        {
            int nFinal = 0;
            if (nTimer > 0)
            {
                nFinal = 1;
            } else
            {
                //更新投票
                if (nUserID > 0)
                {
                    if (nResult == -1 || nResult == 0 || nResult == 1)
                    {
                        for (int i = 0; i < pRecord->nArryInfoIndex; i++)
                        {
                            if (pRecord->arryInfo[i].nUserID == nUserID)
                            {
                                pRecord->arryInfo[i].nResult = nResult;
                                break;
                            }
                        }
                    }
                }

                //检查是否可以结算
                nFinal = 1;
                for (int i = 0; i < (int)pRecord->nArryInfoIndex; i++)
                {
                    if (pRecord->arryInfo[i].nResult < -1)
                    {
                        nFinal = 0;
                        break;
                    }
                }

                if (nFinal == 1)
                {
                    g_dzpkTimer.Remove(nRecordID);
                }
            }

            if (nFinal == 1)
            {
                //记录结算
                int nFinalResult = 0;
                for (int i = 0; i < pRecord->nArryInfoIndex; i++)
                {
                    if (pRecord->arryInfo[i].nResult >= -1)
                    {
                        nFinalResult += pRecord->arryInfo[i].nResult;
                    }
                }

                WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prihibit user final result record id[%d]  final result(%d) nTimer(%d)  \n", pRecord->nVoteUserID, nFinalResult, nTimer);

                //发送结果
                Json::FastWriter  writer1;
                Json::Value  msg_body1;
                char szTooltip[100];
                memset(szTooltip, 0, 100);

                msg_body1["msg_head"] = "dzpk";
                msg_body1["game_number"] = "dzpk";
                msg_body1["area_number"] = "normal";
                msg_body1["room_number"] = pRoom->dealer_id;
                msg_body1["act_commond"] = CMD_PROHIBIT_USER;

                if (nFinalResult >= 0)
                {
                    sprintf(szTooltip, "%s 成功", pRecord->szTooltip);
                    msg_body1["result"] = 1;

                    //成功加入禁用数组
                    FORCE_LEAVE_USER_T  _LeaveUser;
                    _LeaveUser.nUserID = pRecord->nVoteUserID;
                    sprintf(_LeaveUser.szActionName, "%s", pRecord->szActionName);
                    sprintf(_LeaveUser.szTargetName, "%s", pRecord->szTargetName);
                    pRoom->arryForceLeaveUser.push_back(_LeaveUser);
                } else
                {
                    msg_body1["result"] = 0;
                    sprintf(szTooltip, "%s 失败", pRecord->szTooltip);
                }
                msg_body1["tooltip"] = szTooltip;

                string   str_msg1 =   writer1.write(msg_body1);

                for (int i = 0; i < (int)pRoom->players_list.size(); i++)
                {
                    DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_PROHIBIT_USER, (char *)str_msg1.c_str(), strlen(str_msg1.c_str()), pRoom->dealer_id);
                }
                //删除记录
                for (int i = 0; i < (int)pRoom->arryVote.size(); i++)
                {
                    if (pRoom->arryVote[i].nVoteID == nRecordID)
                    {
                        pRoom->arryVote.erase(pRoom->arryVote.begin() + i);
                        break;
                    }
                }
            } else
            {
                int nSupport = 0, nOppose = 0, nAbstain = 0;

                //统计票数
                for (int i = 0; i < pRecord->nArryInfoIndex; i++)
                {
                    if (pRecord->arryInfo[i].nResult >= -1)
                    {
                        if (pRecord->arryInfo[i].nResult == -1)
                        {
                            nOppose++;
                        } else if (pRecord->arryInfo[i].nResult == 0)
                        {
                            nAbstain++;
                        } else if (pRecord->arryInfo[i].nResult == 1)
                        {
                            nSupport++;
                        }
                    }
                }

                WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL, pRoom->dealer_id, nUserID, "prihibit user mid result record id[%d] count(%d)  %d<>%d<>%d   \n", pRecord->nVoteUserID, pRecord->nArryInfoIndex, nSupport, nAbstain, nOppose);

                //发送中间结果
                Json::FastWriter  writer;
                Json::Value  msg_body;
                msg_body["msg_head"] = "dzpk";
                msg_body["game_number"] = "dzpk";
                msg_body["area_number"] = "normal";
                msg_body["room_number"] = pRoom->dealer_id;
                msg_body["act_commond"] = CMD_PROHIBIT_MID_RESULT;
                msg_body["tooltip"] = pRecord->szFirstTooltip;
                msg_body["prohibit_record_id"] = pRecord->nVoteID;          //记录ID
                msg_body["support_vote_count"] = nSupport;
                msg_body["oppose_vote_count"] = nOppose;
                msg_body["abstain_vote_count"] = nAbstain;
                msg_body["record_vote_size"] = pRecord->nArryInfoIndex;
                msg_body["action_user_nick"] = pRecord->szActionName;
                msg_body["target_user_nick"] = pRecord->szTargetName;
                string   str_msg =   writer.write(msg_body);

                for (int i = 0; i < (int)pRoom->players_list.size(); i++)
                {
                    DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_PROHIBIT_MID_RESULT, (char *)str_msg.c_str(), strlen(str_msg.c_str()), pRoom->dealer_id);
                }
            }
        } else
        {
            WriteRoomLog(40030, pRoom->dealer_id, nUserID, "vote can't find record   vote record id[%d] result[%d] \n", nRecordID, nResult);
        }
    }
    return 0;
}
/*进入获得投票详细信息*/
int ProhibitUserGetInfo(dealer *pRoom, int nUserID)
{
    int nRet = 0;

    if (pRoom)
    {
        if (pRoom->arryVote.size() > 0)
        {
            PROHIBIT_INFO_T *pRecord = &pRoom->arryVote[0];

            int nCurTime = time(NULL);
            if (pRecord && nCurTime - pRecord->nStartTime < ROOM_PROHIBIT_USER_TIMEOUT)
            {
                //发自己是否能投票信息
                Json::FastWriter  writer1;
                Json::Value  msg_body1;
                msg_body1["msg_head"] = "dzpk";
                msg_body1["game_number"] = "dzpk";
                msg_body1["area_number"] = "normal";
                msg_body1["room_number"] = pRoom->dealer_id;
                msg_body1["act_commond"] = CMD_PROHIBIT_USER_VOTE;
                msg_body1["tooltip"] = pRecord->szFirstTooltip;
                msg_body1["prohibit_record_id"] = pRecord->nVoteID;          //记录ID
                msg_body1["time_out"] = ROOM_PROHIBIT_USER_TIMEOUT - (nCurTime - pRecord->nStartTime);                    //超时

                //自己是否能投票
                int nView = 1;
                for (int i = 0; i < pRecord->nArryInfoIndex; i++)
                {
                    if (pRecord->arryInfo[i].nUserID == nUserID)
                    {
                        if (pRecord->arryInfo[i].nResult < -1)
                        {
                            nView = 0;
                        }
                    }
                }
                msg_body1["view"] = nView;                                      //是否是旁观
                string   str_msg1 =   writer1.write(msg_body1);
                DzpkSendToUser(nUserID, CMD_PROHIBIT_USER_VOTE, (char *)str_msg1.c_str(), strlen(str_msg1.c_str()), pRoom->dealer_id);


                //正在投票结果
                int nSupport = 0, nOppose = 0, nAbstain = 0;

                //统计票数
                for (int i = 0; i < pRecord->nArryInfoIndex; i++)
                {
                    if (pRecord->arryInfo[i].nResult >= -1)
                    {
                        if (pRecord->arryInfo[i].nResult == -1)
                        {
                            nOppose++;
                        } else if (pRecord->arryInfo[i].nResult == 0)
                        {
                            nAbstain++;
                        } else if (pRecord->arryInfo[i].nResult == 1)
                        {
                            nSupport++;
                        }
                    }
                }

                //发送中间结果
                Json::FastWriter  writer;
                Json::Value  msg_body;
                msg_body["msg_head"] = "dzpk";
                msg_body["game_number"] = "dzpk";
                msg_body["area_number"] = "normal";
                msg_body["room_number"] = pRoom->dealer_id;
                msg_body["act_commond"] = CMD_PROHIBIT_MID_RESULT;
                msg_body["tooltip"] = pRecord->szFirstTooltip;
                msg_body["prohibit_record_id"] = pRecord->nVoteID;          //记录ID
                msg_body["support_vote_count"] = nSupport;
                msg_body["oppose_vote_count"] = nOppose;
                msg_body["abstain_vote_count"] = nAbstain;
                msg_body["record_vote_size"] = pRecord->nArryInfoIndex;
                msg_body["action_user_nick"] = pRecord->szActionName;
                msg_body["target_user_nick"] = pRecord->szTargetName;
                string   str_msg =   writer.write(msg_body);
                DzpkSendToUser(nUserID, CMD_PROHIBIT_MID_RESULT, (char *)str_msg.c_str(), strlen(str_msg.c_str()), pRoom->dealer_id);
            }
        }
    }

    return nRet;
}
/*强迫用户退出房间*/
int ForceUserExitRoom(int nRoomNumber, int nUserID, int nType)
{
	
	WriteLog(4000,"guobin5----getin ForceUserExitRoomn\n");
    int nRemoveUser = 1;
    if (nType == 0)
    {
        int room_number = 0;
        /*取得房间指针*/
        dealer *pRoom = DzpkGetRoomInfo(nRoomNumber);
        if (pRoom)
        {
            room_number = pRoom->nVecIndex;

            int nExit = 0;
            string sUserAccount;

            class CAutoLock Lock(&dealers_vector[room_number]->Mutex);
            Lock.Lock();
            for (int k = 0; k < (int)dealers_vector[room_number]->players_list.size(); k++)
            {
                if (dealers_vector[room_number]->players_list[k].nUserID == nUserID)
                {
                    sUserAccount = dealers_vector[room_number]->players_list[k].user_account;
                    nExit = 1;
                    break;
                }
            }
            Lock.Unlock();

            if (nExit == 1)
            {
                Json::Value   qp_commond;
                qp_commond["msg_head"] = "dzpk";
                qp_commond["game_number"] = "dzpk";
                qp_commond["area_number"] = "normal";
                qp_commond["room_number"] = GetString(nRoomNumber);
                qp_commond["act_user_id"] = sUserAccount;
                qp_commond["act_commond"] = CMD_EXIT_ROOM;
                qp_commond["timer_id"] = "";
                qp_commond["force_exit"] = "0";

                //取得断线玩家离开房间完函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(dealers_vector.at(room_number)->dealer_id);
                if (pModule->msgExitRoom)
                {
                    pModule->msgExitRoom(nUserID, dealers_vector, qp_commond, connpool, level_list, NULL, NULL);
                    nRemoveUser = 0;
                } else
                {}
            }
        }
    }

    if (nRemoveUser == 1)
    {
        NetRemoveNetUser(nUserID);
    }
	
    return 0;
}


/**
*      给荷官小费
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
int dealGiveTipToDealer(int nRoomdID, DZPK_USER_T *pUser, string seat_num)
{
    int nRet = 0;
    int errCode = 0;

    dealer *pRoom = DzpkGetRoomInfo(nRoomdID);
    int smallBlind = pRoom->small_blind;
    //int handChip = pUser->pUser->hand_chips;
    int nUserID = pUser->pUser->nUserID;
    int handChip = 0;

    if (pRoom)
    {
        class CAutoLock Lock(&pRoom->Mutex);
        Lock.Lock();

        //判断用户是否是房间里的用户
        if (StdIfUserInRoom(pRoom, nUserID) != 0)
        {
            return -1;
        }

        int idx = 0;
        for (unsigned int i = 0; i < (pRoom->players_list).size(); i++)
        {
            if ((pRoom->players_list).at(i).nUserID == nUserID)
            {
                handChip = (pRoom->players_list).at(i).hand_chips;
                idx = i;
                break;
            }
        }

        if (handChip < smallBlind
            || ((handChip - smallBlind) < (smallBlind * 2 + smallBlind * (pRoom->charge_per) * 2 / 100)))
        {
            errCode = 1;
        } else
        {
            (pRoom->players_list).at(idx).hand_chips = (pRoom->players_list).at(idx).hand_chips - smallBlind;
            dzpkGiveTipUpdateData(pRoom->small_blind, nUserID);
        }

        //发送操作
        Json::FastWriter writer;
        Json::Value msgBody;

        msgBody["msg_head"] = "dzpk";
        msgBody["game_number"] = "dzpk";
        msgBody["area_number"] = "normal";
        msgBody["act_commond"] = CMD_GIVE_TIP_TO_DEALER;
        msgBody["user_id"] = pUser->pUser->user_id;
        msgBody["seat_num"] = seat_num;
        msgBody["error_code"] = errCode;

        string msgstr = writer.write(msgBody);

        for (unsigned int i = 0; i < pRoom->players_list.size(); i++)
        {
            DzpkSendToUser(pRoom->players_list[i].nUserID, CMD_GIVE_TIP_TO_DEALER, (char *)(msgstr.c_str()), msgstr.size(), pRoom->dealer_id);
        }
    } else
    {
        WriteLog(40005, "deal send msg to room cant't find room [%d] user [%d] \n", nRoomdID, nUserID);
        return -1;
    }

    return nRet;
}

