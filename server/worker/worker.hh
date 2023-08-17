#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <glog/logging.h>
#include <mysql/mysql.h>

#include <cassert>
#include <iostream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../../chat.pb.h"
#include "../../proto.hh"
#include "sql.hh"
#include "task.hh"

// extern std::map<uint64_t, std::weak_ptr<task>> tasksMap; // 登出的时候去erase
// extern std::mutex tasksMapLock;

bool do_Job(std::shared_ptr<task> job, mysql::Mysql &sql);
void log_out(std::shared_ptr<task> job, mysql::Mysql &sql);
