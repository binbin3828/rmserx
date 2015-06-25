/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克玩法
**************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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
#include "standard/custom.h"
#include "standard/dzpk.h"
#include <iostream>
#include "include/json/json.h"
#include "../../core/loger/log.h"
#include "fstream"
#include <openssl/md5.h>
#include "Start.h"
#include "../../com/common.h"
#include "championship/cps.h"
#include "db/dzpkDb.h"
#include "db/MemData.h"




vector<dealer *>  dealers_vector; //所有房间荷官存储队列
vector<level_system>  level_list;
ConnPool *connpool = NULL;
CSafeHash g_dzpkTimer(500);
CSafeHash g_dzpkDealers;
DZPK_DB_INFO_T *g_pDzpkDbInfo = new DZPK_DB_INFO_T();
SYSTEM_ACTIVITY_HEAD_T g_SystemActivity;                    //系统活动
int g_nSystemCountDownMaxLevel = 7;                         //总公七个宝箱

MemData *g_pMemData = NULL;

/*开数据库*/
int dzpkStartDb()
{
    connpool = ConnPool::GetInstance();

    g_pMemData = new MemData();
    g_pMemData->loadMemData();

    return 0;
}
/*初始化*/
int dzpkInit()
{
    //荷官容器第0位填充
	DEALER_T init_dealer;
	init_dealer.pDealer = new dealer();
	init_dealer.pDealer->dealer_id = 0;



    //数据库连接池初始化
    Connection *con = connpool->GetConnection();
	if(con == NULL)
	{
		WriteLog(40008,"get db connection fail \n");
        exit(1);
	}
        Statement *state =con->createStatement();//获取连接状态
        state->execute("use dzpk");//使用指定的数据库
        string  sql="select * from tbl_room_mst order by small_blind asc;";

        string  sql_up="update tbl_room_mst set current_seat_person=0,current_sideline_person=0 where room_id >0;";
                // UPDATE
                state->execute(sql_up);

        // 查询
        ResultSet *result = state->executeQuery(sql);

        vector<room>  room_list;
        // 输出查询
        while (result->next()) {
            room rm;
            rm.room_id = result->getInt("room_id");
            rm.room_name = result->getString("room_name");
            rm.current_seat_person = result->getInt("current_seat_person");
            rm.current_sideline_person = result->getInt("current_sideline_person");
            rm.small_blind = result->getInt("small_blind");
			rm.charge_per = result->getInt("charge_per");
            rm.minimum_bring = result->getString("minimum_bring");
            rm.maximum_bring = result->getString("maximum_bring");
            rm.room_seat_number = result->getInt("room_seat_number");
            rm.room_type= result->getInt("room_type");
            rm.room_level_st = result->getInt("room_level_st");
            rm.room_level_nd = result->getInt("room_level_st");
            rm.bet_time = result->getInt("bet_time");
            rm.host_ip = result->getString("host_ip");
            rm.port = result->getString("port");
            rm.before_chip = result->getString("before_chip");

            room_list.push_back(rm);
        }
        if(result)
        {
            delete result;
            result = NULL;
        }
        delete state;//删除连接状态
        connpool->ReleaseConnection(con);//释放当前连接

        int nRoomCount = 0;

        for(unsigned int i=0;i< room_list.size();i++)
	{
	    DEALER_T init_dealer;
	    init_dealer.pDealer = new dealer();

            room rm = room_list.at(i);

            init_dealer.pDealer->dealer_id = rm.room_id;
            init_dealer.pDealer->small_blind = rm.small_blind;
			init_dealer.pDealer->charge_per = rm.charge_per;
            init_dealer.pDealer->min_buy_chip = atoi(rm.minimum_bring.c_str());
            init_dealer.pDealer->max_buy_chip = atoi(rm.maximum_bring.c_str());
            init_dealer.pDealer->before_chip = atoi(rm.before_chip.c_str());
            init_dealer.pDealer->room_seat_num = rm.room_seat_number;
            init_dealer.pDealer->max_follow_chip = atoi(rm.maximum_bring.c_str());
            init_dealer.pDealer->bet_time = rm.bet_time;
            init_dealer.pDealer->room_type = rm.room_type;
            init_dealer.pDealer->room_level_st = rm.room_level_st;
            init_dealer.pDealer->room_level_nd = rm.room_level_nd;
            init_dealer.pDealer->room_name= rm.room_name +"["+ itoa(rm.room_id,10)+"]";

            int desk_cd[5]={ 0,0,0,0,0 };
            memcpy(init_dealer.pDealer->desk_card,desk_cd,sizeof(desk_cd));//数组拷贝
            //初始化一副牌，百位代表了花色4、3、2、1 分别代表 黑 红 梅 方 , 十位和个位代表牌面2~A
            int cards[52] ={102,103,104,105,106,107,108,109,110,111,112,113,114,202,203,204,205,206,207,208,209,210,211,212,213,214,302,303,304,305,306,307,308,309,310,311,312,313,314,402,403,404,405,406,407,408,409,410,411,412,413,414};
            //先发手牌，再发公共牌
            //int cards[52] ={403,413,405,302,113,308,108,305,207,111,103,312,210,111,414,114,110,306,104,307,211,204,205,212,213,214,302,303,304,305,306,307,308,309,310,311,312,313,314,402,403,404,405,406,407,408,409,410,411,412,413,414};

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
            init_dealer.pDealer->nTurnMinChip=0;
            init_dealer.pDealer->nTurnMaxChip=0;
            init_dealer.pDealer->seat_bank =0;
            init_dealer.pDealer->seat_small_blind =0;
            init_dealer.pDealer->seat_big_blind =0;
            init_dealer.pDealer->nStepOver =0;

            if(nRoomCount == 0)
            {
                DEALER_T init_dealerTemp;
                init_dealerTemp.pDealer = new dealer();
                *init_dealerTemp.pDealer = *init_dealer.pDealer;
                init_dealerTemp.pDealer->dealer_id = 1;

                g_dzpkDealers.Insert(init_dealerTemp.pDealer->dealer_id,(void*)&init_dealerTemp,sizeof(DEALER_T));
                dealers_vector.push_back(init_dealerTemp.pDealer);
            }

            dealers_vector.push_back(init_dealer.pDealer);
	        g_dzpkDealers.Insert(init_dealer.pDealer->dealer_id,(void*)&init_dealer,sizeof(DEALER_T));
            nRoomCount++;
        }

	for(int i=0;i<(int)dealers_vector.size();i++)
	{
		dealers_vector[i]->nVecIndex = i;
	}

        //读取等级-经验值系统
        Connection *con1 = connpool->GetConnection();
        Statement *state1 =con1->createStatement();//获取连接状态
        state1->execute("use dzpk");//使用指定的数据库
        string  sql1="select * from tbl_level_exp_mst;";
        // 查询
        ResultSet *result1 = state1->executeQuery(sql1);

        // 输出查询
        while (result1->next()) {
            level_system lm;
            lm.ex_id = result1->getInt("ex_id");
            lm.level = result1->getInt("level");
            lm.level_title = result1->getString("level_title");
            string slevel_upgrade =result1->getString("exp_upgrade");
            lm.exp_upgrade = atol(slevel_upgrade.c_str());
            string sexp_total =result1->getString("exp_total");
            lm.exp_total = atol(sexp_total.c_str());
            level_list.push_back(lm);
        }
        if(result1)
        {
            delete result1;
            result1 = NULL;
        }
        delete state1;//删除连接状态
        connpool->ReleaseConnection(con1);//释放当前连接


        /*清除用户在房间标识*/
        dzpkDbClearUserRoomMark();

#ifdef CPS_TEXT
        dealer *pRoom = DzpkGetRoomInfo(1030);
        if(pRoom)
        {
            pRoom->room_type = 5;
        }

        CPS_RECORD_T _Record;
        _Record.nRecordID = 100;
        _Record.nType = 5;
        _Record.nStartTime = time(NULL) + 20;
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

	return 0;
}


