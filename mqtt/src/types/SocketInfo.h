#ifndef SOCKET_INFO_H
#define SOCKET_INFO_H

struct SocketInfo : public cObject {
  TcpSocket *socket;
}

#endif // SOCKET_INFO_H
