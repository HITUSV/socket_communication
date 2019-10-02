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

#include "socket_communication.h"

namespace socket_communication {
    void to_json(json &j, const SocketTransmission &socketTransmission) {
        j = json{{"socket_header",     socketTransmission.socket_header},
                 {"header",     socketTransmission.header},
                 {"data",       socketTransmission.data},
                 {"time_stamp", socketTransmission.time_stamp}};
    }

    void from_json(const json &j, SocketTransmission &socketTransmission) {
        socketTransmission.socket_header = j.at("socket_header").get<uint16_t>();
        socketTransmission.header = j.at("header").get<uint8_t>();
        socketTransmission.data = j.at("data").get<std::string>();
        socketTransmission.time_stamp = j.at("time_stamp").get<uint64_t>();
    }

    SocketCommunication::SocketCommunication() {
        socket_thread_ = false;
        is_connected_ = false;
        is_sending_ = false;
        offline_reconnection_ = false;
//        rx_buffer_ = new uint8_t[SOCKET_SIZE];
//        memset(rx_buffer_, '\0', SOCKET_SIZE);
        socket_thread_is_running_ = false;
        send_thread_queue_ = new std::queue<std::thread::id>;
        port_ = 0;
        receive_thread_id_ = nullptr;
    }


    SocketCommunication::SocketCommunication(const std::string &host, uint16_t port) {
        socket_thread_ = false;
        is_connected_ = false;
        is_sending_ = false;
        offline_reconnection_ = false;
        //memset(rx_buffer_, '\0', SOCKET_SIZE);
        host_ = host;
        port_ = port;
        socket_thread_is_running_ = false;
        send_thread_queue_ = new std::queue<std::thread::id>;
        receive_thread_id_ = nullptr;
    }

    SocketCommunication::~SocketCommunication() {
        for (auto &iter : callback_function_list_) {
            for (auto &i: iter.second) {
                delete i;
            }
        }
        for (auto &iter:signal_function_list_) {
            for (auto &i: iter.second) {
                delete i;
            }
        }
        callback_function_list_.clear();
        signal_function_list_.clear();
        delete send_thread_queue_;
    }

    void SocketCommunication::Call(const uint8_t *buffer, uint8_t header) {
        std::string data_string = (const char *) buffer;
        auto f = callback_function_list_.find(header);
        if (f == callback_function_list_.end()) {
            return;
        }
        for (auto &iter: callback_function_list_[header]) {
//            (*(iter->task))(data_string);
            const std::function<void(std::string)> *task;
            task = iter->task;
            std::thread callback_thread([task, data_string]() {
                (*task)(data_string);
            });
            callback_thread.detach();
        }
    }

    void SocketCommunication::SignalCall(socket_communication::SocketSignal socketSignal) {
        auto f = signal_function_list_.find(socketSignal);
        if (f == signal_function_list_.end()) {
            return;
        }
        for (auto &iter: signal_function_list_[socketSignal]) {
            (*(iter->task))();
        }
    }

    bool SocketCommunication::StartSocketReceiveThread() {
        return StartSocketReceiveThread(host_, port_);
    }

    bool SocketCommunication::Connect() {
        return Connect(host_, port_);
    }

    bool SocketCommunication::Connect(const std::string &host, uint16_t port) {
        host_ = host;
        port_ = port;
        return NewConnect() && StartSocketReceiveThread();
    }

    void SocketCommunication::SetHost(const std::string &host) {
        host_ = host;
    }

    void SocketCommunication::SetPort(uint16_t port) {
        port_ = port;
    }

    bool SocketCommunication::IsOpen() {
        return is_connected_;
    }

    void SocketCommunication::Receive() {
        auto *_this = (SocketCommunication*) this;
        //is_connected_ = true;
        _this->socket_thread_is_running_ = true;
        int socket_closed = 0;
        uint8_t* rx_buffer;
        rx_buffer = new uint8_t[SOCKET_SIZE];
        while (_this->socket_thread_) {
            memset(rx_buffer, '\0', SOCKET_SIZE);
            char temp;
            temp = read(_this->socket_, rx_buffer, SOCKET_SIZE - 1);
            if (temp == 0) {
                std::cerr << "[socket] Socket closed" << std::endl;
                socket_closed = 1;
                break;
            }
            if (temp > 0) {
                _this->CallFunction(rx_buffer, (void *) _this);
            }
        }
        if (socket_closed == 1 && _this->socket_thread_) {
            _this->SignalCall(kSocketClose);
            _this->SignalCall(kSocketAbnormalDisconnection);
            _this->is_connected_ = false;
        }
        _this->socket_thread_is_running_ = false;
        delete[]rx_buffer;
    }

