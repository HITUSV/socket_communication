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
    std::cout << "callback a: " << data->a << " b: " << data->b << std::endl;
    std::cout << "args a: " << arg.a << std::endl;
}

void test_callback2(const TestMsg *data) {
    std::cout << "callback2 a: " << data->a << " b: " << data->b << std::endl;
}

void test_disconnect_callback(int args) {
    std::cout << "disconnect callback arg: " << args << std::endl;
}

void test_close_callback() {
    std::cout << "close callback" << std::endl;
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
    socket_communication::SocketCommunication s;
    ///设置数据头1的回调函数
    s.SetCallBackFunction < void(
    const TestMsg*, TestMsg), TestMsg > (test_callback, kTestMsg, arg);
    s.SetCallBackFunction < void(
    const TestMsg*),TestMsg > (test_callback2, kTestMsg);

    ///设置异常断开的回调函数
    s.SetSignalCallBackFunction<void(int)>(test_disconnect_callback, socket_communication::kSocketAbnormalDisconnection,
                                           arg1);

    ///设置Socket断开的回调函数
    s.SetSignalCallBackFunction<void()>(test_close_callback, socket_communication::kSocketClose);

    ///连接
    if (!s.Open("127.0.0.1", 9009)) {
        return -1;
    }

    ///开启接收线程
    s.StartSocketReceiveThread();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ///发送数据 a，数据头 1
    s.SendData(a, 1);
    //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    /// 或者使用ROS风格的发送者来发送数据
    socket_communication::Publisher<TestMsg> pub(&s, 1);
    for (int i = 0; i < 3; ++i) {
        pub.publish(b);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ///关闭接收线程
    s.CloseSocketReceiveThread();

    ///关闭socket
    s.Close();
    return 0;
}