#pragma once
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <unistd.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "../chat.pb.h"
#include "../proto.hh"

std::string calculateMD5(const std::string &input);
using std::shared_ptr;
extern std::shared_ptr<ChatProto::user_data_package> user_data;
struct friend_struct
{
    friend_struct(std::shared_ptr<ChatProto::friend_relation> i)
        : friend_relation(i), is_online(friend_relation->master().is_active())
    {
        if (!friend_relation->has_master())
        {
            friend_relation->mutable_master()->CopyFrom(*user_data);
        }
    }
    std::shared_ptr<ChatProto::friend_relation> friend_relation;
    bool is_online = false;
    bool operator<(const friend_struct &other) const
    {
        return friend_relation->follower().name() < other.friend_relation->follower().name();
    }
};

bool compare_friend(std::shared_ptr<friend_struct> const &i, std::shared_ptr<friend_struct> const &j);
bool compare_group(std::shared_ptr<ChatProto::group_relation> const &i,
                   std::shared_ptr<ChatProto::group_relation> const &j);

class Cache
{ // 要向poll中注册 定时 向文件中刷内存
  public:
    std::string init(int &fd);
    void update_timestamp(Timestamp &&timestamp_of_last_job_done_time);
    void add_friend_relation(std::shared_ptr<ChatProto::friend_relation> fr);
    void add_group_relation(std::shared_ptr<ChatProto::group_relation> gr);
    void reinit(int &skfd);
    void delete_friend_relation(std::shared_ptr<ChatProto::friend_relation> fr);
    void delete_group_relation(std::shared_ptr<ChatProto::group_relation> gr);
    void flush_relation_to_file();
    void add_history_to_file(std::string const &chatid, ChatProto::msg_data_package const &msg);
    std::vector<std::string> get_history_from_file(std::string const &chatid, uint &num);
    std::string get_history_file_name(std::string const &chatid) const;
    void get_online_data(int &skfd);
    const std::string &Timestamp() const
    {
        return timestamp_of_last_job_done_time;
    }
    shared_ptr<friend_struct> find_friend(std::string const &chatid)
    {
        auto a = friend_map->find(chatid);
        return (a == friend_map->end()) ? nullptr : a->second;
    };
    shared_ptr<ChatProto::group_relation> find_group(std::string const &chatid)
    {
        auto a = group_map->find(chatid);
        return (a == group_map->end()) ? nullptr : a->second;
    };
    static std::shared_ptr<ChatProto::group_relation> find_group_from_id(std::string const &id)
    {
        for (auto const &group : *group_map)
        {
            if ((*group.second).id() == id)
                return group.second;
        }
        return nullptr;
    }
    static std::shared_ptr<friend_struct> find_friend_from_id(const int64_t id)
    {
        for (auto const &f : *friend_map)
        {
            if (f.second->friend_relation->follower().id() == id)
                return f.second;
        }
        return nullptr;
    }
    static std::shared_ptr<friend_struct> find_friend_from_id(const std::string &input_id)
    {
        try
        {
            auto id = std::stoul(input_id.c_str());
            for (auto const &f : *friend_map)
            {
                if (f.second->friend_relation->follower().id() == id)
                    return f.second;
            }
            return nullptr;
        }
        catch (const std::exception &e)
        {
            return nullptr;
        }
    }
    void show_friend_data(int fd);
    void show_all_group_data(int fd, bool is_show_details);
    bool show_one_group_data(std::string const &id, int fd);

  protected:
    std::string timestamp_of_last_job_done_time;
    static std::shared_ptr<std::map<std::string, std::shared_ptr<friend_struct>>> friend_map;
    static std::shared_ptr<std::map<std::string, shared_ptr<ChatProto::group_relation>>> group_map;
    std::string history_dir_path = "./historys/";
    // bool is_data_changed = false;
    // std::mutex is_data_changed_lock;
    std::mutex cacheLock;
};

// std::string calculateMD5(const std::string &input)
// ;

class history_show
{
    std::vector<std::string> historys;
    int rows;
    int cols;
    void getTerminalSize();

  public:
    history_show();
    void show(int fd);
    void changed(std::vector<std::string> &&v);
    void add_one(ChatProto::msg_data_package const &msg);
    static int get_T_wide()
    {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        {
            perror("ioctl");
            return 0;
        }
        return ws.ws_col;
    }
    static int get_T_deep()
    {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
        {
            perror("ioctl");
            return 0;
        }
        return ws.ws_row;
    }
};
struct file
{
    static file open(const char *filename, int flags, int flags2)
    {
        return ::open(filename, flags, flags2);
    }
    static file open(const char *filename, int flags)
    {
        return ::open(filename, flags);
    }
    int fd = -1;
    ~file()
    {
        if (fd != -1)
        {
            close(fd);
        }
    }
    int fd_()
    {
        return fd;
    }
    file(file &&f)
    {
        this->fd = f.fd;
        f.fd = -1;
    }
    file &operator=(file &&f)
    {
        if (fd != -1)
            close(fd);
        this->fd = f.fd;
        f.fd = -1;
        return *this;
    }
    file &operator=(file &f) = delete;

  protected:
    file(int fd) : fd(fd)
    {
    };
};
