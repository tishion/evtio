/**
 * @file evtio.hpp
 * @author Sheen Tian Shen (sheentianshen@gmail.com)
 * @brief A header-only library to leverage the high performance
          multiplex IO capacity on different platforms.
 * @date 2021-11-04
 *
 * @copyright Copyright (c) 2021 Sheen Tian Shen
 *
 */
#ifndef EVTIO_H
#define EVTIO_H
#pragma once

#pragma region stl header
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#pragma endregion

#pragma region platform header
#include <errno.h>
#pragma endregion

#if defined(__ANDROID__)
#define ANDROID
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>

typedef int evt_handle;
#elif defined(__linux__)
#define LINUX
#include <unistd.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>

typedef int evt_handle;

#elif defined(__APPLE__)
#define APPLE
#include <unistd.h>

#include <sys/event.h>

typedef int evt_handle;

#elif defined(_WIN32)
#define WINDOWS
#include <WinSock2.h>

#include <windows.h>

typedef HANDLE evt_handle;

#endif

#define logE() std::cerr
#define logI() std::cout

namespace evtio {
/**
 * @brief
 *
 */
enum evt_operation {
  /**
   * @brief
   */
  EVT_OP_READ = 0x00000001,

  /**
   * @brief
   */
  EVT_OP_WRITE = 0x00000002,

  /**
   * @brief
   */
  EVT_OP_MAX = 0xFFFFFFFF
};

/**
 * @brief
 *
 */
struct evt_context {
  /**
   * @brief
   *
   */
  evt_handle handle;

  /**
   * @brief
   *
   */
  void* userdata;

  /**
   * @brief Construct a new evt context object
   *
   * @param h
   * @param d
   */
  evt_context(evt_handle h, void* d)
      : handle(h)
      , userdata(d) {
  }
};

/**
 * @brief
 *
 */
struct evt_event {
  /**
   * @brief
   *
   */
  uint32_t flags;

  /**
   * @brief
   *
   */
  evt_context* context;

  /**
   * @brief Construct a new evt event object
   *
   * @param f
   * @param ctx
   */
  evt_event(uint32_t f, evt_context* ctx)
      : flags(f)
      , context(ctx) {
  }
};

/**
 * @brief
 *
 */
typedef std::vector<evt_event> evt_event_list;

namespace impl {
/**
 * @brief
 *
 */
class evt_impl {
public:
  /**
   * @brief Destroy the evt impl object
   *
   */
  virtual ~evt_impl(){};

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  virtual bool open() = 0;

  /**
   * @brief
   *
   * @param context
   * @param flags
   * @return true
   * @return false
   */
  virtual bool attach(evt_context* context, uint32_t flags) = 0;

  /**
   * @brief
   *
   * @param context
   * @return true
   * @return false
   */
  virtual bool detach(evt_context* context) = 0;

  /**
   * @brief
   *
   * @param event_list
   * @param max_count
   * @param timeout_ms
   * @return true
   * @return false
   */
  virtual bool wait(evt_event_list& event_list, int max_count, int64_t timeout_ms) = 0;

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  virtual bool wakeup() = 0;

  /**
   * @brief
   *
   */
  virtual void close() = 0;
};
} // namespace impl

namespace {

#if defined(__APPLE__)
class evt_kqueue : public impl::evt_impl {
private:
  int kqfd_ = -1;
  int wakeup_evt_id_ = 1;

protected:
  bool kqueue_create() {
    if (kqfd_ >= 0) {
      return true;
    }

    kqfd_ = ::kqueue();
    if (kqfd_ < 0) {
      logE() << "failed to create kqueue instance:(" << errno << ")" << strerror(errno);
      return false;
    }

    return true;
  }

  void kqueue_destroy() {
    if (kqfd_ < 0) {
      ::close(kqfd_);
    }
  }

