/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.5.2
** 版本：1.0
** 说明：德州扑克模块所有数据库操作
**************************************************************
*/
#include "dzpkDb.h"
#include "db_connect_pool.h"
#include "../Start.h"
#include "../standard/dzpkCustom.h"

#include <map>

using namespace  std;
extern Log cLog;

/***********************************
 * 获得数据库用户信息
 * newuser          //用户信息
 * pUserAccount     //用户帐号
 * nSocketID        //用户对应的socket
 * nRoomID          //用户要进的房间
 * return           //成功返回0，失败返回-1
 ***********************************/
int dzpkGetDbUserInfo(user &newuser, char *pUserAccount, int nSocketID, int nRoomID)
{
	int nRet = -1;

	if (pUserAccount && strlen(pUserAccount) > 0)
	{
		newuser.user_account = pUserAccount;               //用户帐号
		newuser.socket_fd = nSocketID;                       //用户对应socket
		newuser.desk_chip = 0;                              //
		newuser.user_staus = 0;                             //用户状态  0表示旁观
		newuser.room_num = nRoomID;                         //连接状态正常
		newuser.seat_num = 0;                             //座位号 没有坐下设为0
		newuser.win_cards.clear();
		newuser.hand_cards.clear();
		newuser.hand_chips = 0;
		newuser.last_chips = 0;
		newuser.auto_buy = 0;
		newuser.max_buy = 0;
		newuser.give_up_count = 0;
		newuser.disconnect_flg = 1;                         //连接状态正常
		newuser.first_turn_blind = 0;                       //连接状态正常
		newuser.countdown_no = 0;                 //倒计时宝箱编号
		newuser.cd_rest_time = 0;                   //该倒计时宝箱领取剩余时间
		newuser.start_countdown = 0;              //该倒计时宝箱开始计时的时间点
		newuser.nTodayPlayTimes = 0;              //该倒计时宝箱开始计时的时间点
		//newuser.nEnterTime = StdGetTime();        //用户进入时间
		newuser.nEnterTime = -1;                //用户进入时间
		newuser.nReadBpFromDb = 0;
		newuser.nReadTaskFromDb = 0;
		newuser.nVipLevel = 0;                   //vip等级

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "select t1.user_id, t1.userstate,t1.user_account,t1.nick_name,t1.head_photo_serial,t2.sex,t2.dzpk_level,t2.dzpk_experience,t1.vip_level,t1.level_gift,t1.countdown,t1.today_playtimes,t1.level_count,t1.chip_account,t1.coin_account,t2.city,t2.total_time,t2.win_per,t2.rank_name,t2.terminal_code,t2.last_ip_address,t1.giftid,t5.mall_path,t4.level_title from tbl_user_mst t1 left join tbl_userinfo_mst t2 on t1.user_id=t2.user_id left join tbl_userpacket_mst t3 on t1.giftid= t3.id left join tbl_level_exp_mst t4 on t2.dzpk_level = t4.level left join tbl_mallbase_mst t5 on t3.mall_id = t5.mall_id where user_account='";
				sql = sql + newuser.user_account + "'";
				sShow = sql;
				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					newuser.user_id = itoa(result->getInt("user_id"), 10);
					newuser.nUserID = atoi(newuser.user_id.c_str());
					newuser.user_account = result->getString("user_account");
					newuser.nick_name = result->getString("nick_name");
					newuser.head_photo_serial = result->getString("head_photo_serial");
					newuser.dzpk_level = result->getInt("dzpk_level");
					string sdzpk_experience = result->getString("dzpk_experience");
					newuser.dzpk_experience = atol(sdzpk_experience.c_str());
					if (result->getString("sex") == "male" || result->getString("sex") == "男")
					{
						newuser.sex = 1;
					}
					else
					{
						newuser.sex = 0;
					}
					newuser.vip_level = result->getInt("vip_level");
					newuser.level_gift = result->getInt("level_gift");
					string schip_account = (result->getString("chip_account"));
					newuser.chip_account = atol(schip_account.c_str());
					string scoin_account = (result->getString("coin_account"));
					newuser.coin_account = atol(scoin_account.c_str());
					newuser.city = result->getString("city");
					newuser.total_time = result->getInt("total_time");
					newuser.win_per = result->getInt("win_per");
					newuser.rank_name = result->getString("rank_name");
					newuser.terminal_code = result->getString("terminal_code");
					newuser.last_ip = result->getString("last_ip_address");
					newuser.giftid = result->getInt("giftid");
					newuser.mall_path = result->getString("mall_path");
					newuser.level_title = result->getString("level_title");
					newuser.countdown_no = result->getInt("level_count"); //倒计时宝箱编号
					newuser.cd_rest_time = result->getInt("countdown"); //领取倒计时宝箱剩余时间 (秒)
					newuser.nTodayPlayTimes = result->getInt("today_playtimes"); //今天玩多少盘

					//获得用户的vip等级
					int nMallID = 0;
					StdIfVipCard(&newuser, nMallID, &newuser.nVipLevel);

					nRet = 0;
					if ( result->getInt("userstate") != 0 )
					{
						nRet = 2;
					}
					break;
				}
				if (result)
				{
					delete result;
					result = NULL;
				}

				if (newuser.nUserID > 0)
				{
					//更新用户所在的房间号到session中
					state->execute("use dzpk");                         //使用指定的数据库
					sql = "update tbl_session_mst set roomnum = " + GetString(nRoomID);
					sql = sql + " where user_id = " + newuser.user_id;
					// UPDATE
					sShow = sql;
					state->execute(sql);
				}

				if (nRet != 0)
				{
					WriteLog(400000, "dzpkGetDbUserInfo sql error :[%s] \n", sql.c_str());
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}
	else
	{
		WriteLog(400000, "dzpkGetDbUserInfo account null:[%s] \n", pUserAccount);
	}

	return nRet;
}





/***********************************
 * 获得数据库用户等级礼包信息
 * lgifts          //等级礼包信息
 * user_id         //用户id
 * return          //成功返回0，失败返回-1
 ***********************************/
int dzpkGetDbLevelGifts(level_gifts &lgifts, string user_id)
{
	int nRet = -1;

	if (strlen(user_id.c_str()) > 0)
	{

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "select t2.level,t2.chip_award,t2.coin_award,t2.gift_award,t2.face_award,t2.vip_award,t2.stone_award from tbl_user_mst t1 inner join tbl_level_gift_mst t2 on t1.level_gift = t2.level  where t1.user_id =";
				sql = sql + user_id;
				sShow = sql;
				WriteLog(4010, " dzpkGetDbLevelGifts: %s  \n", sql.c_str());

				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					lgifts.level = result->getInt("level");
					lgifts.chip_award = result->getString("chip_award");
					lgifts.coin_award = result->getString("coin_award");
					lgifts.gift_award = result->getString("gift_award");
					lgifts.face_award = result->getString("face_award");
					lgifts.vip_award = result->getString("vip_award");
					lgifts.stone_award = result->getString("stone_award");

					nRet = 0;
					break;
				}
				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}





/***********************************
 * 更新数据库用户等级礼包信息
 * lgifts          //等级礼包信息
 * user_id         //用户id
 * return          //成功返回0，失败返回-1
 ***********************************/
