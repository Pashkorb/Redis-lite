#include "database.hpp"

void Database::set(const std::string& key, const std::string& value) {
    data[key] = value;
}

std::string Database::get(const std::string& key) {
    return data.count(key) ? data[key] : "(nil)";
}


bool Database::del(const std::string& key){
    return data.erase(key)>0;

}
bool Database::exists(const std::string& key){
    return data.count(key)>0;
}