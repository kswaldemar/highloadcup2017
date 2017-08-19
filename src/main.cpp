#include "SimpleLog.h"
#include "SimpleDB.h"


#include <ev.h>

extern "C" {
#include <website.h>
}

constexpr uint16_t PORT = 8080;
const std::string JSON_FOLDER = "/home/valdemar/Development/Projects/highloadcup2017/data";

class WebServer {
public:
    enum class ActionType : uint8_t {
        ENT_GET,
        UPDATE,
        CREATE,
        NONE
    };

    struct ReqType {
        ActionType act;
        uint32_t ent_id;
        pod::DATA_TYPE ent_type;
    };

    int reply(ws_request_t *req) {
        static const char *st_404 = "404 Not found";
        static const char *st_200 = "200 Ok";
        static const char *st_400 = "400 You are wrong";

        LOG_INFO("Request %, Method %", req->uri, req->method);
        try {
            auto params = split_params(req->uri);
            ReqType rt = match_action(req->method, req->uri);
            if (rt.act == ActionType::NONE) {
                LOG_DEBUG("Looks like nothing for me");
                send_reply(req, st_404);
            } else if (rt.act == ActionType::ENT_GET) {
                LOG_DEBUG("Ok, parsed: entity type = %, id = %", static_cast<uint16_t>(rt.ent_type), rt.ent_id);
                if (!db_.is_entity_exists(rt.ent_type, rt.ent_id)) {
                    LOG_DEBUG("Does not exists")
                    send_reply(req, st_404);
                } else {
                    msg_ = db_.get_entity(rt.ent_type, rt.ent_id);
                    LOG_DEBUG("Got json % with size %", msg_, msg_.size())
                    send_reply(req, st_200, msg_.c_str());
                }
            } else {
                send_reply(req, "501 Not implemented");
            }
        } catch (...) {
            send_reply(req, st_400, "{}");
        }

        return WS_REPLY_FINISHED;
    }

private:
    static void send_reply(ws_request_t *req, const char *status_line, const char *data) {
        ws_statusline(req, status_line);
        ws_add_header(req, "Content-Type", "application/json; charset=utf-8");
        ws_reply_data(req, data, strlen(data));
    }

    static void send_reply(ws_request_t *req, const char *status_line) {
        ws_statusline(req, status_line);
        ws_reply_data(req, "", 0);
    }


    ReqType match_action(std::string method, char *uri) {
        ReqType ret;
        ret.act = ActionType::NONE;

        //Normalize uri
        size_t uri_len = strlen(uri);
        if (uri[uri_len - 1] == '/') {
            uri[uri_len - 1] = '\0';
        }

        const char *it = uri;
        const char *uri_end = uri + strlen(uri);

        if (it[0] == '/') {
            ++it;
        }

        auto params_sign = std::find(it, uri_end, '?');
        auto first_word_end = std::find(it, params_sign, '/');

        bool ok = true;
        std::string first_word(it, first_word_end - it);
        LOG_DEBUG("First word %", first_word);
        if (first_word == "users") {
            ret.ent_type = pod::DATA_TYPE::User;
        } else if (first_word == "locations") {
            ret.ent_type = pod::DATA_TYPE::Location;
        } else if (first_word == "visits") {
            ret.ent_type = pod::DATA_TYPE::Visit;
        } else {
            ok = false;
        }

        if (!ok) {
            return ret;
        }

        std::string second_word(first_word_end + 1, params_sign - first_word_end - 1);
        LOG_DEBUG("Second word %", second_word);
        if (second_word == "new") {
            ret.act = ActionType::CREATE;
        } else {
            for (const char c : second_word) {
                ok = ok && std::isdigit(c);
            }
            if (ok) {
                if (method == "GET") {
                    ret.act = ActionType::ENT_GET;
                } else if (method == "POST") {
                    ret.act = ActionType::UPDATE;
                }
                ret.ent_id = static_cast<uint32_t>(strtol(second_word.c_str(), nullptr, 10));
            }
        }

        return ret;
    }

    std::vector<size_t> split_params(char *uri) {
        //TODO: Throw if invalid params
        return {};
    }

    std::string msg_;
    SimpleDB db_ = SimpleDB::from_json_folder(JSON_FOLDER);
};


static WebServer g_handler;

int reply(ws_request_t *req) {
    return g_handler.reply(req);
}

int main() {
    ws_server_t serv;
    ws_quickstart(&serv, "127.0.0.1", PORT, reply);
    LOG_INFO("Starting webserver on port %", PORT);
    ev_loop(ev_default_loop(0), 0);
}
