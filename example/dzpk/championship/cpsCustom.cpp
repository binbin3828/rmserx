/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.02
** 版本：1.0
** 说明：德州扑克锦标赛
**************************************************************
*/
#include "cps.h"
#include "cpsCustom.h"
#include "cpsRecord.h"
#include "../Start.h"
#include "../standard/dzpkCustom.h"
#include "../standard/dzpkParsePacket.h"
#include "../../../com/common.h"
#include "cpsUser.h"
#include "../mainDzpk.h"
#include "../db/dzpkDb.h"
#include <stdlib.h>

using namespace  std;
extern CSafeHash g_dzpkDealers;

static class CMutex g_GetRoomMutex;                       //获得一个空房间

//#define USER_LEAVE_SECOND       5*60                    //多少秒没操作
#define USER_LEAVE_SECOND         0                       //多少秒没操作
#define CPS_CHANG_LINE  ""                                //换行标识符号

static CPS_ADD_BLIND_DB_HEAD_T g_AddBlindHead;                     //涨盲注数组
static CPS_AWARD_DB_HEAD_T g_AwardHead;                            //奖励数组

/*******************************************
 * 检查是否到时间开始
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsCheckRecord()
{
    int nRet = 0;

    int nRecordCount = CpsRecordGetSize();
    if(nRecordCount  > 0)
    {
	    int current_time = CpsGetTime();

        SAFE_HASH_LIST_T * pList = CpsRecordGetList();
        CPS_RECORD_T * pRecord = NULL;

        if(pList)
        {
            for(int i=0;i<pList->nSize;i++)
            {
               pRecord = (CPS_RECORD_T *)pList->pTable[i];
               if(pRecord)
               {
                   if(pRecord->nStart == 0)
                   {
                        if(current_time - pRecord->nStartTime > 0)
                        {
                            CpsStart(pRecord);
                        }
                   }
                   else if(pRecord->nStart == 1)
                   {
                        if(current_time - pRecord->nLastAddBlindTime > pRecord->nAddBlindTime)
                        {
                            pRecord->nLastAddBlindTime = current_time;

                            int nLastBlind = pRecord->nCurSmallBlind;

                            int nNextBlind = CpsGetNextBlind(pRecord->nType,pRecord->nCurSmallBlind);
                            if(nNextBlind > 0)
                            {
                                pRecord->nCurSmallBlind = nNextBlind;
                            }
                            else
                            {
                                pRecord->nCurSmallBlind = pRecord->nCurSmallBlind*2;
                            }

                            //发送涨盲提示
                            CpsAddSmallblindTooltip(pRecord,nLastBlind,pRecord->nCurSmallBlind);
                        }
                   }
               }
            }
            CpsRecordFreeList(pList);
        }
    }

    return nRet;
}

/*******************************************
 * 检查是否到时间开始
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsCheckUserOut()
{
    int nRet = 0;

    return nRet;
}

/*******************************************
 * 是否到终极赛,如果是，迁移用户
 * nRoomId 那个房间线程调用
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsNoLockCheckFinalTable(CPS_RECORD_T *pRecord,int nRoomId)
{
    int nRet = -1;
    if(pRecord)
    {
        if(pRecord->nStart == 1)
        {
            if(pRecord->nArryRoomIndex <= 0)
            {
                //基本不会出现这种情况
                nRet = -2;
                CpsNoLockEndBack(pRecord,NULL);
            }
            else if(pRecord->nArryRoomIndex <= 1)
            {
                //已经是终极桌了
                CpsNoLockTableStart(pRecord,pRecord->arryRoom[0]);
                nRet = 0;
            }
            else
            {
                int nUserCount = CpsNoLockGetSeatDownUser(pRecord);
                if(nUserCount <= CPS_LAST_ROOM_NUMBER)
                {
                    //用户迁移,拼成终极桌
                    CpsNoLockComposeFinalTables(pRecord,nRoomId);
                    nRet = 0;
                }
            }
        }
        else
        {
            nRet = -3;
        }
    }
    return nRet;
}
/*******************************************
 * 所有桌开始,这是第一次开始，如果发现有的房间少于可开始用户，要拼桌
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsNoLockStartAllTables(CPS_RECORD_T *pRecord)
{
    int nRet = 0;

    if(pRecord)
    {
        int nTablesUser = 0;
        //检查是否需要合并桌子（有可能一开始分配到有的桌子不够用户开始）
        for(int i=0;i<pRecord->nArryRoomIndex;i++)
        {
            nTablesUser = 0;
            dealer *pRoom = DzpkGetRoomInfo(pRecord->arryRoom[i]);
            if(pRoom)
            {
                nTablesUser = CpsGetRoomNumber(pRoom);

                if(nTablesUser <CPS_MIN_ROOM_NUMBER)
                {
                    CpsNoLockComposeTables(pRecord,pRecord->arryRoom[i],0);
                    i--;
                }
            }
        }

        //比赛正式开始
        for(int i=0;i<pRecord->nArryRoomIndex;i++)
        {
            CpsNoLockTableStart(pRecord,pRecord->arryRoom[i]);
        }
    }

    return nRet;
}
/*******************************************
 * 开始一局
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsNoLockTableStart(CPS_RECORD_T *pRecord,int nRoomId)
{
    int nRet = 0;
    int nCurTime = time(NULL);

	 dealer *pRoom = DzpkGetRoomInfo(nRoomId);
    if(pRecord && pRoom)
    {

        int nCurUser = CpsGetRoomNumber(pRoom);

        if(pRecord->nArryRoomIndex <= 1)
        {
            int nLeaveUser = 0;
            class CAutoLock Lock(&pRoom->Mutex);
            Lock.Lock();
            nLeaveUser = CpsPreCheckUserstatus(pRoom,1,nCurTime);
            Lock.Unlock();

            //终极桌
            if(nCurUser <= 1 || nCurUser - nLeaveUser <= 1)
            {
                //赛事完成
                CpsNoLockEndBack(pRecord,pRoom);
            }
            else
            {
                class CAutoLock Lock(&pRoom->Mutex);
                Lock.Lock();
                CpsCheckUserstatus(pRoom,1,nCurTime);
                Lock.Unlock();
                //继续拼抢

                if(nCurUser <= 1)
                {
                    //赛事完成
                    CpsNoLockEndBack(pRecord,pRoom);
                }
                else
                {
                    class CAutoLock Lock(&pRoom->Mutex);
                    Lock.Lock();

                    StdCheckGameStart(pRoom,0);
                }
            }
        }
        else
        {
            class CAutoLock Lock(&pRoom->Mutex);
            Lock.Lock();
            CpsCheckUserstatus(pRoom,1,nCurTime);
            Lock.Unlock();

            if(nCurUser < CPS_MIN_ROOM_NUMBER)
            {
                //人数太少，需要合并
                CpsNoLockComposeTables(pRecord,nRoomId,1);
            }
            else
            {
                //开始
                class CAutoLock Lock(&pRoom->Mutex);
                Lock.Lock();
                //StdClearRoomInfo(pRoom);
                StdCheckGameStart(pRoom,0);
            }
        }
    }
    return nRet;
}
/*******************************************
 * 获得在玩所有玩家
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsNoLockGetSeatDownUser(CPS_RECORD_T *pRecord)
{
    int nRet = 0;

    if(pRecord)
    {
        //检查是否需要合并桌子（有可能一开始分配到有的桌子不够用户开始）
        for(int i=0;i<pRecord->nArryRoomIndex;i++)
        {
            dealer *pRoom = DzpkGetRoomInfo(pRecord->arryRoom[i]);
            if(pRoom)
            {
                class CAutoLock Lock(&pRoom->Mutex);
                Lock.Lock();
                for(int j=0;j<(int)pRoom->players_list.size();j++)
                {
                    if(pRoom->players_list[j].seat_num > 0)
                    {
                        nRet++;
                    }
                }
            }
        }
    }
    return nRet;
}
/*******************************************
 * 合并终极桌
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsNoLockComposeFinalTables(CPS_RECORD_T *pRecord,int nRoomID)
{
    int nRet = 0;

    if(pRecord->nArryRoomIndex >= 2)
    {
        if(nRoomID <= 0)
        {
            //一开始调用，合并所有没开始桌子
            for(int i=0;i<pRecord->nArryRoomIndex;i++)
            {
                if(pRecord->nArryRoomIndex >= 2)
                {
                    dealer *pRoom = DzpkGetRoomInfo(pRecord->arryRoom[i]);
                    if(pRoom)
                    {
                        if(pRoom->step == 0)
                        {
                            CpsNoLockComposeTables(pRecord,pRecord->arryRoom[i],0);
                            i--;
                        }
                    }
                }
                else
                {
                    break;
                }
            }

            //比赛正式开始,第一次就是终极桌，需要调用开始
            for(int i=0;i<pRecord->nArryRoomIndex;i++)
            {
                CpsNoLockTableStart(pRecord,pRecord->arryRoom[i]);
            }
        }
        else
        {
            //合并当前桌子就行，其它桌他们也会调用,这里要判断是否刚好有别的玩家合并到这里，并且已经调用开始
            CpsNoLockComposeTables(pRecord,nRoomID,1);
        }
    }
    return nRet;
}
/*******************************************
 * 拼桌
 * return           //成功返回0 失败放回 -1
 ******************************************/
