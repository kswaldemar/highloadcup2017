#include "WebServer.h"

extern "C" {
#include <website.h>
}

#include <json.hpp>

#include <experimental/string_view>

using nlohmann::json;

namespace {

void send_reply(ws_request_t *req, const char *status_line, const char *data) {
    ws_statusline(req, status_line);
    ws_add_header(req, "Content-Type", "application/json; charset=utf-8");
    ws_add_header(req, "Server", "hlcup-1.0");
    ws_reply_data(req, data, strlen(data));
}

void send_reply(ws_request_t *req, const char *status_line) {
    ws_statusline(req, status_line);
    ws_add_header(req, "Server", "hlcup-1.0");
    ws_reply_data(req, "", 0);
}

} // anonymous namespace

WebServer::WebServer(const std::string root_dir)
    : db_(SimpleDB::from_json_folder(root_dir)) {
}


int WebServer::reply(ws_request_t *req) {
    LOG_INFO("Request %, Method %", req->uri, req->method);
    try {
        auto params = split_params(req->uri);
        ReqType rt = match_action(req->method, req->uri);
        switch (rt.act) {
            case ActionType::ENT_GET:
                reply_entity_get(req, rt.ent_type, rt.ent_id);
                break;
            case ActionType::UPDATE:
                reply_entity_update(req, rt.ent_type, rt.ent_id);
                break;
            case ActionType::CREATE:
                reply_entity_create(req, rt.ent_type);
                break;
            case ActionType::AVERAGE:
                reply_average(req, rt.ent_id);
                break;
            case ActionType::VISITS:
                reply_visits(req, rt.ent_id);
                break;
            case ActionType::NONE: {
                LOG_DEBUG("Looks like nothing for me");
                send_reply(req, st_404);
                break;
            }
        }
    } catch (const std::exception &e) {
        LOG_INFO("Exception: %", e.what());
        send_reply(req, st_400);
    }

    return WS_REPLY_FINISHED;
}

void WebServer::reply_entity_get(ws_request_t *req, pod::DATA_TYPE type, uint32_t id) {
    LOG_DEBUG("Ok, parsed: entity type = %, id = %", static_cast<uint16_t>(type), id);
    if (!db_.is_entity_exists(type, id)) {
        LOG_DEBUG("Does not exists")
        send_reply(req, st_404);
    } else {
        msg_ = db_.get_entity(type, id);
        LOG_DEBUG("Got json % with size %", msg_, msg_.size())
        send_reply(req, st_200, msg_.c_str());
    }
}

void WebServer::reply_entity_create(ws_request_t *req, pod::DATA_TYPE type) {
    if (create_db_entity_from_json(type, req->body, req->bodylen)) {
        send_reply(req, st_200, "{}");
    } else {
        send_reply(req, st_400);
    }
}

void WebServer::reply_entity_update(ws_request_t *req, pod::DATA_TYPE type, uint32_t id) {
    LOG_DEBUG("Update entity id %", id);
    if (!db_.is_entity_exists(type, id)) {
        send_reply(req, st_404);
    } else {
        if (update_db_entity_from_json(type, req->body, req->bodylen)) {
            send_reply(req, st_200, "{}");
        } else {
            send_reply(req, st_400);
        }
    }
}

void WebServer::reply_average(ws_request_t *req, uint32_t id) {
    LOG_DEBUG("Calculate average for location_id %", id);
    send_reply(req, "501 Not implemented");
}

void WebServer::reply_visits(ws_request_t *req, uint32_t id) {
    LOG_DEBUG("Calculate visits for user_id %", id);
    send_reply(req, "501 Not implemented");
}


///////////////////////////////////////////////////////////////////////////////
WebServer::ReqType WebServer::match_action(std::string method, char *uri) {
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
    std::vector<std::string_view> words;
    while (it < params_sign) {
        auto word_end = std::find(it, params_sign, '/');
        words.emplace_back(it, word_end - it);
        it = std::next(word_end, 1);
    }

    bool ok = words.size() == 2 || words.size() == 3;

    //Checking first word
    LOG_DEBUG("First word %", words[0]);
    if (words[0] == "users") {
        ret.ent_type = pod::DATA_TYPE::User;
    } else if (words[0] == "locations") {
        ret.ent_type = pod::DATA_TYPE::Location;
    } else if (words[0] == "visits") {
        ret.ent_type = pod::DATA_TYPE::Visit;
    } else {
        ok = false;
    }

    if (!ok || words.size() == 1) {
        return ret;
    }

    //Checking second word
    LOG_DEBUG("Second word %", words[1]);
    if (words[1] == "new") {
        ret.act = ActionType::CREATE;
    } else {
        for (const char c : words[1]) {
            ok = ok && std::isdigit(c);
        }
        if (ok) {
            ret.ent_id = static_cast<uint32_t>(strtol(words[1].data(), nullptr, 10));
            if (method == "GET") {
                if (words.size() > 2) {
                    if (words[0] == "locations" && words[2] == "avg") {
                        ret.act = ActionType::AVERAGE;
                    } else if (words[0] == "users" && words[2] == "visits") {
                        ret.act = ActionType::VISITS;
                    }
                } else {
                    ret.act = ActionType::ENT_GET;
                }
            } else if (method == "POST" && words.size() == 2) {
                ret.act = ActionType::UPDATE;
            }
        }
    }

    return ret;
}

std::vector<size_t> WebServer::split_params(char *uri) {
    //TODO: Throw error if invalid params, including type checking
    return {};
}

bool WebServer::create_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen) {
    auto j = json::parse(body, body + bodylen);
    if (db_.is_entity_exists(type, j["id"].get<uint32_t>())) {
        return false;
    }

    if (type == pod::DATA_TYPE::User) {
        pod::User d = j;
        db_.update(d);
    } else if (type == pod::DATA_TYPE::Location) {
        pod::Location d = j;
        db_.update(d);
    } else if (type == pod::DATA_TYPE::Visit) {
        pod::Visit d = j;
        db_.update(d);
    }
    return true;
}

bool WebServer::update_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen) {
    auto j = json::parse(body, body + bodylen);
}