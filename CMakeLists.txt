CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
project(MyTinyWebServer)

set(CMAKE_CXX_STANDARD 14)


find_package(Threads)

add_executable(server src/main.cpp src/buffer/Buffer.cpp src/log/Log.cpp src/pool/SqlConnPool.cpp src/pool/sqlconnRAII.h
        src/server/epoller.cpp src/http/HttpConn.cpp src/http/HttpRequest.cpp src/http/HttpResponse.cpp src/timer/Timer.cpp src/server/WebServer.cpp)
target_link_libraries (server ${CMAKE_THREAD_LIBS_INIT})