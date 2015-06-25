/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2015.5.2
** 版本：1.0
** 说明：德州扑克标准流程活动
**************************************************************
*/
#include "dzpkActivity.h"
#include "dzpkCustom.h"
#include "../Start.h"
#include "dzpkParsePacket.h"
#include "../../../com/common.h"
#include "../db/dzpkDb.h"
#include "dzpkTask.h"
#include "../mainDzpk.h"

#include "../db/MemData.h"


#define DZPK_ACTIVITY_CHAR_LEN       100            //奖励提示

/********************手牌奖励 *********************/
typedef struct tagLuckCardHand
{
    tagLuckCardHand()
    {
        memset(this,0,sizeof(tagLuckCardHand));
    }
    int nHandCard1;                                 //手牌
    int nHandCard2;
    int nSmallBlind;                                //小盲注
    int nAwardHandChip;                             //奖励筹码
    int nAwardMultipleChip;                         //奖励小盲倍数
    char szName[DZPK_ACTIVITY_CHAR_LEN];            //牌名
    int nType;                                      //奖励类型
    tagLuckCardHand *pNext;
} LUCK_CARD_HAND_T;

typedef struct tagLuckCardHandHead
{
    tagLuckCardHandHead()
    {
        memset(this,0,sizeof(tagLuckCardHandHead));
    }
    LUCK_CARD_HAND_T *pFirst;
    LUCK_CARD_HAND_T *pLast;
} LUCK_CARD_HAND_HEAD_T;

/********************牌型奖励 *********************/
typedef struct tagLuckCardType
{
    tagLuckCardType()
    {
        memset(this,0,sizeof(tagLuckCardType));
    }
    int nCardTypeStart;                              //手牌
    int nCardTypeEnd;
    int nSmallBlind;                                //小盲注
    int nAwardHandChip;                             //奖励筹码
    int nAwardMultipleChip;                         //奖励小盲倍数
    char szName[DZPK_ACTIVITY_CHAR_LEN];            //牌名
    int nType;                                      //奖励类型
    tagLuckCardType *pNext;
} LUCK_CARD_TYPE_T;

typedef struct tagLuckCardTypeHead
{
    tagLuckCardTypeHead()
    {
        memset(this,0,sizeof(tagLuckCardTypeHead));
    }
    LUCK_CARD_TYPE_T *pFirst;
    LUCK_CARD_TYPE_T *pLast;
} LUCK_CARD_TYPE_HEAD_T;

/******************************************************/

static LUCK_CARD_HAND_HEAD_T g_LuckCardHandHead;      //幸运手牌
static LUCK_CARD_TYPE_HEAD_T g_LuckCardTypeHead;      //幸运牌型

/*读取系统活动*/
int StdReadSystemActivity(int nType)
{
    //释放之前
    SYSTEM_ACTIVITY_T *p = g_SystemActivity.pFirst;
    SYSTEM_ACTIVITY_T *pFree = p;

    while(p != NULL)
    {
        p = p->pNext;
        delete pFree;
        pFree = p;
    }

    g_SystemActivity.pFirst = NULL;
    g_SystemActivity.pLast = NULL;

    dzpkReadActivityInfo(&g_SystemActivity);

    if(nType == 0)
    {
        StdReadLuckCardAward();
    }

    return 0;
}

