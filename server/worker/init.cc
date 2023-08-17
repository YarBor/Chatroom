#include <fstream>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

uint TIME_OUT_LIMIT = 3;
std::string WORK_PROGRAM_NAME = "WORK_PROGRAM_NAME";
uint MAX_CANONICAL_PROCESS_NUM = 3;
uint PROCESS_THRESHOLD = 2000;

struct itimerspec timeout_limit_times;
std::string LISTEN_IP;
std::string LISTEN_PORT;
int threadNumber = 5;
uint BUFFER_SIZE = 1024;

std::function<std::string()> file_server_port([]() -> std::string {
    try
    {
        std::ifstream file("config.json");
        if (!file.is_open())
        {
            perror("config.json is not open");
            exit(1);
        }
        auto i = nlohmann::json::parse(file);
        file.close();
        return i["fileServer"]["port"].get<std::string>();
    }
    catch (...)
    {
        return "";
    }
});

std::function<std::string()> file_server_ip([]() -> std::string {
    try
    {
        std::ifstream file("config.json");
        if (!file.is_open())
        {
            perror("config.json is not open");
            exit(1);
        }
        auto i = nlohmann::json::parse(file);
        file.close();
        return i["fileServer"]["ip"].get<std::string>();
    }
    catch (...)
    {
        return "";
    }
});

timespec convertToTimespec(double seconds);

void init()
{
    using nlohmann::json;
    using namespace std;
    try
    {
        memset(&timeout_limit_times, 0, sizeof(itimerspec));
        std::ifstream file("config.json");
        if (!file.is_open())
        {
            perror("config.json is not open");
            exit(1);
        }
        json config = json::parse(file);
        file.close();

        LISTEN_IP = config["listen_ip"].get<string>();
        LISTEN_PORT = config["listen_port"].get<string>();
        // PROCESS_COMMUNTCTION_PORT = config["process_communication_port"].get<string>();
        timeout_limit_times.it_value = std::move(convertToTimespec(config["timeout_limit_times"].get<double>()));
        BUFFER_SIZE = config["buffer_size"].get<int>();
        threadNumber = config["worker_thread_number"].get<int>();
        // multi_Progress = config["multi-progress"].get<bool>();
        // auto i = config["fileServer"].get<json>();
        // file_server_ip = i["ip"].get<string>();
        // file_server_port = i["port"].get<string>();
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