int dzpkUpdateDbLevelGifts(string user_id, long chip_account, int level_gift)
{
	int nRet = -1;

	if (strlen(user_id.c_str()) > 0)
	{

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "update tbl_user_mst set chip_account = " + GetString(chip_account);
				sql = sql + ", level_gift = " + GetString(level_gift);
				sql = sql + " where user_id = " + user_id;
				WriteLog(40102, " dzpkUpdateDbLevelGifts: %s  \n", sql.c_str());
				sShow = sql;
				// UPDATE
				state->execute(sql);
				nRet = 0;

				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}





/***********************************
 * 更新数据库用户倒计时宝箱
 * chip_awards          //当前倒计时奖励筹码
 * user_id         //用户id
 * next_cntid         //下一个倒计时宝箱编号
 * next_rest_time         //下一个需要等待的时长
 * return          //成功返回0，失败返回-1
 ***********************************/
int dzpkGetDbCountDown(string& chip_awards, string user_id, int&  next_cntid, int& next_rest_time)
{
	int nRet = -1;

	if (strlen(user_id.c_str()) > 0)
	{

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				int nCurLevel = 1;
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "select t1.user_id,t1.level_count,t2.countdown_time,t2.chip_award from tbl_user_mst t1 left join tbl_countdown_mst t2 on t1.level_count = t2.cd_no where t1.user_id=";
				sql = sql + user_id;
				sShow = sql;

				WriteLog(4008, "countdown:[%s]  \n", sql.c_str());
				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					chip_awards = result->getString("chip_award");
					nCurLevel = result->getInt("level_count");
					nRet = 0;
					break;
				}
				if (result)
				{
					delete result;
					result = NULL;
				}
				string sqlu = "update tbl_user_mst set level_count=level_count+1 where user_id =";
				sqlu = sqlu + user_id;
				sqlu = sqlu + " and level_count<=7 ";
				sShow = sqlu;
				state->execute(sqlu);

				sShow = sql;
				ResultSet *result1 = state->executeQuery(sql);

				// 输出查询
				while (result1->next())
				{
					next_cntid = result1->getInt("level_count");
					next_rest_time = result1->getInt("countdown_time");
					nRet = 0;
					break;
				}

				if (result1)
				{
					delete result1;
					result1 = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}


	return nRet;
}

int dzpkDbGetTimeToIn(string sTime)
{
	int nRet = 0;
	string sHour, sMinute, sSecond, sTimeTemp;

	int index = sTime.find(":");
	if (index >= 0)
	{
		sHour = sTime.substr(0, index);

		sTimeTemp = sTime.substr(index + 1);

		int index = sTimeTemp.find(":");
		if (index >= 0)
		{
			sMinute = sTimeTemp.substr(0, index);
			sSecond = sTimeTemp.substr(index + 1);
		}

		nRet = GetInt(sHour) * 60 * 60 + GetInt(sMinute) * 60 + GetInt(sSecond);
	}


	return nRet;
}

/*获得时间秒数*/
int dzpkDbGetTimeDayToInt(string sDayTime)
{
	int nRet = 0;

	string sYear, sMonth, sDay, sHour, sMinute, sSecond, sTimeTemp;

	int index = sDayTime.find(" ");
	if (index >= 0)
	{
		string sTime = sDayTime.substr(index + 1);
		string sTimeYear = sDayTime.substr(0, index);

		index = sTimeYear.find("-");
		if (index >= 0)
		{
			sYear = sTimeYear.substr(0, index);

			sTimeTemp = sTimeYear.substr(index + 1);

			int index = sTimeTemp.find("-");
			if (index >= 0)
			{
				sMonth = sTimeTemp.substr(0, index);
				sDay = sTimeTemp.substr(index + 1);

				index = sTime.find(":");
				if (index >= 0)
				{
					sHour = sTime.substr(0, index);

					sTimeTemp = sTime.substr(index + 1);

					int index = sTimeTemp.find(":");
					if (index >= 0)
					{
						sMinute = sTimeTemp.substr(0, index);
						sSecond = sTimeTemp.substr(index + 1);
					}

					tm utc_time;
					utc_time.tm_year = GetInt(sYear) - 1900;
					utc_time.tm_mon = GetInt(sMonth) - 1;
					utc_time.tm_mday = GetInt(sDay);
					utc_time.tm_hour = GetInt(sHour);
					utc_time.tm_min = GetInt(sMinute);
					utc_time.tm_sec = GetInt(sSecond);
					utc_time.tm_isdst = 0;
					nRet = mktime(&utc_time) + 8 * 3600;
				}
			}
		}
	}

	return nRet;
}
/*读每天活动*/
int dzpkGetDbEveryDay(CPS_RECORD_DB_HEAD_T *pHead)
{
	int nRet = 0;

	if (pHead)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "SELECT id,type,cycle,gamename,gameimg,starttime,startnum,gamefee,smallblind,handchip,addblind,addblindtime,rankid FROM tbl_gametype_mst WHERE cycle = 0";
				sShow = sql;

				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					CPS_RECORD_DB_T *pRecord = new CPS_RECORD_DB_T();
					if (pRecord)
					{
						string sGameName, sStartTime;

						pRecord->nRecordID = result->getInt("id");
						pRecord->nRecordType = result->getInt("type");
						sGameName = result->getString("gamename");
						sStartTime = result->getString("starttime");
						pRecord->nStartNum = result->getInt("startnum");
						pRecord->nGameFee = result->getInt("gamefee");
						pRecord->nSmallBlind = result->getInt("smallblind");
						pRecord->nHandChip = result->getInt("handchip");
						pRecord->nAddBlindTime = result->getInt("addblindtime");
						pRecord->nRankID = result->getInt("rankid");

						memcpy(pRecord->szGameName, sGameName.c_str(), strlen(sGameName.c_str()));
						pRecord->nStartTime = dzpkDbGetTimeToIn(sStartTime);

						if (pHead->pLast == NULL)
						{
							pHead->pFirst = pRecord;
							pHead->pLast = pRecord;
						}
						else
						{
							pHead->pLast->pNext = pRecord;
							pHead->pLast = pRecord;
						}

						CPS_RECORD_DB_T *pRecord1 = new CPS_RECORD_DB_T();
						if (pRecord1)
						{
							memcpy(pRecord1, pRecord, sizeof(CPS_RECORD_DB_T));
							pRecord1->nStartTime = pRecord->nStartTime + 60 * 60 * 24;
							if (pHead->pLast == NULL)
							{
								pHead->pFirst = pRecord1;
								pHead->pLast = pRecord1;
							}
							else
							{
								pHead->pLast->pNext = pRecord1;
								pHead->pLast = pRecord1;
							}
						}
					}
					else
					{
						break;
					}
				}

				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}
