#include "threadpool.hh"

#include "sql.hh"
#include "worker.hh"
// #include "worker.hh"
extern int threadNumber;
thread_pool::thread_pool::thread_pool(int epfd)
{
    for (int i = 0; i < threadNumber; ++i)
    {
        threads.emplace_back(std::bind(&thread_pool::worker_run, this, epfd));
    }
}
void thread_pool::thread_pool::submit(std::shared_ptr<task> tsk)
{
    std::lock_guard<std::mutex> lock(submit_mutex);
    task_queue.emplace(tsk);
    LOG(INFO) << "submit task :" << tsk->fd << std::endl;
    pop_condition.notify_one();
}
std::shared_ptr<task> thread_pool::thread_pool::pop()
{
    std::unique_lock<std::mutex> lock(pop_mutex);
    LOG(INFO) << "wait" << std::endl;
    pop_condition.wait(lock, [this]() { return !task_queue.empty() || IS_stop; });
    if (task_queue.empty())
        return nullptr;
    auto result = task_queue.front();
    LOG(INFO) << "get task :" << result->fd << std::endl;
    task_queue.pop();
    return result;
}
void thread_pool::thread_pool::stop()
{
    IS_stop = true;
    pop_condition.notify_all();
    for(auto &t : this->threads){
        t.join();
    }
}
void thread_pool::thread_pool::worker_run(int epfd)
{
    mysql::Mysql sqlLink;
    while (1)
    {
        std::shared_ptr<task> job = this->pop();
        if (job == nullptr)
            return;
        if (do_Job(job, sqlLink))
        {
            epoll_event i;
            i.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLET | EPOLLERR;
            i.data = job->event.data;
            int rtv = epoll_ctl(epfd, EPOLL_CTL_MOD, job->fd, &i);
            if (rtv == 0 || (rtv == -1 && errno == ENOENT))
            {
                continue;
            }
            else
            {
                LOG(WARNING) << strerror(errno);
            }
        }
    }
}