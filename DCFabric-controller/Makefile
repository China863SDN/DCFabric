RM=rm
MV=mv
CD=cd
CP=cp

PRODUCT=DCFabric

ifeq (,$(PRO_DIR))
PRO_DIR =$(shell pwd)/src
endif

#Out file path
OPath=$(PRO_DIR)

CFLAGS = -g -Wall -c
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
LIBS+=./third/redis-lib/libhiredis.a
LIBS+=-lrt
LIBS+=-lm
LIBS+=-l:libuuid.so.1
LIBS+=-lpthread


CFLAGS += -O0 -Wno-strict-aliasing -mcmodel=medium
CFLAGS += -DTHREADED

#BIN_TYPE: 0,real; 1,reverse flow mod test; 2,local buffer flow mod test;
#CFLAGS += -D BIN_TYPE=0



FLUSHINC=./inc -I ./inc/openflow -I ./inc/fabric -I ./inc/openstack
JSONINC=./third/json-lib/inc
ZOOKEEPERINC=./third/zookeeper/inc
HTTPDINC=./third/httpd-lib/inc
HBASEINC=./third/redis-lib/inc


INCPATH= -I$(FLUSHINC) -I$(LIBINC) -I$(LIBNETINC) -I$(JSONINC) -I$(ZOOKEEPERINC) -I$(HTTPDINC) -I$(HBASEINC)
CFLAGS += $(INCPATH)

# all source and object files.
SRC = $(wildcard ${PRO_DIR}/*/*.c)   
OBJ = ${patsubst %.c, %.o, ${SRC}}
DOBJ = ${patsubst %.c, %.d, ${SRC}}

all:$(OBJ)
	@echo "+----------------------------------------------------+"
	@echo "+     Start building $(PRODUCT) module......     +"
	@echo "+----------------------------------------------------+"
	$(CC) -g -o $(TARGET) $(OBJ) $(LIBS)
	@echo ""
	@echo "+----------------------------------------------------+"
	@echo "+     Finished building $(PRODUCT) program       +"
	@echo "+----------------------------------------------------+"


%.o: %.c
	$(CC) $(CFLAGS) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -MT"$(@:%.o=%.d)" -o "$@" "$<"



clean:
	$(CD) $(OPath) && $(RM) -f $(OBJ) && $(RM) -f $(DOBJ)
	$(RM) -f ./$(PRODUCT)
	$(RM) -f ./$(PRODUCT)_test
	$(RM) -f ./$(PRODUCT)_debug
	
ctags:
	ctags -R --c++-kinds=+px --fields=+iaS  --extra=+q
