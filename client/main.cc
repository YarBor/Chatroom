#include <arpa/inet.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/timerfd.h>

#include "Cache&Log.hh"
#include "command.hh"
#include "notification.hh"

using dd = ChatProto::data_data_mode;
using ee = ChatProto::error_package_mode;

std::weak_ptr<ChatProto::friend_relation> chat_fr;
std::weak_ptr<ChatProto::group_relation> chat_gr;

std::deque<std::shared_ptr<ChatProto::data>> package_queue;

extern std::string LISTEN_IP;
extern std::string LISTEN_PORT;
bool load_server(int &fd);
bool link(int &fd);
std::shared_ptr<ChatProto::chatHistory_data_package> send_message_to_friend(
    int &skfd, std::shared_ptr<ChatProto::friend_relation> fr, inputPraser &input_praser);
std::shared_ptr<ChatProto::chatHistory_data_package> send_message_to_group(
    int &skfd, std::shared_ptr<ChatProto::group_relation> gr, inputPraser &input_praser);

bool prase_his_htfy(Cache &cache, notifications &notifications_mode, std::shared_ptr<ChatProto::data> data,
                    history_show &hss, int &skfd);
bool deal_notices(Cache &cache, notifications &notifications_mode, std::shared_ptr<ChatProto::data> d, int &skfd);

std::shared_ptr<ChatProto::data> downLoad_his_htfy(int &skfd);

void displayMenu()
{
    std::cout << "Welcome to the Menu!" << std::endl;
    std::cout << "1. LogIn" << std::endl;
    std::cout << "2. SignIn" << std::endl;
    std::cout << "3. Exit" << std::endl;
}
std::string recv_prase_return_package_with_mode(int &skfd, dd mode)
{
    Message_Package<ChatProto::data> d;
    do
    {
        d.Recv(skfd);
        if (d.data == nullptr)
            return "server close connection";
        else if (d.data->action() == ChatProto::data_data_mode::data_data_mode_false_)
        {
            ChatProto::error_package err;
            d.data->details()[0].UnpackTo(&err);
            return std::move(err.reason());
        }
        else if (d.data->action() == mode)
            return "";
        else
        {
            package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(d.data);
        }
    } while (1);
    return "";
}
std::shared_ptr<ChatProto::user_data_package> user_data = nullptr;
#include <string>

std::string trimString(const std::string &str)
{
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == std::string::npos || end == std::string::npos)
    {
        return "";
    }
    return str.substr(start, end - start + 1);
}

bool is_alive = false;
void init();
std::shared_ptr<ChatProto::user_data_package> get_user_data(int fd)
{
    std::string choice, user_name_id, user_password, user_email;
    auto user = std::make_shared<ChatProto::user_data_package>();
    do
    {
        displayMenu();
        std::cout << "Enter your choice: ";
        // std::cin >> choice;
        getline(std::cin, choice);
        if (choice == "1")
        {
            dprintf(fd, "Enter your ID: \n");
            getline(std::cin, user_name_id);
            dprintf(fd, "Enter your password: \n");
            getline(std::cin, user_password);
            try
            {
                user->set_id(std::stol(trimString(user_name_id)));
            }
            catch (std::exception &i)
            {
                dprintf(fd, "input number please");
                continue;
            }
            user->set_password(user_password);
            user->set_status(ChatProto::user_data_package_LOAD_SERVER_LOG_IN);
            break;
        }
        else if (choice == "2")
        {
            dprintf(fd, "Enter your name: \n");
            getline(std::cin, user_name_id);
            dprintf(fd, "Enter your password: \n");
            getline(std::cin, user_password);
            dprintf(fd, "Enter your password again: \n");
            std::string user_password1;
            getline(std::cin, user_password1);
            // getline(std::cin, user_password);
            if (user_password1 != user_password)
            {
                dprintf(fd, "Passwords entered twice are different\n");
            }
            dprintf(fd, "Enter your email: \n");
            getline(std::cin, user_email);
            user->set_name(trimString(user_name_id));
            user->set_email(trimString(user_email));
            user->set_password(user_password);
            user->set_status(ChatProto::user_data_package_LOAD_SERVER_SIGN_UP);
            break;
        }
        else if (choice == "3")
        {
            exit(0);
        }
        else
        {
            dprintf(fd, "Please enter only one number To choose");
        }
    } while (1);
    return user;
}

