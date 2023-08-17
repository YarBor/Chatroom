#include "Cache&Log.hh"
extern std::deque<std::shared_ptr<ChatProto::data>> package_queue;
std::shared_ptr<std::map<std::string, std::shared_ptr<friend_struct>>> Cache::friend_map =
    std::make_shared<std::map<std::string, std::shared_ptr<friend_struct>>>();
std::shared_ptr<std::map<std::string, std::shared_ptr<ChatProto::group_relation>>> Cache::group_map =
    std::make_shared<std::map<std::string, shared_ptr<ChatProto::group_relation>>>();
std::string Cache::init(int &skfd)
{
    bool is_first = false;
    std::string return_string;
    auto dir = opendir(history_dir_path.c_str());
    if (dir == nullptr)
        mkdir(history_dir_path.c_str(), 0755);
    else
        closedir(dir);
    // 创建 历史记录目录

    auto data = std::make_shared<ChatProto::data>();
    // 向这个data里面进行 数据填充 从而初始化

    std::ifstream i("." + user_data->name() + "_" + std::to_string(user_data->id()));
    // 打开 用户文件 加载 用户关系
    if (!i.is_open())
    {
        // 没打开
        std::ofstream out("." + user_data->name() + "_" + std::to_string(user_data->id()));
        if (!out.is_open())
        {
            perror("flie creation");
        }
        // 创建
        Message_Package<ChatProto::data> msg(data);
        msg.data->add_details()->PackFrom(*user_data);
        msg.data->set_action(ChatProto::data_data_mode_LOAD_USER_RELATION_DATA);
        msg.send(skfd);
        do
        {
            msg.Recv(skfd);
            if (msg.data == nullptr)
            {
                dprintf(STDOUT_FILENO, "server closed connection\n");
 
                return "";
            }
            if (msg.data->action() == ChatProto::data_data_mode_LOAD_USER_RELATION_DATA_OK)
            {
                break;
            }
            else if (msg.data->action() == ChatProto::data_data_mode_false_)
            {
                dprintf(STDOUT_FILENO, "initialization cache false \n");
                return "";
            }
            else
            {
                package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
            }
        } while (1);
        data->SerializeToOstream(&out);
        // 下载用户关系 写到文件 并初始化
        out.close();
        is_first = true;
    }
    else
    {
        // 打开了
        // 初始化 data

        if (!data->ParseFromIstream(&i))
        {
            // 初始化不成功 下载 写到文件
            Message_Package<ChatProto::data> msg(data);
            msg.data->add_details()->PackFrom(*user_data);
            msg.data->set_action(ChatProto::data_data_mode_LOAD_USER_RELATION_DATA);
            msg.send(skfd);
            do
            {
                msg.Recv(skfd);
                if (msg.data == nullptr)
                {
                    dprintf(STDOUT_FILENO, "server closed connection\n");
                    return "";
                }
                if (msg.data->action() == ChatProto::data_data_mode_LOAD_USER_RELATION_DATA_OK)
                {
                    break;
                }
                else if (msg.data->action() == ChatProto::data_data_mode_false_)
                {
                    dprintf(STDOUT_FILENO, "init cache false \n");
                    return "";
                }
                else
                {
                    package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
                }
            } while (1);
            std::ofstream out("." + user_data->name() + "_" + std::to_string(user_data->id()));
            data->SerializeToOstream(&out);
            out.close();
            is_first = true;
        }

        i.close();
    }
    if (is_first)
        return_string = Timestamp::now();
    else
        return_string = data->timestamp();
    timestamp_of_last_job_done_time = data->timestamp();
    // 在这里进行初始化
    ChatProto::friend_relation fr;
    ChatProto::group_relation gr;
    for (auto &detail : data->details())
    {
        if (detail.Is<ChatProto::friend_relation>())
        {
            detail.UnpackTo(&fr);
            add_friend_relation(std::make_shared<ChatProto::friend_relation>(fr));
            fr.Clear();
        }
        else if (detail.Is<ChatProto::group_relation>())
        {
            detail.UnpackTo(&gr);
            std::sort(gr.mutable_member()->begin(), gr.mutable_member()->end(),
                      [](ChatProto::user_data_package const &q, ChatProto::user_data_package const &p) -> bool
                      {
                          return q.name() < p.name();
                      });
            std::sort(gr.mutable_manager()->begin(), gr.mutable_manager()->end(),
                      [](ChatProto::user_data_package const &q, ChatProto::user_data_package const &p) -> bool
                      {
                          return q.name() < p.name();
                      });
            add_group_relation(std::make_shared<ChatProto::group_relation>(gr));
            gr.Clear();
        }
    }
    // 下载 在线列表

    if (timestamp_of_last_job_done_time.empty())
    {
        timestamp_of_last_job_done_time = std::move(Timestamp::now());
        printf("\nget last do job time false;\n");
    }

    // try
    // {
    //     Message_Package<ChatProto::data> msg;
    //     msg.data->set_action(ChatProto::data_data_mode_GET_ONLINE_FRIENDS);
    //     msg.data->add_details()->PackFrom(*user_data);
    //     msg.data->set_timestamp(this->timestamp_of_last_job_done_time);
    //     msg.send(skfd);
    //     msg.Recv(skfd);
    //     ChatProto::user_data_package f;
    //     for (auto detail : msg.data->details())
    //     {
    //         detail.UnpackTo(&f);
    //         auto i = this->find_friend_from_id(f.id());
    //         if (i == nullptr)
    //         {
    //             // 在初始化的缓存中 没有找到目标用户
    //             dprintf(STDERR_FILENO, "cache init false , reInit..ing\n");
    //             // re init
    //             reinit(skfd);
    //             dprintf(STDERR_FILENO, "done\n");
    //             return;
    //         }
    //         else
    //         {
    //             i->is_online = true;
    //         }
    //     }
    //     timestamp_of_last_job_done_time = msg.data->timestamp();
    // }
    // catch (std::exception &e)
    // {
    //     dprintf(STDERR_FILENO, "cache init false , reInit..ing\n");
    //     reinit(skfd);
    //     dprintf(STDERR_FILENO, "done\n");
    // }
    return return_string.empty() ? std::move(Timestamp::now()) : std::move(return_string);
}
void Cache::get_online_data(int &skfd)
{
    try
    {
        Message_Package<ChatProto::data> msg;
        msg.data->set_action(ChatProto::data_data_mode_GET_ONLINE_FRIENDS);
        msg.data->add_details()->PackFrom(*user_data);
        msg.data->set_timestamp(this->timestamp_of_last_job_done_time);
        msg.send(skfd);
        ChatProto::user_data_package f;
        do
        {
            msg.Recv(skfd);
            if (msg.data == nullptr)
            {
                dprintf(STDOUT_FILENO, "server closed connection\n");
                return;
            }
            if (msg.data->action() == ChatProto::data_data_mode_GET_ONLINE_FRIENDS)
            {
                for (auto detail : msg.data->details())
                {
                    detail.UnpackTo(&f);
                    auto i = this->find_friend_from_id(f.id());
                    if (i == nullptr)
                    {
                        // 在初始化的缓存中 没有找到目标用户
                        dprintf(STDERR_FILENO, "cache init false , reInit..ing\n");
                        // re init
                        reinit(skfd);
                        dprintf(STDERR_FILENO, "done\n");
                        return;
                    }
                    else
                    {
                        i->is_online = true;
                    }
                }
                break;
            }
            else if (msg.data->action() == ChatProto::data_data_mode_false_)
            {
                ChatProto::error_package err;
                msg.data->details()[0].UnpackTo(&err);
                dprintf(STDOUT_FILENO, "get online friend data false : because of :%s", err.reason().c_str());
                return;
            }
            else
            {
                package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
            }
        } while (1);
    }
    catch (std::exception &e)
    {
        dprintf(STDERR_FILENO, "cache init false , reInit..ing\n");
        reinit(skfd);
        dprintf(STDERR_FILENO, "done\n");
    }
}
void Cache::show_friend_data(int fd)
{
    dprintf(fd, "friend data :\n");
    if (friend_map->empty())
    {
        dprintf(fd, "\t empty\n");
        return;
    }
    std::priority_queue<std::shared_ptr<friend_struct>, std::vector<std::shared_ptr<friend_struct>>,
                        decltype(&compare_friend)>
        friend_queue(&compare_friend);
    for (auto const &f : *friend_map)
    {
        friend_queue.push(f.second);
    }
    while (!friend_queue.empty())
    {
        auto i = friend_queue.top();
        dprintf(fd, "%s [%ld](%s)\n", i->friend_relation->follower().name().c_str(),
                i->friend_relation->follower().id(), i->is_online ? "online" : "offline");
        friend_queue.pop();
    }
}
void Cache::show_all_group_data(int fd, bool is_show_details)
{
    dprintf(fd, "group data :\n");
    if (group_map->empty())
    {
        dprintf(fd, "\t empty\n");
        return;
    }
    std::priority_queue<std::shared_ptr<ChatProto::group_relation>,
                        std::vector<std::shared_ptr<ChatProto::group_relation>>, decltype(&compare_group)>
        group_queue(&compare_group);
    for (auto const &f : *group_map)
    {
        group_queue.push(f.second);
    }
    while (!group_queue.empty())
    {
        auto i = group_queue.top();
        dprintf(fd, "%s [%s] \n%s", i->name().c_str(), i->id().c_str(), is_show_details ? "{\n" : "");
        if (is_show_details)
        {
            dprintf(fd, "\t%s [%ld][owner] \n", i->owner().name().c_str(), i->owner().id());
            for (auto const &ii : i->manager())
            {
                dprintf(fd, "\t\t%s [%ld][manager] \n", ii.name().c_str(), ii.id());
            }
            for (auto const &ii : i->member())
            {
                dprintf(fd, "\t\t%s [%ld][member] \n", ii.name().c_str(), ii.id());
            }
            dprintf(fd, "}\n");
        }
        group_queue.pop();
    }
}
bool Cache::show_one_group_data(std::string const &id, int fd)
{
    auto i = this->find_group_from_id(id);
    if (i == nullptr)
        return false;
    else
    {
        dprintf(fd, "%s [%s] \n%s", i->name().c_str(), i->id().c_str(), "{\n");
        dprintf(fd, "\t%s [%ld][owner] \n", i->owner().name().c_str(), i->owner().id());
        for (auto const &ii : i->manager())
        {
            dprintf(fd, "\t\t%s [%ld][manager] \n", ii.name().c_str(), ii.id());
        }
        for (auto const &ii : i->member())
        {
            dprintf(fd, "\t\t%s [%ld][member] \n", ii.name().c_str(), ii.id());
        }
        dprintf(fd, "}\n");
    }
    return true;
}
void Cache::reinit(int &skfd)
{
    Message_Package<ChatProto::data> msg;
    msg.data->add_details()->PackFrom(*user_data);
    msg.data->set_action(ChatProto::data_data_mode_LOAD_USER_RELATION_DATA);
    msg.send(skfd);
    do
    {
        msg.Recv(skfd);
        if (msg.data == nullptr)
        {
            dprintf(STDOUT_FILENO, "server closed connection\n");
 
            return;
        }
        if (msg.data->action() == ChatProto::data_data_mode_false_)
        {
            puts("reinit false");
            return;
        }
        else if (msg.data->action() == ChatProto::data_data_mode_LOAD_USER_RELATION_DATA_OK)
        {
            friend_map->clear();
            group_map->clear();
            break;
        }
        else
        {
            package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
        }
    } while (1);
    std::ofstream out("." + user_data->name() + "_" + std::to_string(user_data->id()));
    if (!out.is_open())
    {
        dprintf(STDERR_FILENO, "reinit false");
        return;
    }
    msg.data->SerializeToOstream(&out);
    out.close();
    ChatProto::friend_relation fr;
    ChatProto::group_relation gr;
    for (auto &detail : msg.data->details())
    {
        if (detail.Is<ChatProto::friend_relation>())
        {
            detail.UnpackTo(&fr);
            add_friend_relation(std::make_shared<ChatProto::friend_relation>(fr));
            fr.Clear();
        }
        else if (detail.Is<ChatProto::group_relation>())
        {
            detail.UnpackTo(&gr);
            std::sort(gr.mutable_member()->begin(), gr.mutable_member()->end(),
                      [](ChatProto::user_data_package const &q, ChatProto::user_data_package const &p) -> bool
                      {
                          return q.name() < p.name();
                      });
            std::sort(gr.mutable_manager()->begin(), gr.mutable_manager()->end(),
                      [](ChatProto::user_data_package const &q, ChatProto::user_data_package const &p) -> bool
                      {
                          return q.name() < p.name();
                      });
            add_group_relation(std::make_shared<ChatProto::group_relation>(gr));
            gr.Clear();
        }
    }

    // msg.data->Clear();
    // msg.data->set_action(ChatProto::data_data_mode_GET_ONLINE_FRIENDS);
    // msg.data->add_details()->PackFrom(*user_data);
    // msg.send(skfd);
    // msg.Recv(skfd);
    // ChatProto::user_data_package f;
    // for (auto detail : msg.data->details())
    // {
    //     detail.UnpackTo(&f);
    //     this->find_friend_from_id(f.id())->is_online = true;
    // }
    timestamp_of_last_job_done_time = msg.data->timestamp();
}
void Cache::update_timestamp(::Timestamp &&timestamp_of_last_job_done_time)
{
    if (timestamp_of_last_job_done_time < timestamp_of_last_job_done_time.data)
        this->timestamp_of_last_job_done_time.swap(timestamp_of_last_job_done_time.data);
}
void Cache::add_friend_relation(std::shared_ptr<ChatProto::friend_relation> fr)
{
    auto frr = std::make_shared<friend_struct>(fr);
    {
        std::lock_guard<std::mutex> lock(cacheLock);
        (*friend_map)[fr->chatid()] = frr;
    }
    flush_relation_to_file();
};
void Cache::delete_friend_relation(std::shared_ptr<ChatProto::friend_relation> fr)
{
    {
        std::lock_guard<std::mutex> lock(cacheLock);
        (*friend_map).erase(fr->chatid());
    }
    flush_relation_to_file();
};
void Cache::add_group_relation(std::shared_ptr<ChatProto::group_relation> gr)
{
    {
        std::lock_guard<std::mutex> lock(cacheLock);
        (*group_map)[gr->chatid()] = gr;
    }
    flush_relation_to_file();
};
void Cache::delete_group_relation(std::shared_ptr<ChatProto::group_relation> gr)
{
    {
        std::lock_guard<std::mutex> lock(cacheLock);
        (*group_map).erase(gr->chatid());
    }
    flush_relation_to_file();
};
void Cache::flush_relation_to_file()
{
    std::lock_guard<std::mutex> lock(cacheLock);
    ChatProto::data d;
    timestamp_of_last_job_done_time = std::move(Timestamp::now());
    d.set_timestamp(timestamp_of_last_job_done_time);
    for (const auto &i : (*group_map))
    {
        d.add_details()->PackFrom(*i.second);
    }
    for (const auto &i : (*friend_map))
    {
        d.add_details()->PackFrom(*(i.second->friend_relation));
    }
    std::ofstream out("." + user_data->name() + "_" + std::to_string(user_data->id()));
    d.SerializeToOstream(&out);
    out.close();
}
bool compare_friend(std::shared_ptr<friend_struct> const &i, std::shared_ptr<friend_struct> const &j)
{
    return *i < *j;
}
bool compare_group(std::shared_ptr<ChatProto::group_relation> const &i,
                   std::shared_ptr<ChatProto::group_relation> const &j)
{
    return i->name() < j->name();
}

