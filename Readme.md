# ChatRoom
    由C++编写 使用stdcxx11特性 
    因三方库 使用stdcxx17编译

---

## Required
- **Linux environment**
  在`linux`环境下的终端聊天室程序
- **Protocol buffer**
  使用`protobuf`作为协议 详见:>`./chat.proto`
- **openssl**
  在文件传输时用以校验
- **MySQL C api**
  使用`MySql`作为数据库 
- **glog**
  使用`glog`作为日志
- **Cmake+Makefile**
  以来由`Cmake`进行分析 由makefile进行编译
- **nlohmann-jsonPraser**
  使用`nlohmann-json`进行`./config.json`的解析

---

## 如何进行使用
在使用时可以更改config.json
```json
{
    "listen_ip": "127.0.0.1",
    "listen_port": "55175",
    "timeout_limit_times": 30000,
    "buffer_size": 4096,
    "worker_thread_number": 9,
    "Server_log_path": "/path/to/log/directory",
    "fileServer": {
        "ip": "127.0.0.1",
        "port": "59274",
        "file_server_thread_nums": 4,
        "file_server_log_path": "/path/to/log/directory"
    },
    "Mysql_server_ip": "127.0.0.1",
    "Mysql_user_name": "root",
    "Mysql_user_password": "password"
}

```
server 使用多线程进行
可以更改上述参数进行修改 
使用时需要将config.json和可执行文件放置在同一目录

---

## 细节
### server

![chatroom _1_.png](https://s2.loli.net/2023/08/17/mzLcxhGRVZSPi4A.png)
如何上所示

1. 使用epoll作为事件分发器 通过消息队列对工作线程进认为分发
2. 通过数据包的反序列化进行获得任务 对数据库进行操作和对在线用户进行数据包的转发
3. 通过定时器 对config中设置的timeout时间内无数据交流的链接进行断开
4. 主线程和工作线程的临界区在于`task queue` 通过条件变量进行同步
5. server 只进行数据转发 和 数据更新
6. epoll 将为ET触发模式进行高并发处理


---
### file server
![chatroom _2_.png](https://s2.loli.net/2023/08/17/WjOv7Yi8DhSdBJI.png)
如何上所示
1. 在链接`file-server`时 client 将保持原来的链接
2. 文件的收发将异步进行 通过epoll进行监控
3. 使用epoll作为事件分发器 通过消息队列对工作线程进认为分发
4. 使用epoll监控listener 进行及时的链接
5. 通过数据包的反序列化进行获得任务 对数据库进行操作和对在线用户进行数据包的转发
6. 主线程和工作线程的临界区在于`task queue` 通过条件变量进行同步
---
### client
![chatroom _1_ _1_.png](https://s2.loli.net/2023/08/18/jhmqFIu24drs187.png)
如上所示
1. client和本地交互 
   - 记录用户的基本信息和时间戳
   - 记录聊天历史记录
   - 记录通知/事件
2. client 单线程进行 使用select进行IO复用
   - 监控标准输入和server链接描述符
3. client 为类命令控制程序 
   - 通过解析用户输入 进行程序控制和聊天

---
## 目录结构
```shell
$ tree
./
├── chat.proto
├── config.json
├── MySQLtables.sql
├── CMakeLists.txt
├── file_prase.cc
├── proto.cc
├── proto.hh
├── client/
│   ├── Cache&Log.cc
│   ├── Cache&Log.hh
│   ├── CMakeLists.txt
│   ├── command.cc
│   ├── command.hh
│   ├── init.cc
│   ├── main.cc
│   ├── notification.cc
│   └── notification.hh
└── server/
    ├── ftp/
    │   ├── CMakeLists.txt
    │   ├── init.cc
    │   ├── main.cc
    │   └── test.cc
    └── worker/
        ├── CMakeLists.txt
        ├── epoll.cc
        ├── epoll.hh
        ├── init.cc
        ├── listener.cc
        ├── listener.hh
        ├── main.cc
        ├── sql.cc
        ├── sql.hh
        ├── task.cc
        ├── task.hh
        ├── threadpool.cc
        ├── threadpool.hh
        ├── worker.cc
        └── worker.hh
```

---
## 演示
聊天

```shell

----------------------------------------------------------------------------------------------------------------------------------------------------------
[2023-08-01 21:00:00.000000] Yarbor 
Welcome to chatroom

[2023-08-18 21:01:00.999999] yennyart 
hello my friend

----------------------------------------------------------------------------------------------------------------------------------------------------------
Hi im ...[expect to your Enter]

```
命令
```shell

----------------------------------------------------------------------------------------------------------------------------------------------------------
: show -a
----------------------------------------------------------------------------------------------------------------------------------------------------------
id: 100001
name: "YarBor"
email: "yarbor@foxmail.com"

friend data :
Bob [100002](offline)
yennyart [100003](online)

group data :
job_group [432305] 
{
	Bob [100002][owner] 
		Yarbor [100001][manager] 
		J7k [149370][member] 
}
pioneersHome [3] 
{
	Yarbor [100001][owner] 
}
----------------------------------------------------------------------------------------------------------------------------------------------------------

```