int connect_to_server()
{
    int skfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress_;

    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_addr.s_addr = inet_addr(LISTEN_IP.c_str());
    serverAddress_.sin_port = htons(std::stoi(LISTEN_PORT));
    if (-1 == connect(skfd, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)))
    {
        perror("connect");
        skfd = -1;
    }
    else
        is_alive = true;
    return skfd;
}

void print_line(int fd)
{
    std::string i(history_show::get_T_wide(), '-');
    write(fd, i.c_str(), i.size()) && write(fd, "\n", 1);
}

int main()
{
    init();
    int skfd = -1;
    do
    {
        user_data = get_user_data(STDOUT_FILENO);
        skfd = connect_to_server();
        if (skfd == -1)
        {
            printf("link to server False\n");
        }
        print_line(STDOUT_FILENO);
    } while (!load_server(skfd) && (close(skfd) || (skfd = -1)));

    ChatProto::user_data_package realuserdata(*user_data);

    user_data->clear_password();
    user_data->clear_status();

    notifications notification_mode;
    Cache cache;
    std::string &&timestamp_last_time = cache.init(skfd);

    inputPraser input_praser;
    history_show hss;

    Message_Package<ChatProto::data> msg;
    /*UPDATE_DATA*/
    msg.data->set_action(dd::data_data_mode_UPDATE_DATA);
    msg.data->add_details()->PackFrom(*user_data);
    {
        using namespace std;
        cout << "last load time is :> " << cache.Timestamp() << endl
             << endl;
    }
    msg.data->set_timestamp(timestamp_last_time);

    msg.send(skfd);
    msg.Recv(skfd);
    if (msg.data == nullptr)
    {
        dprintf(STDOUT_FILENO, "server closed connection");
        return 0;
    }
    else if (msg.data->action() == dd::data_data_mode_UPDATE_DATA_OK)
        prase_his_htfy(cache, notification_mode, msg.data, hss, skfd);
    else
    {
        ChatProto::error_package error;
        msg.data->details()[0].UnpackTo(&error);
        dprintf(STDOUT_FILENO, "\n\nget notification and update history failed Because of [%s]\n\n",
                error.reason().c_str());
    }

    cache.get_online_data(skfd);
    // int tmfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    // FD_SET(tmfd, &fds);
    // fd_set fds_copy = fds;
    itimerspec timer_set;
    memset(&timer_set, 0, sizeof(timer_set));
    timer_set.it_value.tv_sec = 120; // 两分钟
    // timerfd_settime(tmfd, 0, &timer_set, nullptr);

    print_line(STDOUT_FILENO);
    std::cout << user_data->DebugString() << std::endl;
    printf("enter [: help] to get help\n");
    print_line(STDOUT_FILENO);

    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        if (skfd > 0)
            FD_SET(skfd, &fds);
        int max_fd = std::max(skfd, STDIN_FILENO);

        int rtnum = select(max_fd + 1, &fds, nullptr, nullptr, nullptr);

        if (rtnum == -1)
        {
            perror("select");
            break;
        }
        else
        {
            if (FD_ISSET(STDIN_FILENO, &fds))
            {
                print_line(STDOUT_FILENO);

                if (!is_alive)
                {
                    link(skfd);
                    // FD_SET(skfd, &fds_copy);
                    std::shared_ptr<ChatProto::data> data = downLoad_his_htfy(skfd);
                    if (data->action() == dd::data_data_mode_false_)
                    {
                        puts("pu data failed");
                    }
                    prase_his_htfy(cache, notification_mode, data, hss, skfd);
                }
                input_praser.Get_input(STDIN_FILENO);

                if (input_praser.is_command())
                {
                    if (false == do_command(STDOUT_FILENO, skfd, std::move(input_praser.Commands()), &cache, &hss,
                                            &notification_mode))
                    {
                        dprintf(STDOUT_FILENO, "\ninput [: help] to get help\n");
                    }
                }
                else
                {
                    auto const &input = input_praser.input();
                    auto fr = chat_fr.lock();
                    auto gr = chat_gr.lock();
                    if (input.length() > 450)
                    {
                        dprintf(STDOUT_FILENO, "The Chat-data is Too long to load/send,\n The limit is 450 bytes");
                    }
                    else if (fr)
                    {
                        if (fr->status() < 0)
                        {
                            dprintf(STDOUT_FILENO, "send msg false, Reason: [u has been locked]\n");
                        }
                        else
                        {
                            auto dp = send_message_to_friend(skfd, fr, input_praser);
                            if (dp != nullptr)
                            {
                                auto return_msg = recv_prase_return_package_with_mode(skfd, dd::data_data_mode_CHAT_FRIEND_OK);
                                if (!return_msg.empty())
                                {
                                    dprintf(STDOUT_FILENO, "send msg false Reason: [%s]\n", return_msg.c_str());
                                }
                                else
                                {
                                    for (auto &msgs : dp->messages())
                                    {
                                        hss.add_one(msgs);
                                        cache.add_history_to_file(dp->chatid(), msgs);
                                    }
                                    hss.show(STDOUT_FILENO);
                                }
                            }
                            else
                            {
                                dprintf(STDOUT_FILENO, "send msg false ");
                            }
                        }
                    }
                    else if (gr)
                    {
                        auto dp = send_message_to_group(skfd, gr, input_praser);
                        if (dp != nullptr)
                        {
                            auto return_msg = recv_prase_return_package_with_mode(skfd, dd::data_data_mode_CHAT_GROUP_OK);
                            if (!return_msg.empty())
                            {
                                dprintf(STDOUT_FILENO, "send msg false Reason: [%s]\n", return_msg.c_str());
                            }
                            else
                            {
                                for (auto &msgs : dp->messages())
                                {
                                    hss.add_one(msgs);
                                    cache.add_history_to_file(dp->chatid(), msgs);
                                }
                                hss.show(STDOUT_FILENO);
                            }
                        }
                        else
                        {
                            dprintf(STDOUT_FILENO, "send msg false ");
                        }
                    }
                    else
                        goto wrong_input;
                }
                // timerfd_settime(tmfd, 0, &timer_set, nullptr);
                // cache.flush_relation_to_file
                print_line(STDOUT_FILENO);
            }
            if (!package_queue.empty())
            {
                for (auto &package : package_queue)
                {
                    prase_his_htfy(cache, notification_mode, package, hss, skfd);
                    // timerfd_settime(tmfd, 0, &timer_set, nullptr);
                    print_line(STDOUT_FILENO);
                }
                package_queue.clear();
            }
            if (skfd > 0 && (FD_ISSET(skfd, &fds) || !package_queue.empty()))
            {
                Message_Package<ChatProto::data> msg;
                auto rtmsg = msg.Recv(skfd);
                if (rtmsg == nullptr)
                {
                    dprintf(STDOUT_FILENO, "server close , link has broken \n");
                    continue;
                }
                prase_his_htfy(cache, notification_mode, rtmsg, hss, skfd);
                // timerfd_settime(tmfd, 0, &timer_set, nullptr);
                print_line(STDOUT_FILENO);
            }
            // if (FD_ISSET(tmfd, &fds)) {
            // FD_CLR(skfd, &fds_copy);
            // close(skfd);
            // skfd = -1;
            // is_alive = false;
            // }
            cache.update_timestamp(std::move(Timestamp::now()));
            cache.flush_relation_to_file();
            continue;
        wrong_input:
            dprintf(STDOUT_FILENO, "WRONG INPUT \nPlease enter a command or send a message in the "
                                   "chat environment\n");
            print_line(STDOUT_FILENO);
        }
    }
}
bool link(int &fd)
{
    // close(fd);
    if (fd != -1)
        printf("you have logged into the server\n");
    fd = connect_to_server();
    if (fd == -1)
        return false;
    return load_server(fd);
}

