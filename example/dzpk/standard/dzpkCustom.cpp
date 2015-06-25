/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标准流程通用函数
**************************************************************
*/
#include "dzpkCustom.h"
#include "../Start.h"
#include "dzpkParsePacket.h"
#include "../../../com/common.h"
#include "../db/dzpkDb.h"
#include "dzpkTask.h"
#include "dzpkActivity.h"
#include "../../../http/HttpSocket.h"


using namespace  std;

/*******************************************
 * 获得有效座位号
 * pRoom            //房间信息
 * nSeatNumber      //请求座位号
 * return           //成功返回座位号 失败放回 -1
 ******************************************/
int StdGetEffectiveSeatNum(dealer *pRoom,int nSeatNumber)
{
    int nRet = -1;

    if(pRoom)
    {
        if(pRoom->room_seat_num == 5)
        {
            nRet = StdGetEffectiveSeatNumFive(pRoom,nSeatNumber);
        }
        else
        {
            //房间内【快速坐下】时，由服务器分配座位号，房间内满座时，不能点击【快速坐下】按钮
            if( nSeatNumber == 0 )
            {
                //从座位号1开始轮询，找到一个没有人坐的就返回该座位号
                for(int i=1;i<=pRoom->room_seat_num;i++)
                {
                    int countseat=0;
                    for(unsigned int j=0;j<(pRoom->players_list).size();j++)
                    {
                        if((pRoom->players_list).at(j).seat_num == i )
                        {
                            countseat++;
                            break;
                        }
                    }
                    if ( countseat == 0 )
                    {
                        //快速坐下找到一个空位
                        nRet = i;
                        i = pRoom->room_seat_num+1;
                    }
                }
            }
            else
            {
                //原来请求位置
                nRet = nSeatNumber;
            }

            if(nRet > 0)
            {
                int can_not_seat =0;
                //检查，如果本位子上已坐人
                vector<user>::iterator its;
                for( its = (pRoom->players_list).begin() ;its !=(pRoom->players_list).end();its++)
                {
                     if( (*its).seat_num == nRet)
                     {
                        can_not_seat = 1;
                        break;
                     }
                }

                //位子上已坐人
                if( can_not_seat == 1)
                {
                    //重新找一个空位，如果其它位置都已经坐人了就返回 -1（找不到位置）
                    nRet = -1;
                    for( int h=9;h>0;h--)
                    {
                         int been_seated =0;
                         for( its = (pRoom->players_list).begin() ;its !=(pRoom->players_list).end();its++)
                         {
                             if( (*its).seat_num == h )
                             {
                                been_seated = 1;
                                break;
                             }
                         }

                         if( been_seated == 0)
                         {
                            //找到位置了
                            nRet = h;
                            break;
                         }
                    }
                }
                else
                {
                    //成功位置可以坐下
                }
            }
            else
            {
                //快速找座位，但所有位置都已经坐满人了
            }
        }
    }

    return nRet;
}

/*获得座位号，5人桌*/
int StdGetEffectiveSeatNumFive(dealer *pRoom,int nSeatNumber)
{
    int nRet = -1;

    if(pRoom)
    {
        if(nSeatNumber %2 != 1)
        {
            nSeatNumber = 0;
        }
        //房间内【快速坐下】时，由服务器分配座位号，房间内满座时，不能点击【快速坐下】按钮
        if( nSeatNumber == 0 )
        {
            //从座位号1开始轮询，找到一个没有人坐的就返回该座位号
            //for(int i=1;i<=pRoom->room_seat_num;i = i+2)
            for(int i=1;i<=9;i = i+2)
            {
                int countseat=0;
                for(unsigned int j=0;j<(pRoom->players_list).size();j++)
                {
                    if((pRoom->players_list).at(j).seat_num == i )
                    {
                        countseat++;
                        break;
                    }
                }
                if ( countseat == 0 )
                {
                    //快速坐下找到一个空位
                    nRet = i;
                    //i = pRoom->room_seat_num+1;
                    break;
                }
            }
        }
        else
        {
            //原来请求位置
            nRet = nSeatNumber;
        }

        if(nRet > 0)
        {
            int can_not_seat =0;
            //检查，如果本位子上已坐人
            vector<user>::iterator its;
            for( its = (pRoom->players_list).begin() ;its !=(pRoom->players_list).end();its++)
            {
                 if( (*its).seat_num == nRet)
                 {
                    can_not_seat = 1;
                    break;
                 }
            }

            //位子上已坐人
            if( can_not_seat == 1)
            {
                //重新找一个空位，如果其它位置都已经坐人了就返回 -1（找不到位置）
                nRet = -1;
                for( int h=9;h>0;h = h-2)
                {
                     int been_seated =0;
                     for( its = (pRoom->players_list).begin() ;its !=(pRoom->players_list).end();its++)
                     {
                         if( (*its).seat_num == h )
                         {
                            been_seated = 1;
                            break;
                         }
                     }

                     if( been_seated == 0)
                     {
                        //找到位置了
                        nRet = h;
                        break;
                     }
                }
            }
            else
            {
                //成功位置可以坐下
            }
        }
        else
        {
            //快速找座位，但所有位置都已经坐满人了
        }
    }
    return nRet;
}
/*******************************************
 * 用户坐下
 * pRoom            //房间信息
 * nUserID          //用户ID
 * nSeatNumber      //座位号
 * pInfo            //网络包带上来的结构
 * return           //成功返回 0 失败返回 -2(用户筹码不足) -1 没有找到用户
 ******************************************/
