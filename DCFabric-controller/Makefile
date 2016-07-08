dRM=rm
MV=mv
CD=cd
CP=cp

PRODUCT=DCFabric

ifeq (,$(PRO_DIR))
PRO_DIR =$(shell pwd)/src
endif

#Out file path
OPath=$(PRO_DIR)

CFLAGS = -g
CFLAGS += -D VERSION=0x020000

ifneq ($(TYPE), test)
CC=gcc
TARGET = ./$(PRODUCT)
else
CC=gcc -pg
LIBS+=$(PRO_DIR)/test/bin/gprof-helper.so
TARGET = ./$(PRODUCT)_test
endif

LIBS+=./third/json-lib/libmjson.a
LIBS+=./third/zookeeper/libzookeeper_mt.a
LIBS+=./third/httpd-lib/libmicrohttpd.a
LIBS+=-lrt
LIBS+=-lm
LIBS+=-l:libuuid.so.1
LIBS+=-lpthread
LIBS+=-ljvm

CFLAGS += -O0 -Wno-strict-aliasing -mcmodel=medium

#BIN_TYPE: 0,real; 1,reverse flow mod test; 2,local buffer flow mod test;
#CFLAGS += -D BIN_TYPE=0


JAVAINC=$(JAVA_HOME)/include
JAVALINUXINC=$(JAVA_HOME)/include/linux
FLUSHINC=inc/ -I inc/openflow -I inc/fabric -I inc/openstack
JSONINC=./third/json-lib/inc
ZOOKEEPERINC=./third/zookeeper/inc
HTTPDINC=./third/httpd-lib/inc
HBASEINC=./third/hbase/inc

CFLAGS += -I$(JAVAINC)
CFLAGS += -I$(JAVALINUXINC)
CFLAGS += -Wall -I$(JAVAINC) -I$(JAVALINUXINC) -I$(FLUSHINC) -I$(LIBINC) -I$(LIBNETINC) -I$(JSONINC) -I$(ZOOKEEPERINC) -I$(HTTPDINC) -I$(HBASEINC)

# all object files.
OBJ=$(OPath)/common/ini.o $(OPath)/common/common.o $(OPath)/common/error_info.o $(OPath)/common/mem_pool.o $(OPath)/common/timer.o $(OPath)/conn-svr/conn-svr.o $(OPath)/conn-svr/msg_handler_common.o $(OPath)/conn-svr/msg_handler_of10.o $(OPath)/conn-svr/msg_handler_of13.o $(OPath)/cluster-mgr/cluster-mgr.o $(OPath)/restful-svr/restful-svr.o $(OPath)/restful-svr/json_server.o $(OPath)/topo-mgr/topo-mgr.o $(OPath)/meter-mgr/meter-mgr.o $(OPath)/group-mgr/group-mgr.o $(OPath)/flow-mgr/flow-mgr.o $(OPath)/user-mgr/user-mgr.o $(OPath)/forward-mgr/forward-mgr.o $(OPath)/forward-mgr/l2.o  $(OPath)/forward-mgr/l3.o $(OPath)/tenant-mgr/tenant-mgr.o  $(OPath)/stats-mgr/stats-mgr.o $(OPath)/ovsdb/ovsdb.o $(OPath)/platform/main.o $(OPath)/fabric/fabric_path.o $(OPath)/fabric/fabric_impl.o $(OPath)/fabric/fabric_flows.o $(OPath)/fabric/fabric_arp.o $(OPath)/fabric/fabric_host.o $(OPath)/fabric/fabric_thread.o $(OPath)/event/event_service.o $(OPath)/openstack/openstack_host.o $(OPath)/openstack/openstack_app.o $(OPath)/fabric/fabric_openstack_arp.o $(OPath)/restful-svr/openstack-server.o $(OPath)/openstack/fabric_openstack_nat.o $(OPath)/openstack/fabric_openstack_external.o $(OPath)/fabric/fabric_floating_ip.o $(OPath)/openstack/openstack_security_app.o $(OPath)/openstack/openstack_firewall_app.o $(OPath)/openstack/openstack_lbaas_app.o $(OPath)/fabric/fabric_stats.o $(OPath)/restful-svr/debug_svr.o $(OPath)/cluster-mgr/hbase_client.o $(OPath)/cluster-mgr/hbase_sync.o $(OPath)/overload-mgr/overload-mgr.o $(OPath)/qos-mgr/qos-mgr.o $(OPath)/qos-mgr/qos-queue.o $(OPath)/qos-mgr/qos-queue-ovsdb.o $(OPath)/qos-mgr/qos-meter.o $(OPath)/qos-mgr/qos-policy.o $(OPath)/qos-mgr/qos-policy-file.o
all:$(OBJ)
	@echo "+----------------------------------------------------+"
	@echo "+     Start building $(PRODUCT) module......     +"
	@echo "+----------------------------------------------------+"
	$(CC) -g -o $(TARGET) $(OBJ) $(LIBS)
	@echo ""
	@echo "+----------------------------------------------------+"
	@echo "+     Finished building $(PRODUCT) program       +"
	@echo "+----------------------------------------------------+"


$(OPath)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<



clean:
	$(CD) $(OPath) && $(RM) -f $(OBJ)
	$(RM) -f ./$(PRODUCT)
	$(RM) -f ./$(PRODUCT)_test
	$(RM) -f ./$(PRODUCT)_debug
