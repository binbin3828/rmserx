/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：日志文件头
**************************************************************
*/
#ifndef LOG_H
#define LOG_H

#include <fstream>
#include <string>
#include <sstream>
#include <ctime>

using namespace std;


/**
 * 用于输出log文件的类.
 */
class Log
{
public:
    Log();
    ~Log();

    bool Open(string strFileName);
    void Close();

    bool CommonLogInit(); //打开默认的log 文件
    bool CommonLogInit(string name);
    bool CommonLogInit(int nRoomID);

    void Enable();
    void Disable();

    int m_nWriteCount;
    string GetTimeStr();

    template <typename T> void LogOut(const T& value)
    {
        if (m_bEnabled)
        {
            m_tOLogFile << value;
        }
    }

    template <typename T> void LogOutLn(const T& value)
    {
        if (m_bEnabled)
        {
            m_tOLogFile << value << endl;
        }
    }

    void LogOutLn()
    {
        if (m_bEnabled)
        {
            m_tOLogFile << endl;
        }
    }

    template <typename T> Log& operator<<(const T& value)
    {
        return (*this);

        if (m_bEnabled)
        {
            m_tOLogFile << value;
	    m_tOLogFile.flush();

	    m_nWriteCount++;
	    if(m_nWriteCount > 10000)
	    {
		m_nWriteCount = 0;
		Close();
		CommonLogInit();
	    }
        }
        return (*this);
    }

    Log& operator<<(ostream& (*_Pfn)(ostream&))
    {
        if (m_bEnabled)
        {
            (*_Pfn)(m_tOLogFile);
        }
        return (*this);
    }

    int WriteChar(char *pvalue,int nLen)
    {
        if (m_bEnabled)
        {
            m_tOLogFile.write(pvalue,nLen);
	    m_tOLogFile.flush();
        }
	return 0;
    }

private:
    template<typename T> string ValueToStr(T value)
    {
        ostringstream ost;
        ost << value;
        return ost.str();
    }
private:
    ofstream m_tOLogFile;

    bool m_bEnabled;
};

#endif // LOG_H_INCLUDED
