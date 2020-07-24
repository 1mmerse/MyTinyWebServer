//
// Created by 杨成进 on 2020/7/24.
//

#include "SqlConnPool.h"
#include "../log/Log.h"

SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

MYSQL *SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if (connQueue_.empty()) {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        sql = connQueue_.front();
        connQueue_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL *conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(mtx_);
    connQueue_.push(conn);
    sem_post(&semId_);
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> locker(mtx_);
    return connQueue_.size();
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *password, const char *dbName,
                       int connSize) {
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySQL init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
                                 user, password,
                                 dbName, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySQL connect error!");
        }
        connQueue_.push(sql);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

void SqlConnPool::closePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQueue_.empty()) {
        auto item = connQueue_.front();
        connQueue_.pop();
        mysql_close(item);
    }
    mysql_library_end;
}

SqlConnPool::SqlConnPool() : useCount_(0), freeCount_(0) {
}

SqlConnPool::~SqlConnPool() {
    closePool();
}
