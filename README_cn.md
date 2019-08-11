<p align="left"><img width="340" src="./doc/image/logo.png" alt="cppnet logo"></p>

<p align="left">
    <a href="https://travis-ci.org/caozhiyi/Hudp"><img src="https://travis-ci.org/caozhiyi/Hudp.svg?branch=master" alt="Build Status"></a>
    <a href="https://opensource.org/licenses/BSD-3-Clause"><img src="https://img.shields.io/badge/license-bsd-orange.svg" alt="Licenses"></a>
</p> 

## 简介

Hudp 是一个rudp库， rudp即Reliable UDP，可靠的udp传输。相较于tcp强制不可选择的可靠传输，udp更像是一张白纸，可以自己定义需要的可靠。    
Hudp 提供选项宏来供上层使用，以控制消息传输实现不同的可靠性和优先级。   
Hudp 并不在初始化库和连接时限定传输可靠性，用户可以在发送每个消息的时候选择不同的可靠性选项，实现最细粒度的控制。   
Hudp 只有在用到相应资源时才进行初始化，比如发送窗体和接收顺序队列，以保证使用时最少的资源消耗。   
Hudp 通过一个消息池来控制消息的创建和销毁， 实现常用资源的快速创建。   
Hudp 通过一个责任链控制发送消息的包体处理过程(类似nginx)，可以快速便捷的嵌入额外的处理过程。   
Hudp 还在进一步完善。。。   

## 选项
Hudp提供四种关于可靠性的选项：
```cpp
 enum hudp_tran_flag {
        // only orderly. may lost some bag
        HTF_ORDERLY          = 0x0001，
        // only reliable. may be disorder
        HTF_RELIABLE         = 0x0002，
        // reliable and orderly like tcp
        HTF_RELIABLE_ORDERLY = 0x0004，
        // no other contral. only udp
        HTF_NORMAL           = 0x0008
    };
```
HTF_ORDERLY : 只保证顺序性不保证可靠性，这意味着在接收端接收到网络抖动延迟到达的包时，可能直接丢弃不通知上层。   
HTF_RELIABLE : 只保证可靠性不保证顺序性，上层可能接受到乱序的包，但是每个包都保证被送达。   
HTF_RELIABLE_ORDERLY : 即保证可靠性，又保证顺序性，看起来像是tcp，但不同之处在于上层可以通过回调参与到可靠传输的各个方面。   
HTF_NORMAL : 普通udp传输，没有任何控制的udp传输。   

Hudp提供四种优先级选项：
```cpp
enum hudp_pri_flag {
        // the lowest priority.
        HPF_LOW_PRI          = 0x0010，
        // the normal priority.
        HPF_NROMAL_PRI       = 0x0020，
        // the high priority.
        HPF_HIGH_PRI         = 0x0040，
        // the highest priority.
        HPF_HIGHEST_PRI      = 0x0080
    };
```
优先级依次递增，发送时当所有级别的优先级消息不为空，按照发送两个高优先级消息一个低优先级消息的规则执行发送过程。   
这意味着每发送2个HPF_HIGHEST_PRI，发送1个HPF_HIGH_PRI，每发送8个HPF_HIGHEST_PRI，发送1个HPF_LOW_PRI。   
当然如果只有一个消息时，不论任何优先级都会被发送。   

## 接口
Hudp提供了最少的接口供用户使用，使用起来就像是在用原始的udp接口
```cpp
    void Init(bool log = false);
    bool Start(uint16_t port， const recv_back& func);
    bool Start(const std::string& ip，uint16_t port， const recv_back& func);
    void Join();
    void SendTo(const HudpHandle& handlle， uint16_t flag， const std::string& msg);
    void SendTo(const HudpHandle& handlle， uint16_t flag， const char* msg， uint16_t len);
    void Destroy(const HudpHandle& handlle);
```
对应的接口注释可以查看[Hudp](/include/Hudp.h)。   
Start接口会开启对应端口的监听，有消息到达时会在回调函数中收到通知。   
SendTo接口发送消息到对端，HudpHandle实际上是ip+port的字符串表示。flag参数可以控制本次发送的可靠性和优先级，你需要各选择其中一项组合来发送本次消息，类似：HPF_NROMAL_PRI | HTF_RELIABLE_ORDERLY。   

## 实例

请看[HudpClient](/HudpClient/HudpClient.cpp)和[HudpServer](/HudpServer/HudpServer.cpp)。   
但不要误解server和client的名字，其实对于Hudp来讲两端相同，只要调用了Start接口就有了接收消息的能力。

## 编译(Windows)

你可以使用vs2017来编译Hudp库和示例。

## 编译(Linux)

只需要在源码目录下执行make即可编译Hudp库和示例。
```
$ make
```

## 协议

Hudp使用BSD 3-Clause使用条款，详情请看[https://opensource.org/licenses/BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)。