void Cache::add_history_to_file(std::string const &chatid, ChatProto::msg_data_package const &msg)
{
    std::string file_path(std::move(history_dir_path + "." + calculateMD5(std::to_string(user_data->id()) + chatid)));
    auto fd(file::open(file_path.c_str(), O_RDWR | O_CREAT, 0777));
    char buffer[1024] = {}; // 1K
    snprintf(buffer, 1024, "[%s] %s \n%s", msg.timestamp().c_str(), msg.speaker().name().c_str(),
             msg.message().c_str());
    lseek(fd.fd_(), 0, SEEK_END);
    if (0 >= write(fd.fd_(), buffer, 1024))
    {
        perror("write");
    };
}

std::vector<std::string> Cache::get_history_from_file(std::string const &chatid, uint &num)
{
    std::vector<std::string> result;
    std::string file_path(std::move(history_dir_path + "." + calculateMD5(std::to_string(user_data->id()) + chatid)));
    auto fd(std::move(file::open(file_path.c_str(), O_RDWR | O_CREAT, 0777)));

    struct stat i;
    fstat(fd.fd_(), &i);
    if (i.st_size <= num * 1024)
    {
        num = i.st_size / 1024;
    }
    // num = 6;
    off_t p = lseek(fd.fd_(), i.st_size - (num * 1024), SEEK_SET);
    if (p == -1)
        perror("lseek");

    char buffer[1024] = {};
    for (auto i = 0; i < num; ++i)
    {
        auto rdbt = read(fd.fd_(), buffer, 1024);

        if (1024 != rdbt)
        {
            std::cerr << "Error reading from file " << file_path << " " << fd.fd_() << " " << rdbt << " "
                      << std::string(buffer, 1024) << strerror(errno) << std::endl;
        };
        result.emplace_back(buffer, 1024);
    }
    return std::move(result);
}
// std::vector<std::string> Cache::get_history_from_file(std::string const
// &chatid, uint &num)
// {
//     std::vector<std::string> result;
//     std::string file_path(history_dir_path + "." +
//     calculateMD5(std::to_string(user_data->id()) + chatid)); FILE *file =
//     fopen(file_path.c_str(), "rb"); if (file == nullptr)
//     {
//         file = fopen(file_path.c_str(), "wb");
//         if (file == nullptr)
//         {
//             perror("fopen");
//             return result;
//         }
//     }

