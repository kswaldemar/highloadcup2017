//
// Created by valdemar on 13.08.17.
//
#pragma once

#include "PodTypes.h"

#include <string>
#include <vector>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>

class SimpleDB {
public:
    using id_t = uint32_t;

    static SimpleDB from_folder(const std::string &folder);

    bool is_entity_exists(pod::DATA_TYPE type, id_t id) const;
    void get_entity(pod::DATA_TYPE type, id_t id, char *out) const;

    //Get interface
    bool user_exists(id_t id) const;
    bool location_exists(id_t id) const;
    bool visit_exists(id_t id) const;

    void user_json(id_t id, char *out) const;
    void visit_json(id_t id, char *out) const;
    void location_json(id_t id, char *out) const;

    void location_average(char *out, id_t id, std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                                 std::optional<uint32_t> from_age, std::optional<uint32_t> to_age,
                                 std::optional<char> gender);
    void user_visits(char *out, id_t id, std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                     std::optional<std::string_view> country, std::optional<uint32_t> to_distance);

    //Post interface
    void create(const pod::User &usr);
    void create(const pod::Location &loc);
    void create(const pod::Visit &loc);

    bool update(pod::DATA_TYPE type, uint32_t id, char *body, int body_len);

private:
    void prepare();

    template<typename T>
    using table = std::unordered_map<id_t, T>;

    using users_mapping_t = std::unordered_map<id_t, std::unordered_set<const pod::Visit*>>;
    using locations_mapping_t = std::unordered_map<id_t, std::unordered_set<const pod::Visit*>>;

    SimpleDB() {};

    table<pod::User> users_;
    table<pod::Location> locations_;
    table<pod::Visit> visits_;

    users_mapping_t u2visits_;
    locations_mapping_t loc2visits_;

    std::tm start_timestamp_;
};