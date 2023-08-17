#include <fstream>
#include <iostream>
#include <string>

#include "nlohmann/json.hpp"

uint TIME_OUT_LIMIT = 3000;
std::string WORK_PROGRAM_NAME = "WORK_PROGRAM_NAME";
uint MAX_CANONICAL_PROCESS_NUM = 3;
uint PROCESS_THRESHOLD = 2000;

struct itimerspec timeout_limit_times;
std::string LISTEN_IP;
std::string LISTEN_PORT;
int threadNumber = 5;
uint BUFFER_SIZE = 1024;

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
        threadNumber = config["worker_thread_number"];
    }
    catch (const nlohmann::json::exception &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "config.json Prase False" << std::endl;
        exit(EXIT_FAILURE);
    }

    // char arr[1023] = {0};
    // std::string str(arr,strlen(arr));
}
timespec convertToTimespec(double seconds)
{
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
    return std::move(ts);
}
