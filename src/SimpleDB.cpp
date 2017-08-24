//
// Created by valdemar on 13.08.17.
//
#include "RequestHandler.h"

#include <experimental/filesystem>

#include <fstream>
#include <ctime>
#include <json.hpp>

using nlohmann::json;
namespace fs = std::experimental::filesystem;

namespace {

pod::DATA_TYPE type_by_filename(const std::string &filename) {
    if (filename.find("users_") != std::string::npos) {
        return pod::DATA_TYPE::User;
    } else if (filename.find("locations_") != std::string::npos) {
        return pod::DATA_TYPE::Location;
    } else if (filename.find("visits_") != std::string::npos) {
        return pod::DATA_TYPE::Visit;
    } else {
        return pod::DATA_TYPE::None;
    }
}

bool is_valid_gender(std::string_view gender) {
    return gender.size() == 1 && (gender[0] == 'f' || gender[0] == 'm');
}

bool update(pod::User &u, const json &j) {
    size_t cnt = 0;
    size_t fields_cnt = j.size();
    if (auto it = j.find("gender"); it != j.end()) {
        std::string gender = it.value().get<std::string>();
        if (!is_valid_gender(gender)) {
            LOG_DEBUG("Invalid gender %", gender);
            return false;
        }
        u.gender = gender;
        ++cnt;
    }
    if (auto it = j.find("first_name"); it != j.end()) {
        u.first_name = it.value().get<decltype(u.first_name)>();
        ++cnt;
    }
    if (auto it = j.find("last_name"); it != j.end()) {
        u.last_name = it.value().get<decltype(u.last_name)>();
        ++cnt;
    }
    if (auto it = j.find("email"); it != j.end()) {
        u.email = it.value().get<decltype(u.email)>();
        ++cnt;
    }
    if (auto it = j.find("birth_date"); it != j.end()) {
        u.birth_date = it.value().get<decltype(u.birth_date)>();
        ++cnt;
    }
    LOG_DEBUG("Fields count %, handled %", fields_cnt, cnt);
    return cnt == fields_cnt;
}

bool update(pod::Location &l, const json &j) {
    size_t cnt = 0;
    size_t fields_cnt = j.size();
    if (auto it = j.find("place"); it != j.end()) {
        l.place = it.value().get<decltype(l.place)>();
        ++cnt;
    }
    if (auto it = j.find("country"); it != j.end()) {
        l.country = it.value().get<decltype(l.country)>();
        ++cnt;
    }
    if (auto it = j.find("distance"); it != j.end()) {
        l.distance = it.value().get<decltype(l.distance)>();
        ++cnt;
    }
    if (auto it = j.find("city"); it != j.end()) {
        l.city = it.value().get<decltype(l.city)>();
        ++cnt;
    }
    return cnt == fields_cnt;
}

bool update(pod::Visit &v, const json &j, const SimpleDB &db) {
    size_t cnt = 0;
    size_t fields_cnt = j.size();
    if (auto it = j.find("visited_at"); it != j.end()) {
        v.visited_at = it.value().get<decltype(v.visited_at)>();
        ++cnt;
    }
    if (auto it = j.find("location"); it != j.end()) {
        uint32_t loc_id = it.value().get<uint32_t>();
        if (!db.location_exists(loc_id)) {
            return false;
        }
        v.location = loc_id;
        ++cnt;
    }
    if (auto it = j.find("user"); it != j.end()) {
        uint32_t user_id = it.value().get<uint32_t>();
        if (!db.user_exists(user_id)) {
            return false;
        }
        v.user = user_id;
        ++cnt;
    }
    if (auto it = j.find("mark"); it != j.end()) {
        uint8_t mark = it.value().get<uint8_t>();
        if (mark > 5) {
            return false;
        }
        v.mark = mark;
        ++cnt;
    }
    return cnt == fields_cnt;
}



} //anonymous namespace

