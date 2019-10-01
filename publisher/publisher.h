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
