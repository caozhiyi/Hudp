SRCS = $(wildcard ./base/*.cpp           \
                  ./common/*.cpp         \
				  ./hudp/*.cpp           \
				  ./net/*.cpp            \
				  ./net/unix/*.cpp       \
				  ./net/controller/*.cpp       \
				  ./net/controller/bbr/*.cpp   \
				  ./os/unix/*.cpp              \
				  ./filter/*.cpp) 

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
		   -I./filter   \
           -I./interface            \
		   -I./third/snappy         \
		   -I./net/controller       \
		   -I./net/controller/bbr   \

# NET_LOSS_TEST add packet loss -D NET_LOSS_TEST
CCFLAGS = -lpthread -fPIC -m64 -std=c++11 -lstdc++ -g

TARGET = libhudp.a

all:$(TARGET)

$(TARGET):$(OBJS)
	ar rcs $@ $^

%.o:%.cpp
	$(CC) -c $? -o $@ $(CCFLAGS) $(INCLUDES) 

clean:
	rm -rf $(OBJS) $(TARGET) $(SERBIN) $(CLIBIN)
