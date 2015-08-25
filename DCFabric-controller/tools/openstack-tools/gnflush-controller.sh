#!/bin/bash

sudo ovs-vsctl del-manager
sudo ovs-vsctl del-br br-int
sudo ovs-vsctl del-br br-tun
sudo ovs-vsctl del-br br-ex 

local_ip=""
provider_mappings=""
gnflush_ip=""
external_provider=""

function usage {
    local rc=$1
    local outstr=$2

    if [ "$outstr" != "" ]; then
        echo "$outstr"
        echo
    fi

    echo "Usage: `basename $0` [OPTION...]"
    echo
    echo "Script options:"
    echo "  --local_ip IP                 IP address of the node, will be used as tunnel endpoint"
    echo "  --provider_mappings MAPPINGS  physical provider mappings, i.e physnet1:eth1,physnet2:eth2"
    echo "  --gnflush_ip IP               IP address of GNFlush controller"
    echo "  --external_provider           physical provider, only for network node"
    echo
    echo "Help options:"
    echo "  -?, -h, --h, --help  Display this help and exit"
    echo

    exit $rc
}

function parse_options {
    while true ; do
        case "$1" in
        --local_ip)
            shift; local_ip="$1"; shift
            ;;

         --provider_mappings)
            shift; provider_mappings="$1"; shift
            ;;

         --gnflush_ip)
            shift; gnflush_ip="$1"; shift
            ;;

         --external_provider)
            shift; external_provider="$1"; shift
            ;;

      -? | -h | --h | --help)
            usage 0
            ;;
        "")
            break
            ;;
        *)
            echo "Ignoring unknown option: $1"; shift;
        esac
    done
}

parse_options "$@"

if [ `whoami` != "root" ]; then
    usage 1 "Please execute this script as superuser or with sudo previleges."
fi

read ovstbl <<< $(ovs-vsctl get Open_vSwitch . _uuid)

if [ -n "$provider_mappings" ]; then
    sudo ovs-vsctl set Open_vSwitch $ovstbl other_config:provider_mappings=$provider_mappings
fi

if [ -n "$local_ip" ]; then
    sudo ovs-vsctl set Open_vSwitch $ovstbl other_config:local_ip=$local_ip
fi

if [ -n "$gnflush_ip" ]; then
    echo "setting gnflush_ip=$gnflush_ip"
    sudo ovs-vsctl set-manager tcp:$gnflush_ip:6640
fi

if [ -n "$external_provider" ]; then
    echo "create external network through $external_provider"
    sudo ifconfig $external_provider promisc
    sudo ovs-vsctl add-br br-ex
    sudo ovs-vsctl add-port br-ex $external_provider
    sudo ovs-vsctl set-controller br-ex tcp:$gnflush_ip:6633
fi

sleep 1
sudo ovs-vsctl show

exit 0