int StdUserSeatDown(dealer *pRoom,int nUserID,int nSeatNumber,SEAT_DOWN_REQ_T *pInfo)
{
    int nRet = -1;
    if(pRoom && pInfo && nSeatNumber > 0)
    {
        // 将座位号分配给该玩家，并带入筹码数
         for(unsigned int i=0;i<(pRoom->players_list).size();i++)
         {
             if((pRoom->players_list).at(i).nUserID == nUserID)
             {
                if( pInfo->hand_chips <= (pRoom->players_list).at(i).chip_account )
                {
                    WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL,"userid[%d]  time[%d][%d][%d] resttime[%d]  *********** enter sitdown \n",(pRoom->players_list).at(i).nUserID,
                            (pRoom->players_list).at(i).start_countdown,
                            time(0),time(0) - (pRoom->players_list).at(i).start_countdown,
                            (pRoom->players_list).at(i).cd_rest_time);

                    (pRoom->players_list).at(i).seat_num = nSeatNumber;
                    (pRoom->players_list).at(i).user_staus = 1;
                    (pRoom->players_list).at(i).hand_chips = pInfo->hand_chips;
                    (pRoom->players_list).at(i).last_chips = pInfo->hand_chips;
                    (pRoom->players_list).at(i).desk_chip = 0;
                    (pRoom->players_list).at(i).auto_buy = pInfo->auto_buy;
                    (pRoom->players_list).at(i).max_buy = pInfo->max_buy;
                    (pRoom->players_list).at(i).start_countdown = time(0);  //坐下的时间点记录下来

                    nRet = 0;
                }
                else if((pRoom->players_list).at(i).chip_account>=pRoom->min_buy_chip)
                {
                    WriteLog(USER_SYSTEM_COUNTDOWN_LEVEL,"userid[%d]  time[%d][%d][%d] resttime[%d]  *********** enter sitdown  \n",(pRoom->players_list).at(i).nUserID,
                                (pRoom->players_list).at(i).start_countdown,
                                time(0),time(0) - (pRoom->players_list).at(i).start_countdown,
                                (pRoom->players_list).at(i).cd_rest_time);

                    (pRoom->players_list).at(i).seat_num = nSeatNumber;
                    (pRoom->players_list).at(i).user_staus = 1;
                    (pRoom->players_list).at(i).hand_chips = (pRoom->players_list).at(i).chip_account;
                    (pRoom->players_list).at(i).last_chips = (pRoom->players_list).at(i).chip_account;
                    (pRoom->players_list).at(i).desk_chip = 0;
                    (pRoom->players_list).at(i).auto_buy = pInfo->auto_buy;
                    (pRoom->players_list).at(i).max_buy = pInfo->max_buy;
                    (pRoom->players_list).at(i).start_countdown = time(0);  //坐下的时间点记录下来

                    nRet = 0;
                }
                else
                {
                    WriteRoomLog(50080,pRoom->dealer_id,(pRoom->players_list).at(i).nUserID,"seat down chip no enough.user id[%d] name[%s]  chip[%d] request[%d] room min[%d]  \n",(pRoom->players_list).at(i).nUserID,(pRoom->players_list).at(i).nick_name.c_str(),
                            (pRoom->players_list).at(i).chip_account,
                                pInfo->hand_chips,pRoom->min_buy_chip);

                    nRet = -2;
                }
				//更新玩家携带筹码数
				dzpkDbUpdateChipHand(nUserID,(pRoom->players_list).at(i).hand_chips);
                break;
             }
         }
    }

    return nRet;
}
/*******************************************
 * 清空上一局所有信息
 * pRoom            //房间信息
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdClearRoomInfo(dealer *pRoom,int nStatus)
{
    int nRet = 0;

    if(pRoom)
    {
        int room_number = pRoom->nVecIndex;

        if(nStatus == 0)
        {
            //第一个display over定时器超时后，在牌座上的人可能因为站起或退出等改变状态，此时要清除掉该房间的所有剩余display_over定时器
            dztimer *pTimer = NULL;
            SAFE_HASH_LIST_T * pList = g_dzpkTimer.GetList();
            if(pList)
            {
                for(int i=0;i<pList->nSize;i++)
                {
                    pTimer = (dztimer *)pList->pTable[i];
                    if(pTimer && pTimer->room_num == room_number)
                    {
                        //if(pTimer->timer_type != 3)
                        if(pTimer->timer_type == 1 || pTimer->timer_type == 2)
                        {
                            WriteRoomLog(4000,dealers_vector[room_number]->dealer_id,pTimer->nUserID,"diaplay over before start kill all timer (%d) \n",pTimer->nTimerID);
                            g_dzpkTimer.Remove(pTimer->nTimerID);
                        }
                    }
                }
                g_dzpkTimer.FreeList(pList);
            }

            //牌局状态重新置回到 0：未开始
            (dealers_vector.at(room_number))->step = 0;
            //清空桌面牌
            for(unsigned int i=0;i<5;i++){
                 (dealers_vector.at(room_number))->desk_card[i]=0;
            }
            ////桌面总筹码数清零
            (dealers_vector.at(room_number))->all_desk_chips = 0;
            //边池列表清空
            (dealers_vector.at(room_number))->edge_pool_list.clear();
            //轮到谁清零
            (dealers_vector.at(room_number))->turn = 0;
            (dealers_vector.at(room_number))->look_flag = 1;
            (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
            (dealers_vector.at(room_number))->follow_chip = 0;
            //清空大小盲注记录位
            (dealers_vector.at(room_number))->seat_small_blind = 0;
            (dealers_vector.at(room_number))->seat_big_blind = 0;
            (dealers_vector.at(room_number))->current_player_num = 0;
            //加注上下限
            (dealers_vector.at(room_number))->nTurnMinChip = 0;
            (dealers_vector.at(room_number))->nTurnMaxChip = 0;
            //牌局最后一轮可否跟满注标记
            (dealers_vector.at(room_number))->nStepOver = 0;


            //清空当前在座用户列表属性
            for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
            {
                if( ((dealers_vector.at(room_number))->players_list).at(i).user_staus == 1 )
                {
                    ((dealers_vector.at(room_number))->players_list).at(i).desk_chip=0;//桌面筹码清零
                    ((dealers_vector.at(room_number))->players_list).at(i).hand_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).win_cards.clear();
                    ((dealers_vector.at(room_number))->players_list).at(i).first_turn_blind =0;
                }
            }
        }
        else if(nStatus == 1)
        {
            //清空桌面牌
            for(unsigned int i=0;i<5;i++){
                 (dealers_vector.at(room_number))->desk_card[i]=0;
            }
            ////桌面总筹码数清零
            (dealers_vector.at(room_number))->all_desk_chips = 0;
            //边池列表清空
            (dealers_vector.at(room_number))->edge_pool_list.clear();
            //轮到谁清零
            (dealers_vector.at(room_number))->turn = 0;
            (dealers_vector.at(room_number))->look_flag = 1;
            (dealers_vector.at(room_number))->add_chip_manual_flg = 0;
            (dealers_vector.at(room_number))->follow_chip = 0;
            //清空大小盲注记录位
            (dealers_vector.at(room_number))->seat_small_blind = 0;
            (dealers_vector.at(room_number))->seat_big_blind = 0;
        }
    }
    return nRet;
}
/*******************************************
 * 检查用户筹码状态,开局前需要检查一下用户状态
 * pRoom            //房间信息
 * nStatusChange    //用户状态是否有改变，如果有就置为1 没有0 （状态如果有改变就需要更新房间人数）
 * nCheckFee        //是否需要检查扣台费（锦标赛就不需要）
 * pCurFee          //如果不为空就返回需要扣的台费，0为不需要扣
 * nGiveUpLeave     //是否检查系统帮忙弃牌两次强制站起
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdCheckUserstatus(dealer *pRoom,int &nStatusChange,int nCheckFee,int *pCurFee,int nGiveUpLeave)
{
	WriteLog(4000,"guobin----getin StdCheckUserstatus\n");
    int nRet = 0;
    nStatusChange = 0;

    if(pRoom)
    {
         //台费预扣除
        int  nCurFee = 0;

        //需要检查是否扣台费
        if(nCheckFee == 1)
        {
			//nCurFee = pRoom->small_blind * 2 * pRoom->charge_per / 100;
			nCurFee = pRoom->small_blind * pRoom->charge_per / 50;
        }

        if(pCurFee != NULL)
        {
            *pCurFee = nCurFee;
        }

		
        //被迫离开玩家
        for(int i=0;i<(int)pRoom->arryForceLeaveUser.size();i++)
        {
            WriteLog(4000," exit userID:%d   \n",pRoom->arryForceLeaveUser[i].nUserID);
            CheckForceUserExitRoom(pRoom,pRoom->arryForceLeaveUser[i].nUserID,1,pRoom->arryForceLeaveUser[i].szActionName,pRoom->arryForceLeaveUser[i].szTargetName);
			//更新玩家携带筹码数
			dzpkDbUpdateChipHand(pRoom->arryForceLeaveUser[i].nUserID,0);
        }
        pRoom->arryForceLeaveUser.clear();

		//检测玩家是否被管理员强制下线
		
		WriteLog(4000,"guobin----getin StdCheckUserstatus room_id = %d\n",pRoom->dealer_id);
		for (unsigned int i=0; i<(pRoom->players_list).size(); i++)
		{	
			WriteLog(4000,"guobin----getin StdCheckUserstatus room_id = %d, admin_kick_flag = %d.\n",pRoom->dealer_id, pRoom->add_chip_manual_flg);
			if (pRoom->players_list[i].admin_kick_flag == 1)
			{
				int uid = pRoom->players_list[i].nUserID;
				CheckAdminForceUserExitRoom(pRoom, uid);
				dzpkDbLoginDisable(uid);
			}
		}

		
        //需要检查系统帮弃牌两次强制站起
        if(nGiveUpLeave != 0)
        {
             //判断如果有系统帮助连续弃牌两次的，强制站起
            for(unsigned int i=0;i<(pRoom->players_list).size();i++)
             {
                if(pRoom->players_list[i].user_staus == 1 && pRoom->players_list[i].give_up_count >=2)
                {//状态为坐下的
                    char szSend[1024*5];
                    int nSendLen = 1024*5;
                    memset(szSend,0,nSendLen);

                    int nSuccess = dzpkPutForceLeaveSeat(pRoom,&pRoom->players_list[i],szSend,nSendLen);
                    int nCommond = CMD_LEAVE_SEAT;

                    //更改状态为旁观
                    pRoom->players_list[i].seat_num = 0;
                    pRoom->players_list[i].user_staus = 0;
                    pRoom->players_list[i].desk_chip = 0;
                    pRoom->players_list[i].hand_chips = 0;
                    pRoom->players_list[i].last_chips = 0;
                    pRoom->players_list[i].hand_cards.clear();
                    pRoom->players_list[i].win_cards.clear();
                    pRoom->players_list[i].first_turn_blind =0;
                    pRoom->players_list[i].auto_buy = 0;
                    pRoom->players_list[i].max_buy = 0;
                    pRoom->players_list[i].give_up_count = 0;
					//更新玩家携带筹码数
					dzpkDbUpdateChipHand(pRoom->players_list[i].nUserID,0);

                    //用户状态存在改变
                    nStatusChange = 1;


                    // 发送强制站起
                    if(nSuccess == 0)
                    {
                        //通知房间所有用户
                        for(unsigned int j=0;j<pRoom->players_list.size();j++)
                        {
                            DzpkSendToUser(pRoom->players_list[j].nUserID,nCommond,szSend,nSendLen,pRoom->dealer_id);
                        }
                    }
                    else
                    {
                        WriteRoomLog(4000,pRoom->dealer_id,0,"check user status error user[%d] \n",pRoom->players_list[i].nUserID);
                    }
                }
             }
        }


		/*
		WriteLog(4000,"guobin2----getin StdCheckUserstatus\n");
		for(unsigned int i=0;i<(pRoom->players_list).size();i++)
		{
			
			WriteLog(4000,"guobin3----getin StdCheckUserstatus admin_kick_flag = %d, user_status = %d\n", pRoom->players_list[i].admin_kick_flag, pRoom->players_list[i].user_staus);
			if (pRoom->players_list[i].admin_kick_flag == 0 && pRoom->players_list[i].user_staus <= 1)
			{
				
				WriteLog(4000,"guobin4----getin StdCheckUserstatus, dealer_id = %d\n",pRoom->dealer_id, pRoom->players_list[i].nUserID);
				ForceUserExitRoom(pRoom->dealer_id, pRoom->players_list[i].nUserID, 0);
			}
		}
		*/
		

		/*

		//系统强制退出标志		
		for(unsigned int i=0;i<(pRoom->players_list).size();i++)
		{
			user u_info = pRoom->players_list[i];
			if (u_info.admin_kick_flag == 1)
			{
					//状态为坐下的
                    char szSend[1024*5];
                    int nSendLen = 1024*5;
                    memset(szSend,0,nSendLen);

                    int nSuccess = dzpkPutForceLeaveSeat(pRoom,&pRoom->players_list[i],szSend,nSendLen);
                    int nCommond = CMD_LEAVE_SEAT;

                    //更改状态为旁观
                    pRoom->players_list[i].seat_num = 0;
                    pRoom->players_list[i].user_staus = 0;
                    pRoom->players_list[i].desk_chip = 0;
                    pRoom->players_list[i].hand_chips = 0;
                    pRoom->players_list[i].last_chips = 0;
                    pRoom->players_list[i].hand_cards.clear();
                    pRoom->players_list[i].win_cards.clear();
                    pRoom->players_list[i].first_turn_blind =0;
                    pRoom->players_list[i].auto_buy = 0;
                    pRoom->players_list[i].max_buy = 0;
					//更新玩家携带筹码数
					dzpkDbUpdateChipHand(pRoom->players_list[i].nUserID,0);

                    //用户状态存在改变
                    nStatusChange = 1;


                    // 发送强制站起
                    if(nSuccess == 0)
                    {
                        //通知房间所有用户
                        for(unsigned int j=0;j<pRoom->players_list.size();j++)
                        {
                            DzpkSendToUser(pRoom->players_list[j].nUserID,nCommond,szSend,nSendLen,pRoom->dealer_id);
                        }
                    }
                    else
                    {
                        WriteRoomLog(4000,pRoom->dealer_id,0,"check user status error user[%d] \n",pRoom->players_list[i].nUserID);
                    }
			}
		}
		*/

		
        //开局前进行玩家手上筹码数自动买入判断
         for(unsigned int i=0;i<(pRoom->players_list).size();i++)
         {
            //状态为坐下的
            if(pRoom->players_list[i].user_staus == 1)
            {
                //手上筹码不足2倍小盲注加上台费和前注筹码(如果是必下房)
                if(pRoom->players_list[i].hand_chips < pRoom->small_blind*2 + nCurFee + pRoom->before_chip)
                {
                    char szSend[1024*5];
                    int nSendLen = 1024*5;
                    memset(szSend,0,nSendLen);
                    int nForceLeaveSeat = 0,nCommond = 0,nSuccess = -1;

                    //勾选自动买入
                    if( pRoom->players_list[i].auto_buy == 1)
                    {
                        if(pRoom->players_list[i].max_buy ==1)
                        {
                            //选择最大买入
                            if(pRoom->players_list[i].chip_account >= pRoom->max_buy_chip)
                            {
                                //最大自动买入
                                nSuccess = dzpkPutAutoBuy(pRoom,&pRoom->players_list[i],pRoom->max_buy_chip,szSend,nSendLen);
                                nCommond = CMD_AUTO_BUY;
                                pRoom->players_list[i].hand_chips = pRoom->max_buy_chip;
                                pRoom->players_list[i].last_chips = pRoom->players_list[i].hand_chips;
                            }
                            else
                            {
                                //勾选自动最大买入但不够筹码购买,强制站起
                                nForceLeaveSeat = 1;
                            }
                        }
                        else
                        {
                            //选择最小买入
                            if(pRoom->players_list[i].chip_account >= pRoom->min_buy_chip)
                            {
                                //最小自动买入
                                nSuccess = dzpkPutAutoBuy(pRoom,&pRoom->players_list[i],pRoom->min_buy_chip,szSend,nSendLen);
                                nCommond = CMD_AUTO_BUY;
                                pRoom->players_list[i].hand_chips = pRoom->min_buy_chip;
                                pRoom->players_list[i].last_chips = pRoom->players_list[i].hand_chips;
                            }
                            else
                            {
                                //勾选自动最小买入但不够筹码购买,强制站起
                                nForceLeaveSeat = 1;
                                nCommond = CMD_LEAVE_SEAT;
                            }
                        }
                    }
                    else
                    {
                        //没有勾选自动买入,强制站起
                        nForceLeaveSeat = 1;
                    }

                    if(nForceLeaveSeat != 0)
                    {
                        nSuccess = dzpkPutForceLeaveSeat(pRoom,&pRoom->players_list[i],szSend,nSendLen);
                        nCommond = CMD_LEAVE_SEAT;

                        //更改状态为旁观
                        pRoom->players_list[i].seat_num = 0;
                        pRoom->players_list[i].user_staus = 0;
                        pRoom->players_list[i].desk_chip = 0;
                        pRoom->players_list[i].hand_chips = 0;
                        pRoom->players_list[i].last_chips = 0;
                        pRoom->players_list[i].hand_cards.clear();
                        pRoom->players_list[i].win_cards.clear();
                        pRoom->players_list[i].first_turn_blind =0;
                        pRoom->players_list[i].auto_buy = 0;
                        pRoom->players_list[i].max_buy = 0;
                        pRoom->players_list[i].give_up_count = 0;

                        //用户状态存在改变
                        nStatusChange = 1;
                    }

					//更新玩家携带筹码数
					dzpkDbUpdateChipHand(pRoom->players_list[i].nUserID,pRoom->players_list[i].hand_chips);

                    if(nSuccess == 0)
                    {
                        //通知房间所有用户
                        for(unsigned int j=0;j<pRoom->players_list.size();j++)
                        {
                            DzpkSendToUser(pRoom->players_list[j].nUserID,nCommond,szSend,nSendLen,pRoom->dealer_id);
                        }
                    }
                    else
                    {
                        WriteRoomLog(4000,pRoom->dealer_id,0,"check user status error user[%d] \n",pRoom->players_list[i].nUserID);
                    }
                }
            }
         }
    }

    return nRet;
}
/*******************************************
 * 检查用户筹码状态,开局前需要检查一下用户状态
 * pRoom            //房间信息
 * nCurFee          //当局开始需要扣的台费
 * return           //游戏可以开始返回 0 暂时不能开始返回  -1
 ******************************************/
