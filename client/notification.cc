#include "notification.hh"

notification::notification(google::protobuf::Any const &noti) : notify(std::make_shared<ChatProto::data>())
{
    noti.UnpackTo(notify.get());
    timestamp = notify->timestamp();
    is_done = false;
    mode = notify->action();
}
notification::notification(std::shared_ptr<ChatProto::data> notify)
    : timestamp(notify->timestamp()), notify(notify), is_done(false), mode(notify->action())
{
}
bool notification::operator==(const notification &other)
{
    return (timestamp == other.timestamp);
}
bool notification::operator<(const notification &other)
{
    return (timestamp < other.timestamp);
}
notifications::notifications()
{
    init();
}
void notifications::update()
{
    bool is_active = false;
    std::string &&t = Timestamp::now();
    while (isTimeDifferenceGreaterThanThreeDays(nq.front().timestamp.data, t))
    {
        nq.pop_front();
    }
    flush_to_file();
}
void notifications::add_notification(notification const &ntf)
{
    std::lock_guard<std::mutex> lock(ntfmutex);
    if (nq.back() < ntf)
    {
        nq.emplace_back(ntf);
    }
    else
    {
        nq.emplace_back(ntf);
        sort(nq.begin(), nq.end());
    }
    update();
}
void notifications::add_notification_and_show(int fd, notification const &ntf)
{
    std::lock_guard<std::mutex> lock(ntfmutex);
    if (nq.back() < ntf)
    {
        nq.emplace_back(ntf);
    }
    else
    {
        nq.emplace_back(ntf);
        sort(nq.begin(), nq.end());
    }
    update();
    show_notify(fd, ntf);
}
std::deque<notification> &notifications::get()
{
    return nq;
}

bool notifications::flush_to_file()
{
    std::ofstream file(std::move("." + calculateMD5(std::to_string(user_data->id()) + "notifications")));
    if (!file.is_open())
    {
        perror("init notification failed");
        return false;
    }
    else
    {
        ChatProto::data data;
        for (auto const &item : nq)
        {
            data.add_details()->PackFrom(*item.notify);
        }
        data.SerializeToOstream(&file);
    }
    return true;
};
void notifications::init()
{
    std::ifstream infile(std::move("." + calculateMD5(std::to_string(user_data->id()) + "notifications")));
    if (!infile.is_open())
    {
        std::ofstream file(std::move("." + calculateMD5(std::to_string(user_data->id()) + "notifications")));
        if (!file.is_open())
        {
            perror("init notification failed");
        }
        file.close();
    }
    else
    {
        ChatProto::data data;
        if (!data.ParseFromIstream(&infile))
        {
            puts("Couldn't parse notification file ");
        }
        ChatProto::data noti;
        for (auto &i : data.details())
        {
            nq.emplace_back(i);
        }
    }
    infile.close();
}

#include <stdio.h>

#include <ctime>

bool isTimeDifferenceGreaterThanThreeDays(const std::string &timestamp1, const std::string &timestamp2)
{
    // 将时间戳转换为tm结构体
    std::tm time1 = {};
    std::tm time2 = {};
    strptime(timestamp1.c_str(), "%Y-%m-%d %H:%M:%S", &time1);
    strptime(timestamp2.c_str(), "%Y-%m-%d %H:%M:%S", &time2);

    // 将tm结构体转换为时间戳
    std::time_t t1 = std::mktime(&time1);
    std::time_t t2 = std::mktime(&time2);

    // 计算时间差
    double difference = std::difftime(t2, t1);

    //     判断时间差是否大于三天（259200秒）
    if (difference > 259200)
    {
        return true;
    }
    else
    {
        return false;
    }
}
// //for test
// #include <iostream>
// int main(){
//     std::cout << isTimeDifferenceGreaterThanThreeDays("2000-01-01
//     00:00:00.000000","2000-01-02 00:00:00.000000") << std::endl;
// }

void notifications::show_last_one(int fd)
{
    show_notify(fd, this->nq.back());
}

void notifications::show_all(int fd)
{
    for (auto const &item : this->nq)
    {
        show_notify(fd, item);
    }
}

