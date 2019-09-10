//
// Created by wumode on 2019/9/8.
//

#include "socket_communication.h"
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
    s.SetCallBackFunction<void(const TestMsg*, TestMsg), TestMsg>(test_callback, 1, a);
    s.SetCallBackFunction<void(const TestMsg*),TestMsg>(test_callback2, 1);

    ///设置异常断开的回调函数
    s.SetSignalCallBackFunction<void(int)>(test_disconnect_callback, socket_communication::kSocketAbnormalDisconnection, arg1);

    ///设置Socket断开的回调函数
    s.SetSignalCallBackFunction<void()>(test_close_callback, socket_communication::kSocketClose);

    ///连接
    s.Open("127.0.0.1", 9009);

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