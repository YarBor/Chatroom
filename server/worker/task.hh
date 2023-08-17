#pragma once
#include "../../chat.pb.h"
#include "../../proto.hh"
// #include "task.hh"
// #include "worker.hh"
#include <memory>
#include <mutex>
#include <string>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <vector>
extern struct itimerspec timeout_limit_times;
extern uint32_t online_number;
struct task
{
    ~task()
    {
        close(fd);
    }
    static std::shared_ptr<task> New(int fd, void *data)
    {
        return std::make_shared<task>(fd, data);
    };
    // friend std::shared_ptr<task> std::make_shared<task>(int , void *data);
    /**
     * @param revent
     * @return void*
     *  while(callback)获取事件或者包
     */
    std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> callback(uint32_t);
    int fd;
    epoll_event event;
    epoll_event revent;
    void registe_answer();
    void registe_Hangup();
    void registe_timeout();
    uint64_t ID;
    std::string name;
    std::mutex fd_mutex;
    std::mutex write_fd_mutex;
    task(int fd, void *data);
    void *node;

  private:
    std::vector<
        std::pair<uint32_t, std::function<std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>>(void *)>>>
        eventsCallbackMap;
    std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> Answer_callback(void *data);
    std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> Hangup_callback(void *data);
    std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> deal_timeout(void *data);
};
