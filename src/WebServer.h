#pragma once

#include "PodTypes.h"
#include "SimpleDB.h"
#include "SimpleLog.h"

extern "C" {
#include <website.h>
}

#include <cstdint>

struct my_request_t {
    ws_request_t native;
    char buf[1500];
};


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

    int reply(my_request_t *req);

private:
    using kv_param_t = std::pair<std::string_view, std::string_view>;
    using uri_params_t = std::vector<kv_param_t>;

    void reply_entity_get(my_request_t *req, pod::DATA_TYPE type, uint32_t id);
    void reply_entity_create(my_request_t *req, pod::DATA_TYPE type);
    void reply_entity_update(my_request_t *req, pod::DATA_TYPE type, uint32_t id);
    void reply_average(my_request_t *req, uint32_t id, const uri_params_t &params);
    void reply_visits(my_request_t *req, uint32_t id, const uri_params_t &params);

    ReqType match_action(std::string method, char *uri);
    bool create_db_entity_from_json(pod::DATA_TYPE type, char *body, int bodylen);
    bool update_db_entity_from_json(pod::DATA_TYPE type, uint32_t id, char *body, int bodylen);

    std::optional<std::string_view> get_param_val(const uri_params_t &params, std::string_view key) const;

    uri_params_t split_validate_params(ActionType type, char *uri);

    bool check_param_correct(ActionType type, std::string_view key, std::string_view value);

    SimpleDB db_;
};
