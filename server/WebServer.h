//
// Created by 杨成进 on 2020/7/30.
//

#ifndef MYTINYWEBSERVER_WEBSERVER_H
#define MYTINYWEBSERVER_WEBSERVER_H

#include <unordered_map>
#include <memory>
#include <netinet/in.h>
#include <functional>
#include "../http/HttpConn.h"
#include "../timer/Timer.h"
#include "epoller.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeoutMs, bool OptLinger,
              int sqlPort, const char *sqlUser, const char *sqlPwd,
              const char *dbName, int connPoolNum, int ThreadNum,
              bool openLog, int LogLevel, int LogQueSize);

    ~WebServer();

    void start();

private:
    bool InitSocket_();

    void InitEventMode_(int trigMod);

    void AddClient_(int fd, struct sockaddr_in addr);

    void DealListen_();

    void DealWrite_(HttpConn *client);

    void DealRead_(HttpConn *client);

    static void SendError_(int fd, const char *info);

    void ExtentTime_(HttpConn *client);

    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);

    void OnWrite_(HttpConn *client);

    static const int MAX_FD = 65536;

    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    char *srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<Timer> timer_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;

};


#endif //MYTINYWEBSERVER_WEBSERVER_H