  bool kqueue_add(evt_context* context, int flags) {
    // validate the arguments
    if (kqfd_ < 0 || (!(flags & EVT_OP_READ) && !(flags & EVT_OP_WRITE))) {
      logE() << "invalid kqueue instance";
      return false;
    }

    std::vector<struct kevent> evts;
    // build read event
    if (flags & EVT_OP_READ) {
      struct kevent ev;
      EV_SET(&ev, context->handle, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, context);
      evts.push_back(ev);
    }

    // build write event
    if (flags & EVT_OP_WRITE) {
      struct kevent ev;
      EV_SET(&ev, context->handle, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, context);
      evts.push_back(ev);
    }

    // associate the socket with kqueue read filter
    struct kevent ev;
    EV_SET(&ev, context->handle, EVFILT_READ, EV_ADD, 0, 0, context);
    if (::kevent(kqfd_, evts.data(), evts.size(), nullptr, 0, nullptr) < 0) {
      logE() << "failed to associate the handle with kqueue read filter:(" << errno << ")"
             << strerror(errno);
      return false;
    }

    return true;
  }

  bool kqueue_remove(evt_handle h) {
    if (kqfd_ < 0) {
      logE() << "invalid epoll instance:(" << errno << ")" << strerror(errno);
      return false;
    }

    // remove the socket from the kqueue
    struct kevent ev;

    // remove from read filter
    EV_SET(&ev, h, EVFILT_READ, EV_DELETE, 0, 0, 0);
    if (0 != ::kevent(kqfd_, &ev, 1, nullptr, 0, nullptr) < 0 && errno == ENOENT) {
      logE() << "failed to remove the handle form the kqueue read list:(" << errno << ")"
             << strerror(errno);
    }

    // remove from write filter
    EV_SET(&ev, h, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
    if (::kevent(kqfd_, &ev, 1, nullptr, 0, nullptr) < 0 && errno == ENOENT) {
      logE() << "failed to remove the handle form the kqueue write list:(" << errno << ")"
             << strerror(errno);
    }

    return true;
  }

  bool kqueue_wait(evt_event_list& event_list, int max_count, int timeout_ms) {
    // clear the result vector
    event_list.clear();

    struct timespec limit;
    if (timeout_ms >= 0) {
      limit.tv_sec = timeout_ms / 1000;
      limit.tv_nsec = (timeout_ms % 1000) * 1000000;
    }

    // query the kqueue to get all signaled events
    std::vector<struct kevent> evts(max_count);
    int nfd = ::kevent(kqfd_, nullptr, 0, evts.data(), evts.size(), timeout_ms < 0
                       : nullptr
                       : &limit);
    if (nfd < 0) {
      logE() << "failed to query the socket status from kqueue:(" << errno << ")"
             << strerror(errno);
      return false;
    }

    for (int i = 0; i < nfd; i++) {
      // check the wakeup event
      if (evts[i].filter == EVFILT_USER && evts[i].ident == wakeup_evt_id_) {
        logI() << "kqueue wakeup event received";
        continue;
      }
      if (evts[i].filter == EVFILT_READ) {
        event_list.emplace_back(static_cast<uint32_t>(EVT_OP_READ),
                                static_cast<evt_context*>(evts[i].udata));
      } else if (evts[i].filter == EVFILT_WRITE) {
        event_list.emplace_back(static_cast<uint32_t>(EVT_OP_WRITE),
                                static_cast<evt_context*>(evts[i].udata));
      } else {
        logE() << "unknown event filter";
      }
    }
    return true;
  }

  bool kqueue_add_wakeup_event() {
    struct kevent ev;
    EV_SET(&ev, wakeup_evt_id_, EVFILT_USER, EV_ADD | EV_CLEAR | EV_ENABLE, 0, 0, 0);
    if (::kevent(kqfd_, &ev, 1, nullptr, 0, nullptr) < 0) {
      logE() << "failed to associate the wakeup event with kqueue:(" << errno << ")"
             << strerror(errno);
      return false;
    }

    return true;
  }

  void kqueue_remove_wakeup_event() {
    struct kevent ev;
    EV_SET(&ev, wakeup_evt_id_, EVFILT_USER, EV_DELETE | EV_DISABLE, 0, 0, 0);
    if (::kevent(kqfd_, &ev, 1, nullptr, 0, nullptr) < 0) {
      logE() << "failed to remove the wakeup event from the kqueue:(" << errno << ")"
             << strerror(errno);
    }
  }

  bool kqueue_wakeup() {
    struct kevent event;
    event.ident = wakeup_evt_id_;
    event.filter = EVFILT_USER;
    event.fflags = NOTE_TRIGGER;
    if (::kevent(kqfd_, &event, 1, nullptr, 0, nullptr) < 0) {
      logE() << "failed to signal the wakeup event for kqueue:(" << errno << ")" << strerror(errno);
      return false;
    }

    return true;
  }

public:
  bool open() override {
    if (!kqueue_create()) {
      return false;
    };

    if (!kqueue_add_wakeup_event()) {
      kqueue_destroy();
      return false;
    }

    return true;
  }

  bool attach(evt_context* context, uint32_t flags) override {
    if (!context || (!(flags & EVT_OP_READ) && !(flags & EVT_OP_WRITE))) {
      return false;
    }
    return kqueue_add(context, flags);
  }

  bool detach(evt_context* context) override {
    if (!context) {
      return false;
    }
    return kqueue_remove(context->handle);
  }

  bool wait(evt_event_list& event_list, int max_count, int timeout_ms) override {
    return kqueue_wait(event_list, max_count, timeout_ms);
  }

  bool wakeup() override {
    return kqueue_wakeup();
  }

  void close() override {
    kqueue_remove_wakeup_event();
    kqueue_destroy();
  }
};
#endif

#if defined(__linux__)
class evt_epoll : public impl::evt_impl {
private:
  int epfd_ = -1;

