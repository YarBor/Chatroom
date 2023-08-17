#pragma once
#include <mysql/mysql.h>

#include <iostream>
#include <memory>
#include <string>
extern size_t SQL_READ_BUFFER_SIZE;
namespace mysql
{
class query_resut
{
    friend class Mysql;

  public:
    ~query_resut()
    {
        // std::cout << "~query_resut() " << std::endl;
        mysql_free_result(result);
    }
    query_resut(MYSQL_RES *rlt, std::string command)
        : result(rlt), command(std::move(command)), rows_size(rlt == nullptr ? 0 : mysql_num_rows(rlt))
    // OK(rlt == nullptr ? false : true)
    {
        // std::cout << rlt << std::endl;
    }

    MYSQL_ROW get_value()
    {
        row = mysql_fetch_row(result);
        return row;
    }
    std::string command;
    uint rows_size;
    MYSQL_ROW row;
    MYSQL_RES *result;
    // OK 在创建时 侵入 初始化
    bool OK = false;
};

class Mysql
{
  public:
    Mysql();
    Mysql operator=(Mysql &) = delete;
    ~Mysql()
    {
        if (mysql != NULL) // 关闭MySQL连接
            mysql_close(mysql);
        if (buffer != NULL) // 释放缓冲区内存
            free(buffer);
    }
    std::shared_ptr<query_resut> operator()(std::string const &command);
    std::shared_ptr<query_resut> operator()(std::string &&command);
    std::string get_last_insert_id();
    std::string escapeString(std::string const &str);
    MYSQL *sql()
    {
        return this->mysql;
    }

  private:
    size_t buffer_size = SQL_READ_BUFFER_SIZE;
    MYSQL *mysql = NULL; // MySQL连接对象指针
    char *buffer = nullptr;
};

} // namespace mysql
