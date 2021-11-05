#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H
#pragma once

#include <sys/socket.h>

#include <cstdint>
#include <string>

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
  int fd();

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool open();

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
   * @return ssize_t
   */
  ssize_t recvfrom(uint8_t* buf, ssize_t len, int flag, struct sockaddr* src_addr,
                   socklen_t* addr_len);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @param src_addr
   * @param addr_len
   * @return ssize_t
   */
  ssize_t recv(uint8_t* buf, ssize_t len, int flag, struct sockaddr* src_addr, socklen_t* addr_len);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @param dst_addr
   * @param addr_len
   * @return ssize_t
   */
  ssize_t sendto(const uint8_t* buf, ssize_t len, int flag, struct sockaddr* dst_addr,
                 socklen_t addr_len);

  /**
   * @brief
   *
   * @param buf
   * @param len
   * @param flag
   * @return ssize_t
   */
  ssize_t send(const uint8_t* buf, ssize_t len, int flag);

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
  int sock_fd_ = -1;
};
#endif