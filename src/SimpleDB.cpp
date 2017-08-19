//
// Created by valdemar on 13.08.17.
//

#include "SimpleDB.h"
#include "SimpleLog.h"

#include <json.hpp>

#include <experimental/filesystem>

#include <fstream>
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
        case pod::DATA_TYPE::User: return user_exists(id);
        case pod::DATA_TYPE::Location: return location_exists(id);
        case pod::DATA_TYPE::Visit: return visit_exists(id);
        case pod::DATA_TYPE::None: return false;
    }
}

std::string SimpleDB::get_entity(pod::DATA_TYPE type, id_t id) {
    switch (type) {
        case pod::DATA_TYPE::User: return user_json(id);
        case pod::DATA_TYPE::Location: return location_json(id);
        case pod::DATA_TYPE::Visit: return visit_json(id);
        case pod::DATA_TYPE::None: return "";
    }
}