SimpleDB SimpleDB::from_folder(const std::string &folder) {
    SimpleDB ret;
    json data;
    LOG_URGENT("Starting parsing json from folder %", folder);
    for (const auto &p : fs::directory_iterator(folder)) {
        const auto filename = p.path().filename().string();
        const auto type = type_by_filename(filename);
        if (type == pod::DATA_TYPE::None) {
            LOG_ERROR("invalid json name %", filename);
            continue;
        }
        const std::string file_str = p.path().string();
        std::ifstream in(file_str);
        if (!in.is_open()) {
            LOG_ERROR("Cannot open file %", file_str);
            continue;
        } else {
            LOG_URGENT("Parse file %", filename);
        }
        in >> data;
        if (type == pod::DATA_TYPE::User) {
            for (const auto &uj : data["users"]) {
                pod::User usr = uj;
                ret.users_[usr.id] = usr;
            }
        } else if (type == pod::DATA_TYPE::Location) {
            for (const auto lc : data["locations"]) {
                pod::Location loc = lc;
                ret.locations_[loc.id] = loc;
            }
        } else {
            for (const auto v : data["visits"]) {
                pod::Visit vis = v;
                ret.visits_[vis.id] = v;
            }
        }
    }
    LOG_URGENT("Json parsing done: users = %, locations = %, visits = %",
               ret.users_.size(), ret.locations_.size(), ret.visits_.size());

    LOG_URGENT("Extract time");
    std::ifstream options(folder + "/options.txt");
    time_t ts;
    options >> ts;
    LOG_INFO("Read timestamp %", ts);
    ret.start_timestamp_ = *std::localtime(&ts);
    LOG_URGENT("Start date %", std::put_time(&ret.start_timestamp_, "%d/%m/%Y %T (%Z)"));

    ret.prepare();

    return ret;
}

//Public interface
bool SimpleDB::is_entity_exists(pod::DATA_TYPE type, id_t id) const {
    switch (type) {
        case pod::DATA_TYPE::User:
            return user_exists(id);
        case pod::DATA_TYPE::Location:
            return location_exists(id);
        case pod::DATA_TYPE::Visit:
            return visit_exists(id);
        case pod::DATA_TYPE::None:
            return false;
    }
}

void SimpleDB::get_entity(pod::DATA_TYPE type, id_t id, char *out) const {
    switch (type) {
        case pod::DATA_TYPE::User:
            user_json(id, out);
            break;
        case pod::DATA_TYPE::Location:
            location_json(id, out);
            break;
        case pod::DATA_TYPE::Visit:
            visit_json(id, out);
            break;
        case pod::DATA_TYPE::None:
            break;
    }
}

bool SimpleDB::user_exists(id_t id) const {
    return users_.find(id) != users_.end();
}

bool SimpleDB::location_exists(id_t id) const {
    return locations_.find(id) != locations_.end();
}

bool SimpleDB::visit_exists(id_t id) const {
    return visits_.find(id) != visits_.end();
}

void SimpleDB::user_json(id_t id, char *out) const {
    static const char *format =
        "{\"id\":%u,\"email\":%s,\"first_name\":%s,\"last_name\":%s,\"gender\":%s,\"birth_date\":%d}";
    const auto &u = users_.at(id);
    sprintf(out, format,
            u.id, u.email.c_str(), u.first_name.c_str(), u.last_name.c_str(), u.gender.c_str(), u.birth_date);
}

void SimpleDB::visit_json(id_t id, char *out) const {
    static const char *format =
        "{\"id\":%u,\"location\":%u,\"user\":%u,\"visited_at\":%llu,\"mark\":%d}";
    const auto &v = visits_.at(id);
    sprintf(out, format, v.id, v.location, v.user, v.visited_at, v.mark);
}

void SimpleDB::location_json(id_t id, char *out) const {
    static const char *format =
        "{\"id\":%u,\"place\":%s,\"country\":%s,\"city\":%s,\"distance\":%u}";
    const auto &l = locations_.at(id);
    sprintf(out, format, l.id, l.place.c_str(), l.country.c_str(), l.city.c_str(), l.distance);
}

