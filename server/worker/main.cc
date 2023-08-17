#include "epoll.hh"
#include "threadpool.hh"
extern uint TIME_OUT_LIMIT;
extern std::string WORK_PROGRAM_NAME;
extern uint MAX_CANONICAL_PROCESS_NUM;
extern uint PROCESS_THRESHOLD;
extern struct itimerspec heartbeat_interval;
extern std::string LISTEN_IP;
extern std::string LISTEN_PORT;
extern std::string PROCESS_COMMUNTCTION_PORT;
extern int threadNumber;

extern bool multi_Progress;

// uint BUFFER_SIZE = 1024;
void init();

/**
 * undone
 * 销户
 * 如何进行(优雅的)终止
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
Epoll::Epoll *Ep = nullptr;
void handle_sigint(int sig)
{
    Ep->tp->stop();
    printf("Received SIGINT signal. Exiting...\n");
    printf("done\n");
    exit(0);
}
int main()
{
    init();
    signal(SIGINT, handle_sigint);
    mysql_library_init(0, NULL, NULL); // 初始化MySQL库
    /**
     * 从 "config.json" 文件中读取 multi_Progress => <bool>
     *
     * multi_Progress
     * 如果为 true，则 epoll 链接控制进程在 "LISTEN_IP :PROCESS_COMMUNTCTION_PORT" 上运行并且使用随机的 "IP :PORT" 创建
     * listenfd
     *
     * 否则，epoll 只会在 config.json 中创建 listenfd，使用 "LISTEN_IP :LISTEN_PORT"
     */
    {
        mysql::Mysql sql;
        sql("delete from online_users");
    }
    Epoll::Epoll ep;
    Ep = &ep;
    /**
     * 线程数量由 config.json 文件设置
     */
    // thread_pool::thread_pool threads;
    ep.run();
}