/*读取幸运手牌奖励*/
int StdReadLuckCardAward()
{
    //释放之前
    LUCK_CARD_HAND_T *p = g_LuckCardHandHead.pFirst;
    LUCK_CARD_HAND_T *pFree =p;
    while(p != NULL)
    {
        p = p->pNext;
        delete pFree;
        pFree = p;
    }
    g_LuckCardHandHead.pFirst = NULL;
    g_LuckCardHandHead.pLast = NULL;

    LUCK_CARD_TYPE_T *p1 = g_LuckCardTypeHead.pFirst;
    LUCK_CARD_TYPE_T *pFree1 =p1;
    while(p1 != NULL)
    {
        p1 = p1->pNext;
        delete pFree1;
        pFree1 = p1;
    }
    g_LuckCardTypeHead.pFirst = NULL;
    g_LuckCardTypeHead.pLast = NULL;

    //手牌奖励
    char szName[100];
    sprintf(szName,"%s","QQ手牌");
    StdSetHandCardAward(12,12,1000,600,1,szName,1);         //QQ
    sprintf(szName,"%s","KK手牌");
    StdSetHandCardAward(13,13,1000,1200,2,szName,2);         //KK
    sprintf(szName,"%s","AA手牌");
    StdSetHandCardAward(14,14,1000,2000,3,szName,3);        //AA

    //牌型奖励
    sprintf(szName,"%s","四条牌型");
    StdSetHandTypeAward(802,814,1000,4000,5,szName,1001);         //四条
    sprintf(szName,"%s","同花顺牌型");
    StdSetHandTypeAward(906,913,1000,8000,10,szName,1002);         //同花顺
    sprintf(szName,"%s","皇家同花顺牌型");
    StdSetHandTypeAward(914,914,1000,20000,20,szName,1003);        //皇家同花顺

#ifdef _DEBUG
    sprintf(szName,"%s","三条牌型");
    StdSetHandTypeAward(402,414,500,300,1,szName,1004);         //三条
    sprintf(szName,"%s","两对牌型");
    StdSetHandTypeAward(300,399,500,300,1,szName,1005);         //两队
    sprintf(szName,"%s","一对牌型");
    StdSetHandTypeAward(200,299,500,300,1,szName,1006);         //一对
    sprintf(szName,"%s","高牌牌型");
    //StdSetHandTypeAward(100,199,500,300,1,szName);         //高牌
#endif

    return 0;
}

/*开始手牌奖励*/
int StdSetHandCardAward(int nHandCard1,int nHandCard2,int nSmallBlind,int nHandChip,int nMultiple,char *pName,int nType)
{
    LUCK_CARD_HAND_T *p = new LUCK_CARD_HAND_T();
    if(p)
    {
        p->nHandCard1 = nHandCard1;
        p->nHandCard2 = nHandCard2;
        p->nSmallBlind = nSmallBlind;
        p->nAwardHandChip = nHandChip;
        p->nAwardMultipleChip = nMultiple;
        p->nType = nType;
        if(pName)
        {
            sprintf(p->szName,"%s",pName);
        }

        if(g_LuckCardHandHead.pFirst == NULL)
        {
            g_LuckCardHandHead.pFirst = p;
            g_LuckCardHandHead.pLast = p;
        }
        else
        {
            if(g_LuckCardHandHead.pLast)
            {
                g_LuckCardHandHead.pLast->pNext = p;
                g_LuckCardHandHead.pLast = p;
            }
        }
    }
    return 0;
}

/*牌型奖励*/
int StdSetHandTypeAward(int nCardTypeStart,int nCardTypeEnd,int nSmallBlind,int nHandChip,int nMultiple,char *pName,int nType)
{
    LUCK_CARD_TYPE_T *p = new LUCK_CARD_TYPE_T();
    if(p)
    {
        p->nCardTypeStart = nCardTypeStart;
        p->nCardTypeEnd = nCardTypeEnd;
        p->nSmallBlind = nSmallBlind;
        p->nAwardHandChip = nHandChip;
        p->nAwardMultipleChip = nMultiple;
        p->nType = nType;
        if(pName)
        {
            sprintf(p->szName,"%s",pName);
        }

        if(g_LuckCardTypeHead.pFirst == NULL)
        {
            g_LuckCardTypeHead.pFirst = p;
            g_LuckCardTypeHead.pLast = p;
        }
        else
        {
            if(g_LuckCardTypeHead.pLast)
            {
                g_LuckCardTypeHead.pLast->pNext = p;
                g_LuckCardTypeHead.pLast = p;
            }
        }
    }
    return 0;
}

