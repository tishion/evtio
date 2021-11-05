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

static bool g_exit = false;

static void sigexit(int signo) {
  logi() << "exit signal received...";
  g_exit = true;
}

static void set_signal(int signo, void (*handler)(int)) {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));

  sa.sa_handler = (void (*)(int))handler;
  if (sigaction(signo, &sa, nullptr) < 0) {
    loge() << "failed to set signal handler";
  }
}

static void usage(const char* program) {
  std::cout << "Usage:" << program << " ip port" << std::endl;
  std::cout << "Example:" << program << " 0.0.0.0 8000" << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    usage(basename(argv[0]));
    return -1;
  }

  set_signal(SIGINT, sigexit);
  set_signal(SIGQUIT, sigexit);

  udp_socket udp_sock;
  if (!udp_sock.open()) {
    loge() << "failed to open udp socket";
    return -1;
  }

  if (!udp_sock.bind(argv[1], std::atoi(argv[2]))) {
    loge() << "failed to bind socket to specified address";
    return -1;
  }

  evtio::evt engine;
  if (!engine.open()) {
    loge() << "failed to open evt";
    return -1;
  }

  evtio::evt_context sock_ctx(udp_sock.fd(), &udp_sock);
  if (!engine.attach(&sock_ctx, evtio::EVT_OP_READ)) {
    loge() << "failed to attach socket to evt";
    return -1;
  }

  logi() << "server is listening on " << argv[1] << ":" << argv[2];

  evtio::evt_event_list event_list;
  while (!g_exit) {
    if (!engine.wait(event_list, 64, -1)) {
      loge() << "failed to wait evt";
      break;
    }

    for (const auto& evt : event_list) {
      if (evt.flags & evtio::EVT_OP_READ && evt.context->userdata) {
        udp_socket* sock = static_cast<udp_socket*>(evt.context->userdata);

        struct sockaddr src_addr;
        socklen_t addr_len;
        std::vector<uint8_t> buf(1024);
        int rlen = sock->recvfrom(buf.data(), buf.size(), 0, &src_addr, &addr_len);
        if (rlen < 0) {
          loge() << "recvfrom failed with return code:" << rlen;
        } else if (rlen == 0) {
          logi() << "recvfrom returned 0, peer socket closed";
        } else {
          int wlen = sock->sendto(buf.data(), rlen, 0, &src_addr, addr_len);
          logi() << "sendto returned " << wlen;
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