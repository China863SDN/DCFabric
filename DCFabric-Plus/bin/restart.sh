#!/bin/sh

sh stop.sh
sh start.sh
if [ $? -eq 0 ];then
    echo "Restart DCFabric Success"
else
    echo "Restart DCFabric Failure"
fi
