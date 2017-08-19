#include "SimpleLog.h"
#include "SimpleDB.h"
#include "WebServer.h"

constexpr uint16_t PORT = 8080;
const std::string JSON_FOLDER = "/home/valdemar/Development/Projects/highloadcup2017/data";

static WebServer g_handler(JSON_FOLDER);

int reply(ws_request_t *req) {
    return g_handler.reply(req);
}

int main() {
    ws_server_t serv;
    ws_quickstart(&serv, "127.0.0.1", PORT, reply);
    LOG_INFO("Starting webserver on port %", PORT);
    ev_loop(ev_default_loop(0), 0);
}