bool load_server(int &fd)
{
    if (fd == -1)
        return false;
    Message_Package<ChatProto::data> msg;
    msg.data->add_details()->PackFrom(*user_data);
    msg.data->set_action(ChatProto::data_data_mode_LOAD_SERVER);
    msg.send(fd);
    Message_Package<ChatProto::data> rmsg;
    // auto i = recv_prase_return_package_with_mode(fd);
    rmsg.Recv(fd);
    if (rmsg.data == nullptr)
    {
        dprintf(STDOUT_FILENO, "server closed connection\n");
        close(fd);
        fd = -1;
        return false;
    }
    else if (rmsg.data->action() == dd::data_data_mode_false_)
    {
        ChatProto::error_package err;
        rmsg.data->details()[0].UnpackTo(&err);
        printf("Loading server false Because [%s]\n", err.reason().c_str());
        return false;
    }
    else
    {
        rmsg.data->details()[0].UnpackTo(user_data.get());
    }
    return true;
}

std::shared_ptr<ChatProto::data> downLoad_his_htfy(int &skfd)
{
    Message_Package<ChatProto::data> msg;
    ChatProto::data &result = *(msg.data);
    result.set_action(ChatProto::data_data_mode_UPDATE_DATA);
    result.set_timestamp(Timestamp::now());
    result.add_details()->PackFrom(*user_data);

    msg.send(skfd);
    msg.data->Clear();
    do
    {
        msg.Recv(skfd);
        if (msg.data == nullptr)
        {
            dprintf(STDOUT_FILENO, "server closed connection");
            close(skfd);
            skfd = -1;
            return nullptr;
        }
        else if (msg.data->action() == dd::data_data_mode_UPDATE_DATA_OK)
        {
            return msg.data;
        }
        else
        {
            package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
        }

    } while (1);
    return msg.data;
}

