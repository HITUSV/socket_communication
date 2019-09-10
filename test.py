import socket
import threading
import time
import json
from json import JSONDecodeError

kHandShake1 = 1
kHandShake2 = 2
kHandShake3 = 3
kNormalData = 4
client_num = 0
client_id_list = []
user_id_list = []
SocketErrorType = []
kErrorFree = 0
kMissHandShake = 1
SOCKET_SIZE = 2048
SocketErrorType.append(kErrorFree)
SocketErrorType.append(kMissHandShake)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client_socket = {}
user_socket = {}
while True:
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('0.0.0.0', 9009))
        break
    except OSError:
        s.close()
        time.sleep(1)
        print("try")
s.listen(20)


def data_encapsulation(data, header):
    data_str = json.dumps(data)
    data_send = {'header': header, 'time_stamp': int(1000*time.time()),
                 'data': data_str}
    data_send_str = json.dumps(data_send)
    return data_send_str.encode('utf-8')


def tcp_link(sock, addr):
    print('Accept new connection from %s:%s...' % addr)
    while True:
        try:
            data = sock.recv(SOCKET_SIZE)
        except OSError:
            return None
        if not data:
            break
        data = str(data, 'utf-8')
        print(data)
        return_data = {"a": 1, "b": 2}
        send_str = data_encapsulation(return_data, 1)
        sock.send(send_str)
    print('Close connection from %s:%s...' % addr)


if __name__ == '__main__':
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        while True:
            sock, addr = s.accept()
            t = threading.Thread(target=tcp_link, args=(sock, addr))
            t.start()
    except KeyboardInterrupt:
        sock.shutdown(socket.SHUT_RDWR)
        s.shutdown(socket.SHUT_RDWR)
        client_id_list.clear()
