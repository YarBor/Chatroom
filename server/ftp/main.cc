#include <arpa/inet.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <sys/dir.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../../chat.pb.h"
#include "../../proto.hh"
#include <openssl/evp.h>

int Close(int fd)
{
    printf("fd %d:> will close \n", fd);
    return close(fd);
}

class task;
void init();
void do_job(std::shared_ptr<task> t, int fd);
// 返回的是第一次读完后是否需要加入epoll
bool recv_file(std::shared_ptr<task> t);
// 返回的是第一次读完后是否需要加入epoll
bool send_file(std::shared_ptr<task> t);
extern uint BUFFER_SIZE;

using ee = ChatProto::error_package_mode;
using dd = ChatProto::data_data_mode;
#include <fstream>
#include <openssl/evp.h>
std::string calculateFileMD5(std::fstream &fs)
{
    fs.seekg(0, std::ios::beg);
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    EVP_MD_CTX *ctx;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);

    char buffer[8192];
    while (!fs.eof())
    {
        fs.read(buffer, sizeof(buffer));
        std::streamsize bytes_read = fs.gcount();
        if (bytes_read > 0)
            EVP_DigestUpdate(ctx, buffer, bytes_read);
    }

    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    // Convert to hex string
    std::string md5_str;
    char buf[3];
    for (unsigned int i = 0; i < digest_len; ++i)
    {
        snprintf(buf, sizeof(buf), "%02x", digest[i]);
        md5_str.append(buf);
    }
    return md5_str;
}
std::string calculateFileMD5(int fd)
{
    lseek(fd, 0, SEEK_SET);
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    EVP_MD_CTX *ctx;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);

    char buffer[8192];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        EVP_DigestUpdate(ctx, buffer, bytes_read);
    }

    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    // Convert to hex string
    std::string md5_str;
    char buf[3];
    for (unsigned int i = 0; i < digest_len; ++i)
    {
        snprintf(buf, sizeof(buf), "%02x", digest[i]);
        md5_str.append(buf);
    }
    return md5_str;
}

class lock
{
    friend struct task;
    bool is_locked_ = false;
    lock(std::mutex *i) : mtx(i)
    {
        is_locked_ = i->try_lock();
    }

    std::mutex *mtx;

public:
    bool is_lock() const
    {
        return is_locked_;
    }
    ~lock()
    {
        if (is_locked_ == true)
            mtx->unlock();
    }
};

struct task
{
    task(int fd) : fd(fd)
    {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
        file_data = new ChatProto::file;
    }
    ~task()
    {
        Close(fd);
        LOG(INFO) << "fd closed  :" << fd << std::endl;
        delete file_data;
    }
    int fd;
    ChatProto::file *file_data;
    int status = 0; // 1 recv // 2 send
    epoll_event revent;
    int step = 0;
    std::mutex mtx;
    lock get_lock()
    {
        return &mtx;
    }
};

class Epoll
{
public:
    Epoll(std::string IP, std::string port) : IP(IP), port(port)
    {
        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd == -1)
        {
            perror("socket create error");
            return;
        }

        serverAddress_.sin_family = AF_INET;
        serverAddress_.sin_addr.s_addr = inet_addr(IP.c_str());
        serverAddress_.sin_port = htons(std::stoi(port));

        if (-1 == bind(listenfd, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)) ||
            listen(listenfd, 100) == -1)
        {
            LOG(ERROR) << "Failed to bind / listen  to ip/port " << IP << ":" << port << std::endl
                       << strerror(errno) << std::endl;
            exit(1);
        }
        epfd = epoll_create(100);
        epoll_event ee;
        ee.data.fd = listenfd;
        ee.events = EPOLLIN;
        epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ee);
    }
    ~Epoll()
    {
        Close(epfd);
        Close(listenfd);
    }
    int wait()
    {
        return epoll_wait(epfd, buffer.data(), buffer.size(), -1);
    }
    bool is_listenfd(int fd) const
    {
        return fd == listenfd;
    }
    static void add_event(std::shared_ptr<task> t, uint32_t events)
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks[t->fd] = t;
        epoll_event event;
        event.data.fd = t->fd;
        event.events = events;
        LOG(INFO) << " add  event " << events << " to tasks " << t->fd << "  " << t->file_data->filename() << " "
                  << t->file_data->size() << std::endl;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, t->fd, &event) == -1)
        {
            perror("Error adding file descriptor to epoll");
            // 处理错误的逻辑
        }
    }
    static void remove_event(int fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.erase(fd);
        LOG(INFO) << " del  event " << fd << std::endl;
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0) == -1)
        {
            perror("Error remove file descriptor to epoll");
            // 处理错误的逻辑
        }
    }
    static std::shared_ptr<task> find_event(int fd)
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto i = tasks.find(fd);
        return i == tasks.end() ? nullptr : i->second;
    }
    int listenfd;
    static std::vector<epoll_event> buffer;
    static std::map<int, std::shared_ptr<task>> tasks;
    static int epfd;