//     fseek(file, 0, SEEK_END);
//     long file_size = ftell(file);
//     rewind(file);

//     if (file_size <= num * 1024)
//     {
//         num = file_size / 1024;
//     }

//     fseek(file, -(num * 1024), SEEK_END);

//     char buffer[1024] = {};
//     for (auto i = 0; i < num; ++i)
//     {
//         size_t bytes_read = fread(buffer, sizeof(char), 1024, file);

//         if (bytes_read != 1024)
//         {
//             std::cerr << "Error reading from file " << file_path << " " <<
//             bytes_read << " "
//                       << std::string(buffer, bytes_read) << strerror(errno)
//                       << std::endl;
//         }

//         result.emplace_back(buffer, bytes_read);
//     }

//     fclose(file);
//     return result;
// }

// std::vector<std::string> Cache::get_history_from_file(std::string const
// &chatid, uint &num)
// {
//     std::vector<std::string> result;
//     std::string file_path(std::move(history_dir_path + "." +
//     calculateMD5(std::to_string(user_data->id()) + chatid)));

//     std::fstream file(file_path,std::ios:: std::ios::binary | std::ios::in |
//     std::ios::out); if (!file.is_open())
//     {
//         std::cerr << "Error opening file: " << file_path << strerror(errno)
//         << std::endl; return result;
//     }

