SRCS = $(wildcard ./base/*.cpp ./common/*.cpp ./hudp/*.cpp ./net/*.cpp ./net/unix/*.cpp ./os/unix/*.cpp) 

OBJS = $(patsubst %.cpp, %.o, $(SRCS))

CC = g++

OUTPUT = output

INCLUDES = -I.          \
           -I./base     \
           -I./net      \
           -I./net/unix \
           -I./common   \
           -I./hudp     \
           -I./include  \
		   -I./os/unix  \
# NET_LOSS_TEST add packet loss -D NET_LOSS_TEST
CCFLAGS = -lpthread -fPIC -m64 -std=c++11 -lstdc++

TARGET = libhudp.a

all:$(TARGET)

$(TARGET):$(OBJS)
	ar rcs $@ $^

%.o:%.cpp
	$(CC) -c $? -o $@ $(CCFLAGS) $(INCLUDES) 

clean:
	rm -rf $(OBJS) $(TARGET) $(SERBIN) $(CLIBIN)