int dzpkGetDbAddBlind(CPS_ADD_BLIND_DB_HEAD_T *pHead)
{
	int nRet = 0;

	if (pHead)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "SELECT id,type,NAME,VALUE FROM tbl_gameaddchip_mst ORDER BY VALUE asc";
				sShow = sql;

				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					CPS_ADD_BLIND_DB_T *pRecord = new CPS_ADD_BLIND_DB_T();
					if (pRecord)
					{
						string sName;

						pRecord->nID = result->getInt("id");
						pRecord->nType = result->getInt("type");
						pRecord->nValue = result->getInt("VALUE");
						sName = result->getString("NAME");

						memcpy(pRecord->szName, sName.c_str(), strlen(sName.c_str()));

						if (pHead->pLast == NULL)
						{
							pHead->pFirst = pRecord;
							pHead->pLast = pRecord;
						}
						else
						{
							pHead->pLast->pNext = pRecord;
							pHead->pLast = pRecord;
						}
					}
					else
					{
						break;
					}
				}

				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}
	return nRet;
}
int dzpkGetDbAward(CPS_AWARD_DB_HEAD_T *pHead)
{
	int nRet = 0;

	if (pHead)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "SELECT id,rankname,rankid,TYPE,rank,reward,state FROM tbl_gamerank_mst";
				sShow = sql;

				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				int nCurIndex = 0;
				while (result->next())
				{
					nCurIndex++;
					CPS_AWARD_DB_T *pRecord = new CPS_AWARD_DB_T();
					if (pRecord)
					{
						string sRankName, sReward;

						pRecord->nId = result->getInt("id");
						sRankName = result->getString("rankname");
						pRecord->nRankID = result->getInt("rankid");
						pRecord->nRank = result->getInt("rank");
						pRecord->nType = result->getInt("TYPE");
						sReward = result->getString("reward");
						pRecord->nStatue = result->getInt("state");

						memcpy(pRecord->szRankName, sRankName.c_str(), strlen(sRankName.c_str()));
						memcpy(pRecord->szReward, sReward.c_str(), strlen(sReward.c_str()));

						if (pHead->pLast == NULL)
						{
							pHead->pFirst = pRecord;
							pHead->pLast = pRecord;
						}
						else
						{
							pHead->pLast->pNext = pRecord;
							pHead->pLast = pRecord;
						}
					}
					else
					{
						break;
					}
				}

				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}
	return nRet;
}
/*更新锦标赛状态*/
int dzpkUpdateDbCpsStatus(int nID, int nStatus)
{
	int nRet = 0;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			char szSql[200];
			memset(szSql, 0, 200);
			sprintf(szSql, "UPDATE tbl_gametype_mst SET state = %d WHERE id = %d", nStatus, nID);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}
int dzpkDbClearUserRoomMark()
{
	int nRet = 0;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			string sql_a = "UPDATE tbl_session_mst SET roomnum = 0 where roomnum>0";
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return nRet;
}
/*更新等级礼包时间*/
int dzpkDbUpdateLevelGift(int nUserID, int nGiftLevel, int nCountTime, int nTodayPlayTimes)
{
	int nRet = 0;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			char szSql[200];
			memset(szSql, 0, 200);
			sprintf(szSql, "UPDATE tbl_user_mst SET level_count = %d,countdown = %d,today_playtimes = %d  WHERE user_id = %d", nGiftLevel, nCountTime, nTodayPlayTimes, nUserID);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return nRet;
}
/*更新用户筹码,差值*/
/*nType：0为普通模式 1为锦标赛, 2 只更新筹码*/
int dzpkDbUpdateUserChip(int nUserID, long nChip, long &lUserChip, int nType, user *pUserInfo)
{	
#if 1
	if ( nType == DZPK_USER_CHIP_CHANGE_PLAY_GAME && nChip > 0)
	{
		StdHttpPostChipsWinMessage(nUserID, pUserInfo, nChip);
	}
#endif
	int nRet = 0;

	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();//获取连接状态
			state->execute("use dzpk");//使用指定的数据库

			char szSql[200];
			sprintf(szSql, "select t1.user_id,chip_account,bigest_win_chip,bigest_chip from `tbl_user_mst` t1 left join `tbl_userinfo_mst` t2 on t1.user_id = t2.user_id where t1.user_id =%d ", nUserID);
			string query_sql = szSql;
			sShow = query_sql;
			ResultSet *result = state->executeQuery(query_sql);
			string user_id = "";
			string db_chip_account = "";
			string   bigest_win_chip = "";
			string   bigest_chip = "";

			while (result->next())
			{
				user_id = result->getString("user_id");
				db_chip_account = result->getString("chip_account");
				bigest_win_chip = result->getString("bigest_win_chip");
				bigest_chip = result->getString("bigest_chip");
				break;
			}

			if (result)
			{
				delete result;
				result = NULL;
			}
			if (user_id != "")
			{
				long lUpChip = atol(db_chip_account.c_str()),               //之前筹码
					lUpBigWinAcnt = atol(bigest_win_chip.c_str()),           //之前赢的最大筹码
					lUpBigAcnt = atol(bigest_chip.c_str());                //曾经拥有最大筹码数

				if (pUserInfo)
				{
					if (nType == DZPK_USER_CHIP_CHANGE_PLAY_GAME)
					{
						char szTooltip[100];
						sprintf(szTooltip, "普通玩牌");
						dzpkDbWriteLuckCardLog(2001, pUserInfo->room_num, nUserID, nChip, szTooltip, lUpChip);
					}
					else if (nType == DZPK_USER_CHIP_CHANGE_CPS_AWARD)
					{
						char szTooltip[100];
						sprintf(szTooltip, " 比赛获得奖励");
						dzpkDbWriteLuckCardLog(2002, pUserInfo->room_num, nUserID, nChip, szTooltip, lUpChip);
					}
					else if (nType == DZPK_USER_CHIP_CHANGE_CPS_ENTRYFEE)
					{
						char szTooltip[100];
						sprintf(szTooltip, "比赛退还报名费用");
						dzpkDbWriteLuckCardLog(2003, pUserInfo->room_num, nUserID, nChip, szTooltip, lUpChip);
					}
					else if (nType == DZPK_USER_CHIP_CHANGE_CPS_FACE)
					{
						char szTooltip[100];
						sprintf(szTooltip, " 锦标赛使用表情");
						dzpkDbWriteLuckCardLog(2003, pUserInfo->room_num, nUserID, nChip, szTooltip, lUpChip);
					}
					else if (nType == DZPK_USER_CHIP_CHANGE_COUNT_DOWN)
					{
						char szTooltip[100];
						sprintf(szTooltip, "%s 倒计时宝箱获得筹码", pUserInfo->nick_name.c_str());
						dzpkDbWriteLuckCardLog(10001, pUserInfo->room_num, nUserID, nChip, szTooltip, lUpChip);
					}
				}

				char szSql[400];
				memset(szSql, 0, 400);
				int nContinue = 0;
				switch (nType)
				{
				case DZPK_USER_CHIP_CHANGE_PLAY_GAME:       //普通玩牌
				{
					if (nChip > 0 && nChip > lUpBigWinAcnt)
					{
						//之前一盘赢的最大筹码
						lUpBigWinAcnt = nChip;
					}

					lUpChip = lUpChip + nChip;
					if (lUpChip < 0)
					{
						//更新筹码
						lUpChip = 0;
					}

					//更新内存数据
					//lUserChip = lUpChip;

					if (lUpChip > lUpBigAcnt)
					{
						//曾经拥有最大筹码数
						lUpBigAcnt = lUpChip;
					}

					if (nChip > 0)
					{
						sprintf(szSql, "update tbl_userinfo_mst,tbl_user_mst set total_time = total_time +1,win_per = win_per +1,bigest_win_chip =%ld,bigest_chip=%ld,chip_account=%ld where tbl_user_mst.user_id= %d and tbl_userinfo_mst.user_id = tbl_user_mst.user_id", lUpBigWinAcnt, lUpBigAcnt, lUpChip, nUserID);
					}
					else
					{
						sprintf(szSql, "update tbl_userinfo_mst,tbl_user_mst set total_time = total_time +1,bigest_win_chip =%ld,bigest_chip=%ld,chip_account=%ld where tbl_userinfo_mst.user_id = tbl_user_mst.user_id and tbl_user_mst.user_id= %d", lUpBigWinAcnt, lUpBigAcnt, lUpChip, nUserID);
					}
					nContinue = 1;
				}
				break;
				case DZPK_USER_CHIP_CHANGE_PLAY_CPS:        //锦标赛玩牌
				{
				}
				break;
				case DZPK_USER_CHIP_CHANGE_CPS_AWARD:       //锦标赛奖励
				case DZPK_USER_CHIP_CHANGE_CPS_ENTRYFEE:    //锦标赛退还比赛费用
				case DZPK_USER_CHIP_CHANGE_LUCK_HAND_CARD:  //幸运手牌获得奖励
				case DZPK_USER_CHIP_CHANGE_CPS_FACE:        //锦标赛使用表情
				case DZPK_USER_CHIP_CHANGE_COUNT_DOWN:      //倒计时宝箱
				{
					lUpChip = lUpChip + nChip;
					if (lUpChip < 0)
					{
						//更新筹码
						lUpChip = 0;
					}

					//更新内存数据
					//lUserChip = lUpChip;

                    printf("--------退还用户报名费----------\n");
					sprintf(szSql, "update tbl_user_mst set chip_account=%ld where user_id=%d", lUpChip, nUserID);
					nContinue = 1;
				}
				break;
				}

				if (nContinue == 1)
				{
					//更新数据库筹码
					sShow = szSql;
					string sql_a = szSql;
					state->execute(sql_a);
				}
			}

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}

