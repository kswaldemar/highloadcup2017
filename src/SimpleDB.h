//
// Created by valdemar on 13.08.17.
//
#pragma once

#include "PodTypes.h"

#include <string>
#include <vector>
#include <optional>
#include <set>

class SimpleDB {
public:
    using id_t = uint32_t;

    static SimpleDB from_folder(const std::string &folder);

    bool is_entity_exists(pod::DATA_TYPE type, id_t id) const;
    std::string get_entity(pod::DATA_TYPE type, id_t id) const;

    //Get interface
    bool user_exists(id_t id) const;
    bool location_exists(id_t id) const;
    bool visit_exists(id_t id) const;

    std::string user_json(id_t id) const;
    std::string visit_json(id_t id) const;
    std::string location_json(id_t id) const;

    std::string location_average(id_t id, std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                                 std::optional<uint32_t> from_age, std::optional<uint32_t> to_age,
                                 std::optional<char> gender);
    std::string user_visits(id_t id, std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                            std::optional<std::string_view> country, std::optional<uint32_t> to_distance);

    //Post interface
    void create(const pod::User &usr);
    void create(const pod::Location &loc);
    void create(const pod::Visit &loc);

    pod::User &user(id_t id);
    pod::Location &location(id_t id);
    pod::Visit &visit(id_t id);

private:
    void prepare();

    using locations_t = std::map<id_t, pod::Location>;
    using mapping_t = std::map<id_t, std::set<id_t>>;

    SimpleDB() {};

    std::map<id_t, pod::User> users_;
    locations_t locations_;
    std::map<id_t, pod::Visit> visits_;

    mapping_t u2visits_;
    mapping_t loc2visits_;

    std::tm start_timestamp_;
};