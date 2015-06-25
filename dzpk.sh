#!/bin/sh
# 
# Name:  dzpk.sh 
# Func:  dzpk game server start,stop,restart shell script.
# Autor: guobin
# Date:  2015/06/17 

CheckProcess()
{
	if [ "$1" = "" ];then
		return 1
	fi
	PROCESS_NUM=`ps -ef | grep "$1" | grep -v "grep" | wc -l`
	if [ $PROCESS_NUM -eq 1 ];then 
		return 0
	else
		return 1
	fi
}

DoBeforeStart()
{
	ulimit -c unlimited
	myPath="/data/core/"
	if [ ! -d "$myPath" ]; then
		mkdir "$myPath"
	fi
	/sbin/sysctl -w kernel.core_pattern=/data/core/%e.core.%t > /dev/null
	#chmod a+rwx rmserx 
}

StartInfo()
{	
	echo "starting."
		sleep 1
	echo "starting.."
		sleep 1
	echo "starting..."
		sleep 1
	
	PROCESS_NUM=`ps -ef | grep rmserx | grep -v "grep" | wc -l`
	if [ $PROCESS_NUM -eq 1 ];then 
		echo "start SUCCESS."
	else
		echo "start FAILURE."
	fi
}

StopInfo()
{
	echo "stopping..."
	sleep 1
	PROCESS_NUM=`ps -ef | grep rmserx | grep -v "grep" | wc -l`
	if [ $PROCESS_NUM -eq 0 ];then 
		echo "stop SUCCESS."
	else
		echo "stop FAILURE."
	fi
}


case "$1" in
	status)
	CheckProcess "rmserx"
	Ret=$?
	if [ $Ret -eq 0 ]; then
		echo "dzpk server is running."		
	else
		echo "dzpk server is no running."
	fi
			
	;;

	start)
	CheckProcess "rmserx"
	Ret=$?
	
	if [ $Ret -eq 0 ];then
		echo "dzpk server is runing now."
	else
		DoBeforeStart 
		exec ./bin/Release/rmserx > /dev/null &
		StartInfo
	fi	        	
	;;

	stop)
	CheckProcess "rmserx"
	Ret=$?
	if [ $Ret -eq 0 ];then
		killall -9 rmserx
		StopInfo
	else
		echo "dzpk server is no runing, you can start."
	fi
	;;

	restart)
	CheckProcess "rmserx"
	Ret=$?
	if [ $Ret -eq 0 ];then
		killall -9 rmserx
		StopInfo
		DoBeforeStart
		exec ./bin/Release/rmserx > /dev/null &
		StartInfo
	else
		echo "unnecessary stop, stop FAILURE."
		DoBeforeStart
		exec ./bin/Release/rmserx > /dev/null &
		StartInfo
	fi
	;;
	
	*)
	echo "Usage: $0 start|stop|restart|status"
esac	

