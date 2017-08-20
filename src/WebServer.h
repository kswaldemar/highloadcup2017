#pragma once

#include "PodTypes.h"
#include "SimpleDB.h"
#include "SimpleLog.h"

extern "C" {
#include <website.h>
}

#include <cstdint>

class WebServer {
    static constexpr const char *st_404 = "404 Not found";
    static constexpr const char *st_200 = "200 Ok";
    static constexpr const char *st_400 = "400 You are wrong";
public:
    enum class ActionType : uint8_t {
        ENT_GET,
        UPDATE,
        CREATE,
        AVERAGE,
        VISITS,
        NONE
    };

    struct ReqType {
        ActionType act;
        uint32_t ent_id;
        pod::DATA_TYPE ent_type;
    };

    WebServer(const std::string root_dir);

    int reply(ws_request_t *req);

private:
    void reply_entity_get(ws_request_t *req, pod::DATA_TYPE type, uint32_t id);
    void reply_entity_create(ws_request_t *req, pod::DATA_TYPE type);
    void reply_entity_update(ws_request_t *req, pod::DATA_TYPE type, uint32_t id);
    void reply_average(ws_request_t *req, uint32_t id);
    void reply_visits(ws_request_t *req, uint32_t id);

    ReqType match_action(std::string method, char *uri);
    bool create_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen);
    bool update_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen);

    std::vector<size_t> split_params(char *uri);

    std::string msg_;
    SimpleDB db_;
};
