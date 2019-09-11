/******************************************************************************
 *
 * Copyright 2019 wumo1999@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

//
// Created by wumode on 19-7-16.
//

#ifndef SOCKET_COMMUNICATION_H
#define SOCKET_COMMUNICATION_H

#include <string>
#include <iostream>
#include <thread>
#include <map>
#include <chrono>
#include <json.hpp>
#include <climits>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <netdb.h>
#include <vector>
#include <queue>
#include <exception>
#include "callback_function.h"

#ifdef USE_GLOG
#include <glog/logging.h>
#endif

#define SOCKET_SIZE 2048

using nlohmann::json;

namespace socket_communication {
    /**
     * Socket收发数据结构体
     */
    typedef struct SocketTransmission {
        uint8_t header;
        int64_t time_stamp;
        std::string data;
    } SocketTransmission;

class SocketException:public std::exception{
    const char * what () const noexcept override
    {
        return "Data parse error";
    }
};

    void to_json(json &j, const SocketTransmission &socketTransmission);

    void from_json(const json &j, SocketTransmission &socketTransmission);

    /**
     * Socket回调信号类型
     * kSocketClose： socket断开
     * kSocketConnected： 连接成功
     * kSocketOfflineReconnected： 断开后重连
     * kSocketAbnormalDisconnection： 异常断开
     */
    typedef enum SocketSignal {
        kSocketClose = 1,
        kSocketConnected = 2,
        kSocketOfflineReconnected = 3,
        kSocketAbnormalDisconnection = 4
    } SocketSignal;

    /**
     * 回调函数封装，重写了数据转换方法Conversion
     * @tparam D 回调函数接收数据类型
     * @tparam C 原始数据类型
     */
    template<typename D, typename C>
    class SocketCallBackFunction : public CallBackFunction<D, C> {
        virtual bool Conversion(const C *c, D *d) {
            //std::string string_rec = (const char*)c;
//            std::cout<<"cov: "<<*c<<std::endl;
            json j;
            try {
                j = json::parse(*c);
            }
            catch (nlohmann::detail::parse_error &e) {
#ifdef USE_GLOG
                LOG(ERROR)<<"Dara parse error, data: "<<string_rec<<std::endl;
#endif
                std::cerr << "Data parse error, data: " << *c << std::endl;
                return false;
            }
            try {
                *d = j;
            }
            catch (nlohmann::detail::type_error &e) {
#ifdef USE_GLOG
                LOG(ERROR)<<"Dara parse error, data: "<<string_rec<<std::endl;
#endif
                std::cerr << "Data parse error, data: " << *c << std::endl;
                return false;
            }
            return true;
        }
    };

    class SocketCommunication {
    public:
        SocketCommunication();

        SocketCommunication(const std::string &host, uint16_t port);

        ~SocketCommunication();

        /**
         设置消息回调函数

         @param fun 函数接口或lambda代码块
         @param args 参数
         @param header 数据头
         @return
        */
        template<typename callable, typename D, typename... A>
        void SetCallBackFunction(callable &&fun, uint8_t header, A &&... arg);

        /**
         设置信号回调函数，信号类型见枚举SocketSignal

         @param fun 函数接口或lambda代码块
         @param args 参数
         @return
        */
        template<typename callable, typename... A>
        void SetSignalCallBackFunction(callable &&fun, SocketSignal socket_signal, A &&... arg);

        static void *SocketReceive(void *__this);

        /**
         开启接收线程

         @return true:成功，否则失败
        */
        bool StartSocketReceiveThread();

        /**
         开启接收线程
         @param host:域名或地址
         @param port:端口
         @return true:成功，否则失败
        */
        bool StartSocketReceiveThread(const std::string &host, uint16_t port);

        bool StartSocketReceiveThread(const std::string &host, uint16_t port, void *__this);

        /**
         关闭接收线程
         @return
        */
        void CloseSocketReceiveThread();

        /**
         移除回调函数
         @param header:数据头
        */
        void RemoveCallBackFunction(uint8_t header);

        /**
         移除回调函数
         @param s:socket信号
        */
        void RemoveSignalCallbackFunction(SocketSignal s);

        void ResetOfflineFlag();

        bool OfflineReconnection();

        /**
         发送数据
         @param data:数据
         @param header:数据头
        */
        template<typename T>
        void SendData(const T &data, uint8_t header);

        /**
         设置主机
         @param host:域名或地址
        */
        void SetHost(const std::string &host);

        /**
        设置端口
        @param port:端口
       */
        void SetPort(uint16_t port);

        /**
        连接状态
        @return true：已连接
       */
        bool IsOpen();

        /**
        连接
        @return true：连接成功
       */
        bool Open();

        bool Open(const std::string &host, uint16_t port);

        /**
        关闭串口
       */
        void Close();

    private:
        void CallFunction(uint8_t *data, void *__this);

        void Call(const uint8_t *buffer, uint8_t header);

        void SignalCall(SocketSignal socketSignal);

        volatile bool socket_thread_;
        volatile bool is_sending_;

        pthread_t *receive_id_;
        int *client_socket_ptr_;
        std::string host_;
        uint16_t port_;
        volatile bool is_open_;
        volatile bool offline_reconnection_;
        volatile bool receive_thread_;
        uint8_t rx_buffer_[SOCKET_SIZE];
        std::map<uint8_t, std::vector<CallbackPair<std::string> *>> callback_function_list_;
        std::map<SocketSignal, std::vector<CallbackPair<> *>> signal_function_list_;
        std::queue<std::thread::id> *send_thread_queue_;
    };

    template<typename callable, typename D, typename... A>
    void SocketCommunication::SetCallBackFunction(callable &&fun, uint8_t header, A &&... arg) {
        std::shared_ptr<SocketCallBackFunction<D, std::string>>
                c(new SocketCallBackFunction<D, std::string>);
        auto *c_p = new CallbackPair<std::string>(c,
                                                  c->SetFunction(std::forward<callable>(fun), std::forward<A>(arg)...));
        auto iter = callback_function_list_.find(header);
        if (iter == callback_function_list_.end()) {
            std::vector<CallbackPair<std::string> *> v;
            callback_function_list_[header] = v;
        }
        callback_function_list_[header].push_back(c_p);

    }

    template<typename callable, typename... A>
    void SocketCommunication::SetSignalCallBackFunction(callable &&fun, SocketSignal socket_signal, A &&... arg) {
        std::shared_ptr<CallBackFunctionWithoutData> c(new CallBackFunctionWithoutData);
        auto *c_p = new CallbackPair<>(c, c->SetFunction(std::forward<callable>(fun), std::forward<A>(arg)...));
        auto iter = signal_function_list_.find(socket_signal);
        if (iter == signal_function_list_.end()) {
            std::vector<CallbackPair<> *> v;
            signal_function_list_[socket_signal] = v;
        }
        signal_function_list_[socket_signal].push_back(c_p);
    }

    template<typename T>
    inline void SocketCommunication::SendData(const T &data, uint8_t header) {
        if (is_open_) {
            json j = data;
            std::string string_data;
            std::string string_send;
            SocketTransmission s_t;
            const uint8_t *str_send;
            string_data = j.dump();
            s_t.data = string_data;
            s_t.header = header;
            auto timeNow = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch());
            s_t.time_stamp = timeNow.count();
            json q = s_t;
            string_send = q.dump();
            str_send = (const uint8_t *) string_send.c_str();
            std::thread::id this_id = std::this_thread::get_id();
            send_thread_queue_->push(this_id);
            while (is_sending_ || this_id != send_thread_queue_->front()) {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
            is_sending_ = true;
            char temp;
            temp = write(*client_socket_ptr_, str_send, strlen((const char *) str_send));
            is_sending_ = false;
            send_thread_queue_->pop();
        } else {
            std::cerr << "Failed to send data, socket has closed" << std::endl;
        }
    };

}


#endif //SHIP_SOCKET_COMMUNICATION_H
