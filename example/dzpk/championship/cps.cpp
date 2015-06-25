/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.02
** 版本：1.0
** 说明：德州扑克锦标赛
**************************************************************
*/
#include "../mainDzpk.h"
#include "../Start.h"
#include "cps.h"
#include "../../../com/common.h"
#include "../standard/dzpkParsePacket.h"
#include "../db/dzpkDb.h"
#include "../standard/dzpkCustom.h"
#include "../standard/dzpkHead.h"
#include "cpsCustom.h"
#include "cpsUser.h"

using namespace  std;

int CpsTimer()
{
	static int nTickTime = GetTickTime();
    int nCurTickTime = GetTickTime();
    if(nCurTickTime -  nTickTime > 1000)
    {
        nTickTime = nCurTickTime;

        //检查活动是否开始
        CpsCheckRecord();

        //检查用户是否长时间没操作
        CpsCheckUserOut();

        //读配置文件
        CpsReadDbEveryDay();
    }
    return 0;
}
/**
*      处理进入房间消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*/
void CpsEnterRoom( int socketfd,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{
    int nRet = 0;
    user  newuser;
    int room_number = 0;
    int nPreRoomID = 0;
    int nEnterCode = ERR_ENTER_ROOM_ERROR;
    int nCpsRecordID  = atoi((msg_commond["cps_record_id"].asString()).c_str());


    //获取用户的房间号
    ENTER_ROOM_REQ_T recInfo;
    if(dzpkGetEnterRoomPack(&recInfo,msg_commond) != 0)
    {
        //解释网络包出错
        return;
    }

    if(nRet == 0)
    {
        //获得用户数据库信息
        nRet= dzpkGetDbUserInfo(newuser,recInfo.act_user_id,socketfd,0);
    }

    //网络库用户注册
    if(nRet == 0)
    {
        nRet = -1;
        int nUserID = newuser.nUserID;
        if(nUserID > 0)
        {
            //更新用户资料
            pSocketUser->nUserID = nUserID;

            /*用户注册*/
            void *pUserInfo = NULL;
            pUserInfo = NetGetNetUser(nUserID);
            if( NULL != pUserInfo)
            {
                nPreRoomID = ((DZPK_USER_T *)pUserInfo)->nRoomID;

                if(nCpsRecordID <= 0)
                {
                    CPS_USER_T *pRecordUser = CpsGetUser(nUserID);
                    if(pRecordUser)
                    {
                        nCpsRecordID = pRecordUser->nCpsID;
                        CpsFreeNetUser(pRecordUser);
                    }
                }

                WriteRoomLog(4003,((DZPK_USER_T *)pUserInfo)->nRoomID,nUserID,"user has in a room previous room [%d] \n",((DZPK_USER_T *)pUserInfo)->nRoomID);
                NetFreeNetUser(pUserInfo);
                NetRemoveNetUser(nUserID);
            }

            pUserInfo = NetGetNetUser(nUserID);
            if( NULL == pUserInfo)
            {
                if(NetAddUserToNet(nUserID,(void *)pSocketUser,sizeof(DZPK_USER_T)) == 0)
                {
                    nRet = 0;
                }
            }
        }
    }


    if(nRet == 0)
    {
        CPS_RECORD_T * pRecord = CpsGetRecord(nCpsRecordID);
        if(pRecord)
        {
            if(pRecord->nStart == 0)
            {
                class CAutoLock Lock(pRecord->pMutex);
                Lock.Lock();

                int nContinue = 1;
                if(nPreRoomID > 0)
                {
                    nRet = -1;

                    int nFind = 0;
                    for(int i=0;i<pRecord->nArryRoomIndex;i++)
                    {
                        if(pRecord->arryRoom[i] == nPreRoomID)
                        {
                            nFind = 1;
                            break;
                        }
                    }

                    if(nFind == 1)
                    {
                        dealer *pRoom = DzpkGetRoomInfo(nPreRoomID);
                        if(pRoom)
                        {
                            class CAutoLock Lock(&pRoom->Mutex);
                            Lock.Lock();

                            if(pRoom->nCpsRecordID == pRecord->nRecordID)
                            {
                                for(int i=0;i<(int)pRoom->players_list.size();i++)
                                {
                                    if(pRoom->players_list[i].nUserID == newuser.nUserID)
                                    {
                                        nRet = CpsEnter(pRoom,newuser);
                                        if(nRet == 0)
                                        {
                                            //更新用户在那个房间
                                            CpsUpdateUserInRoom(newuser.nUserID,pRoom->dealer_id);

                                            //返回倒计时
                                            int nCountDown = pRecord->nStartTime - CpsGetTime();
                                            if(nCountDown < 0)
                                            {
                                                nCountDown = 0;
                                            }
                                            char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                                            int nSendLen = SEND_PACK_MAX_LEN;
                                            if(pSend)
                                            {
                                                memset(pSend,0,nSendLen);
                                                int nSuccess = dzpkPutCpsCountDown(pRoom,nCountDown,pSend,nSendLen);
                                                if(nSuccess == 0)
                                                {
                                                    DzpkSendToUser(newuser.nUserID,CMD_CPS_COUNTDOWN,pSend,nSendLen,pRoom->dealer_id);
                                                }

                                                free(pSend);
                                                pSend = NULL;
                                            }
                                            else
                                            {
                                                nRet = -1;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    nContinue = 0;
                }

                if(nContinue  == 1)
                {
                    /*取得房间指针*/
                    dealer *pRoom = CpsGetUseRoom(pRecord);

                    int nRegister = 0;                                             //是否需要注册
                    CPS_USER_T _RecordUser;
                    memset(&_RecordUser,0,sizeof(CPS_USER_T));
                    if(pRoom)
                    {
                        class CAutoLock Lock(&pRoom->Mutex);
                        Lock.Lock();

                        if(nRet == 0)
                        {
                            newuser.hand_chips = pRecord->nInitHandChip;          //分配1000筹码
                            nRet = CpsEnter(pRoom,newuser);
                            if(nRet == 0)
                            {
                                nRet = CpsSeatDownIn(pRoom,newuser.nUserID);
                                if(nRet != 0)
                                {
                                    //退出房间
                                }
                                else
                                {
                                    //返回倒计时
                                    int nCountDown = pRecord->nStartTime - CpsGetTime();
                                    if(nCountDown < 0)
                                    {
                                        nCountDown = 0;
                                    }
                                    char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                                    int nSendLen = SEND_PACK_MAX_LEN;
                                    if(pSend)
                                    {
                                        memset(pSend,0,nSendLen);
                                        int nSuccess = dzpkPutCpsCountDown(pRoom,nCountDown,pSend,nSendLen);
                                        if(nSuccess == 0)
                                        {
                                            DzpkSendToUser(newuser.nUserID,CMD_CPS_COUNTDOWN,pSend,nSendLen,pRoom->dealer_id);
                                        }

                                        nRegister = 1;
                                        _RecordUser.nCpsID = pRecord->nRecordID;
                                        _RecordUser.nCurHandChip = newuser.hand_chips;
                                        _RecordUser.nStart = 1;
                                        _RecordUser.nType = pRecord->nType;
                                        _RecordUser.nUserID = newuser.nUserID;
                                        _RecordUser.nFailTime = 0;
                                        _RecordUser.nActiveTime = time(NULL);
                                        _RecordUser.nEnterTime = time(NULL);
                                        _RecordUser.nUserLeaveGame = 0;

                                        //更新用户在那个房间
                                        CpsUpdateUserInRoom(newuser.nUserID,pRoom->dealer_id);

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
                    }
                    else
                    {
                        nRet = -1;
                    }

                    //注册
                    if(nRegister == 1)
                    {
                        CpsInserUser(_RecordUser.nUserID,&_RecordUser,sizeof(CPS_USER_T));
                    }
                }
            }
            else if(pRecord->nStart == 1)
            {
                nRet = -1;
                nEnterCode = ERR_CPS_HAD_START;

                class CAutoLock Lock(pRecord->pMutex);
                Lock.Lock();

                if(nPreRoomID > 0)
                {
                    int nFind = 0;
                    for(int i=0;i<pRecord->nArryRoomIndex;i++)
                    {
                        if(pRecord->arryRoom[i] == nPreRoomID)
                        {
                            nFind = 1;
                            break;
                        }
                    }


                    if(nFind == 1)
                    {
                        dealer *pRoom = DzpkGetRoomInfo(nPreRoomID);
                        if(pRoom)
                        {
                            class CAutoLock Lock(&pRoom->Mutex);
                            Lock.Lock();

                            if(pRoom->nCpsRecordID == pRecord->nRecordID)
                            {
                                for(int i=0;i<(int)pRoom->players_list.size();i++)
                                {
                                    if(pRoom->players_list[i].nUserID == newuser.nUserID)
                                    {
                                        nRet = CpsEnter(pRoom,newuser);
                                        if(nRet == 0)
                                        {
                                            //更新用户在那个房间
                                            CpsUpdateUserInRoom(newuser.nUserID,pRoom->dealer_id);

                                            room_number = pRoom->nVecIndex;

                                            //判断是否到我操作
                                            if((dealers_vector.at(room_number))->step != 0)
                                            {
                                                SAFE_HASH_LIST_T * pList = g_dzpkTimer.GetList();
                                                if(pList)
                                                {
                                                    dztimer *pTimer = NULL;
                                                    for(int i=0;i<pList->nSize;i++)
                                                    {
                                                        pTimer = (dztimer *)pList->pTable[i];
                                                        if( pTimer &&  pTimer->room_num == room_number && pTimer->nUserID == pSocketUser->nUserID && pTimer->timer_type == 1)
                                                        {
                                                            //有弃牌定时器
                                                            //判断下一个玩家是不是我
                                                            for(unsigned int p=0;p< (dealers_vector.at(room_number))->players_list.size();p++)
                                                            {
                                                                if(  ((dealers_vector.at(room_number))->players_list).at(p).user_id == newuser.user_id )
                                                                {
                                                                    if(dealers_vector.at(room_number)->turn > 0 && dealers_vector.at(room_number)->turn == ((dealers_vector.at(room_number))->players_list).at(p).seat_num)
                                                                    {
                                                                        //发送操作
                                                                        Json::FastWriter  writer;
                                                                        Json::Value  msg_body;

                                                                        msg_body["msg_head"]="dzpk";
                                                                        msg_body["game_number"]="dzpk";
                                                                        msg_body["area_number"]="normal";
                                                                        msg_body["act_commond"]=CMD_RE_ENTER_TURN_ME;
                                                                        msg_body["min_chip"]=dealers_vector.at(room_number)->nTurnMinChip;
                                                                        msg_body["max_chip"]=dealers_vector.at(room_number)->nTurnMaxChip;
                                                                        msg_body["timer_id"]=pTimer->nTimerID;

                                                                        Json::Value  hand_cards;
                                                                        if(((dealers_vector.at(room_number))->players_list).at(p).hand_cards.size() >= 2)
                                                                        {
                                                                            hand_cards.append(((dealers_vector.at(room_number))->players_list).at(p).hand_cards[0]);
                                                                            hand_cards.append( ((dealers_vector.at(room_number))->players_list).at(p).hand_cards[1]);
                                                                        }
                                                                        msg_body["hand_cards"]=hand_cards;

                                                                        string   str_msg =   writer.write(msg_body);
                                                                        DzpkSendP((void*)pSocketUser,CMD_RE_ENTER_TURN_ME,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,pSocketUser->nUserID);
                                                                    }
                                                                    else
                                                                    {
                                                                        WriteLog(4005,"turn error  turn[%d] seat num[%d]\n",dealers_vector.at(room_number)->turn,((dealers_vector.at(room_number))->players_list).at(p).seat_num);
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
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                //已经结束
                nRet = -1;
            }
            CpsFreeRecord(pRecord);
        }
        else
        {

            nRet = -1;
        }
    }
    if(nRet == 0)
    {
        //用户进入房间成功
        //更新用户信息
        StdUpdateUserInfo(newuser.nUserID,newuser);

        //发送排名回去
        CpsSendRankToUser(nCpsRecordID,newuser.nUserID);
    }
    else
    {
            //更新玩家位置
            if(newuser.user_account != "")
            {
                update_user_persition(newuser.user_account,0,connpool);
            }

            Json::FastWriter  writer;
            Json::Value  msg_body;

            msg_body["msg_head"]="dzpk";
            msg_body["game_number"]="dzpk";
            msg_body["area_number"]="normal";
            msg_body["room_number"]=newuser.room_num;
            msg_body["act_user_id"]=newuser.user_account;
            msg_body["act_commond"]=CMD_ENTER_ROOM;
            msg_body["error_code"]=nEnterCode;
            msg_body["result"]=-1;
            msg_body["tooltip"]="进入房间失败";

            string   str_msg =   writer.write(msg_body);
            DzpkSendP((void*)pSocketUser,CMD_ENTER_ROOM,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,pSocketUser->nUserID);
            NetCloseSocketUser(pSocketUser->nSocketID);

        //进入房间失败
    }
}

int CpsEnter(dealer *pRoom,user newuser)
{
    int nRet = -1;

    if(pRoom)
    {
        int room_number = pRoom->nVecIndex;
        newuser.room_num = pRoom->dealer_id;
        nRet = 0;

        //将新人加入到该房间的玩家队列，如果发现已经在该房间有该玩家说明是重新连接，更新socket_fd
        if(nRet == 0)
        {
            int count_sameplayer=0;
            for(unsigned int p=0;p< (dealers_vector.at(room_number))->players_list.size();p++)
            {
                if(  ((dealers_vector.at(room_number))->players_list).at(p).user_id == newuser.user_id )
                {
                    ((dealers_vector.at(room_number))->players_list).at(p).give_up_count = 0;

                    ((dealers_vector.at(room_number))->players_list).at(p).disconnect_flg  = 1;//重新激活掉线的玩家
                    count_sameplayer++;

                    break;
                }
            }
            if(count_sameplayer ==0)
            {
                //将用户加进数组
                ((dealers_vector.at(room_number))->players_list).push_back(newuser);

                //更新房间内人数,锦标赛可以不用更新，因为房间列表不显示此类房间
                //DzpkUpdateRoomPerson(pRoom->dealer_id,connpool);
            }
        }


        //根据房间号获取该房间所有玩家的信息,并返回信息给用户
        if(nRet == 0)
        {

            ENTER_ROOM_REQ_T recInfo;
            recInfo.room_number = pRoom->dealer_id;
            memcpy(recInfo.act_user_id,newuser.user_account.c_str(),strlen(newuser.user_account.c_str()));           //用户帐号

            char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
            if(pSend)
            {
                int nSendLen = SEND_PACK_MAX_LEN;
                memset(pSend,0,nSendLen);
                nRet = dzpkPutEnterRoom(pRoom,recInfo.act_user_id,newuser.nUserID,pSend,nSendLen);
                if(nRet == 0)
                {
                    DzpkSendToUser(newuser.nUserID,CMD_ENTER_ROOM,pSend,nSendLen,dealers_vector[room_number]->dealer_id);
                }

                //更新玩家session中的位置：由大厅进入房间
                update_user_persition(newuser.user_account ,pRoom->dealer_id,connpool);

                free(pSend);
                pSend = NULL;
            }
            else
            {
                nRet = -1;
            }
        }
    }
    return nRet;
}



void CpsSeatDown( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{

}

/**
*      处理坐下消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*/
int CpsSeatDownIn(dealer *pRoom,int nUserID)
{
    int nRet = -1,room_number = 0,nSeatNumber = 0;

    if(pRoom)
    {
        nRet = 0;
        room_number = pRoom->nVecIndex;

        /*用户不能重复坐下*/
        for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
        {
             if(((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID)
             {
                 if(((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0)
                 {
                     WriteRoomLog(40001,dealers_vector[room_number]->dealer_id,nUserID,"user seat down again [%d] \n",nUserID);
                     //如果已经坐下，返回成功
                     return 0;
                 }
                 break;
             }
        }

        //房间内【快速坐下】时，由服务器分配座位号，房间内满座时，不能点击【快速坐下】按钮
        if(nRet == 0)
        {
            //获得用户有效位置
            nRet = StdGetEffectiveSeatNum(pRoom,0);
            if(nRet > 0)
            {
                nSeatNumber = nRet;
                nRet = 0;
            }
            else
            {
                //找不到位置,返回错误
                nRet = -1;
            }
        }


        SEAT_DOWN_REQ_T SeatDownInfo;
        memset(&SeatDownInfo,0,sizeof(SeatDownInfo));

        // 将座位号分配给该玩家，并带入筹码数
        if(nRet == 0)
        {
            nRet = -1;
            // 将座位号分配给该玩家，并带入筹码数
             for(unsigned int i=0;i<(pRoom->players_list).size();i++)
             {
                 if((pRoom->players_list).at(i).nUserID == nUserID)
                 {
                    memcpy(SeatDownInfo.act_user_id,pRoom->players_list[i].user_account.c_str(),strlen(pRoom->players_list[i].user_account.c_str()));
                    (pRoom->players_list).at(i).seat_num = nSeatNumber;
                    (pRoom->players_list).at(i).user_staus = 1;
                    (pRoom->players_list).at(i).desk_chip = 0;
                    nRet = 0;
                    break;
                 }
             }
        }

        //向房间内的所有玩家发送消息周知
        if(nRet == 0)
        {
            char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
            int nSendLen = SEND_PACK_MAX_LEN;
            if(pSend)
            {
                memset(pSend,0,nSendLen);
                SeatDownInfo.seat_number = nSeatNumber;
                nRet = dzpkPutSeatDownPack(pRoom,&SeatDownInfo,nUserID,pSend,nSendLen);

                if(nRet == 0)
                {
                    //发送给房间内所有玩家
                     for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
                     {
                        DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_SEAT_DOWN,pSend,nSendLen,dealers_vector[room_number]->dealer_id,atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
                     }
                }
                else
                {
                    WriteLog(400000," error  lend(%d) %s  \n",nRet,pSend);
                }

                free(pSend);
                pSend = NULL;
            }
            else
            {
                nRet = -1;
            }
            //这一步保证成功
            nRet = 0;
        }

    }
    return nRet;
}


void  CpsLeaveSeat( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{

}

/**
*      处理离座消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
int CpsLeaveSeatIn(dealer *pRoom,int nUserID)
{
    int nRet = -1;
    int room_number   =0; //命令码，整型
    int seat_number  = 0; //座位号
    int force_exit   = 0; //强制离座标记
    int dealer_id = 0;
    string user_account;

    if(pRoom)
    {
        dealer_id = pRoom->dealer_id;
        room_number = pRoom->nVecIndex;

        //快速坐下的玩家，发送离座消息不带座位号，需要找到该玩家的座位号
         for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
         {
            if(atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str()) == nUserID)
            {
                user_account = ((dealers_vector.at(room_number))->players_list).at(i).user_account;
                seat_number = ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                nRet = 0;
                break;
            }
         }


        //如果是在玩牌中的玩家正好轮到他操作，自动弃牌
        if(nRet == 0)
        {
            vector<user>::iterator its;
            for( its = ((dealers_vector.at(room_number))->players_list).begin() ;its !=((dealers_vector.at(room_number))->players_list).end();its++)
            {
                if( atoi((*its).user_id.c_str()) == nUserID&&  (dealers_vector.at(room_number))->step > 0 )
                {
                    if((*its).user_staus == 2 ||(*its).user_staus == 3 ||(*its).user_staus == 4 ||(*its).user_staus == 6)
                    {
                       force_exit = 1;
                    }
                    break;
                }
            }
        }

        if(nRet == 0)
        {
           vector<user>::iterator its;
          if( force_exit == 1)
          {
              for( its = ((dealers_vector.at(room_number))->players_list).begin() ;its !=((dealers_vector.at(room_number))->players_list).end();its++)
              {
                 if( atoi((*its).user_id.c_str()) == nUserID&&  (dealers_vector.at(room_number))->step > 0 )
                 {
                     //如果该玩家是小盲注，移交小盲注指定给他的下一家
                     if( (*its).seat_num == (dealers_vector.at(room_number))->seat_small_blind )
                     {
                         (dealers_vector.at(room_number))->seat_small_blind = next_player((dealers_vector.at(room_number))->seat_small_blind,dealers_vector,room_number);
                     }

                     if( (*its).seat_num == (dealers_vector.at(room_number))->turn && (dealers_vector.at(room_number))->turn > 0 )
                     {//当前轮到正欲离座的人动作，先做弃牌操作
                            Json::Value   qp_commond;
                            qp_commond["msg_head"]="dzpk";
                            qp_commond["game_number"]="dzpk";
                            qp_commond["area_number"]="normal";
                            qp_commond["room_number"]=itoa(dealer_id,10);
                            qp_commond["act_user_id"]=user_account;
                            qp_commond["nick_name"]= (*its).nick_name;
                            qp_commond["act_commond"]=CMD_ADD_CHIP;
                            qp_commond["betMoney"]="0";
                            qp_commond["seat_number"]=itoa((*its).seat_num,10);
                            qp_commond["ctl_type"]="0";

                            SAFE_HASH_LIST_T * pList = g_dzpkTimer.GetList();
                            if(pList)
                            {
                                dztimer *pTimer = NULL;
                                for(int i=0;i<pList->nSize;i++)
                                {
                                    pTimer = (dztimer *)pList->pTable[i];
                                     if(pTimer &&  pTimer->nUserID == nUserID )
                                     {
                                        qp_commond["timer_id"]=GetString(pTimer->nTimerID);
                                        break;
                                     }
                                }
                                g_dzpkTimer.FreeList(pList);
                            }

                            deal_add_chip( nUserID,dealers_vector,qp_commond,connpool,level_list ,NULL,NULL,DEAL_ADD_CHIP_TYPE_LEAVESEAT);

                     }else {


                            dztimer timer;

                            timer.nTimerID = DzpkGetTimerID();
                            timer.nUserID =  (*its).nUserID;

                            timer.socket_fd =  (*its).socket_fd;
                            timer.seat_num =  (*its).seat_num;
                            memcpy(timer.szUserAccount,(*its).user_account.c_str(),strlen((*its).user_account.c_str()));
                            timer.room_num = room_number;
                            timer.start_time = time(NULL);
                            timer.timer_type = 1;
                            timer.time_out = 17;
                            g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
                            WriteRoomLog(500,dealers_vector[room_number]->dealer_id ,timer.nUserID,"leave seat set timer (%d)  \n",timer.nTimerID);

                           //弃牌操作
                            Json::Value   qp_commond;
                            qp_commond["msg_head"]="dzpk";
                            qp_commond["game_number"]="dzpk";
                            qp_commond["area_number"]="normal";
                            qp_commond["room_number"]=itoa(dealer_id,10);
                            qp_commond["act_user_id"]=user_account;
                            qp_commond["nick_name"]= (*its).nick_name;
                            qp_commond["act_commond"]=CMD_ADD_CHIP;
                            qp_commond["betMoney"]="0";
                            qp_commond["seat_number"]=itoa((*its).seat_num,10);
                            qp_commond["ctl_type"]="0";

                            SAFE_HASH_LIST_T * pList = g_dzpkTimer.GetList();
                            if(pList)
                            {
                                dztimer *pTimer = NULL;
                                for(int i=0;i<pList->nSize;i++)
                                {
                                    pTimer = (dztimer *)pList->pTable[i];
                                    if( pTimer &&  pTimer->nUserID == nUserID){
                                        qp_commond["timer_id"]=GetString(pTimer->nTimerID);
                                    }
                                }
                                g_dzpkTimer.FreeList(pList);
                            }

                            /*异常棋牌   0：正常弃牌流程 1：异常弃牌流程*/
                            deal_add_chip( nUserID,dealers_vector,qp_commond,connpool,level_list ,NULL,NULL,DEAL_ADD_CHIP_TYPE_FORCELEAVE);

                     }
                     //弃牌完了之后计算筹码并通知大厅服务器扣除
                    //(*its).chip_account+=((*its).hand_chips - (*its).last_chips);
                    //update_user_chip_account((*its).user_account,(*its).chip_account,connpool);//更新玩家筹码账户

                    break;
                 }
             }
          }
          else
          {
               for( its = ((dealers_vector.at(room_number))->players_list).begin() ;its !=((dealers_vector.at(room_number))->players_list).end();its++)
               {
                 if( (*its).nUserID == nUserID  &&  (dealers_vector.at(room_number))->step > 0 )
                 {
                     //先找到该玩家
                     //如果该玩家是小盲注，移交小盲注指定给他的下一家
                     if( (*its).seat_num == (dealers_vector.at(room_number))->seat_small_blind )
                     {
                         (dealers_vector.at(room_number))->seat_small_blind = next_player((dealers_vector.at(room_number))->seat_small_blind,dealers_vector,room_number);
                     }
                     break;
                 }
               }
          }
        }

        //修改该玩家的属性，状态改为 旁观
        if(nRet == 0)
        {
             for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
             {
                 if(((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID )
                 {
                    (dealers_vector.at(room_number))->leave_players_list.push_back( ((dealers_vector.at(room_number))->players_list).at(i) ); //离开或站起的玩家 放入另一个队列保存
                    ((dealers_vector.at(room_number))->players_list).at(i).seat_num = 0;
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 0;
                    ((dealers_vector.at(room_number))->players_list).at(i).desk_chip = 0;
                    ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).win_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).first_turn_blind =0;
                    ((dealers_vector.at(room_number))->players_list).at(i).auto_buy = 0;
                    ((dealers_vector.at(room_number))->players_list).at(i).max_buy = 0;
                    ((dealers_vector.at(room_number))->players_list).at(i).give_up_count = 0;

                 }
             }
        }


        //向房间内的所有玩家发送消息周知
        if(nRet == 0)
        {
            Json::FastWriter  writer;
            Json::Value  msg_body;
            Json::Value  player_property;

            msg_body["msg_head"]="dzpk";
            msg_body["game_number"]="dzpk";
            msg_body["area_number"]="normal";
            msg_body["room_number"]=dealer_id;
            msg_body["act_user_id"]=user_account;
            msg_body["act_commond"]=CMD_LEAVE_SEAT;
            for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
            {
                if( ((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID )
                {
                    msg_body["chip_account"]=itoa((((dealers_vector.at(room_number))->players_list).at(i)).chip_account,10);
                }
            }

            msg_body["seat_number"]=seat_number;
            msg_body["force_exit"]=force_exit;

             //向房间内的所有玩家发送消息周知
            for(unsigned int i=0; i< ((dealers_vector.at(room_number))->players_list).size();i++)
            {
                if( ((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID )
                {
                    player_property["user_id"]=((dealers_vector.at(room_number))->players_list).at(i).user_id;
                    player_property["user_account"]=((dealers_vector.at(room_number))->players_list).at(i).user_account;
                    player_property["nick_name"]=((dealers_vector.at(room_number))->players_list).at(i).nick_name;
                    player_property["dzpk_level"]=((dealers_vector.at(room_number))->players_list).at(i).dzpk_level;
                    player_property["dzpk_experience"]=itoa(((dealers_vector.at(room_number))->players_list).at(i).dzpk_experience,10);
                    player_property["vip_level"]=((dealers_vector.at(room_number))->players_list).at(i).vip_level;
                    player_property["chip_account"]=itoa(((dealers_vector.at(room_number))->players_list).at(i).chip_account,10);
                    player_property["rank_name"]=((dealers_vector.at(room_number))->players_list).at(i).rank_name;
                    player_property["head_photo_serial"]=((dealers_vector.at(room_number))->players_list).at(i).head_photo_serial;
                    player_property["desk_chip"]=((dealers_vector.at(room_number))->players_list).at(i).desk_chip;
                    player_property["user_staus"]=((dealers_vector.at(room_number))->players_list).at(i).user_staus;
                    player_property["room_num"]=((dealers_vector.at(room_number))->players_list).at(i).room_num;
                    player_property["seat_num"]=((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                    player_property["hand_chips"]=((dealers_vector.at(room_number))->players_list).at(i).hand_chips;
                    player_property["giftid"]=((dealers_vector.at(room_number))->players_list).at(i).giftid;
                    player_property["mall_path"]=((dealers_vector.at(room_number))->players_list).at(i).mall_path;
                    player_property["level_title"]=((dealers_vector.at(room_number))->players_list).at(i).level_title;
                    break;
                }
            }

            msg_body["player_property"]=player_property;
            string   str_msg =   writer.write(msg_body);

            for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
            {
                DzpkSend( (((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_LEAVE_SEAT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
            }

        }
    }
    return nRet;
}

/**
*      处理展示完毕消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  CpsDisplayOver( int nUserID, vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{

     int room_number   =atoi((msg_commond["room_number"].asString()).c_str()); //房间号
     string user_account  =  (msg_commond["act_user_id"].asString()).c_str(); //用户ID
     int seat_number = atoi((msg_commond["seat_number"].asString()).c_str()); //座位号
     string timer_id = (msg_commond["timer_id"].asString()).c_str(); //timer ID
     int nCurTimerID = GetInt(timer_id);

     int dealer_id = 0;
	 dealer *pRoom = DzpkGetRoomInfo(room_number);
	if(pRoom)
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
	WriteRoomLog(600,dealers_vector[room_number]->dealer_id,nUserID," **************** stop play :ring:%d **************\n",dealers_vector[room_number]->nRing);

	/*帮助客户端找回丢失的timer_id*/
	if( timer_id == "")
	{
		dztimer *pTimer = NULL;
		SAFE_HASH_LIST_T * pList = g_dzpkTimer.GetList();
		if(pList)
		{
			for(int i=0;i<pList->nSize;i++)
			{
				pTimer = (dztimer *)pList->pTable[i];
				if(pTimer && pTimer->nUserID == nUserID && pTimer->timer_type == 2 && pTimer->room_num == room_number)
				{
					timer_id = GetString(pTimer->nTimerID);
					WriteRoomLog(50005,dealers_vector[room_number]->dealer_id,pTimer->nUserID,"lost timer ID:[%d]\n",pTimer->nTimerID);
					break;
				}
			}
			g_dzpkTimer.FreeList(pList);
		}
	}

    //删除预设的定时器，每次只删除一个同类型的定时器
    dztimer *pTimer = (dztimer *)g_dzpkTimer.GetVal(nCurTimerID);
    if(pTimer)
    {
        if( pTimer->nTimerID == nCurTimerID && pTimer->nUserID == nUserID &&  pTimer->timer_type == 2 && pTimer->room_num == room_number  )
        {
            WriteRoomLog(600,dealers_vector[room_number]->dealer_id,pTimer->nUserID,"display over kill timer (%d)  \n",pTimer->nTimerID);
            g_dzpkTimer.Remove(nCurTimerID);

             //一局结束，重新更新玩家的状态为坐下
             for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
             {
                if (((dealers_vector.at(room_number))->players_list).at(i).nUserID == nUserID
                    && ((dealers_vector.at(room_number))->players_list).at(i).seat_num > 0 &&
                    ((dealers_vector.at(room_number))->players_list).at(i).seat_num < 10)
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus = 1;
                    break;
                }
             }

              //当前在座用户列表属性
             for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
             {
                if( ((dealers_vector.at(room_number))->players_list).at(i).seat_num == 0 )
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).user_staus=0;//桌面筹码清零
                    ((dealers_vector.at(room_number))->players_list).at(i).desk_chip=0;//桌面筹码清零
                    ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).win_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).first_turn_blind =0;
                }
              }

             //检查当前参与玩牌的玩家数量
             int player_num=0;
             for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
             {
                if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus >=2 )
                {
                    //任然有在玩牌中的玩家
                    player_num++;
                }
             }


             if( player_num == 0)
             {

                //开局前检查
                int nRet = 0;

                //全部都已经结束牌局，清除上局的所有数据
                if(nRet == 0)
                {
                    //清空上一局所有消息
                    nRet = StdClearRoomInfo(pRoom);
                }


                CPS_RECORD_T * pRecord =  CpsGetRecord(pRoom->nCpsRecordID);

                if(pRecord)
                {
                    //更新玩家筹码(要在CpsCheckUserstatus 之前)
                    for(int i=0;i<(int)pRoom->players_list.size();i++)
                    {
                        if(pRoom->players_list[i].seat_num > 0)
                        {
                            CpsUpdateRecordUserChip(pRoom->players_list[i].nUserID,pRoom->players_list[i].hand_chips);
                        }
                        else
                        {
                            pRoom->players_list[i].hand_chips = 0;
                        }
                    }
                    //涨盲注
                    pRoom->small_blind = pRecord->nCurSmallBlind;

                    if(pRecord->nArryRoomIndex > 1)
                    {
                        CpsCheckUserstatus(pRoom,1,time(NULL));
                    }

                    Lock.Unlock();


                    class CAutoLock LockRecord(pRecord->pMutex);
                    LockRecord.Lock();
                    int nContinue = 1;
                    if(pRecord->nArryRoomIndex <= 4)
                    {
                        //检查是否人数少于终极桌
                        int nReturn = CpsNoLockCheckFinalTable(pRecord,pRoom->dealer_id);
                        if(nReturn == -1)
                        {
                            nContinue = 1;
                        }
                        else
                        {
                            nContinue = 0;
                        }
                    }

                    //开始新一局，新一局开始前会判断人数是否够，如果不够会调用拼桌
                    if(nContinue  == 1)
                    {
                        CpsNoLockTableStart(pRecord,pRoom->dealer_id);
                    }

                    LockRecord.Unlock();

                    //发送排名给用户
                    CpsSendRankToUser(pRecord->nRecordID);

                    CpsFreeRecord(pRecord);
                }
                else
                {

                }
            }
        }
    	g_dzpkTimer.FreeVal((void*)pTimer);
    }
}

/*加注*/
void CpsAddChip(int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead,int nType)
{
    if(DEAL_ADD_CHIP_TYPE_USER  == nType)
    {
        //更新用户最后活动时间
        int nGameLeaveMark = CpsGetUserLeaveMark(nUserID);
        if(nGameLeaveMark  == 0)
        {
            CpsUpdateRecordUserActive(nUserID);
        }
    }
    deal_add_chip(nUserID,dealers_vector,msg_commond,connpool,level_list,pSocketUser,pHead,nType);
}


/**
*      处理离开房间消息
*      players_vector 所有玩家信息的队列
*      msg_commond    接收到的消息
*
*/
void  CpsExitRoom( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{
     int room_number   =atoi((msg_commond["room_number"].asString()).c_str()); //房间号
    dealer *pRoom = DzpkGetRoomInfo(room_number);
    if(pRoom)
    {
        class CAutoLock Lock(&pRoom->Mutex);
        Lock.Lock();

        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            if(pRoom->players_list[i].nUserID == nUserID)
            {
                CpsUserFail(&pRoom->players_list[i],pRoom,pRoom->nCpsRecordID,0);
                break;
            }
        }

        Lock.Unlock();
    }

    CpsRemoveUser(nUserID);
    deal_exit_room(nUserID,dealers_vector,msg_commond,connpool,level_list,pSocketUser,pHead);
}

/*获得比赛列表*/
void CpsGetRankList( int nUserID,vector<dealer *>&  dealers_vector,Json::Value msg_commond,ConnPool *connpool,vector<level_system>  level_list,DZPK_USER_T *pSocketUser,PACKET_HEAD_T *pHead)
{
    int nCpsRecordID  = atoi((msg_commond["cps_record_id"].asString()).c_str());


    if(nCpsRecordID > 0)
    {
        CpsSendRankToUser(nCpsRecordID,nUserID);
    }
}

/*开始游戏*/
int CpsStart(CPS_RECORD_T * pRecord)
{
    int nRet = 0;

    if(pRecord)
    {

	    class CAutoLock Lock(pRecord->pMutex);
		Lock.Lock();

        //查看是否满足比赛条件
        int nContinue = 1;

        if(pRecord->nType == ROOM_TYPE_CPS + 1)
        {
            pRecord->nRecordUserCount = CpsUserGetOnline(pRecord->nType,pRecord->nRecordID);
            if (pRecord->nRecordUserCount <=1)
            {
                return nRet;
            }
            if(pRecord->nRecordUserCount < CPS_TYPE_SIX_MIN_COUNT)
            {
                nContinue = 0;

                CpsNoLockEndBack(pRecord,NULL,1);
            }
        }

        if(nContinue == 1)
        {
            pRecord->nStart = 1;
            dzpkUpdateDbCpsStatus(pRecord->nRecordID,pRecord->nStart);
            pRecord->nRecordUserCount = CpsUserGetOnline(pRecord->nType,pRecord->nRecordID);
            if (pRecord->nRecordUserCount < pRecord->nStartNum)
            {
                CpsForceStop(pRecord,pRecord->nType);
                pRecord->nStart = 2;
                dzpkUpdateDbCpsStatus(pRecord->nRecordID,pRecord->nStart);
                CpsRemoveRecord(pRecord->nRecordID);
                return nRet;
            }
            int nSuccess = CpsNoLockCheckFinalTable(pRecord,-1);
            if(nSuccess == -1)
            {
                CpsNoLockStartAllTables(pRecord);
            }
        }
    }

    return nRet;
}
int CpsNoLockEndBack(CPS_RECORD_T * pRecord,dealer *pRoom,int nType)
{
    int nRet = 0;

    if(pRecord)
    {
        if(nType == 0)
        {
            if(pRoom)
            {
                //算出最后谁赢
                user *arryTemp = new user[CPS_MAX_ROOM_USER];
                int nCurIndex = 0;
                class CAutoLock Lock(&pRoom->Mutex);
                Lock.Lock();

                for(int i=0;i<(int)pRoom->players_list.size();i++)
                {
                    if(pRoom->players_list[i].seat_num > 0)
                    {
                        if(nCurIndex < CPS_MAX_ROOM_USER - 1)
                        {
                            arryTemp[nCurIndex] = pRoom->players_list[i];
                            nCurIndex++;
                        }
                    }
                }
                Lock.Unlock();

                int nWinUserID = 0;
                int nMax = 0;
                for(int i=0;i<nCurIndex;i++)
                {
                    if(arryTemp[i].hand_chips >= nMax)
                    {
                        nMax = arryTemp[i].hand_chips;
                        nWinUserID = arryTemp[i].nUserID;
                    }
                }
                CpsUserSuccess(pRecord,pRoom,nWinUserID);

                if(arryTemp)
                {
                    delete []arryTemp;
                    arryTemp = NULL;
                }
            }
        }
        else
        {
            //用户太少，不能开始
            CpsForceStop(pRecord,pRecord->nType);
        }
        pRecord->nStart = 2;
        dzpkUpdateDbCpsStatus(pRecord->nRecordID,pRecord->nStart);
        CpsRemoveRecord(pRecord->nRecordID);
    }

#ifdef CPS_TEXT

        CPS_RECORD_T _Record;
        _Record.nRecordID = 100;
        _Record.nType = 5;
        _Record.nStartTime = time(NULL) + 30;
        _Record.nStart = 0;
        _Record.nArryRoomIndex= 0;
        _Record.nCurSmallBlind = 50;
        _Record.nInitHandChip= 1000;
        _Record.pMutex = new CMutex();
        _Record.nLastAddBlindTime = _Record.nStartTime;
        _Record.nAddBlind = CPS_ADD_ONE_BLIND;
        _Record.nAddBlindTime = CPS_ADD_BLIND_TIME;
        _Record.nRecordUserCount = 0;

        CpsInsertRecord(_Record.nRecordID,&_Record,sizeof(_Record));

        SAFE_HASH_LIST_T *pList = CpsRecordGetList();
        if(pList)
        {
            if(pList->nSize > 0)
            {
                CPS_RECORD_T *p = (CPS_RECORD_T *)pList->pTable[0];
            }
            CpsRecordFreeList(pList);
        }

#endif
    return nRet;
}
/*锦标赛*/
int CpsUserGiveUp(int nType,user *pUser)
{
    if(pUser)
    {
        int nGameLeaveMark = CpsGetUserLeaveMark(pUser->nUserID);
        if(nType == DEAL_ADD_CHIP_TYPE_USER && nGameLeaveMark != 0)
        {
        }
        else
        {
            pUser->give_up_count = pUser->give_up_count-1;
        }

        if(pUser->give_up_count == 2)
        {
        }
    }
    return 0;
}

