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

ifneq ($(TYPE), release)
ifneq ($(TYPE), test)
#for debug
CC=gcc
LIBS+=-lpthread
TARGET = ./$(PRODUCT)_debug
else
#for test
CC=gcc -pg
LIBS+=$(PRO_DIR)/test/bin/gprof-helper.so
TARGET = ./$(PRODUCT)_test
endif
else
#for release
CC=colorgcc
LIBS+=-lpthread
#hbase config
LIBS+=-ljvm
HBASEINC=./third/hbase/inc
CFLAGS += -I$(HBASEINC)
CFLAGS += -D HBASE_CLIENT
HBASE=$(OPath)/cluster-mgr/hbase_client.o
TARGET = ./$(PRODUCT)
endif

LIBS+=./third/json-lib/libmjson.a
LIBS+=./third/zookeeper/libzookeeper_mt.a
LIBS+=./third/httpd-lib/libmicrohttpd.a
LIBS+=-lrt
LIBS+=-lm
LIBS+=-l:libuuid.so.1

CFLAGS += -O0 -Wno-strict-aliasing -mcmodel=medium

#BIN_TYPE: 0,real; 1,reverse flow mod test; 2,local buffer flow mod test;
#CFLAGS += -D BIN_TYPE=0


JAVAINC=$(JAVA_HOME)/include
JAVALINUXINC=$(JAVA_HOME)/include/linux
FLUSHINC=inc/ -I inc/openflow -I inc/fabric -I inc/openstack
JSONINC=./third/json-lib/inc
ZOOKEEPERINC=./third/zookeeper/inc
HTTPDINC=./third/httpd-lib/inc

CFLAGS += -I$(JAVAINC)
CFLAGS += -I$(JAVALINUXINC)
CFLAGS += -Wall -I$(JAVAINC) -I$(JAVALINUXINC) -I$(FLUSHINC) -I$(LIBINC) -I$(LIBNETINC) -I$(JSONINC) -I$(ZOOKEEPERINC) -I$(HTTPDINC)

# all object files.
#OBJ=$(OPath)/common/ini.o $(OPath)/common/toolkits.o $(OPath)/mem_pool.o $(OPath)/timer.o $(OPath)/flow_table.o $(OPath)/json_server.o $(OPath)/stats_manager.o $(OPath)/meter_manager.o $(OPath)/tenant_manager.o $(OPath)/sdn_cluster.o $(OPath)/L2.o $(OPath)/ovsdb.o $(OPath)/main.o $(OPath)/of_server.o $(OPath)/of_proc.o $(OPath)/of_msg.o $(OPath)/lldp.o $(OPath)/shortest_path.o $(OPath)/user.o $(OPath)/cmd_server.o $(OPath)/cmd_tools.o $(OPath)/rest_httpd.o  $(OPath)/L3.o $(OPath)/firewall.o  $(OPath)/static_flowentry.o $(HBASE)
OBJ=$(OPath)/common/ini.o $(OPath)/common/common.o $(OPath)/common/error_info.o $(OPath)/common/mem_pool.o $(OPath)/common/timer.o $(OPath)/conn-svr/conn-svr.o $(OPath)/conn-svr/msg_handler_common.o $(OPath)/conn-svr/msg_handler_of10.o $(OPath)/conn-svr/msg_handler_of13.o $(OPath)/cluster-mgr/cluster-mgr.o $(OPath)/restful-svr/restful-svr.o $(OPath)/restful-svr/json_server.o $(OPath)/topo-mgr/topo-mgr.o $(OPath)/meter-mgr/meter-mgr.o $(OPath)/group-mgr/group-mgr.o $(OPath)/flow-mgr/flow-mgr.o $(OPath)/user-mgr/user-mgr.o $(OPath)/forward-mgr/forward-mgr.o $(OPath)/forward-mgr/l2.o  $(OPath)/forward-mgr/l3.o $(OPath)/tenant-mgr/tenant-mgr.o  $(OPath)/stats-mgr/stats-mgr.o $(OPath)/ovsdb/ovsdb.o $(OPath)/platform/main.o $(OPath)/fabric/fabric_path.o $(OPath)/fabric/fabric_impl.o $(OPath)/fabric/fabric_flows.o $(OPath)/fabric/fabric_arp.o $(OPath)/fabric/fabric_host.o $(OPath)/fabric/fabric_thread.o $(OPath)/event/event_service.o $(OPath)/openstack/openstack_host.o $(OPath)/openstack/openstack_app.o $(OPath)/fabric/fabric_openstack_arp.o $(OPath)/restful-svr/openstack-server.o $(OPath)/openstack/fabric_openstack_nat.o $(OPath)/openstack/fabric_openstack_external.o $(OPath)/fabric/fabric_floating_ip.o $(OPath)/openstack/openstack_security_app.o $(OPath)/openstack/openstack_firewall_app.o
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
