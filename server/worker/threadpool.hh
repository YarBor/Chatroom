#pragma once
#include <cassert>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "task.hh"
extern int threadNumber;
namespace thread_pool
{
class thread_pool
{
  public:
    thread_pool(int);
    void submit(std::shared_ptr<task> tsk);
    std::shared_ptr<task> pop();
    void stop();
    void worker_run(int);

  private:
    std::vector<std::thread> threads;
    bool IS_stop = false;
    std::mutex submit_mutex;
    std::mutex pop_mutex;
    std::condition_variable pop_condition;
    std::queue<std::shared_ptr<task>> task_queue;
};
} // namespace thread_pool