//     file.seekg(0, std::ios::end);
//     std::streampos file_size = file.tellg();
//     file.seekg(0, std::ios::beg);

//     if (file_size <= num * 1024)
//     {
//         num = file_size / 1024;
//     }

//     char buffer[1024] = {};
//     for (auto i = 0; i < num; ++i)
//     {
//         file.seekg(-(num * 1024), std::ios::end);
//         file.read(buffer, 1024);

//         if (!file)
//         {
//             std::cerr << "Error reading from file " << file_path <<
//             std::endl; break;
//         }

//         result.emplace_back(buffer, 1024);
//     }

//     file.close();
//     return result;
// }

std::string Cache::get_history_file_name(std::string const &chatid) const
{
    return std::move("." + calculateMD5(std::to_string(user_data->id()) + chatid));
}

#include <openssl/evp.h>

#include <iomanip>
std::string calculateMD5(const std::string &input)
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    EVP_MD_CTX *ctx;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
    EVP_DigestUpdate(ctx, input.c_str(), input.size());
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

history_show::history_show()
{
    getTerminalSize();
}
void history_show::show(int fd)
{
    getTerminalSize();
    for (auto &i : historys)
    {
        if (write(fd, i.c_str(), i.size()) <= 0 || write(fd, "\n", 1) <= 0)
        {
            perror("write");
        };
    }
}
void history_show::changed(std::vector<std::string> &&v)
{
    historys.swap(v);
}
void history_show::add_one(ChatProto::msg_data_package const &msg)
{
    char buffer[1024] = {}; // 1K
    snprintf(buffer, 1024, "[%s] %s \n%s", msg.timestamp().c_str(), msg.speaker().name().c_str(),
             msg.message().c_str());
    historys.emplace_back(buffer, strlen(buffer) + 1);
}
void history_show::getTerminalSize()
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        perror("ioctl");
        return;
    }
    this->rows = ws.ws_row;
    this->cols = ws.ws_col;
}
