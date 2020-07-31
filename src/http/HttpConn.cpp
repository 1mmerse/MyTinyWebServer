//
// Created by 杨成进 on 2020/7/27.
//

#include "HttpConn.h"

//定义静态成员！！！
bool HttpConn::isET;
const char *HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::init(int sockFd, const sockaddr_in &addr) {
    assert(sockFd > 0);
    userCount++;
    addr_ = addr;
    fd_ = sockFd;
    reset();
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), static_cast<int>(userCount));
}

void HttpConn::reset() {
    recvBuff_.RetrieveAll();
    sendBuff_.RetrieveAll();
    isClose_ = false;
    request_.Init();
}

ssize_t HttpConn::read(int *savaErrno) {
    ssize_t len = -1;
    do {
        len = recvBuff_.ReadFd(fd_, savaErrno);
        if (len <= 0)
            break;
    } while (isET);
    return len;
}

ssize_t HttpConn::write(int *savaErrno) {
    ssize_t len = -1;
    do {
        //ssize_t writev(int fd, const struct iovec *vector, int count);
        //将多块分散的内存一并写入文件描述符中
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *savaErrno = errno;
            break;
        }
        //传输结束
        if (iov_[0].iov_len + iov_[1].iov_len == 0) break;
            //iov[0]读完
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t *) iov_[1].iov_base + len - iov_[0].iov_len;
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            //回收iov[0]
            if (iov_[0].iov_len) {
                sendBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
            //iov[0]没有读完
        else {
            iov_[0].iov_base = (uint8_t *) iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            sendBuff_.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

void HttpConn::Close() {
    response_.UnmapFile();
    if (!isClose_) {
        isClose_ = true;
        userCount--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), static_cast<int>(userCount));
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

const char *HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

void HttpConn::process() {
    if (request_.parse(recvBuff_)) {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else
        response_.Init(srcDir, request_.path(), false, 400);
    response_.MakeResponse(sendBuff_);
    //响应头
    iov_[0].iov_base = const_cast<char *>(sendBuff_.Peek());
    iov_[0].iov_len = sendBuff_.ReadableBytes();
    iovCnt_ = 1;
    //响应文件
    if (response_.Filelen() > 0 && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.Filelen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("file size:%d, %d to %d", response_.Filelen(), iovCnt_, ToWriteBytes());


}
