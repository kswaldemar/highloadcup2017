//
// Created by valdemar on 13.08.17.
//
#pragma once

#include "PodTypes.h"

#include <string>
#include <vector>
#include <optional>

class SimpleDB {
public:
    using id_t = uint32_t;

    static SimpleDB from_folder(const std::string &folder);

    bool is_entity_exists(pod::DATA_TYPE type, id_t id);
    std::string get_entity(pod::DATA_TYPE type, id_t id);

    //Get interface
    bool user_exists(id_t id);
    bool location_exists(id_t id);
    bool visit_exists(id_t id);

    std::string user_json(id_t id);
    std::string visit_json(id_t id);
    std::string location_json(id_t id);

    std::string location_average(id_t id, std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                                 std::optional<uint32_t> from_age, std::optional<uint32_t> to_age,
                                 std::optional<char> gender);
    std::string user_visits(id_t id, std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                            std::optional<std::string_view> country, std::optional<uint32_t> to_distance);

    //Post interface
    bool update(const pod::User &usr);
    bool update(const pod::Location &loc);
    bool update(const pod::Visit &loc);

private:
    using locations_t = std::map<id_t, pod::Location>;

    SimpleDB() {};

    std::map<id_t, pod::User> users_;
    locations_t locations_;
    std::map<id_t, pod::Visit> visits_;
    std::tm start_timestamp_;
};