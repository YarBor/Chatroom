#include "epoll.hh"
extern struct itimerspec timeout_limit_times;
extern std::string LISTEN_IP;
extern std::string LISTEN_PORT;
extern std::string PROCESS_COMMUNTCTION_PORT;
extern bool multi_Progress;
// #include "../proto.hh"
// int pipefds[2];
// extern std::mutex pipefds_mutex;

Epoll::Epoll::Epoll() : listener(), timer(this)
{
    if (!listener.OK)
    {
        LOG(FATAL) << "Failed to initialize epoll listener " << std::endl;
    }
    epollfd = epoll_create(100); // 参数随意 系统会忽略
    registe(listener);           // 不注册callback是因为 在while(epoll)中处理accept()

    // if 有 调度
    timerfd = timerfd_create(CLOCK_REALTIME, 0);
    registe_timeout(timerfd);
    timerfd_settime(timerfd, 0, &timeout_limit_times, nullptr);

    // registe pipe
    // pipe(pipefds);
    // epoll_event pipeEvent;
    // pipeEvent.events |= EPOLLIN;
    // pipeEvent.data.ptr = NULL;
    // epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefds[0], &pipeEvent);
    this->tp = std::make_shared<thread_pool::thread_pool>(epollfd);
}
int Epoll::Epoll::registe(Listener &l) // 在本线程操作
{
    epoll_event ee;
    ee.events = EPOLLIN;
    ee.data.ptr = &(this->listener);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listener.socketfd(), &ee);
    revent_arr.emplace_back();
    return true;
}
int Epoll::Epoll::registe_timeout(int time_fd)
{
    epoll_event event;
    event.data.ptr = &(this->timer);
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, time_fd, &event);
    revent_arr.emplace_back();
    return true;
}
int Epoll::Epoll::registe(std::shared_ptr<task> events)
{
    eventMap.emplace(events->fd, events);
    auto i = events->event;
    i.events |= EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, events->fd, &i);
    revent_arr.emplace_back();
    return true;
}
int Epoll::Epoll::unregiste(std::shared_ptr<task> events)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, events->fd, nullptr);
    eventMap.erase(events->fd);
    tp->submit(events);
    return true;
}

void Epoll::Epoll::run()
{
    while (true)
    {
        // memset(revent_arr.data(), 0, revent_arr.size() * sizeof(struct epoll_event));
        ssize_t event_nums;
        bool timeout = false;
        do
        {
            event_nums = epoll_wait(epollfd, revent_arr.data(), revent_arr.size(), -1);
        } while (event_nums == -1 && errno == EINTR);

        printf("Epoll returned %ld\n", event_nums);
        if (event_nums == -1)
        {
            LOG(FATAL) << "EPOLL FALSE" << std::endl;
        }
        else
        {
            size_t unlink_time = 0;
            for (auto a = 0; a < event_nums; ++a)
            {
                if (revent_arr[a].events & EPOLLIN && revent_arr[a].data.ptr == &(this->listener))
                {
                    auto fd = listener.Accept();
                    auto new_task = task::New(fd, nullptr);
                    new_task->registe_answer(); // 在task中注册回调
                    new_task->registe_Hangup();
                    registe(new_task); // 向epoll中注册事件
                    timer.update(new_task->fd, new_task);
                }
                else if (revent_arr[a].events & EPOLLIN && revent_arr[a].data.ptr == &(this->timer))
                {
                    timeout = true;
                }
                else if (revent_arr[a].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                {
                    auto i = eventMap.find(((task *)revent_arr[a].data.ptr)->fd);
                    if (i == eventMap.end())
                        continue;
                    i->second->revent = revent_arr[a];
                    // 在里面调submit
                    unregiste(i->second);
                    ++unlink_time;
                }
                else
                {
                    auto i = eventMap.find(((task *)revent_arr[a].data.ptr)->fd);
                    if (i == eventMap.end())
                        continue;
                    i->second->revent = revent_arr[a];
                    tp->submit(eventMap[i->second->fd]);
                }
            }
            while (unlink_time--)
            {
                revent_arr.pop_back();
            }
        }
        if (timeout)
        {
            this->timer.remove();
            uint64_t expirationCount;
            ssize_t bytesRead = read(timerfd, &expirationCount, sizeof(expirationCount));
            timerfd_settime(timerfd, 0, &timeout_limit_times, nullptr);
            LOG(INFO) << "timeout" << std::endl;
            timeout = false;
        }
    }
}

Epoll::time_manager::time_manager(Epoll *epp)
    : ep(epp), now_use(std::make_shared<node>()), next_use(std::make_shared<node>()),
      time_out_call_back(std::bind(&time_manager::Recycle, this, std::placeholders::_1))
{
}
Epoll::time_manager::~time_manager()
{
}

void Epoll::time_manager::Recycle(std::weak_ptr<task> *t)
{
    auto i = t->lock();
    if (i == nullptr)
        return;
    i->revent.events = EPOLLHUP;
    ep->unregiste(i);
    ep->tp->submit(i);
    delete t;
}
void Epoll::time_manager::update(int fd, std::shared_ptr<task> &in_task)
{
    std::shared_ptr<std::weak_ptr<task>> i;
    auto ii = now_use->tasks_w.find(fd);
    if (ii != now_use->tasks_w.end())
    {
        i = ii->second;
    }
    if (i == nullptr)
    {
        i = std::shared_ptr<std::weak_ptr<task>>(new std::weak_ptr<task>(in_task), time_out_call_back);
    }
    next_use->tasks_w[fd] = i;
}
void Epoll::time_manager::remove()
{
    std::shared_ptr<time_manager::node> new_one(new node);
    new_one.swap(now_use);
    now_use.swap(next_use);
}