//更新玩家携带筹码数
void dzpkDbUpdateChipHand(int nUserID, int nChip)
{
	if (nChip < 0)
		nChip = 0;
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();//获取连接状态
			state->execute("use dzpk");//使用指定的数据库

			char szSql[512];
			sprintf(szSql, "update tbl_user_mst set chip_hand=%d where user_id=%d", nChip, nUserID);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

}

//插入玩家牌局信息
void dzpkDbInsertPlayCard(user *pUser, dealer *pDealer, int nChip)
{
	if (pUser->hand_cards.size() < 2)
		return;

	//更新玩家携带筹码数
	if (nChip != pUser->hand_chips - pUser->last_chips)
		dzpkDbUpdateChipHand(pUser->nUserID, nChip + pUser->last_chips);
	else
		dzpkDbUpdateChipHand(pUser->nUserID, pUser->hand_chips);

	pUser->nChipUpdate += nChip; //用户在房间中游戏的筹码变更

	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();//获取连接状态
			state->execute("use dzpk");//使用指定的数据库
			char stDesk[64], stHand[32];
			memset(stDesk, 0, 64);
			sprintf(stDesk, "%d,%d,%d,%d,%d", pDealer->desk_card[0], pDealer->desk_card[1], pDealer->desk_card[2], pDealer->desk_card[3], pDealer->desk_card[4]);
			memset(stHand, 0, 32);
			sprintf(stHand, "%d,%d", pUser->hand_cards.at(0), pUser->hand_cards.at(1));

			char szSql[512];
			memset(szSql, 0, 512);
			sprintf(szSql, "insert into tbl_playcards_mst (playtime,room_id,smallblind,charge_per,ring,step,desk_card,hand_card,user_id,code,chips,last_remain_chip,curr_remain_chip) \
                values (now(),%d,%d,%d,%d,%d,'%s','%s',%d,'%s',%d,%ld,%ld)",
				pDealer->dealer_id, pDealer->small_blind, pDealer->charge_per, pDealer->nRing, pDealer->step, stDesk, stHand, pUser->nUserID,
				pUser->terminal_code.c_str(), nChip, pUser->chip_account - nChip, pUser->chip_account);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
}


/*插如用户获得筹码*/
int dzpkDbInsertGetChip(int nUserID, long nChip, int nFromUserID, int nType)
{
	int nRet = 0;

	time_t now;
	char szTime[200];
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			char szSql[200];
			memset(szSql, 0, 200);
			sprintf(szSql, "INSERT INTO tbl_getchip_mst (user_id, chip,gettime,gettype,fromuser,row_index) VALUES (%d,%ld,'%s',%d,%d,0)", nUserID, nChip, szTime, nType, nFromUserID);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return nRet;
}
/*判断用户是否有踢人卡*/
int dzpkDbUseProhibitCar(int nUserID, int *pToolCount)
{
	int nRet = -1;

	if (pToolCount != NULL)
	{
		*pToolCount = 0;
	}
	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[200];
			sprintf(szSql, "SELECT id,mall_id,starttime,mall_usertime FROM tbl_userpacket_mst WHERE user_id = %d AND mall_id =%d AND isdelete = 0", nUserID, DZPK_USER_PROHIBIT_USER_CARD);

			string  sql = szSql;
			sShow = sql;
			// 查询
			ResultSet *result = state->executeQuery(sql);

			USER_BACK_PACK_T pack;
			pack.nRecordID = 0;
			pack.nEndTime = 0;
			// 输出查询
			while (result->next())
			{
				string sStartTime;
				int nLastTime = 0;

				pack.nRecordID = result->getInt("id");
				pack.nMallID = result->getInt("mall_id");

				sStartTime = result->getString("starttime");

				nLastTime = result->getInt("mall_usertime");
				pack.nEndTime = nLastTime;

				break;
			}

			if (result)
			{
				delete result;
				result = NULL;
			}
			if (pack.nRecordID > 0 && pack.nEndTime > 0)
			{
				pack.nEndTime--;

				if (pToolCount != NULL)
				{
					*pToolCount = pack.nEndTime;
				}

				char szSql[200];
				memset(szSql, 0, 200);
				sprintf(szSql, "UPDATE tbl_userpacket_mst SET mall_usertime = %d WHERE id = %d", pack.nEndTime, pack.nRecordID);
				string sql_a = szSql;
				sShow = szSql;
				state->execute(sql_a);
				nRet = 0;
			}

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}

/*判断用户是否有互动道具*/
int dzpkDbUseSendToolCard(int nUserID, int *pCurCount)
{
	int nRet = -1;

	if (pCurCount)
	{
		*pCurCount = 0;
	}

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[200];
			sprintf(szSql, "SELECT id,mall_id,starttime,mall_usertime FROM tbl_userpacket_mst WHERE user_id = %d AND mall_usertime>0 AND mall_id =%d  AND isdelete = 0", nUserID, DZPK_USER_SEND_TOOL_CARD);

			string  sql = szSql;
			sShow = sql;
			// 查询
			ResultSet *result = state->executeQuery(sql);

			USER_BACK_PACK_T pack;
			pack.nRecordID = 0;
			pack.nEndTime = 0;
			// 输出查询
			while (result->next())
			{
				string sStartTime;
				int nLastTime = 0;

				pack.nRecordID = result->getInt("id");
				pack.nMallID = result->getInt("mall_id");

				sStartTime = result->getString("starttime");

				nLastTime = result->getInt("mall_usertime");
				pack.nEndTime = nLastTime;

				break;
			}

			if (result)
			{
				delete result;
				result = NULL;
			}

			if (pack.nRecordID > 0 && pack.nEndTime > 0)
			{
				pack.nEndTime--;

				if (pCurCount)
				{
					*pCurCount = pack.nEndTime;
				}

				char szSql[200];
				memset(szSql, 0, 200);
				sprintf(szSql, "UPDATE tbl_userpacket_mst SET mall_usertime = %d WHERE id = %d", pack.nEndTime, pack.nRecordID);
				string sql_a = szSql;
				sShow = szSql;
				state->execute(sql_a);
				nRet = 0;
			}

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}

void getCurrDateTime(char *datetime)
{
	time_t tick;
	struct tm tm;

	tick = time(NULL);
	tm = *localtime(&tick);
	strftime(datetime, 90, "%Y-%m-%d %H:%M:%S", &tm);
}


