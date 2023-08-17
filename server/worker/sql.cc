#include "sql.hh"
size_t SQL_READ_BUFFER_SIZE = 1024;
std::shared_ptr<mysql::query_resut> mysql::Mysql::operator()(std::string const &command)
{
    bool OK = mysql_query(mysql, command.c_str());
    if (OK == 0)
    {
        // puts("123");
        auto result = std::make_shared<query_resut>(mysql_store_result(mysql), std::move(command));
        result->OK = true;
        return result;
    }
    else
    {
        // puts("1123");
        auto result = std::make_shared<query_resut>(nullptr, std::move(command));
        result->OK = false;
        return result;
    }
}
std::shared_ptr<mysql::query_resut> mysql::Mysql::operator()(std::string &&command)
{
    bool OK = mysql_query(mysql, command.c_str());
    if (OK == 0)
    {
        // puts("123");
        auto result = std::make_shared<query_resut>(mysql_store_result(mysql), std::move(command));
        result->OK = true;
        return result;
    }
    else
    {
        // puts("1123");
        auto result = std::make_shared<query_resut>(nullptr, std::move(command));
        result->OK = false;
        return result;
    }
}
std::string mysql::Mysql::get_last_insert_id()
{
    mysql_query(mysql, "SELECT LAST_INSERT_ID()");
    auto tmp = mysql_store_result(mysql);
    std::string result(mysql_fetch_row(tmp)[0]);
    mysql_free_result(tmp);
    return std::move(result);
}

mysql::Mysql::Mysql()
{
    mysql = mysql_init(NULL); // 初始化MySQL连接对象
    if (mysql == NULL)        // 处理连接对象初始化失败的情况
    {
        fprintf(stderr, "Error initializing MySQL: %s\n", mysql_error(mysql));
        exit(1);
    }
    mysql = mysql_real_connect(mysql, Mysql_server_ip.c_str(), sql_user.c_str(), sql_user_password.c_str(), "chatroom", 0, NULL,
                               0); // 连接到指定的数据库
    if (mysql == NULL)             // 处理连接到数据库失败的情况
    {
        fprintf(stderr, "Error connecting to MySQL: %s\n", mysql_error(mysql));
        exit(1);
    }
    buffer = (char *)malloc(sizeof(char) * SQL_READ_BUFFER_SIZE); // 分配用于存储转义后字符串的缓冲区
    if (buffer == NULL)                                           // 处理分配缓冲区失败的情况
    {
        fprintf(stderr, "Error allocating memory for buffer.\n");
        exit(1);
    }
}

std::string mysql::Mysql::escapeString(std::string const &str)
{
    if ((str.size() * 2 + 1) > buffer_size) // 如果缓冲区大小不足以保存转义后的字符串
    {
        buffer_size *= (str.size() * 2 + 1) * 2;       // 扩大缓冲区大小
        buffer = (char *)realloc(buffer, buffer_size); // 重新分配内存
    }
    // 转义字符串并将结果存储到缓冲区中
    return std::move(std::string(buffer, mysql_real_escape_string(mysql, buffer, str.c_str(), str.length())));
}