    void SocketCommunication::CloseSocketThread() {
        void *status;
        if (socket_thread_) {
            socket_thread_ = false;
            socket_thread_is_running_ = false;
            pthread_join(*receive_thread_id_, &status);
            delete receive_thread_id_;
        }
    }

    void *SocketCommunication::SocketReceive(void *__this) {
        auto *_this = (SocketCommunication*) __this;
        _this->Receive();
        pthread_exit(nullptr);
        return nullptr;
    }

    void SocketCommunication::ResetOfflineFlag() {
        offline_reconnection_ = false;
    }

    bool SocketCommunication::OfflineReconnection() {
        return offline_reconnection_;
    }

    void SocketCommunication::RemoveCallBackFunction(uint8_t header) {
        auto iter = callback_function_list_.find(header);
        if (iter == callback_function_list_.end()) {
            return;
        }
        for (auto &i:callback_function_list_[header]) {
            delete i;
        }
        callback_function_list_.erase(header);
    }

    void SocketCommunication::RemoveSignalCallbackFunction(socket_communication::SocketSignal s) {
        auto iter = signal_function_list_.find(s);
        if (iter == signal_function_list_.end()) {
            return;
        }
        for (auto &i:signal_function_list_[s]) {
            delete i;
        }
        signal_function_list_.erase(s);
    }

    void SocketCommunication::CallFunction(uint8_t *data, void *__this) {
        auto *_this = (SocketCommunication *) __this;
        std::string data_receive = (const char *) data;
        json j;
        try {
            j = json::parse(data_receive);
        }
        catch (nlohmann::detail::parse_error &e) {
#ifdef USE_GLOG
            LOG(ERROR)<<"[socket] Data parse error, data: "<<data_receive<<std::endl;
#endif
            std::cerr << "[socket] Data parse error, data: " << data_receive << std::endl;
            return;
        }
        SocketTransmission s_t;
        try {
            s_t = j;
        }
        catch (nlohmann::detail::type_error &e) {
#ifdef USE_GLOG
            LOG(ERROR)<<"[socket] Data parse error, data: "<<data_receive<<std::endl;
#endif
            std::cerr << "[socket] Data parse error, data: " << data_receive << std::endl;
            return;
        }
        if(s_t.socket_header==kData){
            Call((uint8_t *) s_t.data.c_str(), s_t.header);
        }
    }

    template<typename T>
    SocketCommunication &operator<<(SocketCommunication &sock, const T &data) {
        return sock;
    }

    void SocketCommunication::Close() {
        CloseSocketThread();
        if (is_connected_) {
            shutdown(socket_, SHUT_RDWR);
            SignalCall(kSocketClose);
        }
        is_connected_ = false;
    }

    SocketCommunicationServer::SocketCommunicationServer() {
        max_clients_num = 1;
        is_listening_ = false;
        listen_socket_ = 0;

        debug_name_ = "server";
    }

    SocketCommunicationServer::SocketCommunicationServer(const std::string &host, uint16_t port) : SocketCommunication(
            host, port) {
        max_clients_num = 1;
        is_listening_ = false;
        listen_socket_ = 0;
    }

