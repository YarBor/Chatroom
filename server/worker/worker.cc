#include "worker.hh"
std::map<uint64_t, std::weak_ptr<task>> tasksMap; // 登出的时候去erase
std::mutex tasksMapLock;
std::string calculateMD5(const std::string &input);
void join_userMap(std::shared_ptr<task> &t, mysql::Mysql &sql);
void erase_userMap(std::shared_ptr<task> &t, mysql::Mysql &sql);
std::weak_ptr<task> find_userMap(uint64_t i);
void send_err(std::shared_ptr<task> &t, ChatProto::error_package_mode mode, std::string const &reason);
int send_data_package(std::shared_ptr<task> &t, ChatProto::data_data_mode mode, google::protobuf::Message &message);
int send_data_package(std::shared_ptr<task> &t, ChatProto::data_data_mode mode);
void load_server(ChatProto::user_data_package &userDataPackge, std::shared_ptr<task> job, mysql::Mysql &sql);
void chat_friend(std::shared_ptr<ChatProto::data> &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void chat_group(std::shared_ptr<ChatProto::data> &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void request_make_friend(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql);
void answer_make_friend(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql);
void delete_friends(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql);
void make_group(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql);
void manage_gruop(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql);
void request_join_group(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql);
void send_file(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void recv_file(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void log_out(std::shared_ptr<task> job, mysql::Mysql &sql);
void ignore_friend(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void unignore_friend(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void send_file_done(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void update_data(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void load_user_relation_data(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);
void get_online_friend(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql);


#define do_sql(PUTIN)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        std::stringstream ss;                                                                                          \
        ss << PUTIN;                                                                                                   \
        auto x = sql(ss.str());                                                                                        \
        if (x->OK == false)                                                                                            \
            LOG(WARNING) << "SQL ERROR " << x->command;                                                                \
        else                                                                                                           \
        {                                                                                                              \
            LOG(INFO) << "SQL SUCCESS" << x->command;                                                                  \
        }                                                                                                              \
    } while (0)

#define select_sql(X, SELECT)                                                                                          \
    stringstream ss##X;                                                                                                \
    ss##X << SELECT;                                                                                                   \
    auto X = sql(ss##X.str());                                                                                         \
    LOG(INFO) << "SQL " << X->command;

using dd = ChatProto::data_data_mode;
using ee = ChatProto::error_package_mode;

uint32_t online_number;
std::mutex online_number_lock;
std::hash<std::string> hasher;

struct lock
{
    lock(std::mutex &target_lock) : thislock(&target_lock)
    {
        if (target_lock.try_lock())
            is_locked = true;
        else
        {
            is_locked = false;
            thislock = nullptr;
        }
    }
    ~lock()
    {
        if (is_locked)
        {
            this->thislock->unlock();
        }
    }
    bool is_locked = false;

  private:
    std::mutex *thislock = nullptr;
};

bool do_Job(std::shared_ptr<task> job, mysql::Mysql &sqlLink)
{
    bool is_do_job = false;
    lock lk(job->fd_mutex);
    if (!lk.is_locked)
        return false;

    if (job->revent.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        job->revent.events -= job->revent.events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR);
        do_Job(job, sqlLink); // 看看有没有epollIN事物
        job->callback(EPOLLRDHUP | EPOLLHUP | EPOLLERR);
        log_out(job, sqlLink);
        return false;
    }
    else if (job->event.events & EPOLLIN)
        while (1)
        {
            Message_Package<ChatProto::data> msg;
            auto message = msg.Recvs(job->fd);
            if (message == nullptr)
                return is_do_job;
            else
                is_do_job = true;
            using ChatProto::data_data_mode;
            for (auto &i : *message)
            {
                LOG(INFO) << "get job " << i->DebugString() << " " << i->action();
                switch (i->action())
                {
                case data_data_mode::data_data_mode_LOAD_SERVER: {
                    if (!i->details()[0].Is<ChatProto::user_data_package>())
                        goto error;
                    ChatProto::user_data_package userDataPackge;
                    i->details()[0].UnpackTo(&userDataPackge);
                    load_server(userDataPackge, job, sqlLink);
                    break;
                }
                case data_data_mode::data_data_mode_CHAT_FRIEND: {
                    if (!i->details()[0].Is<ChatProto::chatHistory_data_package>())
                        goto error;
                    chat_friend(i, job, sqlLink);
                    break;
                }

                case dd::data_data_mode_CHAT_GROUP: {
                    if (!i->details()[0].Is<ChatProto::chatHistory_data_package>())
                        goto error;
                    chat_group(i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_REQUEST_MAKE_FRIEND: {
                    if (!i->details()[0].Is<ChatProto::friend_relation>())
                        goto error;
                    request_make_friend(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_ANSWER_MAKE_FRIEND: {
                    if (!i->details()[0].Is<ChatProto::friend_relation>())
                        goto error;
                    answer_make_friend(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_DELETE_FRIEND: {
                    if (!i->details()[0].Is<ChatProto::friend_relation>())
                        goto error;
                    delete_friends(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_MAKE_GROUP: {
                    if (!i->details()[0].Is<ChatProto::group_relation>())
                        goto error;
                    make_group(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_REQUEST_JOIN_GROUP: {
                    if (!i->details()[0].Is<ChatProto::group_relation>())
                        goto error;
                    request_join_group(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_MANAGE_GROUP: {
                    if (!i->details()[0].Is<ChatProto::group_relation>())
                        goto error;
                    manage_gruop(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_SEND_FILE: {
                    if (!i->details()[0].Is<ChatProto::file>())
                        goto error;
                    send_file(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_SEND_FILE_DONE: {
                    if (!i->details()[0].Is<ChatProto::file>())
                        goto error;
                    send_file_done(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_IGNORE_FRIEND: {
                    if (!i->details()[0].Is<ChatProto::friend_relation>())
                        goto error;
                    ignore_friend(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_UNIGNORE_FRIEND: {
                    if (!i->details()[0].Is<ChatProto::friend_relation>())
                        goto error;
                    unignore_friend(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_UPDATE_DATA: {
                    if (!i->details()[0].Is<ChatProto::user_data_package>())
                        goto error;
                    update_data(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_LOAD_USER_RELATION_DATA: {
                    if (!i->details()[0].Is<ChatProto::user_data_package>())
                        goto error;
                    load_user_relation_data(*i, job, sqlLink);
                    break;
                }
                case dd::data_data_mode_GET_ONLINE_FRIENDS: {
                    if (!i->details()[0].Is<ChatProto::user_data_package>())
                        goto error;
                    get_online_friend(*i, job, sqlLink);
                    break;
                }
                case data_data_mode::data_data_mode_RECV_FILE: {
                    if (!i->details()[0].Is<ChatProto::file>())
                        goto error;
                    recv_file(*i, job, sqlLink);
                    break;
                }

                default:
                error:
                    LOG(ERROR) << "UB  " << i->DebugString();
                }
            }
        }
    return true;
}
void join_userMap(std::shared_ptr<task> &t, mysql::Mysql &sql)
{
    {
        std::lock_guard<std::mutex> lock(tasksMapLock);
        tasksMap[t->ID] = t;
    }
    std::stringstream command;
    command = std::stringstream();
    command << "insert into online_users values( " << t->ID << ", '" << t->name << "' ) ";
    sql(command.str());
}
void erase_userMap(std::shared_ptr<task> &t, mysql::Mysql &sql)
{
    {
        std::lock_guard<std::mutex> lock(tasksMapLock);
        tasksMap.erase(t->ID);
    }
    std::stringstream command;
    command = std::stringstream();
    command << "delete from online_users WHERE id = " << t->ID;
    sql(command.str());
}
std::weak_ptr<task> find_userMap(uint64_t i)
{
    std::lock_guard<std::mutex> lock(tasksMapLock);
    auto f = tasksMap.find(i);
    if (f == tasksMap.end())
        return std::shared_ptr<task>(nullptr);
    else
        return f->second;
}
void send_err(std::shared_ptr<task> &t, ChatProto::error_package_mode mode, std::string const &reason)
{
    ChatProto::error_package err;
    err.set_stat(mode);
    err.set_reason(reason);
    Message_Package<ChatProto::data> message;

    message.data->set_action(ChatProto::data_data_mode::data_data_mode_false_);
    message.data->add_details()->PackFrom(err);
    message.data->set_timestamp(Timestamp::now());
    {
        std::lock_guard<std::mutex> lock(t->write_fd_mutex);
        message.send(t->fd);
    }
    return;
}
int send_data_package(std::shared_ptr<task> &t, ChatProto::data_data_mode mode, google::protobuf::Message &message)
{
    Message_Package<ChatProto::data> mp;
    mp.data->set_action(mode);
    mp.data->add_details()->PackFrom(message);
    mp.data->set_timestamp(Timestamp::now());
    {
        std::lock_guard<std::mutex> lock(t->write_fd_mutex);
        return mp.send(t->fd);
    }
}
int send_data_package(std::shared_ptr<task> &t, ChatProto::data_data_mode mode)
{
    Message_Package<ChatProto::data> mp;
    mp.data->set_action(mode);
    mp.data->clear_details();
    mp.data->set_timestamp(Timestamp::now());
    {
        std::lock_guard<std::mutex> lock(t->write_fd_mutex);
        return mp.send(t->fd);
    }
}

using std::stringstream;

void load_server(ChatProto::user_data_package &userDataPackge, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    stringstream command;
    if (userDataPackge.status() == ChatProto::user_data_package_LOAD_SERVER::user_data_package_LOAD_SERVER_LOG_IN)
    {
        command << "SELECT name , email FROM users WHERE id = " << userDataPackge.id() << " AND password = '"
                << sql.escapeString(calculateMD5(userDataPackge.password())) << "'";
        auto result = sql(std::move(command.str()));
        if (result->OK == false || result->rows_size == 0)
        {
            send_err(job, ChatProto::error_package_mode::error_package_mode_LOAD_SERVER_FAIL, "Not found");
            return;
        }
        else
        {
            auto i = find_userMap(userDataPackge.id()).lock();
            if (i != nullptr)
            {
                send_err(job, ChatProto::error_package_mode::error_package_mode_LOAD_SERVER_FAIL,
                         "Logged in elsewhere");
                return;
            }
            auto re = result->get_value();
            userDataPackge.set_name(re[0]);
            userDataPackge.set_email(re[1]);
            send_data_package(job, ChatProto::data_data_mode::data_data_mode_LOAD_SERVER_OK, userDataPackge);
            job->ID = userDataPackge.id();
            job->name = userDataPackge.name();
            if (i != nullptr)
            {
                return;
            }
        }
    }
    else if (userDataPackge.status() == ChatProto::user_data_package_LOAD_SERVER::user_data_package_LOAD_SERVER_SIGN_UP)
    {
        command << "insert into users (name, password, email) values ('" << sql.escapeString(userDataPackge.name())
                << "','" << calculateMD5(userDataPackge.password()) << "','" << sql.escapeString(userDataPackge.email())
                << "')";
        auto result = sql(std::move(command.str()));
        if (result->OK == false)
        {
            LOG(WARNING) << "SQL FALSE  " << result->command;
            return;
        }
        else
        {
            userDataPackge.set_id(std::stol(sql.get_last_insert_id()));
            send_data_package(job, ChatProto::data_data_mode_LOAD_SERVER_OK, userDataPackge);
            job->ID = userDataPackge.id();
            job->name = userDataPackge.name();
        }
    }
    userDataPackge.set_password("");
    join_userMap(job, sql);
    command = stringstream();
    command << "select online_users.id from (select tgt2 , chatid from relationship "
               "where "
               "tgt1 = "
            << job->ID
            << ") as friend  , online_users where "
               "friend.tgt2 = online_users.id and friend.chatid < 'g' order by name ";
    auto result = sql(command.str());
    if (result->OK == false)
        LOG(WARNING) << "SQL FALSE " << result->command;
    else
        while (1)
        {
            auto row = result->get_value();
            if (row == nullptr)
                break;
            else
            {
                auto ptr = find_userMap(std::stol(row[0])).lock();
                if (ptr == nullptr)
                {
                    LOG(WARNING) << " 缓存和 数据库 不一致" << std::endl;
                    continue;
                }
                else
                {
                    send_data_package(ptr, ChatProto::data_data_mode_FRIEND_LOAD_SERVER, userDataPackge);
                }
            }
        }
    {
        std::lock_guard<std::mutex> l(online_number_lock);
        ++online_number;
    }
    return;
}
void chat_friend(std::shared_ptr<ChatProto::data> &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::chatHistory_data_package chatHistory;
    data->details()[0].UnpackTo(&chatHistory);
    for (auto i : chatHistory.messages())
    {
        std::string msg = i.message();
        stringstream ss;
        ss << "INSERT INTO chathistory (speaker_id, speaker_name, chatid, message, "
              "messagetime) values ("
           << i.speaker().id() << ", '" << i.speaker().name() << "', '" << chatHistory.chatid() << "','"
           << sql.escapeString(msg) << "','" << i.timestamp() << "')";

        auto result = sql(ss.str());
        if (result->OK == false)
        {
            LOG(WARNING) << "SQL FALSE " << result->command;
        }
    }

    auto ptr = find_userMap(chatHistory.recevier_id()).lock();
    if (ptr != nullptr)
        send_data_package(ptr, dd::data_data_mode_CHAT_FRIEND, chatHistory);
    send_data_package(job, dd::data_data_mode_CHAT_FRIEND_OK);

    return;
}
void chat_group(std::shared_ptr<ChatProto::data> &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::chatHistory_data_package dp;
    data->details()[0].UnpackTo(&dp);
    if (dp.messages().size() == 0)
    {
        send_data_package(job, dd::data_data_mode_CHAT_GROUP_OK);
        return;
    }
    for (auto &i : dp.messages())
    {
        std::string msg = i.message();
        stringstream ss;
        ss << "INSERT INTO chathistory (speaker_id, speaker_name, chatid, message, "
              "messagetime) values ("
           << i.speaker().id() << ", '" << i.speaker().name() << "', '" << dp.chatid() << "','" << sql.escapeString(msg)
           << "','" << i.timestamp() << "')";

        auto result = sql(ss.str());
        if (result->OK == false)
        {
            LOG(WARNING) << "SQL FALSE " << result->command;
        }
    }
    stringstream ss;
    ss << "select id from online_users , (select tgt2 from relationship where "
          "chatid = '"
       << dp.chatid() << "' and tgt2 != " << dp.messages()[0].speaker().id()
       << ") as friend where friend.tgt2 = online_users.id";
    auto result = sql(ss.str());
    if (result->OK == false)
        LOG(WARNING) << "SQL FALSE " << result->command;
    {
        while (1)
        {
            auto row = result->get_value();
            if (row == nullptr)
                break;
            else
            {
                auto i = find_userMap(std::stol(row[0])).lock();
                if (i == nullptr)
                    continue;
                else
                {
                    send_data_package(i, dd::data_data_mode_CHAT_GROUP, dp);
                }
            }
        }
    }
    send_data_package(job, dd::data_data_mode_CHAT_GROUP_OK);
    return;
}

void request_make_friend(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::friend_relation fr;
    Data.details()[0].UnpackTo(&fr);
    std::stringstream ss;
    {
        select_sql(X, "select name from users where id = " << fr.follower().id());
        if (!(X->OK && X->rows_size > 0))
        {
            send_err(job, ChatProto::error_package_mode_REQUEST_MAKE_FRIEND_FAIL, "user not found");
            return;
        }
    }

    // ss << "insert into notify (receiver_id,mode,data,time) values (" <<
    // fr.follower().id() << " , "
    //    << dd::data_data_mode_REQUEST_MAKE_FRIEND << " , '" <<
    //    sql.escapeString(Data.SerializeAsString()) << "' , '"
    //    << Data.timestamp() << "')";

    // std::cout << sql.escapeString(Data.SerializeAsString()) << std::endl;
    // auto i = sql(ss.str());
    select_sql(i, "insert into notify (receiver_id,mode,data,time) values( "
                      << fr.follower().id() << " , " << dd::data_data_mode_REQUEST_MAKE_FRIEND << " , '"
                      << Data.SerializeAsString() << "' , '" << Data.timestamp() << "' )");

    if (i->OK == false)
    {
        LOG(WARNING) << "SQL FALSE " << i->command << "   " << mysql_error(sql.sql()) << " " << mysql_errno(sql.sql())
                     << std::endl;
        send_err(job, ChatProto::error_package_mode_REQUEST_MAKE_FRIEND_FAIL, "sql failed");
        return;
    }

    auto ptr = find_userMap(fr.follower().id()).lock();
    if (ptr != nullptr)
    {
        send_data_package(ptr, dd::data_data_mode_REQUEST_MAKE_FRIEND, fr);
        //    send_err(job, ChatProto::error_package_mode_REQUEST_MAKE_FRIEND_FAIL,
        //    "send request error");
    }
    send_data_package(job, dd::data_data_mode_REQUEST_MAKE_FRIEND_OK);
    return;
}

void answer_make_friend(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::friend_relation fr;
    Data.details()[0].UnpackTo(&fr);
    std::stringstream ss;

    fr.set_chatid("f" + calculateMD5(std::to_string(fr.master().id()) + std::to_string(fr.follower().id())));
    Data.clear_details();
    Data.add_details()->PackFrom(fr);

    ss << "insert into notify (receiver_id,mode,data,time) values (" << fr.master().id() << " , "
       << dd::data_data_mode_ANSWER_MAKE_FRIEND << " ,'" << Data.SerializeAsString() << "' , '" << Data.timestamp()
       << "')";

    auto i = sql(ss.str());
    auto ptr = find_userMap(fr.master().id()).lock();
    if (i->OK == false)
    {
        LOG(WARNING) << "SQL FALSE " << i->command;
        send_err(job, ee::error_package_mode_ANSWER_MAKE_FRIEND_FAIL, "SQL FALSE");
        return;
    }
    if (fr.status() == 1)
    {
        fr.mutable_master()->set_is_active(find_userMap(fr.master().id()).lock() ? true : false);
        // fr.set_chatid("f" + calculateMD5(std::to_string(fr.master().id()) +
        // std::to_string(fr.follower().id())));
        send_data_package(job, dd::data_data_mode_ANSWER_MAKE_FRIEND_OK, fr);
        if (ptr != nullptr)
            send_data_package(ptr, dd::data_data_mode_ANSWER_MAKE_FRIEND, fr);
        do_sql("insert into relationship(tgt1, tgt2,stat,chatid) values ("
               << fr.follower().id() << "," << fr.master().id() << "," << fr.status() << ",'"
               << fr.chatid().substr(0, 255) << "')");
        do_sql("insert into relationship(tgt1, tgt2,stat,chatid) values ("
               << fr.master().id() << "," << fr.follower().id() << "," << fr.status() << ",'"
               << fr.chatid().substr(0, 255) << "')");
    }
    else if (fr.status() == 0)
    {
        send_data_package(ptr, dd::data_data_mode_ANSWER_MAKE_FRIEND, fr);
        send_data_package(job, dd::data_data_mode_ANSWER_MAKE_FRIEND_OK);
    }
    return;
}

void delete_friends(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::friend_relation fr;
    Data.details()[0].UnpackTo(&fr);
    fr.set_status(-1);
    do_sql("insert into notify (receiver_id,mode,data,time) values ("
           << fr.follower().id() << " , " << dd::data_data_mode_DELETE_FRIEND << " ,'" << Data.SerializeAsString()
           << "' , '" << Data.timestamp() << "')");
    auto ptr = find_userMap(fr.follower().id()).lock();
    if (ptr != nullptr)
    {
        if (false == send_data_package(ptr, dd::data_data_mode_DELETE_FRIEND, fr))
            send_err(job, ChatProto::error_package_mode_DELETE_FRIEND_FAIL, "send error");
        else
            send_data_package(job, dd::data_data_mode_DELETE_FRIEND_OK);
    }
    else
        send_data_package(job, dd::data_data_mode_DELETE_FRIEND_OK);
    do_sql("delete from relationship where chatid = '" << fr.chatid() << "'");
}

void make_group(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::group_relation gr;
    Data.details()[0].UnpackTo(&gr);
    std::string const &group_name = gr.master().name();
    std::string const &owner_name = gr.owner().name();
    int32_t const &own_id = gr.owner().id();

    gr.set_id(sql.get_last_insert_id());
    gr.set_chatid("g" + calculateMD5(group_name + "c*3nS&" + gr.id()));

    do_sql("insert into user_set(name, owner_name , owner_id ,chatid ) values('"
           << sql.escapeString(group_name) << "', '" << sql.escapeString(owner_name) << "' , " << own_id << " ,'"
           << sql.escapeString(gr.chatid()) << "')");
    gr.set_status(3);
    gr.set_id(sql.get_last_insert_id());
    gr.mutable_master()->set_id(std::stol(gr.id()));

    // if (!gr.has_follower()) {
    //   ChatProto::user_data_package u(gr.owner());
    //   gr.set_allocated_follower(&u);
    // }
    do_sql("insert into relationship(tgt1, tgt2 , stat , chatid) values ( "
           << gr.id() << " , " << gr.owner().id() << " , " << 3 << ", '" << gr.chatid() << "' )");
    send_data_package(job, dd::data_data_mode_MAKE_GROUP_OK, gr);
}

void manage_gruop(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::group_relation member_change;
    Data.details()[0].UnpackTo(&member_change);
    std::string reason;
    // 5 to upgrade member
    // 6 to downgrade manager
    // 7 to join member  // 需要成员细节
    // 8 to delete member
    switch (member_change.status())
    {
    case 1:
    case 2:
    case 3:
        LOG(ERROR) << "UB  " << Data.DebugString();
        reason = "UB  ";
        goto error;
    case 5: {
        do_sql("update relationship SET stat = 2 WHERE tgt2 = " << member_change.follower().id() << " and chatid = '"
                                                                << member_change.chatid() << "'");
        break;
    }
    case 6: {
        do_sql("update relationship SET stat = 1 WHERE tgt2 = " << member_change.follower().id() << " and chatid = '"
                                                                << member_change.chatid() << "'");
        break;
    }
    case 7: {
        do_sql("insert into relationship (tgt1,tgt2,stat,chatid) values("
               << member_change.master().id() << "," << member_change.follower().id() << "," << 1 << ",'"
               << member_change.chatid() << "')");
        break;
    }
    case 8: {
        do_sql("delete from relationship  where chatid = '" << member_change.chatid()
                                                            << "' AND tgt2 = " << member_change.follower().id());
        break;
    }
    default:
        LOG(ERROR) << "UB  " << Data.DebugString();
        reason = "UB  ";
        goto error;
    }
    {
        std::shared_ptr<task> ptr;
        if ((ptr = find_userMap(member_change.follower().id()).lock()) != nullptr)
            send_data_package(ptr, dd::data_data_mode_MANAGE_GROUP, member_change);
        do_sql("insert into notify(receiver_id,mode,data,time) VALUES("
               << member_change.follower().id() << "   ,   " << dd::data_data_mode_MANAGE_GROUP << "   ,   '"
               << Data.SerializeAsString() << "'  ,  '" << Data.timestamp() << "')");
        send_data_package(job, dd::data_data_mode_MANAGE_GROUP_OK);

        member_change.clear_member();
        member_change.clear_manager();
        select_sql(X, "SELECT tgt2 FROM relationship WHERE tgt1 = " << member_change.master().id()
                                                                    << " and tgt2 != " << member_change.follower().id()
                                                                    << " and tgt2 != " << job->ID << " and chatid = '"
                                                                    << member_change.chatid() << "' ORDER BY stat");
        if (X->OK == false)
        {
            LOG(ERROR) << " SQL Error " << X->command;
            reason = "SQL Error";
            goto error;
        }
        else
        {
            while (1)
            {
                auto row = X->get_value();
                if (row == nullptr)
                    break;
                auto member = find_userMap(std::stol(row[0])).lock();
                if (member != nullptr)
                    send_data_package(member, dd::data_data_mode_MANAGE_GROUP, member_change);
                do_sql("insert into notify(receiver_id,mode,data,time) VALUES("
                       << row[0] << "," << dd::data_data_mode_MANAGE_GROUP << ",'" << Data.SerializeAsString() << "','"
                       << Data.timestamp() << "')");
            }
        }
    }
    return;
error:
    send_err(job, ee::error_package_mode_MANAGE_GROUP_FAIL, reason);
    return;
}

// 发一个 msg_data_package 包
//  message 内容是 GroupID
void request_join_group(ChatProto::data &Data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::group_relation gr;
    Data.details()[0].UnpackTo(&gr);
    select_sql(X, "select tgt2 from relationship where relationship.tgt1 = " << gr.master().id()
                                                                             << " and relationship.stat > 1");
    if (X->OK == false || X->rows_size == 0)
    {
        send_err(job, ee::error_package_mode_REQUEST_JOIN_GROUP_FAIL,
                 "<" + std::to_string(gr.master().id()) + "> group not found");
    }
    else
    {
        while (1)
        {
            auto result = X->get_value();
            if (result == nullptr)
            {
                break;
            }
            auto ptr = find_userMap(std::stol(result[0])).lock();
            if (ptr != nullptr)
                send_data_package(ptr, dd::data_data_mode_REQUEST_JOIN_GROUP, gr);
            do_sql("insert into notify(receiver_id , mode,data,time) values ("
                   << result[0] << "," << dd::data_data_mode_REQUEST_JOIN_GROUP << ",'" << Data.SerializeAsString()
                   << "','" << Data.timestamp() << "')");
        }
        send_data_package(job, dd::data_data_mode_REQUEST_JOIN_GROUP_OK);
    }
    return;
}

// 需要client另起一个线程
// 文件下载 文件发送server
#include <functional>
extern std::function<std::string()> file_server_ip;
extern std::function<std::string()> file_server_port;

void send_file(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::file file_data;
    data.details()[0].UnpackTo(&file_data);
    file_data.set_ip(file_server_ip());
    file_data.set_port(file_server_port());
    send_data_package(job, dd::data_data_mode_SEND_FILE_OK, file_data);
}
void send_file_done(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::file file_data;
    data.details()[0].UnpackTo(&file_data);
    ChatProto::friend_relation relation;
    data.details()[1].UnpackTo(&relation);
    auto i = find_userMap(relation.follower().id()).lock();
    if (i != nullptr)
        send_data_package(i, dd::data_data_mode_SEND_FILE_DONE, data);

    do_sql("insert into notify(receiver_id , mode,data,time) values ("
           << relation.follower().id() << "," << dd::data_data_mode_SEND_FILE_DONE << ",'" << data.SerializeAsString()
           << "','" << data.timestamp() << "')");

    // file_data.set_ip(ftp_server_ip);
    // file_data.set_port(ftp_server_port);
    send_data_package(job, dd::data_data_mode_SEND_FILE_DONE_OK, file_data);
}
void recv_file(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::file file_data;
    data.details()[0].UnpackTo(&file_data);
    file_data.set_ip(file_server_ip());
    file_data.set_port(file_server_port());
    send_data_package(job, dd::data_data_mode_RECV_FILE_OK, file_data);
}

void log_out(std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::user_data_package user_data;
    user_data.set_id(job->ID);
    user_data.set_name(job->name);
    select_sql(X, "SELECT id FROM online_users , relationship WHERE relationship.tgt1 = "
                      << job->ID
                      << " AND relationship.tgt2 = online_users.id AND "
                         "relationship.chatid < 'g'");
    if (X->OK == false)
    {
        LOG(WARNING) << " SQL FALSE " << X->command;
    }
    else
    {
        while (1)
        {
            auto row = X->get_value();
            if (row == NULL)
                break;
            else
            {
                auto ptr = find_userMap(std::stol(row[0])).lock();
                if (ptr != nullptr)
                {
                    send_data_package(ptr, dd::data_data_mode_FRIEND_LOG_OUT, user_data);
                }
            }
        }
    }
    erase_userMap(job, sql);
}

void ignore_friend(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::friend_relation fr;
    data.details()[0].UnpackTo(&fr);
    fr.set_status(-1);
    auto ptr = find_userMap(fr.follower().id()).lock();
    if (ptr != nullptr)
        send_data_package(ptr, dd::data_data_mode_IGNORE_FRIEND, fr);
    do_sql("insert into notify(receiver_id,mode,data,time) values("
           << fr.follower().id() << "," << dd::data_data_mode_IGNORE_FRIEND << ",'" << data.SerializeAsString() << "','"
           << data.timestamp() << "')");
    do_sql("UPDATE relationship SET stat = -1 WHERE tgt1 = " << fr.follower().id() << " AND tgt2 = " << fr.master().id()
                                                             << " AND chatid < 'g' ");
    send_data_package(job, dd::data_data_mode_IGNORE_FRIEND_OK);
}
void unignore_friend(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::friend_relation fr;
    data.details()[0].UnpackTo(&fr);
    fr.set_status(1);
    auto ptr = find_userMap(fr.follower().id()).lock();
    if (ptr != nullptr)
        send_data_package(ptr, dd::data_data_mode_UNIGNORE_FRIEND, fr);
    do_sql("insert into notify(receiver_id,mode,data,time) values("
           << fr.follower().id() << "," << dd::data_data_mode_UNIGNORE_FRIEND << ",'" << data.SerializeAsString()
           << "','" << data.timestamp() << "')");
    do_sql("UPDATE relationship SET stat = 1 WHERE tgt1 = " << fr.follower().id() << " AND tgt2 = " << fr.master().id()
                                                            << " AND chatid < 'g' ");
    send_data_package(job, dd::data_data_mode_UNIGNORE_FRIEND_OK);
}
void update_data(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::user_data_package user;
    data.details()[0].UnpackTo(&user);
    std::string timestamp = data.timestamp();
    if (timestamp.empty())
        timestamp = std::move(Timestamp::now());
    std::shared_ptr<ChatProto::data> dataa = std::make_shared<ChatProto::data>();
    dataa->set_action(dd::data_data_mode_UPDATE_DATA_OK);

    select_sql(X, "SELECT chathistory.speaker_id, chathistory.speaker_name, "
                  "chathistory.chatid, chathistory.message, "
                  "chathistory.messagetime "
                  "FROM chathistory "
                  "JOIN (SELECT chatid FROM relationship WHERE tgt1 = "
                      << user.id()
                      << ") AS relation ON relation.chatid = chathistory.chatid "
                         "WHERE chathistory.messagetime >= '"
                      << timestamp << "' ORDER BY chathistory.chatid, chathistory.messagetime ");
    if (X->OK == false)
    {
        send_err(job, ee::error_package_mode_UPDATE_DATA_FAIL, "select failed");
        LOG(ERROR) << "SQL FAISE  " << X->command;
        return;
    }
    // row[0] speaker_id  //row[1] speaker_name  //row[2] chatid;  //row[3]
    // message  //row[4] message_time

    ChatProto::chatHistory_data_package dp;
    while (1)
    {
        auto row = X->get_value();
        if (row == nullptr)
        {
            if (!dp.chatid().empty())
            {
                dataa->add_details()->PackFrom(dp);
            }
            break;
        }
        if (dp.chatid() != row[2])
        {
            if (!dp.chatid().empty())
            {
                dataa->add_details()->PackFrom(dp);
                dp.Clear();
            }
            dp.set_chatid(row[2]);
        }
        auto new_msg = dp.add_messages();
        new_msg->set_message(row[3]);
        new_msg->set_timestamp(row[4]);
        auto speaker = new ChatProto::user_data_package;
        speaker->set_name(row[1]);
        speaker->set_id(std::stol(row[0]));
        new_msg->set_allocated_speaker(speaker);
    }
    select_sql(notifys, "SELECT mode , data , time FROM notify WHERE receiver_id = " << user.id() << "  and time >= '"
                                                                                     << timestamp << "' ORDER BY time");
    if (notifys->OK == false)
    {
        send_err(job, ee::error_package_mode_UPDATE_DATA_FAIL, "select failed");
        LOG(ERROR) << "SQL FAISE  " << notifys->command;
        return;
    }
    else
    {
        ChatProto::data i;
        while (1)
        {
            auto row = notifys->get_value();
            if (row == nullptr)
                break;
            i.Clear();
            i.ParseFromString(row[1]);
            i.set_action((dd)(std::stol(row[0])));
            i.set_timestamp(row[2]);
            dataa->add_details()->PackFrom(i);
        }
    }
    Message_Package<ChatProto::data> msg(dataa);
    msg.send(job->fd);
}

// #include <iostream>
// #include <cryptopp/md5.h>
// #include <cryptopp/hex.h>
// std::string calculateMD5(const std::string& input) {
//     CryptoPP::Weak1::MD5 hash;
//     std::string digest;

//     CryptoPP::StringSource(input, true,
//         new CryptoPP::HashFilter(hash,
//             new CryptoPP::HexEncoder(
//                 new CryptoPP::StringSink(digest))));
//     return digest;
// }
#include <openssl/evp.h>
#include <openssl/md5.h>

#include <iomanip>
#include <iostream>
#include <sstream>

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

void load_user_relation_data(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    Message_Package<ChatProto::data> data_return;
    ChatProto::user_data_package user_data;
    data.details()[0].UnpackTo(&user_data);
    {
        select_sql(result, "SELECT  relationship.tgt2,  users.name,  users.email,  "
                           "relationship.stat,  relationship.chatid  FROM  relationship  INNER "
                           "JOIN users ON users.id = relationship.tgt2  WHERE  relationship.tgt1 "
                           "= " << user_data.id()
                                << " and relationship.chatid < 'g'"
                                   " ORDER BY  users.name ");
        if (!result->OK)
        {
            send_err(job, ee::error_package_mode_LOAD_USER_RELATION_DATA_FAIL, "sql error");
            LOG(ERROR) << "sql error  " << result->command;
            return;
        }
        else
        {
            ChatProto::friend_relation fr;
            ChatProto::user_data_package follower;
            while (1)
            {
                auto row = result->get_value();
                if (row == nullptr)
                    break;
                fr.set_chatid(row[4]);
                fr.set_status(std::stoi(row[3]));
                auto nf = follower.New();
                nf->set_email(row[2]);
                nf->set_id(std::stoi(row[0]));
                nf->set_name(row[1]);
                fr.set_allocated_follower(nf);
                data_return.data->add_details()->PackFrom(fr);
            }
        }
    }
    {
        select_sql(result, "SELECT us.user_set_id, us.name, r.stat, u.id, u.name, "
                           "us.chatid FROM users AS u INNER JOIN ( SELECT tgt1 "
                           "FROM relationship WHERE tgt2 = "
                               << user_data.id()
                               << " and chatid > 'g' ) AS group_ids "
                                  "INNER JOIN relationship AS r ON r.tgt1 = group_ids.tgt1 AND chatid > 'g' "
                                  "INNER JOIN "
                                  "user_set AS us ON us.user_set_id = group_ids.tgt1 WHERE u.id = "
                                  "r.tgt2 ORDER BY us.user_set_id, r.stat DESC, u.name ");
        if (!result->OK)
        {
            send_err(job, ee::error_package_mode_LOAD_USER_RELATION_DATA_FAIL, "sql error");
            LOG(ERROR) << "sql error  " << result->command;
            return;
        }
        else
        {
            ChatProto::group_relation gr;
            // ChatProto::user_data_package u;
            while (1)
            {
                auto row = result->get_value();
                if (row == nullptr)
                    break;
                if (std::string(row[0]) != gr.id())
                {
                    if (!gr.id().empty())
                    {
                        data_return.data->add_details()->PackFrom(gr);
                        gr.Clear();
                    }
                    gr.set_id(row[0]);
                    gr.set_name(row[1]);
                    gr.set_chatid(row[5]);
                }
                if (strcmp(row[2], "3") == 0)
                {
                    auto u = gr.mutable_owner();
                    u->set_id(std::stol(row[3]));
                    u->set_name(row[4]);
                }
                else if (strcmp(row[2], "2") == 0)
                {
                    auto m = gr.add_manager();
                    m->set_id(std::stol(row[3]));
                    m->set_name(row[4]);
                }
                else if (strcmp(row[2], "1") == 0)
                {
                    auto m = gr.add_member();
                    m->set_id(std::stol(row[3]));
                    m->set_name(row[4]);
                }
                if (std::stol(row[3]) == user_data.id())
                {
                    gr.set_status(std::stol(row[2]));
                }
            }
            if (!gr.id().empty())
                data_return.data->add_details()->PackFrom(gr);
        }
    }
    data_return.data->set_action(dd::data_data_mode_LOAD_USER_RELATION_DATA_OK);
    data_return.data->set_timestamp(data.timestamp());
    data_return.send(job->fd);
}

void get_online_friend(ChatProto::data &data, std::shared_ptr<task> job, mysql::Mysql &sql)
{
    ChatProto::user_data_package user_data;
    data.details()[0].UnpackTo(&user_data);
    Message_Package<ChatProto::data> data_return;
    {
        select_sql(X, "select online_users.id, online_users.name from online_users, ( select "
                      "tgt2 FROM relationship where tgt1 = "
                          << user_data.id()
                          << " and chatid < 'g') as friend_ids WHERE online_users.id = friend_ids.tgt2 ORDER "
                             "BY online_users.name") if (X->OK == false)
        {
            LOG(ERROR) << " sql false " << X->command;
            send_err(job, ee::error_package_mode_GET_ONLINE_FRIENDS_FAIL, "query failed");
        }
        else
        {
            ChatProto::user_data_package user;
            while (1)
            {
                auto row = X->get_value();
                if (row == nullptr)
                {
                    data_return.data->set_action(dd::data_data_mode_GET_ONLINE_FRIENDS);
                    break;
                }
                user.set_id(std::stol(row[0]));
                user.set_name(row[1]);
                data_return.data->add_details()->PackFrom(user);
            }
        }
    }
    data_return.send(job->fd);
}