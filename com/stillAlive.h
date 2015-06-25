#ifndef __STILLALIVE_H__
#define __STILLALIVE_H__

/*JaclFrost 2014.2.7 .线程是否活动*/

/*线程心跳,15秒调用一次*/
int PutAliveMark(int nIndex);

/*获得线程标识，一个线程只能调用一次*/
int GetAliveMark();

/*检查线程状态，由主线程调用，间隔时间用户定建议一分钟调用一次*/
int CheckAlive();

#endif  //__STILLALIVE_H__