int StdCheckGameStart(dealer *pRoom,int nCurFee)
{
    int nRet = -1;

    if(pRoom)
    {
        WriteRoomLog(4000, pRoom->dealer_id, 0, "pRoom->step: ***[%d]*** \n", pRoom->step);
        if(pRoom->step == 0)
        {
            //清除过时记录
            StdUpdateNoInUser(pRoom);

            int nSeatNum =0;
            for(unsigned int i=0;i<(pRoom->players_list).size();i++)
            {
                if( pRoom->players_list[i].user_staus == 1)
                {
                    nSeatNum++;
                }
            }

            //两人是游戏开始的基本条件
            if(nSeatNum >= 2)
            {
                //清空上一局所有消息
                StdClearRoomInfo(pRoom);

                //当前轮数加一
                pRoom->nRing++;
                WriteRoomLog(40000,pRoom->dealer_id,0," start play  ring:[%d] \n",pRoom->nRing);
                WriteRoomLog(4005,pRoom->dealer_id,0," start play  ring:[%d] \n",pRoom->nRing);

                //多少用户玩游戏
                pRoom->current_player_num =nSeatNum;

                //洗牌
                StdShuffleCards(pRoom);

                //发牌
                StdDealCards(pRoom);

                pRoom->step=1;                              //牌局状态调整为 1、发底牌
                pRoom->leave_players_list.clear();          //清空上局离开或站起玩家缓存队列

                //幸运手牌活动
                StdLuckyCardActivity(pRoom,0);

                for(unsigned int i=0;i<pRoom->players_list.size();i++)
                {
                    pRoom->players_list[i].last_chips = pRoom->players_list[i].hand_chips;                                                              //再次更新上局手牌数据，防止中途赠送筹码产生差异
                }
                //扣台费
                if(nCurFee > 0)
                {
                    for( unsigned int i=0;i<pRoom->players_list.size();i++)
                    {
                        if(pRoom->players_list[i].user_staus >= 1)
                        {
                             //之前有检查筹码比台费加前注筹码(如果是必下房)多
                             pRoom->players_list[i].hand_chips = pRoom->players_list[i].hand_chips - nCurFee;
                             //pRoom->players_list[i].chip_account = pRoom->players_list[i].chip_account - nCurFee;
                             //pRoom->players_list[i].last_chips = pRoom->players_list[i].hand_chips;//及时更新
                        }
                    }
                }

                //下大小盲注
                StdBigSmallBind(pRoom);

                //计算到谁操作和follow
                pRoom->turn = next_player(pRoom->seat_big_blind,dealers_vector,pRoom->nVecIndex);

                //跟注值
                pRoom->follow_chip = StdGetFollowChip(pRoom);
                if(pRoom->follow_chip <= 0)
                {
                    pRoom->follow_chip = pRoom->small_blind * 2;
                }

                 if(pRoom->turn <= 0)
                 {
                     //强制结束
                    give_up_branch(0,dealers_vector,pRoom->nVecIndex,"",0,0,connpool,level_list,NULL,DEAL_RESULT_TYPE_FORCELEAVE);
                 }
                 else
                 {

                    //发送下一个加注用户和其它用户预操作
                    StdTurnToUser(pRoom);
                 }

                nRet = 0;
            }
            else
            {
                pRoom->seat_small_blind = 0;
                pRoom->seat_big_blind = 0;
                pRoom->seat_bank =0;

                //发送清除房间信息消息
                Json::FastWriter  writer;
                Json::Value  msg_body;
                msg_body["msg_head"]="dzpk";
                msg_body["game_number"]="dzpk";
                msg_body["area_number"]="normal";
                msg_body["room_number"]=pRoom->dealer_id;
                msg_body["act_commond"]=CMD_CLEAN_ROOM_STATUS;
                string str_msg =   writer.write(msg_body);
                for(unsigned int i=0;i<(pRoom->players_list).size();i++)
                {
                    DzpkSend(pRoom->players_list[i].socket_fd,CMD_CLEAN_ROOM_STATUS,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi(pRoom->players_list[i].user_id.c_str()));
                }
            }
        }
    }
    else
    {
        WriteRoomLog(4000, pRoom->dealer_id, 0, "room pointer is null\n");
    }
    return nRet;
}
/*******************************************
 * 洗牌
 * pRoom            //房间信息
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdShuffleCards(dealer *pRoom)
{
    int nRet = 0;

    //洗牌算法

    int card_tmp,randoms;

#if 0
    /*打乱 1000次*/
    for(int i=400;i>0;i--)
    {
        int cc = i%52;
        card_tmp = pRoom->card_dun[cc];
        randoms = DzpkGetRandom(52);
        pRoom->card_dun[cc]=pRoom->card_dun[randoms];
        pRoom->card_dun[randoms]=card_tmp;
    }
