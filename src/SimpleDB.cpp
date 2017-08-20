//
// Created by valdemar on 13.08.17.
//
#include "WebServer.h"
#include "SimpleDB.h"
#include "SimpleLog.h"

#include <json.hpp>

#include <experimental/filesystem>

#include <fstream>
#include <string_view>
#include <cstdint>

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


SimpleDB SimpleDB::from_json_folder(const std::string &folder) {
    SimpleDB ret;
    json data;
    LOG_INFO("Starting parsing json from folder %", folder);
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
            LOG_INFO("Parse file %", filename);
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
    LOG_INFO("Json parsing done");
    return ret;
}

//Public interface
bool SimpleDB::user_exists(id_t id) {
    return users_.find(id) != users_.end();
}

bool SimpleDB::location_exists(id_t id) {
    return locations_.find(id) != locations_.end();
}

bool SimpleDB::visit_exists(id_t id) {
    return visits_.find(id) != visits_.end();
}

std::string SimpleDB::user_json(id_t id) {
    return nlohmann::json(users_[id]).dump();
}

std::string SimpleDB::visit_json(id_t id) {
    return nlohmann::json(visits_[id]).dump();
}

std::string SimpleDB::location_json(id_t id) {
    return nlohmann::json(locations_[id]).dump();
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
            ok = ok && (!gender || *gender != u.gender[0]);
            //TODO: Add from_age, to_age
        }
        if (ok) {
            mean += v.mark;
            ++cnt;
        }
    }
    cnt = std::max<size_t>(cnt, 1);
    mean /= cnt;
    static const uint32_t mul = 10000;
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

bool SimpleDB::update(const pod::User &usr) {
    if (pod::is_valid(usr)) {
        users_[usr.id] = usr;
        return true;
    }
    return false;
}

bool SimpleDB::update(const pod::Location &loc) {
    if (pod::is_valid(loc)) {
        locations_[loc.id] = loc;
        return true;
    }
    return false;
}

bool SimpleDB::update(const pod::Visit &vis) {
    if (pod::is_valid(vis)) {
        visits_[vis.id] = vis;
        return true;
    }
    return false;
}

bool SimpleDB::is_entity_exists(pod::DATA_TYPE type, id_t id) {
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

std::string SimpleDB::get_entity(pod::DATA_TYPE type, id_t id) {
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
