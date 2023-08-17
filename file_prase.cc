#include <arpa/inet.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <sys/dir.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <condition_variable>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "chat.pb.h"
using namespace std;
int main()
{
    string data;
    char buffer[1024];
    while (1)
    {
        auto i = read(STDIN_FILENO, buffer, 1024);
        if (i == 0)
            break;
        else
            data.append(buffer, i);
    }
    ChatProto::data d;
    d.ParsePartialFromString(data);
    cout << d.DebugString() << std::endl;
}