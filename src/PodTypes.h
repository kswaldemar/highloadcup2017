//
// Created by valdemar on 15.08.17.
//
#pragma once

#include <cstdint>
#include <string>

#include <json.hpp>

namespace pod {

enum class DATA_TYPE {
    User,
    Location,
    Visit,
    None
};

struct User {
    uint32_t id;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string gender;
    int64_t birth_date;
};

void to_json(nlohmann::json &j, const User &u);
void from_json(const nlohmann::json &j, User &u);
bool is_valid(const User &u);

struct Location {
    uint32_t id;
    std::string place;
    std::string country; // 50 max
    std::string city; // 50 max
    uint32_t distance;
};

void to_json(nlohmann::json &j, const Location &loc);
void from_json(const nlohmann::json &j, Location &loc);
bool is_valid(const Location &u);

struct Visit {
    uint32_t id;
    uint32_t location;
    uint32_t user;
    uint64_t visited_at;
    uint8_t mark; //0 - 5
};

void to_json(nlohmann::json &j, const Visit &v);
void from_json(const nlohmann::json &j, Visit &v);
bool is_valid(const Visit &u);

}