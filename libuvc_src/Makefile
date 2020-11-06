Version=0.1.4
#PREX_CC=arm-linux-gnueabihf-
PREX_CC=

C=gcc
G=g++
#CFLAGS = -Wall -O3 -g -std=gnu99  -DEMBED_PLATFORM -fPIC  -mfloat-abi=hard  -mfpu=vfpv3  -ffast-math -march=armv7-a  -mtune=cortex-a9 
#CFLAGS = -Wall -O3 -g -std=gnu99  -fPIC  -mfloat-abi=softfp  -mfpu=vfpv4-d16  -ffast-math -march=armv7-a  -mtune=cortex-a9 
CFLAGS = -Wall -O3 -g -std=gnu99  -fPIC 

LDFLAGS = -lm
INCLUDE= -I./  -I./include
TARGET = ./libuvc.a
  
%.o:%.c  
	$(PREX_CC)$(C)  $(INCLUDE)   $(CFLAGS) -c $< -o $@  
	  
%.o:%.cpp 
	$(PREX_CC)$(G)  $(INCLUDE)  $(CFLAGS) -c $< -o $@  

DIR_INC = ./include
DIR_SRC = ./lib

SOURCES = $(wildcard *.c *.cpp ${DIR_SRC}/*.c) 
OBJS = $(patsubst %.c,%.o, $(patsubst %.cpp,%.o,$(SOURCES))) 

$(TARGET):$(OBJS)
	$(PREX_CC)ar -crv $(TARGET)  $(OBJS) 
	chmod +x $(TARGET)

clean:  
	rm -rf *.o  $(TARGET)


