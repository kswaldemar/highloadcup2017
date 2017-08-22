//
// Created by valdemar on 13.08.17.
//
#include "WebServer.h"

#include <experimental/filesystem>

#include <fstream>
#include <ctime>

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
    LOG_URGENT("Json parsing done");

    LOG_URGENT("Extract time");
    std::ifstream options(folder + "/options.txt");
    time_t ts;
    options >> ts;
    LOG_INFO("Read timestamp %", ts);
    ret.start_timestamp_ = *std::localtime(&ts);
    LOG_URGENT("Start date %", std::put_time(&ret.start_timestamp_, "%d/%m/%Y %T (%Z)"));

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
    for (const auto &[v_id, v] : visits_) {
        ok = v.location == id;
        ok = ok && (!from_date || v.visited_at > *from_date);
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
                    //LOG_DEBUG("Birth date % > % = %", u.birth_date, ts, ok);
                }
                if (ok && to_age) {
                    auto tm = start_timestamp_;
                    tm.tm_year -= *to_age;
                    ts = std::mktime(&tm);
                    ok = ok && u.birth_date > ts;
                    //auto b_tm = *std::localtime(&u.birth_date);
                    //LOG_DEBUG("Birth date % (%) < % (%) = %, mark %", u.birth_date, std::put_time(&b_tm, "%d/%m/%Y %T (%Z)"), ts, std::put_time(&tm, "%d/%m/%Y %T (%Z)"), ok, (int)v.mark);
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
    for (const auto &[v_id, v] : visits_) {
        ok = v.user == id;
        if (!ok) {
            continue;
        }

        ok = ok && (!from_date || v.visited_at > *from_date);
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
}

pod::User &SimpleDB::user(id_t id) {
    return users_[id];
}

pod::Location &SimpleDB::location(id_t id) {
    return locations_[id];
}

pod::Visit &SimpleDB::visit(id_t id) {
    return visits_[id];
}
