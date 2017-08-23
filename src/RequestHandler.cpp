#include "RequestHandler.h"
#include "SimpleDB.h"

#include "urldecode.h"

extern "C" {
#include <website.h>
}

#include <json.hpp>

#include <string_view>
#include <unordered_set>
#include <optional>

using nlohmann::json;

namespace {

void send_reply(my_request_t *req, const char *status_line, const char *data) {
    ws_statusline(&req->native, status_line);
    ws_add_header(&req->native, "Content-Type", "application/json; charset=utf-8");
    ws_add_header(&req->native, "Server", "hlcup-1.0");
    size_t data_len = strlen(data);
    memcpy(req->buf, data, data_len);
    req->buf[data_len] = '\0';
    ws_reply_data(&req->native, req->buf, data_len);
}

void send_reply(my_request_t *req, const char *status_line) {
    ws_statusline(&req->native, status_line);
    ws_add_header(&req->native, "Server", "hlcup-1.0");
    ws_reply_data(&req->native, "", 0);
}

bool is_uint(std::string_view str) {
    for (const char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

bool is_int(std::string_view str) {
    if (str.empty() || (str[0] != '-' && !isdigit(str[0]))) {
        return false;
    }
    str.remove_prefix(1);
    for (const char c : str) {
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

bool is_double(std::string_view str) {
    if (str.empty() || str[0] == '.' || (str[0] != '-' && !isdigit(str[0]))) {
        return false;
    }
    str.remove_prefix(1);
    bool pt_found = false;
    for (const char c : str) {
        if (!isdigit(c) && (c != '.' || pt_found)) {
            return false;
        }
        pt_found = pt_found || c == '.';
    }
    return true;
}

std::optional<uint32_t> optional_to_uint(const std::optional<std::string_view> &opt) {
    if (opt) {
        return strtoul(opt->data(), nullptr, 10);
    }
    return {};
}

bool is_valid_uri_param(std::string_view val) {
    return val.find('?') == std::string_view::npos;
}

bool is_valid_gender(std::string_view gender) {
    return gender.size() == 1 && (gender[0] == 'f' || gender[0] == 'm');
}

} // anonymous namespace

RequestHandler::RequestHandler(const std::string root_dir)
    : db_(SimpleDB::from_folder(root_dir)) {
}


int RequestHandler::reply(my_request_t *req) {
    auto &native = req->native;
    LOG_INFO("Request %, Method %", native.uri, native.method);

    try {
        ReqType rt = match_action(native.method, native.uri);
        auto params = split_validate_params(rt.act, native.uri);
        std::string params_str;
        for (const auto &[k, v] : params) {
            if (!params_str.empty()) {
                params_str += ", ";
            }
            params_str += k;
            params_str += "=";
            params_str += v;
        }
        LOG_INFO("Get params: [%]", params_str);

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
                reply_average(req, rt.ent_id, params);
                break;
            case ActionType::VISITS:
                reply_visits(req, rt.ent_id, params);
                break;
            case ActionType::NONE: {
                LOG_DEBUG("Looks like nothing for me");
                send_reply(req, st_404);
                break;
            }
        }
    } catch (const std::exception &e) {
        LOG_WARN("Exception: %", e.what());
        send_reply(req, st_400);
    }

    return WS_REPLY_FINISHED;
}

void RequestHandler::reply_entity_get(my_request_t *req, pod::DATA_TYPE type, uint32_t id) {
    LOG_DEBUG("Ok, parsed: entity type = %, id = %", static_cast<uint16_t>(type), id);
    if (!db_.is_entity_exists(type, id)) {
        LOG_DEBUG("Does not exists")
        send_reply(req, st_404);
    } else {
        std::string msg = db_.get_entity(type, id);
        LOG_DEBUG("Got json % with size %", msg, msg.size())
        send_reply(req, st_200, msg.c_str());
    }
}

void RequestHandler::reply_entity_create(my_request_t *req, pod::DATA_TYPE type) {
    if (create_db_entity_from_json(type, req->native.body, req->native.bodylen)) {
        send_reply(req, st_200, "{}");
    } else {
        send_reply(req, st_400);
    }
}

void RequestHandler::reply_entity_update(my_request_t *req, pod::DATA_TYPE type, uint32_t id) {
    LOG_DEBUG("Update entity id %", id);
    if (!db_.is_entity_exists(type, id)) {
        send_reply(req, st_404);
    } else {
        if (update_db_entity_from_json(type, id, req->native.body, req->native.bodylen)) {
            send_reply(req, st_200, "{}");
        } else {
            LOG_DEBUG("Update failed for some reason");
            send_reply(req, st_400);
        }
    }
}

void RequestHandler::reply_average(my_request_t *req, uint32_t id, const uri_params_t &params) {
    LOG_DEBUG("Calculate average for location_id %", id);
    if (!db_.is_entity_exists(pod::DATA_TYPE::Location, id)) {
        send_reply(req, st_404);
    } else {
        std::optional<uint32_t> from_date;
        std::optional<uint32_t> to_date;
        std::optional<uint32_t> from_age;
        std::optional<uint32_t> to_age;
        std::optional<char> gender;

        from_date = optional_to_uint(get_param_val(params, "fromDate"));
        to_date = optional_to_uint(get_param_val(params, "toDate"));
        from_age = optional_to_uint(get_param_val(params, "fromAge"));
        to_age = optional_to_uint(get_param_val(params, "toAge"));

        if (auto val = get_param_val(params, "gender")) {
            gender = val->at(0);
        }

        LOG_DEBUG("Location average filters [fromDate=%, toDate=%, fromAge=%, toAge=%, gender=%]",
                  from_date.value_or(0), to_date.value_or(0),
                  from_age.value_or(0), to_age.value_or(0),
                  gender.value_or('.'));

        std::string msg = db_.location_average(id, from_date, to_date, from_age, to_age, gender);
        send_reply(req, st_200, msg.c_str());
    }
}

void RequestHandler::reply_visits(my_request_t *req, uint32_t id, const uri_params_t &params) {
    LOG_DEBUG("Calculate visits for user_id %", id);
    if (!db_.is_entity_exists(pod::DATA_TYPE::User, id)) {
        send_reply(req, st_404);
    } else {
        std::optional<uint32_t> from_date = optional_to_uint(get_param_val(params, "fromDate"));
        std::optional<uint32_t> to_date = optional_to_uint(get_param_val(params, "toDate"));
        std::optional<uint32_t> to_distance = optional_to_uint(get_param_val(params, "toDistance"));
        std::optional<std::string_view> country = get_param_val(params, "country");

        LOG_DEBUG("User visits filter: [fromDate=%, toDate=%, country=%, toDistance=%]",
                  from_date.value_or(0), to_date.value_or(0), country.value_or("None"), to_distance.value_or(0));

        std::string msg = db_.user_visits(id, from_date, to_date, country, to_distance);
        send_reply(req, st_200, msg.c_str());
    }
}

std::optional<std::string_view> RequestHandler::get_param_val(const RequestHandler::uri_params_t &params,
                                                              std::string_view key) const {

    if (auto it = find_if(params.begin(), params.end(), [&key](const kv_param_t &kv) { return kv.first == key; });
        it != params.end()) {

        return it->second;
    }
    return {};
}


///////////////////////////////////////////////////////////////////////////////
RequestHandler::ReqType RequestHandler::match_action(std::string method, char *uri) {
    ReqType ret;
    ret.act = ActionType::NONE;

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

RequestHandler::uri_params_t RequestHandler::split_validate_params(ActionType type, char *uri) {
    size_t uri_len = strlen(uri);
    const auto uri_end = uri + uri_len;
    auto start_it = std::find(uri, uri_end, '?');
    if (start_it == uri_end) {
        return {};
    }

    uri_params_t ret;
    ++start_it;
    while (start_it != uri_end) {
        auto eq = std::find(start_it, uri_end, '=');
        if (eq != uri_end) {
            *eq++ = '\0';
        }

        if (eq == uri_end) {
            throw std::logic_error("Bad uri parameters, missing value");
        }

        auto next = std::find(eq, uri_end, '&');
        if (next != uri_end) {
            *next++ = '\0';
        }

        if (!is_valid_uri_param(eq)) {
            throw std::logic_error("Value contains forbidden symbol");
        }

        dlib::inplace_urldecode(eq);

        if (!check_param_correct(type, start_it, eq)) {
            throw std::logic_error("Invalid parameter or value");
        }

        ret.emplace_back(start_it, eq);
        start_it = next;
    }
    if (ret.empty()) {
        throw std::logic_error("Only '?' sign was found");
    }
    return ret;
}

bool RequestHandler::create_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen) {
    auto j = json::parse(body, body + bodylen);
    uint32_t id = j["id"].get<uint32_t>();
    if (db_.is_entity_exists(type, id)) {
        LOG_DEBUG("Already exists");
        return false;
    }
    if (type == pod::DATA_TYPE::User) {
        pod::User d = j;
        if (!is_valid_gender(d.gender)) {
            LOG_DEBUG("Invalid gender '%'", d.gender)
            return false;
        }
        db_.create(d);
    } else if (type == pod::DATA_TYPE::Location) {
        pod::Location d = j;
        db_.create(d);
    } else if (type == pod::DATA_TYPE::Visit) {
        pod::Visit d = j;
        if (!db_.location_exists(d.location) || !db_.user_exists(d.user) || d.mark > 5) {
            LOG_DEBUG("No corresponding location or user, mark = %", d.mark);
            return false;
        }
        db_.create(d);
    }
    return true;
}

bool RequestHandler::update_db_entity_from_json(pod::DATA_TYPE type, uint32_t id, char *body, int bodylen) {
    if (!db_.is_entity_exists(type, id)) {
        return false;
    }

    if (!db_.update(type, id, body, bodylen)) {
        return false;
    }
    return true;
}

bool RequestHandler::check_param_correct(ActionType type, std::string_view key, std::string_view value) {
    LOG_DEBUG("Validate params pair '%=%'", key, value);

    if (type == ActionType::ENT_GET) {
        return false;
    }

    bool ok;
    switch (type) {
        case ActionType::CREATE:
        case ActionType::UPDATE:
            ok = key == "query_id";
            break;
        case ActionType::AVERAGE: {
            ok = key == "fromDate" || key == "toDate" || key == "fromAge" || key == "toAge" || key == "gender";
            ok = ok && (key != "fromDate" || is_uint(value));
            ok = ok && (key != "toDate" || is_uint(value));
            ok = ok && (key != "fromAge" || is_uint(value));
            ok = ok && (key != "toAge" || is_uint(value));
            ok = ok && (key != "gender" || is_valid_gender(value));
            break;
        }
        case ActionType::VISITS: {
            ok = key == "fromDate" || key == "toDate" || key == "country" || key == "toDistance";
            ok = ok && (key != "fromDate" || is_uint(value));
            ok = ok && (key != "toDate" || is_uint(value));
            ok = ok && (key != "toDistance" || is_double(value));
            break;
        }
        default:
            return false;
    }
    return ok;
}