private:
    static std::mutex mtx;
    std::string IP;
    std::string port;
    struct sockaddr_in serverAddress_;
};

std::vector<epoll_event> Epoll::buffer(512);
std::map<int, std::shared_ptr<task>> Epoll::tasks;
int Epoll::epfd;
std::mutex Epoll::mtx;

class thread_pool
{
public:
    thread_pool(int nums, int fd)
    {
        for (int i = 0; i < nums; i++)
        {
            threads.emplace_back(&thread_pool::run, this, fd);
        }
    }
    std::shared_ptr<task> pop()
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        pop_condition.wait(lock, [&]()
                           { return !tasks_queue.empty() || is_stop; });

        if (!is_stop)
        {
            auto it = tasks_queue.front();
            tasks_queue.pop();
            return it;
        }
        else
            return nullptr;
    }
    void submit(std::shared_ptr<task> task)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks_queue.emplace(task);
        }
        pop_condition.notify_one();
    }
    void run(int epollfd)
    {
        while (1)
        {
            auto job = this->pop();
            if (job == nullptr)
                return;
            else
            {
                auto lock_ = job->get_lock();
                if (lock_.is_lock())
                    do_job(job, epollfd);
            }
        }
    }
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            is_stop = true;
        }
        pop_condition.notify_all();
    }

private:
    std::vector<std::thread> threads;
    std::condition_variable pop_condition;
    std::mutex queue_mutex; // pop mutex
    std::queue<std::shared_ptr<task>> tasks_queue;
    bool is_stop = false;
};
extern std::string file_server_ip;
extern std::string file_server_port;
extern int file_server_thread_nums;
extern std::string log_path;