  int wakeup_fd_ = -1;

protected:
  bool epoll_create() {
    // exist already
    if (epfd_ >= 0) {
      return true;
    }

    // create epoll instance
    epfd_ = ::epoll_create(1024);
    if (epfd_ < 0) {
      logE() << "failed to create epoll instance:(" << errno << ")" << strerror(errno);
      return false;
    }

    return true;
  }

  void epoll_destroy() {
    if (epfd_ < 0) {
      ::close(epfd_);
    }
  }

  bool epoll_add(evt_context* context, int flags) {
    // validate the epoll instance
    if (epfd_ < 0 || (!(flags & EVT_OP_READ) && !(flags & EVT_OP_WRITE))) {
      logE() << "invalid epoll instance";
      return false;
    }

    // associate the socket with epoll
    struct epoll_event ev;
    // !note we use level trigger here
    uint32_t f = 0;
    if (flags & EVT_OP_READ) {
      f |= EPOLLIN;
    }
    if (flags & EVT_OP_WRITE) {
      f |= EPOLLOUT;
    }

    ev.events = f;
    ev.data.ptr = context;
    if (0 != ::epoll_ctl(epfd_, EPOLL_CTL_ADD, context->handle, &ev)) {
      logE() << "failed to associate the handle with epoll:(" << errno << ")" << strerror(errno);
      return false;
    }

    return true;
  }

  bool epoll_remove(evt_handle h) {
    // validate the epoll instance
    if (epfd_ < 0) {
      logE() << "invalid epoll instance:(" << errno << ")" << strerror(errno);
      return false;
    }

    // remove the socket from epoll
    if (0 != ::epoll_ctl(epfd_, EPOLL_CTL_DEL, h, nullptr)) {
      logE() << "failed to remove the handle from epoll:(" << errno << ")" << strerror(errno);
      return false;
    }

    return true;
  }