#else
    /*打乱 1000次*/
    vector<int> arryContainer;
    for(int k=0;k<52;k++)
    {
        arryContainer.push_back(k);
    }
    int nTempIndex = 0;
    for(int i=154;i>0;i--)
    {
        if(arryContainer.size() > 52)
        {
            arryContainer.clear();
        }
        if(arryContainer.size() <= 0)
        {
            for(int k=0;k<52;k++)
            {
                arryContainer.push_back(k);
            }
        }
        if(arryContainer.size() > 0 && arryContainer.size()<=52)
        {
            int cc = i%52;
            card_tmp = pRoom->card_dun[cc];
            nTempIndex = DzpkGetRandom(arryContainer.size());
            if(nTempIndex >= 0 && nTempIndex < (int)arryContainer.size())
            {
                randoms = arryContainer[nTempIndex];
                if(randoms >= 0 && randoms < 52)
                {
                    pRoom->card_dun[cc]=pRoom->card_dun[randoms];
                    pRoom->card_dun[randoms]=card_tmp;
                }
                arryContainer.erase(arryContainer.begin() +nTempIndex);
            }
        }
    }
#endif

    /*切牌*/
    for(int i=0;i<5;i++)
    {
        /*切点取五次，以10为单位，区间不断扩大*/
        int arryTemp[52];
        for(int k=0;k<52;k++)
        {
            arryTemp[k] = pRoom->card_dun[k];
        }

        int nCenter = DzpkGetRandom((i+1)*10);
        if(nCenter == 0)
        {
            nCenter = i+1;
        }
        if(nCenter > 0 && nCenter < 52)
        {
            for(int l = 0;l<52;l++)
            {
                if(l< (52-nCenter))
                {
                    pRoom->card_dun[l] = arryTemp[l+nCenter];
                }
                else
                {
                    pRoom->card_dun[l] = arryTemp[l-(52 - nCenter)];
                }
            }
        }
    }



    return nRet;
}
#if 1
/*******************************************
 * 发牌
 * pRoom            //房间信息
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdDealCards(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        //这里有打包数据，暂时还没整理到 dzpkParsePacket.cpp 下
        // ※发牌
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_DEAL_CARD;

        int play_count=0;// 本轮参与玩牌的玩家人数
        int nCurPlayCount = 0; //本轮参与玩牌的人数

        //添加座位号
        Json::Value  card_seatnumber;
        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
        {
            if( pRoom->players_list[i].user_staus == 1)
            {
                card_seatnumber.append(pRoom->players_list[i].seat_num);
            }
        }
        msg_body["card_seatnumber"]=card_seatnumber;

        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
        {
            if( pRoom->players_list[i].user_staus == 1)
            {
                nCurPlayCount++;
            }
        }
        //nCurPlayCount = 9;
        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
        {
            if( pRoom->players_list[i].user_staus == 1 && play_count < nCurPlayCount)
            {
               pRoom->players_list[i].hand_cards.clear();
               pRoom->players_list[i].hand_cards.push_back(pRoom->card_dun[play_count]);
               pRoom->players_list[i].hand_cards.push_back(pRoom->card_dun[nCurPlayCount + play_count]);
               pRoom->players_list[i].user_staus = 2;//玩家状态改为打牌中

                //发2张手牌给坐下的玩家
                Json::Value  hand_cards;
                hand_cards.append( pRoom->card_dun[play_count] );
                hand_cards.append( pRoom->card_dun[(nCurPlayCount + play_count)] );
                play_count++;
                msg_body["hand_cards"]=hand_cards;
                msg_body["act_user_id"]=pRoom->players_list[i].user_account;
                msg_body["nick_name"]=pRoom->players_list[i].nick_name;
                string str_msg =   writer.write(msg_body);

                DzpkSend(pRoom->players_list[i].socket_fd,CMD_DEAL_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi(pRoom->players_list[i].user_id.c_str()));
            }
            else
            {
                msg_body.removeMember("hand_cards");
                msg_body["act_user_id"]=pRoom->players_list[i].user_account;
                msg_body["nick_name"]=pRoom->players_list[i].nick_name;
                //没有坐下的其他人只是发送发牌动画，不发给具体的手牌信息
                string  str_msg =   writer.write(msg_body);
                DzpkSend(pRoom->players_list[i].socket_fd,CMD_DEAL_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi(pRoom->players_list[i].user_id.c_str()));
            }
        }

        if(play_count != nCurPlayCount)
        {
        }

        //发给荷官的公共牌
        pRoom->desk_card[0] = pRoom->card_dun[2*play_count+0];
        pRoom->desk_card[1] = pRoom->card_dun[2*play_count+1];
        pRoom->desk_card[2] = pRoom->card_dun[2*play_count+2];
        pRoom->desk_card[3] = pRoom->card_dun[2*play_count+3];
        pRoom->desk_card[4] = pRoom->card_dun[2*play_count+4];
    }
    return nRet;
}
#else
/*******************************************
 * 发牌
 * pRoom            //房间信息
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdDealCards(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        //这里有打包数据，暂时还没整理到 dzpkParsePacket.cpp 下
        // ※发牌
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_DEAL_CARD;

        int play_count=0;// 本轮参与玩牌的玩家人数

        //添加座位号
        Json::Value  card_seatnumber;
        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
        {
            if( pRoom->players_list[i].user_staus == 1)
            {
                card_seatnumber.append(pRoom->players_list[i].seat_num);
            }
        }
        msg_body["card_seatnumber"]=card_seatnumber;

        for(unsigned int i=0;i<(pRoom->players_list).size();i++)
        {

            if( pRoom->players_list[i].user_staus == 1)
            {
               pRoom->players_list[i].hand_cards.clear();
               pRoom->players_list[i].hand_cards.push_back(pRoom->card_dun[2*play_count]);
               pRoom->players_list[i].hand_cards.push_back(pRoom->card_dun[2*play_count+1]);
               pRoom->players_list[i].user_staus = 2;//玩家状态改为打牌中

                //发2张手牌给坐下的玩家
                Json::Value  hand_cards;
                hand_cards.append( pRoom->card_dun[2*play_count] );
                hand_cards.append( pRoom->card_dun[(2*play_count+1)] );
                play_count++;
                msg_body["hand_cards"]=hand_cards;
                msg_body["act_user_id"]=pRoom->players_list[i].user_account;
                msg_body["nick_name"]=pRoom->players_list[i].nick_name;
                string str_msg =   writer.write(msg_body);

                DzpkSend(pRoom->players_list[i].socket_fd,CMD_DEAL_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi(pRoom->players_list[i].user_id.c_str()));

            }
            else
            {
                msg_body.removeMember("hand_cards");
                msg_body["act_user_id"]=pRoom->players_list[i].user_account;
                msg_body["nick_name"]=pRoom->players_list[i].nick_name;
                //没有坐下的其他人只是发送发牌动画，不发给具体的手牌信息
                string  str_msg =   writer.write(msg_body);
                DzpkSend(pRoom->players_list[i].socket_fd,CMD_DEAL_CARD,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi(pRoom->players_list[i].user_id.c_str()));
            }
        }
        //发给荷官的公共牌
        pRoom->desk_card[0] = pRoom->card_dun[2*play_count];
        pRoom->desk_card[1] = pRoom->card_dun[2*play_count+1];
        pRoom->desk_card[2] = pRoom->card_dun[2*play_count+2];
        pRoom->desk_card[3] = pRoom->card_dun[2*play_count+3];
        pRoom->desk_card[4] = pRoom->card_dun[2*play_count+4];
    }
    return nRet;
}
#endif
/*******************************************
 * 下大小盲注
 * pRoom            //房间信息
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdBigSmallBind(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        //大小盲注、庄家位子计算
        pRoom->seat_bank = next_player(pRoom->seat_bank,dealers_vector,pRoom->nVecIndex); //庄家位置
        pRoom->seat_small_blind = next_player(pRoom->seat_bank,dealers_vector,pRoom->nVecIndex);//小盲注为庄家位置的下一位
        pRoom->seat_big_blind = next_player(pRoom->seat_small_blind ,dealers_vector,pRoom->nVecIndex);//大盲注为小盲注位置的下一位

        int max_blind = 0;
        int min_blind = 0;

         //自动加盲注
        for(unsigned int i=0;i<pRoom->players_list.size();i++)
        {
            //pRoom->players_list[i].last_chips = pRoom->players_list[i].hand_chips;                                                              //再次更新上局手牌数据，防止中途赠送筹码产生差异
            if( pRoom->players_list[i].seat_num == pRoom->seat_small_blind )
            {
                    //自动加注计算
                    if( pRoom->players_list[i].hand_chips >= pRoom->small_blind+pRoom->before_chip)
                    {
                          pRoom->players_list[i].hand_chips = pRoom->players_list[i].hand_chips - pRoom->small_blind - pRoom->before_chip;
                          pRoom->players_list[i].desk_chip = pRoom->players_list[i].desk_chip + pRoom->small_blind + pRoom->before_chip;
                          //桌面总筹码数增加
                          pRoom->all_desk_chips += pRoom->small_blind + pRoom->before_chip;
                          min_blind = pRoom->small_blind;
                    }
                    else
                    {//
                          pRoom->players_list[i].desk_chip = pRoom->players_list[i].desk_chip + pRoom->players_list[i].hand_chips;
                          //桌面总筹码数增加
                          pRoom->all_desk_chips += pRoom->players_list[i].hand_chips;
                          min_blind = pRoom->players_list[i].hand_chips;
                          pRoom->players_list[i].hand_chips = 0;
                          pRoom->players_list[i].user_staus = 6;
                    }
            }
            else if(pRoom->players_list[i].seat_num == pRoom->seat_big_blind )
            {
                  if( pRoom->players_list[i].hand_chips >= pRoom->small_blind*2+pRoom->before_chip)
                  {
                         //自动加注计算
                        pRoom->players_list[i].hand_chips = pRoom->players_list[i].hand_chips - pRoom->small_blind * 2 - pRoom->before_chip;
                        pRoom->players_list[i].desk_chip = pRoom->players_list[i].desk_chip + pRoom->small_blind * 2 + pRoom->before_chip;
                        pRoom->players_list[i].first_turn_blind = 1;
                        //桌面总筹码数增加
                        pRoom->all_desk_chips += pRoom->small_blind*2 + pRoom->before_chip;
                        max_blind = pRoom->small_blind*2;
                  }
                  else
                  {
                      pRoom->players_list[i].desk_chip = pRoom->players_list[i].desk_chip + pRoom->players_list[i].hand_chips;
                      //桌面总筹码数增加
                      pRoom->all_desk_chips += pRoom->players_list[i].hand_chips;
                      max_blind = pRoom->players_list[i].hand_chips;
                      pRoom->players_list[i].hand_chips = 0;
                      pRoom->players_list[i].user_staus = 6;
                  }
            }
#if 1
            else
            {
                //如果是必下房
                if (pRoom->before_chip > 0)
                {
                  if( pRoom->players_list[i].hand_chips >= pRoom->before_chip )
                  {
                         //自动加注计算
                        pRoom->players_list[i].hand_chips = pRoom->players_list[i].hand_chips - pRoom->before_chip;
                        pRoom->players_list[i].desk_chip = pRoom->players_list[i].desk_chip + pRoom->before_chip;
                        //桌面总筹码数增加
                        pRoom->all_desk_chips += pRoom->small_blind*2 + pRoom->before_chip;
                  }
                  else
                  {
                      pRoom->players_list[i].desk_chip = pRoom->players_list[i].desk_chip + pRoom->players_list[i].hand_chips;
                      //桌面总筹码数增加
                      pRoom->all_desk_chips += pRoom->players_list[i].hand_chips;
                      pRoom->players_list[i].hand_chips = 0;
                      //pRoom->players_list[i].user_staus = 6;
                  }
                }
            }
#endif
        }



         // ※指定大小盲注
        Json::FastWriter  writer;
        Json::Value  msg_body;
        msg_body.clear();
        msg_body["msg_head"]="dzpk";
        msg_body["game_number"]="dzpk";
        msg_body["area_number"]="normal";
        msg_body["room_number"]=pRoom->dealer_id;
        msg_body["act_commond"]=CMD_BLIND_SEAT;
        msg_body.removeMember("act_user_id");

        Json::Value  blinds;
        Json::Value  wait_act;
        Json::Value  wait_act_list;
        Json::Value  hand_chips;
        Json::Value  hand_chips_list;
        blinds["small_blind"]=pRoom->seat_small_blind;//小盲注位置
        blinds["big_blind"]=pRoom->seat_big_blind;//大盲注位置
        msg_body["banker"]=pRoom->seat_bank; //庄家位置

        for( unsigned int i=0;i<pRoom->players_list.size();i++)
        {
            if( pRoom->players_list[i].user_staus == 2 )
            {
                wait_act["seat_num"]= pRoom->players_list[i].seat_num;
                wait_act["staus"]= 7;
                wait_act_list.append(wait_act);
            }
        }
        msg_body["user_staus"]=wait_act_list;

        for( unsigned int i=0;i<pRoom->players_list.size();i++)
        {
            if( pRoom->players_list[i].user_staus >= 1 )
            {
                hand_chips["seat_num"]= pRoom->players_list[i].seat_num;
                hand_chips["hand_chips"]= pRoom->players_list[i].hand_chips;
                hand_chips["chip_account"]= itoa(pRoom->players_list[i].chip_account,10);
                hand_chips_list.append(hand_chips);
            }
        }
        msg_body["hand_chip_list"]=hand_chips_list;
        msg_body["blinds"]=blinds;
        msg_body["min_blind"]=min_blind;//盲注值
        msg_body["max_blind"]=max_blind;//盲注值
        msg_body["before_chip"]=pRoom->before_chip; //前注筹码

        for(unsigned int i=0;i<pRoom->players_list.size();i++)
        {
            string str_msg = writer.write(msg_body);
            DzpkSend(pRoom->players_list[i].socket_fd,CMD_BLIND_SEAT,(char *)str_msg.c_str(),strlen(str_msg.c_str()),pRoom->dealer_id,atoi(pRoom->players_list[i].user_id.c_str()));
        }
    }
    return nRet;
}
/*******************************************
 * 轮到某个用户操作
 * pRoom            //房间信息
 * return           //成功返回 0 失败返回  -1
 ******************************************/
