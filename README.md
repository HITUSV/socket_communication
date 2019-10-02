# C++ Socket通信库
### 特性

socket_communication是为了实现像ROS中一样的通信，它拥有以下特性

1. 异步接收
2. 支持回调函数
3. 单个数据可对应多个回调函数
4. 自定义额外回调参数
5. 数据json封装
6. socket异常时调用对应的回调函数
7. 线程安全，支持多线程发送
8. 接收回调函数并发执行
9. ROS风格发布消息

### 依赖
- [nlohmann/json](https://github.com/nlohmann/json "nlohmann/json")  高性能的现代C++ json库

目前只支持linux，nlohmann/json已经放在*3rdparty*下了，不需要再下载

### 说明
#### 数据封装
socket_communication 会给要发送数据做一层封装
```json
{
	"data": "{...}",
	"header": 1,
	"time_stamp": 1568082111190
}
```
其中
- time_stamp：发送时刻的毫秒时间戳
- header：数据头
- data：发送数据的json字符串

####  数据头
一个数据头（**uisigned char**）对应一种数据类型，socket_communication会根据接收到数据的数据头，调用对应的回调函数，并传入数据。

比如要发送以下结构体，定义数据头为1
```cpp
{
typedef struct TestMsg{
    int a;
    int b;
}TestMsg;
}
```
最终发出数据为：
```json
{
    "socket_header": 1,
	"data": "{\"a\":15,\"b\":2}",
	"header": 1,
	"time_stamp": 1568082111190
}
```
#### 回调函数
回调支持，函数接口或Lambda代码块
例如：
```cpp
void test_callback(const TestMsg* data, TestMsg b){
    std::cout<<"callback a: "<<data->a<<" b: "<<data->b<<std::endl;
    std::cout<<"args a: "<<b.a<<" b: "<<b.b<<std::endl;
}

void test_callback2(const TestMsg* data){
    std::cout<<"callback2 a: "<<data->a<<" b: "<<data->b<<std::endl;
}

```
其中data为接收到的数据，b为额外参数，额外参数可以是任意数据类型，通过完美转发实现参数的内存安全
一个数据头可以对应多个回调函数，回调函数并发调用

#### 回调函数类
callback_function.h定义对象CallBackFunction， CallBackFunction继承自CallBackFunctionInterface, 实现了对回调函数的封装

#### ROS风格的发送
可以像在ROS中一样创建一个发布者，来发布消息, 这种方式是异步发送

```cpp
socket_communication::Publisher<TestMsg> pub(&s, 1);
pub.publish(b);
```

### 示例
```cpp
//
// Created by wumode on 2019/9/8.
//

#include "socket_communication.h"
#include <publisher.h>

typedef enum SocketHeader {
    kTestMsg = 1,
    kTestMsg2 = 2
} SocketHeader;

typedef struct TestMsg {
    int a;
    int b;
} TestMsg;

void test_callback(const TestMsg *data, TestMsg arg) {
    std::cout << "[callback] callback a: " << data->a << " b: " << data->b << " args a: " << arg.a << std::endl;
}

void test_callback2(const TestMsg *data) {
    std::cout << "[callback] callback2 a: " << data->a << " b: " << data->b << std::endl;
}

void test_client_disconnect_callback(int args) {
    std::cout << "[callback] client socket abnormal close, arg: " << args << std::endl;
}

void test_client_close_callback() {
    std::cout << "[callback] client socket close" << std::endl;
}

void test_server_close_callback() {
    std::cout << "[callback] server socket close" << std::endl;
}

void to_json(json &j, const TestMsg &socketTransmission) {
    j = json{{"a", socketTransmission.a},
             {"b", socketTransmission.b}};
}

void from_json(const json &j, TestMsg &socketTransmission) {
    socketTransmission.a = j.at("a").get<int>();
    socketTransmission.b = j.at("b").get<int>();
}

int main(int argc, char *argv[]) {
    TestMsg arg, a, b;
    arg.a = 1;
    arg.b = 1;
    a.a = 2;
    a.b = 2;
    b.a = 3;
    b.b = 3;
    int arg1 = 2;
    socket_communication::SocketCommunicationClient client;
    socket_communication::SocketCommunicationServer server;

    ///设置数据头1的回调函数
    client.SetCallBackFunction < void(
    const TestMsg*, TestMsg), TestMsg > (test_callback, kTestMsg, arg);
    client.SetCallBackFunction < void(
    const TestMsg*),TestMsg > (test_callback2, kTestMsg);

    ///设置异常断开的回调函数
    client.SetSignalCallBackFunction<void(int)>(test_client_disconnect_callback, socket_communication::kSocketAbnormalDisconnection,
                                                arg1);

    ///设置Socket正常断开的回调函数
    client.SetSignalCallBackFunction<void()>(test_client_close_callback, socket_communication::kSocketClose);
    server.SetCallBackFunction < void(
    const TestMsg*, TestMsg), TestMsg > (test_callback, kTestMsg, arg);
    server.SetSignalCallBackFunction<void()>(test_server_close_callback, socket_communication::kSocketClose);
    if(!server.Connect("127.0.0.1", 9008)){
        return -2;
    }

    ///连接
    if (!client.Connect("127.0.0.1", 9008)) {
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    /// client断开
    client.Close();
    client.Connect();

    ///发送数据 a，数据头 1
    client.SendData(a, 1);

    /// 或者使用ROS风格的发送者来发送数据
    socket_communication::Publisher<TestMsg> pub(&client, 1);
    for (int i = 0; i < 3; ++i) {
        pub.publish(b);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    ///关闭socket
    server.Close();
    //client.Close();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
```

编译
```
git clone git@github.com:HITUSV/socket_communication.git
cd socket_communication
mkdir build
cd build
cmake ..
make
```

运行

```
build/./test
```

结果如下， 数据打印不正常是由于异步回调，多线程同时打印

	[server] Accept a new connect
    [callback] client socket close
    [socket] Socket closed
    [callback] server socket close
    [server] Accept a new connect
    [callback] callback a: 2 b: 2 args a: 1
    [callback] callback a: 3 b: 3 args a: 1
    [callback] callback a: 3 b: 3 args a: 1
    [callback] callback a: 3 b: 3 args a: 1
    [socket] Socket closed
    [callback] server socket close
    [callback] client socket close
    [callback] client socket abnormal close, arg: 2