/**
 * @file echo_udp_server.cpp
 * @author Sheen Tian Shen (sheentianshen@gmail.com)
 * @brief
 * @date 2021-11-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#include <signal.h>
#include <string.h>

#include <iostream>

#include <evtio.hpp>

#include "common/log.hpp"
#include "common/udp_socket.hpp"

#if defined(_WIN32)

#pragma comment(lib, "ws2_32")

class WindowsSocketApi {
private:
  bool initialized = false;

public:
  WindowsSocketApi() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
      initialized = false;
      loge() << "WSAStartup failed with error: " << err;
      return;
    }

    initialized = true;
    return;
  }

  ~WindowsSocketApi() {
    if (initialized) {
      WSACleanup();
      initialized = false;
    }
  }
};

WindowsSocketApi gWinSocketApi;
#endif

static bool g_exit = false;

static void sigexit(int signo) {
  logi() << "exit signal received...";
  g_exit = true;
}

// static void usage(const char* program) {
//  std::cout << "Usage:" << program << " ip port" << std::endl;
//  std::cout << "Example:" << program << " 0.0.0.0 8000" << std::endl;
//}

int main(int argc, char* argv[]) {
  //  if (argc != 3) {
  //#if defined(_WIN32)
  //    char fn[64] = {0};
  //    _splitpath(argv[0], nullptr, nullptr, fn, nullptr);
  //    usage(fn);
  //#else
  //    usage(basename(argv[0]));
  //#endif
  //    return -1;
  //  }

  signal(SIGINT, sigexit);

  udp_socket udp_sock;
  if (!udp_sock.open()) {
    loge() << "failed to open UDP socket";
    return -1;
  }

  if (!udp_sock.set_nonblock(true)) {
    loge() << "failed to set non-block mode for socket";
    return -1;
  }

  if (!udp_sock.bind("0.0.0.0", 8888 /*std::atoi(argv[2])*/)) {
    loge() << "failed to bind socket to specified address";
    return -1;
  }

  evtio::evt engine;
  if (!engine.open()) {
    loge() << "failed to open evt";
    return -1;
  }

  evtio::evt_context sock_ctx((evt_handle)udp_sock.fd(), &udp_sock);
  if (!engine.attach(&sock_ctx, evtio::EVT_OP_READ)) {
    loge() << "failed to attach socket to evt";
    return -1;
  }

  logi() << "server is listening on "
         << "0.0.0.0"
         << ":" << 8888 /*argv[2]*/;

  struct sockaddr src_addr;
  memset(&src_addr, 0, sizeof(sockaddr));
  int addr_len = sizeof(sockaddr);

  std::vector<uint8_t> buf(1024);
  evtio::evt_event_list event_list;
  while (!g_exit) {
    if (!engine.wait(event_list, 64, -1)) {
      loge() << "failed to wait evt";
      break;
    }

    for (const auto& evt : event_list) {
      if (evt.flags & evtio::EVT_OP_READ && evt.context->userdata) {
        udp_socket* sock = static_cast<udp_socket*>(evt.context->userdata);

        int rlen = sock->recvfrom(buf.data(), buf.size(), 0, &src_addr, &addr_len);
        if (rlen < 0) {
          loge() << "recvfrom failed with return code:" << errno;
        } else if (rlen == 0) {
          logi() << "recvfrom returned 0, peer socket closed";
        } else {
          int wlen = sock->sendto(buf.data(), rlen, 0, &src_addr, addr_len);
          if (wlen <= 0) {
            loge() << "sendto returned " << wlen;
          }
        }
      }
    }
  }

  engine.detach(&sock_ctx);
  engine.close();

  udp_sock.close();

  logi() << "application is exiting...";
  return 0;
}