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

using namespace std;
// std::string calculateFileMD5(int fd)
// {
//     lseek(fd, 0, SEEK_SET);
//     constexpr int BUFFER_SIZE = 4096;
//     unsigned char buffer[BUFFER_SIZE];
//     MD5_CTX md5Context;
//     MD5_Init(&md5Context);

//     ssize_t bytesRead;
//     while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0)
//     {
//         MD5_Update(&md5Context, buffer, bytesRead);
//     }

//     unsigned char hash[MD5_DIGEST_LENGTH];
//     MD5_Final(hash, &md5Context);

//     std::stringstream ss;
//     for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
//     {
//         ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
//     }
//     return ss.str();
// }
#include <openssl/evp.h>

std::string calculateFileMD5(std::fstream &fs)
{
    fs.seekg(0, std::ios::beg);
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    EVP_MD_CTX *ctx;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_md5(), NULL);

    char buffer[8192];
    while (!fs.eof())
    {
        fs.read(buffer, sizeof(buffer));
        std::streamsize bytes_read = fs.gcount();
        if (bytes_read > 0)
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

int main()
{
    auto i = open("./test.cc", O_RDONLY);
    cout << calculateFileMD5(i) << endl;
    close(i);
    fstream f("./test.cc", ios::in);
    cout << calculateFileMD5(f) << endl;
    f.close();
}
