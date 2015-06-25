/*
*******************************************************************************
**  Copyright (c) 2012, 深圳市飞瑞斯科技有限公司, All rights reserved.
**  作者：oscar
**  当前版本：v1.0
**  创建日期: 2012.02.21
**  文件说明: 存放SIU 的一些临时配置
*******************************************************************************
*/

#ifndef __SYSCONFIG_H__
#define __SYSCONFIG_H__

int SysConfInit(char *pFileName);

/*返回读到的值*/
char *SysConfGetChar(char *pName);

/*返回读到的值*/
int SysConfGetInt(char *pName);

#endif //__SYSCONFIG_H__

