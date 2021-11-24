#include "udp_socket.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <string.h>

#include "log.hpp"

udp_socket::udp_socket() {
}

udp_socket::~udp_socket() {
}

SOCKFD udp_socket::fd() {
  return sock_fd_;
}

bool udp_socket::open() {
  // open socket
  SOCKFD sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock <= 0) {
    loge() << "failed to open socket:" << strerror(errno);
    return false;
  }

  // set socket address reuse
  int optval = 1;
  if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) < 0) {
    logw() << "failed to set socket option:" << strerror(errno);
  }

#if defined(_WIN32)
  // disable the UDP port unreachable reporting on windows platform
  // refer to: https://docs.microsoft.com/en-us/windows/win32/winsock/winsock-ioctls
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
  BOOL bNewBehavior = FALSE;
  DWORD dwBytesReturned = 0;
  if (0 != ::WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0,
                      &dwBytesReturned, NULL, NULL)) {
    logw() << "Failed to set ignore the connection reset status" << strerror(errno);
  }
#endif

  // keep socket fd
  sock_fd_ = sock;
  return true;
}

bool udp_socket::set_nonblock(bool nonblock) {
#if defined(_WIN32)
  u_long mode = nonblock ? 1 : 0;
  int rc = ::ioctlsocket(sock_fd_, FIONBIO, &mode);
  if (rc == -1) {
    loge() << "failed to set socket to non-blocking: " << strerror(errno);
    return false;
  }
#else
  int flags = ::fcntl(sock_fd_, F_GETFL, 0);
  if (flags < 0) {
    logw() << "failed to get the original flags:" << strerror(errno);
  }

  if (nonblock) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  if (::fcntl(sock_fd_, F_SETFL, flags) < 0) {
    loge() << "failed to change the blocking mode:" << strerror(errno);
    return false;
  }
#endif

  return true;
}

bool udp_socket::bind(const std::string& ip, uint16_t port) {
  // build the address
  struct sockaddr_in local_addr;
  memset(&local_addr, 0, sizeof(local_addr));
  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = inet_addr(ip.c_str());
  local_addr.sin_port = htons(port);

  // bind the socket on the address
  if (::bind(sock_fd_, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
    loge() << "failed to bind socket on address:" << strerror(errno);
    return false;
  }

  return true;
}

bool udp_socket::connect(const std::string& ip, uint16_t port) {
  // build the address
  struct sockaddr_in remote_addr;
  memset(&remote_addr, 0, sizeof(remote_addr));
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

int udp_socket::recvfrom(uint8_t* buf, int len, int flag, struct sockaddr* src_addr,
                         int* addr_len) {
  return ::recvfrom(sock_fd_, (char*)buf, len, flag, src_addr, addr_len);
}

int udp_socket::recv(uint8_t* buf, int len, int flag) {
  return ::recv(sock_fd_, (char*)buf, len, flag);
}

int udp_socket::sendto(const uint8_t* buf, int len, int flag, struct sockaddr* dst_addr,
                       int addr_len) {
  return ::sendto(sock_fd_, (char*)buf, len, flag, dst_addr, addr_len);
}

int udp_socket::send(const uint8_t* buf, int len, int flag) {
  return ::send(sock_fd_, (char*)buf, len, flag);
}

void udp_socket::shutdown() {
#if defined(_WIN32)
  ::shutdown(sock_fd_, SD_BOTH);
#else
  ::shutdown(sock_fd_, SHUT_RDWR);
#endif
}

void udp_socket::close() {
  // close the socket
  if (sock_fd_ >= 0) {
#if defined(_WIN32)
    ::closesocket(sock_fd_);
#else
    ::close(sock_fd_);
#endif
    sock_fd_ = INVALID_SOCKET;
  }
} // namespace vnet
