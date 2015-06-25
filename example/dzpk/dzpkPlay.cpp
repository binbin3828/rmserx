/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克玩法
**************************************************************
*/
#include "dzpkPlay.h"
#include "Start.h"
#include "standard/dzpk.h"
#include <string>
#include "mainDzpk.h"
#include "championship/cpsRecord.h"
#include "championship/cps.h"

/*获得解释*/
int GetJsonValue(char *pBuf,Json::Value   &msg_commond,int *nMsgCommond = NULL)
{
	int nRet = -1;
	if(pBuf)
	{
		//解析JSON数据包
		std::string strRecMsg = pBuf;

		//Delete TGW head
		int index = strRecMsg.find("tgw_l7_forward");
		if( index >=0)
		{
		    index = strRecMsg.find_first_of("{");
		    strRecMsg = strRecMsg.substr(index);

		}

		Json::Reader  reader;
		if(reader.parse(strRecMsg,msg_commond))
		{
			if(nMsgCommond)
			{
				string  str_commond   =msg_commond["act_commond"].asString(); //命令码，整型
                            	*nMsgCommond = atoi(str_commond.c_str());
			}
			nRet = 0;
		}
		else
		{
			nRet = -2;
		}
	}
	return nRet;
}

/*进入房间*/
int DzpkUserEnterRoom(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
    //printf("enter DzpkUserEnterRoom\n");
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_ENTER_ROOM == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv  content:%s",pBuf);

                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                int nCpsRecordID  = atoi((msg_commond["cps_record_id"].asString()).c_str());
                if(nCpsRecordID > 0)
                {
                    CPS_RECORD_T *pRecord =  CpsGetRecord(nCpsRecordID);
                    if(pRecord)
                    {
                        pModule = DzpkGetModuleFromRomType(pRecord->nType);
                        CpsFreeRecord(pRecord);
                    }
                    else
                    {
                        //返回进不去
    				    CpsEnterRoom( pUser->nSocketID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                        return 0;
                    }
                }

                //取得进入房间函数指针
                if(pModule == NULL)
                {
                    pModule = DzpkGetModule(room_number);
                }

                if(pModule->msgEnterRoom)
                {
    				pModule->msgEnterRoom( pUser->nSocketID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] enter room pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"enter room msg error[%d]<>[%d]  content:%s \n",CMD_ENTER_ROOM,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"enter room GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*用户坐下*/
int DzpkUserSeatDown(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	//调用deal_seat_down函数
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_SEAT_DOWN == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv send content:%s",pBuf);

                //取得坐下函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgSeatDown)
                {
    				pModule->msgSeatDown( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] seat down pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"seat down msg error [%d]<>[%d]  content:%s \n",CMD_SEAT_DOWN,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"seat down GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*加注*/
int DzpkUserAddChip(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_ADD_CHIP == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得加注函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgAddChip)
                {
    				pModule->msgAddChip( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead,DEAL_ADD_CHIP_TYPE_USER);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] add chip pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"add chip msg error [%d]<>[%d]  content:%s \n",CMD_ADD_CHIP,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"add chip GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*聊天*/
int DzpkUserSpeekWords(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_SPEEK_WORDS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"packlen[%d<>%d] recv content:%s",nLen,pHead->nLen,pBuf);

                //取得聊天函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgSpeekWorks)
                {
    				pModule->msgSpeekWorks( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] speek words pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"speak words msg error [%d]<>[%d]  content:%s \n",CMD_SPEEK_WORDS,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"speak words GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*离开位置*/
int DzpkUserLeaveSeat(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_LEAVE_SEAT == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得站起函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgLeaveSeat)
                {
    				pModule->msgLeaveSeat( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] leave seat pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"leave seat msg error [%d]<>[%d]  content:%s \n",CMD_LEAVE_SEAT,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"leave seat GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*播放结束动画*/
int DzpkUserDisplayOver(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_DISPALY_OVER == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得播放完毕函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgDisplayOver)
                {
    				pModule->msgDisplayOver( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] display over pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"display over  msg error:[%d] <> [%d]  content:%s \n",CMD_DISPALY_OVER,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"display over GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*用户离开房间*/
int DzpkUserExitRoom(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_EXIT_ROOM == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得离开房间函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgExitRoom)
                {
    				pModule->msgExitRoom( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] exit room pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"exit room  msg error:[%d]<>[%d]  content:%s \n",CMD_EXIT_ROOM,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"exit room GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*快速开始*/
int DzpkUserQuickStart(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_QUICK_START == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得快速开始函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgQuickStart)
                {
    				pModule->msgQuickStart( pUser->nSocketID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] quick start pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"quick start  msg error[%d]<>[%d]  content:%s \n",CMD_QUICK_START,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"quick start GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}
/*赠送礼物*/
int DzpkPresentGift(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_PRESENT_GIFT == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得赠送礼物函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgPresentGift)
                {
    				pModule->msgPresentGift( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] present gift pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"update user  msg error:[%d]<>[%d] content:%s \n",CMD_PRESENT_GIFT,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"update user GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

int DzpkTimer(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	dzpkTimerCheck();
	return 0;
}
int DzpkUserLeave(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
    switch(nMsgID)
    {
        case CMD_USER_CLOSE:
            {
                if(nLen == sizeof(SOCKET_USER_CLOSE_T) && pBuf)
                {
                    SOCKET_USER_CLOSE_T *pSocketClose = (SOCKET_USER_CLOSE_T *)pBuf;
                    if(pSocketClose)
                    {
                        if(pSocketClose->nType >= 0)
                        {
                            dzpkUserExitAction(pSocketClose->nUserID,pSocketClose->nRoomID);
                        }
                    }
                }
            }
            break;
    }
    return 0;
}
int DzpkPresentChips(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_PRESENT_CHIPS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得赠送筹码函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgPresentChip)
                {
    				pModule->msgPresentChip( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] present ship pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"present chips  msg error[%d]<>[%d]  content:%s \n",CMD_PRESENT_CHIPS,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"present chips GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}
int DzpkAddFriends(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_ADD_FRIEND == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得添加好友函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgAddFriend)
                {
    				pModule->msgAddFriend( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] add frients pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"add friend  msg error [%d]<>[%d]  content:%s \n",CMD_ADD_FRIEND,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"add friend GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

int DzpkHeartWare(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_HEART_WAVE == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得心跳函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgHeartWave)
                {
    				pModule->msgHeartWave( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] heart ware pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_HEART_WAVE,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}



int DzpkGetStandby(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_GET_STANDBY == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
				deal_get_standby( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_GET_STANDBY,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}


int DzpkLayHandCards(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_LAY_CARDS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
				lay_hand_cards( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_GET_STANDBY,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}



int DzpkResetHandChips(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_RESET_CHIPS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
				reset_hand_chips( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_GET_STANDBY,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

int DzpkGetLevelGifts(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_LEVEL_GIFTS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
				get_level_gift( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_LEVEL_GIFTS,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

int DzpkDealLevelGifts(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_DEAL_GIFTS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
				deal_level_gift( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_DEAL_GIFTS,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

int DzpkDealCountDown(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_CNTD_GIFTS == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
				deal_count_down( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_CNTD_GIFTS,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*获得锦标赛列表*/
int DzpkCpsGetRankList(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_CPS_RANK_LIST == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv  content:%s",pBuf);

                //取得获得用户列表函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);

                if(pModule->msgCpsGetRankList)
                {
    				pModule->msgCpsGetRankList( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] cps rank list pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"cps rank list msg error[%d]<>[%d]  content:%s \n",CMD_CPS_RANK_LIST,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"enter room GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}
/*发送消息*/
int DzpkSendMsgToRoomUser(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{

	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_SEND_MSG_TO_ROOM == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
                int nType   =atoi((msg_commond["type"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
                deal_send_to_room(pUser->nUserID,room_number,pBuf,nLen,nType);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware  msg error [%d]<>[%d]  content:%s \n",CMD_SEND_MSG_TO_ROOM,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}
/*发起投票禁止某个用户*/
int DzpkProhibitUser(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_PROHIBIT_USER == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得离开房间函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgProhibitUser)
                {
    				pModule->msgProhibitUser(pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] prohibit user pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"prohibit user  msg error:[%d]<>[%d]  content:%s \n",CMD_PROHIBIT_USER,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"prohibit user GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*投票*/
int DzpkProhibitUserVote(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_PROHIBIT_USER_VOTE== nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得离开房间函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgProhibitUserVote)
                {
    				pModule->msgProhibitUserVote( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] prohibit vote pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"prohibit vote  msg error:[%d]<>[%d]  content:%s \n",CMD_PROHIBIT_USER_VOTE,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"prohibit vote  GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}

/*告诉服务器重新读背包*/
int DzpkReReadPack(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_RE_READ_USER_PACK == nMsgID)
			{
                int room_number   =atoi((msg_commond["room_number"].asString()).c_str());
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);

                //取得离开房间函数指针
                DZPK_MODULE_T *pModule = DzpkGetModule(room_number);
                if(pModule->msgReReadPack)
                {
    				pModule->msgReReadPack( pUser->nUserID,dealers_vector,msg_commond,connpool,level_list,pUser,pHead);
                }
                else
                {
				    WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"room_number:[%d] re read pack pointer null \n",room_number);
                }
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"re read pack  msg error:[%d]<>[%d]  content:%s \n",CMD_RE_READ_USER_PACK,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"re read pack GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}


int dzpkGiveTipToDealer(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_GIVE_TIP_TO_DEALER == nMsgID)
			{
                int room_number = atoi((msg_commond["room_number"].asString()).c_str());
                string seat_num = msg_commond["seat_num"].asString();
				WriteRoomLog(4000,room_number,pUser->nUserID,"\n\n");
				WriteRoomLog(4000,room_number,pUser->nUserID,"recv content:%s",pBuf);
                dealGiveTipToDealer(room_number, pUser, seat_num);
			}
			else
			{
				WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware msg error [%d]<>[%d] content:%s \n",CMD_GIVE_TIP_TO_DEALER,nMsgID,pBuf);
			}
		}
		else
		{
			WriteRoomLog(50000,pUser->nRoomID,pUser->nUserID,"heart ware GetJsonValue error content:%s \n",pBuf);
		}
	}
	return 0;
}


int reloadMemData(void *pSocketUser,int nMsgID,PACKET_HEAD_T *pHead,char *pBuf,int nLen,char **pBackUserBuf,int *pBackBufLen)
{
	DZPK_USER_T *pUser = (DZPK_USER_T *)pSocketUser;
	if(pUser)
	{
		Json::Value   msg_commond;
		int nMsgID = 0;
		if(GetJsonValue(pBuf,msg_commond,&nMsgID) == 0)
		{
			if(CMD_READ_MEM_DATA == nMsgID)
			{
                printf("recv reload memory data message\n");
                g_pMemData->loadMemData();
			}
		}
	}
	return 0;
}


