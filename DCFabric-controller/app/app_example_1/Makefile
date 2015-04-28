RM=rm
MV=mv
CD=cd
CP=cp

PRODUCT=GNFlush_app_example

ifeq (,$(PRO_DIR))
PRO_DIR =$(shell pwd)/
endif

#Out file path
OPath=$(PRO_DIR)

CFLAGS = -g
CFLAGS += -D VERSION=0x020001

ifneq ($(TYPE), release)
ifneq ($(TYPE), test)
#for debug
CC=colorgcc
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
HBASEINC=../../third/hbase/inc
CFLAGS += -I$(HBASEINC)
CFLAGS += -D HBASE_CLIENT
HBASE=$(OPath)/cluster-mgr/hbase_client.o
TARGET = ./$(PRODUCT)
endif

LIBS+=../../GNFlush.a
LIBS+=../../third/json-lib/libmjson.a
LIBS+=../../third/zookeeper/libzookeeper_mt.a
LIBS+=../../third/httpd-lib/libmicrohttpd.a
LIBS+=-lrt
LIBS+=-lm
LIBS+=-l:libuuid.so.1

CFLAGS += -O0 -Wno-strict-aliasing -mcmodel=medium


JAVAINC=$(JAVA_HOME)/include
JAVALINUXINC=$(JAVA_HOME)/include/linux
FLUSHINC=../../inc/ -I ../../inc/openflow
JSONINC=../../third/json-lib/inc
ZOOKEEPERINC=../../third/zookeeper/inc
HTTPDINC=../../third/httpd-lib/inc

CFLAGS += -I$(JAVAINC)
CFLAGS += -I$(JAVALINUXINC)
CFLAGS += -Wall -I$(JAVAINC) -I$(JAVALINUXINC) -I$(FLUSHINC) -I$(LIBINC) -I$(LIBNETINC) -I$(JSONINC) -I$(ZOOKEEPERINC) -I$(HTTPDINC)

# all object files.
OBJ=$(OPath)/app_example.o

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
