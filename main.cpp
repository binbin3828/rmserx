/*
**************************************************************
** 版权：深圳市智狐科技有限公司
** 创建时间：2015.06.25
** 版本：1.0
** 说明：德州扑克入口函数,main函数
**************************************************************
*/
#include <iostream>
#include <stdio.h>
#include <signal.h>
#include "com/common.h"
#include <string.h>
#include "com/sysConfig.h"
#include "com/stillAlive.h"
#include "com/md5.h"
#include "com/base64.h"
#include "http/HttpSocket.h"
#include "udp/UdpBusiness.h"

static int g_nMainFlag = 1;
#define SERVER_VERSION	"DZPK-GB-2.0"

static void QuitMain( int )
{
	g_nMainFlag = 0;
}

static void DoNothing( int )
{
}

static void SignalInstall()
{
	signal( SIGINT, QuitMain );
	signal( SIGTERM, SIG_IGN );
	signal( SIGQUIT, SIG_IGN );
	signal( SIGPIPE, DoNothing );
	signal( SIGTRAP, DoNothing );

	//allow core dump
	struct rlimit limit;
	limit.rlim_cur = RLIM_INFINITY;
	limit.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE,&limit);

	daemon(1,1);//后台运行 不改变运行目录和不屏蔽输出
}

/*初始化*/
static void InitAll()
{
	SignalInstall();
    char szLogPath[100],szLogName[100];
    sprintf(szLogPath,"%s",".");
    sprintf(szLogName,"%s","Rmserx");
	ServiceLogerStart(szLogPath,szLogName);

    struct rlimit rt;
    //设置每个进程允许打开的最大文件数
    rt.rlim_max = rt.rlim_cur = NET_SOCKET_USER_MAX*5;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1)
    {
        WriteLog(40051,"set max file error \n");
        exit(1);
    }
    else
    {
    }

    char szName[100];
    sprintf(szName,"server_room.conf");
    SysConfInit(szName);

    sprintf(szName,"ServerPort");
    int nListerPort = SysConfGetInt(szName);
    if(nListerPort <= 0)
    {
        WriteLog(60004,"read conf error ServerPort[%d] \n",nListerPort);
        exit(1);
    }

    sprintf(szName,"MysqlIp");
    char *pSqlIP = SysConfGetChar(szName);
    if(pSqlIP == NULL)
    {
        WriteLog(60004,"read conf error  MysqlIp NULL \n");
        exit(1);
    }

    sprintf(szName,"MysqlPort");
    int nSqlPort = SysConfGetInt(szName);
    if(nSqlPort <= 0)
    {
        WriteLog(60004,"read conf error MysqlPort[%d] \n",nSqlPort);
        exit(1);
    }

    sprintf(szName,"MysqlLoginName");
    char *pSqlLoginName = SysConfGetChar(szName);
    if(pSqlLoginName == NULL)
    {
        WriteLog(60004,"read conf error  MysqlLoginName NULL \n");
        exit(1);
    }

    sprintf(szName,"MysqlLoginPwd");
    char *pSqlLoginPwd = SysConfGetChar(szName);
    if(pSqlLoginPwd == NULL)
    {
        WriteLog(60004,"read conf error  pSqlLoginPwd NULL \n");
        exit(1);
    }

    ServiceNetSetPort(nListerPort);

    char szDbName[100];
    sprintf(szDbName,"dzpk");

    WriteLog(4005,"init listen port[%d] mysql ip [%s]  mysql port [%d] mysql login name[%s] login password[%s] \n",nListerPort,pSqlIP,nSqlPort,pSqlLoginName,"******");
    DzpkSetDbInfo(pSqlIP,nSqlPort,pSqlLoginName,pSqlLoginPwd,szDbName);
    DzpkInit();

	//init udp server	
	//udp client服务器
	if(0 != ClientServer.InitServer(htons(nListerPort),1))
	{
		printf("ClientServer init failed!\n");
		exit(1);
	}
	//udp client业务处理线程
	pthread_t th;
	if(0 != pthread_create(&th,NULL,ClientBusiness,&ClientServer))
	{
		printf("ClientServer thread create failed!\n");
		exit(1);
	}

	//init httpClient
	sprintf(szName,"HttpPort");
	int iHttpPort = SysConfGetInt(szName);
	if(iHttpPort <= 0)
	{
		printf("no HttpPort configure!\n");
		exit(1);
	}
	sprintf(szName,"PostThread");
	int iPostThread = SysConfGetInt(szName);
	if(iPostThread <= 0)
	{
		printf("no PostThread configure!\n");
		exit(1);
	}
	sprintf(szName,"HttpIP");
	char *lpHttpIP = SysConfGetChar(szName);
	if(NULL == lpHttpIP)
	{
		printf("no HttpIP configure!\n");
		exit(1);
	}
	sprintf(szDbName,"HttpHost");
	char *lpHttpHost = SysConfGetChar(szDbName);
	if(NULL == lpHttpHost)
	{
		printf("no HttpHost configure!\n");
		exit(1);
	}
	sprintf(szDbName,"WebPathFile");
	char *lpWebPathFile = SysConfGetChar(szDbName);
	if(NULL == lpWebPathFile)
	{
		printf("no WebPathFile configure!\n");
		exit(1);
	}
	WriteLog(4005,"init HTTP HttpPort[%d] PostThread [%d]	HttpIp[%s]	HttpHost[%s] WebPathFile[%s]\n",iHttpPort,iPostThread,lpHttpIP,lpHttpHost,lpWebPathFile,"******");
	//http post任务处理线程
	if(0 != HttpServer.InitServer(lpHttpHost,lpWebPathFile,inet_addr(lpHttpIP),htons(iHttpPort),iPostThread))
	{
		printf("HttpServer init failed!\n");
		exit(1);
	}
}

static void DeInitAll()
{
    DzpkUninit();
}

/*开启系统服务*/
static void StartAllService()
{
	ServiceNetStart();
	ServiceDzpkStart();
}

/*关闭系统服务*/
static void StopAllService()
{
	ServiceNetStop();
	ServiceDzpkStop();
	ServiceLogerStop();
}

static void MainLoop()
{
    long nRunCount = 1;
	while( g_nMainFlag)
	{
		sleep(2);
        nRunCount++;
#ifdef _DEBUG
        if(nRunCount%5 == 0)
#else
        if(nRunCount%30 == 0)
#endif
        {
            if(nRunCount%300 == 0)
            {
                DzpkServerStatus(1,2,NetGetSocketUserCount() -1,NetGetNetUserCount());
            }
            else
            {
                WriteLog(4000,"server current socket count:[%d] user count:[%d]\n",NetGetSocketUserCount() -1,NetGetNetUserCount());
            }

        }
	}
}

int main(int argc, char* argv[])
{
    if (argc == 2)
    {
        if (strcmp(argv[1],"-v")==0)
        {
            printf("version: rmserx [%s]\nbuild: %s %s\n",SERVER_VERSION,__DATE__,__TIME__);
        }
        else
        {
            printf("parameter error: please input rmserx -v\n");
        }

        return 0;
    }
	
	InitAll();
	StartAllService();
	WriteLog(40000,"program start  Version:[%s]\n",SERVER_VERSION);

	MainLoop();

	WriteLog(40000,"program stop \n");
	StopAllService();

	WriteLog(40000,"program stop 1 \n");
	DeInitAll();

	return 0;
}
