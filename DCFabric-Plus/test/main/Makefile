INCLUDE += -I./../../inc 
INCLUDE += -I./../../inc/openflow 
INCLUDE += -I.

CFLAGS  += -g -O0 -mcmodel=medium -Wall -Wno-reorder -lpthread
CFLAGS += $(INCLUDE)

TARGET = trafficGenerator

all:
	g++ -o $(TARGET) $(CFLAGS) main.cpp CTimer.cpp CThread.cpp CSwitchSimulator.cpp

clean:
	$(RM) -f $(TARGET) *.o 