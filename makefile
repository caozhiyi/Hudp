SRCS = $(wildcard ./base/*.cpp ./common/*.cpp ./hudp/*.cpp ./HudpServer/*.cpp ./net/*.cpp ./net/unix/*.cpp)

OBJS = $(patsubst %.cpp, %.o, $(SRCS))

CC = g++

INCLUDES = -I.          \
           -I./base     \
           -I./net      \
           -I./net/unix \
           -I./common   \
           -I./hudp     \
           -I./include  \

CCFLAGS = -lpthread -fPIC -m64 -std=c++11 -lstdc++ -fpermissive

TARGET = libhudp.a
all:$(TARGET)

$(TARGET):$(OBJS)
	ar rcs $(TARGET) $^

$(OBJS):$(SRCS)
	$(CC) $(CCFLAGS) $(INCLUDES) -c $^

clean:
	rm -rf *.a *.o

.PHONY:clean