bool prase_his_htfy(Cache &cache, notifications &notifications_mode, std::shared_ptr<ChatProto::data> data,
                    history_show &hss, int &skfd)
{
    // std::cout << data->DebugString() << std::endl;

    if (data->action() == dd::data_data_mode_UPDATE_DATA_OK) // 是 update
    {
        for (auto &a : data->details())
        {
            if (a.Is<ChatProto::chatHistory_data_package>()) // 历史聊天
            {
                ChatProto::chatHistory_data_package cd;
                a.UnpackTo(&cd);
                for (auto &b : cd.messages())
                {
                    cache.add_history_to_file(cd.chatid(), b);
                }
            }
            else if (a.Is<ChatProto::data>()) // 通知
            {
                std::shared_ptr<ChatProto::data> d = std::make_shared<ChatProto::data>();
                a.UnpackTo(d.get());

                deal_notices(cache, notifications_mode, d, skfd);
                notifications_mode.add_notification(d);
            }
        }
        cache.flush_relation_to_file();
    }
    else if (data->action() == dd::data_data_mode_CHAT_FRIEND || data->action() == dd::data_data_mode_CHAT_GROUP)
    {
        ChatProto::chatHistory_data_package cd;
        for (auto &i : data->details())
        {
            bool is_show = false;
            i.UnpackTo(&cd);
            for (auto &b : cd.messages())
            {
                cache.add_history_to_file(cd.chatid(), b);
                auto cf = chat_fr.lock();
                auto cg = chat_gr.lock();
                if (cf && cf->chatid() == cd.chatid())
                {
                    hss.add_one(b);
                    hss.show(STDOUT_FILENO);
                    is_show = true;
                }
                else if (cg && cg->chatid() == cd.chatid())
                {
                    hss.add_one(b);
                    hss.show(STDOUT_FILENO);
                    is_show = true;
                }
                else
                {
                    if (dd::data_data_mode_CHAT_FRIEND == data->action())
                    {
                        auto i = cache.find_friend(cd.chatid());
                        if (i == nullptr)
                        {
                            dprintf(STDERR_FILENO, "something is wrong ; Need ReInit / update ");
                        }
                        else
                        {
                            dprintf(STDOUT_FILENO, "friend %s[%ld] recv one NEW message \n",
                                    i->friend_relation->follower().name().c_str(), i->friend_relation->follower().id());
                        }
                    }
                    else if (dd::data_data_mode_CHAT_GROUP == data->action())
                    {
                        auto i = cache.find_group(cd.chatid());
                        if (i == nullptr)
                        {
                            dprintf(STDERR_FILENO, "something is wrong ; Need ReInit / update ");
                        }
                        else
                        {
                            dprintf(STDOUT_FILENO, "group %s[%s] recv one NEW message \n", i->name().c_str(),
                                    i->id().c_str());
                        }
                    }
                }
            }
        }
    }
    // else if (data->action() == dd::data_data_mode_false_){
    //     ChatProto::error_package error;
    //     data->details()[0].UnpackTo(&error);

    // }
    else
    {
        if (deal_notices(cache, notifications_mode, data, skfd))
            // notification nd ();
            notifications_mode.add_notification_and_show(STDOUT_FILENO, data);
        else
            notifications_mode.show_notify(STDOUT_FILENO, data);
        cache.update_timestamp(data->timestamp());
    }
    cache.update_timestamp(Timestamp::now());
    return true;
}