int main()
{
    init();
    google::InitGoogleLogging(argv[0]);

    // 设置日志输出目录
    if (log_path.empty())
        FLAGS_log_dir = log_path.c_str();
    auto dir = opendir("files");
    if (dir == NULL)
    {
        mkdir("files", 0755);
    }
    else
    {
        closedir(dir);
    }
    Epoll ep(file_server_ip, file_server_port);
    thread_pool tp(file_server_thread_nums, ep.epfd);
    while (1)
    {
        int unlinktime = 0;
        int num = 0;
        do
        {
            num = epoll_wait(ep.epfd, ep.buffer.data(), ep.buffer.size(), -1);
        } while (num == -1 && errno == EINTR);

        for (auto i = 0; i < num; i++)
        {
            // puts("returned \n");
            if (ep.buffer[i].data.fd == ep.listenfd)
            {
                auto t = std::make_shared<task>(accept(ep.listenfd, nullptr, nullptr));
                LOG(INFO) << "accept " << t->fd << std::endl;
                tp.submit(t);
            }
            else if (ep.buffer[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
            {
                LOG(INFO) << "hup " << ep.buffer[i].data.fd << std::endl;
                ep.remove_event(ep.buffer[i].data.fd);
            }
            else
            {
                // LOG(INFO) << "IN " << ep.buffer[i].data.fd << std::endl;
                auto a = ep.find_event(ep.buffer[i].data.fd);
                if (a != nullptr)
                {
                    tp.submit(a);
                    // if (epoll_ctl(ep.epfd, EPOLL_CTL_DEL, a->fd, 0) == -1)
                    // {
                    //     perror("Error adding file descriptor to epoll");
                    //     // 处理错误的逻辑
                    // }
                }
            }
        }
    }
}

void do_job(std::shared_ptr<task> t, int epollfd)
{
    if (t->status == 0)
    {
        Message_Package<ChatProto::data> msg;
        auto data = msg.Recv(t->fd);
        data->details()[0].UnpackTo(t->file_data);
        t->status = data->action();
        switch (data->action())
        {
        case dd::data_data_mode_SEND_FILE:
        {
            if (recv_file(t))
                Epoll::add_event(t, EPOLLIN | EPOLLHUP | EPOLLERR | EPOLLRDHUP);
            break;
        }
        case dd::data_data_mode_RECV_FILE:
        {
            if (send_file(t))
                Epoll::add_event(t, EPOLLOUT | EPOLLHUP | EPOLLERR | EPOLLRDHUP);
            break;
        }
        default:
            LOG(ERROR) << "Unknown action " << data->DebugString();
        }
    }
    else if (t->status == dd::data_data_mode_SEND_FILE)
    {
        // 重启 epoll 字段
        if (recv_file(t) == false)
            Epoll::remove_event(t->fd);
    }
    else if (t->status == dd::data_data_mode_RECV_FILE)
    {
        // 重启 epoll 字段
        if (send_file(t) == false)
            Epoll::remove_event(t->fd);
    }
}

void send_err(std::shared_ptr<task> &t, ChatProto::error_package_mode mode, std::string const &reason)
{
    auto i = fcntl(t->fd, F_GETFL, 0);
    if (i & O_NONBLOCK)
        fcntl(t->fd, F_SETFL, i - O_NONBLOCK);

    ChatProto::error_package err;
    err.set_stat(mode);
    err.set_reason(reason);
    Message_Package<ChatProto::data> message;

    message.data->set_action(ChatProto::data_data_mode::data_data_mode_false_);
    message.data->add_details()->PackFrom(err);
    message.data->set_timestamp(Timestamp::now());

    message.send(t->fd);
    if (i & O_NONBLOCK)
        fcntl(t->fd, F_SETFL, i);
    return;
}
int send_data_package(std::shared_ptr<task> &t, ChatProto::data_data_mode mode, google::protobuf::Message &message)
{
    auto i = fcntl(t->fd, F_GETFL, 0);
    if (i & O_NONBLOCK)
        fcntl(t->fd, F_SETFL, i - O_NONBLOCK);

    Message_Package<ChatProto::data> mp;
    mp.data->set_action(mode);
    mp.data->add_details()->PackFrom(message);
    mp.data->set_timestamp(Timestamp::now());
    auto is_success = mp.send(t->fd);
    if (i & O_NONBLOCK)
        fcntl(t->fd, F_SETFL, i);
    return is_success;
}

int send_data_package(std::shared_ptr<task> &t, ChatProto::data_data_mode mode)
{
    auto i = fcntl(t->fd, F_GETFL, 0);
    if (i & O_NONBLOCK)
        fcntl(t->fd, F_SETFL, i - O_NONBLOCK);
    Message_Package<ChatProto::data> mp;
    mp.data->set_action(mode);
    mp.data->clear_details();
    mp.data->set_timestamp(Timestamp::now());
    auto is_success = mp.send(t->fd);
    if (i & O_NONBLOCK)
        fcntl(t->fd, F_SETFL, i);
    return is_success;
}

bool send_file(std::shared_ptr<task> t)
{
    std::string filename =
        "./files/" + std::to_string(t->file_data->owner().id()) + std::string("_") + t->file_data->filename();
    int fd = open(filename.c_str(), O_RDONLY);
    switch (t->step)
    {
    case 0:
    {
        if (fd == -1)
        {
            send_err(t, ee::error_package_mode_RECV_FILE_FAIL, "creating file failed");
            return false;
        }
        else
        {
            send_data_package(t, dd::data_data_mode_RECV_FILE_OK);
        }
        t->step++;
    }
    case 1:
    {
        off_t offset = t->file_data->offset();
        auto i = sendfile(t->fd, fd, &offset, t->file_data->size() - t->file_data->offset());
        if (i == -1)
        {
            close(fd);
            return true;
        }
        t->file_data->set_offset(offset);
        if (t->file_data->size() == t->file_data->offset() && i == 0)
        {
            LOG(INFO) << "sended " << t->file_data->filename() << " (" << t->file_data->size() << ") :> " << offset << std::endl;
            t->step++;
        }
        else
        {
            LOG(INFO) << "sended " << t->file_data->filename() << " (" << t->file_data->size() << ") :> " << offset << std::endl;
            close(fd);
            return true;
        }
    }
    default:
        Close(fd);
    }
    // int fd = open(filename.c_str(), O_RDONLY);
    // if (fd < 0)
    // {
    //   send_err(t, ee::error_package_mode_RECV_FILE_FAIL, "creating file failed");
    //   Close(t->fd);
    // }
    // else
    // {
    //   send_data_package(t, dd::data_data_mode_RECV_FILE_OK);
    // }
    // sendfile(t->fd, fd, nullptr, t->file_data->size());
    // Message_Package<ChatProto::data> msg;
    // // msg.Recv(t->fd)->action() == dd::data_data_mode_SEND_FILE_OK;
    // auto data = msg.Recv(t->fd);
    // Close(fd);
    // Close(t->fd);
    // return;
    return false;
}
bool recv_file(std::shared_ptr<task> t)
{
    std::string filename =
        "./files/" + std::to_string(t->file_data->owner().id()) + std::string("_") + t->file_data->filename();
    int i = open(filename.c_str(), O_RDWR | O_CREAT, 0644);

    if (i == -1)
    {
        std::cerr << "Failed to open file. Error code: " << errno << ", Error message: " << strerror(errno)
                  << std::endl;
        return false;
    }
    else
        close(i);

    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    auto size = t->file_data->size();
    char buffer[BUFFER_SIZE];
    switch (t->step)
    {
    case 0:
    {
        if (!file.is_open())
        {
            send_err(t, ee::error_package_mode_SEND_FILE_FAIL, strerror(errno));
            perror("Error creating file : ");
            file.close();
            return false;
            // 对端关闭 skfd
        }
        else
        {
            send_data_package(t, dd::data_data_mode_SEND_FILE_OK);
        }
        t->step++;
    }
    case 1:
    {
        auto i = t->file_data->offset();
        file.seekp(i, std::ios::beg);
        auto size = t->file_data->size();
        while (i < size)
        {
            auto rb = recv(t->fd, buffer, std::min((long)BUFFER_SIZE, size - i), 0);
            if (rb <= 0)
            {
                if (rb == -1 && (errno & (EAGAIN | EWOULDBLOCK)))
                {
                    t->file_data->set_offset(i);
                    file.close();
                    LOG(INFO) << "recved " << t->file_data->filename() << " (" << t->file_data->size() << ") :> " << i << std::endl;
                    return true;
                }
                perror("Error reading");
                Close(t->fd);
                file.close();
                return false;
            }
            else
            {
                file.write(buffer, rb);
                i += rb;
                t->file_data->set_offset(i);
            }
        }
        if (file.is_open())
            file.close();

        // int iasdf = open(filename.c_str(), O_RDONLY);

        // if (!(calculateFileMD5(iasdf) == t->file_data->md5()))
        // {
        // send_err(t, ee::error_package_mode_SEND_FILE_FAIL, "md5 check failed");
        // file.close();
        // return false;
        // }
        // else
        t->step++;
        // Close(i);
    }
    case 2:
    {
        send_data_package(t, dd::data_data_mode_RECV_FILE_OK);
        t->step++;
        if (file.is_open())
            file.close();
    }
    default:
        return false;
    }
    // int fd = open(filename.c_str(), O_RDWR | O_TRUNC);
    // if (fd < 0)
    // {
    //   send_err(t, ee::error_package_mode_SEND_FILE_FAIL, "creating file failed");
    //   Close(t->fd);
    // }
    // else
    // {
    //   send_data_package(t, dd::data_data_mode_SEND_FILE_OK);
    // }
    // auto size = t->file_data->size();
    // char buffer[BUFFER_SIZE];
    // while (size > 0)
    // {
    //   auto bytes =
    //       recv(t->fd, buffer, size > BUFFER_SIZE ? size : BUFFER_SIZE, 0);
    //   if (bytes < 0)
    //   {
    //     LOG(WARNING) << "recv error " << t->file_data->DebugString();
    //     send_err(t, ee::error_package_mode_SEND_FILE_FAIL, "md5 check failed");
    //     Close(t->fd);
    //     Close(fd);
    //     unlink(filename.c_str());
    //     return;
    //   }
    //   assert(0 > write(fd, buffer, bytes));
    //   size -= bytes;
    // }
    // if (calculateFileMD5(fd) == t->file_data->md5())
    // {
    //   send_data_package(t, dd::data_data_mode_RECV_FILE_OK);
    //   Close(t->fd);
    //   Close(fd);
    // }
    // else
    // {
    //   send_err(t, ee::error_package_mode_SEND_FILE_FAIL, "md5 check failed");
    //   Close(t->fd);
    //   Close(fd);
    //   unlink(filename.c_str());
    // }
}
