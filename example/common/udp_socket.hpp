#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H
#pragma once

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <cstdint>
#include <string>

#if defined(_WIN32)
#define SOCKFD SOCKET
#else
#define SOCKFD int
#endif

/**
 * @brief
 *
 */
class udp_socket {
public:
  /**
   * @brief Construct a new udp socket object
   *
   */
  udp_socket();

  /**
   * @brief Destroy the udp socket object
   *
   */
  ~udp_socket();

  /**
   * @brief
   *
   * @return int
   */
  SOCKFD fd();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool open();

  /**
   * @brief Sets the nonblock object
   *
   * @param nonblock
   * @return true
   * @return false
   */
  bool set_nonblock(bool nonblock);

  /**
   * @brief
   *
   * @param ip
   * @param port
   * @return true
   * @return false
   */
  bool bind(const std::string& ip, uint16_t port);

  /**
   * @brief
   *
   * @param ip
   * @param port
   * @return true
   * @return false
   */
  bool connect(const std::string& ip, uint16_t port);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @param src_addr
   * @param addr_len
   * @return int
   */
  int recvfrom(uint8_t* buf, int len, int flag, struct sockaddr* src_addr, int* addr_len);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @return int
   */
  int recv(uint8_t* buf, int len, int flag);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @param dst_addr
   * @param addr_len
   * @return int
   */
  int sendto(const uint8_t* buf, int len, int flag, struct sockaddr* dst_addr, int addr_len);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @return int
   */
  int send(const uint8_t* buf, int len, int flag);

  /**
   * @brief
   *
   */
  void shutdown();

  /**
   * @brief
   *
   */
  void close();

private:
  /**
   * @brief
   *
   */
  SOCKFD sock_fd_ = INVALID_SOCKET;
};
#endif