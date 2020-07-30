//
// Created by 杨成进 on 2020/7/27.
//

#ifndef MYTINYWEBSERVER_HTTPCONN_H
#define MYTINYWEBSERVER_HTTPCONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <cstdlib>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../buffer/Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpConn {
public:
    HttpConn();

    ~HttpConn();

    void init(int sockFd, const sockaddr_in &addr);

    void reset();

    ssize_t read(int *savaErrno);

    ssize_t write(int *savaErrno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char *GetIP() const;

    sockaddr_in GetAddr() const;

    void process();

    int ToWriteBytes() {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }


    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
//    struct sockaddr_in {
//        short int sin_family;    //地址族(Address Family)
//        unsigned short int sin_port;    //16位 TCP/UDP端口号
//        struct in_addr sin_addr;    //32位 IP地址
//        char sin_zero[8];    //不使用,对齐作用
//    };
    struct sockaddr_in addr_;

    bool isClose_;

//    struct iovec {
//        ptr_t iov_base; /* Starting address */
//        size_t iov_len; /* Length in bytes */
//    };
    int iovCnt_;
    struct iovec iov_[2];

    Buffer recvBuff_;
    Buffer sendBuff_;

    HttpRequest request_;
    HttpResponse response_;
};


#endif //MYTINYWEBSERVER_HTTPCONN_H
