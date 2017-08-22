//
// Created by valdemar on 13.08.17.
//
#include "WebServer.h"

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

std::string SimpleDB::get_entity(pod::DATA_TYPE type, id_t id) const {
    switch (type) {
        case pod::DATA_TYPE::User:
            return user_json(id);
        case pod::DATA_TYPE::Location:
            return location_json(id);
        case pod::DATA_TYPE::Visit:
            return visit_json(id);
        case pod::DATA_TYPE::None:
            return "";
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

std::string SimpleDB::user_json(id_t id) const {
    return nlohmann::json(users_.at(id)).dump();
}

std::string SimpleDB::visit_json(id_t id) const {
    return nlohmann::json(visits_.at(id)).dump();
}

std::string SimpleDB::location_json(id_t id) const {
    return nlohmann::json(locations_.at(id)).dump();
}

std::string SimpleDB::location_average(id_t id,
                                       std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                                       std::optional<uint32_t> from_age, std::optional<uint32_t> to_age,
                                       std::optional<char> gender) {
    double mean = 0;
    size_t cnt = 0;
    bool ok;
    for (auto v_id : loc2visits_[id]) {
        const auto &v = visits_[v_id];
        ok = (!from_date || v.visited_at > *from_date);
        ok = ok && (!to_date   || v.visited_at < *to_date);
        if (from_age || to_age || gender) {
            const auto &u = users_[v.user];
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
            mean += v.mark;
            ++cnt;
        }
    }
    cnt = std::max<size_t>(cnt, 1);
    mean /= cnt;
    static const uint32_t mul = 100000;
    mean = std::round(mean * mul) / mul;
    char buf[30] = {};
    sprintf(buf, "{\"avg\": %g}", mean);

    return buf;
}

std::string SimpleDB::user_visits(id_t id,
                                  std::optional<uint32_t> from_date, std::optional<uint32_t> to_date,
                                  std::optional<std::string_view> country, std::optional<uint32_t> to_distance) {
    struct desc_t {
        desc_t(uint8_t mark, size_t visited_at, const char *place_ptr)
            : mark(mark),
              visited_at(visited_at),
              place_ptr(place_ptr) {
        }

        uint8_t mark;
        size_t visited_at;
        const char *place_ptr;
    };

    std::vector<desc_t> values;
    bool ok;
    for (id_t v_id : u2visits_[id]) {
        const auto &v = visits_[v_id];

        ok = (!from_date || v.visited_at > *from_date);
        ok = ok && (!to_date   || v.visited_at < *to_date);

        if (ok) {
            const auto &loc = locations_[v.location];
            if ((country && *country != loc.country) || (to_distance && *to_distance <= loc.distance)) {
                continue;
            }
            values.emplace_back(v.mark, v.visited_at, loc.place.c_str());
        }
    }
    if (values.empty()) {
        return "{\"visits\": []}";
    }

    std::sort(values.begin(), values.end(), [](const desc_t &lhs, const desc_t &rhs) {
        return lhs.visited_at < rhs.visited_at;
    });
    json ar;
    for (const auto &d : values) {
        ar.push_back({{"mark", d.mark}, {"visited_at", d.visited_at}, {"place", d.place_ptr}});
    }
    return "{\"visits\": " + ar.dump() + "}";
}

void SimpleDB::create(const pod::User &usr) {
    users_[usr.id] = usr;
}

void SimpleDB::create(const pod::Location &loc) {
    locations_[loc.id] = loc;
}

void SimpleDB::create(const pod::Visit &vis) {
    visits_[vis.id] = vis;
    u2visits_[vis.user].insert(vis.id);
    loc2visits_[vis.location].insert(vis.id);
}

bool SimpleDB::update(pod::DATA_TYPE type, uint32_t id, char *body, int body_len) {
    auto j = json::parse(body, body + body_len);
    switch (type) {
        case pod::DATA_TYPE::User: {
            auto &u = users_[id];
            return ::update(u, j);
        }
        case pod::DATA_TYPE::Location: {
            auto &loc = locations_[id];
            return ::update(loc, j);
        }
        case pod::DATA_TYPE::Visit: {
            auto &vis = visits_[id];
            uint32_t old_loc = vis.location;
            uint32_t old_user = vis.user;
            if (!::update(vis, j, *this)) {
                return false;
            }
            u2visits_[old_user].erase(vis.id);
            u2visits_[vis.user].insert(vis.id);
            loc2visits_[old_loc].erase(vis.id);
            loc2visits_[vis.location].insert(vis.id);
            return true;
        }
        default: return false;
    }
}

void SimpleDB::prepare() {
    for (const auto &[id, v] : visits_) {
        u2visits_[v.user].insert(id);
        loc2visits_[v.location].insert(id);
    }
}
