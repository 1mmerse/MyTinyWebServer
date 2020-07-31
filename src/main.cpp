//
// Created by 杨成进 on 2020/7/24.
//

#include <unistd.h>
#include "server/WebServer.h"

int main() {

    WebServer server(
            1234, 3, 5000, true,
            3306, "root", "ycj0307..", "webserver",
            12, 6, true, 0, 1024);
    server.start();
    return 0;
}
