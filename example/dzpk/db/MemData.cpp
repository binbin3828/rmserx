#include "MemData.h"

#include "db_connect_pool.h"
#include "../Start.h"
#include "../standard/dzpkCustom.h"

#include <iostream>



void MemData::loadMemData()
{
	if (!getAward() || !getService())
	{
		printf("load tbl_award_mst tables data failed!\n");
	}
}

bool MemData::getAward()
{
	bool ret = false;

	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sql;
		try
		{
			Statement *state = con->createStatement();
			state->execute("use dzpk");

			char szSql[400];
			sprintf(szSql, "SELECT awardid, idtype, awardtype, awardname, award, num \
                    from  tbl_award_mst");
			sql = szSql;

			ResultSet *result = state->executeQuery(sql);

            pthread_rwlock_wrlock(&m_rwlockAward);

            map<int, Award *>::iterator ite = awardMap.begin();
            for (; ite!=awardMap.end(); ite++)
            {
                delete ite->second;
            }
            awardMap.clear();

			while (result->next())
			{
				Award *award = new Award;
				award->awardid = result->getInt("awardid");
				award->idtype = result->getInt("idtype");
				award->awardtype = result->getInt("awardtype");
				award->awardname = result->getString("awardname");
				award->award = result->getInt("award");
				award->num = result->getInt("num");
				awardMap[award->awardid] = award;
			}
            pthread_rwlock_unlock(&m_rwlockAward);

			if (result)
			{
				delete result;
				result = NULL;
			}

			delete state;
			ret = true;
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sql.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);
	}

	map<int, Award *>::iterator ite = awardMap.begin();
	for (; ite != awardMap.end(); ite++)
	{
		cout << ite->first << ":" << ite->second->awardid << " "
			<< ite->second->awardid << " "
			<< ite->second->idtype << " "
			<< ite->second->awardtype << " "
			<< ite->second->awardname << " "
			<< ite->second->award << " "
			<< ite->second->num << endl;
	}

	return ret;
}


bool MemData::getService()
{
	bool ret = false;
	Connection *con = connpool->GetConnection();
	if (con)
	{
		string sql;
		try
		{
			Statement *state = con->createStatement();
			state->execute("use dzpk");

			char szSql[400];
			sprintf(szSql, "select r.roomId, f.columnName, f.gainType, f.needPerson  \
				from  tbl_function_mst f, tbl_room_fun r \
				where r.funId= f.funId;");
			sql = szSql;

			ResultSet *result = state->executeQuery(sql);

            pthread_rwlock_wrlock(&m_rwlockService);

            map<int, Service *>::iterator itSer = serviceMap.begin();
		    for (; itSer != serviceMap.end(); itSer++)
		    {
			    delete itSer->second;
		    }
		    serviceMap.clear();

            int roomid = 0;
			string columnName;

			while (result->next())
			{
				Service *ser = new Service;
				roomid = result->getInt("roomId");
				ser->columnName = result->getString("columnName");
				ser->gainType = result->getInt("gainType");
				ser->needPerson = result->getInt("needPerson");

				serviceMap.insert(make_pair(roomid, ser));
			}
            pthread_rwlock_unlock(&m_rwlockService);

			if (result)
			{
				delete result;
				result = NULL;
			}

			delete state;
			ret = true;
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sql.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}
		connpool->ReleaseConnection(con);
	}

	multimap<int, Service *>::iterator ite = serviceMap.begin();
	while (ite != serviceMap.end())
	{
		cout << ite->first << ":" << ite->second->columnName
			<< " : " << ite->second->gainType
			<< " : " << ite->second->needPerson << endl;
		ite++;
	}

	return ret;
}
/***
 * roomID: 房间ID
 * userId: 用户ID
 * smallBlind: 小盲注的钱
 * playNum: 座子玩牌人数
 * isWin:是否赢了
 ***/

