//
// Created by valdemar on 15.08.17.
//

#include "PodTypes.h"

#include <json.hpp>

using nlohmann::json;

namespace pod {

//User
void to_json(nlohmann::json &j, const User &u) {
    j = json{
        {"id", u.id},
        {"email", u.email},
        {"first_name", u.first_name},
        {"last_name", u.last_name},
        {"gender", u.gender},
        {"birth_date", u.birth_date}
    };
}

void from_json(const nlohmann::json &j, User &u) {
    u.id = j.at("id").get<uint32_t>();
    u.email = j.at("email").get<std::string>();
    u.first_name = j.at("first_name").get<std::string>();
    u.last_name = j.at("last_name").get<std::string>();
    u.gender = j.at("gender").get<std::string>();
    u.birth_date = j.at("birth_date").get<time_t>();
}

bool is_valid(const User &u) {
    return u.gender == "m" || u.gender == "f";
}

//Location
void to_json(nlohmann::json &j, const Location &loc) {
    j = nlohmann::json{{"id",       loc.id},
                       {"place",    loc.place},
                       {"country",  loc.country},
                       {"city",     loc.city},
                       {"distance", loc.distance}};
}

void from_json(const nlohmann::json &j, Location &loc) {
    loc.id = j.at("id").get<uint32_t>();
    loc.place = j.at("place").get<std::string>();
    loc.country = j.at("country").get<std::string>();
    loc.city = j.at("city").get<std::string>();
    loc.distance = j.at("distance").get<uint32_t>();
}

bool is_valid(const Location &l) {
    return true;
}

//Visit
void to_json(nlohmann::json &j, const Visit &v) {
    j = nlohmann::json{{"id",         v.id},
                       {"location",   v.location},
                       {"user",       v.user},
                       {"visited_at", v.visited_at},
                       {"mark",       v.mark}};
}

void from_json(const nlohmann::json &j, Visit &v) {
    v.id = j.at("id").get<uint32_t>();
    v.location = j.at("location").get<uint32_t>();
    v.user = j.at("user").get<uint32_t>();
    v.visited_at = j.at("visited_at").get<uint64_t>();
    v.mark = j.at("mark").get<uint8_t>();
}

bool is_valid(const Visit &v) {
    return v.mark <= 5;
}


}