/*是否抽奖活动正在进行*/
int StdActivityAwardRunning(int nActivityID,/*SYSTEM_ACTIVITY_T*/ void *pInfo)
{
    int nRet = -1;

    SYSTEM_ACTIVITY_T *p = g_SystemActivity.pFirst;
    while(p != NULL)
    {
        if(p->nID == nActivityID)
        {
            int nCurTime = StdGetTime();
            if(nCurTime >= p->nStartTime && nCurTime <= p->nEndTime)
            {
                if(nActivityID == SYSTEM_LUCKY_CARD_AWARD)
                {
                    //需要判断一天某个时段
                    int nCurSecond = nCurTime%(60*60*24);
                    if(nCurSecond >= atoi(p->szTeam1)*60*60 && nCurSecond <= atoi(p->szTeam2)*60*60)
                    {
                        WriteLog(USER_ACTIVITY_AWARD_LEVEL,"StdActivityAwardRunning  success. nCurTime[%d] startTime[%d] endtime[%d] \n",nCurTime,p->nStartTime,p->nEndTime);
                        nRet = 0;
                    }
                    else
                    {
                        WriteLog(USER_ACTIVITY_CARD_LEVEL,"luck card time error %s <> %s \n",p->szTeam1,p->szTeam2);
                    }
                }
                else
                {
                    WriteLog(USER_ACTIVITY_AWARD_LEVEL,"StdActivityAwardRunning  success. nCurTime[%d] startTime[%d] endtime[%d] \n",nCurTime,p->nStartTime,p->nEndTime);
                    nRet = 0;
                }
            }
            else
            {
                WriteLog(USER_ACTIVITY_AWARD_LEVEL,"StdActivityAwardRunning  fail. nCurTime[%d] startTime[%d] endtime[%d] \n",nCurTime,p->nStartTime,p->nEndTime);
            }
            break;
        }
        p = p->pNext;
    }

    return nRet;
}
/*获得下一次奖励次数*/
int StdGetNextPlayTimes(int nCurTimes)
{
    int nRet = -1;

    if(nCurTimes >= 0)
    {
        if(nCurTimes < 5)
        {
            nRet = 5-nCurTimes;
        }
        else if(nCurTimes < 10)
        {
            nRet = 10-nCurTimes;
        }
        else if(nCurTimes < 20)
        {
            nRet = 20-nCurTimes;
        }
        else if(nCurTimes < 30)
        {
            nRet = 30-nCurTimes;
        }
        else if(nCurTimes < 50)
        {
            nRet = 50-nCurTimes;
        }
    }
    return nRet;
}

int getJinDouNum(int playTimes, int smallBind)
{
    int awardId = 0;
    switch(smallBind)
    {
    case 100:
        awardId = 18;
        break;
    case 200:
        awardId = 19;
        break;
    case 500:
        awardId = 20;
        break;
    case 1000:
        awardId = 21;
        break;
    case 2000:
        awardId = 22;
        break;
    case 5000:
        awardId = 23;
        break;
    case 10000:
        awardId = 24;
        break;
    case 20000:
        awardId = 25;
        break;
    default:
        break;
    }

    int award = 0;
    int num = 0;
    if (!queryAwardJinDou(awardId, award, num))
    {
        award = 0;
    }
    else
    {
        if (playTimes % num != 0)
        {
            award = 0;
        }
    }

    return award;
}