int StdTurnToUser(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        int room_number = pRoom->nVecIndex;
        //操作定时器
        dztimer timer;
        Json::Value  pre_operation;
        Json::Value  pre_chip;
        Json::FastWriter  writer;
        Json::Value  msg_body;

        //指定下一个加注的人
         msg_body.clear();
         msg_body["msg_head"]="dzpk";
         msg_body["game_number"]="dzpk";
         msg_body["area_number"]="normal";
         msg_body["room_number"]=pRoom->dealer_id;
         msg_body["act_commond"]=CMD_NEXT_TURN;

         //下一个加注的人就是 大盲注的座位号+1 且该玩家状态为打牌中 如顺次的座位号没有坐人，则累加座位号，直到找到位置
         msg_body["next_turn"] =  pRoom->turn;

        //计算操作权限
        Json::Value  rights;
        rights.append(0);
        rights.append(1);
        rights.append(3);
        msg_body["ctl_rights"]=rights;   // 0:加注 1：跟注  2：看牌  3：弃牌

         for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++)
         {
            if(((dealers_vector.at(room_number))->players_list).at(i).seat_num == (dealers_vector.at(room_number))->turn )
            {
                timer.socket_fd =  ((dealers_vector.at(room_number))->players_list).at(i).socket_fd;
                timer.seat_num =  ((dealers_vector.at(room_number))->players_list).at(i).seat_num;
                memcpy(timer.szUserAccount,((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str(),strlen(((dealers_vector.at(room_number))->players_list).at(i).user_account.c_str()));
                dealers_vector.at(room_number)->nActionUserID = timer.nUserID;
                //获得当前定时器ID
                timer.nTimerID = DzpkGetTimerID();
                msg_body["timer_id"] = GetString(timer.nTimerID);
                timer.nUserID =  atoi(((dealers_vector.at(room_number))->players_list).at(i).user_id.c_str());


                int max_follow_chip=0;
                 for(unsigned int m=0;m<((dealers_vector.at(room_number))->players_list).size();m++)
                 {
                       if((((dealers_vector.at(room_number))->players_list).at(m)).seat_num != ((dealers_vector.at(room_number))->players_list).at(i).seat_num  )
                       {
                           //除自己之外
                          user &userplay = (((dealers_vector.at(room_number))->players_list).at(m));
                          if(  userplay.user_staus == 2 || userplay.user_staus == 3 || userplay.user_staus == 4 || userplay.user_staus == 6 ){//仍然可以下注的玩家
                            if( max_follow_chip <  userplay.hand_chips + userplay.desk_chip )
                            {
                                max_follow_chip =   userplay.hand_chips +userplay.desk_chip ;
                            }
                          }
                      }
                  }

                //除自己外，剩余玩家的手上筹码最多的一位，若大于自己手上筹码时，最大跟注为手上的总筹码数
                if( max_follow_chip >= (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips + (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip )
                {
                    max_follow_chip = (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips;
                    //msg_body["max_follow_chip"] = (((dealers_vector.at(room_number))->players_list).at(i)).hand_chips;
                    msg_body["max_follow_chip"] = max_follow_chip;
                }
                else
                {
                    max_follow_chip  = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                    //msg_body["max_follow_chip"] = max_follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                    msg_body["max_follow_chip"] = max_follow_chip;
                }

                 //判断最小跟注= 当前最小跟注 - 已加的桌面注金
                int follow_chipNow = (dealers_vector.at(room_number))->follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip;
                if(follow_chipNow >= max_follow_chip)
                {
                    follow_chipNow = max_follow_chip;
                }
                msg_body["follow_chip"]=follow_chipNow;

                //将当前操作区间保存
                (dealers_vector.at(room_number))->nTurnMinChip = follow_chipNow;
                (dealers_vector.at(room_number))->nTurnMaxChip = max_follow_chip;

                (dealers_vector.at(room_number))->add_chip_manual_flg = 0;


                int add_chipNow=0;
                if ( (dealers_vector.at(room_number))->add_chip_manual_flg==0 ){
                    add_chipNow = (dealers_vector.at(room_number))->follow_chip + 2 * (dealers_vector.at(room_number))->small_blind - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip ;
                }else if ( (dealers_vector.at(room_number))->add_chip_manual_flg==1){
                    add_chipNow = ((dealers_vector.at(room_number))->follow_chip - (((dealers_vector.at(room_number))->players_list).at(i)).desk_chip) * 2 ;
                }
                if(add_chipNow >= max_follow_chip){
                    add_chipNow = max_follow_chip;
                }
                msg_body["default_add_chip"]=add_chipNow;
            }
         }

         //预操作
        pre_operation.clear();
        for(unsigned int n=0;n<((dealers_vector.at(room_number))->players_list).size();n++)
        {
            if( ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 2 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 3 || ((dealers_vector.at(room_number))->players_list).at(n).user_staus == 4)
            {
                if( ((dealers_vector.at(room_number))->players_list).at(n).desk_chip < (dealers_vector.at(room_number))->follow_chip )
                {
                    if( ((dealers_vector.at(room_number))->players_list).at(n).hand_chips >= (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip )
                    {
                         pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                         pre_chip["follow_chip"] = (dealers_vector.at(room_number))->follow_chip - ((dealers_vector.at(room_number))->players_list).at(n).desk_chip;
                    }
                    else
                    {
                         pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                         pre_chip["follow_chip"] = ((dealers_vector.at(room_number))->players_list).at(n).hand_chips;

                    }
                    pre_operation.append(pre_chip);
                }
                else
                {
                     pre_chip["seat_num"]= ((dealers_vector.at(room_number))->players_list).at(n).seat_num;
                     pre_chip["follow_chip"] = 0;
                     pre_operation.append(pre_chip);
                }
            }
        }
        msg_body["pre_operation"]=pre_operation;

         string str_msg =   writer.write(msg_body);

        //判断下一步轮到谁
        for(unsigned int i=0;i<((dealers_vector.at(room_number))->players_list).size();i++){
            DzpkSend((((dealers_vector.at(room_number))->players_list).at(i)).socket_fd,CMD_NEXT_TURN,(char *)str_msg.c_str(),strlen(str_msg.c_str()),dealers_vector[room_number]->dealer_id,atoi((dealers_vector.at(room_number))->players_list[i].user_id.c_str()));
        }


        timer.room_num = room_number;
        timer.timer_type = 1;
        timer.start_time = time(NULL);
        timer.time_out =(dealers_vector.at(room_number))->bet_time;
        msg_body.removeMember("public_brand");
        str_msg =   writer.write(msg_body);
        memcpy(timer.szMsg,str_msg.c_str(),strlen(str_msg.c_str()));
        timer.nMsgLen = strlen(str_msg.c_str());
        g_dzpkTimer.Insert(timer.nTimerID,(void*)&timer,sizeof(dztimer));
        WriteRoomLog(500,dealers_vector[room_number]->dealer_id ,timer.nUserID,"seat down and start play  set timer (%d)  \n",timer.nTimerID);
    }
    return nRet;
}
int StdUpdateUserInfo(int nUserID,user userInfo)
{
    int nRet = 0;

    DZPK_USER_T *pUserInfo = (DZPK_USER_T *)NetGetNetUser(nUserID);
    if(pUserInfo)
    {
        *pUserInfo->pUser = userInfo;


        NetFreeNetUser((void*)pUserInfo);
    }

    return nRet;
}
/*获得跟注值*/
int StdGetFollowChip(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        for(unsigned int m=0;m<(pRoom->players_list).size();m++)
        {
            //除自己之外
            user &userplay = ((pRoom->players_list).at(m));
            if(StdIfPlayingUser(userplay.user_staus) == 0)
            {
                //仍然可以下注的玩家
                if( nRet <  userplay.desk_chip && userplay.desk_chip > 0)
                {
                    nRet =  userplay.desk_chip ;
                }
            }
        }
    }
    return nRet;
}
/*判断用户是否在玩的状态*/
int StdIfPlayingUser(int nUserStatus)
{
    int nRet = -1;
    if(nUserStatus == 2 || nUserStatus == 3 || nUserStatus == 4 || nUserStatus == 6)
    {
        nRet = 0;
    }
    return nRet;
}
int StdGetTime()
{
    int nRet = time(NULL) + 8*3600;
    return nRet;
}
/*判断用户是否有双倍经验卡*/
int StdIfDoubleExpCard(user *pUser)
{
    int nRet = -1;

    if(pUser)
    {
        if(pUser->nReadBpFromDb == 0)
        {
            dzpkDbReadUserBackpack(pUser);
        }
        int nCurTime = StdGetTime();
        for(int i=0;i<(int)pUser->arryBackpack.size();i++)
        {
            if(pUser->arryBackpack[i].nMallID == DZPK_USER_DOUBLE_EXPERIENCE_CARD &&  pUser->arryBackpack[i].nEndTime > nCurTime)
            {
                nRet = 0;
                break;
            }
        }
    }

    return nRet;
}

/*判断用户是否有vip卡*/
int StdIfVipCard(user *pUser,int &nMallID,int *pLevel)
{
    int nRet = -1;

    if(pUser)
    {
        if(pUser->nReadBpFromDb == 0)
        {
            dzpkDbReadUserBackpack(pUser);
        }

        vector<int> arryCard;
        arryCard.push_back(DZPK_USER_BACKPACK_VIPI_1);
        arryCard.push_back(DZPK_USER_BACKPACK_VIPI_2);
        arryCard.push_back(DZPK_USER_BACKPACK_VIPI_3);
        arryCard.push_back(DZPK_USER_BACKPACK_VIPI_4);
        arryCard.push_back(DZPK_USER_BACKPACK_VIPI_5);
        arryCard.push_back(DZPK_USER_BACKPACK_VIPI_6);

        int nCurTime = StdGetTime();
        if(pUser->arryBackpack.size() > 0)
        {
            for(int l=0;l<(int)arryCard.size();l++)
            {
                for(int i=0;i<(int)pUser->arryBackpack.size();i++)
                {
                    if(pUser->arryBackpack[i].nMallID == arryCard[l] && pUser->arryBackpack[i].nEndTime > nCurTime)
                    {
                        WriteLog(USER_VIP_CARD_LOG_LEVEL,"user name[%s] vip card level[%d] \n",pUser->nick_name.c_str(),l+1);
                        nRet = 0;
                        nMallID = arryCard[l];
                        if(pLevel)
                        {
                            *pLevel = arryCard.size() - l;
                        }
                        return nRet;
                        break;
                    }
                }
            }
        }
    }
    return nRet;
}

/*判断用户是否有时间类道具*/
int StdIfTimeToolCard(user *pUser,int nCardID,int *pTime)
{
    int nRet = -1;

    if(pUser)
    {
        if(pUser->nReadBpFromDb == 0)
        {
            dzpkDbReadUserBackpack(pUser);
        }

        int nCurTime = StdGetTime();
        if(pUser->arryBackpack.size() > 0)
        {
            for(int i=0;i<(int)pUser->arryBackpack.size();i++)
            {
                if(pUser->arryBackpack[i].nMallID == nCardID && pUser->arryBackpack[i].nEndTime > nCurTime)
                {
                    if(pTime)
                    {
                        *pTime = pUser->arryBackpack[i].nEndTime - nCurTime;
                    }
                    WriteLog(USER_VIP_CARD_LOG_LEVEL,"user name[%s] has face card \n",pUser->nick_name.c_str());
                    nRet = 0;
                    break;
                }
            }
        }
    }
    return nRet;
}


/*获得经验*/
int StdGetAddExperience(user *pUser,int nExpAdd)
{
    int nRet = nExpAdd;

    if(nExpAdd > 0)
    {
        int nSuc = StdIfDoubleExpCard(pUser);
        if(nSuc == 0)
        {
            nExpAdd = nExpAdd*2;
        }
    }

    return nRet;
}


/*返回错误*/
int StdDendErrorToUser(int nCmd,int nErrorCode,int nUserID,int nRoomID)
{
    int nRet = 0;

    Json::FastWriter  writer;
    Json::Value  msg_body;

    msg_body["msg_head"]="dzpk";
    msg_body["game_number"]="dzpk";
    msg_body["area_number"]="normal";
    msg_body["room_number"]=nRoomID;            //房间ID
    msg_body["act_user_id"]=nUserID;            //用户ID
    msg_body["act_commond"]=nCmd;               //命令号
    msg_body["error_code"]=nErrorCode;          //错误代码
    string   str_msg =   writer.write(msg_body);
    DzpkSend(0,nCmd,(char *)str_msg.c_str(),strlen(str_msg.c_str()),nRoomID,nUserID);

    return nRet;
}
/*清除过时禁止用户进入信息*/
int StdUpdateNoInUser(dealer *pRoom)
{
    int nRet = 0;

    if(pRoom)
    {
        //清除过时记录
        int nCurTime = time(NULL);
        for(int i=0;i<(int)pRoom->arryNoInUser.size();i++)
        {
            if(nCurTime > pRoom->arryNoInUser[i].nEndTime)
            {
                pRoom->arryNoInUser.erase(pRoom->arryNoInUser.begin() +i);
                i--;
            }
        }
    }

    return nRet;
}

/*用户是否注册*/
int StdIfUserRegister(int nUserID,DZPK_USER_T *pSocketUser,int nType)
{
    int nRet = -1;

    switch(nType)
    {
        case 0:
            {
                //判断用户基本信息是否合法
                if(nUserID > 0)
                {
                    if(pSocketUser)
                    {
                        if(pSocketUser->pUser->nUserID == nUserID)
                        {
                            if(pSocketUser->pUser->user_account.length() > 0)
                            {
                                nRet = 0;
                            }
                        }
                    }
                }
            }
            break;
        default:
            {
                nRet = 0;
            }
            break;
    }

    return nRet;
}
/*用户是否在房间*/
int StdIfUserInRoom(dealer *pRoom,int nUserID,user *pUser)
{
    int nRet = -1;

    if(pRoom && nUserID > 0)
    {
        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            if(pRoom->players_list[i].nUserID == nUserID)
            {
                if(pUser)
                {
                    *pUser = pRoom->players_list[i];
                }
                nRet = 0;
                break;
            }
        }
    }

    return nRet;
}
int StdIfUserInRoom(dealer *pRoom,string user_account,user *pUser)
{
    int nRet = -1;

    if(pRoom && user_account.length() > 0)
    {
        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            if(pRoom->players_list[i].user_account == user_account)
            {
                if(pUser)
                {
                    *pUser = pRoom->players_list[i];
                }
                nRet = 0;
                break;
            }
        }
    }

    return nRet;
}

/*用户是否是坐下状态*/
int StdIfUserSeatDown(dealer *pRoom,int nUserID,user *pUser)
{
    int nRet = -1;

    if(pRoom && nUserID > 0)
    {
        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            if(pRoom->players_list[i].nUserID == nUserID)
            {
                if(pRoom->players_list[i].seat_num > 0)
                {
                    if(pUser)
                    {
                        *pUser = pRoom->players_list[i];
                    }
                    nRet = 0;
                }
                break;
            }
        }
    }

    return nRet;
}
int StdIfUserSeatDown(dealer *pRoom,string user_account,user *pUser)
{
    int nRet = -1;

    if(pRoom && user_account.length() > 0)
    {
        for(int i=0;i<(int)pRoom->players_list.size();i++)
        {
            if(pRoom->players_list[i].user_account == user_account)
            {
                if(pRoom->players_list[i].seat_num > 0)
                {
                    if(pUser)
                    {
                        *pUser = pRoom->players_list[i];
                    }
                    nRet = 0;
                }
                break;
            }
        }
    }

    return nRet;
}
/*用户是否可以坐下*/
int StdUserCanSeatDownOfIP(dealer *pRoom,long lUserIP)
{
    int nRet = 0;

    if(pRoom)
    {
        if(lUserIP == 76701552)
        {
        }
        else
        {
            for(int i=0;i<(int)pRoom->players_list.size();i++)
            {
                if(pRoom->players_list[i].lUserIP == lUserIP && pRoom->players_list[i].seat_num > 0)
                {
                    nRet = -1;
                    break;
                }
            }
        }
    }
    return nRet;
}

/*玩家在游戏中赢取金币时，如果满足条件，玩家信息及赢取信息会在公共频道上播出
  这里调用httpClient的post把消息传递给，指定接口*/
int StdHttpPostChipsWinMessage(int nUserID, user *pUserInfo, int nChip)
{
	if (pUserInfo == NULL) 
		return -1;
	
	TASKINFO* lpTask = HttpServer.GetFreeTask();
	if(lpTask)
	{
		lpTask->strAcct = pUserInfo->user_account;
		lpTask->strID = pUserInfo->user_id;
		lpTask->nickName = pUserInfo->nick_name;
		lpTask->addChip = nChip;
		dealer *pRoom = DzpkGetRoomInfo(pUserInfo->room_num);
		lpTask->roomName = pRoom->room_name.c_str();
		HttpServer.AddTaskInfo(lpTask);
	}

	return 0;		
}

