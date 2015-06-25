#ifndef __MAINLOGER_H__
#define __MAINLOGER_H__

/*JackFrost 2013.11.6 管理日志模块*/

#define HIGHEST_LEVEL_ERROR     40000000    //最严重错误

/*开启日志模块*/
/*pPath不要使用/结束 当前目录 .  其它目录格式为 /user/local*/
int ServiceLogerStart(char *pPath,char *pName);

/*关闭日志模块*/
int ServiceLogerStop();

#define WRITE_LOG_LEN       1024*10

/*获得模块运行状态，是否有运行*/
/*0 为没有运行  1 为正常运行*/
int GetLogerStatus();		

#define WriteLog(nWarning, format, arg... )					WriteFileLog(nWarning, __FILE__, __LINE__, format, ##arg )
#define WriteRoomLog(nWarning,nRoomID,nUserID,format, arg... )			WriteRoomFileLog(nWarning,nRoomID,nUserID, __FILE__, __LINE__, format, ##arg )

void WriteFileLog(int nWarning,const char *file, int line, const char *format, ... );

void WriteRoomFileLog(int nWarning,int nRoomID,int nUserID,const char *file, int line, const char *format, ... );

#endif //__MAINLOGER_H__
