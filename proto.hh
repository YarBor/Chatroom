
#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#define HEART_BEAT 1

#define regular_expression1 R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{0,6}$)"
#define regular_expression2 R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)"
#include "chat.pb.h"

extern uint BUFFER_SIZE;

class Timestamp
{
  public:
    bool validateString(const std::string &input);
    static std::string now();
    Timestamp(std::string const &time);
    std::string cut();
    Timestamp();
    bool operator==(const Timestamp &other) const;
    bool operator<(const Timestamp &other) const;

    static std::shared_ptr<Timestamp> New()
    {
        return std::make_shared<Timestamp>(now());
    }
    std::string const &str();
    std::string data;
};

template <typename T> struct Message_Package
{
    Message_Package(T const &package) : data(std::make_shared<T>(package))
    {
        timestamp = std::move(Timestamp::now());
    }
    Message_Package(std::shared_ptr<T> &package) : data(package){};
    Message_Package() : data(std::make_shared<T>())
    {
        timestamp = std::move(Timestamp::now());
    }
    std::shared_ptr<T> data;
    std::shared_ptr<T> Recv(int &fd);
    std::shared_ptr<std::vector<std::shared_ptr<T>>> Recvs(int fd);
    int send(int &fd);
    std::string debug_string()
    {
        return data->DebugString();
    }
    Timestamp timestamp;
};

template <typename T> int Message_Package<T>::send(int &fd)
{
    // 获取原始的文件描述符标志
    int original_flags = fcntl(fd, F_GETFL, 0);

    // 设置文件描述符为阻塞模式
    int flags = original_flags & ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    std::string tmp = this->data->SerializeAsString();
    if (this->data->GetTypeName().compare("ChatProto.data") == 0)
        if (this->data->timestamp().empty())
        {
            this->data->set_timestamp(this->timestamp.data.empty() ? std::move(Timestamp::now())
                                                                   : std::move(this->timestamp.data));
        }
    size_t size = tmp.size();
    bool i = ::send(fd, &size, sizeof(decltype(size)), 0) > 0 && ::send(fd, tmp.c_str(), tmp.size(), 0) > 0;
    // 恢复原始的文件描述符标志
    fcntl(fd, F_SETFL, original_flags);

    // if (i)
        // std::cout << "SEND()ed \n" << this->data->DebugString() << std::endl << fd << std::endl << std::endl;

    return i;
}

template <typename T> std::shared_ptr<T> Message_Package<T>::Recv(int &fd)
{
    // 获取原始的文件描述符标志
    int original_flags = fcntl(fd, F_GETFL, 0);

    // 设置文件描述符为阻塞模式
    int flags = original_flags & ~O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);

    this->data->Clear();
    char buffer[BUFFER_SIZE];
    size_t size = 0;
    if (recv(fd, &size, sizeof(size_t), 0) <= 0)
    {
        this->data = nullptr;
        close(fd);
        fd = -1;
        return nullptr;
    }
    std::string tmp_string;
    int n = -1;
    while (size > 0)
    {
        int num = recv(fd, buffer, std::min(static_cast<size_t>(BUFFER_SIZE), size), 0);
        if (num <= 0)
        {
            this->data = nullptr;
            close(fd);
            fd = -1;
            return nullptr;
        }
        tmp_string.append(buffer, num);
        size -= num;
    }
    this->data->Clear();
    this->data->ParseFromString(tmp_string);

    // std::cout << "RECV()ed \n" << this->data->DebugString() << std::endl << fd << std::endl << std::endl;

    // 恢复原始的文件描述符标志
    fcntl(fd, F_SETFL, original_flags);

    return this->data;
}

template <typename T> std::shared_ptr<std::vector<std::shared_ptr<T>>> Message_Package<T>::Recvs(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    assert(flags != -1);
    // return nullptr;
    assert(-1 != fcntl(fd, F_SETFL, flags | O_NONBLOCK));
    // return nullptr;
    std::shared_ptr<std::vector<std::shared_ptr<T>>> result = std::make_shared<std::vector<std::shared_ptr<T>>>();
    char buffer[BUFFER_SIZE];
    size_t size = 0;
    while (1)
    {
        // if (recv(fd, &size, sizeof(size_t), 0) <= 0)
        // break;
        int t = 0;

        while (1)
        {
            if (t == sizeof(size_t))
                break;
            else
            {
                int bt = recv(fd, &size, sizeof(size_t) - t, 0);
                if (bt == -1 || ((bt == 0 && (errno & (EAGAIN | EWOULDBLOCK))) && t == 0))
                    goto done;
                else
                    t += bt;
            }
        }

        ssize_t n = -1;
        std::string tmp;
        while (size > 0)
        {
            n = recv(fd, buffer, std::min((size_t)BUFFER_SIZE, size), 0);
            if (n == -1 && !(errno & (EAGAIN | EWOULDBLOCK)))
                goto done;
            tmp.append(buffer, n > 0 ? n : 0);
            size -= n > 0 ? n : 0;
        }
        std::shared_ptr<T> i = std::make_shared<T>();
        if (i->ParseFromString(tmp) == false)
            std::cerr << "Parse error recver " << tmp << std::endl;
        result->push_back(i);
        // size = 0;
    }
done:
    return result->size() > 0 ? result : nullptr;
}

template <typename T> bool msg_send(int &fd, const T &input_package, ChatProto::data_data_mode mode)
{
    Message_Package<ChatProto::data> msg;
    msg.data->set_action(mode);
    msg.data->add_details()->PackFrom(input_package);
    msg.data->set_timestamp(Timestamp::now());

    return msg.send(fd);
}

std::string recv_prase_return_package(int &skfd);