bool MemData::updateGameNum(int roomId, int userId, int smallBlind, int playNum, int isWin)
{
	cout << "enter updateGameNum: roomId: " << roomId << ":" << "smallBlind: " << smallBlind << endl;
	cout << "playNum: " << playNum << ":" << "isWin: " << isWin << endl;	
	bool ret = false;

	Connection *con = connpool->GetConnection();
	int  updateCount = 0;

	if (con)
	{
		string sql;
		try
		{
			Statement *state = con->createStatement();
			state->execute("use dzpk");

			multimap<int, Service *>::iterator mite, beg, end;

            pthread_rwlock_rdlock(&m_rwlockService);
			beg = serviceMap.lower_bound(roomId);
			end = serviceMap.upper_bound(roomId);

			ostringstream oss;
			oss << "update tbl_game_num set ";
			int flag = 0;
			for (mite = beg; mite != end; mite++)
			{
				if (mite->second->gainType == 1
					&& isWin == 0
					&& playNum >= mite->second->needPerson)
				{
					if (flag == 1)
					{
						oss << "," << mite->second->columnName << "=" << mite->second->columnName << "+1";
					}
					else
					{
						oss << mite->second->columnName << "=" << mite->second->columnName << "+1";
					}
					flag = 1;
				}
				else if (mite->second->gainType == 2
					&& isWin == 1
					&& playNum >= mite->second->needPerson)
				{
					if (flag == 1)
					{
						oss << "," << mite->second->columnName << "=" << mite->second->columnName << "+1";
					}
					else
					{
						oss << mite->second->columnName << "=" << mite->second->columnName << "+1";
					}
					flag = 1;
				}
			}

			if (flag == 1)
			{
				oss << " where user_id = " << userId << " and small_blind=" << smallBlind;
				sql = oss.str();
				cout << "sql text: " << sql.c_str() << endl;
				updateCount = state->executeUpdate(sql);				
			}

			if (flag == 1 && updateCount == 0)
			{
				flag = 0;
				ostringstream oss1;
				oss1 << "insert into tbl_game_num(user_id, small_blind";
				int flag = 0;
				for (mite = beg; mite != end; mite++)
				{
					oss1 << "," << mite->second->columnName;
					flag = 1;
				}
				if (flag == 1 && isWin == 0)
				{
					oss1 << ") values(" << userId << "," << smallBlind;
					for (mite = beg; mite != end; mite++)
					{
						if (mite->second->gainType == 1
							&& playNum >= mite->second->needPerson)
						{
							oss1 << "," << 1;
						}
						else if (mite->second->gainType == 1
							&& playNum < mite->second->needPerson)
						{
							oss1 << "," << 0;
						}
						else if (mite->second->gainType == 2)
						{
							oss1 << "," << 0;
						}
					}
				}
				else if (flag == 1 && isWin == 1)
				{
					oss1 << ") values(" << userId << "," << smallBlind;
					for (mite = beg; mite != end; mite++)
					{
						if (mite->second->gainType == 2
							&& playNum >= mite->second->needPerson)
						{
							oss1 << "," << 1;
						}
						else if (mite->second->gainType == 2
							&& playNum < mite->second->needPerson)
						{
							oss1 << "," << 0;
						}
						else if (mite->second->gainType == 1)
						{
							oss1 << "," << 0;
						}
					}
				}
				
				if (flag == 1)
				{
					oss1 << ")";
					sql = oss1.str();
					
					WriteLog(40000,"----guobin---- sql: %s\n",sql.c_str());
					state->execute(sql);
				}
			}
            pthread_rwlock_unlock(&m_rwlockService);
			delete state;
			ret = true;
		}
		catch (sql::SQLException &e)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "sql error[%s] sql [%s] \n", e.what(), sql.c_str());
		}
		catch (...)
		{
			WriteLog(HIGHEST_LEVEL_ERROR, "error \n");
		}

		connpool->ReleaseConnection(con);
	}

    return ret;
}