  bool epoll_wait(evt_event_list& event_list, int max_count, int64_t timeout_ms) {
    // clear the result vector
    event_list.clear();

    // query the epoll to get all signaled events
    std::vector<struct epoll_event> evts(max_count);
    int nfd = 0;
    do {
      nfd = ::epoll_wait(epfd_, evts.data(), evts.size(), timeout_ms < 0 ? -1 : timeout_ms);
    } while (nfd < 0 && EINTR == errno);
    if (nfd < 0) {
      logE() << "failed to query the handle from the epoll:(" << errno << ")" << strerror(errno);
    }

    // process the result
    for (int i = 0; i < nfd; i++) {
      // validate the event data
      if (!evts[i].data.ptr) {
        continue;
      }

      // read eventfd to reset its status
      if (evts[i].data.ptr == nullptr) {
        uint64_t n;
        ::read(wakeup_fd_, &n, sizeof(uint64_t));
      }

      // build event
      uint32_t flags = 0;
      if (evts[i].events & EPOLLIN) {
        flags |= EVT_OP_READ;
      }
      if (evts[i].events & EPOLLOUT) {
        flags |= EVT_OP_WRITE;
      }
      if (flags == 0) {
        logE() << "unknown event type";
        continue;
      }

      // push back the event
      event_list.emplace_back(flags, static_cast<evt_context*>(evts[i].data.ptr));
    }

    return true;
  }

  bool epoll_add_wakeup_event() {
    wakeup_fd_ = eventfd(0, EFD_NONBLOCK);
    if (wakeup_fd_ < 0) {
      logE() << "failed to create eventfd:(" << errno << ")" << strerror(errno);
      return false;
    }
    return epoll_add(wakeup_fd_, EVT_OP_READ, nullptr);
  }

  void epoll_remove_wakeup_event() {
    epoll_remove(wakeup_fd_);
  }

  bool epoll_wakeup() {
    uint64_t n = 1;
    return (sizeof(uint64_t) == write(wakeup_fd_, &n, sizeof(uint64_t)));
  }

public:
  bool open() override {
    if (!epoll_create()) {
      return false;
    };

    if (!epoll_add_wakeup_event()) {
      epoll_destroy();
      return false;
    }

    return true;
  }

  bool attach(evt_context* context, uint32_t flags) override {
    if (!context || (!(flags & EVT_OP_READ) && !(flags & EVT_OP_WRITE))) {
      return false;
    }
    return epoll_add(context, flags);
  }

  bool detach(evt_context* context) override {
    if (!context) {
      return false;
    }
    return epoll_remove(context->handle);
  }

  bool wait(evt_event_list& event_list, int max_count, int64_t timeout_ms) override {
    return epoll_wait(event_list, max_count, timeout_ms);
  }

  bool wakeup() override {
    return epoll_wakeup();
  }

  void close() override {
    epoll_remove_wakeup_event();
    epoll_destroy();
  }
};
#endif

#if defined(_WIN32)
class evt_iocp : public impl::evt_impl {
private:
  typedef struct OverlappedContext : OVERLAPPED {
    int operation;

    OverlappedContext(int op, void* ctx) {
      memset(this, 0, sizeof(OverlappedContext));
      operation = op;
      Pointer = ctx;
    }

    void Reset() {
      memset(this, 0, sizeof(OverlappedContext));
    }

    void Reset(int op, void* ctx) {
      memset(this, 0, sizeof(OverlappedContext));
      operation = op;
      Pointer = ctx;
    }
  } OverlappedContext;

  typedef std::unordered_map<HANDLE, OverlappedContext*> OverlappedContextMap;

private:
  HANDLE iocp_ = nullptr;
  OverlappedContextMap overlapped_map_;
  CRITICAL_SECTION cs_lock_;

protected:
  void iocp_clear_ctx() {
    ::EnterCriticalSection(&cs_lock_);
    for (auto& kv : overlapped_map_) {
      if (kv.second) {
        delete kv.second;
      }
    }
    overlapped_map_.clear();
    ::LeaveCriticalSection(&cs_lock_);
  }

