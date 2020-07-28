//
// Created by 杨成进 on 2020/7/24.
//

#include "SqlConnPool.h"

SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool connPool;
    return &connPool;
}

//从连接池中返回一个可用连接，更新使用和空闲连接数
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

//释放当前连接
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
    //创建connSize条连接
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
    //将信号量初始为最大连接数
    sem_init(&semId_, 0, MAX_CONN_);
}

//销毁连接池
void SqlConnPool::closePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    //关闭数据库连接
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