int dzpkStartUninit()
{
    ConnPool::ReleaseInstance();
    delete g_pMemData;
    return 0;
}
/*
*  德州扑克开始
*/
int dzpkStart()
{
	/*初始化*/
	dzpkInit();
	return 0;
}

int dzpkUserExit(user *pUser,int nRoomIndex)
{
    if(pUser)
    {
        pUser->disconnect_flg = 0;//连接状态设为离线
        //更新session时间
        update_session_time(pUser->user_account,connpool);
        //设置定时器删除该玩家
        dztimer timer;

        timer.nTimerID = DzpkGetTimerID();
        timer.nUserID = pUser->nUserID;

        timer.socket_fd =  pUser->socket_fd;
        timer.seat_num = pUser->seat_num;
        memcpy(timer.szUserAccount,pUser->user_account.c_str(),strlen(pUser->user_account.c_str()));
        timer.room_num = nRoomIndex;
        timer.start_time = time(NULL);
        timer.timer_type = 3;//删除玩家信息定时器
        //timer.time_out = 15*60;
        timer.time_out = 5*60;
        g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
    }
    return 0;
}

/*玩家在房间断线处理*/
int dzpkUserExitAction(int nUserID,int nRoomID)
{

    int nContinue = 1;

    if(nRoomID > 0)
    {
        dealer *pRoom = DzpkGetRoomInfo(nRoomID);
        if(pRoom)
        {
            class CAutoLock Lock(&pRoom->Mutex);
            Lock.Lock();

            for(unsigned int j=0;j< pRoom->players_list.size();j++ )
            {
                if(nUserID == pRoom->players_list.at(j).nUserID )
                {
                    dzpkUserExit(&pRoom->players_list.at(j),pRoom->nVecIndex);
                    nContinue = 0;
                    break;
                }
            }
        }
    }

	//标记该玩家为离线状态
    if(nContinue  == 1)
    {
        for(unsigned int i=0;i<dealers_vector.size();i++)
        {
            class CAutoLock Lock(&dealers_vector[i]->Mutex);
            Lock.Lock();

            for(unsigned int j=0;j< dealers_vector.at(i)->players_list.size();j++ )
            {
                if(nUserID == dealers_vector.at(i)->players_list.at(j).nUserID )
                {
                    dzpkUserExit(&dealers_vector.at(i)->players_list.at(j),i);
                    break;
                }
            }
        }
    }
	return 0;
}


