#!/bin/sh

export LD_LIBRARY_PATH=../lib/:$LD_LIBRARY_PATH

PID=`ps -ef | grep DCFabric | grep -v grep | awk '{print $2}'`
if [ "$PID" != "" ];then
	kill -9 $PID
fi

echo "Stop DCFabric Success"
exit 0
