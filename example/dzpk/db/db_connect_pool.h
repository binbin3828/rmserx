
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
*连接池类
*/
class ConnPool {
    private:
        int curSize;//当前已建立的连接数
        int maxSize;//最大连接数
        string  username;
        string  password;
        string  url;
        list<Connection*> connList;//连接池的容器队列
        pthread_mutex_t lock; //线程锁
        static  ConnPool *connPool;
        Driver* driver;

        Connection* CreateConnection();//创建一个连接
        void InitConnection(int iInitialSize);//初始化连接池
        void DestoryConnection(Connection *conn);//销毁连接对象
        void DestoryConnPool();//销毁连接池
        ConnPool(string url,string user,string password,int maxSize);//构造方案
    public:
    ~ConnPool();
    Connection* GetConnection();//获得数据库连接
    void ReleaseConnection(Connection *conn);//释放连接
    static ConnPool* GetInstance();//获取连接池对象
    static int ReleaseInstance();       //释放对象
};

#endif //__DBCONNECTPOOL_H__
