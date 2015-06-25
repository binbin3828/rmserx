#!/bin/sh
#------------------------------------------------------------------------------

ulimit -c unlimited

myPath="/data/core/"

if [ ! -d "$myPath" ];then
    mkdir "$myPath"
fi

/sbin/sysctl -w kernel.core_pattern=/data/core/%e.core.%t

chmod a+rwx rmserx
chmod a+rwx stopRmserx.sh

CheckProcess()
{
  if [ "$1" = "" ];
  then
    return 1
  fi
 
  PROCESS_NUM=`ps -ef | grep "$1" | grep -v "grep" | wc -l` 
  if [ $PROCESS_NUM -eq 1 ];
  then
    return 0
  else
    return 1
  fi
}
  
while [ 1 ] ; do
 CheckProcess "rmserx"
 CheckQQ_RET=$?
 if [ $CheckQQ_RET -eq 1 ];
 then
   killall -9 test
   exec ./rmserx &  
 fi
 sleep 5
done
