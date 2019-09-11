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
        j = json{{"header",     socketTransmission.header},
                 {"data",       socketTransmission.data},
                 {"time_stamp", socketTransmission.time_stamp}};
    }

    void from_json(const json &j, SocketTransmission &socketTransmission) {
        socketTransmission.header = j.at("header").get<uint8_t>();
        socketTransmission.data = j.at("data").get<std::string>();
        socketTransmission.time_stamp = j.at("time_stamp").get<uint64_t>();
    }

    SocketCommunication::SocketCommunication() {
        client_socket_ptr_ = nullptr;
        socket_thread_ = false;
        is_open_ = false;
        is_sending_ = false;
        offline_reconnection_ = false;
        memset(rx_buffer_, '\0', SOCKET_SIZE);
        receive_thread_ = false;
        send_thread_queue_ = new std::queue<std::thread::id>;
    }

    SocketCommunication::SocketCommunication(const std::string &host, uint16_t port) {
        client_socket_ptr_ = nullptr;
        socket_thread_ = false;
        is_open_ = false;
        is_sending_ = false;
        offline_reconnection_ = false;
        memset(rx_buffer_, '\0', SOCKET_SIZE);
        host_ = host;
        port_ = port;
        receive_thread_ = false;
        send_thread_queue_ = new std::queue<std::thread::id>;
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
        Close();
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

    bool SocketCommunication::StartSocketReceiveThread(const std::string &host, uint16_t port) {
        port_ = port;
        host_ = host;
        return StartSocketReceiveThread(host_, port_, this);;
    }

    bool SocketCommunication::StartSocketReceiveThread(const std::string &host, uint16_t port, void *__this) {
        auto *_this = (SocketCommunication *) __this;
        if (!_this->is_open_) {
            return false;
        }
        if (_this->receive_thread_) {
            CloseSocketReceiveThread();
        }
        _this->socket_thread_ = true;
        receive_id_ = new pthread_t;
        pthread_create(receive_id_, nullptr, SocketReceive, (void *) this);
        _this->port_ = port;
        _this->host_ = host;
        return true;
    }

    bool SocketCommunication::Open() {
        return Open(host_, port_);
    }


    bool SocketCommunication::Open(const std::string &host, uint16_t port) {
        host_ = host;
        port_ = port;
        if (is_open_) {
            Close();
        }
        client_socket_ptr_ = new int;
        *client_socket_ptr_ = socket(AF_INET, SOCK_STREAM, 0);
        if (*client_socket_ptr_ == -1) {
            delete client_socket_ptr_;
            client_socket_ptr_ = nullptr;
            std::cerr << "Start socket error" << std::endl;
            return false;
        }
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        struct hostent *h_ptr = gethostbyname(host_.c_str());
        if (h_ptr == nullptr) {
            std::cerr << "Host name error" << std::endl;
            return false;
        }
        addr.sin_addr.s_addr = *((unsigned long *) h_ptr->h_addr_list[0]);
        //inet_aton(_this->host_.c_str(), &(addr.sin_addr));
        int addrlen = sizeof(addr);
        int32_t bReuseaddr = 1;
        setsockopt(*client_socket_ptr_, SOL_SOCKET, SO_REUSEADDR, (const char *) &bReuseaddr, sizeof(int32_t));

        int listen_socket = connect(*client_socket_ptr_, (struct sockaddr *) &addr, addrlen);
        if (listen_socket == -1) {
            //LOG(ERROR)<<"connect "<<host_<<":"<<port_<<" error";
            delete client_socket_ptr_;
            client_socket_ptr_ = nullptr;
            std::cerr << "connect " << host_ << ":" << port_ << " error" << std::endl;
            return false;
        }
        struct timeval timeout = {0, 5000};
        int ret = setsockopt(*client_socket_ptr_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
        if (ret == -1) {
            std::cerr << "Failed to set socket timeout" << std::endl;
        }
        is_open_ = true;
        return true;
    }

    void SocketCommunication::SetHost(const std::string &host) {
        host_ = host;
    }

    void SocketCommunication::SetPort(uint16_t port) {
        port_ = port;
    }

    bool SocketCommunication::IsOpen() {
        return is_open_;
    }

    void *SocketCommunication::SocketReceive(void *__this) {
        auto *_this = (SocketCommunication *) __this;
        _this->receive_thread_ = true;
        int socket_closed = 0;
        while (_this->socket_thread_) {
            memset(_this->rx_buffer_, '\0', SOCKET_SIZE);
            char temp;
            temp = read(*_this->client_socket_ptr_, _this->rx_buffer_, SOCKET_SIZE - 1);
            if (temp == 0) {
                std::cerr << "Socket closed" << std::endl;
                socket_closed = 1;
                break;
            }
            if (temp > 0) {
                _this->CallFunction(_this->rx_buffer_, (void *) _this);
            }
        }
        if (socket_closed == 1 && _this->socket_thread_) {
            _this->SignalCall(kSocketClose);
            _this->SignalCall(kSocketAbnormalDisconnection);
            _this->is_open_ = false;
        }
        _this->receive_thread_ = false;
        pthread_exit(NULL);
        return nullptr;
    }

    void SocketCommunication::ResetOfflineFlag() {
        offline_reconnection_ = false;
    }

    bool SocketCommunication::OfflineReconnection() {
        return offline_reconnection_;
    }

    void SocketCommunication::CloseSocketReceiveThread() {
        void *status;
        int rc;
        if (socket_thread_) {
            socket_thread_ = false;
            rc = pthread_join(*receive_id_, &status);
            delete receive_id_;
        }
    }

    void SocketCommunication::Close() {
        socket_thread_ = false;
        CloseSocketReceiveThread();
        if (is_open_) {
            shutdown(*client_socket_ptr_, SHUT_RDWR);
            delete client_socket_ptr_;
            client_socket_ptr_ = nullptr;
            SignalCall(kSocketClose);
        }
        is_open_ = false;
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
            LOG(ERROR)<<"Dara parse error, data: "<<data_receive<<std::endl;
#endif
            std::cerr << "Dara parse error, da    ta: " << data_receive << std::endl;
            return;
        }
        SocketTransmission s_t;
        try {
            s_t = j;
        }
        catch (nlohmann::detail::type_error &e) {
#ifdef USE_GLOG
            LOG(ERROR)<<"Dara parse error, data: "<<data_receive<<std::endl;
#endif
            std::cerr << "Dara parse error, data: " << data_receive << std::endl;
            return;
        }
        Call((uint8_t *) s_t.data.c_str(), s_t.header);
    }
}