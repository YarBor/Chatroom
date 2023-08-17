#pragma once

#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../proto.hh"
#include "Cache&Log.hh"
#include "notification.hh"
extern std::weak_ptr<ChatProto::friend_relation> chat_fr;
extern std::weak_ptr<ChatProto::group_relation> chat_gr;
using dd = ChatProto::data_data_mode;
extern uint lines;
bool link(int &);

class inputPraser
{
public:
  inputPraser();
  ~inputPraser();
  inputPraser &operator=(inputPraser const &) = delete;
  void Get_input(int input_fd);
  const std::vector<std::string> &Commands() const;
  bool is_command() const;
  const std::string &input() const;

private:
  std::string input_;
  bool is_commands = false;
  std::vector<std::string> commands;
  uint buffer_size = 1024;
  char *buffer;
  std::vector<std::string> splitString(std::string &input_);
};

// bool do_command(int fd ,const std::vector<std::string> &commands);
bool do_command(int fd, int &skfd, const std::vector<std::string> &commands, Cache *cache, history_show *hss,
                notifications *notifications_mode);
std::string calculateFileMD5(int fd);
