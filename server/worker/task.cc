#include "task.hh"
/**
 * @brief 进行while(callback) 返回return null 结束
 *
 * @return std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>>
 */
std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> task::callback(uint32_t event)
{
    for (int i = 0; i < eventsCallbackMap.size(); i++)
    {
        if (event & eventsCallbackMap[i].first)
        {
            return (eventsCallbackMap[i].second)(nullptr);
        }
    }
    return nullptr;
}
void task::registe_answer()
{
    event.events |= EPOLLIN;
    eventsCallbackMap.emplace_back(
        std::make_pair(EPOLLIN, std::bind(&task::Answer_callback, this, (void *)event.data.ptr)));
}
void task::registe_Hangup()
{
    event.events |= EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    eventsCallbackMap.emplace_back(std::make_pair(EPOLLRDHUP | EPOLLHUP | EPOLLERR,
                                                  std::bind(&task::Hangup_callback, this, (void *)event.data.ptr)));
}

void task::registe_timeout()
{
    // 设置服务器地址和端口
}
task::task(int fd, void *data) : fd(fd)
{
    if (data != nullptr)
        event.data.ptr = data;
    else
        event.data.ptr = this;
}
std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> task::Answer_callback(void *data)
{
    Message_Package<ChatProto::data> msg;
    return msg.Recvs(fd);
}
std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> task::Hangup_callback(void *data)
{
    // void log_out(std::shared_ptr<task> job, mysql::Mysql &sql);
    // std::pair<std::shared_ptr<task>, mysql::Mysql *> *i = (std::pair<std::shared_ptr<task>, mysql::Mysql *> *)data;
    // log_out(i->first, *i->second);
    // delete i;
    return nullptr;
}
#include <mutex>
extern std::mutex online_number_lock;
#include "glog/logging.h"
std::shared_ptr<std::vector<std::shared_ptr<ChatProto::data>>> task::deal_timeout(void *data)
{
    int fd = ((epoll_data_t *)data)->fd;
    uint64_t num_exp;
    if (read(fd, &num_exp, sizeof(uint64_t)) == -1)
        LOG(ERROR) << "timer_fd read false";
    Message_Package<ChatProto::data> hb_p;
    {
        std::lock_guard<std::mutex> lock(online_number_lock);
        // hb_p.data->set_online_num(online_number);
    }
    {
        std::lock_guard<std::mutex> lock(this->fd_mutex);
        // hb_p.send(fd);
    }
    timerfd_settime(this->fd, 0, &timeout_limit_times, nullptr);
    return nullptr;
}
