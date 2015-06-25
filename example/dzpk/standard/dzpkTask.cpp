/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标任务系统
**************************************************************
*/
#include "dzpkTask.h"
#include "../Start.h"
#include "dzpkParsePacket.h"
#include "../../../com/common.h"
#include "../db/dzpkDb.h"
#include "dzpkCustom.h"

SYSTEM_TASK_HEAD_T g_SystemTaskHead;

/*读取系统任务*/
int dzpkReadSystemTask(int nType)
{
    int nContinue = 0;

    if(nType == 0)
    {
        nContinue = 1;
    }

    if(nContinue == 1)
    {
        //释放之前
        TASK_BASE_T *p = g_SystemTaskHead.pFirst;
        TASK_BASE_T *pFree = p;

        while(p != NULL)
        {
            p = p->pNext;
            delete pFree;
            pFree = p;
        }

        g_SystemTaskHead.pFirst = NULL;
        g_SystemTaskHead.pLast = NULL;

        dzpkReadEveryDayTaskInfo(&g_SystemTaskHead);
    }

    return 0;
}
/*任务完成一*/
int dzpkTaskTypePlusOne(int nTaskType,user *pUserInfo)
{
        WriteLog(USER_SYSTEM_TASK_LEVEL,"enter  dzpkTaskTypePlusOne type:%d user id:%d \n",nTaskType,pUserInfo->nUserID);

    return 0;
}

/*具体任务加一*/
int dzpkTaskSpecific(int nTaskID,user *pUserInfo,int nTaskTotal)
{
    /*
    if(pUserInfo)
    {
        if(pUserInfo->nReadTaskFromDb == 0)
        {
            //WriteLog(USER_SYSTEM_TASK_LEVEL,"dzpkTaskSpecific entern dzpkDbReadUserTask  nTaskID:%d user id:%d \n",nTaskID,pUserInfo->nUserID);
            dzpkDbReadUserTask(pUserInfo);
        }
        int nCurTime = StdGetTime();
        int nCurSecond = nCurTime%(60*60*24);
        int nDateSecond = nCurTime - nCurSecond;

        int nFind = 0;
        for(int i=0;i<(int)pUserInfo->arryTask.size();i++)
        {
            if(pUserInfo->arryTask[i].nTaskID == nTaskID)
            {
                if(pUserInfo->arryTask[i].nCreateTime >= nDateSecond && pUserInfo->arryTask[i].nCreateTime <= nDateSecond + 60*60*24)
                {
                    if(pUserInfo->arryTask[i].nCurrent < pUserInfo->arryTask[i].nTotal)
                    {
                        pUserInfo->arryTask[i].nCurrent++;
                        //WriteLog(USER_SYSTEM_TASK_LEVEL,"dzpkTaskSpecific  nTaskID:%d user id:%d right cur:%d total:%d \n",nTaskID,pUserInfo->nUserID,pUserInfo->arryTask[i].nCurrent,pUserInfo->arryTask[i].nTotal);

                        //更新任务
                        dzpkUpdateTaskStage(pUserInfo->arryTask[i].nID,pUserInfo->arryTask[i].nCurrent);

                        //检查是否需要提示
                        dzpkCheckTaskComplete(pUserInfo,nTaskID);
                    }
                    else
                    {
                        //WriteLog(USER_SYSTEM_TASK_LEVEL,"dzpkTaskSpecific  nTaskID:%d user id:%d task compelete cur:%d total:%d \n",nTaskID,pUserInfo->nUserID,pUserInfo->arryTask[i].nCurrent,pUserInfo->arryTask[i].nTotal);
                    }
                    nFind = 1;
                }
                else
                {
                    WriteLog(USER_SYSTEM_TASK_LEVEL,"dzpkTaskSpecific  nTaskID:%d user id:%d  time error \n",nTaskID,pUserInfo->nUserID);
                }
                break;
            }
        }

        if(nFind == 0)
        {
            if(nTaskTotal >= 1)
            {
                //WriteLog(USER_SYSTEM_TASK_LEVEL,"dzpkTaskSpecific  nTaskID:%d user id:%d  add success \n",nTaskID,pUserInfo->nUserID);
                //添加任务
                //dzpkAddTask(pUserInfo->nUserID,nTaskID,1,nTaskTotal);
                //pUserInfo->nReadTaskFromDb = 0;
                //dzpkCheckTaskComplete(pUserInfo,nTaskID);
            }
        }
    }
    */
    return 0;
}