bool deal_notices(Cache &cache, notifications &notifications_mode, std::shared_ptr<ChatProto::data> d, int &skfd)
{
    switch (d->action())
    {
    case dd::data_data_mode_REQUEST_MAKE_FRIEND:
    {
        std::shared_ptr<ChatProto::friend_relation> fr = std::make_shared<ChatProto::friend_relation>();
        d->details()[0].UnpackTo(fr.get());
        break;
    }
    case dd::data_data_mode_ANSWER_MAKE_FRIEND:
    {
        std::shared_ptr<ChatProto::friend_relation> fr = std::make_shared<ChatProto::friend_relation>();
        d->details()[0].UnpackTo(fr.get());
        if (fr->status() == 1)
        {
            if (cache.find_friend(fr->chatid()) == nullptr)
                cache.add_friend_relation(fr);
            cache.get_online_data(skfd);
        }
        //   cache.flush_relation_to_file();

        break;
    }
    case dd::data_data_mode_DELETE_FRIEND:
    {
        std::shared_ptr<ChatProto::friend_relation> fr = std::make_shared<ChatProto::friend_relation>();
        d->details()[0].UnpackTo(fr.get());
        if (fr->status() == 1)
        {
            if (cache.find_friend(fr->chatid()) == nullptr)
                cache.delete_friend_relation(fr);
        }
        //   cache.flush_relation_to_file();
        break;
    }
    case dd::data_data_mode_MANAGE_GROUP:
    {
        std::shared_ptr<ChatProto::group_relation> gr = std::make_shared<ChatProto::group_relation>();
        d->details()[0].UnpackTo(gr.get());
        auto ptr = cache.find_group(gr->chatid());
        if (ptr == nullptr)
        {
            if (gr->status() == 7)
            {
                gr->add_member()->CopyFrom(*user_data);
                cache.add_group_relation(gr);
            }
            else
                puts("cache error");
        }
        else
        {
            if (gr->status() == 5)
            {
                for (int i = 0; i < ptr->member().size(); i++)
                {
                    if (ptr->member()[i].id() == gr->follower().id())
                    {
                        ptr->mutable_member()->DeleteSubrange(i, 1);
                        break;
                    }
                }
                ptr->add_manager()->CopyFrom(gr->follower());
                std::sort(ptr->mutable_manager()->begin(), ptr->mutable_manager()->end(),
                          [](const ChatProto::user_data_package &i, const ChatProto::user_data_package &p) -> bool
                          {
                              return i.name() < p.name();
                          });
                if (gr->follower().id() == user_data->id())
                {
                    ptr->set_status(2);
                }
            }
            else if (gr->status() == 6)
            {
                for (int i = 0; i < ptr->manager().size(); i++)
                {
                    if (ptr->manager()[i].id() == gr->follower().id())
                    {
                        ptr->mutable_manager()->DeleteSubrange(i, 1);
                        break;
                    }
                }
                ptr->add_member()->CopyFrom(gr->follower());
                std::sort(ptr->mutable_member()->begin(), ptr->mutable_member()->end(),
                          [](const ChatProto::user_data_package &i, const ChatProto::user_data_package &p) -> bool
                          {
                              return i.name() < p.name();
                          });
                if (gr->follower().id() == user_data->id())
                {
                    ptr->set_status(1);
                }
            }
            else if (gr->status() == 7)
            {
                // 收到 成员进入小组中时 查看自己是否是该组的管理或群主
                // 遍历
                // 是的话 将代办改成 已办
                auto asdf = cache.find_group(gr->chatid());
                if (asdf == nullptr)
                {
                    dprintf(STDOUT_FILENO, "cache needs to be reinit or update ");
                    return false;
                }
                if (asdf->status() > 1)
                {
                    for (auto &i : notifications_mode.get())
                    {
                        if (i.mode == dd::data_data_mode_REQUEST_JOIN_GROUP)
                        {
                            ChatProto::group_relation gr1;
                            i.notify->details()[0].UnpackTo(&gr1);
                            gr1.id() == gr->id();
                            i.is_done = true;
                        }
                    }
                    notifications_mode.update();
                }
                ptr->add_member()->CopyFrom(gr->follower());
                // ptr->add_member()->CopyFrom(gr->follower());
                std::sort(ptr->mutable_member()->begin(), ptr->mutable_member()->end(),
                          [](const ChatProto::user_data_package &i, const ChatProto::user_data_package &p) -> bool
                          {
                              return i.name() < p.name();
                          });
            }
            else if (gr->status() == 8)
            {
                if (gr->follower().id() == user_data->id())
                {
                    cache.delete_group_relation(ptr);
                }
                else
                {
                    for (int i = 0; i < ptr->manager_size(); i++)
                    {
                        if (ptr->manager()[i].id() == gr->follower().id())
                        {
                            ptr->mutable_manager()->DeleteSubrange(i, 1);
                            goto done;
                        }
                    }
                    for (int i = 0; i < ptr->member_size(); i++)
                    {
                        if (ptr->member()[i].id() == gr->follower().id())
                        {
                            ptr->mutable_member()->DeleteSubrange(i, 1);
                            goto done;
                        }
                    }
                done:;
                }
            }
            else
            {
                puts("ub");
            }
            d->clear_details();
            gr->clear_manager();
            gr->clear_member();
            d->add_details()->PackFrom(*gr);
        }

        cache.flush_relation_to_file();
        break;
    }
    case dd::data_data_mode_REQUEST_JOIN_GROUP:
    {
        ;
        break;
    }
    case dd::data_data_mode_IGNORE_FRIEND:
    {
        std::shared_ptr<ChatProto::friend_relation> fr = std::make_shared<ChatProto::friend_relation>();
        d->details()[0].UnpackTo(fr.get());

        auto ptr = cache.find_friend(fr->chatid());
        if (ptr == nullptr)
            puts("some data lost");
        else
        {
            ptr->friend_relation->set_status(-1);
        }
        break;
    }
    case dd::data_data_mode_UNIGNORE_FRIEND:
    {
        std::shared_ptr<ChatProto::friend_relation> fr = std::make_shared<ChatProto::friend_relation>();
        d->details()[0].UnpackTo(fr.get());

        auto ptr = cache.find_friend(fr->chatid());
        if (ptr == nullptr)
        {
            puts("some data lost .. reiniting");
            cache.reinit(skfd);
        }
        else
        {
            ptr->friend_relation->set_status(1);
        }
        break;
    }
    case dd::data_data_mode_FRIEND_LOAD_SERVER:
    {
        std::shared_ptr<ChatProto::user_data_package> u = std::make_shared<ChatProto::user_data_package>();
        d->details()[0].UnpackTo(u.get());
        auto ptr = cache.find_friend_from_id(u->id());
        if (ptr == nullptr)
        {
            puts("ERROR: Inconsistency between local and server data\n");
        }
        else
        {
            ptr->is_online = true;
        }
        return false;
    }
    case dd::data_data_mode_FRIEND_LOG_OUT:
    {
        std::shared_ptr<ChatProto::user_data_package> u = std::make_shared<ChatProto::user_data_package>();
        d->details()[0].UnpackTo(u.get());
        auto ptr = cache.find_friend_from_id(u->id());
        if (ptr == nullptr)
        {
            puts("ERROR: Inconsistency between local and server data");
        }
        else
        {
            ptr->is_online = false;
        }
        return false;
    }
    }
    return true;
}

