project(Network)

cmake_minimum_required(VERSION 3.15)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
SET(CMAKE_CXX_STANDARD 17)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -fsanitize=address")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g ")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O2  -Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -Og -g")
SET(CMAKE_BUILE_TYPE "Debug")
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# SET(PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/NewIM.proto)
# SET(GRPC_CPP_PLUGIN_PATH /usr/bin/grpc_cpp_plugin)


# PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS NewIM.proto)
# PROTOBUF_GENERATE_CPP(GRPC_SRCS GRPC_HDRS NewIM.proto PLUGIN protoc-gen-grpc=/usr/bin/grpc_cpp_plugin)
# PROTOBUF_GENERATE_GRPC_CPP(GRPC_SRCS GRPC_HDRS NewIM.proto)


include_directories(
        # ${CMAKE_CURRENT_SOURCE_DIR}
        # ${CMAKE_CURRENT_BINARY_DIR}
        # ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTO_HDRS}
        # ${GRPC_HDRS}
)

include_directories(${PROTOBUF_INCLUDE_DIRS})

add_executable(
        worker
        
        epoll.cc  
        init.cc  
        listener.cc  
        main.cc  
        sql.cc  
        task.cc  
        threadpool.cc
        worker.cc 
        ../../chat.pb.cc
        ../../proto.cc

)
# add_executable(init )
# add_executable(Proto  chat.pb.cc)
# add_executable(server.out NetworkTest.grpc.pb.cc NetworkTest.pb.cc test_server.cpp)

target_link_libraries(
        worker
        
        ${PROTOBUF_LIBRARIES}
        gRPC::grpc
        gRPC::grpc++
        -lglog
        -lmysqlclient
        -lssl
        -lcrypto
)
#target_link_libraries(
#        init.o
#        -lglog
#        # server.out
#       # ${PROTOBUF_LIBRARIES}
#       # gRPC::grpc
#       # gRPC::grpc++
#)
#
#
#
#target_link_libraries(
#        Proto
#        ${PROTOBUF_LIBRARIES}
#        gRPC::grpc
#        gRPC::grpc++
#)