int CpsNoLockComposeTables(CPS_RECORD_T *pRecord,int nRoomID,int nStart)
{
    int nRet = 0;

    if(pRecord)
    {
        //一般合并需要桌子大于等于2
        if(pRecord->nArryRoomIndex >= 2)
        {
            user *arryTemp = new user[CPS_MAX_ROOM_USER];
            int nCurIndex = 0;
            int nSize = 0;
            int nDelete = 1;

            for(int i=0;i<pRecord->nArryRoomIndex;i++)
            {
                if(pRecord->arryRoom[i] == nRoomID)
                {
                    dealer *pRoom = DzpkGetRoomInfo(pRecord->arryRoom[i]);
                    if(pRoom)
                    {
                        class CAutoLock Lock(&pRoom->Mutex);
                        Lock.Lock();

                        if(pRoom->step == 0)
                        {
                            nSize = (int)pRoom->players_list.size();
                            for(int j=0;j<(int)pRoom->players_list.size();j++)
                            {
                                if(nCurIndex < CPS_MAX_ROOM_USER - 1)
                                {
                                    arryTemp[nCurIndex] = pRoom->players_list[j];
                                    nCurIndex++;
                                }
                            }
                            pRoom->players_list.clear();            //清空用户
                            pRoom->leave_players_list.clear();      //清空用户
                            pRoom->nCpsRecordID = -1;
                        }
                        else
                        {
                            //暂时不能删除
                            nDelete = 0;
                            WriteLog(40002,"composeTables but room is playing record[%d] roomId[%d]\n",pRecord->nRecordID,pRoom->dealer_id);
                        }
                    }

                    if(nDelete == 1)
                    {
                        //删除原先记录
                        for(int j=i + 1;j<pRecord->nArryRoomIndex;j++)
                        {
                            pRecord->arryRoom[j-1] = pRecord->arryRoom[j];
                        }
                        pRecord->nArryRoomIndex--;
                    }
                    break;
                }
            }

            //将用户分配到其它房间
            for(int i=0;i<nCurIndex;i++)
            {
                int nView = 1;                  //是否是旁观
                if(arryTemp[i].seat_num > 0)
                {
                    nView = 0;
                }
                dealer *pRoom =  CpsNoLockGetLessTables(pRecord,nView);
                if(pRoom)
                {
                    class CAutoLock Lock(&pRoom->Mutex);
                    Lock.Lock();
                    //进入房间
                    arryTemp[i].seat_num = 0;
                    arryTemp[i].user_staus = 0;

                    int nSuccess = CpsEnter(pRoom,arryTemp[i]);
                    if(nSuccess == 0)
                    {
                        //非旁观坐下
                        if(nView == 0)
                        {
                            //更新用户在那个房间
                            CpsUpdateUserInRoom(arryTemp[i].nUserID,pRoom->dealer_id);

                            nSuccess = CpsSeatDownIn(pRoom,arryTemp[i].nUserID);
                            if(nSuccess != 0)
                            {
                                //这是一个很大错误
                            }
                            else
                            {
                                //坐下需要调用是否需要开始
                                if(nStart == 1)
                                {
                                    StdCheckGameStart(pRoom,0);
                                }
                            }
                        }
                    }
                    else
                    {
                        //不可能
                    }
                }
                else
                {
                    //不可能的
                    int nCount = CpsUserGetOnline(pRecord->nType,pRecord->nRecordID);
                    WriteLog(40004,"user find room fail[%d] tables[%d] count[%d] view[%d]   \n\n\n\n",arryTemp[i].nUserID,pRecord->nArryRoomIndex,nCount,nView);

                    //出错只能委屈用户，帮用户退出房间
                    update_user_persition(arryTemp[i].user_account,0,connpool);
                    NetRemoveNetUser(arryTemp[i].nUserID);
                }
            }

            if(arryTemp)
            {
                delete []arryTemp;
            }
        }
    }
    return nRet;
}
/*******************************************
 * 拼桌获得一个人数少于标准的桌
 * return           //无论如何要返回成功
 ******************************************/
