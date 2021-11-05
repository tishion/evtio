#include "udp_socket.hpp"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "log.hpp"

udp_socket::udp_socket() {
}

udp_socket::~udp_socket() {
}

int udp_socket::fd() {
  return sock_fd_;
}

bool udp_socket::open() {
  // open socket
  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock <= 0) {
    loge() << "failed to open socket:" << strerror(errno);
    return false;
  }

  // set socket address resuse
  int optval = 1;
  if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    logw() << "failed to set socket option:" << strerror(errno);
  }

  // keep socket fd
  sock_fd_ = sock;
  return true;
}

bool udp_socket::bind(const std::string& ip, uint16_t port) {
  // build the address
  struct sockaddr_in local_addr;
  bzero(&local_addr, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = inet_addr(ip.c_str());
  local_addr.sin_port = htons(port);

  // bnd the socket on the address
  if (::bind(sock_fd_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
    loge() << "faield to bind socket on address:" << strerror(errno);
    return false;
  }

  return true;
}

bool udp_socket::connect(const std::string& ip, uint16_t port) {
  // build the address
  struct sockaddr_in remote_addr;
  bzero(&remote_addr, sizeof(remote_addr));
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_addr.s_addr = inet_addr(ip.c_str());
  remote_addr.sin_port = htons(port);

  // connect the socket to the address
  if (::connect(sock_fd_, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
    loge() << "failed to connect remote peer:" << strerror(errno);
    return false;
  }

  return true;
}

ssize_t udp_socket::recvfrom(uint8_t* buf, ssize_t len, int flag, struct sockaddr* src_addr,
                             socklen_t* addr_len) {
  return ::recvfrom(sock_fd_, buf, len, flag, src_addr, addr_len);
}

ssize_t udp_socket::recv(uint8_t* buf, ssize_t len, int flag, struct sockaddr* src_addr,
                         socklen_t* addr_len) {
  return ::recv(sock_fd_, buf, len, flag);
}

ssize_t udp_socket::sendto(const uint8_t* buf, ssize_t len, int flag, struct sockaddr* dst_addr,
                           socklen_t addr_len) {
  return ::sendto(sock_fd_, buf, len, flag, dst_addr, addr_len);
}

ssize_t udp_socket::send(const uint8_t* buf, ssize_t len, int flag) {
  return ::send(sock_fd_, buf, len, flag);
}

void udp_socket::shutdown() {
  ::shutdown(sock_fd_, SHUT_RDWR);
}

void udp_socket::close() {
  // close the socket
  if (sock_fd_ >= 0) {
    ::close(sock_fd_);
    sock_fd_ = -1;
  }
} // namespace vnet
