#pragma once
#include <glog/logging.h>
#include <sys/epoll.h>

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <sys/timerfd.h>
#include <vector>

#include "../../chat.pb.h"
#include "../../proto.hh"
#include "listener.hh"
#include "task.hh"
#include "threadpool.hh"
#include "worker.hh"

namespace Epoll
{
// using msgsptr = ;
class Epoll;
class time_manager
{
  public:
    struct node
    {
        std::map<int, std::shared_ptr<std::weak_ptr<task>>> tasks_w;
    };
    time_manager(Epoll *epp);
    ~time_manager();
    void Recycle(std::weak_ptr<task> *t);
    void update(int fd, std::shared_ptr<task> &in_task);
    void remove();
    std::function<void(std::weak_ptr<task> *)> time_out_call_back;
    std::shared_ptr<time_manager::node> now_use;
    std::shared_ptr<time_manager::node> next_use;
    Epoll *ep;
};
class Epoll
{
    friend class time_manager;

  public:
    Epoll();
    // Epoll();
    ~Epoll()
    {
        close(epollfd);
        close(timerfd);
    }
    std::shared_ptr<thread_pool::thread_pool> tp;

    int unregiste(std::shared_ptr<task> events);

  private:
    int registe(Listener &l);
    int registe_timeout(int timefd);
    int registe(std::shared_ptr<task> events);
    std::vector<epoll_event> revent_arr;
    int epollfd = -1;
    std::map<int, std::shared_ptr<task>> eventMap;
    Listener listener;
    // std::list<int>fds;
    int heartBeat_fd;
    int timerfd;
    // void send_heartbeat(int fd);
    time_manager timer;

  public:
    void run();
};

} // namespace Epoll
