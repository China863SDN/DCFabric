
#!/bin/sh
while true;do
	ps -aux |grep DCFabric | grep -v grep	
	if [ $? -ne 0 ]
	then
		chmod +x DCFabric_start.sh
		echo "DCFabric start process....."
		sh ./DCFabric_start.sh &

		echo "kill keepalived process"
		pkill keepalived
		echo "delay 3s, then start keepalived process"
		sleep 3s
		sh /etc/keepalived/keepalived_start.sh
	#else
		#echo "DCFabric is runing....."
	fi
	sleep 7s
done
