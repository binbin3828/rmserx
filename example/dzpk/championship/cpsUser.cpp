/*
**************************************************************
** 版权：深圳市华诚亿科技有限公司
** 创建时间：2014.05.02
** 版本：1.0
** 说明：德州扑克锦标赛
**************************************************************
*/

#include "cpsUser.h"
#include <string.h>
#include <stdlib.h>
#include "../Start.h"

static CSafeHash g_CpsUser;

int CpsInserUser(int nUserID,CPS_USER_T *pData,int nLen)
{
	return g_CpsUser.Insert(nUserID,(void*)pData,nLen);
}

/*获得用户信息*/
CPS_USER_T * CpsGetUser(int nUserID)
{
	return (CPS_USER_T *)g_CpsUser.GetVal(nUserID);
}

/*释放用户信息*/
int CpsFreeNetUser(CPS_USER_T *pData)
{
	g_CpsUser.FreeVal((void*)pData);
	return 0;
}

/*删除用户*/
int CpsRemoveUser(int nUserID)
{
	return g_CpsUser.Remove(nUserID);
}

/*获得本活动的人数,不包括旁观*/
int CpsGetUserCount(int nType,int nCpsID)
{
    int nRet = 0;

    SAFE_HASH_LIST_T * pList = g_CpsUser.GetList();

    if(pList)
    {
        pList->arry = NULL;
        pList->nArrySize=0;

        if( pList->nSize > 0)
        {
            CPS_USER_T * p = NULL;
           for(int i=0;i<pList->nSize;i++)
           {
              p =  (CPS_USER_T *)pList->pTable[i];
              if(p && p->nCpsID == nCpsID && p->nType == nType)
              {
                  nRet++;
              }
           }
        }

        g_CpsUser.FreeList(pList);
    }

    return nRet;
}

/*
 * p < p1 返回 -1
 * p = p1 返回 0
 * p > p1 返回 1
 * */
int CpsCompareUser(CPS_USER_T *p,CPS_USER_T *p1,int nType)
{
    int nRet = 0;

    if(p && p1)
    {
        switch(nType)
        {
            case 0:
                {
                    if(p->nStart < p1->nStart)
                    {
                        nRet = -1;
                    }
                    else if(p->nStart > p1->nStart)
                    {
                        nRet = 1;
                    }
                    else
                    {
                        nRet = 0;
                    }
                }
                break;
            case 1:
                {
                    if(p->nCurHandChip < p1->nCurHandChip)
                    {
                        nRet = -1;
                    }
                    else if(p->nCurHandChip > p1->nCurHandChip)
                    {
                        nRet = 1;
                    }
                    else
                    {
                        nRet = 0;
                    }
                }
                break;
            case 2:
                {
                    if(p->nFailTime < p1->nFailTime)
                    {
                        nRet = -1;
                    }
                    else if(p->nFailTime > p1->nFailTime)
                    {
                        nRet = 1;
                    }
                    else
                    {
                        nRet = 0;
                    }
                }
                break;
            case 3:
                {
                    //跟其他不同，从小到大
                    if(p->nEnterTime > p1->nEnterTime)
                    {
                        nRet = -1;
                    }
                    else if(p->nEnterTime< p1->nEnterTime)
                    {
                        nRet = 1;
                    }
                    else
                    {
                        nRet = 0;
                    }
                }
                break;
        }
    }

    return nRet;
}

/*nType 0 是否在线 1 筹码 2 失败时间 3 进入时间(从小到大)*/
int CpsSortHandChip(CPS_USER_T * arry,int nStart,int nCount,int nType)
{
    if(nCount >= 2)
    {
        CPS_USER_T pT;
        for(int i=nStart;i<nStart+nCount;i++)
        {
            for(int j=i+1;j<nStart+nCount;j++)
            {
                if(CpsCompareUser(&arry[i],&arry[j],nType) < 0)
               {
                    pT = arry[i];
                    arry[i]= arry[j];
                    arry[j]= pT;
               }
            }
        }
    }
    return 0;
}

