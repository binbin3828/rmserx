/*
*******************************************************************************
**  Copyright (c) 2012, �����з���˹�Ƽ����޹�˾, All rights reserved.
**  ���ߣ�oscar
**  ��ǰ�汾��v1.0
**  ��������: 2012.02.21
**  �ļ�˵��: ���SIU ��һЩ��ʱ����
*******************************************************************************
*/

#ifndef __SYSCONFIG_H__
#define __SYSCONFIG_H__

int SysConfInit(char *pFileName);

/*���ض�����ֵ*/
char *SysConfGetChar(char *pName);

/*���ض�����ֵ*/
int SysConfGetInt(char *pName);

#endif //__SYSCONFIG_H__