/*看用户是否玩了一盘*/
int StdTodayPlayTimes(dealer *pRoom)
{
    if(pRoom)
    {
        if(DzpkIfCpsOfType(pRoom->room_type) == 0)
        {
        }
        else
        {
            //任务玩一局
            dzpkCheckTaskPlayGame(pRoom);

            for(unsigned int i=0; i<(pRoom->players_list).size(); i++)
            {
                if(pRoom->players_list[i].seat_num > 0 && pRoom->players_list[i].user_staus >= 2)
                {
                    g_pMemData->updateGameNum(pRoom->dealer_id, pRoom->players_list[i].nUserID,
                        pRoom->small_blind, pRoom->current_player_num);

                    //活动是否正在进行
                    if(StdActivityAwardRunning(SYSTEM_ACTIVITY_AWARD) == 0) //9为宝箱房，10为金豆房
                    {
                        pRoom->players_list[i].nTodayPlayTimes++;

                        Json::Value  msg_body;
                        Json::FastWriter  writer;
                        msg_body["msg_head"]="dzpk";
                        msg_body["game_number"]="dzpk";
                        msg_body["area_number"]="normal";
                        msg_body["act_commond"]=CMD_TODAY_PLAY_TIMES;
                        msg_body["today_play_times"]=pRoom->players_list[i].nTodayPlayTimes;
                        msg_body["user_account"]=pRoom->players_list[i].user_account;

                        int nCountTimes = -1;
                        nCountTimes = StdGetNextPlayTimes(pRoom->players_list[i].nTodayPlayTimes);
                        msg_body["count_times"]=nCountTimes;

#if 0
                        if (pRoom->room_type == 10)
                        {
                            int jinDouNum = getJinDouNum(pRoom->players_list[i].nTodayPlayTimes, pRoom->small_blind);
                            if (jinDouNum > 0)
                            {
                                addUserJinDou(pRoom->players_list[i].nUserID, jinDouNum, pRoom->dealer_id, pRoom->small_blind, pRoom->players_list[i].nTodayPlayTimes);
                            }
                        }

                        if (pRoom->room_type == 9 && pRoom->small_blind >= 100)
                        {
                            int chests = 0;
                            if (pRoom->players_list[i].nTodayPlayTimes % 10 == 0)
                            {
                                chests = 1;
                                addGetChestsLog(pRoom->players_list[i].nUserID, pRoom->dealer_id, pRoom->small_blind, pRoom->players_list[i].nTodayPlayTimes);
                            }
                            msg_body["chests"] = chests;  //0: 没有宝箱；1: 有宝箱

                            printf("可以抽取宝箱: %d; 玩牌次数: %d\n", chests, pRoom->players_list[i].nTodayPlayTimes);
                        }
#endif
                        //////////////////////////////////////////////

                        string   str_msg =   writer.write(msg_body);

                        DzpkSend( ((pRoom->players_list).at(i)).socket_fd,CMD_TODAY_PLAY_TIMES,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi((pRoom->players_list).at(i).user_id.c_str()));

                        if(pRoom->players_list[i].nTodayPlayTimes == 5 ||pRoom->players_list[i].nTodayPlayTimes == 10 ||pRoom->players_list[i].nTodayPlayTimes == 20 ||pRoom->players_list[i].nTodayPlayTimes == 30||pRoom->players_list[i].nTodayPlayTimes == 50)
                        {
                            //奖励
                            int nCurAwardCard = dzpkDbAddAwardChangeCard(pRoom->players_list[i].nUserID,1);
                            if(nCurAwardCard > 0)
                            {
                                Json::Value  msg_body;
                                Json::FastWriter  writer;
                                msg_body["msg_head"]="dzpk";
                                msg_body["game_number"]="dzpk";
                                msg_body["area_number"]="normal";
                                msg_body["act_commond"]=CMD_AWARD_CHANGE_ONE;
                                msg_body["user_account"]=pRoom->players_list[i].user_account;
                                msg_body["award_change_cards"]=nCurAwardCard;
                                msg_body["award_change_add"]=1;
                                string   str_msg =   writer.write(msg_body);

                                DzpkSend( ((pRoom->players_list).at(i)).socket_fd,CMD_AWARD_CHANGE_ONE,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi((pRoom->players_list).at(i).user_id.c_str()));
                            }
                            else
                            {
                                WriteLog(4008,"today play award error. play times[ %d] db award[%d] \n",pRoom->players_list[i].nTodayPlayTimes,nCurAwardCard );
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
/*手牌获得奖励*/
int StdGetHandCardAward(int nHandCard1,int nHandCard2,/*LUCK_CARD_HAND_T*/void *pArg)
{
    int nRet = -1;
    int nCardTemp1 = nHandCard1%100;
    int nCardTemp2 = nHandCard2%100;

    WriteLog(USER_ACTIVITY_CARD_LEVEL,"enter  StdGetHandCardAward  handCard %d <> %d \n",nHandCard1,nHandCard2);

    LUCK_CARD_HAND_T *p = g_LuckCardHandHead.pFirst;
    while(p != NULL)
    {
        if( (p->nHandCard1 == nCardTemp1 && p->nHandCard2 == nCardTemp2) || (p->nHandCard2 == nCardTemp1 && p->nHandCard1 == nCardTemp2))
        {
            if(pArg)
            {
                memcpy(pArg,p,sizeof(LUCK_CARD_HAND_T));
                nRet = 0;
            }
            break;
        }
        p = p->pNext;
    }

#ifdef _DEBUG
    if(nRet != 0)
    {
        LUCK_CARD_HAND_T *p = g_LuckCardHandHead.pFirst;
        while(p != NULL)
        {
            if( p->nHandCard1 == nCardTemp1 || p->nHandCard2 == nCardTemp2)
            {
                if(pArg)
                {
                    memcpy(pArg,p,sizeof(LUCK_CARD_HAND_T));
                    nRet = 0;
                }
                break;
            }
            p = p->pNext;
        }
    }
#endif
    return nRet;
}

/*牌型获得奖励*/
int StdGetCardTypeAward(int nCardType,/*LUCK_CARD_TYPE_T*/void *pArg)
{
    int nRet = -1;
    WriteLog(USER_ACTIVITY_CARD_LEVEL,"enter  StdGetCardTypeAward nCardType %d\n",nCardType);

    LUCK_CARD_TYPE_T *p = g_LuckCardTypeHead.pFirst;
    while(p != NULL)
    {
        if(nCardType >= p->nCardTypeStart && nCardType <= p->nCardTypeEnd)
        {
            if(pArg)
            {
                memcpy(pArg,p,sizeof(LUCK_CARD_TYPE_T));
                nRet = 0;
            }
            break;
        }
        p = p->pNext;
    }

    return nRet;
}
/*发送奖励*/
int StdSendLuckCardPack(dealer *pRoom,user *pInfo,int nSmallBlind,int nHandChip,int nMultiple,char *pName,int visible,int nAwardType)
{
    if(pRoom && pInfo && pName)
    {
        char szTooltip[200];
        memset(szTooltip,0,200);
        int nAwardChip = nHandChip;
        int nPreChipCount = pInfo->chip_account;
        if(pRoom->small_blind >= nSmallBlind)
        {
            nAwardChip = pRoom->small_blind*nMultiple;
        }
        sprintf(szTooltip,"恭喜 %s 在小盲 %d 场抓到%s,奖励筹码%d。",(char *)pInfo->nick_name.c_str(),pRoom->small_blind,
                pName,nAwardChip);

        pInfo->chip_account += nAwardChip;
        dzpkDbUpdateUserChip(pInfo->nUserID,nAwardChip,pInfo->chip_account,DZPK_USER_CHIP_CHANGE_LUCK_HAND_CARD,pInfo);

        if(visible == 0)
        {
            //放进数组，等结束再发送
            LUCK_CARD_PARAM_T _LuckParam;
            memcpy(_LuckParam.szUserAccount,pInfo->user_account.c_str(),strlen(pInfo->user_account.c_str()));
            memcpy(_LuckParam.szTooltip,szTooltip,strlen(szTooltip));
            _LuckParam.nAwardChip = nAwardChip;
            _LuckParam.nUserID = pInfo->nUserID;
            memcpy(_LuckParam.szHeadPhoto,pInfo->head_photo_serial.c_str(),strlen(pInfo->head_photo_serial.c_str()));

            pRoom->arryLuckCard.push_back(_LuckParam);
        }

        stdSendLuckCardParam(pRoom,pInfo->nUserID,(char *)pInfo->user_account.c_str(),nAwardChip,szTooltip,visible,1,(char *)pInfo->head_photo_serial.c_str());

        //写日志
        char szHandTooltip[100];
        sprintf(szHandTooltip,"小盲 %d 场抓到%s,奖励筹码%d。",pRoom->small_blind,
                pName,nAwardChip);
        dzpkDbWriteLuckCardLog(nAwardType,pRoom->dealer_id,pInfo->nUserID,nAwardChip,szHandTooltip,nPreChipCount);
    }
    return 0;
}
/*发送包*/
int stdSendLuckCardParam(dealer *pRoom,int nUserID,char *pUserAccount,int nAwardChip,char *pTooltip,int nVisible,int nAddChip,char *pHeadPhoto)
{
    if(pRoom && pUserAccount && pTooltip)
    {
        Json::Value  msg_body;
        Json::FastWriter  writer;
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["act_commond"]=CMD_LUCK_CARD_AWARD;
        msg_body["user_account"]=pUserAccount;                  //用户帐号
        msg_body["award_chip"]=nAwardChip;                      //奖励多少筹码
        msg_body["room_number"]=pRoom->dealer_id;               //所在房间
        msg_body["tooltip"]=pTooltip;                           //提示
        msg_body["show"]=nVisible;                              //是否可见
        msg_body["add_chip"]=nAddChip;                          //是否需要增加筹码
        msg_body["head_photo_serial"]=pHeadPhoto;               //用户头像
        string   str_msg =   writer.write(msg_body);

        DzpkSend(0,CMD_LUCK_CARD_AWARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,nUserID);

        WriteLog(USER_ACTIVITY_CARD_LEVEL,"send user_id(%d) content %s",nUserID,(char *)str_msg.c_str());
    }
    return 0;
}
/*幸运手牌活动,nStage = 0 开始手牌，1 结束牌型,2 结束没牌型*/
int StdLuckyCardActivity(dealer *pRoom,int nStage)
{
    if(pRoom)
    {
        if(DzpkIfCpsOfType(pRoom->room_type) == 0)
        {
        }
        else
        {
            //活动是否正在进行
            if(StdActivityAwardRunning(SYSTEM_LUCKY_CARD_AWARD) == 0)
            {
                if(nStage == 0)
                {
                    //开始手牌
                    for(unsigned int i=0; i<(pRoom->players_list).size(); i++)
                    {
                        if(pRoom->players_list[i].seat_num > 0 && pRoom->players_list[i].user_staus >= 2)
                        {
                            if(pRoom->players_list[i].hand_cards.size() >= 2)
                            {
                                LUCK_CARD_HAND_T _CarHand;
                                if(StdGetHandCardAward(pRoom->players_list[i].hand_cards[0],
                                                       pRoom->players_list[i].hand_cards[1],(void*)&_CarHand) == 0)
                                {
                                    //获得奖励
                                    StdSendLuckCardPack(pRoom,&pRoom->players_list[i],_CarHand.nSmallBlind,_CarHand.nAwardHandChip,_CarHand.nAwardMultipleChip,_CarHand.szName,0,_CarHand.nType);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if(nStage == 1 || nStage == 2)
                    {
                        //结束之前发送手牌消息
                        if(pRoom->arryLuckCard.size() > 0)
                        {
                            for(int k=0; k<(int)pRoom->arryLuckCard.size(); k++)
                            {
                                stdSendLuckCardParam(pRoom,pRoom->arryLuckCard[k].nUserID,
                                                     pRoom->arryLuckCard[k].szUserAccount,pRoom->arryLuckCard[k].nAwardChip,pRoom->arryLuckCard[k].szTooltip,1,0,
                                                     pRoom->arryLuckCard[k].szHeadPhoto);

                            }
                            pRoom->arryLuckCard.clear();
                        }
                    }

                    if(nStage == 1)
                    {
                        //结束牌型
                        for(unsigned int i=0; i<(pRoom->players_list).size(); i++)
                        {
                            if(pRoom->players_list[i].seat_num > 0)
                            {
                                int nUserStatus = pRoom->players_list[i].user_staus;
                                if(nUserStatus == 2 || nUserStatus == 3 || nUserStatus == 4 || nUserStatus == 6)
                                {
                                    if(pRoom->players_list[i].win_cards.size() == 6)
                                    {
                                        LUCK_CARD_TYPE_T _CarType;
                                        if(StdGetCardTypeAward(pRoom->players_list[i].win_cards[5],(void*)&_CarType) == 0)
                                        {
                                            //获得奖励
                                            StdSendLuckCardPack(pRoom,&pRoom->players_list[i],_CarType.nSmallBlind,_CarType.nAwardHandChip,_CarType.nAwardMultipleChip,_CarType.szName,1,_CarType.nType);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
