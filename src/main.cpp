#include "SimpleLog.h"
#include "SimpleDB.h"
#include "RequestHandler.h"

#include <arpa/inet.h>

#ifndef NDEBUG
constexpr uint16_t PORT = 8080;
#else
constexpr uint16_t PORT = 80;
#endif

const std::string JSON_FOLDER = "./data";

static RequestHandler g_handler(JSON_FOLDER);

int reply(my_request_t *req) {
    return g_handler.reply(req);
}

int main() {
    struct ev_loop *loop = ev_default_loop(0);
    ws_server_t serv;
    ws_server_init(&serv, loop);
    if (ws_add_tcp(&serv, inet_addr("0.0.0.0"), PORT) < 0) {
        perror("Error binding port");
    }
    LOG_URGENT("Starting webserver on port %", PORT);
    ws_REQUEST_STRUCT(&serv, my_request_t);
    ws_REQUEST_CB(&serv, reply);

    ws_server_start(&serv);
    ev_loop(loop, 0);
}
