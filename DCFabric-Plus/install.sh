#
# GNFlush SDN Controller GPL Source Code
# Copyright (C) 2016, Greenet <greenet@greenet.net.cn>
#
# This file is part of the GNFlush SDN Controller. GNFlush SDN
# Controller is a free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, , see <http://www.gnu.org/licenses/>.
#

##############################################################################
#                                                                            #
#  File Name   : install.sh           *
#  Author      : bnc zgzhao           *
#  Create Date : 2016-6-30           *
#  Version     : 1.0           *
#  Function    : .           *
#                                                                            #
##############################################################################
#!/bin/sh

INSTALL_TYPE=$1
INSTALL_PATH=$2
DATE=`date +%Y%m%d%H%M%S`
MODULE_NAME=DCFabric-controller
INSTALL_DIR=$INSTALL_PATH/$MODULE_NAME

show_help()
{
    echo "\033[0;31;1minvalid parameter !\033[0m"
    echo "eg: sh install.sh install_type install_path"
    echo "install_type: 1. install 2. uninstall 3. reinstall"
    echo "install_path: /home/bnc"
}

check_path()
{
    if [ "$INSTALL_PATH" = "" ];then
        echo "\033[0;31;1minvalid install_path !\033[0m"
        show_help
        return 1
    fi

    LEN=`expr length $INSTALL_PATH`
    LAST=`echo $INSTALL_PATH | cut -c $LEN`
    if [ "${LAST}" = "/" ];then
        LEN=`expr $LEN - 1`
        INSTALL_PATH=`echo $INSTALL_PATH | cut -c 1-$LEN`
        echo "install_path=$INSTALL_PATH"
    fi

    INSTALL_DIR=$INSTALL_PATH/$MODULE_NAME
    return 0
}

install_module()
{
    check_path
    if [ $? -eq 1 ];then
        return 1
    fi

    if [ -d $INSTALL_DIR ];then
        tar -zcvf $MODULE_NAME-$DATE.tar.gz $INSTALL_DIR
        rm -rf $INSTALL_DIR
    fi

    mkdir -p $INSTALL_DIR
    cp -rf bin $INSTALL_DIR
    cp -rf conf $INSTALL_DIR
    cp -rf inc $INSTALL_DIR
    cp -rf log $INSTALL_DIR
    cp -rf lib $INSTALL_DIR
    chmod u+x $INSTALL_DIR/bin/*
    chmod u+x $INSTALL_DIR/lib/*

    cd $INSTALL_DIR/bin
    sh start.sh
    cd -
    return 0
}

uninstall_module()
{
    check_path
    if [ $? -eq 1 ];then
        return 1
    fi

    if [ -d $INSTALL_DIR ];then
        cd $INSTALL_DIR/bin
        sh stop.sh
        cd -
        tar -zcvf $INSTALL_PATH/$MODULE_NAME-$DATE.tar.gz $INSTALL_DIR
        rm -rf $INSTALL_DIR
    fi
    return 0
}


case $INSTALL_TYPE in
    install)
        echo "+----------------------------------------------------+"
        echo "+           Begin to install DCFabric ...            +"
        echo "+----------------------------------------------------+"
        install_module
        echo "+----------------------------------------------------+"
        echo "+           Install DCFabric Success                 +"
        echo "+----------------------------------------------------+"
        ;;
    uninstall)
        echo "+----------------------------------------------------+"
        echo "+           Begin to uninstall DCFabric ...          +"
        echo "+----------------------------------------------------+"
        uninstall_module
        echo "+----------------------------------------------------+"
        echo "+           Uninstall DCFabric Success               +"
        echo "+----------------------------------------------------+"
        ;;
    reinstall)
        echo "+----------------------------------------------------+"
        echo "+           Begin to reinstall DCFabric ...          +"
        echo "+----------------------------------------------------+"
        uninstall_module
        install_module
        echo "+----------------------------------------------------+"
        echo "+           Reinstall DCFabric Success               +"
        echo "+----------------------------------------------------+"
        ;;
    *)
        show_help
        ;;
esac
