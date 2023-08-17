#include "listener.hh"
#include <ifaddrs.h>
// #include <netinet/in.h>
extern bool multi_Progress;
extern std::string LISTEN_IP;
extern std::string LISTEN_PORT;
#include <netdb.h>
std::string get_local_ipv4();

Listener::Listener(std::string IP, std::string port) : IP_(IP), port_(port)
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        LOG(ERROR) << "Failed to create socket.  " << strerror(errno) << std::endl;
        return;
    }
    // 设置服务器地址和端口
    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_addr.s_addr = inet_addr(IP_.c_str());
    serverAddress_.sin_port = htons(std::stoi(port_));
    // 绑定socket到服务器地址和端口
    if (bind(socket_fd, (struct sockaddr *)&serverAddress_, sizeof(serverAddress_)) < 0)
    {
        std::cout << IP << ": " << port_ << std::endl;
        LOG(ERROR) << "Failed to bind socket to address and port." << strerror(errno) << std::endl;
        return;
    }
    if (port == "0")
    {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        if (getsockname(socket_fd, (struct sockaddr *)&addr, &addr_len) == -1)
        {
            perror("getsockname");
            exit(1);
        }
        port_ = std::to_string(ntohs(addr.sin_port));
        // printf("监听在随机端口：%d\n", port);
    }

    if (listen(socket_fd, 100) < 0)
    {
        LOG(ERROR) << "Failed to start listening for connections." << std::endl;
        return;
    }
    OK = true;
}

Listener::Listener()
    : Listener(  LISTEN_IP,  LISTEN_PORT)
{
}

#include <fcntl.h>
int Listener::Accept()
{
    auto a = accept(socket_fd, NULL, NULL);
    fcntl(a, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK);
    if (a == -1)
        perror("accept");
    else
        puts("Accepted");
    return a;
}

Listener::~Listener()
{
    // 关闭socket
    close(socket_fd);
}

std::string get_local_ipv4()
{
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    std::string ipv4;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
            continue;

        int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if (s != 0)
        {
            perror("getnameinfo");
            exit(EXIT_FAILURE);
        }

        if (strcmp(ifa->ifa_name, "lo") != 0)
        {
            ipv4 = std::string(host);
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ipv4;
}