  OverlappedContext* iocp_create_ctx(evt_context* ctx, int op) {
    auto pOverlappedCtx = new OverlappedContext(op, ctx);
    pOverlappedCtx->Pointer = ctx;
    pOverlappedCtx->operation = op;

    ::EnterCriticalSection(&cs_lock_);
    auto it = overlapped_map_.find(ctx->handle);
    if (it != overlapped_map_.end()) {
      if (it->second) {
        delete it->second;
      }
      it->second = pOverlappedCtx;
    } else {
      overlapped_map_[ctx->handle] = pOverlappedCtx;
    }
    ::LeaveCriticalSection(&cs_lock_);

    return pOverlappedCtx;
  }

  OverlappedContext* iocp_get_ctx(HANDLE h) {
    OverlappedContext* p = nullptr;

    ::EnterCriticalSection(&cs_lock_);
    auto it = overlapped_map_.find(h);
    if (it != overlapped_map_.end()) {
      if (it->second) {
        p = it->second;
      } else {
        overlapped_map_.erase(it);
      }
    }
    ::LeaveCriticalSection(&cs_lock_);

    return p;
  }

  void iocp_destroy_ctx(HANDLE h) {
    ::EnterCriticalSection(&cs_lock_);
    auto it = overlapped_map_.find(h);
    if (it != overlapped_map_.end()) {
      if (it->second) {
        delete it->second;
      }
      overlapped_map_.erase(it);
    }
    ::LeaveCriticalSection(&cs_lock_);
  }

  bool PostRecvOperation(HANDLE h, OverlappedContext* overlapped) {
    overlapped->Reset(EVT_OP_READ, overlapped->Pointer);
    WSABUF buf = {0, 0};
    DWORD flags = MSG_PEEK;

    // post an overlapped read operation
    int rc = ::WSARecv((SOCKET)h, &buf, 1, nullptr, &flags, overlapped, nullptr);
    int ec = ::WSAGetLastError();
    if ((rc == SOCKET_ERROR) && (WSA_IO_PENDING != ec)) {
      logE() << "WSARecv failed with error: " << ec << std::endl;
      return false;
    }

    return true;
  }

  bool iocp_create() {
    ::InitializeCriticalSection(&cs_lock_);

    // create IOCP instance
    iocp_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (nullptr == iocp_) {
      logE() << "failed to create IOCP instance:" << GetLastError() << std::endl;
      ::DeleteCriticalSection(&cs_lock_);
      return false;
    }

    iocp_clear_ctx();
    return true;
  }

  void iocp_destroy() {
    if (iocp_) {
      // close IOCP instance
      ::CloseHandle(iocp_);
      iocp_ = nullptr;
    }

    iocp_clear_ctx();
    ::DeleteCriticalSection(&cs_lock_);
  }

  bool iocp_add(evt_context* context, int flags) {
    if (iocp_ == nullptr) {
      logE() << "invalid IOCP instance" << std::endl;
      return false;
    }

    if (!(flags & EVT_OP_READ) || (flags & EVT_OP_WRITE)) {
      logE() << "Invalid operation, only read operation is supported" << std::endl;
      return false;
    }

    auto overlapped = iocp_create_ctx(context, EVT_OP_READ);
    if (!overlapped) {
      logE() << "failed to create overlapped context IOCP instance" << std::endl;
      return false;
    }

    // associate the socket with IOCP instance
    if (NULL ==
        ::CreateIoCompletionPort((HANDLE)context->handle, iocp_, (ULONG_PTR)context->handle, 0)) {
      logE() << "failed to associate the socket with IOCP instance:" << GetLastError() << std::endl;
      iocp_destroy_ctx(context->handle);
      return false;
    }

    // post first read operation
    if (!PostRecvOperation(context->handle, overlapped)) {
      logE() << "failed to post overlapped read operation" << std::endl;
      iocp_destroy_ctx(context->handle);
      return false;
    }

    return true;
  }