void SimpleDB::location_average(char *out, id_t id,
                                std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                                std::optional<uint32_t> from_age, std::optional<uint32_t> to_age,
                                std::optional<char> gender) {
    double mean = 0;
    size_t cnt = 0;
    bool ok;
    for (const auto &v : loc2visits_[id]) {
        ok = (!from_date || v->visited_at > *from_date);
        ok = ok && (!to_date   || v->visited_at < *to_date);
        if (from_age || to_age || gender) {
            const auto &u = users_[v->user];
            ok = ok && (!gender || *gender == u.gender[0]);
            if (ok && (from_age || to_age)) {
                time_t ts;
                if (from_age) {
                    auto tm = start_timestamp_;
                    tm.tm_year -= *from_age;
                    ts = std::mktime(&tm);
                    ok = ok && u.birth_date < ts;
                }
                if (ok && to_age) {
                    auto tm = start_timestamp_;
                    tm.tm_year -= *to_age;
                    ts = std::mktime(&tm);
                    ok = ok && u.birth_date > ts;
                }
            }
        }
        if (ok) {
            mean += v->mark;
            ++cnt;
        }
    }
    cnt = std::max<size_t>(cnt, 1);
    mean /= cnt;
    static const uint32_t mul = 100000;
    mean = std::round(mean * mul) / mul;
    sprintf(out, "{\"avg\": %g}", mean);
}

void SimpleDB::user_visits(char *out, id_t id,
                           std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                           std::optional<std::string_view> country, std::optional<uint32_t> to_distance) {
    static const char *format = "{\"mark\":%u,\"visited_at\":%llu,\"place\":%s}";
    static const char start[] = "{\"visits\": [";
    static const char end[] = "]}";

    char *out_it = out;
    memcpy(out_it, start, sizeof(start));
    out_it += sizeof(start);
    bool ok;
    for (const auto &v : u2visits_[id]) {
        ok = (!from_date || v->visited_at > *from_date);
        ok = ok && (!to_date   || v->visited_at < *to_date);

        if (ok) {
            const auto &loc = locations_[v->location];
            if ((country && *country != loc.country) || (to_distance && *to_distance <= loc.distance)) {
                continue;
            }

            if (out_it > out + sizeof(start)) {
                *out_it++ = ',';
            }
            int writed_cnt = sprintf(out_it, format, v->mark, v->visited_at, loc.place.c_str());
            out_it += writed_cnt;
        }
    }
    memcpy(out_it, end, sizeof(end));
}

void SimpleDB::create(const pod::User &usr) {
    users_[usr.id] = usr;
}

void SimpleDB::create(const pod::Location &loc) {
    locations_[loc.id] = loc;
}

void SimpleDB::create(const pod::Visit &vis) {
    visits_[vis.id] = vis;
    const auto *ptr = &visits_[vis.id];
    u2visits_[vis.user].insert(ptr);
    loc2visits_[vis.location].insert(ptr);
}

bool SimpleDB::update(pod::DATA_TYPE type, uint32_t id, char *body, int body_len) {
    auto j = json::parse(body, body + body_len);
    switch (type) {
        case pod::DATA_TYPE::User: {
            auto u = users_[id];
            if (::update(u, j)) {
                users_[id] = u;
                return true;
            }
            return false;
        }
        case pod::DATA_TYPE::Location: {
            auto loc = locations_[id];
            if (::update(loc, j)) {
                locations_[id] = loc;
                return true;
            }
            return false;
        }
        case pod::DATA_TYPE::Visit: {
            auto vis = visits_[id];
            uint32_t old_loc = vis.location;
            uint32_t old_user = vis.user;
            if (!::update(vis, j, *this)) {
                return false;
            }
            visits_[id] = vis;
            const auto *visit_ptr = &visits_[id];
            u2visits_[old_user].erase(visit_ptr);
            u2visits_[vis.user].insert(visit_ptr);
            loc2visits_[old_loc].erase(visit_ptr);
            loc2visits_[vis.location].insert(visit_ptr);
            return true;
        }
        default: return false;
    }
}

void SimpleDB::prepare() {
    const uint32_t max_post_requests = 15000;

    users_.rehash(users_.size() + max_post_requests);
    locations_.rehash(locations_.size() + max_post_requests);
    visits_.rehash(visits_.size() + max_post_requests);

    u2visits_.rehash(users_.size() + max_post_requests);
    loc2visits_.rehash(locations_.size() + max_post_requests);

    for (const auto &[id, v] : visits_) {
        u2visits_[v.user].insert(&v);
        loc2visits_[v.location].insert(&v);
    }
}
