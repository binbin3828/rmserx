/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.02
** 版本：1.0
** 说明：日志输出
**************************************************************
*/
#include "log.h"
#include <sys/time.h>
#include "../../example/dzpk/standard/custom.h"
#include <cstddef>
#include <dirent.h>
#include <sys/stat.h>

Log::Log()
    :m_bEnabled(false)
{
	m_nWriteCount = 0;
}

Log::~Log()
{
}

bool Log::Open(string sFileName)
{
    m_tOLogFile.open(sFileName.c_str(), ios::binary|ios_base::out | ios_base::app);
    m_nWriteCount = 0;
    if( !m_tOLogFile )
    {
        return false;
    }
    m_bEnabled = true;
    return true;
}

void Log::Close()
{
    if(m_tOLogFile.is_open())
    {
        m_tOLogFile.close();
    }
}

bool Log::CommonLogInit()
{
    time_t tNowTime;
    time(&tNowTime);

    tm* tLocalTime = localtime(&tNowTime);

    //得到日期的字符串
    string sDateStr = ValueToStr(tLocalTime->tm_year+1900) + "_" +
        ValueToStr(tLocalTime->tm_mon+1) + "-" +
        ValueToStr(tLocalTime->tm_mday) + "_" +
	ValueToStr(tLocalTime->tm_hour) + "." +
	ValueToStr(tLocalTime->tm_min) + "." +
	ValueToStr(tLocalTime->tm_sec);

	if(NULL == opendir("./logs"))
	{
		mkdir("./logs",0775);
	}

    return Open("./logs/room_server" + sDateStr + ".log");
}
bool Log::CommonLogInit(string name)
{
    time_t tNowTime;
    time(&tNowTime);

    tm* tLocalTime = localtime(&tNowTime);

    //得到日期的字符串
    string sDateStr = ValueToStr(tLocalTime->tm_year+1900) + "-" +
        ValueToStr(tLocalTime->tm_mon+1) + "-" +
        ValueToStr(tLocalTime->tm_mday) + "_" +
	ValueToStr(tLocalTime->tm_hour) + "." +
	ValueToStr(tLocalTime->tm_min) + "." +
	ValueToStr(tLocalTime->tm_sec);

	if(NULL == opendir("./logs"))
	{
		mkdir("./logs",0775);
	}

    return Open("./logs/Log_" + name + sDateStr + ".log");
}

bool Log::CommonLogInit(int nRoomID)
{
    time_t tNowTime;
    time(&tNowTime);

    tm* tLocalTime = localtime(&tNowTime);
    string name = "room_" + ValueToStr(nRoomID) + "_" ;
    //得到日期的字符串
    string sDateStr = ValueToStr(tLocalTime->tm_year+1900) + "_" +
        ValueToStr(tLocalTime->tm_mon+1) + "_" +
        ValueToStr(tLocalTime->tm_mday) + "_" +
	ValueToStr(tLocalTime->tm_hour) + "." +
	ValueToStr(tLocalTime->tm_min) + "." +
	ValueToStr(tLocalTime->tm_sec);

	if(NULL == opendir("./logs"))
	{
		mkdir("./logs",0775);
	}
    return Open("./logs/" + name + sDateStr + ".log");
}
void Log::Enable()
{
    m_bEnabled = true;
}

void Log::Disable()
{
    m_bEnabled = false;
}

//得到当前时间的字符串
string Log::GetTimeStr()
{
    string str;
    return str;

    time_t tNowTime;
    time(&tNowTime);

    tm* tLocalTime = localtime(&tNowTime);

    //"2011-07-18 23:03:01 ";
    string strFormat = "%Y-%m-%d %H:%M:%S ";


    struct timeval  tv;
    gettimeofday(&tv,NULL);
    //return  tv.tv_sec *1000

    char strDateTime[30] ;//= {};
    strftime(strDateTime, 30, strFormat.c_str(), tLocalTime);

    string s2 = strDateTime;
    string s3 = itoa((tv.tv_usec/1000),10);
    if(s3.length()==1){
        s3 = "00"+s3;
    }else if(s3.length()==2){
        s3 = "0"+s3;
    }
    string strRes = s2+"."+s3;
    return strRes;
}
