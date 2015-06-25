/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.02
** 版本：1.0
** 说明：数据库连接池
**************************************************************
*/
#include <stdexcept>
#include <exception>
#include <stdio.h>
#include "db_connect_pool.h"
#include "../../../core/loger/log.h"
#include "../Start.h"

using namespace std;
using namespace sql;
extern Log cLog;

ConnPool *ConnPool::connPool = NULL;

//连接池构造函数
ConnPool::ConnPool(string url,string username,string password,int maxsize)
{

	pthread_mutex_init( &lock, NULL );
    this->maxSize = maxsize;
    this->curSize = 0;
    this->username = username;
    this->password = password;
    this->url = url;
    try{
        this->driver = sql::mysql::get_driver_instance();
    }catch(sql::SQLException&e){
       // cLog<<cLog.GetTimeStr()<<"[ERR]GET MYSQL DRIVER INSTANCE FAILED!" <<endl;
    }catch(std::runtime_error&e){
       // cLog<<cLog.GetTimeStr()<<"[ERR]RUNTIME ERROR!" <<endl;
    }
    this->InitConnection(maxSize /2 );
}

//获取一个连接池实例
ConnPool* ConnPool::GetInstance(){
    if(connPool == NULL)
    {
        char szConnect[100];
        sprintf(szConnect,"tcp://%s:%d",g_pDzpkDbInfo->szIP,g_pDzpkDbInfo->nPort);
        //connPool = new ConnPool("tcp://localhost:3306","ltdzpk","ltdzpk",20);
        connPool = new ConnPool(szConnect,g_pDzpkDbInfo->szLoginName,g_pDzpkDbInfo->szLoginPwd,20);
     }
    return connPool;
}

int ConnPool::ReleaseInstance()
{
    if(connPool)
    {
        delete connPool;
        connPool = NULL;
    }
    return 0;
}


//初始化连接池，创建最大连接数的一半的连接数
void ConnPool::InitConnection(int iInitialSize){
    Connection* conn;
    pthread_mutex_lock(&lock);
    for(int i=0;i<iInitialSize;i++){
        conn = this->CreateConnection();
        if(conn){
            connList.push_back(conn);
            ++(this->curSize);
        }else {
           // cLog<<cLog.GetTimeStr()<<"[ERR]CREATE CONNECTION ERROR!" <<endl;
        }
    }
     pthread_mutex_unlock(&lock);
}


//创建连接
Connection* ConnPool::CreateConnection(){
     Connection * conn;
     try{
         conn = driver->connect(this->url,this->username,this->password);//建立连接
         return conn;
     }catch(sql::SQLException&e){
         // cLog<<cLog.GetTimeStr()<<"[ERR]GET MYSQL DRIVER INSTANCE FAILED!" <<endl;
          return NULL;
     }catch(std::runtime_error&e){
         // cLog<<cLog.GetTimeStr()<<"[ERR]RUNTIME ERROR!" <<endl;
          return NULL;
     }
}

//在连接池中获取一个连接
Connection* ConnPool::GetConnection(){
    Connection* con;
    pthread_mutex_lock(&lock);

    if(connList.size() > 0){//连接池中还有连接
        con = connList.front();//得到第一个连接
        connList.pop_front();//移除第一个连接
        if(con->isClosed()){//如果连接已经被关闭，删除后重新建立一个
            delete con;
            con = this->CreateConnection();
        }
        if(con == NULL){
            --curSize;
        }
        pthread_mutex_unlock(&lock);
        return con;
    }else{
        if(curSize < maxSize ){//还可以创建新的连接
            con = this->CreateConnection();
            if(con){
                ++curSize;
                pthread_mutex_unlock(&lock);
                return con;
            }else{
                pthread_mutex_unlock(&lock);
                return NULL;
            }
        }else{//建立的连接数已经达到maxSize
            pthread_mutex_unlock(&lock);

            return NULL;
        }
    }
}


//回收数据库连接
void ConnPool::ReleaseConnection(sql::Connection * conn){
    if(conn){
        pthread_mutex_lock(&lock);
        connList.push_back(conn);
        pthread_mutex_unlock(&lock);
    }
}

//连接池的析构函数
ConnPool::~ConnPool()
{
    this->DestoryConnPool();
	pthread_mutex_destroy( &lock);
}

//销毁连接池,首先要销毁连接池中的连接
void ConnPool::DestoryConnPool(){
    list<Connection*>::iterator icon;
    pthread_mutex_lock(&lock);
    for(icon=connList.begin();icon!=connList.end();++icon){
        this->DestoryConnection(*icon);
    }
    curSize=0;
    connList.clear();
    pthread_mutex_unlock(&lock);
}

//销毁一个连接
void ConnPool::DestoryConnection(Connection* conn){
    if(conn){
        try{
            conn->close();
        }catch(sql::SQLException&e){
           // cLog<<cLog.GetTimeStr()<<"[ERR]CLOSE CONNECTION ERROR!" <<endl;
        }catch(std::exception&e){
          // cLog<<cLog.GetTimeStr()<<"[ERR]DESTORY CONNECTION ERROR!" <<endl;
        }

        delete conn;
    }
}