/*获得全部用户*/
/*nSortChip 0 不要要排序  1在线按筹码排序 2 全部在线筹码，不在线时间  3 在线按进入时间*/
SAFE_HASH_LIST_T *CpsUserGetList(int nType,int nCpsID,int nSortChip)
{
    SAFE_HASH_LIST_T * pList = g_CpsUser.GetList();

    if(pList)
    {
        pList->arry = NULL;
        pList->nArrySize=0;

        if( pList->nSize > 0)
        {
            int nSize = sizeof(CPS_USER_T) * pList->nSize;
            pList->arry = (void **)malloc(nSize);
            if( NULL != pList->arry)
            {
                memset( pList->arry, 0x00, nSize);

                CPS_USER_T * pArryUser = (CPS_USER_T *)pList->arry;

                CPS_USER_T * p = NULL;

               if(nSortChip == 1)
               {
                   //只找出在玩用户
                   for(int i=0;i<pList->nSize;i++)
                   {
                      p =  (CPS_USER_T *)pList->pTable[i];
                      if(p && p->nCpsID == nCpsID && p->nType == nType)
                      {
                          if(p->nStart == 1)
                          {
                              CpsUserCopy(&pArryUser[pList->nArrySize],p);
                              pList->nArrySize++;
                          }
                      }
                   }

                   //按筹码排序
                    CpsSortHandChip(pArryUser,0,pList->nArrySize,1);
               }
               else if(nSortChip == 2)
               {
                   //找出所有用户
                   for(int i=0;i<pList->nSize;i++)
                   {
                      p =  (CPS_USER_T *)pList->pTable[i];
                      if(p && p->nCpsID == nCpsID && p->nType == nType)
                      {
                          CpsUserCopy(&pArryUser[pList->nArrySize],p);
                          pList->nArrySize++;
                      }
                   }
                    //需要按筹码排序
                   if(pList->nArrySize >= 2)
                   {
                       //按在玩跟非玩排序
                        CpsSortHandChip(pArryUser,0,pList->nArrySize,0);

                        int nOnSeatUsercount = 0;
                        for(int i=0;i<pList->nArrySize;i++)
                        {
                            if(pArryUser[i].nStart == 0)
                            {
                                nOnSeatUsercount = i;
                                break;
                            }
                        }

                        //按筹码
                        CpsSortHandChip(pArryUser,0,nOnSeatUsercount,1);

                        //按退出时间
                        CpsSortHandChip(pArryUser,nOnSeatUsercount,pList->nArrySize - nOnSeatUsercount,2);

                        //按筹码
                        int nExitTime = 0;
                        int nStartIndex = 0;
                        for(int i=nOnSeatUsercount;i<pList->nArrySize;i++)
                        {
                            if(nExitTime <= 0)
                            {
                               nExitTime = pArryUser[i].nFailTime;
                               nStartIndex = i;
                            }
                            else
                            {
                                if(pArryUser[i].nFailTime !=nExitTime)
                                {
                                    if(i - nStartIndex >= 2)
                                    {
                                       CpsSortHandChip(pArryUser,nStartIndex,i-nStartIndex,1);
                                       nExitTime = 0;
                                       nStartIndex = 0;
                                    }
                                    else
                                    {
                                       nExitTime = pArryUser[i].nFailTime;
                                       nStartIndex = i;
                                    }
                                }
                                else
                                {
                                    //相同
                                    if(i == pList->nArrySize -1)
                                    {
                                        CpsSortHandChip(pArryUser,nStartIndex,i-nStartIndex+1,1);
                                    }
                                    else
                                    {
                                        //不是最后一个，不用做什么
                                    }
                                }
                            }
                        }
                   }
               }
               else if(nSortChip == 3)
               {
                   //只找出在玩用户
                   for(int i=0;i<pList->nSize;i++)
                   {
                      p =  (CPS_USER_T *)pList->pTable[i];
                      if(p && p->nCpsID == nCpsID && p->nType == nType)
                      {
                          if(p->nStart == 1)
                          {
                              CpsUserCopy(&pArryUser[pList->nArrySize],p);
                              pList->nArrySize++;
                          }
                      }
                   }

                   //按筹码排序
                    CpsSortHandChip(pArryUser,0,pList->nArrySize,3);
               }
               else
               {
                   //不需要排序
                   for(int i=0;i<pList->nSize;i++)
                   {
                      p =  (CPS_USER_T *)pList->pTable[i];
                      if(p && p->nCpsID == nCpsID && p->nType == nType)
                      {
                          CpsUserCopy(&pArryUser[pList->nArrySize],p);
                          pList->nArrySize++;
                      }
                   }
               }
            }
        }
    }
	return pList;
}
int CpsUserGetOnline(int nType,int nCpsID)
{
    int nRet = 0;

    SAFE_HASH_LIST_T * pList = g_CpsUser.GetList();

    if(pList)
    {
        CPS_USER_T * p = NULL;
        for(int i=0;i<pList->nSize;i++)
        {
            p = (CPS_USER_T *)pList->pTable[i];
            if(p && p->nCpsID == nCpsID && p->nType == nType && p->nStart == 1)
            {
                nRet++;
            }
        }

        g_CpsUser.FreeList(pList);
    }
    return nRet;
}
/*获得全部用户释放函数*/
int CpsUserFreeList(SAFE_HASH_LIST_T * pList)
{
    if(pList && pList->arry)
    {
        free(pList->arry);
        pList->arry = NULL;
        pList->nArrySize = 0;
    }
	return g_CpsUser.FreeList(pList);
}
//获得所有记录个数
int CpsUserGetCount()
{
    return g_CpsUser.GetSize();
}
int CpsUserCopy(CPS_USER_T *p,CPS_USER_T *p1)
{
    if(p && p1)
    {
        p->nCpsID = p1->nCpsID;
        p->nUserID = p1->nUserID;
        p->nCurHandChip = p1->nCurHandChip;
        p->nStart = p1->nStart;
        p->nType = p1->nType ;
        p->nFailTime = p1->nFailTime;
        p->nActiveTime = p1->nActiveTime;
        p->nEnterTime = p1->nEnterTime;
        p->nUserLeaveGame = p1->nUserLeaveGame;
    }
    return 0;
}