void notifications::show_notify(int fd, const notification &n)
{
    using dd = ChatProto::data_data_mode;
    std::string output;

    std::cout << n.notify->DebugString() << std::endl;

    switch (n.notify->action())
    {
    case dd::data_data_mode_REQUEST_MAKE_FRIEND:
    {
        ChatProto::friend_relation fr;
        n.notify->details()[0].UnpackTo(&fr);
        output = fr.master().name() + "[" + std::to_string(fr.master().id()) + "] reuqests to be ur friend\n";
        break;
    }
    case dd::data_data_mode_ANSWER_MAKE_FRIEND:
    {
        ChatProto::friend_relation fr;
        n.notify->details()[0].UnpackTo(&fr);
        output = fr.follower().name() + "[" + std::to_string(fr.follower().id()) +
                 (fr.status() == 1 ? "] accept to be ur friend\n" : "] reject to be ur friend\n");
        break;
    }
    case dd::data_data_mode_DELETE_FRIEND:
    {
        ChatProto::friend_relation fr;
        n.notify->details()[0].UnpackTo(&fr);
        output = fr.master().name() + "[" + std::to_string(fr.master().id()) + "] Removed you from friend list\n";
        break;
    }
    case dd::data_data_mode_MANAGE_GROUP:
    {
        ChatProto::group_relation gr;
        n.notify->details()[0].UnpackTo(&gr);
        output = gr.follower().name() + "[" + std::to_string(gr.follower().id()) + "]  has " + [&]() -> std::string
        {
            switch (gr.status())
            {
            case 5:
                return "upgraded";
            case 6:
                return "downgraded";
            case 7:
                return "joined";
            default:
                return "removed";
            }
            return "";
        }() + " in group " + gr.master().name() + "[" + std::to_string(gr.master().id()) + "]\n";
        break;
    }
    case dd::data_data_mode_REQUEST_JOIN_GROUP:
    {
        ChatProto::group_relation gr;
        n.notify->details()[0].UnpackTo(&gr);

        output = gr.follower().name() + "[" + std::to_string(gr.follower().id()) + "]  requests join " +
                 Cache::find_group_from_id(std::to_string(gr.master().id()))->name() + "[G-id :>" +
                 std::to_string(gr.master().id()) + "]\n";
        break;
    }
    case dd::data_data_mode_SEND_FILE_DONE:
    {
        ChatProto::data dt;
        ChatProto::file f;
        ChatProto::friend_relation fr;
        if (n.notify->details_size() == 1)
        {
            n.notify->details()[0].UnpackTo(&dt);
            for (auto &i : dt.details())
                if (i.Is<ChatProto::friend_relation>())
                    i.UnpackTo(&fr);
                else if (i.Is<ChatProto::file>())
                    i.UnpackTo(&f);
            output = fr.master().name() + "[" + std::to_string(fr.master().id()) +
                     "] send file to u \n name :" + f.filename() + "\n size :" + std::to_string(f.size()) + "\n";
        }
        else if (n.notify->details_size() == 2)
        {
            for (auto &i : n.notify->details())
                if (i.Is<ChatProto::friend_relation>())
                    i.UnpackTo(&fr);
                else if (i.Is<ChatProto::file>())
                    i.UnpackTo(&f);
            output = fr.master().name() + "[" + std::to_string(fr.master().id()) +
                     "] send file to u \n name :" + f.filename() + "\n size :" + std::to_string(f.size()) + "\n";
        }
        break;
    }
    case dd::data_data_mode_IGNORE_FRIEND:
    {
        ChatProto::friend_relation fr;
        n.notify->details()[0].UnpackTo(&fr);
        output =
            fr.master().name() + "[" + std::to_string(fr.master().id()) + "]   Blocked you from his/her friends list\n";
        break;
    }
    case dd::data_data_mode_UNIGNORE_FRIEND:
    {
        ChatProto::friend_relation fr;
        n.notify->details()[0].UnpackTo(&fr);
        output =
            fr.master().name() + "[" + std::to_string(fr.master().id()) + "]   has resumed communication with you\n";
        break;
    }
    case dd::data_data_mode_FRIEND_LOG_OUT:
    {
        ChatProto::user_data_package u;
        n.notify->details()[0].UnpackTo(&u);
        output = u.name() + "[" + std::to_string(u.id()) + "]  log out\n";
        break;
    }
    case dd::data_data_mode_FRIEND_LOAD_SERVER:
    {
        ChatProto::user_data_package u;
        n.notify->details()[0].UnpackTo(&u);
        output = u.name() + "[" + std::to_string(u.id()) + "]  log in\n";
        break;
    }
    case dd::data_data_mode_SEND_FILE_DONE_OK:
    {

        ChatProto::file f;
        n.notify->details()[0].UnpackTo(&f);
        if (f.owner().id() == user_data->id())
            output = "[" + f.filename() + "] upload done\n";
        else
            output = "friend {" + f.owner().name() + " [" + std::to_string(f.owner().id()) + "]} send file [" +
                     f.filename() + "] to u\n";
        break;
    }
    }
    auto wide = history_show::get_T_wide();
    for (auto i = 0; i < wide; i++)
        if (0 > write(fd, "-", 1))
            perror("write");
    if (write(fd, "\n", 1) <= 0)
        perror("write");
    //
    if (0 > write(fd, output.c_str(), output.length()))
    {
        perror("write");
    };
    //
    for (auto i = 0; i < wide; i++)
        if (0 > write(fd, "-", 1))
        {
            perror("write");
        };
    if (write(fd, "\n", 1) <= 0)
        perror("write");
}
