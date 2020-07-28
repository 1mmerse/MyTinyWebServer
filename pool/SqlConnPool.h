//
// Created by 杨成进 on 2020/7/24.
//

#ifndef MYTINYWEBSERVER_SQLCONNPOOL_H
#define MYTINYWEBSERVER_SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <thread>
#include <string>
#include <semaphore.h>
#include <cassert>
#include "../log/Log.h"
#include "threadpool.h"

class SqlConnPool {
public:
    //局部静态变量单例模式
    static SqlConnPool *Instance();

    MYSQL *GetConn();

    void FreeConn(MYSQL *conn);

    int GetFreeConnCount();

    void Init(const char *host, int port,
              const char *user, const char *password,
              const char *dbName, int connSize);

    void closePool();

private:
    SqlConnPool();

    ~SqlConnPool();

    int MAX_CONN_;
    int useCount_;
    int freeCount_;

    std::queue<MYSQL *> connQueue_;
    std::mutex mtx_;
    sem_t semId_;

};


#endif //MYTINYWEBSERVER_SQLCONNPOOL_H