/*检查是否玩了一局*/
int dzpkCheckTaskPlayGame(dealer *pRoom)
{
    int nTickTime = GetTickTime();
    /*
    if(pRoom)
    {
        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
        {
            if(pRoom->players_list[i].seat_num > 0 && pRoom->players_list[i].user_staus >= 2)
            {
                dzpkTaskTypePlusOne(DZPK_SYSTEM_TASK_PLAY_GAME_TYPE,&pRoom->players_list[i]);
            }
        }
    }
    */
    WriteRoomLog(40000,pRoom->dealer_id,0," room task [%d] \n",GetTickTime() - nTickTime);
    return 0;
}
/*检查是否赢牌和赢的大*/
int dzpkCheckTaskWin(int nChip,user *pUserInfo)
{
    if(nChip > 0)
    {
        /*
        dzpkTaskTypePlusOne(DZPK_SYSTEM_TASK_WIN_GAME_TYPE,pUserInfo);
        if(nChip > 3000)
        {
            dzpkTaskTypePlusOne(DZPK_SYSTEM_TASK_WIN_CHIP_TYPE,pUserInfo);
        }
        */
    }
    return 0;
}
/*发完成消息给用户*/
int dzpkCheckTaskComplete(user *pUserInfo,int nTaskID)
{
    /*
    if(pUserInfo)
    {
        if(pUserInfo->nReadTaskFromDb == 0)
        {
            dzpkDbReadUserTask(pUserInfo);
        }

        //检查当前任务是否已经完成
        int nCurTime = StdGetTime();
        int nCurSecond = nCurTime%(60*60*24);
        int nDateSecond = nCurTime - nCurSecond;

        for(int i=0;i<(int)pUserInfo->arryTask.size();i++)
        {
            if(pUserInfo->arryTask[i].nTaskID == nTaskID)
            {
                if(pUserInfo->arryTask[i].nCreateTime >= nDateSecond && pUserInfo->arryTask[i].nCreateTime <= nDateSecond + 60*60*24)
                {
                    if(pUserInfo->arryTask[i].nCurrent >= pUserInfo->arryTask[i].nTotal)
                    {
                        //需要提示
                        dzpkSendTaskComplete(pUserInfo);
                    }
                }
                else
                {
                    WriteLog(USER_SYSTEM_TASK_LEVEL,"dzpkTaskSpecific  nTaskID:%d user id:%d  time error \n",nTaskID,pUserInfo->nUserID);
                }
                break;
            }
        }
    }
    */
    return 0;
}
/*进房间检查是否需要提示任务完成*/
int dzpkEnterCheckTaskComplete(user *pUserInfo)
{
    /*
    if(pUserInfo)
    {
        if(pUserInfo->nReadTaskFromDb == 0)
        {
            dzpkDbReadUserTask(pUserInfo);
        }

        //检查当前任务是否已经完成
        int nCurTime = StdGetTime();
        int nCurSecond = nCurTime%(60*60*24);
        int nDateSecond = nCurTime - nCurSecond;

        for(int i=0;i<(int)pUserInfo->arryTask.size();i++)
        {
            if(pUserInfo->arryTask[i].nCreateTime >= nDateSecond && pUserInfo->arryTask[i].nCreateTime <= nDateSecond + 60*60*24)
            {
                if(pUserInfo->arryTask[i].nCurrent >= pUserInfo->arryTask[i].nTotal)
                {
                    if(pUserInfo->arryTask[i].nStatus != -1)
                    {
                        //需要提示
                        dzpkSendTaskComplete(pUserInfo);
                        break;
                    }
                }
            }
            else
            {
            }
        }
    }
    */
    return 0;
}
/*发送有任务完成提示*/
int dzpkSendTaskComplete(user *pUserInfo)
{
    if(pUserInfo)
    {
        Json::Value  msg_body;
        Json::FastWriter  writer;
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["act_commond"]=CMD_TASK_COM_TOOLTIP;
        msg_body["user_account"]=pUserInfo->user_account;                  //用户帐号
        string   str_msg =   writer.write(msg_body);

        DzpkSend(0,CMD_TASK_COM_TOOLTIP,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pUserInfo->room_num,pUserInfo->nUserID);

    }
    return 0;
}