/*200毫秒调用一次*/
int dzpkTimerCheck()
{
    //锦标赛
    CpsTimer();

	//检查是否有定时器超时,每200MS检查一次
	long current_time= time(NULL);
	dztimer *pTimer = NULL;

	if(g_dzpkTimer.GetSize() <= 0)
	{
		return -1;
	}

	SAFE_HASH_LIST_T * pList = g_dzpkTimer.GetList();
	if(pList)
	{
		for(int i=0;i<pList->nSize;i++)
		{
			pTimer = (dztimer *)pList->pTable[i];
			if(pTimer && ( (current_time - pTimer->start_time -1) >= pTimer->time_out))
			{
				if(pTimer->room_num >= 0 && pTimer->room_num < (int)dealers_vector.size())
				{
					//如果是弃牌定时器，系统帮助弃牌 ；如果是display over定时器，系统帮助发送display over消息
					switch(pTimer->timer_type)
					{
					    case 1:
					    {//弃牌定时器
						 //弃牌操作
						Json::Value   qp_commond;
						qp_commond["msg_head"]="dzpk";
						qp_commond["game_number"]="dzpk";
						qp_commond["area_number"]="normal";
						qp_commond["room_number"]= itoa((dealers_vector.at(pTimer->room_num))->dealer_id ,10);
						qp_commond["act_user_id"]=pTimer->szUserAccount;
						qp_commond["act_commond"]=CMD_ADD_CHIP;
						qp_commond["betMoney"]="0";
						qp_commond["timer_id"]=GetString(pTimer->nTimerID);
						qp_commond["seat_number"]=itoa(pTimer->seat_num,10);
						qp_commond["ctl_type"]="0";
						//记录系统帮助弃牌次数
						for( unsigned int k=0;k<((dealers_vector.at(pTimer->room_num))->players_list).size();k++ )
						{
							    if( ((dealers_vector.at(pTimer->room_num))->players_list).at(k).nUserID == pTimer->nUserID)
							    {
								((dealers_vector.at(pTimer->room_num))->players_list).at(k).give_up_count += 2 ;
							    }
						}

						DZPK_USER_T *pUserInfo = NULL;
						PACKET_HEAD_T Head;
						memset(&Head,0,sizeof(PACKET_HEAD_T ));
						pUserInfo = (DZPK_USER_T *)NetGetNetUser(pTimer->nUserID);
						WriteRoomLog(4120,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter timer type 1 timerID:%d \n",pTimer->nTimerID);

                        //取得加注函数指针
                        DZPK_MODULE_T *pModule = DzpkGetModule(dealers_vector.at(pTimer->room_num)->dealer_id);
                        if(pModule->msgAddChip)
                        {
                            pModule->msgAddChip( pTimer->nUserID,dealers_vector,qp_commond,connpool,level_list,pUserInfo,&Head,DEAL_ADD_CHIP_TYPE_TIMER);
                        }
                        else
                        {
                            WriteRoomLog(4000,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"room_number:[%d] timer add chip pointer null \n",dealers_vector.at(pTimer->room_num)->dealer_id);
                        }

						if( NULL != pUserInfo)
						{
							NetFreeNetUser(pUserInfo);
						}
					    }
					    break;
					    case 2:
					    {//display over 定时器

                             //发送DISPLAY OVER 信息
                            Json::Value   qp_commond;
                            qp_commond["msg_head"]= "dzpk";
                            qp_commond["game_number"]= "dzpk";
                            qp_commond["area_number"]= "normal";
                            qp_commond["room_number"]= itoa((dealers_vector.at(pTimer->room_num))->dealer_id,10);
                            qp_commond["act_user_id"]= pTimer->szUserAccount;
                            qp_commond["act_commond"]= CMD_DISPALY_OVER;
                            qp_commond["timer_id"]=GetString(pTimer->nTimerID);
                            qp_commond["seat_number"]= itoa(pTimer->seat_num,10);

                            DZPK_USER_T *pUserInfo = NULL;
                            PACKET_HEAD_T Head;
                            memset(&Head,0,sizeof(PACKET_HEAD_T ));
                            pUserInfo = (DZPK_USER_T *)NetGetNetUser(pTimer->nUserID);
                            WriteRoomLog(4000,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter timer type 2 timerID:%d \n",pTimer->nTimerID);

                            //取得播放完毕函数指针
                            DZPK_MODULE_T *pModule = DzpkGetModule(dealers_vector.at(pTimer->room_num)->dealer_id);
                            if(pModule->msgDisplayOver)
                            {
                                pModule->msgDisplayOver( pTimer->nUserID,dealers_vector,qp_commond,connpool,level_list,pUserInfo,&Head);
                            }
                            else
                            {
                                WriteRoomLog(4000,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"room_number:[%d] timer diaplay over pointer null \n",dealers_vector.at(pTimer->room_num)->dealer_id);
                            }

                            if( NULL != pUserInfo)
                            {
                                NetFreeNetUser(pUserInfo);
                            }
					    }
					    break;
					    case 3:
					    {
						//删除掉线玩家残留 定时器
						int nExit = 0;
                        class CAutoLock Lock(&dealers_vector[pTimer->room_num]->Mutex);
                        Lock.Lock();
						for(int k = 0;k< (int)dealers_vector[pTimer->room_num]->players_list.size();k++)
						{
							if(dealers_vector[pTimer->room_num]->players_list[k].nUserID == pTimer->nUserID)
							{
								if(dealers_vector[pTimer->room_num]->players_list[k].disconnect_flg == 0)
								{
									nExit = 1;
								}
								else
								{
								}

								break;
							}
						}
                        Lock.Unlock();

						if(nExit == 1)
						{
							Json::Value   qp_commond;
							qp_commond["msg_head"]= "dzpk";
							qp_commond["game_number"]= "dzpk";
							qp_commond["area_number"]= "normal";
							qp_commond["room_number"]= itoa((dealers_vector.at(pTimer->room_num))->dealer_id,10);
							qp_commond["act_user_id"]= pTimer->szUserAccount;
							qp_commond["act_commond"]= CMD_EXIT_ROOM;
							qp_commond["timer_id"]=GetString(pTimer->nTimerID);
							qp_commond["force_exit"]="1";

							DZPK_USER_T *pUserInfo = NULL;
							PACKET_HEAD_T Head;
							memset(&Head,0,sizeof(PACKET_HEAD_T ));
							pUserInfo = (DZPK_USER_T *)NetGetNetUser(pTimer->nUserID);
							WriteRoomLog(4008,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter timer type 3 timerID:%d \n",pTimer->nTimerID);

                            //取得断线玩家离开房间完函数指针
                            DZPK_MODULE_T *pModule = DzpkGetModule(dealers_vector.at(pTimer->room_num)->dealer_id);
                            if(pModule->msgExitRoom)
                            {
                                pModule->msgExitRoom( pTimer->nUserID,dealers_vector,qp_commond,connpool,level_list,pUserInfo,&Head);
                            }
                            else
                            {
                                WriteRoomLog(50000,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"room_number:[%d] timer exit room pointer null \n",dealers_vector.at(pTimer->room_num)->dealer_id);
                            }

							if( NULL != pUserInfo)
							{
								NetFreeNetUser(pUserInfo);
							}
						}
						g_dzpkTimer.Remove(pTimer->nTimerID);
					    }
					    break;
                        case 4:         //踢人到时间
                        {
                            if(pTimer->room_num >= 0 && pTimer->room_num < (int)dealers_vector.size())
                            {
    							WriteRoomLog(USER_PROHIBIT_CAR_LOG_LEVEL,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter prohibit user enter  timer timerID:%d \n",pTimer->nTimerID);
    							WriteRoomLog(4009,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter prohibit user enter  timer timerID:%d \n",pTimer->nTimerID);
                                ProhibitUserVote(dealers_vector.at(pTimer->room_num),-1,-100,pTimer->nTimerID,pTimer->nTimerID);
                            }
							g_dzpkTimer.Remove(pTimer->nTimerID);
                        }
                        break;
					    default:
					   {
							WriteRoomLog(4000,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter timer timerID:%d but can't find the type (%d) \n",pTimer->nTimerID,pTimer->timer_type);
							g_dzpkTimer.Remove(pTimer->nTimerID);
					   }
					   break;
					}
				}
				else
				{
					WriteRoomLog(4000,dealers_vector.at(pTimer->room_num)->dealer_id,pTimer->nUserID,"enter timer timerID:%d but room_num is wrong (%d) \n",pTimer->nTimerID,pTimer->room_num);
					g_dzpkTimer.Remove(pTimer->nTimerID);
				}
			}
		}
		g_dzpkTimer.FreeList(pList);
	}

	return 0;
}
dealer * DzpkGetRoomInfo(int nRoomNum)
{
	dealer *p = NULL;
	DEALER_T *pDealer = (DEALER_T *)g_dzpkDealers.GetVal(nRoomNum);
	if(pDealer)
	{
		p = pDealer->pDealer;
	}
	return p;
}
