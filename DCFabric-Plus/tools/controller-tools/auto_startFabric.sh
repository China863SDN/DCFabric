#!/bin/sh
iptables -F

cd /root/DCFabric_msgTree_C++/DCFabric-controller
while true
do
	ps -aux |grep DCFabric_c++ | grep -v grep	
	if [ $? -ne 0 ]
	then
		sh ./tools/controller-tools/delete_virtual_ip.sh
		nohup ./DCFabric_c++ &
	fi
	sleep 10s
done
