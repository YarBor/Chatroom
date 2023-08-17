#include "command.hh"

#include <cstring>
bool operator==(const std::string &lhs, const char *rhs)
{
    return strcmp(lhs.c_str(), rhs) == 0;
}
extern std::deque<std::shared_ptr<ChatProto::data>> package_queue;

std::string recv_prase_return_package_command(int &skfd, ChatProto::data_data_mode mode)
{
    Message_Package<ChatProto::data> d;
    do
    {
        d.Recv(skfd);
        if (d.data == nullptr)
        {
            dprintf(STDOUT_FILENO, "server closed connection\n`");
            return "server closed connection\n`";
        }
        else if (d.data->action() == ChatProto::data_data_mode::data_data_mode_false_)
        {
            ChatProto::error_package err;
            d.data->details()[0].UnpackTo(&err);
            return std::move(err.reason());
        }
        else if (d.data->action() == mode)
            break;
        else
        {
            package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(d.data);
        }
    } while (1);
    return "";
}
std::string calculateFileMD5(int fd);
inputPraser::inputPraser()
{
    buffer = (char *)malloc(buffer_size);
};
inputPraser::~inputPraser()
{
    free(buffer);
};
//    inputPraser &inputPraser::operator=(inputPraser const &) = delete;
void inputPraser::Get_input(int input_fd)
{
    input_.clear();
    int flag = fcntl(input_fd, F_GETFL, 0);
    fcntl(input_fd, F_SETFL, flag | O_NONBLOCK);
    ssize_t bytesRead;
    while (1)
    {
        bytesRead = read(input_fd, buffer, buffer_size);
        if (bytesRead == -1 || bytesRead == 0)
        {
            break;
        }
        input_.append(buffer, bytesRead);
    }
    fcntl(input_fd, F_SETFL, flag);
    if (input_[0] == ':' && input_[1] == ' ')
    {
        is_commands = true;
        commands = std::move(this->splitString(input_));
    }
    else
    {
        is_commands = false;
    }
}

uint lines = 100;
const std::vector<std::string> &inputPraser::Commands() const
{
    return commands;
}
bool inputPraser::is_command() const
{
    return is_commands;
}
const std::string &inputPraser::input() const
{
    return input_;
}

std::vector<std::string> inputPraser::splitString(std::string &input_)
{
    static auto trim = [](std::string &&s) -> std::string &&
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                             { return !std::isspace(ch); })
                    .base(),
                s.end());
        return std::move(s);
    };

    input_.erase(
        std::find_if(input_.rbegin(), input_.rend(), [](unsigned char ch)
                     { return !std::isspace(ch); })
            .base(),
        input_.end());

    if (input_.back() == '\n')
        input_.back() = '\0';

    std::vector<std::string> result;
    std::istringstream iss(input_);
    std::string token;
    while (iss >> token)
    {
        auto &&i = trim(std::move(token));
        if (!i.empty())
        {
            result.emplace_back(std::move(i));
        }
    }
    return std::move(result);
}