  bool iocp_remove(HANDLE h) {
    if (iocp_ == nullptr) {
      logE() << "invalid IOCP instance" << std::endl;
      return false;
    }

    // cancel pending operation if any
    ::CancelIo(h);
    iocp_destroy_ctx(h);
    return true;
  }

  bool iocp_wait(evt_event_list& event_list, int max_count, int64_t timeout_ms) {
    // clear the result vector
    event_list.clear();

    // get queued completion status
    ULONG ulRemoved = 0;
    std::vector<OVERLAPPED_ENTRY> evts(max_count);
    if (!::GetQueuedCompletionStatusEx(iocp_, evts.data(), static_cast<ULONG>(evts.size()),
                                       &ulRemoved, static_cast<DWORD>(timeout_ms), FALSE)) {
      int ec = GetLastError();
      if (WAIT_TIMEOUT != ec) {
        logE() << "failed to get completed port:" << ec << std::endl;
      }

      // operation was canceled
      if (ERROR_OPERATION_ABORTED == ec) {
        logE() << "operation canceled" << std::endl;
      }

      return false;
    }

    // process the result
    for (ULONG i = 0; i < ulRemoved; i++) {
      // validate the event data
      if (evts[i].lpOverlapped == nullptr) {
        // wakeup event or invalid socket
        continue;
      }

      // convert to OverlappedContext*
      OverlappedContext* pOverlappedCtx = static_cast<OverlappedContext*>(evts[i].lpOverlapped);

      // build event
      uint32_t flags = pOverlappedCtx->operation;
      if (flags != EVT_OP_READ) {
        logE() << "invalid event type" << std::endl;
        continue;
      }

      // push back the event
      auto ctx = static_cast<evt_context*>(pOverlappedCtx->Pointer);
      event_list.emplace_back(flags, ctx);

      // launch next read operation
      if (!PostRecvOperation(ctx->handle, pOverlappedCtx)) {
        logE() << "failed to post overlapped read operation" << std::endl;
      }
    }

    return true;
  }

  bool iocp_wakeup() {
    if (iocp_) {
      return (TRUE == ::PostQueuedCompletionStatus(iocp_, 0, 0, nullptr));
    }

    return false;
  }

public:
  bool open() override {
    if (!iocp_create()) {
      return false;
    };

    return true;
  }

  bool attach(evt_context* context, uint32_t flags) override {
    if (!context || (!(flags & EVT_OP_READ) && !(flags & EVT_OP_WRITE))) {
      return false;
    }
    return iocp_add(context, flags);
  }

  bool detach(evt_context* context) override {
    if (!context) {
      return false;
    }
    return iocp_remove(context->handle);
  }

  bool wait(evt_event_list& context_list, int max_count, int64_t timeout_ms) override {
    return iocp_wait(context_list, max_count, timeout_ms);
  }

  bool wakeup() override {
    return iocp_wakeup();
  }

  void close() override {
    iocp_destroy();
  }
};
#endif
} // namespace

class evt {
public:
  evt()
      :
#if defined(__linux__)
      impl_(new evt_epoll())
#elif defined(__APPLE__)
      impl_(new evt_kqueue())
#elif defined(_WIN32)
      impl_(new evt_iocp())
#else
#error "unsupported platform"
#endif
  {
  }

  ~evt() {
  }

  bool open() {
    return impl_->open();
  }

  bool attach(evt_context* context, uint32_t flags) {
    return impl_->attach(context, flags);
  }

  bool detach(evt_context* context) {
    return impl_->detach(context);
  }

  bool wait(evt_event_list& event_list, int max_count, int64_t timeout_ms) {
    return impl_->wait(event_list, max_count, timeout_ms);
  }

  bool wakeup() {
    return impl_->wakeup();
  }

  void close() {
    impl_->close();
  }

private:
  std::unique_ptr<impl::evt_impl> impl_;
};
#endif
} // namespace evtio
