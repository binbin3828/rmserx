/*
**************************************************************
** ��Ȩ�������л����ڿƼ����޹�˾
** ����ʱ�䣺2014.05.02
** �汾��1.0
** ˵�������ݿ����ӳ�
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

//���ӳع��캯��
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

//��ȡһ�����ӳ�ʵ��
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


//��ʼ�����ӳأ����������������һ���������
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


//��������
Connection* ConnPool::CreateConnection(){
     Connection * conn;
     try{
         conn = driver->connect(this->url,this->username,this->password);//��������
         return conn;
     }catch(sql::SQLException&e){
         // cLog<<cLog.GetTimeStr()<<"[ERR]GET MYSQL DRIVER INSTANCE FAILED!" <<endl;
          return NULL;
     }catch(std::runtime_error&e){
         // cLog<<cLog.GetTimeStr()<<"[ERR]RUNTIME ERROR!" <<endl;
          return NULL;
     }
}

//�����ӳ��л�ȡһ������
Connection* ConnPool::GetConnection(){
    Connection* con;
    pthread_mutex_lock(&lock);

    if(connList.size() > 0){//���ӳ��л�������
        con = connList.front();//�õ���һ������
        connList.pop_front();//�Ƴ���һ������
        if(con->isClosed()){//��������Ѿ����رգ�ɾ�������½���һ��
            delete con;
            con = this->CreateConnection();
        }
        if(con == NULL){
            --curSize;
        }
        pthread_mutex_unlock(&lock);
        return con;
    }else{
        if(curSize < maxSize ){//�����Դ����µ�����
            con = this->CreateConnection();
            if(con){
                ++curSize;
                pthread_mutex_unlock(&lock);
                return con;
            }else{
                pthread_mutex_unlock(&lock);
                return NULL;
            }
        }else{//�������������Ѿ��ﵽmaxSize
            pthread_mutex_unlock(&lock);

            return NULL;
        }
    }
}


//�������ݿ�����
void ConnPool::ReleaseConnection(sql::Connection * conn){
    if(conn){
        pthread_mutex_lock(&lock);
        connList.push_back(conn);
        pthread_mutex_unlock(&lock);
    }
}

//���ӳص���������
ConnPool::~ConnPool()
{
    this->DestoryConnPool();
	pthread_mutex_destroy( &lock);
}

//�������ӳ�,����Ҫ�������ӳ��е�����
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

//����һ������
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




