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