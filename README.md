<p align="left"><img width="340" src="./doc/image/logo.png" alt="cppnet logo"></p>

<p align="left">
    <a href="https://travis-ci.org/caozhiyi/Hudp"><img src="https://travis-ci.org/caozhiyi/Hudp.svg?branch=master" alt="Build Status"></a>
    <a href="https://opensource.org/licenses/BSD-3-Clause"><img src="https://img.shields.io/badge/license-bsd-orange.svg" alt="Licenses"></a>
</p> 

See [chinese](/README_cn.md) 
## Introduction

Hudp is a rudp library, rudp means reliable UDP. But comparing with TCP mandatory and non-optional reliable transmission, UDP is more like a blank sheet of paper that you can define the reliability you need.    
Hudp provides different options for users, realizing different reliability and priority by controlling message transmission.   
Hudp doesn't limit transmission reliability when initializing libraries and connections, users can choose different reliability options when sending each message to achieve the finest granularity of control.   
Hudp is initialized only when resources are used, such as  send form and receive order queue, to ensure the minimum resource consumption in use.   
Hudp controls the creation and destruction of messages by a message pool to achieve rapid creation of common resources.   
Hudp controls the package processing of sending messages through a chain of responsibility (similar to nginx), which can quickly and conveniently embed additional processing.   
Hudp use BBR algorithm to control congestion.   
Hudp is still improving.   

## Options
Hudp offers four options for reliability：
```cpp
    // Transmission reliability control
    enum hudp_tran_flag {
        // only orderly. may lost some bag
        HTF_ORDERLY          = 0x01,
        // only reliable. may be disorder
        HTF_RELIABLE         = 0x02,
        // reliable and orderly like tcp
        HTF_RELIABLE_ORDERLY = 0x04,
        // no other contral. only udp
        HTF_NORMAL           = 0x08
    };
```
HTF_ORDERLY : Only sequentiality is guaranteed, which means that when the remote receives the packet arrival delay because of network jitter, it may directly discard and not notification upper layer.   
HTF_RELIABLE : Only reliability is guaranteed, the upper layer may receive disorderly packages, but each package is guaranteed to be delivered.   
HTF_RELIABLE_ORDERLY : It looks like TCP to ensure reliability and sequentiality, but the difference is that the upper layer can participate in all aspects of reliable transmission through callbacks.   
HTF_NORMAL : Ordinary UDP transmission, without any control.   

Hudp provides four priority options： 
```cpp
    // about priority. Send two high-level packages and one low-level package when busy
    enum hudp_pri_flag {
        // the lowest priority.
        HPF_LOW_PRI          = 0x10,
        // the normal priority.
        HPF_NROMAL_PRI       = 0x20,
        // the high priority.
        HPF_HIGH_PRI         = 0x40,
        // the highest priority.
        HPF_HIGHEST_PRI      = 0x80
    };
Priority increases in turn.    
When the priority messages at all levels are not empty, the sending process is executed according to the rule of sending two high priority messages and one low priority message.    
This means each sending two HPF_HIGHEST_PRI, send one HPF_HIGH_PRI, each sending eight HPF_HIGHEST_PRI, send one HPF_LOW_PRI are sent.   
Of course, if there is only one message, any priority will be sent.

## Interface
Hudp provides a minimum number of interfaces for users to use, which is like using the original UDP interface.
```c++
    // init library
    void Init();
    
    // start thread and recv with ip and port
    bool Start(const std::string& ip,uint16_t port, const recv_back& func);
    void Join();
    // send msg
    bool SendTo(const HudpHandle& handle, uint16_t flag, std::string& msg);
    bool SendTo(const HudpHandle& handle, uint16_t flag, const char* msg, uint32_t len);
    // destory socket. release resources
    void Close(const HudpHandle& handle);
```
The annotations of interface can be see[Hudp](/include/Hudp.h).   
The Start interface opens listening on the port and receives notification in the callback function when a message arrives.   
The SendTo interface sends messages to the remote. Flag parameters can control the reliability and priority of this message, you need to choose one of the combinations to send this message, like：HPF_NROMAL_PRI | HTF_RELIABLE_ORDERLY.   

## Example

See [HudpClient](/test/example/HudpClient.cpp) and [HudpServer](/test/example/HudpServer.cpp).   
But don't misunderstand the names of server and client. In fact, for Hudp, the two ends are the same, as long as the Start interface is called, it has the ability to receive messages.   

## Build(Windows)

You can compile Hudp library and example with vs2017.

## Build(Linux)

The Hudp library and examples can be compiled simply by executing make in the source directory.
```
$ make
```

## Licenses

Hudp is under the terms of the BSD 3-Clause License. [https://opensource.org/licenses/BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)。
