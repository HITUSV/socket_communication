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
// Created by wumode on 2019/9/19.
//

#ifndef SOCKET_COMMUNICATION_PUBLISHER_H
#define SOCKET_COMMUNICATION_PUBLISHER_H

#include "../socket_communication.h"
namespace socket_communication {
    template<typename T>
    class Publisher {
    public:
        Publisher(socket_communication::SocketCommunication *s, uint8_t header);

        void publish(T &msg);

        void publish(T &&msg);

    private:
        socket_communication::SocketCommunication *socket;
        uint8_t header_;
    };
    template<typename T>
    Publisher<T>::Publisher(socket_communication::SocketCommunication *s, uint8_t header) {
        socket = s;
        header_ = header;
    }

    template<typename T>
    void Publisher<T>::publish(T &msg) {
        std::thread send_thread([this, &msg]() {
            this->socket->SendData(msg, this->header_);
        });
        send_thread.detach();
    }

    template<typename T>
    void Publisher<T>::publish(T &&msg) {
        std::thread send_thread([this, &msg]() {
            this->socket->SendData(msg, this->header_);
        });
        send_thread.detach();
    }
}

#endif //SOCKET_COMMUNICATION_PUBLISHER_H
