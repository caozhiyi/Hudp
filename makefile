SRCS = $(wildcard ./base/*.cpp ./common/*.cpp ./hudp/*.cpp ./HudpServer/*.cpp ./HudpServer/*.cpp ./net/*.cpp ./net/unix/*.cpp)

OBJS = $(SRCS:.cpp = .o)

CC = g++

INCLUDES = -I.          \
           -I./base     \
           -I./net      \
           -I./net/unix \
           -I./common   \
           -I./hudp     \
           -I./include  \

CCFLAGS = -lpthread -fPIC -m64 -std=c++11 -lstdc++ -fpermissive

OUTPUT = HudpServer

all:$(OUTPUT)

$(OUTPUT) : $(OBJS)
	$(CC) $^ -o $@ $(INCLUDES)  $(CCFLAGS)

%.o : %.c
	$(CC) -c $< $(CCFLAGS)

clean:
	rm -rf *.out *.o

.PHONY:clean