std::shared_ptr<ChatProto::chatHistory_data_package> send_message_to_friend(
    int &skfd, std::shared_ptr<ChatProto::friend_relation> fr, inputPraser &input_praser)
{
    auto msg_data = std::make_shared<ChatProto::data>();
    msg_data->set_action(dd::data_data_mode_CHAT_FRIEND);
    auto dp = std::make_shared<ChatProto::chatHistory_data_package>();
    auto data = dp->add_messages();
    data->mutable_speaker()->CopyFrom(*user_data);
    data->set_message(input_praser.input());
    data->set_timestamp(Timestamp::now());
    dp->set_chatid(fr->chatid());
    dp->set_recevier_id(fr->follower().id());
    msg_data->add_details()->PackFrom(*dp);
    Message_Package<ChatProto::data> d(msg_data);
    if (d.send(skfd))
        return dp;
    else
        return nullptr;
}
std::shared_ptr<ChatProto::chatHistory_data_package> send_message_to_group(
    int &skfd, std::shared_ptr<ChatProto::group_relation> gr, inputPraser &input_praser)
{
    auto msg_data = std::make_shared<ChatProto::data>();
    msg_data->set_action(dd::data_data_mode_CHAT_GROUP);
    auto dp = std::make_shared<ChatProto::chatHistory_data_package>();
    auto data = dp->add_messages();
    data->mutable_speaker()->CopyFrom(*user_data);
    data->set_message(input_praser.input());
    data->set_timestamp(Timestamp::now());
    dp->set_chatid(gr->chatid());
    msg_data->add_details()->PackFrom(*dp);
    Message_Package<ChatProto::data> d(msg_data);
    if (d.send(skfd))
        return dp;
    else
        return nullptr;
}

// auto i = 0X44f47d6a;
