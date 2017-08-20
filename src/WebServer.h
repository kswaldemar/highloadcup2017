#pragma once

#include "PodTypes.h"
#include "SimpleDB.h"
#include "SimpleLog.h"

extern "C" {
#include <website.h>
}

#include <cstdint>

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

    WebServer(const std::string root_dir);

    int reply(ws_request_t *req);

private:
    ReqType match_action(std::string method, char *uri);
    bool create_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen);
    bool update_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen);

    std::vector<size_t> split_params(char *uri);

    std::string msg_;
    SimpleDB db_;
};