/*记录荷官获得小费日志*/
int dzpkGiveTipUpdateData(int roomType, int userId)
{
	int nRet = -1;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char currDate[100] = { '\0' };
			getCurrDateTime(currDate);

			char szSql[200];
			sprintf(szSql, "insert into tbl_dealer_log(room_type,user_id,date) values(%d,%d,'%s')", roomType, userId, currDate);

			string  sql = szSql;
			sShow = sql;
			state->execute(sql);
			WriteLog(40000, "dzpkGiveTipUpdateData sql: %s\n", szSql);
			/*
						sprintf(szSql,"update tbl_user_mst set chip_account = chip_account+%d where user_id=%d", chip, userId);
						sql=szSql;
						sShow = sql;
						state->execute(sql);
						*/

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}


/*更新用户背包时间*/
int dzpkDbUpdateBackpackTime(int nUserID)
{
	int nRet = 0;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			char szSql[400];
			memset(szSql, 0, 400);
			//sprintf(szSql,"UPDATE  tbl_userpacket_mst p SET p.apply = 2 WHERE  p.user_id = %d AND p.apply = 1  AND p.isdelete = 0 AND (p.mall_usertime - (UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(p.starttime)))<0",nUserID);
			sprintf(szSql, "UPDATE tbl_userpacket_mst p LEFT JOIN tbl_mallbase_mst m ON m.mall_id = p.mall_id SET p.apply = 2,p.isdelete = 1,p.mall_usertime=0,p.sale_time = NOW() WHERE p.user_id = %d AND p.isdelete = 0 AND m.mall_type=3 AND p.apply = 1 AND (UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(p.starttime))>=p.mall_usertime", nUserID);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);

			sprintf(szSql, "UPDATE tbl_userpacket_mst p LEFT JOIN tbl_mallbase_mst m ON m.mall_id = p.mall_id SET p.mall_usertime=p.mall_usertime-(UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(p.starttime)),apply = 0,p.starttime = NOW() WHERE p.user_id = %d AND p.apply = 1 AND p.isdelete = 0 AND m.mall_type=3", nUserID);
			sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);

			int nVipLevel = 0;
			if (dzpkDbGetUserMaxVIP(nUserID, nVipLevel) == 0)
			{
				sprintf(szSql, "UPDATE tbl_userpacket_mst p SET p.apply = 1,p.starttime = NOW() WHERE p.user_id = %d AND p.apply = 0 and id=%d", nUserID, nVipLevel);
				sql_a = szSql;
				sShow = sql_a;
				state->execute(sql_a);
			}

			sprintf(szSql, "update tbl_user_mst set vip_level=%d where user_id=%d", nVipLevel, nUserID);
			sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}
