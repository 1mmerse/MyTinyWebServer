//
// Created by 杨成进 on 2020/7/24.
//

#ifndef MYTINYWEBSERVER_SQLCONNRAII_H
#define MYTINYWEBSERVER_SQLCONNRAII_H
#include "SqlConnPool.h"

class SqlConnRAII{
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* connPool){
        assert(connPool);
        *sql = connPool->GetConn();
        sql_ = *sql;
        connPool_ = connPool;
    }

    ~SqlConnRAII(){
        if(sql_){
            connPool_->FreeConn(sql_);
        }
    }

private:
    MYSQL *sql_;
    SqlConnPool* connPool_;
};
#endif //MYTINYWEBSERVER_SQLCONNRAII_H
