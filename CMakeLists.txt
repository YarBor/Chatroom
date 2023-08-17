project(Network)

cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
SET(CMAKE_CXX_STANDARD 17)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 ")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -fsanitize=address")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 ")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O2 -g -Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2 -g")
SET(CMAKE_BUILE_TYPE "Debug")
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# SET(PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/NewIM.proto)
# SET(GRPC_CPP_PLUGIN_PATH /usr/bin/grpc_cpp_plugin)

# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")
#     set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
# endif()


# PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS NewIM.proto)
# PROTOBUF_GENERATE_CPP(GRPC_SRCS GRPC_HDRS NewIM.proto PLUGIN protoc-gen-grpc=/usr/bin/grpc_cpp_plugin)
# PROTOBUF_GENERATE_GRPC_CPP(GRPC_SRCS GRPC_HDRS NewIM.proto)


include_directories(
        # ${CMAKE_CURRENT_SOURCE_DIR}
        # ${CMAKE_CURRENT_BINARY_DIR}
        # ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTO_HDRS}
)

include_directories(${PROTOBUF_INCLUDE_DIRS})
# CMakeLists.txt

# 添加子目录

add_executable(
        fS

        chat.pb.cc
        proto.cc
        server/ftp/main.cc
        server/ftp/init.cc
)
add_executable(
        P
        chat.pb.cc
        file_prase.cc
)
# add_executable(init )
# add_executable(Proto  chat.pb.cc)
# add_executable(server.out NetworkTest.grpc.pb.cc NetworkTest.pb.cc test_server.cpp)
target_link_libraries(
        P
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc
        gRPC::grpc++
)
target_link_libraries(
        fS
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc
        gRPC::grpc++
        -lglog
        -lssl
        -lcrypto
)
# 在这里添加其他构建规则
add_executable(
        S

        server/worker/epoll.cc
        server/worker/init.cc
        server/worker/listener.cc
        server/worker/main.cc
        server/worker/sql.cc
        server/worker/task.cc
        server/worker/threadpool.cc
        server/worker/worker.cc
        chat.pb.cc
        proto.cc

)
# add_executable(init )
# add_executable(Proto  chat.pb.cc)
# add_executable(server.out NetworkTest.grpc.pb.cc NetworkTest.pb.cc test_server.cpp)

target_link_libraries(
        S

        ${PROTOBUF_LIBRARIES}
        gRPC::grpc
        gRPC::grpc++
        -lglog
        -lmysqlclient
        -lssl
        -lcrypto
)
#        # server.out
#        # ${PROTOBUF_LIBRARIES}
#        # gRPC::grpc
#        # gRPC::grpc++
#)

add_executable(
        C

        client/main.cc
        client/Cache&Log.cc
        client/command.cc
        client/init.cc
        client/notification.cc
        chat.pb.cc
        proto.cc
)
# add_executable(init )
# add_executable(Proto  chat.pb.cc)
# add_executable(server.out NetworkTest.grpc.pb.cc NetworkTest.pb.cc test_server.cpp)

target_link_libraries(
        C

        ${PROTOBUF_LIBRARIES}
        gRPC::grpc
        gRPC::grpc++
        -lglog
        -lssl
        -lcrypto
)