/*读用户背包东西*/
int dzpkDbReadUserBackpack(user *pUser)
{
	int nRet = 0;

	if (pUser && pUser->nUserID > 0)
	{

		dzpkDbUpdateBackpackTime(pUser->nUserID);

		pUser->arryBackpack.clear();

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库

				char szSql[200];
				//apply=0:使用类道具 1:佩戴类道具 2:状态类道具
				sprintf(szSql, "SELECT id,mall_id,starttime,mall_usertime FROM tbl_userpacket_mst WHERE user_id = %d AND isdelete = 0", pUser->nUserID);

				string  sql = szSql;
				sShow = sql;
				// 查询
				ResultSet *result = state->executeQuery(sql);
				// 输出查询
				while (result->next())
				{
					string sStartTime;
					int nLastTime = 0, nStartTime = 0;
					USER_BACK_PACK_T pack;

					pack.nRecordID = result->getInt("id");
					pack.nMallID = result->getInt("mall_id");

					sStartTime = result->getString("starttime");
					nStartTime = dzpkDbGetTimeDayToInt(sStartTime);

					nLastTime = result->getInt("mall_usertime");
					pack.nEndTime = nStartTime + nLastTime;

					pUser->arryBackpack.push_back(pack);
				}
				pUser->nReadBpFromDb = 1;
				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}


/*增加一张抽奖券*/
int dzpkDbAddAwardChangeCard(int nUserID, int nAddCards)
{
	int nRet = -1;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[200];
			sprintf(szSql, "SELECT user_id,ticknum FROM act_plateuser_mst WHERE user_id = %d", nUserID);

			string  sql = szSql;
			sShow = sql;
			// 查询
			ResultSet *result = state->executeQuery(sql);

			// 输出查询
			int nTickNumCount = 0;
			int nFind = 0;
			while (result->next())
			{
				nTickNumCount = result->getInt("ticknum");
				nFind = 1;
				break;
			}

			if (result)
			{
				delete result;
				result = NULL;
			}

			if (nFind == 1)
			{
				if (nTickNumCount < 0)
				{
					nTickNumCount = 0;
				}

				char szTooltip[100];
				sprintf(szTooltip, "获得抽奖券");
				dzpkDbWritePacketLog(3, -1, nUserID, nAddCards, szTooltip, nTickNumCount);

				nTickNumCount += nAddCards;
				if (nTickNumCount < 0)
				{
					nTickNumCount = 0;
				}

				char szSql[200];
				memset(szSql, 0, 200);
				sprintf(szSql, "UPDATE act_plateuser_mst SET ticknum = %d WHERE user_id = %d", nTickNumCount, nUserID);
				string sql_a = szSql;
				sShow = szSql;
				state->execute(sql_a);

				nRet = nTickNumCount;
			}
			else
			{
				nTickNumCount += nAddCards;
				if (nTickNumCount < 0)
				{
					nTickNumCount = 0;
				}

				time_t now;
				char szTime[200];
				struct tm *timenow;
				time(&now);
				timenow = localtime(&now);
				sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

				//增加
				char szSql[200];
				memset(szSql, 0, 200);
				sprintf(szSql, "insert act_plateuser_mst (user_id,ticknum,createtime) values (%d,%d,'%s')", nUserID, nTickNumCount, szTime);

				string sql_a = szSql;
				sShow = szSql;
				state->execute(sql_a);

				nRet = nTickNumCount;
			}

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return nRet;
}


/*重新读用户信息*/
int dzpkDbReadUserInfo(user *pUser, int nType)
{
	int nRet = -1;

	if (pUser && pUser->nUserID > 0)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库

				char szSql[400];
				switch (nType)
				{
				case 1:
				{
					pUser->giftid = 0;
					pUser->mall_path = "";

					sprintf(szSql, "SELECT tbl_user_mst.user_id,tbl_user_mst.giftid,tbl_mallbase_mst.mall_path,tbl_userpacket_mst.user_id FROM tbl_user_mst,tbl_mallbase_mst,tbl_userpacket_mst WHERE tbl_user_mst.user_id = %d AND tbl_user_mst.giftid = tbl_userpacket_mst.id AND  tbl_mallbase_mst.mall_id = tbl_userpacket_mst.mall_id", pUser->nUserID);

					string  sql = szSql;
					sShow = sql;
					// 查询
					ResultSet *result = state->executeQuery(sql);

					// 输出查询
					while (result->next())
					{
						pUser->giftid = result->getInt("giftid");
						pUser->mall_path = result->getString("mall_path");

						nRet = 0;
						break;
					}

					if (result)
					{
						delete result;
						result = NULL;
					}
				}
				break;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}

/*记录用户玩牌时间*/
int dzpkDbUserPlayTime(int nUserID, int nRoomID, int nEnterTime, int nGames, int nChips, int nBlind, int nPercent)
{
	int nRet = 0;

	if (nEnterTime > 0)
	{
		int nPlayTime = StdGetTime() - nEnterTime;
		if (nPlayTime > 0 && nPlayTime < 5 * 24 * 3600)
		{
			//从连接池获取一个连接
			time_t now;
			char *szTime = new char[200];
			if (szTime)
			{
				struct tm *timenow;
				time(&now);
				timenow = localtime(&now);
				sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

				//从连接池获取一个连接
				Connection *con = connpool->GetConnection();
				if (con)
				{
					string sShow;
					try
					{
						Statement *state = con->createStatement();           //获取连接状态
						state->execute("use dzpk");                         //使用指定的数据库
						char szSql[200];
						memset(szSql, 0, 200);
						sprintf(szSql, "INSERT INTO tbl_usertime_mst (user_id, creattime,roomId,timelength,games,chips,smallblind,charge_per) VALUES (%d,'%s',%d,%d,%d,%d,%d,%d)", nUserID, szTime, nRoomID, nPlayTime, nGames, nChips, nBlind, nPercent);
						string sql_a = szSql;
						sShow = sql_a;
						state->execute(sql_a);
						//更新玩家总时长:秒数
						memset(szSql, 0, 200);
						sprintf(szSql, "update tbl_userinfo_mst set total_secs=total_secs+%d where user_id=%d", nPlayTime, nUserID);
						sql_a = szSql;
						sShow = sql_a;
						state->execute(sql_a);
						delete state;//删除连接状态
					}
					catch (sql::SQLException &e)
					{
						WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
					}
					catch (...)
					{
						WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
					}
					connpool->ReleaseConnection(con);//释放当前连接
				}

				delete szTime;
			}
		}
	}
	return nRet;
}

/*读取活动参数*/
int dzpkReadActivityInfo(SYSTEM_ACTIVITY_HEAD_T *pHead)
{
	int nRet = 0;

	if (pHead)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "SELECT id,NAME,starttime,endtime,overtime,team1,team2,team3,team4,team5,team6,team7,team8,team9,team10 FROM act_actionbase_mst";
				sShow = sql;

				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					SYSTEM_ACTIVITY_T *pRecord = new SYSTEM_ACTIVITY_T();
					if (pRecord)
					{
						string sName, sStartTime, sEndTime, sOverTime, sTeamString[10];

						pRecord->nID = result->getInt("id");
						sName = result->getString("NAME");
						sStartTime = result->getString("starttime");
						sEndTime = result->getString("endtime");
						sOverTime = result->getString("overtime");
						sTeamString[0] = result->getString("team1");
						sTeamString[1] = result->getString("team2");
						sTeamString[2] = result->getString("team3");
						sTeamString[3] = result->getString("team4");
						sTeamString[4] = result->getString("team5");
						sTeamString[5] = result->getString("team6");
						sTeamString[6] = result->getString("team7");
						sTeamString[7] = result->getString("team8");
						sTeamString[8] = result->getString("team9");
						sTeamString[9] = result->getString("team10");

						memcpy(pRecord->szName, sName.c_str(), strlen(sName.c_str()));

						pRecord->nStartTime = dzpkDbGetTimeDayToInt(sStartTime);
						pRecord->nEndTime = dzpkDbGetTimeDayToInt(sEndTime);
						pRecord->nOverTime = dzpkDbGetTimeDayToInt(sOverTime);

						memcpy(pRecord->szTeam1, sTeamString[0].c_str(), strlen(sTeamString[0].c_str()));
						memcpy(pRecord->szTeam2, sTeamString[1].c_str(), strlen(sTeamString[1].c_str()));
						memcpy(pRecord->szTeam3, sTeamString[2].c_str(), strlen(sTeamString[2].c_str()));
						memcpy(pRecord->szTeam4, sTeamString[3].c_str(), strlen(sTeamString[3].c_str()));
						memcpy(pRecord->szTeam5, sTeamString[4].c_str(), strlen(sTeamString[4].c_str()));
						memcpy(pRecord->szTeam6, sTeamString[5].c_str(), strlen(sTeamString[5].c_str()));
						memcpy(pRecord->szTeam7, sTeamString[6].c_str(), strlen(sTeamString[6].c_str()));
						memcpy(pRecord->szTeam8, sTeamString[7].c_str(), strlen(sTeamString[7].c_str()));
						memcpy(pRecord->szTeam9, sTeamString[8].c_str(), strlen(sTeamString[8].c_str()));
						memcpy(pRecord->szTeam10, sTeamString[9].c_str(), strlen(sTeamString[9].c_str()));

						if (pHead->pLast == NULL)
						{
							pHead->pFirst = pRecord;
							pHead->pLast = pRecord;
						}
						else
						{
							pHead->pLast->pNext = pRecord;
							pHead->pLast = pRecord;
						}

						WriteLog(USER_ACTIVITY_AWARD_LEVEL, "dzpkReadActivityInfo id[%d] name[%s] start[%s][%d] curTime[%d] endtime[%s][%d] over[%s][%d] team 1[%s] 2[%s]  3[%s] 4[%s] 5[%s] 6[%s] 7[%s] 8[%s] 9[%s] 10[%s]  \n",
							pRecord->nID,
							sName.c_str(),
							sStartTime.c_str(), pRecord->nStartTime,
							StdGetTime(),
							sEndTime.c_str(), pRecord->nEndTime,
							sOverTime.c_str(), pRecord->nOverTime,
							pRecord->szTeam1, pRecord->szTeam2, pRecord->szTeam3, pRecord->szTeam4, pRecord->szTeam5, pRecord->szTeam6, pRecord->szTeam7, pRecord->szTeam8, pRecord->szTeam9, pRecord->szTeam10);
					}
					else
					{
						break;
					}
				}

				if (result)
				{
					delete result;
					result = NULL;
				}

				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}
/*时间类道具奖励*/
int dzpkDbAwardTimeTool(int nUserID, int nMallID, int nSecond)
{
	int nRet = 0;
	dzpkDbUpdateBackpackTime(nUserID);

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[200];
			sprintf(szSql, "select id,mall_usertime from tbl_userpacket_mst where user_id=%d and mall_id = %d and apply = 1 AND isdelete = 0", nUserID, nMallID);

			string  sql = szSql;
			sShow = sql;
			// 查询
			ResultSet *result = state->executeQuery(sql);

			// 输出查询
			int nMallTime = 0, nRecordID = 0;
			int nFind = 0;
			while (result->next())
			{
				nRecordID = result->getInt("id");
				nMallTime = result->getInt("mall_usertime");
				nFind = 1;
				break;
			}

			if (result)
			{
				delete result;
				result = NULL;
			}

			if (nFind == 1)
			{
				if (nMallTime < 0)
				{
					nMallTime = 0;
				}
				nMallTime += nSecond;
				if (nMallTime < 0)
				{
					nMallTime = 0;
				}

				char szSql[200];
				memset(szSql, 0, 200);
				sprintf(szSql, "UPDATE tbl_userpacket_mst SET mall_usertime = %d WHERE id = %d", nMallTime, nRecordID);
				string sql_a = szSql;
				sShow = szSql;
				state->execute(sql_a);

				nRet = nMallTime;
			}
			else
			{
				nMallTime = nSecond;
				if (nMallTime < 0)
				{
					nMallTime = 0;
				}

				time_t now;
				char szTime[200];
				struct tm *timenow;
				time(&now);
				timenow = localtime(&now);
				sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

				//增加
				char szSql[400];
				memset(szSql, 0, 400);
				sprintf(szSql, "INSERT INTO tbl_userpacket_mst(user_id,mall_id,apply,starttime,buy_time,sale_time,sale_chip,mall_usertime,source,usertype,isdelete,islook) VALUES(%d,%d,%d,'%s','%s','%s',%d,%d,%d,%d,%d,%d)",
					nUserID, nMallID, 1, szTime, szTime, "", 0, nMallTime, 0, 0, 0, 1);

				string sql_a = szSql;
				sShow = szSql;
				state->execute(sql_a);

				nRet = nMallTime;
			}

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}
/*读取每日任务*/
int dzpkReadEveryDayTaskInfo(SYSTEM_TASK_HEAD_T *pHead)
{
	int nRet = 0;

	if (pHead)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库
				string  sql = "SELECT id,TYPE,title,total,award FROM tbl_taskbase_mst";
				sShow = sql;

				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					TASK_BASE_T *pRecord = new TASK_BASE_T();
					if (pRecord)
					{
						string sTitle;

						pRecord->nID = result->getInt("id");
						pRecord->nType = result->getInt("TYPE");
						sTitle = result->getString("title");
						pRecord->nTotal = result->getInt("total");
						pRecord->nAward = result->getInt("award");

						memcpy(pRecord->szTitle, sTitle.c_str(), strlen(sTitle.c_str()));

						if (pHead->pLast == NULL)
						{
							pHead->pFirst = pRecord;
							pHead->pLast = pRecord;
						}
						else
						{
							pHead->pLast->pNext = pRecord;
							pHead->pLast = pRecord;
						}

						WriteLog(USER_SYSTEM_TASK_LEVEL, "dzpkReadEveryDayTaskInfo id[%d] type[%d] title[%s] total[%d] award[%d]\n",
							pRecord->nID,
							pRecord->nType,
							pRecord->szTitle,
							pRecord->nTotal,
							pRecord->nAward);
					}
					else
					{
						break;
					}
				}

				if (result)
				{
					delete result;
					result = NULL;
				}

				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}