    bool SocketCommunicationServer::NewConnect() {
        if (is_connected_) {
            Close();
        }
        listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_socket_ == -1) {
            std::cerr << "[server] Create socket error" << std::endl;
            return false;
        }
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        struct hostent *h_ptr = gethostbyname(host_.c_str());
        if (h_ptr == nullptr) {
            std::cerr << "[server] Host name error" << std::endl;
            return false;
        }
        addr.sin_addr.s_addr = *((unsigned long *) h_ptr->h_addr_list[0]);
        //inet_aton(_this->host_.c_str(), &(addr.sin_addr));
        //int addrlen = sizeof(addr);
        int32_t bReuseaddr = 1;
        setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, (const char *) &bReuseaddr, sizeof(int32_t));
        bind(listen_socket_, (struct sockaddr*)&addr, sizeof(addr));
        int res = listen(listen_socket_, max_clients_num);
        if (res == -1) {
            std::cerr << "[server] Listen " << host_ << ":" << port_ << " error" << std::endl;
            return false;
        }
        struct timeval timeout = {0, 5000};
        int ret = setsockopt(listen_socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
        if (ret == -1) {
            std::cerr << "[server] Failed to set socket timeout" << std::endl;
        }
        is_listening_ = true;
        return true;
    }

    void *SocketCommunicationServer::SocketAccept(void *__this) {
        auto _this = (SocketCommunicationServer*)__this;
        _this->socket_thread_ = true;
        while(_this->socket_thread_) {
            struct sockaddr_in clnt_addr;
            socklen_t clnt_addr_size = sizeof(clnt_addr);
            struct timeval timeout = {1, 5000};
            int ret = setsockopt(_this->listen_socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
            if (ret == -1) {
                std::cerr << "[server] Failed to set socket timeout" << std::endl;
            }
            _this->socket_ = accept(_this->listen_socket_, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
            if(_this->socket_ == -1){
                continue;
            }
            std::cout<<"[server] Accept a new connect"<<std::endl;
            _this->is_connected_ = true;
            _this->Receive();
        }
        _this->socket_thread_is_running_  = false;
        //std::cout<<"[server] Close"<<std::endl;
        return nullptr;
    }

    bool SocketCommunicationServer::StartSocketReceiveThread(const std::string &host, uint16_t port) {
        if (!is_listening_) {
            return false;
        }
        if (socket_thread_is_running_) {
            CloseSocketThread();
        }
        socket_thread_ = true;
        //pthread_t* receive_thread_id_;
        receive_thread_id_ = new pthread_t;
        pthread_create(receive_thread_id_, nullptr, SocketAccept, (void *) this);
        //receive_id_vector_.push_back(receive_thread_id_);
        port_ = port;
        host_ = host;
        return true;
    }


    SocketCommunicationServer::~SocketCommunicationServer() {
        Close();
    }

    SocketCommunicationClient::SocketCommunicationClient() {
        debug_name_ = "client";
    }

    SocketCommunicationClient::SocketCommunicationClient(const std::string &host, uint16_t port) : SocketCommunication(
            host, port) {

    }

    bool SocketCommunicationClient::NewConnect() {
        if (is_connected_) {
            Close();
        }
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == -1) {
            std::cerr << "[socket] Create socket error" << std::endl;
            return false;
        }
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        struct hostent *h_ptr = gethostbyname(host_.c_str());
        if (h_ptr == nullptr) {
            std::cerr << "[client] Host name error" << std::endl;
            return false;
        }
        addr.sin_addr.s_addr = *((unsigned long *) h_ptr->h_addr_list[0]);
        //inet_aton(_this->host_.c_str(), &(addr.sin_addr));
        int addrlen = sizeof(addr);
        int32_t bReuseaddr = 1;
        setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char *) &bReuseaddr, sizeof(int32_t));

        int listen_socket = connect(socket_, (struct sockaddr *) &addr, addrlen);
        if (listen_socket == -1) {
            //LOG(ERROR)<<"connect "<<host_<<":"<<port_<<" error";
            std::cerr << "[client] Connect " << host_ << ":" << port_ << " error" << std::endl;
            return false;
        }
        struct timeval timeout = {0, 5000};
        int ret = setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
        if (ret == -1) {
            std::cerr << "[client] Failed to set socket timeout" << std::endl;
        }
        is_connected_ = true;
        return true;
    }

    bool SocketCommunicationClient::StartSocketReceiveThread(const std::string &host, uint16_t port) {
        if (!is_connected_) {
            return false;
        }
        if (socket_thread_is_running_) {
            CloseSocketThread();
        }
        socket_thread_ = true;
        receive_thread_id_ = new pthread_t;
        pthread_create(receive_thread_id_, nullptr, SocketReceive, (void *) this);
        port_ = port;
        host_ = host;
        return true;
    }

    SocketCommunicationClient::~SocketCommunicationClient() {
        Close();
    }
}