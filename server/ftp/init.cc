#include <fstream>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

std::string file_server_ip;
std::string file_server_port;
int file_server_thread_nums = 0;
timespec convertToTimespec(double seconds);
uint BUFFER_SIZE = 1024;
void init()
{
    using nlohmann::json;
    using namespace std;
    try
    {
        // memset(&timeout_limit_times, 0, sizeof(itimerspec));
        std::ifstream file("config.json");
        if (!file.is_open())
        {
            perror("config.json is not open");
            exit(1);
        }
        json config = json::parse(file);
        file.close();
        auto i = config["fileServer"].get<json>();
        file_server_ip = i["ip"].get<string>();
        file_server_port = i["port"].get<string>();
        file_server_thread_nums = i["file_server_thread_nums"].get<int>();
        BUFFER_SIZE = config["buffer_size"].get<uint>();
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "config.json Prase False" << std::endl;
        exit(EXIT_FAILURE);
    }
}
timespec convertToTimespec(double seconds)
{
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
    return std::move(ts);
}
