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
#  File Name   : build.sh           *
#  Author      : bnc zgzhao           *
#  Create Date : 2016-6-30           *
#  Version     : 1.0           *
#  Function    : .           *
#                                                                            #
##############################################################################
#!/bin/sh
export CPATH="./inc"
export LIBRARY_PATH="./lib/"
export LD_LIBRARY_PATH="./lib/"

MODULE_NAME=DCFabric-controller
MODULE_VERSION=1.0.0
DATE=`date +%Y%m%d`
PKG_NAME=$MODULE_NAME-runtime-install-$MODULE_VERSION-$DATE.tar.gz


echo "+----------------------------------------------------+"
echo "+           Begin to build DCFabric ...              +"
echo "+----------------------------------------------------+"

cd lib
ln -sf libjemalloc.so.2 libjemalloc.so
ln -sf liblog4cplus-1.2.so.5 liblog4cplus.so
cd -

make clean ; make -j4
if [ $? = 0 ];then
    mkdir DCFabric-controller
    cp -rf bin DCFabric-controller
    cp -rf conf DCFabric-controller
    cp -rf inc DCFabric-controller
    cp -rf log DCFabric-controller
    cp -rf lib DCFabric-controller
    cp -rf install.sh DCFabric-controller
    tar -zcvf $PKG_NAME DCFabric-controller
    rm -rf DCFabric-controller
else
    echo "Make Failure"
    exit 1
fi

echo "+----------------------------------------------------+"
echo "+              Build DCFabric Success                +"
echo "+----------------------------------------------------+"