/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.02
** 版本：1.0
** 说明：德州扑克锦标赛
**************************************************************
*/
#include "cpsRecord.h"
#include "../../../com/common.h"

void CpsRecordRealFreeDataBack(void *ptr)
{
    CPS_RECORD_T * p = (CPS_RECORD_T *)ptr;
    if(p)
    {
        if(p->pMutex)
        {
            delete p->pMutex;
            p->pMutex = NULL;
        }
    }
}

static CSafeHash g_CpsRecore(100,true,0,CpsRecordRealFreeDataBack);

int CpsInsertRecord(int nRecordID,CPS_RECORD_T *pData,int nLen)
{
    CPS_RECORD_T * p = CpsGetRecord(nRecordID);
    if(p)
    {
        CpsFreeRecord(p);
        return -1;
    }

	return g_CpsRecore.Insert(nRecordID,(void*)pData,nLen);
}

/*获得记录信息*/
CPS_RECORD_T * CpsGetRecord(int nRecordID)
{
	return (CPS_RECORD_T *)g_CpsRecore.GetVal(nRecordID);
}

/*释放记录信息*/
int CpsFreeRecord(CPS_RECORD_T *pData)
{
	g_CpsRecore.FreeVal((void*)pData);
	return 0;
}

/*删除记录*/
int CpsRemoveRecord(int nRecordID)
{
	return g_CpsRecore.Remove(nRecordID);
}

SAFE_HASH_LIST_T *CpsRecordGetList()
{
	return g_CpsRecore.GetList();
}
int CpsRecordFreeList(SAFE_HASH_LIST_T * pList)
{
	return g_CpsRecore.FreeList(pList);
}
int CpsRecordGetSize()
{
    return g_CpsRecore.GetSize();
}
