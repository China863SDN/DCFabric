#!/bin/sh
conf=./conf/controller.conf
itf=$(sed '/controller_eth/!d;s/.*=//' $conf)
vip=$(sed '/virtual_ip/!d;s/.*=//' $conf)

echo "ip addr del $vip dev $itf"
ip addr del $vip dev $itf
