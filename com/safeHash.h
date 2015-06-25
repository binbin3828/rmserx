/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.2
** 版本：1.0
** 说明：德州扑克标准流程线程安全哈希表
**************************************************************
*/
#ifndef __SAFEHASH_H__
#define __SAFEHASH_H__

#include "autoLock.h"

typedef struct _SafeHashSt_
{
	unsigned int nKey;
	void *pVal;
	struct _SafeHashSt_ *next;
} SAFE_HASH_T;

typedef struct tagSafeHashList
{
	int nSize;
	void ** pTable;
	int nArrySize;
	void ** arry;
}SAFE_HASH_LIST_T;

typedef void HashRealFreeDataBack(void *ptr);

/* 线程安全的哈希类*/
class CSafeHash
{
public:
	//bThreadSafe 是否支持线程安全
	CSafeHash( int maxNode = 1000,bool bThreadSafe = true,int nPublicMutex = 0,HashRealFreeDataBack *pFreeDataBack = NULL);
	~CSafeHash();

	//插入数据
	int Insert( unsigned int nKey, void *pVal, int nValSize , const char *file = __FILE__, int line = __LINE__);
	int InsertHadShareMalloc( unsigned int nKey, void *pVal, int nValSize , const char *file = __FILE__, int line = __LINE__);
	int InsertShare(unsigned int nKey, void *pVal, const char *file = __FILE__, int line = __LINE__);
	//删除数据
	int Remove( unsigned int nKey , const char *file = __FILE__, int line = __LINE__);
	//查询数据
	void *GetVal( unsigned int nKey , const char *file = __FILE__, int line = __LINE__);
	//释放数据，查询获得的数据需要释放
	void FreeVal( void *pVal );

	//获得总共有多少数据
	int GetSize(const char *file = __FILE__, int line = __LINE__);
	//清除哈希所有数据
	int Clear(const char *file = __FILE__, int line = __LINE__);

	//获得所有数据
	SAFE_HASH_LIST_T *GetList(const char *file = __FILE__, int line = __LINE__);
	//获得所有数据需要调用此接口释放
	int FreeList(SAFE_HASH_LIST_T * pList,const char *file = __FILE__, int line = __LINE__);

private:
	void FreeNode( SAFE_HASH_T *pNode,int nForceFree = 0);
private:
	SAFE_HASH_T **m_ppTable;
	int		m_maxNode;
	class CRwMutex	m_rwlock;
	class CMutex  m_MemoryLock;
	HashRealFreeDataBack *m_pHashRealFreeDataBack;
	bool m_bThreadSafe;		//是否支持线程安全
	int m_nPublicMutex;
	int m_nCount;
	int m_nExternCount;
};

void *ShareMallocHash( long nBytes);
void ShareFreeHashInterFail( void *ptr);

#endif  // __SAFEHASH_H__

