//
// Created by 杨成进 on 2020/7/30.
//

#include "WebServer.h"

WebServer::WebServer(int port, int trigMode, int timeoutMs, bool OptLinger, int sqlPort, const char *sqlUser,
                     const char *sqlPwd, const char *dbName, int connPoolNum, int threadNum, bool openLog, int logLevel,
                     int logQueSize) :
        port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMs), isClose_(false),
        timer_(new Timer()), threadPool_(new ThreadPool(threadNum)), epoller_(new Epoller()) {
    /*
     * 函数原型：char *getcwd( char *buffer, int maxlen );
     * 参数说明：getcwd()会将当前工作目录的绝对路径复制到参数buffer所指的内存空间中,参数maxlen为buffer的空间大小。
     * 返 回 值：成功则返回当前工作目录，失败返回 FALSE。
     */
    srcDir_ = getcwd(nullptr, 256);
    assert(srcDir_);
    strncat(srcDir_, "/resources/", 16);
    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir_;
    SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
    InitEventMode_(trigMode);
    if (!InitSocket_())
        isClose_ = true;

    if (openLog) {
        Log::Instance()->Init(logLevel, "./log", ".log", logQueSize);
        if (isClose_) {
            LOG_ERROR("server init error!");
        } else {
            LOG_INFO("Server INIT");
        }
    }


}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnPool::Instance()->closePool();
}

void WebServer::start() {
    int timeMS = -1;
    if (!isClose_)
        LOG_INFO("Server Start");
    while (!isClose_) {
        if (timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        int eventCnt = epoller_->Wait(timeMS);
        for (int i = 0; i < eventCnt; ++i) {
            //处理时间
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if (fd == listenFd_)
                DealListen_();
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            } else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            } else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            } else
                LOG_ERROR("Unexpect event!");
        }
    }
}

bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("port:%d error", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
//    struct linger {
//        　　int l_onoff;
//        　　int l_linger;
//    };
//    三种断开方式：
//    1. l_onoff = 0; l_linger忽略
//    close()立刻返回，底层会将未发送完的数据发送完成后再释放资源，即优雅退出。
//    2. l_onoff != 0; l_linger = 0;
//    close()立刻返回，但不会发送未发送完成的数据，而是通过一个REST包强制的关闭socket描述符，即强制退出。
//    3. l_onoff != 0; l_linger > 0;
//    close()不会立刻返回，内核会延迟一段时间，这个时间就由l_linger的值来决定。
//    如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，close()会返回正确，socket描述符优雅性退出。
//    否则，close()会直接返回错误值，未发送数据丢失，socket描述符被强制性退出。
//    需要注意的时，如果socket描述符被设置为非堵塞型，则close()会直接返回值。
    struct linger optLinger = {0};
    if (openLinger_) {
        //直到所有数据发送完毕或超时
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        LOG_ERROR("create socket error!");
        return false;
    }

    //设置延迟关闭
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!");
        return false;
    }
    int on = 1;
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void *)&on, sizeof(int));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("set reuseAddr error!");
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("bind port:%d error", port_);
        return false;
    }
    ret = listen(listenFd_, 6);
    if (ret < 0) {
        close(listenFd_);
        LOG_ERROR("listen port:%d error", port_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if (ret == 0) {
        close(listenFd_);
        LOG_ERROR("add listen error");
        return false;
    }

    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

void WebServer::InitEventMode_(int trigMod) {
//    EPOLLIN：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）
//    EPOLLOUT：表示对应的文件描述符可以写
//    EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）
//    EPOLLERR：表示对应的文件描述符发生错误
//    EPOLLHUP：表示对应的文件描述符被挂断；
//    EPOLLET：将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)而言的
//    EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里
    listenEvent_ = EPOLLRDHUP;
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMod) {
        case 0:
            break;
        case 1:
            connEvent_ |= EPOLLET;
            break;
        case 2:
            listenEvent_ |= EPOLLET;
            break;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

void WebServer::AddClient_(int fd, struct sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if (timeoutMS_ > 0)
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *) &addr, &len);
        if (fd < 0) break;
        else if (HttpConn::userCount >= MAX_FD) {
            SendError_(fd, "Sever busy!");
            LOG_WARN("Server full!");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);

}

void WebServer::DealWrite_(HttpConn *client) {
    assert(client);
    //活动连接，调整定时器堆上位置
    ExtentTime_(client);
    threadPool_->AddTask([this, client] { OnWrite_(client); });
}

void WebServer::DealRead_(HttpConn *client) {
    assert(client);
    ExtentTime_(client);
    threadPool_->AddTask([this, client] { OnRead_(client); });
}

void WebServer::SendError_(int fd, const char *info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
        LOG_WARN("send error to client[%d] error!", fd);
}

void WebServer::ExtentTime_(HttpConn *client) {
    assert(client);
    if (timeoutMS_ > 0)
        timer_->adjust(client->GetFd(), timeoutMS_);
}

void WebServer::CloseConn_(HttpConn *client) {
    assert(client);
    LOG_INFO("client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::OnRead_(HttpConn *client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    client->process();
    //将文件描述符变为可写
    epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
}

void WebServer::OnWrite_(HttpConn *client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    //传输完成
    if (client->ToWriteBytes() == 0) {
        if (client->IsKeepAlive()) {
            LOG_DEBUG("client[%d] Keep Alive!", client->GetFd());
            client->reset();
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}
