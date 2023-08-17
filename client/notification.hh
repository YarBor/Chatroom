#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <list>
#include <queue>
#include <string>
#include <vector>

#include "../chat.pb.h"
#include "../proto.hh"
#include "Cache&Log.hh"

extern std::shared_ptr<ChatProto::user_data_package> user_data;

std::string calculateMD5(const std::string &input);
struct notification
{
    notification(google::protobuf::Any const &noti);
    notification(std::shared_ptr<ChatProto::data> notify);
    bool operator==(const notification &other);
    bool operator<(const notification &other);
    std::shared_ptr<ChatProto::data> notify;
    Timestamp timestamp;
    uint mode;
    bool is_done;
    std::string target_id;
};
class notifications
{
  public:
    notifications();
    void update();
    void add_notification(notification const &ntf);
    std::deque<notification> &get();
    void show_last_one(int fd);
    void show_all(int fd);
    void add_notification_and_show(int outputFd, notification const &ntf);
    void show_notify(int fd, const notification &n);

  private:
    std::deque<notification> nq;
    std::mutex ntfmutex;
    // std::deque<ChatProto::user_data_package> requeast_make_friend;
    // std::deque<ChatProto::user_data_package> requeast_join_group;
    bool flush_to_file();
    void init();
};

#include <stdio.h>

#include <ctime>

bool isTimeDifferenceGreaterThanThreeDays(const std::string &timestamp1,
                                          const std::string &timestamp2); // //for test
                                                                          // #include <iostream>
                                                                          // int main(){
//     std::cout << isTimeDifferenceGreaterThanThreeDays("2000-01-01
//     00:00:00.000000","2000-01-02 00:00:00.000000") << std::endl;
// }
