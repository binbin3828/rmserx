/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标准流程线程安全链表
**************************************************************
*/
#ifndef __SAFELIST_H__
#define __SAFELIST_H__

#include "autoLock.h"

/*使用链表需要继承本结构*/
typedef struct tagSafeList
{
	tagSafeList *pNext;
	long nReserve;
}SAFE_LIST_T;

/*链表头*/
typedef struct tagSafeListHead
{
	SAFE_LIST_T * pFirst;		//链表第一个数据
	SAFE_LIST_T * pLast;		//链表最后一个数据
	int nSize;			//总共有多少个
}SAFELIST_HEAD_T;

/* 线程安全链表,特殊需求可以继承此类再实现*/
class CSafeList
{
public:
	//nMaxNode 		最多支持多少个，超出插入失败
	//bThreadSafe		是否支持线程安全
	CSafeList(int nMaxNode = 10000,bool bThreadSafe = true);
	~CSafeList();
protected:
	CMutex	m_Lock;			//线程锁，不需要可设置bThreadSafe为false
	bool m_bThreadSafe;		//是否使用锁
	int m_nMaxNode;			//最多支持多少个
	SAFELIST_HEAD_T m_SafeListhead;	//链表头
	int m_nSize;			//多少个
	bool m_bCreate;			//运行标识
	int InternalClear();		//内部清理函数
	int InternalPutBuffer(SAFE_LIST_T * pSafeList);
	SAFE_LIST_T * InternalPopBuffer();
	char m_szName[100];
	int m_nErrorCount;
public:
	virtual int FreeData(SAFE_LIST_T *pSafeList);
public:
	/*添加数据到链表*/
	int PutBuffer(SAFE_LIST_T * pSafeList, const char *file = __FILE__, int line = __LINE__);
	/*取链表第一个数据*/
	SAFE_LIST_T * PopBuffer(const char *file = __FILE__, int line = __LINE__);

	/*清空链表*/
	int Clear(const char *file = __FILE__, int line = __LINE__);
	/*取当前链表个数*/
	int GetSize();

	/*设置显示内容*/
	void SetName(char *pName);
};


#endif //__SAFELIST_H__
