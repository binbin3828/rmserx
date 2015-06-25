
#ifndef __DBCONNECTPOOL_H__
#define __DBCONNECTPOOL_H__
#include "../include/mysql_connection.h"
#include "../include/mysql_driver.h"
#include "../include/cppconn/exception.h"
#include "../include/cppconn/driver.h"
#include "../include/cppconn/connection.h"
#include "../include/cppconn/resultset.h"
#include "../include/cppconn/prepared_statement.h"
#include "../include/cppconn/statement.h"
#include <pthread.h>
#include <list>

using namespace std;
using namespace sql;


/*
*���ӳ���
*/
class ConnPool {
    private:
        int curSize;//��ǰ�ѽ�����������
        int maxSize;//���������
        string  username;
        string  password;
        string  url;
        list<Connection*> connList;//���ӳص���������
        pthread_mutex_t lock; //�߳���
        static  ConnPool *connPool;
        Driver* driver;

        Connection* CreateConnection();//����һ������
        void InitConnection(int iInitialSize);//��ʼ�����ӳ�
        void DestoryConnection(Connection *conn);//�������Ӷ���
        void DestoryConnPool();//�������ӳ�
        ConnPool(string url,string user,string password,int maxSize);//���췽��
    public:
    ~ConnPool();
    Connection* GetConnection();//������ݿ�����
    void ReleaseConnection(Connection *conn);//�ͷ�����
    static ConnPool* GetInstance();//��ȡ���ӳض���
    static int ReleaseInstance();       //�ͷŶ���
};

#endif //__DBCONNECTPOOL_H__