bool do_command(int fd, int &skfd, const std::vector<std::string> &commands, Cache *cache, history_show *hss,
                notifications *notifications_mode)
{
    /* command[0] == ':' */
    if (commands.size() < 2)
        return false;
    std::string const &c = commands[1];
    if (c == "help" && commands.size() == 2)
    {
        std::string i(R"(
    Commands:
    - [: chat done] 
                                    # end chat
                                                                     
    - [: chat {--group/-g} {ID}] 
                                    # Start a chat with the specified group and set chat environment
                                                                     
    - [: chat {--friend/-f} {ID}] 
                                    # Start a chat with a specified friend and set chat environment
                                    
    - [: request {--makefriend/-mf} {friend-ID}] 
                                    # request to make friend
                                                                     
    - [: request {--joingroup/-jg} {group-ID}] 
                                    # request to join group
    
    - [: request {--makegroup/-mg} {group-name}] 
                                    # request to make group

    - [: show] 
                                    # show friends-groups

    - [: show {--more/-m/more}] 
                                    # show more chathistory If in chat environment
                                                                     
    - [: show {--all/-a}] 
                                    # Show friends-groups (including group members)
                                                                     
    - [: show {--friend/-f}] 
                                    # show friends
                                                                     
    - [: show {--group/-g} {group-ID}] 
                                    # Display the specified group members
                                                                     
    - [: showhistory {--more/-m}] 
                                    # show more history
                                                                     
    - [: manage {--friend/-f} {friend-ID} {--ignore/-i OR --remind/-r OR --accept/-a OR --delete/-d}] 
                                    # manage friends (ignore, remind, accept, delete)
                                                                     
    - [: manage {--group/-g} {group-ID} {--accept/-a OR --upgrade/-ug OR --downgrade/dg OR --delete/-d} {target_member-ID} ] 
                                    # Manage group members (accept, promote, demote, delete)
                                                                     
    - [: {q/quit}] 
                                    # Quit the program
                                                                     
    - [: recvfile {friend-ID} {file-name} {load/path}] 
                                    # Receive files sent by friends
                                                                     
    - [: sendfile {friend-ID} {path/to/file}] 
                                    # send file to friend

    - [: relink] 
                                    # relink to server

    - [: reinit] 
                                    # reinit self relation
                                                                     
        )");
        dprintf(fd, "%s\n", i.c_str());
        return true;
    }
    else if (c == "relink" && commands.size() == 2)
    {
        if (link(skfd))
        {
            dprintf(fd, "linked seccessfully\n");
        }
        else
        {
            dprintf(fd, "linked false\n");
        }
    }
    else if (c == "request")

    {
        if (commands.size() != 4)
            return false;
        else if (commands[2] == "--makefriend" || commands[2] == "-mf")
        {
            ChatProto::friend_relation fr;
            fr.mutable_master()->CopyFrom(*user_data);
            if (commands[3].find_first_of("1234567890") != commands[3].npos)
                fr.mutable_follower()->set_id(std::stol(commands[3]));
            else
            {
                dprintf(fd, "expect input the rigth ID-num\n");
                return false;
            }
            msg_send(skfd, fr, dd::data_data_mode_REQUEST_MAKE_FRIEND);
            auto rtstr = recv_prase_return_package_command(skfd, dd::data_data_mode_REQUEST_MAKE_FRIEND_OK);
            if (rtstr.empty())
            {
                dprintf(fd, "request successed\n");
                return true;
            }
            else
            {
                dprintf(fd, "request false Reason:[%s]\n", rtstr.c_str());
                return false;
            }
        }
        else if (commands[2] == "--joingroup" || commands[2] == "-jg")
        {
            ChatProto::group_relation gr;
            gr.mutable_follower()->CopyFrom(*user_data);
            if (commands[3].find_first_of("1234567890") != commands[3].npos)
                gr.mutable_master()->set_id(std::stol(commands[3]));
            else
            {
                dprintf(fd, "expect input the rigth ID-num\n");
                return false;
            }
            msg_send(skfd, gr, dd::data_data_mode_REQUEST_JOIN_GROUP);
            auto rtstr = recv_prase_return_package_command(skfd, dd::data_data_mode_REQUEST_JOIN_GROUP_OK);
            if (rtstr.empty())
            {
                dprintf(fd, "request successed\n");
                return true;
            }
            else
            {
                dprintf(fd, "request false Reason:[%s]\n", rtstr.c_str());
                return false;
            }
        }
        else if (commands[2] == "--makegroup" || commands[2] == "-mg")
        {
            std::string const &grName = commands[3];
            std::shared_ptr<ChatProto::group_relation> gr = std::make_shared<ChatProto::group_relation>();
            gr->mutable_owner()->CopyFrom(*user_data);
            gr->set_name(grName);
            gr->set_status(3);
            gr->mutable_master()->set_name(grName);
            Message_Package<ChatProto::data> msg;
            msg.data->add_details()->PackFrom(*gr);
            msg.data->set_action(dd::data_data_mode_MAKE_GROUP);
            msg.send(skfd);
            do
            {
                msg.Recv(skfd);
                if (msg.data == nullptr)
                {
                    dprintf(STDOUT_FILENO, "server closed connection\n`");
                    return true;
                }
                else if (msg.data->action() == dd::data_data_mode_MAKE_GROUP_OK)
                {
                    msg.data->details()[0].UnpackTo(gr.get());
                    break;
                }
                else
                {
                    package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
                }
            } while (1);
            cache->add_group_relation(gr);
            dprintf(fd, "request successfully Group : %s[%s]\n", gr->name().c_str(), gr->id().c_str());
            return true;
        }
    }
    else if (c == "reinit" || c == "update")
    {
        dprintf(fd, " %s...ing\n", c.c_str());
        cache->reinit(skfd);
        dprintf(fd, " %s  done\n", c.c_str());
        return true;
    }
    else if (c == "chat")
    {
        try
        {
            if (commands.size() == 3 && commands[2] == "done")
            {
                chat_fr.reset();
                chat_gr.reset();
                hss->changed({});
                dprintf(fd, "chat finished \n");
                return true;
            }

            if (commands.size() != 4)
            {
                dprintf(fd, "please input the right ID to chat \n [: chat {--group/-g} "
                            "{ID}] OR [: chat {--friend/-f} {ID}]\n");
                return false;
            }
            else if (commands[2] == "--friend" || commands[2] == "-f")
            {
                auto fr = cache->find_friend_from_id(std::stol(commands[3]));
                if (fr == nullptr)
                {
                    dprintf(fd, "please input the right ID to chat \n [: chat {--friend/-f} "
                                "{ID}]\n");
                    return false;
                }
                chat_fr = fr->friend_relation;
                lines = 100;
                hss->changed(std::move(cache->get_history_from_file(chat_fr.lock()->chatid(), lines)));
                hss->show(fd);
                dprintf(fd, "OK , chat environment changed , show %ud lines history\n", lines);
                return true;
            }
            else if (commands[2] == "--group" || commands[2] == "-g")
            {
                auto gr = cache->find_group_from_id(commands[3]);
                if (gr == nullptr)
                {
                    dprintf(fd, "please input the right ID to chat \n [: chat {--group/-g} "
                                "{ID}]\n");
                    return false;
                }
                chat_gr = gr;
                lines = 100;
                hss->changed(std::move(cache->get_history_from_file(gr->chatid(), lines)));
                hss->show(fd);
                dprintf(fd, "OK , chat environment changed , show %ud lines history\n", lines);
                // dprintf(fd, "OK , chat environment changed\n");
                return true;
            }
            else
            {
                dprintf(fd, "please input the right ID to chat \n [: chat {--group/-g} "
                            "{ID}] OR [: chat {--friend/-f} {ID}]\n");
                return false;
            }
        }
        catch (const std::exception &e)
        {
            dprintf(fd, "please input the right ID to chat \n [: chat {--group/-g} {ID}] "
                        "OR [: chat {--friend/-f} {ID}]\n");
            return false;
        }
    }
    else if (c == "manage")
    {
        if (commands.size() != 5)
        {
            goto wrong_manage;
        }
        if (commands[2] == "--friend" || commands[2] == "-f")
        {
            if (commands.size() != 5)
            {
                goto wrong_manage;
            }
            auto fr = cache->find_friend_from_id(std::stol(commands[3]));
            if (fr == nullptr && !(commands[4] == "--accept" || commands[4] == "-a"))
            {
                goto wrong_manage;
            }
            else
            {
                // ChatProto::friend_relation fr;
                if (commands[4] == "--ignore" || commands[4] == "-i")
                {
                    msg_send<ChatProto::friend_relation>(skfd, *fr->friend_relation, dd::data_data_mode_IGNORE_FRIEND);
                    auto rt_string = recv_prase_return_package_command(skfd, dd::data_data_mode_IGNORE_FRIEND_OK);
                    if (!rt_string.empty())
                    {
                        dprintf(fd, "ignore false Because of: %s\n", rt_string.c_str());
                        return false;
                    }
                    // fr->friend_relation->set_status(0);
                    return true;
                }
                else if (commands[4] == "--remind" || commands[4] == "-r")
                {
                    if (fr->friend_relation->status() == -1)
                    {
                        dprintf(fd, "u has been blocked to %s[%ld]\n", fr->friend_relation->follower().name().c_str(), fr->friend_relation->follower().id());
                        return false;
                    }
                    msg_send<ChatProto::friend_relation>(skfd, *fr->friend_relation,
                                                         dd::data_data_mode_UNIGNORE_FRIEND);
                    auto rt_string = recv_prase_return_package_command(skfd, dd::data_data_mode_UNIGNORE_FRIEND_OK);
                    if (!rt_string.empty())
                    {
                        dprintf(fd, "ignore false Because of: %s\n", rt_string.c_str());
                        return false;
                    }
                    fr->friend_relation->set_status(1);
                    return true;
                }
                else if (commands[4] == "--accept" || commands[4] == "-a")
                {
                    std::shared_ptr<ChatProto::friend_relation> target_relation = nullptr;
                    ChatProto::friend_relation tmp_relation_data;
                    auto asdff = Cache::find_friend_from_id(commands[3]);
                    if (asdff)
                    {
                        dprintf(fd, "%s[%ld] has already ur friend", asdff->friend_relation->follower().name().c_str(),
                                asdff->friend_relation->follower().id());
                        return false;
                    }
                    for (auto &i : notifications_mode->get())
                    {
                        if (i.mode == dd::data_data_mode_REQUEST_MAKE_FRIEND)
                        {
                            i.notify->details()[0].UnpackTo(&tmp_relation_data);
                            if (std::to_string(tmp_relation_data.master().id()) == commands[3])
                            {
                                target_relation = std::make_shared<ChatProto::friend_relation>(tmp_relation_data);
                            }
                        }
                    }
                    if (target_relation == nullptr)
                    {
                        dprintf(fd, "[%s] not request to be your friend\n", commands[3].c_str());
                        return false;
                    }
                    else
                    {
                        target_relation->set_status(1);
                        target_relation->mutable_follower()->CopyFrom(*user_data);
                        if (!msg_send<ChatProto::friend_relation>(skfd, *target_relation,
                                                                  dd::data_data_mode_ANSWER_MAKE_FRIEND))
                        {
                            dprintf(fd, "massage send false to answer [%s]'s request \n", commands[3].c_str());
                            return false;
                        }
                        Message_Package<ChatProto::data> rtmsg;
                        do
                        {
                            auto rt_data = rtmsg.Recv(skfd);
                            if (rtmsg.data == nullptr)
                            {
                                dprintf(STDOUT_FILENO, "server closed connection\n`");

                                return true;
                            }
                            else if (rt_data->action() == dd::data_data_mode_false_)
                            {
                                ChatProto::error_package msg_error;
                                rt_data->details()[0].UnpackTo(&msg_error);
                                dprintf(fd, "accpet friend false Because of :%s\n", msg_error.reason().c_str());
                                return false;
                            }
                            else if (rtmsg.data->action() == dd::data_data_mode_ANSWER_MAKE_FRIEND_OK)
                            {
                                auto fr = std::make_shared<ChatProto::friend_relation>();
                                rt_data->details()[0].UnpackTo(fr.get());
                                fr->mutable_follower()->Swap(fr->mutable_master());
                                cache->add_friend_relation(fr);
                                cache->get_online_data(skfd);
                                cache->flush_relation_to_file();
                                break;
                            }
                            else
                            {
                                package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(rtmsg.data);
                            }
                        } while (1);
                        return true;
                    }
                }
                else if (commands[4] == "--delete" || commands[4] == "-d")
                {
                    auto fr = cache->find_friend_from_id(commands[3]);
                    if (fr == nullptr)
                    {
                        dprintf(fd, "Bad id number [%s]\n", commands[3].c_str());
                        return false;
                    }
                    else
                    {
                        if (!msg_send<ChatProto::friend_relation>(skfd, *(fr->friend_relation),
                                                                  dd::data_data_mode_DELETE_FRIEND))
                        {
                            dprintf(fd, "delete friend [%s] error , Send message err\n", commands[3].c_str());
                            return false;
                        }
                        auto rtstr = recv_prase_return_package_command(skfd, dd::data_data_mode_DELETE_FRIEND_OK);
                        if (!rtstr.empty())
                        {
                            dprintf(fd, "delete friend [%s] error ,Becase of %s\n", commands[3].c_str(), rtstr.c_str());
                            return false;
                        }
                        else
                        {
                            cache->delete_friend_relation(fr->friend_relation);
                            return true;
                        }
                    }
                }
                else
                {
                    goto wrong_manage;
                }
            }
        }
        else if (commands[2] == "--group" || commands[2] == "-g")
        {
            auto gr = cache->find_group_from_id(commands[3]);
            if (gr == nullptr || commands.size() != 6)
            {
                goto wrong_manage;
            }
            else
            {
                std::shared_ptr<ChatProto::user_data_package> target = nullptr;
                ChatProto::group_relation gr_data;
                if (commands[4] == "--accept" || commands[4] == "-a")
                {
                    if (gr->status() <= 1)
                    {
                        dprintf(fd, "u are not manager/owner \n");
                        return false;
                    }
                    ChatProto::group_relation g;
                    for (auto i : notifications_mode->get())
                    {
                        if (i.mode == dd::data_data_mode_REQUEST_JOIN_GROUP)
                        {
                            i.notify->details()[0].UnpackTo(&g);
                            if (std::to_string(g.follower().id()) == commands[5])
                            {
                                target = std::make_shared<ChatProto::user_data_package>(g.follower());
                                break;
                            }
                        }
                    }
                    if (target == nullptr)
                    {
                        dprintf(fd, "[%s] did not request join group\n", commands[5].c_str());
                        return false;
                    }
                    else
                    {
                        // gr_data.mutable_member()->CopyFrom(gr->member());
                        // gr_data.mutable_manager()->CopyFrom(gr->manager());
                        for (auto &i : gr->member())
                        {
                            if (i.id() == target->id())
                            {
                                dprintf(fd, "%s[%ld] has joined group\n", target->name().c_str(), target->id());
                                return false;
                            }
                        }
                        for (auto &i : gr->manager())
                        {
                            if (i.id() == target->id())
                            {
                                dprintf(fd, "%s[%ld] has joined group\n", target->name().c_str(), target->id());
                                return false;
                            }
                        }
                        gr_data.CopyFrom(*gr);
                        gr_data.set_status(7);
                    }
                }
                else if (commands[4] == "--delete" || commands[4] == "-d")
                {
                    if (commands[5] == std::to_string(user_data->id()))
                    {
                        target = std::make_shared<ChatProto::user_data_package>(*user_data);
                    }
                    else if (gr->status() <= 1)
                    {
                        dprintf(fd, "u are not manager/owner \n");
                        return false;
                    }
                    else
                        for (auto i : gr->member())
                        {
                            if (commands[5] == std::to_string(i.id()))
                            {
                                target = std::make_shared<ChatProto::user_data_package>(i);
                                break;
                            }
                        }
                    if (target == nullptr)
                    {
                        dprintf(fd, "target member not found \n");
                        return false;
                    }
                    else if (target->id() == gr->owner().id())
                    {
                        dprintf(fd, "Cannot delete self because U are the owner \n");
                        return false;
                    }
                    else
                    {
                        gr_data.set_status(8);
                    }
                }
                else if (commands[4] == "--upgrade" || commands[4] == "-ug")
                {
                    if (gr->status() <= 2)
                    {
                        dprintf(fd, "u are not owner \n");
                        return false;
                    }
                    for (auto i : gr->member())
                    {
                        if (commands[5] == std::to_string(i.id()))
                        {
                            target = std::make_shared<ChatProto::user_data_package>(i);
                            break;
                        }
                    }
                    if (target == nullptr)
                    {
                        dprintf(fd, "target member not found \n");
                        return false;
                    }
                    else
                    {
                        gr_data.set_status(5); // magic number // status mode to upgrade member
                    }
                }
                else if (commands[4] == "--downgrade" || commands[4] == "-dg")
                {
                    if (gr->status() <= 2)
                    {
                        dprintf(fd, "u are not owner \n");
                        return false;
                    }
                    for (auto i : gr->manager())
                    {
                        if (commands[5] == std::to_string(i.id()))
                        {
                            target = std::make_shared<ChatProto::user_data_package>(i);
                            break;
                        }
                    }
                    if (target == nullptr)
                    {
                        dprintf(fd, "target manager not found \n");
                        return false;
                    }
                    else
                    {
                        gr_data.set_status(6); // magic number // status mode to upgrade member
                    }
                }
                gr_data.mutable_master()->set_id(std::stol(gr->id()));
                gr_data.mutable_master()->set_name(gr->name());
                gr_data.set_chatid(gr->chatid());
                gr_data.mutable_follower()->CopyFrom(*target);
                if (!msg_send<decltype(gr_data)>(skfd, gr_data, dd::data_data_mode_MANAGE_GROUP))
                {
                    dprintf(fd, "send message to server false\n");
                    return false;
                }
                auto returned = recv_prase_return_package_command(skfd, dd::data_data_mode_MANAGE_GROUP_OK);
                if (!returned.empty())
                {
                    dprintf(fd, "send message false [%s]\n", returned.c_str());
                    return false;
                }

                else
                { // 发送成功 更新自己
                    switch (gr_data.status())
                    {
                    case 5:
                        for (auto i = 0; i < gr->member().size(); ++i)
                        {
                            if (gr->member()[i].id() == target->id())
                            {
                                gr->mutable_member()->DeleteSubrange(i, 1);
                                break;
                            }
                        }
                        gr->add_manager()->CopyFrom(*target);
                        break;
                    case 6:
                        for (auto i = 0; i < gr->manager().size(); ++i)
                        {
                            if (gr->manager()[i].id() == target->id())
                            {
                                gr->mutable_manager()->DeleteSubrange(i, 1);
                                break;
                            }
                        }
                        gr->add_member()->CopyFrom(*target);
                        break;
                    case 7:
                        gr->add_member()->CopyFrom(*target);
                        break;
                    case 8:
                        if (target->id() == user_data->id())
                        {
                            cache->delete_group_relation(gr);
                        }
                        else
                            for (auto i = 0; i < gr->member().size(); ++i)
                            {
                                if (gr->member()[i].id() == target->id())
                                {
                                    gr->mutable_member()->DeleteSubrange(i, 1);
                                    break;
                                }
                            }
                    }
                    // 加入通知
                    auto iasdf = std::make_shared<ChatProto::data>();
                    iasdf->set_action(dd::data_data_mode_MANAGE_GROUP);
                    iasdf->add_details()->PackFrom(gr_data);
                    iasdf->set_timestamp(std::move(Timestamp::now()));
                    notifications_mode->add_notification_and_show(fd, iasdf);
                }
                return true;
            }
        }
        else
        {
        wrong_manage:
            dprintf(fd, "please input the right ID / command to manage \n "
                        "[: manage { --friend/-f }  { friend-ID }  { --ignore/-i  OR  "
                        "--remind/-r  OR  --accept/-a  OR  --delete/-d } ]\n OR \n"
                        "[: manage { --group/-g } { group-ID } { --accept/-a  OR  "
                        "--upgrade/-ug  OR  --downgrade/dg  OR  "
                        "--delete/-d } { target_member-ID } ]\n ");
        }
    }
    else if (c == "showhistory")
    {
        if (commands.size() == 3 && (commands[2] == "--more" || commands[2] == "-m"))
        {
            lines *= 2;
            if (chat_fr.lock() != nullptr)
            {
                hss->changed(std::move(cache->get_history_from_file(chat_fr.lock()->chatid(), lines)));
            }
            else if (chat_gr.lock() != nullptr)
            {
                hss->changed(std::move(cache->get_history_from_file(chat_gr.lock()->chatid(), lines)));
            }
            else
            {
                dprintf(fd, "u are not in chat\n");
                return false;
            }
            dprintf(fd, "show %ud lines history\n", lines);
            return true;
        }
        else
        {
            return false;
        }
    }
    else if (c == "show")
    {
        if (commands.size() == 2)
        {
            auto &&i = user_data->DebugString();
            dprintf(fd, "%s\n", i.c_str());
            cache->show_friend_data(fd);
            cache->show_all_group_data(fd, false);
            return true;
        }
        else if (commands.size() == 3 && (commands[2] == "--more" || commands[2] == "-m" || commands[2] == "more"))
        {
            auto cf = chat_fr.lock();
            auto cg = chat_gr.lock();
            if (cf)
            {
                lines += 100;
                cache->get_history_from_file(cf->chatid(), lines);
            }
            else if (cg)
            {
                lines += 100;
                cache->get_history_from_file(cg->chatid(), lines);
            }
            else
            {
                dprintf(fd, "please set CHAT environment First");
                return false;
            }
            dprintf(fd, "showed %ud lines history", lines);
            return true;
        }
        else if (commands.size() == 3 && (commands[2] == "--notices" || commands[2] == "-n"))
        {
            auto &&i = user_data->DebugString();
            dprintf(fd, "%s\n", i.c_str());

            notifications_mode->show_all(fd);
            return true;
        }
        else if (commands.size() == 3 && (commands[2] == "--all" || commands[2] == "-a"))
        {
            auto &&i = user_data->DebugString();
            dprintf(fd, "%s\n", i.c_str());

            cache->show_friend_data(fd);
            cache->show_all_group_data(fd, true);
            return true;
        }
        else if (commands.size() == 3 && (commands[2] == "--friend" || commands[2] == "-f"))
        {
            auto &&i = user_data->DebugString();
            dprintf(fd, "%s\n", i.c_str());
            cache->show_friend_data(fd);
            return true;
        }
        else if (commands.size() == 4 && (commands[2] == "--group" || commands[2] == "-g"))
        {
            // auto&& i = user_data->DebugString();
            // dprintf(fd, "%s\n", i.c_str());
            if (false == cache->show_one_group_data(commands[3], fd))
            {
                dprintf(fd, "[%s] group not found\n", commands[3].c_str());
                return false;
            }
            return true;
        }
        else
        {
            dprintf(fd, "please input the right command / ID\nlike\n "
                        "[: show] OR\n"
                        "[: show {--all/-a} ] OR\n"
                        "[: show {--friend/-f} ] OR\n"
                        "[: show {--group/-g} {group-ID}]");
            return false;
        }
    }
    else if ((c == "quit" || c == "q") && commands.size() == 2)
    {
        exit(0);
    }
    else if (c == "sendfile" && commands.size() == 4)
    {
        auto i = cache->find_friend_from_id(commands[2]);
        if (i == nullptr)
        {
            dprintf(fd,
                    "[%s] Not found\nplease input the right command / path / "
                    "ID\nlike\n [: sendfile {friend-ID} {path/to/file}]\n",
                    commands[2].c_str());
            return false;
        }
        auto filefd(std::move(file::open(commands[3].c_str(), O_RDONLY)));
        if (filefd.fd_() == -1)
        {
            dprintf(fd,
                    " file : [%s] Not found\nplease input the right command / path / "
                    "ID\nlike\n [: sendfile "
                    "{friend-ID} {path/to/file}]\n",
                    commands[3].c_str());
            return false;
        }
        struct stat stbf;
        if (-1 == fstat(filefd.fd_(), &stbf))
        {
            dprintf(STDERR_FILENO, "error get data with file [%s] , Because of : %s\n", commands[3].c_str(),
                    strerror(errno));
            return false;
        }
        else if (!S_ISREG(stbf.st_mode))
        {
            dprintf(STDERR_FILENO, "error get data with file [%s] , it's not normal file\n", commands[3].c_str());
            return false;
        }
        auto asdfasdf = cache->find_friend_from_id(commands[2]);
        if (asdfasdf == nullptr)
        {
            dprintf(fd, "friend [%s] not found", commands[2].c_str());
            return false;
        }
        ChatProto::file f;
        f.mutable_owner()->CopyFrom(*user_data);
        f.set_filename(commands[3].substr(commands[3].find_last_of('/') + 1));
        f.set_act_mode(ChatProto::file_file_act_send);
        f.set_offset(0);
        Message_Package<ChatProto::data> msgf;
        msgf.data->add_details()->PackFrom(f);
        msgf.data->set_action(dd::data_data_mode_SEND_FILE);
        msgf.data->set_timestamp(Timestamp::now());
        msgf.send(skfd);
        do
        {
            msgf.Recv(skfd);
            if (msgf.data == nullptr)
            {
                dprintf(STDOUT_FILENO, "server closed connection\n");
                return true;
            }
            else if (msgf.data->action() == dd::data_data_mode_SEND_FILE_OK)
            {
                msgf.data->details()[0].UnpackTo(&f);
                break;
            }
            else if (msgf.data->action() == dd::data_data_mode_false_)
            {
                ChatProto::error_package err;
                msgf.data->details()[0].UnpackTo(&err);
                dprintf(STDOUT_FILENO, "sendfile failed because of : %s\n", err.reason().c_str());
            }
            else
            {
                package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msgf.data);
            }
        } while (1);
        int newskfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serverAddress_;

        serverAddress_.sin_family = AF_INET;
        serverAddress_.sin_addr.s_addr = inet_addr(f.ip().c_str());
        serverAddress_.sin_port = htons(std::stoi(f.port()));
        if (-1 == connect(newskfd, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)))
        {
            dprintf(STDERR_FILENO, "error connecting to file server\n");
            return false;
        }

        f.set_size(stbf.st_size);
        f.set_md5(calculateFileMD5(filefd.fd_()));
        Message_Package<ChatProto::data> msg;
        msg.data->set_action(dd::data_data_mode_SEND_FILE);
        msg.data->add_details()->PackFrom(f);
        msg.data->set_timestamp(msg.timestamp.data);
        msg.send(newskfd);
        msg.Recv(newskfd);
        if (msg.data == nullptr)
        {
            dprintf(STDOUT_FILENO, "file server closed connection\n");
            close(newskfd);
            return true;
        }
        if (msg.data->action() != dd::data_data_mode_SEND_FILE_OK)
        {
            ChatProto::error_package err;
            msg.data->details()[0].UnpackTo(&err);
            dprintf(fd, "file send error Because of : %s\n", err.reason().c_str());
            close(newskfd);
            return false;
        }
        int mask = fcntl(filefd.fd_(), F_GETFL, 0);
        fcntl(filefd.fd_(), F_SETFL, mask & (~O_NONBLOCK));

        off_t offset = 0;
        int sendfilereturnvalue = sendfile(newskfd, filefd.fd_(), &offset, f.size());

        if (sendfilereturnvalue == -1 || offset != f.size())
        {

            dprintf(fd, "file send false Because server closed link\n");
            close(newskfd);
            return false;
        };
        fcntl(filefd.fd_(), F_SETFL, mask);

        msg.Recv(newskfd);
        if (msg.data == nullptr)
        {
            dprintf(STDOUT_FILENO, "file server closed connection\n");
            close(newskfd);
            return true;
        }
        if (msg.data->action() != dd::data_data_mode_RECV_FILE_OK)
        {
            ChatProto::error_package err;
            msg.data->details()[0].UnpackTo(&err);
            dprintf(fd, "file send error Because of : %s\n", err.reason().c_str());
            close(newskfd);
            return false;
        }
        else
        {
            close(newskfd);
            msg.data->Clear();
            msg.data->add_details()->PackFrom(f);
            msg.data->add_details()->PackFrom(*(i->friend_relation));
            msg.data->set_timestamp(msg.timestamp.data);
            msg.data->set_action(dd::data_data_mode_SEND_FILE_DONE);
            msg.send(skfd);
            msg.Recv(skfd);
            if (msg.data == nullptr)
            {
                dprintf(STDOUT_FILENO, "server closed connection\n");

                return true;
            }
            if (msg.data->action() == dd::data_data_mode_SEND_FILE_DONE_OK)
            {
                notifications_mode->add_notification_and_show(fd, msg.data);
                return true;
            }
            else
            {
                ChatProto::error_package err;
                msg.data->details()[0].UnpackTo(&err);
                dprintf(fd, "sendfile err Because of :%s", err.reason().c_str());
                return false;
            }
        }
        // dprintf(fd, "please input the right command / path / ID\nlike\n [:
        // sendfile {friend-ID} {path/to/file}] OR\n");
    }
    else if (c == "recvfile")
    {
        if (commands.size() != 5)
        {
            dprintf(fd, "please input the right command / path / ID\nlike\n [: recvfile "
                        "{friend-ID} {file-name} {load/path}] \n");
            return false;
        }
        auto i = cache->find_friend_from_id(commands[2]);
        if (i == nullptr)
        {
            dprintf(fd, "firend [%s] not found\n", commands[2].c_str());
            dprintf(fd, "please input the right command / path / ID\nlike\n [: recvfile "
                        "{friend-ID} {file-name} {load/path}] \n");
            return false;
        }
        std::shared_ptr<ChatProto::file> file_data = nullptr;
        auto tmp_file_data = std::make_shared<ChatProto::file>();
        ChatProto::data tmpdata;
        for (auto &nt : notifications_mode->get())
        {
            if (nt.mode == dd::data_data_mode_SEND_FILE_DONE)
            {
                if (!nt.notify->details()[0].Is<ChatProto::data>())
                {
                    for (auto &i : nt.notify->details())
                        if (i.Is<ChatProto::file>())
                        {
                            i.UnpackTo(tmp_file_data.get());
                            if (tmp_file_data->filename() == commands[3])
                            {
                                file_data = tmp_file_data;
                                break;
                            }
                        }
                }
                else
                {
                    nt.notify->details()[0].UnpackTo(&tmpdata);
                    for (auto &i : tmpdata.details())
                        if (i.Is<ChatProto::file>())
                        {
                            i.UnpackTo(tmp_file_data.get());
                            if (tmp_file_data->filename() == commands[3])
                            {
                                file_data = tmp_file_data;
                                break;
                            }
                        }
                }
            }
        }
        if (file_data == nullptr)
        {
            dprintf(fd, "file [%s] not found\n", commands[3].c_str());
            dprintf(fd, "please input the right command / path / ID\nlike\n [: recvfile "
                        "{friend-ID} {file-name} "
                        "{load/DIR/path}] \n");
            return false;
        }
        auto iii = opendir(commands[4].c_str());
        if (iii == nullptr)
        {
            dprintf(fd, "the path must be direct to a directory , [%s] cannot \n", commands[4].c_str());
            return false;
        }
        else
        {
            closedir(iii);
        }
        auto file_name = commands[4] + ("/" + (commands[4].back() == '/')) + file_data->filename();
        auto file_fd(file::open(file_name.c_str(), O_RDWR | O_CREAT, 0755));
        if (file_fd.fd_() == -1)
        {
            dprintf(fd, "file [%s] cannot be opened \n", file_name.c_str());
            return false;
        }
        msg_send(skfd, *file_data, dd::data_data_mode_RECV_FILE);
        Message_Package<ChatProto::data> msg;
        do
        {
            msg.Recv(skfd);
            if (msg.data == nullptr)
            {
                dprintf(STDOUT_FILENO, "server closed connection\n");
                return true;
            }
            if (msg.data->action() == dd::data_data_mode_RECV_FILE_OK)
            {
                msg.data->details()[0].UnpackTo(file_data.get());
                break;
            }
            else
            {
                package_queue.emplace_back(std::make_shared<ChatProto::data>()).swap(msg.data);
            }
        } while (1);
        int newskfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serverAddress_;

        serverAddress_.sin_family = AF_INET;
        serverAddress_.sin_addr.s_addr = inet_addr(file_data->ip().c_str());
        serverAddress_.sin_port = htons(std::stoi(file_data->port()));

        if (-1 == connect(newskfd, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)))
        {
            dprintf(STDERR_FILENO, "error connecting to file server\n");
            return false;
        }
        // Message_Package<ChatProto::data> msg;
        msg.data->Clear();
        msg.data->add_details()->PackFrom(*file_data);
        msg.data->set_action(dd::data_data_mode_RECV_FILE);
        msg.data->set_timestamp(Timestamp::now());
        msg.send(newskfd);
        msg.Recv(newskfd);
        if (msg.data == nullptr)
        {
            dprintf(STDOUT_FILENO, "server closed connection\n");
            return true;
        }
        if (msg.data->action() == dd::data_data_mode_false_)
        {
            ChatProto::error_package err;
            msg.data->details()[0].UnpackTo(&err);
            dprintf(fd, "recv error Because of :%s\n", err.reason().c_str());
            return false;
        }
        auto size = file_data->size();
        char buffer[BUFFER_SIZE];
        while (size > 0)
        {
            auto bytes = recv(newskfd, buffer, size < BUFFER_SIZE ? size : BUFFER_SIZE, 0);
            if (bytes < 0)
            {
                // LOG(WARNING) << "recv error " << t->file_data.DebugString();
                dprintf(fd, "recv error Because of :%s\n", strerror(errno));
                close(newskfd);
                return true;
            }
            if (-1 == write(file_fd.fd_(), buffer, bytes))
            {
                perror("write To file Error");
                return false;
            }
            size -= bytes;
        }

        if (calculateFileMD5(file_fd.fd_()) != file_data->md5())
        {
            dprintf(fd, "[%s] Inconsistent data verification\n", file_name.c_str());
        }
        else
        {
            dprintf(fd, "[%s] recv done \n", file_name.c_str());
        }
        close(newskfd);
        return true;
    }
    else
    {
        // dprintf(fd, "enter [: help] to view the commands rule\n");
        return false;
    }
    return false;
}
#include <openssl/evp.h>
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