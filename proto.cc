
#include "proto.hh"

#include "chat.pb.h"
// #include "loadBalancing.hh"

class Timestamp;

bool Timestamp::validateString(const std::string &input)
{
    std::regex regex_pattern1(regular_expression1);
    std::regex regex_pattern2(regular_expression2);
    return std::regex_match(input, regex_pattern1) || std::regex_match(input, regex_pattern2);
}

Timestamp::Timestamp(std::string const &time)
{
    // if (validateString(time))
    //     data = std::move(time + (&(".000000"[(8 - (27 - time.length())) > 7 ? 7 : (8 - (27 - time.length()))])));
    // else
    data = std::move(now());
}
std::string Timestamp::cut()
{
    return data.substr(0, data.find("."));
}
Timestamp::Timestamp() : data(now())
{
}
bool Timestamp::operator==(const Timestamp &other) const
{
    return data == other.data;
}
bool Timestamp::operator<(const Timestamp &other) const
{
    return data < other.data;
}

std::string const &Timestamp::str()
{
    return data;
}

std::string Timestamp::now()
{
    static char timestamp[28];
    memset(timestamp, 0, 28);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t nowtime = tv.tv_sec;
    struct tm *nowtm = localtime(&nowtime);
    int microseconds = tv.tv_usec;
    strftime(timestamp, 28, "%Y-%m-%d %H:%M:%S", nowtm);
    sprintf(timestamp + strlen(timestamp), ".%06d", microseconds);
    timestamp[26] = '\0';
    return std::move(std::string(timestamp));
}

// using namespace std;
// int main() {
//   std::string ip = "127.0.0.1";
//   std::string port = "55555";
//   sockaddr_in serverAddress_;
//   serverAddress_.sin_family = AF_INET;
//   serverAddress_.sin_addr.s_addr = inet_addr(ip.c_str());
//   serverAddress_.sin_port = htons(std::stoi(port));

//   if (fork() == 0) {
//     int fd = socket(AF_INET, SOCK_STREAM, 0);
//     sleep(1);
//     connect(fd, (const sockaddr *)&serverAddress_, sizeof(serverAddress_));
//     Message_Package<ChatProto::Msg_data_package> msg;
//     int aa = 1;
//     while (1) {
//       msg.data->Clear();
//       msg.data->set_timestamp(msg.timestamp.str());
//       msg.data->set_message(std::to_string(aa++));
//       msg.send(fd);
//       sleep(1);
//     }

//   } else {
//     int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (socket_fd == -1) {
//       perror("socket");
//       return 0;
//     }
//     // 设置服务器地址和端口
//     // 绑定socket到服务器地址和端口
//     if (bind(socket_fd, (struct sockaddr *)&serverAddress_,
//              sizeof(serverAddress_)) < 0) {
//       perror("bind");
//       return 0;
//     }
//     if (listen(socket_fd, 5) < 0) {
//       perror("listen");
//       return 0;
//     }
//     int fd = accept(socket_fd, nullptr, nullptr);
//     Message_Package<ChatProto::Msg_data_package> msg;
//     while (1) {
//       msg.Recv(fd);
//       cout << msg.debug_string() << endl;
//     }
//   }
// }
std::string recv_prase_return_package(int &skfd)
{
    Message_Package<ChatProto::data> d;
    d.Recv(skfd);
    if (d.data->action() == ChatProto::data_data_mode::data_data_mode_false_)
    {
        ChatProto::error_package err;
        d.data->details()[0].UnpackTo(&err);
        return std::move(err.reason());
    }
    else
        return "";
}