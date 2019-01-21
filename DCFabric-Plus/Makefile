RM=rm
MV=mv
CD=cd
CP=cp

PRODUCT = DCFabric_c++

ifeq (,$(PRO_DIR))
PRO_DIR = $(shell pwd)/src
endif

#Out file path
OPath 	= $(PRO_DIR)

CC      = g++
TARGET  = ./$(PRODUCT)
CFLAGS  += -g -O0 -mcmodel=medium -Wall -Wno-reorder 
#CFLAGS  += -DLOG4CPLUS
#CFLAGS  += -DLOG4PRINT

# all library
LIBS	+= -lpthread -ldl -l:libuuid.so.1
LIBS	+= -L ./lib/libjemalloc.a
LIBS 	+= ./lib/liblog4cplus.a
LIBS 	+= ./lib/libhiredis.a
LIBS 	+= ./lib/libsqlite3.so
LIBS 	+= ./lib/libtbb.a
LIBS 	+= ./lib/libzookeeper_mt.a
LIBS 	+= ./lib/libevent.a
LIBS 	+= ./lib/libmjson.a

# all include path
INCLUDE += -I./inc 
INCLUDE += -I./inc/openflow 
INCLUDE += -I./inc/log4cplus 
INCLUDE += -I./inc/jemalloc 
INCLUDE += -I./inc/rapidjson 
INCLUDE += -I./inc/redis 
INCLUDE += -I./inc/sqlite
INCLUDE += -I./inc/tbb 
INCLUDE += -I./inc/zookeeper 
INCLUDE += -I./inc/libevent 
INCLUDE += -I./inc/json 
INCLUDE += -I$(OPath)/common
INCLUDE += -I$(OPath)/config 
INCLUDE += -I$(OPath)/control
INCLUDE += -I$(OPath)/handler 
INCLUDE += -I$(OPath)/log 
INCLUDE += -I$(OPath)/msgtree 
INCLUDE += -I$(OPath)/restful
INCLUDE += -I$(OPath)/server
INCLUDE += -I$(OPath)/switch
INCLUDE += -I$(OPath)/notification
INCLUDE += -I$(OPath)/openstack
INCLUDE += -I$(OPath)/connect
INCLUDE += -I$(OPath)/host
INCLUDE += -I$(OPath)/flow
INCLUDE += -I$(OPath)/utils  
INCLUDE += -I$(OPath)/resmgr
INCLUDE += -I$(OPath)/proxyres
INCLUDE += -I$(OPath)/localres
INCLUDE += -I$(OPath)/service
INCLUDE += -I$(OPath)/event
INCLUDE += -I$(OPath)/clusternew
INCLUDE += -I$(OPath)/flood
INCLUDE += -I$(OPath)/networking
INCLUDE += -I$(OPath)/nat
INCLUDE += -I$(OPath)/ovsdb

CFLAGS += $(INCLUDE)

# all source and object files.
SRC = $(wildcard ${PRO_DIR}/*/*.cpp)   
OBJ = ${patsubst %.cpp, %.o, ${SRC}}
DOBJ = ${patsubst %.cpp, %.d, ${SRC}}


all:$(OBJ)
	@echo "+----------------------------------------------------+"
	@echo "+     Start building $(PRODUCT) module......     +"
	@echo "+----------------------------------------------------+"
	@echo $(PRO_DIR)
	@echo $(SRC)
	$(CC) -rdynamic -o $(TARGET) $(OBJ) $(LIBS)
	@echo ""
	@echo "+----------------------------------------------------+"
	@echo "+     Finished building $(PRODUCT) program       +"
	@echo "+----------------------------------------------------+"


$(OPath)/%.o:$(OPath)/%.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<
#	$(CC) $(CFLAGS) -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -MT"$(@:%.o=%.d)" -o "$@" "$<"

clean:
	$(CD) $(OPath) && $(RM) -f $(OBJ) && $(RM) -f $(DOBJ)
	$(RM) -f ./$(PRODUCT)
