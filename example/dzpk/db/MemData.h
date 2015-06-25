#ifndef MEMDATA_H
#define MEMDATA_H

#include <string>
#include <map>

#include <pthread.h>


using namespace std;



struct Award
{
    int awardid;
    int idtype;
    int awardtype;
    string awardname;
    int award;
    int num;
};

struct Service
{
	string columnName;
	int gainType;
	int needPerson;
};


class MemData
{
public:
    map<int, Award *> awardMap;
    multimap<int, Service *> serviceMap;

private:
    pthread_rwlock_t m_rwlockAward;
    pthread_rwlock_t m_rwlockService;
public:
	MemData()
	{
        pthread_rwlock_init(&m_rwlockAward, NULL);
        pthread_rwlock_init(&m_rwlockService, NULL);
	}
    ~MemData()
    {
        map<int, Award *>::iterator ite = awardMap.begin();
        for (; ite!=awardMap.end(); ite++)
        {
            delete ite->second;
        }
        awardMap.clear();

		map<int, Service *>::iterator itSer = serviceMap.begin();
		for (; itSer != serviceMap.end(); itSer++)
		{
			delete itSer->second;
		}
		serviceMap.clear();

        pthread_rwlock_destroy(&m_rwlockAward);
        pthread_rwlock_destroy(&m_rwlockService);
    }

public:
    void loadMemData();
    bool getAward();
    bool getService();
	bool updateGameNum(int roomId, int userId, int smallBlind, int playNum, int isWin = 0);
};

#endif