/*更新一个任务的个数*/
int dzpkUpdateTaskStage(int nRecordID, int nCurrent)
{
	int nRet = 0;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			char szSql[200];
			memset(szSql, 0, 200);
			sprintf(szSql, "UPDATE tbl_taskuser_mst SET current = %d WHERE id = %d", nCurrent, nRecordID);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}

/*增加任务*/
int dzpkAddTask(int nUserID, int nTaskID, int nCurrent, int nTotal)
{
	int nRet = 0;

	time_t now;
	char szTime[200];
	struct tm *timenow;
	time(&now);
	timenow = localtime(&now);
	sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库
			char szSql[200];
			memset(szSql, 0, 200);
			sprintf(szSql, "INSERT INTO tbl_taskuser_mst (user_id,taskid,current,total,createtime,status) VALUES (%d,%d,%d,%d,'%s',%d)",
				nUserID, nTaskID, nCurrent, nTotal, szTime, 0);
			string sql_a = szSql;
			sShow = sql_a;
			state->execute(sql_a);
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return nRet;
}
/*读用户任务*/
int dzpkDbReadUserTask(user *pUser)
{
	int nRet = 0;

	if (pUser && pUser->nUserID > 0)
	{
		WriteLog(USER_SYSTEM_TASK_LEVEL, "enter read user task  from db user name[%s] \n", pUser->nick_name.c_str());

		pUser->arryTask.clear();

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库

				char szSql[400];
				//char *pTemp = "AND DATEDIFF(CURDATE() ,(DATE_FORMAT(tbl_taskuser_mst.createtime,'%Y-%m-%d')))=0";
				char szTemp[200];
				memset(szTemp, 0, 200);
				memcpy(szTemp, "AND DATEDIFF(CURDATE() ,(DATE_FORMAT(tbl_taskuser_mst.createtime,'%Y-%m-%d')))=0", strlen("AND DATEDIFF(CURDATE() ,(DATE_FORMAT(tbl_taskuser_mst.createtime,'%Y-%m-%d')))=0"));
				sprintf(szSql, "SELECT id,taskid,current,total,createtime,status FROM tbl_taskuser_mst WHERE user_id = %d %s ", pUser->nUserID, szTemp);

				string  sql = szSql;
				sShow = sql;
				// 查询
				ResultSet *result = state->executeQuery(sql);

				//int nCurTime = StdGetTime();
				// 输出查询
				while (result->next())
				{
					string sStartTime;
					int nStartTime = 0;
					USER_TASK_INFO_T pack;

					pack.nID = result->getInt("id");
					pack.nTaskID = result->getInt("taskid");
					pack.nCurrent = result->getInt("current");
					pack.nTotal = result->getInt("total");
					pack.nStatus = result->getInt("status");

					sStartTime = result->getString("createtime");
					nStartTime = dzpkDbGetTimeDayToInt(sStartTime);
					pack.nCreateTime = nStartTime;

					/*
											WriteLog(USER_SYSTEM_TASK_LEVEL,"read from db user task name[%s]  id[%d] taskid[%d] current[%d]  total[%d]  createtime[%s]  \n",
												pUser->nick_name.c_str()
												,pack.nID,
												pack.nTaskID,
												pack.nCurrent,
												pack.nTotal,
												sStartTime.c_str());
					*/
					pUser->arryTask.push_back(pack);
				}
				pUser->nReadTaskFromDb = 1;
				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}
/*幸运手牌日志*/
int dzpkDbWriteLuckCardLog(int nType, int nRoomID, int nUserID, int nChip, char *pTooltip, long nCurChipCount)
{
	int nRet = 0;

	if (pTooltip)
	{
		time_t now;
		char szTime[200];
		struct tm *timenow;
		time(&now);
		timenow = localtime(&now);
		sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				//state->execute("use dzpk");                         //使用指定的数据库
				state->execute("use dzpklog");                         //使用指定的数据库
				char szSql[400];
				memset(szSql, 0, 400);
				sprintf(szSql, "INSERT INTO tbl_luckcard_log (type,roomid,userid,chip,tooltip,createtime,curchipcount) VALUES (%d,%d,%d,%d,'%s','%s',%ld)",
					nType, nRoomID, nUserID, nChip, pTooltip, szTime, nCurChipCount);
				string sql_a = szSql;
				sShow = sql_a;
				state->execute(sql_a);
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}
	return nRet;
}


int dzpkDbReadRoomVisible()
{
	//从连接池获取一个连接
	int ret = 0;
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[400];
			sprintf(szSql, "SELECT room_id,timedelete FROM tbl_room_mst");

			string  sql = szSql;
			sShow = sql;
			// 查询
			ResultSet *result = state->executeQuery(sql);

			// 输出查询
			int nRoomID = 0, nTimeDelete = 0;
			while (result->next())
			{
				nRoomID = result->getInt("room_id");
				nTimeDelete = result->getInt("timedelete");

				dealer *pRoom = DzpkGetRoomInfo(nRoomID);
				if (pRoom)
				{
					pRoom->nTimeDelete = nTimeDelete;
				}
			}
			if (result)
			{
				delete result;
				result = NULL;
			}
			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			ret = -1;
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			ret = -1;
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return ret;
}
/*背包日志*/
int dzpkDbWritePacketLog(int nType, int nRoomID, int nUserID, int nCount, char *pTooltip, long nCurToolCount)
{
	int nRet = 0;

	if (pTooltip)
	{
		time_t now;
		char szTime[200];
		struct tm *timenow;
		time(&now);
		timenow = localtime(&now);
		sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				//state->execute("use dzpk");                         //使用指定的数据库
				state->execute("use dzpklog");                         //使用指定的数据库
				char szSql[400];
				memset(szSql, 0, 400);
				sprintf(szSql, "INSERT INTO tbl_userpacket_log(type,roomid,userid,toolcount,tooltip,createtime,curtoolcount) VALUES (%d,%d,%d,%d,'%s','%s',%ld)",
					nType, nRoomID, nUserID, nCount, pTooltip, szTime, nCurToolCount);
				string sql_a = szSql;
				sShow = sql_a;
				state->execute(sql_a);
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}
	return nRet;
}
/*服务器人数日志*/
int dzpkDbWriteOnlineNumLog(int nServerID, int nServerType, int nSocketCount, int nUserCount)
{
	int nRet = 0;

	if (1)
	{
		time_t now;
		char szTime[200];
		struct tm *timenow;
		time(&now);
		timenow = localtime(&now);
		sprintf(szTime, "%04d-%02d-%02d %02d:%02d:%02d", timenow->tm_year + 1900, timenow->tm_mon + 1, timenow->tm_mday, timenow->tm_hour, timenow->tm_min, timenow->tm_sec);

		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				//state->execute("use dzpk");                         //使用指定的数据库
				state->execute("use dzpklog");                         //使用指定的数据库
				char szSql[400];
				memset(szSql, 0, 400);
				sprintf(szSql, "INSERT INTO tbl_servernum_log(serverid,servertype,socketcount,usercount,tcreatetime) VALUES (%d,%d,%d,%d,'%s')",
					nServerID, nServerType, nSocketCount, nUserCount, szTime);
				string sql_a = szSql;
				sShow = sql_a;
				state->execute(sql_a);
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}
	return nRet;
}
/*查询用户vip*/
int dzpkDbGetUserMaxVIP(int nUserID, int &nVip)
{
	int nRet = -1;
	nVip = 0;

	if (nUserID > 0)
	{
		//从连接池获取一个连接
		Connection *con = connpool->GetConnection();
		if (con)
		{
			string sShow;
			try
			{
				Statement *state = con->createStatement();           //获取连接状态
				state->execute("use dzpk");                         //使用指定的数据库

				char szSql[400];
				sprintf(szSql, "select m.level,p.id from tbl_userpacket_mst p left join tbl_mallbase_mst m on m.mall_id = p.mall_id where p.user_id = %d and m.mall_type = 3 and p.apply = 0 and m.level=(select max(m.level) from tbl_userpacket_mst p left join  tbl_mallbase_mst m on m.mall_id = p.mall_id where p.user_id = %d and m.mall_type = 3 and p.apply = 0 and p.isdelete = 0) and p.isdelete = 0", nUserID, nUserID);

				string  sql = szSql;
				sShow = sql;
				// 查询
				ResultSet *result = state->executeQuery(sql);

				// 输出查询
				while (result->next())
				{
					nVip = result->getInt("id");
					nRet = 0;
					break;
				}
				if (result)
				{
					delete result;
					result = NULL;
				}
				delete state;//删除连接状态
			}
			catch (sql::SQLException &e)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
			}
			catch (...)
			{
				WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
			}
			connpool->ReleaseConnection(con);//释放当前连接
		}
	}

	return nRet;
}