dealer * CpsNoLockGetLessTables(CPS_RECORD_T *pRecord,int nView)
{
    dealer *pRoom = NULL;

    if(pRecord)
    {
        if(pRecord->nArryRoomIndex > 0)
        {
            //找一个小于6人的房间
            dealer *pTemp = NULL;
            int nTablesUser = 0;
            int nCurNum = 0;
            for(int i=0;i<pRecord->nArryRoomIndex;i++)
            {
                pTemp = DzpkGetRoomInfo(pRecord->arryRoom[i]);
                if(pTemp)
                {
                    nTablesUser = CpsGetRoomNumber(pTemp);
                    if(pRoom == NULL)
                    {
                        //如果是旁观找一个房间人数大于可以基本玩的房间就可以停
                        if(nView == 1)
                        {
                            pRoom = pTemp;
                            nCurNum = nTablesUser;
                            if(nTablesUser  >= CPS_MIN_ROOM_NUMBER)
                            {
                                break;
                            }
                        }
                        else
                        {
                            //先找一个可以坐下的，保证能找到
                            if(nTablesUser  < CPS_ROOM_MAX_NUMBER)
                            {
                                pRoom = pTemp;
                                nCurNum = nTablesUser;
                            }
                        }
                    }
                    else
                    {
                        if(nTablesUser < nCurNum)
                        {
                            //找到一个人数最少少的房间
                            pRoom = pTemp;
                            nCurNum = nTablesUser;

                            //如果是旁观找一个房间人数大于可以基本玩的房间就可以停
                            if(nView == 1)
                            {
                                if(nTablesUser  >= CPS_MIN_ROOM_NUMBER)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return pRoom;
}
/*******************************************
 * 获得一个房间用户数量，里面要自己加锁
 * return           //无论如何要返回成功
 ******************************************/
int CpsGetRoomNumber(dealer *pRoom,int nAllUser)
{
    int nRet = 0;

    class CAutoLock Lock(&pRoom->Mutex);
    Lock.Lock();

    if(nAllUser == 1)
    {
        //取全部用户个数
        nRet = (int)pRoom->players_list.size();
    }
    else
    {
        //取在玩用户个数
        for(int j=0;j<(int)pRoom->players_list.size();j++)
        {
            if(pRoom->players_list[j].seat_num > 0)
            {
                nRet++;
            }
        }
    }
    Lock.Unlock();

    return nRet;
}
/*******************************************
 * 检查房间用户
 * return           //无论如何要返回成功
 ******************************************/
int CpsCheckUserstatus(dealer *pRoom,int nGiveUpLeave,int nCurTime)
{
    int nRet = 0;

    if(pRoom)
    {
        CPS_LAST_RESULT_T *arryUserResult = new CPS_LAST_RESULT_T[CPS_MAX_ROOM_USER];
        int nArryUserResultIndex = 0;

        //需要检查系统帮弃牌两次强制站起
        if(nGiveUpLeave != 0)
        {
             //判断如果有系统帮助连续弃牌两次的，强制站起
            for(unsigned int i=0;i<(pRoom->players_list).size();i++)
             {
                if(pRoom->players_list[i].user_staus == 1 && pRoom->players_list[i].give_up_count  >=2)
                {//状态为坐下的

                    int nContinue = 1;
                    CPS_USER_T * pRecordUser = CpsGetUser(pRoom->players_list[i].nUserID);
                    if(pRecordUser)
                    {
                        if(nCurTime - pRecordUser->nActiveTime > USER_LEAVE_SECOND)
                        {
                        }
                        else
                        {
                            //时间少于5分钟
                            nContinue = 0;
                        }
                    }

                    if(nContinue == 1)
                    {
                        //用户被淘汰,加入淘汰数组
                        if(nArryUserResultIndex < CPS_MAX_ROOM_USER - 1)
                        {
                            arryUserResult[nArryUserResultIndex].pUser = &pRoom->players_list[i];
                            arryUserResult[nArryUserResultIndex].nPreHandChip = CpsGetRecordUserChip(pRoom->players_list[i].nUserID);
                            nArryUserResultIndex++;
                        }

                        char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                        int nSendLen = SEND_PACK_MAX_LEN;

                        if(pSend)
                        {
                            memset(pSend,0,nSendLen);

                            int nSuccess = dzpkPutForceLeaveSeat(pRoom,&pRoom->players_list[i],pSend,nSendLen);
                            int nCommond = CMD_LEAVE_SEAT;

                            //更改状态为旁观
                            pRoom->players_list[i].seat_num = 0;
                            pRoom->players_list[i].user_staus = 0;
                            pRoom->players_list[i].desk_chip = 0;
                            //pRoom->players_list[i].hand_chips = 0;
                            pRoom->players_list[i].last_chips = 0;
                            pRoom->players_list[i].hand_cards.clear();
                            pRoom->players_list[i].win_cards.clear();
                            pRoom->players_list[i].first_turn_blind =0;
                            pRoom->players_list[i].auto_buy = 0;
                            pRoom->players_list[i].max_buy = 0;
                            pRoom->players_list[i].give_up_count = 0;

                            // 发送强制站起
                            if(nSuccess == 0)
                            {
                                //通知房间所有用户
                                for(unsigned int j=0;j<pRoom->players_list.size();j++)
                                {
                                    DzpkSendToUser(pRoom->players_list[j].nUserID,nCommond,pSend,nSendLen,pRoom->dealer_id);
                                }
                            }
                            else
                            {
                                WriteRoomLog(40122,pRoom->dealer_id,0,"check user status error user[%d] \n",pRoom->players_list[i].nUserID);
                            }

                            free(pSend);
                            pSend = NULL;
                        }
                        else
                        {
                            //退出失败
                        }
                    }
                }
             }
        }

        //开局前进行玩家手上筹码数自动买入判断
         for(unsigned int i=0;i<(pRoom->players_list).size();i++)
         {
            //状态为坐下的
            if(pRoom->players_list[i].user_staus == 1)
            {
                //手上筹码不足2倍小盲注加上台费
                //if(pRoom->players_list[i].hand_chips < pRoom->small_blind*2)
                if(pRoom->players_list[i].hand_chips <= 0)
                {
                    char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                    int nSendLen = SEND_PACK_MAX_LEN;
                    if(pSend)
                    {
                        memset(pSend,0,nSendLen);
                        int nForceLeaveSeat = 0,nCommond = 0,nSuccess = -1;

                        //没有勾选自动买入,强制站起
                        nForceLeaveSeat = 1;

                        if(nForceLeaveSeat != 0)
                        {
                            //用户被淘汰
                            if(nArryUserResultIndex < CPS_MAX_ROOM_USER - 1)
                            {
                                arryUserResult[nArryUserResultIndex].pUser = &pRoom->players_list[i];
                                arryUserResult[nArryUserResultIndex].nPreHandChip = CpsGetRecordUserChip(pRoom->players_list[i].nUserID);
                                nArryUserResultIndex++;
                            }

                            nSuccess = dzpkPutForceLeaveSeat(pRoom,&pRoom->players_list[i],pSend,nSendLen);
                            nCommond = CMD_LEAVE_SEAT;

                            //更改状态为旁观
                            pRoom->players_list[i].seat_num = 0;
                            pRoom->players_list[i].user_staus = 0;
                            pRoom->players_list[i].desk_chip = 0;
                            //pRoom->players_list[i].hand_chips = 0;
                            pRoom->players_list[i].last_chips = 0;
                            pRoom->players_list[i].hand_cards.clear();
                            pRoom->players_list[i].win_cards.clear();
                            pRoom->players_list[i].first_turn_blind =0;
                            pRoom->players_list[i].auto_buy = 0;
                            pRoom->players_list[i].max_buy = 0;
                            pRoom->players_list[i].give_up_count = 0;
                        }

                        if(nSuccess == 0)
                        {
                            //通知房间所有用户
                            for(unsigned int j=0;j<pRoom->players_list.size();j++)
                            {
                                DzpkSendToUser(pRoom->players_list[j].nUserID,nCommond,pSend,nSendLen,pRoom->dealer_id);
                            }
                        }
                        else
                        {
                            WriteRoomLog(40122,pRoom->dealer_id,0,"check user status error user[%d] \n",pRoom->players_list[i].nUserID);
                        }

                        free(pSend);
                        pSend = NULL;
                    }
                    else
                    {
                        //分配失败
                    }
                }
            }
         }

        //排序
        CpsSortResultUser(arryUserResult,nArryUserResultIndex);

        //奖励在这里发
        for(int i=nArryUserResultIndex - 1;i>=0;i--)
        {
            if(CpsUserFail(arryUserResult[i].pUser,pRoom,pRoom->nCpsRecordID,0) == 0)
            {
            }
        }

        if(arryUserResult)
        {
            delete []arryUserResult;
           arryUserResult = NULL;
        }
    }

    return nRet;
}

/*检查在游戏中用户两次棋牌*/
int CpsCheckGameingUserStatus(dealer *pRoom,int nUserID)
{
    int nRet = 0;
    int nCurTime = time(NULL);

    if(pRoom)
    {
         //判断如果有系统帮助连续弃牌两次的，强制站起
        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
         {
             if(pRoom->players_list[i].nUserID == nUserID)
             {
                if(pRoom->players_list[i].user_staus == 5 && pRoom->players_list[i].give_up_count  >=2)
                {//状态为坐下的

                    int nContinue = 1;
                    CPS_USER_T * pRecordUser = CpsGetUser(pRoom->players_list[i].nUserID);
                    if(pRecordUser)
                    {
                        if(nCurTime - pRecordUser->nActiveTime > USER_LEAVE_SECOND)
                        {
                        }
                        else
                        {
                            //时间少于5分钟
                            nContinue = 0;
                        }
                    }

                    if(nContinue == 1)
                    {
                        //用户被淘汰.
                        CpsUserFail(&pRoom->players_list[i],pRoom,pRoom->nCpsRecordID,0);

                        char *pSend =(char *)malloc(SEND_PACK_MAX_LEN);
                        int nSendLen = SEND_PACK_MAX_LEN;

                        if(pSend)
                        {
                            memset(pSend,0,nSendLen);

                            int nSuccess = dzpkPutForceLeaveSeat(pRoom,&pRoom->players_list[i],pSend,nSendLen);
                            int nCommond = CMD_LEAVE_SEAT;

                            //更改状态为旁观
                            pRoom->players_list[i].seat_num = 0;
                            pRoom->players_list[i].user_staus = 0;
                            pRoom->players_list[i].desk_chip = 0;
                            //pRoom->players_list[i].hand_chips = 0;
                            pRoom->players_list[i].last_chips = 0;
                            pRoom->players_list[i].hand_cards.clear();
                            pRoom->players_list[i].win_cards.clear();
                            pRoom->players_list[i].first_turn_blind =0;
                            pRoom->players_list[i].auto_buy = 0;
                            pRoom->players_list[i].max_buy = 0;
                            pRoom->players_list[i].give_up_count = 0;

                            // 发送强制站起
                            if(nSuccess == 0)
                            {
                                //通知房间所有用户
                                for(unsigned int j=0;j<pRoom->players_list.size();j++)
                                {
                                    DzpkSendToUser(pRoom->players_list[j].nUserID,nCommond,pSend,nSendLen,pRoom->dealer_id);
                                }
                            }
                            else
                            {
                                WriteRoomLog(40122,pRoom->dealer_id,0,"check user status error user[%d] \n",pRoom->players_list[i].nUserID);
                            }

                            free(pSend);
                            pSend = NULL;
                        }
                        else
                        {
                            //退出失败
                        }
                    }
                }
                break;
             }
         }
    }

    return nRet;
}

int CpsPreCheckUserstatus(dealer *pRoom,int nGiveUpLeave,int nCurTime)
{
    int nRet = 0;

    if(pRoom)
    {
        //需要检查系统帮弃牌两次强制站起
        if(nGiveUpLeave != 0)
        {
             //判断如果有系统帮助连续弃牌两次的，强制站起
            for(unsigned int i=0;i<(pRoom->players_list).size();i++)
             {
                if(pRoom->players_list[i].user_staus == 1 && pRoom->players_list[i].give_up_count  >=2)
                {//状态为坐下的

                    int nContinue = 1;
                    CPS_USER_T * pRecordUser = CpsGetUser(pRoom->players_list[i].nUserID);
                    if(pRecordUser)
                    {
                        if(nCurTime - pRecordUser->nActiveTime > USER_LEAVE_SECOND)
                        {
                        }
                        else
                        {
                            //时间少于5分钟
                            nContinue = 0;
                        }
                    }

                    if(nContinue == 1)
                    {
                        nRet++;
                    }
                }
             }
        }

        //开局前进行玩家手上筹码数自动买入判断
         for(unsigned int i=0;i<(pRoom->players_list).size();i++)
         {
            //状态为坐下的
            if(pRoom->players_list[i].user_staus == 1)
            {
                //手上筹码不足2倍小盲注加上台费
                //if(pRoom->players_list[i].hand_chips < pRoom->small_blind*2)
                if(pRoom->players_list[i].hand_chips <= 0)
                {
                    nRet++;
                }
            }
         }
    }

    return nRet;
}

/*******************************************
 * 刚开始获得一个可以进的房间
 * return           //无论如何要返回成功
 ******************************************/
dealer *CpsGetUseRoom(CPS_RECORD_T *pRecord)
{
    dealer *pRoom = NULL;

    if(pRecord)
    {
        class CAutoLock Lock(&g_GetRoomMutex);
        Lock.Lock();

        dealer *pCopy = NULL;
        //for(int i=0;i<(int)dealers_vector.size();i++)
        for(int i=(int)dealers_vector.size() -1;i>0;i--)
        {
            if(dealers_vector[i] && dealers_vector[i]->room_type == pRecord->nType)
            {
                pCopy = dealers_vector[i];
                if(dealers_vector[i]->nCpsRecordID == pRecord->nRecordID)
                {
                    int nCurSeatUser = CpsGetRoomNumber(dealers_vector[i]);
                    if(nCurSeatUser  <CPS_INIT_ROOM_NUMBER)
                    {
                        int nBreak = 0;
                        //在比赛记录可以找到本放将
                        for(int l=0;l<pRecord->nArryRoomIndex;l++)
                        {
                            if(dealers_vector[i]->dealer_id == pRecord->arryRoom[l])
                            {
                                pRoom = dealers_vector[i];
                                nBreak = 1;
                                break;
                            }
                        }
                        if(nBreak == 1)
                        {
                            break;
                        }
                        /*
                        pRoom = dealers_vector[i];
                        break;
                        */
                    }
                }
                else if(dealers_vector[i]->nCpsRecordID < 0)
                {
                    //取一个里面用户也没有的房间
                    int nCurAllUser = CpsGetRoomNumber(dealers_vector[i],1);
                    if(nCurAllUser <= 0)
                    {
                        //找到一个空房间
                        StdClearRoomInfo(pRoom);
                        pRoom = dealers_vector[i];
                        pRoom->nCpsRecordID = pRecord->nRecordID;
                        pRoom->small_blind = pRecord->nCurSmallBlind;
                        pRecord->arryRoom[pRecord->nArryRoomIndex] = pRoom->dealer_id;
                        pRecord->nArryRoomIndex++;
                        pRoom->room_name= pRecord->szGameName;
                        pRoom->seat_bank = 0;    //庄家的座位号
                        pRoom->seat_small_blind = 0;    //小盲注的座位号
                        pRoom->seat_big_blind = 0;      //大盲注的座位号

                        //清空房间信息
                        StdClearRoomInfo(pRoom);
                        break;
                    }
                }

            }
        }

        if(pRoom == NULL)
        {
            if(pCopy)
            {
                pRoom = CpsAddRoom(pCopy,pRecord);
                if(pRoom)
                {
                    //新增加一个房间
                    pRoom->room_name= pRecord->szGameName;
                    pRoom->nCpsRecordID = pRecord->nRecordID;
                    pRecord->arryRoom[pRecord->nArryRoomIndex] = pRoom->dealer_id;
                    pRecord->nArryRoomIndex++;
                }
            }
            else
            {
                //没有找到相同类型的房间
            }
        }
    }
    return pRoom;
}

/*******************************************
 * 添加一个锦标赛房间
 * return           //无论如何要返回成功
 ******************************************/
dealer *CpsAddRoom(dealer *pRoomCopy,CPS_RECORD_T *pRecord)
{
    dealer *pRoom = NULL;

    if(pRoomCopy && pRecord)
    {

        //取一个空房间ID
        int nDealerMax = 1,nCurDealerID = 0;
        for(int i=1;i<(int)dealers_vector.size();i++)
        {
            if(dealers_vector[i] && dealers_vector[i]->dealer_id > nDealerMax)
            {
                nDealerMax = dealers_vector[i]->dealer_id;
            }
        }

        int nWhileCount = 0;
        while(1)
        {
            nDealerMax++;
            dealer *pRoomTemp = DzpkGetRoomInfo(nDealerMax);
            if(pRoomTemp  == NULL)
            {
                nCurDealerID = nDealerMax;
                break;
            }

            nWhileCount++;
            if(nWhileCount > 100)
            {
                break;
            }
        }

        //找到一个空房间号
        if(nCurDealerID > 0)
        {
            DEALER_T init_dealer;
            init_dealer.pDealer = new dealer();

            if(init_dealer.pDealer)
            {
                init_dealer.pDealer->dealer_id = nCurDealerID;
                init_dealer.pDealer->small_blind = pRecord->nCurSmallBlind;
                init_dealer.pDealer->min_buy_chip = pRoomCopy->min_buy_chip;
                init_dealer.pDealer->max_buy_chip = pRoomCopy->max_buy_chip;
                init_dealer.pDealer->before_chip = 0; //pRoomCopy->before_chip;
                init_dealer.pDealer->room_seat_num = pRoomCopy->room_seat_num;
                init_dealer.pDealer->max_follow_chip = pRoomCopy->max_follow_chip;
                init_dealer.pDealer->bet_time = pRoomCopy->bet_time;
                init_dealer.pDealer->room_type = pRoomCopy->room_type;
                init_dealer.pDealer->room_level_st = pRoomCopy->room_level_st;
                init_dealer.pDealer->room_level_nd = pRoomCopy->room_level_nd;
                //init_dealer.pDealer->room_name= pRoomCopy->room_name;
                init_dealer.pDealer->room_name= pRecord->szGameName;

                int desk_cd[5]={ 0,0,0,0,0 };
                memcpy(init_dealer.pDealer->desk_card,desk_cd,sizeof(desk_cd));//数组拷贝
                 //初始化一副牌，百位代表了花色4、3、2、1 分别代表 黑 红 梅 方 , 十位和个位代表牌面2~A
                int cards[52] ={102,103,104,105,106,107,108,109,110,111,112,113,114,202,203,204,205,206,207,208,209,210,211,212,213,214,302,303,304,305,306,307,308,309,310,311,312,313,314,402,403,404,405,406,407,408,409,410,411,412,413,414};
                memcpy(init_dealer.pDealer->card_dun,cards,sizeof(cards));//数组拷贝，初始化荷官手中的牌

                init_dealer.pDealer->step=0;
                init_dealer.pDealer->all_desk_chips=0;
                init_dealer.pDealer->turn = 0;
                init_dealer.pDealer->seat_small_blind = 0;
                init_dealer.pDealer->seat_big_blind = 0;
                init_dealer.pDealer->look_flag = 0;
                init_dealer.pDealer->add_chip_manual_flg = 0;
                init_dealer.pDealer->current_player_num =0;
                init_dealer.pDealer->nRing =0;
                init_dealer.pDealer->nActionUserID=0;
                init_dealer.pDealer->nCpsRecordID=0;
                init_dealer.pDealer->nVecIndex = 0;

                dealers_vector.push_back(init_dealer.pDealer);
                g_dzpkDealers.Insert(init_dealer.pDealer->dealer_id,(void*)&init_dealer,sizeof(DEALER_T));

                for(int i=1;i<(int)dealers_vector.size();i++)
                {
                    if(dealers_vector[i] && dealers_vector[i]->nVecIndex <= 0)
                    {
                        dealers_vector[i]->nVecIndex = i;
                    }
                }

                pRoom = init_dealer.pDealer;
                StdClearRoomInfo(pRoom);
                pRoom->seat_bank = 0;    //庄家的座位号
                pRoom->seat_small_blind = 0;    //小盲注的座位号
                pRoom->seat_big_blind = 0;      //大盲注的座位号
            }
        }
    }
    return pRoom;
}

/*******************************************
 * 更新用户所在房间
 * return           //无论如何要返回成功
 ******************************************/
int CpsUpdateUserInRoom(int nUserID,int nRoomID)
{
    int nRet = 0;

    DZPK_USER_T *pUserInfo = (DZPK_USER_T *)NetGetNetUser(nUserID);
    if(pUserInfo)
    {
        pUserInfo->nRoomID = nRoomID;
        NetFreeNetUser((void*)pUserInfo);
    }

    return nRet;
}
/*******************************************
 * 用户失败
 * return           //无论如何要返回成功
 ******************************************/
int CpsUserSuccessFirst(user *pUser,dealer *pRoom,int nRecordID)
{
    int nRet = -1;
    CPS_RECORD_T * pRecord = CpsGetRecord(nRecordID);
    if(pUser && pRoom && pRecord)
    {
        CPS_USER_T *pRecordUser = CpsGetUser(pUser->nUserID);
        if(pRecordUser)
        {
            if(pRecordUser->nStart == 1 && pRecord->nStart == 1)
            {
                //pRecordUser->nStart = 0;
                pRecordUser->nFailTime =time(NULL);

                //发送是否推出
                Json::FastWriter  writer;
                Json::Value  msg_body;

                msg_body["msg_head"]="dzpk";
                msg_body["game_number"]="dzpk";
                msg_body["area_number"]="normal";
                msg_body["room_number"]=pRoom->dealer_id;
                msg_body["act_commond"]=CMD_CPS_RESULT;

                char szTooltip[200];
                memset(szTooltip,0,200);

                CPS_AWARD_DB_HEAD_T _AwardHead;                            //奖励数组
                _AwardHead.pFirst = NULL;
                _AwardHead.pLast = NULL;
                int nReward = CpsRewardUser(pUser->nUserID,1,pRecord,szTooltip,pUser,&_AwardHead);

                Json::Value  cps_award_list;
                CPS_AWARD_DB_T *p = _AwardHead.pFirst;
                CPS_AWARD_DB_T *pFree = p;

                while(p != NULL)
                {
                    Json::Value  cps_award;
                    cps_award["reward_type"]=p->nType;
                    cps_award["reward_reward"]=p->szReward;
                    cps_award_list.append(cps_award);

                    p = p->pNext;
                    if(pFree)
                    {
                        delete pFree;
                        pFree = NULL;
                    }
                    pFree = p;
                }

                msg_body["cps_award"]=cps_award_list;

                _AwardHead.pFirst = NULL;
                _AwardHead.pLast = NULL;

                msg_body["tooltipType"]=nReward;
                msg_body["tooltip"]=szTooltip;


                string   str_msg =   writer.write(msg_body);
                DzpkSendToUser(pUser->nUserID,CMD_CPS_RESULT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id);
                nRet = 0;
            }
            CpsFreeNetUser(pRecordUser );
        }
    }

    if(pRecord)
    {
        CpsFreeRecord(pRecord);
    }

    return nRet;
}
/*******************************************
 * 用户失败
 * return           //无论如何要返回成功
 ******************************************/
int CpsUserFail(user *pUser,dealer *pRoom,int nRecordID,int nFinal)
{
    int nRet = -1;
    CPS_RECORD_T * pRecord = CpsGetRecord(nRecordID);
    if(pUser && pRoom && pRecord)
    {
        CPS_USER_T *pRecordUser = CpsGetUser(pUser->nUserID);
        if(pRecordUser)
        {
            if(pRecordUser->nStart == 1 && pRecord->nStart == 1)
            {
                pRecordUser->nStart = 0;
                pRecordUser->nFailTime =time(NULL);

                //发送是否推出
                Json::FastWriter  writer;
                Json::Value  msg_body;

                msg_body["msg_head"]="dzpk";
                msg_body["game_number"]="dzpk";
                msg_body["area_number"]="normal";
                msg_body["room_number"]=pRoom->dealer_id;

                if(nFinal == 1)
                {
                    msg_body["act_commond"]=CMD_CPS_RESULT;
                }
                else
                {
                    msg_body["act_commond"]=CMD_CPS_VIEW;
                }

                char szTooltip[200];
                memset(szTooltip,0,200);
                int nOnlineUser = CpsUserGetOnline(pRecord->nType,pRecord->nRecordID);

                CPS_AWARD_DB_HEAD_T _AwardHead;                            //奖励数组
                _AwardHead.pFirst = NULL;
                _AwardHead.pLast = NULL;
                int nReward = CpsRewardUser(pUser->nUserID,nOnlineUser+1,pRecord,szTooltip,pUser,&_AwardHead);

                Json::Value  cps_award_list;
                CPS_AWARD_DB_T *p = _AwardHead.pFirst;
                CPS_AWARD_DB_T *pFree = p;

                while(p != NULL)
                {
                    Json::Value  cps_award;
                    cps_award["reward_type"]=p->nType;
                    cps_award["reward_reward"]=p->szReward;
                    cps_award_list.append(cps_award);

                    p = p->pNext;
                    if(pFree)
                    {
                        delete pFree;
                        pFree = NULL;
                    }
                    pFree = p;
                }

                msg_body["cps_award"]=cps_award_list;

                _AwardHead.pFirst = NULL;
                _AwardHead.pLast = NULL;

                msg_body["tooltipType"]=nReward;
                msg_body["tooltip"]=szTooltip;


                string   str_msg =   writer.write(msg_body);
                DzpkSendToUser(pUser->nUserID,CMD_CPS_RESULT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id);
                nRet = 0;
            }
            CpsFreeNetUser(pRecordUser );
        }
    }

    if(pRecord)
    {
        CpsFreeRecord(pRecord);
    }

    return nRet;
}
int CpsUserSuccess(CPS_RECORD_T * pRecord,dealer *pRoom,int nSucUserID)
{
    if(pRecord && pRoom)
    {
        class CAutoLock Lock(&pRoom->Mutex);
        Lock.Lock();


        //发送信息，退出房间
        Json::FastWriter  writer;
        Json::Value  msg_body;

        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_CPS_RESULT;
        msg_body["tooltip"]="比赛结束";

        string   str_msg =   writer.write(msg_body);

        CPS_LAST_RESULT_T *arryUserResult = new CPS_LAST_RESULT_T[CPS_MAX_ROOM_USER];
        int nArryUserResultIndex = 0;

        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            if(pRoom->players_list[i].nUserID == nSucUserID)
            {
                if(CpsUserSuccessFirst(&pRoom->players_list[i],pRoom,pRoom->nCpsRecordID) == 0)
                {
                }
                else
                {
                    Json::FastWriter  writer1;
                    Json::Value  msg_body1;

                    msg_body1["msg_head"]="dzpk";
                    msg_body1["game_number"]="dzpk";
                    msg_body1["area_number"]="normal";
                    msg_body1["room_number"]=pRoom->dealer_id;
                    msg_body1["act_commond"]=CMD_CPS_RESULT;
                    msg_body1["tooltip"]="恭喜您获得第一名";

                    string   str_msg1 =   writer1.write(msg_body1);
                    DzpkSendToUser(pRoom->players_list[i].nUserID,CMD_CPS_RESULT,(char *)str_msg1.c_str(),strlen(str_msg1.c_str()),pRoom->dealer_id);
                }
            }
            else
            {
                if(pRoom->players_list[i].seat_num > 0)
                {
                    if(nArryUserResultIndex < CPS_MAX_ROOM_USER-1)
                    {
                        arryUserResult[nArryUserResultIndex].pUser = &pRoom->players_list[i];
                        arryUserResult[nArryUserResultIndex].nPreHandChip = CpsGetRecordUserChip(pRoom->players_list[i].nUserID);
                        nArryUserResultIndex++;
                    }
                }
                else
                {
                    DzpkSendToUser(pRoom->players_list[i].nUserID,CMD_CPS_RESULT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id);
                }

            }

            //更新玩家位置
            update_user_persition(pRoom->players_list[i].user_account,0,connpool);

            //帮用户退出房间
	        //NetRemoveNetUser(pRoom->players_list[i].nUserID);
        }

        //排序
        CpsSortResultUser(arryUserResult,nArryUserResultIndex);

        //奖励在这里发
        for(int i=nArryUserResultIndex - 1;i>=0;i--)
        {
            if(CpsUserFail(arryUserResult[i].pUser,pRoom,pRoom->nCpsRecordID,1) == 0)
            {
            }
            else
            {
                DzpkSendToUser(arryUserResult[i].pUser->nUserID,CMD_CPS_RESULT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id);
            }
        }

        //赢的用户在这里更新
        CPS_USER_T *pRecordUser = CpsGetUser(nSucUserID);
        if(pRecordUser)
        {
            if(pRecordUser->nStart == 1 && pRecord->nStart == 1)
            {
                pRecordUser->nStart = 0;
            }
        }

        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            //帮用户退出当前锦标赛
            CpsRemoveUser(pRoom->players_list[i].nUserID);
        }
        /*
        pRoom->players_list.clear();            //清空用户
        pRoom->leave_players_list.clear();      //清空用户
        */

        //这里可以置为-1，因为找房间会找 -1 加里面一个用户也没有的房间来使用
        pRoom->nCpsRecordID = -1;

        if(arryUserResult)
        {
            delete []arryUserResult;
            arryUserResult = NULL;
        }
    }
    return 0;
}
/*比赛异常结束*/
int CpsForceStop(CPS_RECORD_T * pRecord,int nType)
{
    if(pRecord)
    {
        for(int i=0;i<pRecord->nArryRoomIndex;i++)
        {
            dealer *pRoom = DzpkGetRoomInfo(pRecord->arryRoom[i]);
            if(pRoom)
            {
                CpsForceUserLeave(pRecord,pRoom,nType);
            }
        }
    }
    return 0;
}

/*强制用户离开比赛房间*/
int CpsForceUserLeave(CPS_RECORD_T * pRecord,dealer *pRoom,int nType)
{
    if(pRecord && pRoom)
    {
        //发送信息，退出房间
        Json::FastWriter  writer;
        Json::Value  msg_body;

        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_CPS_RESULT;
        //msg_body["tooltip"]="人数太少，比赛取消并归还报名费";
        msg_body["tooltip"]="由于参赛人数没有达到开赛要求导致比赛取消，抱歉！";

        Json::Value  cps_award_list;

        if(1)
        {
            Json::Value  cps_award;
            cps_award["reward_type"]=1;
            cps_award["reward_reward"]="";
            cps_award_list.append(cps_award);
        }
        msg_body["cps_award"]=cps_award_list;
        //msg_body["tooltipType"]=nReward;

        string   str_msg =   writer.write(msg_body);

        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            DzpkSendToUser(pRoom->players_list[i].nUserID,CMD_CPS_RESULT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id);

            //归还报名费
            if(pRecord->nGameFee > 0)
            {
                pRoom->players_list[i].chip_account += pRecord->nGameFee;
                dzpkDbUpdateUserChip(pRoom->players_list[i].nUserID,pRecord->nGameFee,pRoom->players_list[i].chip_account,DZPK_USER_CHIP_CHANGE_CPS_ENTRYFEE,&pRoom->players_list[i]);
            }
            //更新玩家位置
            update_user_persition(pRoom->players_list[i].user_account,0,connpool);
        }

        //这里可以置为-1，因为找房间会找 -1 加里面一个用户也没有的房间来使用
        pRoom->nCpsRecordID = -1;
    }
    return 0;
}

/*******************************************
 * 更新锦标赛用户筹码
 * return           //无论如何要返回成功
 ******************************************/
int CpsUpdateRecordUserChip(int nUserId,int nHandChip)
{
    int nRet = 0;

    CPS_USER_T *pRecordUser = CpsGetUser(nUserId);
    if(pRecordUser)
    {
        pRecordUser->nCurHandChip = nHandChip;
        CpsFreeNetUser(pRecordUser);
    }

    return nRet;
}
/*******************************************
 * 更新锦标赛用户活动时间
 * return           //无论如何要返回成功
 ******************************************/
int CpsUpdateRecordUserActive(int nUserID)
{
    int nRet = 0;

    CPS_USER_T *pRecordUser = CpsGetUser(nUserID);
    if(pRecordUser)
    {
        pRecordUser->nActiveTime = time(NULL);
        CpsFreeNetUser(pRecordUser);
    }

    return nRet;
}

int CpsGetChar(int nRank,char *szRank)
{
    if(szRank)
    {
        switch(nRank)
        {
            case 1:
                sprintf(szRank,"一");
                break;
            case 2:
                sprintf(szRank,"二");
                break;
            case 3:
                sprintf(szRank,"三");
                break;
            case 4:
                sprintf(szRank,"四");
                break;
            case 5:
                sprintf(szRank,"五");
                break;
            case 6:
                sprintf(szRank,"六");
                break;
            case 7:
                sprintf(szRank,"七");
                break;
            case 8:
                sprintf(szRank,"八");
                break;
            case 9:
                sprintf(szRank,"九");
                break;
            case 10:
                sprintf(szRank,"十");
                break;
            default:
                sprintf(szRank,"%d",nRank);
                break;
        }
    }
    return 0;
}

int CpsRewardUser(int nUserID,int nRank,CPS_RECORD_T * pRecord,char *pTooltip,user *pUser,CPS_AWARD_DB_HEAD_T *pHead)
{
    int nRet = 0;
    char szRank[200];
    memset(szRank,0,200);


    char szTooltip[200];
    memset(szTooltip,0,200);
    nRet = CpsUserCpsResult(nUserID,pRecord->nRanK,nRank,szTooltip,pUser,pHead);

    int awardType = 0;

    if (nRet == 0)
    {
        awardType = pHead->pFirst->nType;
    }

    //记录用户比赛结果
    if (!insertGameRank(pRecord->nRecordID, nUserID, nRank, awardType))
    {
        printf("write game log failed!\n");
    }

    if(nRet==0)
    {
        CpsGetChar(nRank,szRank);
        if(pTooltip)
        {
            sprintf(pTooltip,"恭喜你比赛获得第%s名,%s",szRank,szTooltip);
        }
        nRet = 0;
    }
    else
    {
        if(pTooltip)
        {
            sprintf(pTooltip,"祝贺您在锦标赛中成功战胜%d对手%s，获得了第%d名,很遗憾未能拿到奖%s金！下次继续加油吧！",pRecord->nRecordUserCount - nRank,CPS_CHANG_LINE,nRank,CPS_CHANG_LINE);
        }
        nRet = -1;
    }

    return nRet;
}
/*从数据库读每天锦标赛活动*/
int CpsReadDbEveryDay()
{
    int nRet = 0;
    int nCurTime = CpsGetTime();

    static int nLastTime = 0;

    //没一个小时读一次
    if(nCurTime - nLastTime > 60*60)
    {
      int nCurSecond = nCurTime%(60*60*24);

        nLastTime = nCurTime;

        CPS_RECORD_DB_HEAD_T _Head;
        dzpkGetDbEveryDay(&_Head);

        CPS_RECORD_DB_T *p = _Head.pFirst;
        CPS_RECORD_DB_T *pFree = p;
        while(p != NULL)
        {
            //读两个小时内的记录
            if(p->nStartTime > nCurSecond &&  p->nStartTime - nCurSecond < 24*60*60)
            {
                CPS_RECORD_T _Record;
                memset(&_Record,0,sizeof(CPS_RECORD_T));
                _Record.nRecordID = p->nRecordID;
                _Record.nType =p->nRecordType;
                _Record.nStartTime = (p->nStartTime - nCurSecond) + nCurTime;
                _Record.nStartNum = p->nStartNum;
                _Record.nStart = 0;
                _Record.nArryRoomIndex= 0;
                _Record.nCurSmallBlind = p->nSmallBlind;
                if(_Record.nCurSmallBlind <= 0)
                {
                    _Record.nCurSmallBlind = CPS_INIT_SMALL_BLIND;
                }
                _Record.nInitHandChip= p->nHandChip;
                if(_Record.nInitHandChip <= 0)
                {
                    _Record.nInitHandChip = CPS_INIT_HAND_SHIP;
                }
                _Record.pMutex = new CMutex();
                _Record.nLastAddBlindTime = _Record.nStartTime;
                _Record.nGameFee = p->nGameFee;
                _Record.nAddBlind = CPS_ADD_ONE_BLIND;                  //这个参数没有使用
                _Record.nAddBlindTime = p->nAddBlindTime;
                if(_Record.nAddBlindTime <= 0)
                {
                    _Record.nAddBlindTime = CPS_ADD_BLIND_TIME;
                }
                _Record.nRecordUserCount = 0;
                _Record.nRanK = p->nRankID;
                memcpy(_Record.szGameName,p->szGameName,strlen(p->szGameName));

                CpsInsertRecord(_Record.nRecordID,&_Record,sizeof(_Record));
            }

            p = p->pNext;
            delete pFree;
            pFree = p;
        }
    }

    return nRet;
}

/**/
int CpsReadDbAddBlind()
{
    //释放之前
    CPS_ADD_BLIND_DB_T *p = g_AddBlindHead.pFirst;
    CPS_ADD_BLIND_DB_T *pFree = p;

    while(p != NULL)
    {
        p = p->pNext;
        delete pFree;
        pFree = p;
    }

    g_AddBlindHead.pFirst = NULL;
    g_AddBlindHead.pLast = NULL;

    //读取
    dzpkGetDbAddBlind(&g_AddBlindHead);

    return 0;
}
/*从数据库读所有奖励*/
int cpsReadDbReward()
{
    int nRet = 0;
    //释放之前
    CPS_AWARD_DB_T *p = g_AwardHead.pFirst;
    CPS_AWARD_DB_T *pFree = p;

    while(p != NULL)
    {
        p = p->pNext;
        delete pFree;
        pFree = p;
    }

    g_AwardHead.pFirst = NULL;
    g_AwardHead.pLast = NULL;

    //读取
    dzpkGetDbAward(&g_AwardHead);

    return nRet;
}

/*启动读所有数据*/
int CpsReadDb()
{
    int nRet = 0;

    //读取每天活动
    CpsReadDbEveryDay();

    //读取涨盲注
    CpsReadDbAddBlind();

    //读取所有奖励
    cpsReadDbReward();
    return nRet;
}
int CpsGetTime()
{
    //int nRet = time(NULL) + 8*3600;
    int nRet = StdGetTime();
    return nRet;
}
int CpsGetNextBlind(int nType,int nCurBlind)
{
    int nRet = 0;

    CPS_ADD_BLIND_DB_T *p = g_AddBlindHead.pFirst;

    int nFind = 0;
    while(p != NULL)
    {
        if(p->nType == nType && nFind == 1)
        {
            //找到下一个
            nRet = p->nValue;
            break;
        }

        if(p->nType == nType && p->nValue >= nCurBlind)
        {
            nFind = 1;
        }
        p = p->pNext;
    }
    return nRet;
}


/*获得第几名提示,里面调用奖励*/
int CpsUserCpsResult(int nUserID,int nRankID,int nRank,char *pTooltip,user *pUser,CPS_AWARD_DB_HEAD_T *pHead)
{
    int nRet = -1;

    CPS_AWARD_DB_T *p = g_AwardHead.pFirst;

    while(p != NULL)
    {
        if(p->nRankID == nRankID && p->nRank == nRank)
        {
            if(pTooltip)
            {
                memcpy(pTooltip + strlen(pTooltip),CPS_CHANG_LINE,strlen(CPS_CHANG_LINE));
                memcpy(pTooltip + strlen(pTooltip),p->szRankName,strlen(p->szRankName));
            }
            //服务器获得奖励
            CpsUserGetAward(nUserID,p,pUser);

            //奖励返回给客户端
            CPS_AWARD_DB_T *pTemp = new CPS_AWARD_DB_T();
            if(pTemp)
            {
                memcpy(pTemp,p,sizeof(CPS_AWARD_DB_T));
                pTemp->pNext = NULL;

                if(pHead->pLast == NULL)
                {
                    pHead->pFirst = pTemp;
                    pHead->pLast = pTemp;
                }
                else
                {
                    pHead->pLast->pNext = pTemp;
                    pHead->pLast = pTemp;
                }
            }

            nRet = 0;
        }
        p = p->pNext;
    }

    return nRet;
}
/*用户获得奖励*/
int CpsUserGetAward(int nUserID,CPS_AWARD_DB_T *p,user *pUser)
{
    int nRet = 0;

    if(p && pUser)
    {
        //获得奖励
        switch(p->nType)
        {
            case 1:             //筹码
                {
                    pUser->chip_account += atoi(p->szReward);
                    dzpkDbUpdateUserChip(pUser->nUserID,atol(p->szReward),pUser->chip_account,DZPK_USER_CHIP_CHANGE_CPS_AWARD,pUser);
                }
                break;
        }
    }
    return nRet;
}
/*涨盲注提示*/
int CpsAddSmallblindTooltip(CPS_RECORD_T * pRecord,int nLastBlind,int nCurBlind)
{
    int nRet = 0;

    SAFE_HASH_LIST_T *p = CpsUserGetList(pRecord->nType,pRecord->nRecordID,0);
    if(p)
    {
        Json::FastWriter  writer;
        Json::Value  msg_body;
        Json::Value  player_property;

        char szTooltip[100];
        memset(szTooltip,0,100);
        sprintf(szTooltip,"下一局盲注涨到%d",nCurBlind);

        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["act_commond"]=CMD_CPS_ADD_BLIND;
        msg_body["current_smallblind"]=nLastBlind;
        msg_body["next_smallblind"]=nCurBlind;
        msg_body["add_blind_tooptip"]=szTooltip;

        string   str_msg =   writer.write(msg_body);

        CPS_USER_T * pArryUser = (CPS_USER_T *)p->arry;
        if(pArryUser)
        {
            for(int i=0;i<p->nArrySize;i++)
            {
                DzpkSendToUser(pArryUser[i].nUserID,CMD_CPS_ADD_BLIND,(char *)str_msg.c_str(),strlen(str_msg.c_str()),0);
            }
        }
        CpsUserFreeList(p);
    }

    return nRet;
}

/*发送排名给用户*/
int CpsSendRankToUser(int nRecordID,int nUserID)
{
    int nRet = 0;

    if(nRecordID > 0)
    {
        CPS_RECORD_T *pRecord =  CpsGetRecord(nRecordID);
        if(pRecord)
        {
            //按筹码多少（多->少）
            int nSortType = 1;

            if(pRecord->nStart == 0)
            {
                //按进入时间排序（小->大）
                nSortType = 3;
            }
            SAFE_HASH_LIST_T *p = CpsUserGetList(pRecord->nType,pRecord->nRecordID,nSortType);
            if(p)
            {
                Json::FastWriter  writer;
                Json::Value  msg_body;
                Json::Value  player_property;
                Json::Value  player_list;

                msg_body["msg_head"]="dzpk";
                msg_body["game_number"]="dzpk";
                msg_body["area_number"]="normal";
                msg_body["act_commond"]=CMD_CPS_RANK_LIST;
                msg_body["cps_record_id"]=nRecordID;

                CPS_USER_T * pArryUser = (CPS_USER_T *)p->arry;
                if(pArryUser)
                {
                    for(int i=0;i<p->nArrySize;i++)
                    {
                        DZPK_USER_T *pUserInfo = (DZPK_USER_T *)NetGetNetUser(pArryUser[i].nUserID);
                        if(pUserInfo)
                        {
                            player_property["user_id"]=pArryUser[i].nUserID;
                            player_property["hand_chip"]=pArryUser[i].nCurHandChip;
                            player_property["head_photo_serial"]=pUserInfo->pUser->head_photo_serial;
                            player_property["nick_name"]=pUserInfo->pUser->nick_name;
                            player_list.append(player_property);
                            NetFreeNetUser((void*)pUserInfo);
                        }
                    }
                }

                msg_body["player_property"]=player_list;
                string   str_msg =   writer.write(msg_body);


                if(nUserID > 0)
                {
                    DzpkSendToUser(nUserID,CMD_CPS_RANK_LIST,(char *)str_msg.c_str(),strlen(str_msg.c_str()),0);
                }
                else
                {
                    //全部发
                    //SAFE_HASH_LIST_T *p1 = CpsUserGetList(pRecord->nType,pRecord->nRecordID,0);
                    SAFE_HASH_LIST_T *p1 = CpsUserGetList(pRecord->nType,pRecord->nRecordID,1);
                    if(p1)
                    {
                        CPS_USER_T * pArryUser1 = (CPS_USER_T *)p1->arry;
                        if(pArryUser1)
                        {
                            for(int i=0;i<p->nArrySize;i++)
                            {
                                DzpkSendToUser(pArryUser1[i].nUserID,CMD_CPS_RANK_LIST,(char *)str_msg.c_str(),strlen(str_msg.c_str()),0);
                            }
                        }

                        //释放全部玩家数组
                        CpsUserFreeList(p1);
                    }
                }

                //释放在玩玩家数组
                CpsUserFreeList(p);
            }
            CpsFreeRecord(pRecord);
        }
    }
    return nRet;
}
/*******************************************
 * 获得之前筹码
 * return           //无论如何要返回成功
 ******************************************/
int CpsGetRecordUserChip(int nUserId)
{
    int nRet = 0;

    CPS_USER_T *pRecordUser = CpsGetUser(nUserId);
    if(pRecordUser)
    {
        nRet = pRecordUser->nCurHandChip;
        CpsFreeNetUser(pRecordUser);
    }

    return nRet;
}
/*
 * p < p1 返回 -1
 * p = p1 返回 0
 * p > p1 返回 1
 * */
int CpsCompareResultUser(CPS_LAST_RESULT_T  *p,CPS_LAST_RESULT_T  *p1,int nType)
{
    int nRet = 0;

    if(p && p1 &&p->pUser && p1->pUser)
    {
        switch(nType)
        {
            case 0:
                {
                    WriteLog(4008,"cps  chip[%d] [%d]  \n",p->pUser->hand_chips,p1->pUser->hand_chips);
                    if(p->pUser->hand_chips < p1->pUser->hand_chips)
                    {
                        nRet = -1;
                    }
                    else if(p->pUser->hand_chips> p1->pUser->hand_chips)
                    {
                        nRet = 1;
                    }
                    else
                    {
                        nRet = 0;
                    }
                }
                break;
            case 1:
                {
                    if(p->nPreHandChip< p1->nPreHandChip)
                    {
                        nRet = -1;
                    }
                    else if(p->nPreHandChip > p1->nPreHandChip)
                    {
                        nRet = 1;
                    }
                    else
                    {
                        nRet = 0;
                    }
                }
                break;
        }
    }

    return nRet;
}
int CpsSortResultUserHandChip(CPS_LAST_RESULT_T  *arry,int nStart,int nCount,int nType)
{
    if(nCount >= 2)
    {
        CPS_LAST_RESULT_T  pT;
        for(int i=nStart;i<nStart+nCount;i++)
        {
            for(int j=i+1;j<nStart+nCount;j++)
            {
                if(CpsCompareResultUser(&arry[i],&arry[j],nType) < 0)
               {
                    pT = arry[i];
                    arry[i]= arry[j];
                    arry[j]= pT;
               }
            }
        }
    }
    return 0;
}
/*排名排序*/
int CpsSortResultUser(CPS_LAST_RESULT_T  *pArry,int nCount)
{
    if(nCount >= 2)
    {
        //按当前筹码
        CpsSortResultUserHandChip(pArry,0,nCount,0);

        //按筹码
        int nExitTime = -200;
        int nStartIndex = 0;

        for(int i=0;i<nCount;i++)
        {
            if(pArry[i].pUser)
            {
                if(nExitTime < 0)
                {
                   nExitTime = pArry[i].pUser->hand_chips;
                   nStartIndex = i;
                }
                else
                {
                    if(pArry[i].pUser->hand_chips !=nExitTime)
                    {
                        if(i - nStartIndex >= 2)
                        {
                           CpsSortResultUserHandChip(pArry,nStartIndex,i-nStartIndex,1);
                           nExitTime = -200;
                           nStartIndex = 0;
                        }
                        else
                        {
                           nExitTime = pArry[i].pUser->hand_chips;
                           nStartIndex = i;
                        }
                    }
                    else
                    {
                        //相同
                        if(i == nCount -1)
                        {
                            CpsSortResultUserHandChip(pArry,nStartIndex,i-nStartIndex+1,1);
                        }
                        else
                        {
                            //不是最后一个，不用做什么
                        }
                    }
                }
            }
        }
    }
    return 0;
}
/*获得用户是否走开标识*/
int CpsGetUserLeaveMark(int nUserID)
{
    int nRet = 0;

    CPS_USER_T *pRecordUser = CpsGetUser(nUserID);
    if(pRecordUser)
    {
        nRet = pRecordUser->nUserLeaveGame;
        CpsFreeNetUser(pRecordUser);
    }

    return nRet;
}
