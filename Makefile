DEF= -DCOMPILE_DATE=\"$(DATE)\" -DCOMPILE_TIME=\"$(TIME)\"

INC = -I./core -I/usr/include/mysql/ -I/usr/local/tokyocabinet/include/
LIB = -lpthread -ldl -lz -lm  -L//usr/lib/i386-linux-gnu/ -lmysqlclient -L/usr/local/tokyocabinet/lib -ltokyocabinet
#INC = -I./core -I/usr/local/mysql/include/mysql/
#LIB = -lpthread -ldl -lz -lm  -L/usr/local/mysql/lib/mysql/ -lmysqlclient

C_FLAGS =  -O0  $(DEF) -g -ggdb3 -Wall  -DLOG_LEVEL=1

#判断系统架构(32bit, 64bit)
ARCH := $(shell getconf LONG_BIT)
ifeq ($(ARCH),64)
C_FLAGS+= -DMS_PTR_SIZE=8
else
C_FLAGS+= -DMS_PTR_SIZE=4
endif

#C_FLAGS =  -O0  $(DEF) -g -fomit-frame-pointer -Wall  
CXX             = g++ 
CC				= gcc
RANLIB          = ranlib
AR              = ar

SRCS = $(wildcard *.c core/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

TARGET = mysync
all:$(TARGET)

$(TARGET):$(OBJS)
	@echo "Linking $(TARGET) ... "
	@$(CC) $(C_FLAGS)  $(INC) -o $@ $^ $(LIB) 

%.o:%.c
	@echo "Compiling $<  ... "
	@$(CC) $(C_FLAGS)  $(INC) -c -o $@ $<  

db_tool:example/db_tool.c
	@echo "Compiling $<  ... "
	@$(CC) $(C_FLAGS)  $(INC) -o $@ $<  $(LIB)

clean:
	@rm -f *.o ./core/*.o ./example/*.o
	@rm -f ./$(TARGET) ./db_tool

install:
	install mysync ./bin/