/*查询玩牌次数可以奖励的金豆数*/
bool queryAwardJinDou(int awardId, int &award, int &num)
{
	bool ret = false;
	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sShow;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[200];
			sprintf(szSql, "SELECT award, num FROM tbl_award_mst WHERE awardid = %d", awardId);

			string  sql = szSql;
			sShow = sql;
			// 查询
			ResultSet *result = state->executeQuery(sql);

			// 输出查询
			while (result->next())
			{
				award = result->getInt("award");
				num = result->getInt("num");
				ret = true;
				break;
			}


			if (result)
			{
				delete result;
				result = NULL;
			}

			delete state;//删除连接状态
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sShow.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return ret;
}


/*记录用户获取的金豆*/
int addUserJinDou(int userId, int jinDouNum, int roomId, int smallBind, int playtime)
{
	int nRet = -1;

	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sql;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[500];

			sprintf(szSql, "update tbl_user_mst set jindou=jindou+%d where user_id=%d", jinDouNum, userId);

			sql = szSql;
			state->execute(sql);

			char currDate[100] = { '\0' };
			getCurrDateTime(currDate);

			memset(szSql, 0, sizeof(szSql));
			sprintf(szSql, "INSERT INTO tbl_userjindou_log(userid, jindou, datetime, roomid, smallbind, playtime, type) VALUES(%d, %d,'%s', %d, %d, %d, %d)",
				userId, jinDouNum, currDate, roomId, smallBind, playtime, 1);

			sql = szSql;
			state->execute(sql);

			delete state;//删除连接状态
			nRet = 0;
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sql.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}

	return nRet;
}


/*记录用户玩牌获取宝箱抽奖日志*/
bool addGetChestsLog(int userid, int roomid, int smallBlind, int playtime)
{
	bool ret = false;
	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sqlText;
		try
		{
			Statement *state = con->createStatement();           //获取连接状态
			state->execute("use dzpk");                         //使用指定的数据库

			char szSql[200];
			memset(szSql, 0, 200);

			char currDate[100] = { '\0' };
			getCurrDateTime(currDate);

			sprintf(szSql, "INSERT INTO tbl_user_chests_log(userid, roomid, smallblind, createtime, isuse, playtime) VALUES (%d, %d, %d, '%s', %d, %d)",
				userid, roomid, smallBlind, currDate, 0, playtime);

			sqlText = szSql;
			printf("执行数据插入 \n");
			state->execute(sqlText);

			//删除连接状态
			delete state;

			ret = true;
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sqlText.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return ret;
}

//记录用户比赛结果日志
bool insertGameRank(int recoredId, int userId, int rank, int awardType)
{
	bool ret = false;
	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con == NULL)
	{
		return false;
	}
	string sqlText;
	try
	{
		Statement *state = con->createStatement();           //获取连接状态
		if (state == NULL)
		{
			return false;
		}

		state->execute("use dzpk");                         //使用指定的数据库

		char szSql[200];
		memset(szSql, 0, 200);

		char currDate[100] = { '\0' };
		getCurrDateTime(currDate);

		sprintf(szSql, "INSERT INTO tbl_game_rank(game_id, game_rank, create_time, award_type, user_id) \
            VALUES (%d, %d, '%s', %d, %d)",
			recoredId, rank, currDate, awardType, userId);

		sqlText = szSql;
		state->execute(sqlText);

		//删除连接状态
		delete state;

		ret = true;
	}
	catch (sql::SQLException &e)
	{
		WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sqlText.c_str());
	}
	catch (...)
	{
		WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
	}
	connpool->ReleaseConnection(con);//释放当前连接
	return ret;
}

//修改玩家禁止登陆标志
bool dzpkDbLoginDisable(int user_id)
{
	bool ret = false;
	//从连接池获取一个连接
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sqlText;
		try
		{
			Statement *state = con->createStatement();			 //获取连接状态
			if (state == NULL) return false;
			state->execute("use dzpk");
			char szSql[200];
			memset(szSql, 0, 200);
			sprintf(szSql, "UPDATE tbl_user_mst SET userstate = 2 WHERE user_id = %d", user_id);
			sqlText = szSql;
			state->execute(sqlText);
			ret = true;
		}
		catch(sql::SQLException &e)
		{			
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sqlText.c_str());
		}
		catch(...)
		{			
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);//释放当前连接
	}
	return ret;
}


