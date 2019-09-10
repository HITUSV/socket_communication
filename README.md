# C++ Socket通信库
### 特性
1. 异步接收
2. 支持回调函数
3. 单个数据可对应多个回调函数
4. 自定义额外回调参数
3. 数据json封装
4. socket异常时调用对应的回调函数

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
一个数据头可以对应多个回调函数，按设定回调函数的先后顺序进行调用


### 示例
```cpp
//
// Created by wumode on 2019/9/8.
//

#include "socket_communication.h"

typedef enum SocketHeader{
    kTestMsg = 1,
    kTestMsg2 = 2
}SocketHeader;

typedef struct TestMsg{
    int a;
    int b;
}TestMsg;

void test_callback(const TestMsg* data, TestMsg b){
    std::cout<<"callback a: "<<data->a<<" b: "<<data->b<<std::endl;
    std::cout<<"args a: "<<b.a<<" b: "<<b.b<<std::endl;
}

void test_callback2(const TestMsg* data){
    std::cout<<"callback2 a: "<<data->a<<" b: "<<data->b<<std::endl;
}

void test_disconnect_callback(int args){
    std::cout<<"disconnect callback arg: "<<args<<std::endl;
}

void test_close_callback(){
    std::cout<<"close callback"<<std::endl;
}

void to_json(json &j, const TestMsg &socketTransmission) {
    j = json{{"a",       socketTransmission.a},
             {"b",       socketTransmission.b}};
}

void from_json(const json &j, TestMsg &socketTransmission) {
    socketTransmission.a = j.at("a").get<int >();
    socketTransmission.b = j.at("b").get<int>();
}

int main(int argc, char* argv[]){
    TestMsg testMsg,a, b;
    testMsg.a = 5;
    testMsg.b = 80;
    a.a = 15;
    a.b = 2;
    b.a = 0;
    b.b = 0;
    int arg1 = 2;
    socket_communication::SocketCommunication s;

    ///设置数据头1的回调函数
    s.SetCallBackFunction<void(const TestMsg*, TestMsg), TestMsg>(test_callback, kTestMsg, a);
    s.SetCallBackFunction<void(const TestMsg*),TestMsg>(test_callback2, kTestMsg);

    ///设置异常断开的回调函数
    s.SetSignalCallBackFunction<void(int)>(test_disconnect_callback, socket_communication::kSocketAbnormalDisconnection, arg1);

    ///设置Socket断开的回调函数
    s.SetSignalCallBackFunction<void()>(test_close_callback, socket_communication::kSocketClose);

    ///连接
    if(!s.Open("127.0.0.1", 9009)){
        return -1;
    }

    ///开启接收线程
    s.StartSocketReceiveThread();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ///发送数据 a，数据头 1
    s.SendData(a, 1);
    std::this_thread::sleep_for(std::chrono::seconds(5));

    ///关闭接收线程
    s.CloseSocketReceiveThread();

    ///关闭socket
    s.Close();
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
python test.py
build/./test
```

结果如下

	callback a: 1 b: 2
	args a: 15 b: 2
	callback2 a: 1 b: 2
	close callback